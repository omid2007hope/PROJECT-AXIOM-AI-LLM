#include "../include/ai.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

#include "../include/parser.hpp"
#include "../include/transfer.hpp"


AI::AI(size_t input_dimension,
       size_t output_dimension,
       float weight_min,
       float weight_max,
       Elements& elements)
    : input_dimension(input_dimension),
      output_dimension(output_dimension),
      weight_min(weight_min),
      weight_max(weight_max) {

	// input_dimension = token * dim = 512
	// output_dimension = target * dim = 4

	// 512 * 4 = 2048
	elements.weight_elements = input_dimension * output_dimension;

	// output_dimension = 4
	elements.bias_elements = output_dimension;

	host_weights.resize(elements.weight_elements);
	host_bias.resize(elements.bias_elements);

	vocab_size = 65536;
	embedding_dim = output_dimension;
	if (embedding_dim == 0)
		embedding_dim = 4;

	host_embeddings.resize(vocab_size * embedding_dim);

	reset_weights(elements);
	if (allocate_gpu_embeddings(&gpu_embeddings, vocab_size, embedding_dim)) {
		download_embeddings(host_embeddings.data(), &gpu_embeddings);
	}
}

void AI::reset_weights(Elements& elements) {
	elements.weight_elements = input_dimension * output_dimension;
	elements.bias_elements = output_dimension;

	// Initialize with Chaos (Symmetry Breaking)
	// We use the Mersenne Twister engine for high-quality random numbers
	std::random_device rd;
	std::mt19937 gen(rd());

	// Uniform distribution between weight_min and weight_max
	std::uniform_real_distribution<float> dis(weight_min, weight_max);

	for (size_t i = 0; i < elements.weight_elements; ++i) {
		host_weights[i] = dis(gen);
	}

	for (size_t i = 0; i < elements.bias_elements; ++i) {
		// Biases can often safely start at 0.0, because the chaotic weights
		// will do the heavy lifting to break the symmetry on the first pass.
		host_bias[i] = 0.0f;
	}

	float bound = 1.0f / sqrtf(static_cast<float>(embedding_dim));
	std::uniform_real_distribution<float> dis_emb(-bound, bound);
	for (float& val : host_embeddings) {
		val = dis_emb(gen);
	}
	std::fill(
		host_embeddings.begin(), host_embeddings.begin() + embedding_dim, 0.0f); // PAD row

	if (gpu_embeddings.d_table_master != nullptr) {
		upload_embeddings(host_embeddings.data(), &gpu_embeddings);
	}

	std::cout << "[AI Brain] Weights, Biases, and Embeddings reset to chaotic state.\n"
		  << std::flush;
}

AI::~AI() {
	free_training_graph_resources();
	free_paged_cache_memory();
	free_gpu(&gpu_portion);
	free_gpu_weights(&gpu_weights);
	free_gpu_activations(&gpu_activations);
	free_gpu_embeddings(&gpu_embeddings);
}

bool AI::upload_weights() {
	bool ok = Transfer::upload_weights(
		host_weights, host_bias, pending_steps, gpu_weights, gpu_activations);
	if (!ok)
		return false;

	if (gpu_embeddings.d_table != nullptr && !host_embeddings.empty()) {
		if (!Transfer::upload_embeddings(host_embeddings, gpu_embeddings)) {
			return false;
		}
	}
	return true;
}

bool AI::download_weights() {
	bool ok = Transfer::download_weights(host_weights, host_bias, gpu_weights);
	if (!ok)
		return false;

	if (gpu_embeddings.d_table_master != nullptr && !host_embeddings.empty()) {
		if (!Transfer::download_embeddings(host_embeddings, gpu_embeddings)) {
			return false;
		}
	}
	return true;
}

bool AI::read_loss(double& mean_loss, size_t& steps) {
	float loss_sum = 0.0f;

	if (!download_loss(&gpu_activations, &loss_sum)) {
		return false;
	}

	steps = pending_steps;
	pending_steps = 0;
	mean_loss = (steps > 0) ? static_cast<double>(loss_sum) / static_cast<double>(steps) : 0.0;

	return true;
}

TrainStatus AI::train(Optimizer& optimizer, Portion& portion) {
	// Sync the stream to ensure the GPU is done with this buffer before we
	// overwrite it.
	synchronize_stream(&gpu_portion, portion.buffer_index);

	bool has_data = optimizer.load_context_window(portion);

	if (!has_data) {
		return TrainStatus::NoData;
	}

	int buffer_index = portion.buffer_index;
	float learning_rate = optimizer.learning_rate;

	if (portion.has_token_ids && portion.context_token_ids[buffer_index] != nullptr &&
	    gpu_embeddings.d_table != nullptr) {
		// Transfer only 128 uint32_t token IDs across PCIe per context window (512 bytes!)
		if (!upload_tokens_to_gpu(portion.context_token_ids[buffer_index],
					  portion.context_window_size,
					  portion.target_tokens[buffer_index],
					  portion.target_elements,
					  &gpu_portion,
					  buffer_index)) {
			std::cerr << "[AI Brain] Failed to upload tokens to GPU.\n";
			return TrainStatus::Failed;
		}

		// Launch CUDA gather kernel + CUTLASS EVT GEMM forward pass
		gpu_embeddings.active_vocab_size = active_vocab_size;
		if (!forward_pass_with_embeddings(&gpu_portion,
						  &gpu_embeddings,
						  &gpu_weights,
						  &gpu_activations,
						  input_dimension,
						  output_dimension,
						  buffer_index)) {
			std::cerr << "[AI Brain] Forward pass with embeddings failed.\n";
			return TrainStatus::Failed;
		}

		// Launch CUDA fused AdamW + backward scatter embedding update
		if (!backward_pass_with_embeddings(&gpu_portion,
						   &gpu_embeddings,
						   &gpu_weights,
						   &gpu_activations,
						   input_dimension,
						   output_dimension,
						   learning_rate,
						   buffer_index,
						   true /* use_adamw */)) {
			std::cerr << "[AI Brain] Backward pass with embeddings failed.\n";
			return TrainStatus::Failed;
		}
	} else {
		// Fallback for float-based training
		if (!upload_to_gpu(portion.context_window_tokens[buffer_index],
				   portion.all_elements,
				   portion.target_tokens[buffer_index],
				   portion.target_elements,
				   &gpu_portion,
				   buffer_index)) {
			std::cerr << "[AI Brain] Failed to upload portion to GPU.\n";
			return TrainStatus::Failed;
		}

		if (!forward_pass(&gpu_portion,
				  &gpu_weights,
				  &gpu_activations,
				  input_dimension,
				  output_dimension,
				  buffer_index)) {
			std::cerr << "[AI Brain] Forward pass failed.\n";
			return TrainStatus::Failed;
		}

		if (!backward_pass(&gpu_portion,
				   &gpu_weights,
				   &gpu_activations,
				   input_dimension,
				   output_dimension,
				   learning_rate,
				   buffer_index)) {
			std::cerr << "[AI Brain] Backward pass failed.\n";
			return TrainStatus::Failed;
		}
	}

	// Slide to the next buffer for the next training step
	portion.buffer_index = 1 - portion.buffer_index;
	++pending_steps;

	return TrainStatus::Trained;
}

bool AI::save_weights(const std::string& path) {
	// 1. Synchronize CUDA streams to ensure in-flight work completes
	synchronize_stream(&gpu_portion, 0);
	synchronize_stream(&gpu_portion, 1);

	// 2. Download latest trained weights and embeddings from GPU if allocated
	if (gpu_weights.d_weights_master != nullptr) {
		if (!download_weights()) {
			std::cerr
				<< "[AI Brain] Failed to download weights from GPU before saving.\n";
			return false;
		}
	}

	// 3. Populate binary header
	AxiomWeightHeader header {};
	std::memcpy(header.magic, "AXIOM_V1", 8);
	header.input_dimension = input_dimension;
	header.output_dimension = output_dimension;
	header.weight_count = host_weights.size();
	header.bias_count = host_bias.size();
	header.vocab_size = vocab_size;
	header.embedding_dim = embedding_dim;

	if (header.weight_count != input_dimension * output_dimension ||
	    header.bias_count != output_dimension) {
		std::cerr << "[AI Brain] Weight or bias dimension mismatch during serialization.\n";
		return false;
	}

	for (float w : host_weights) {
		if (!std::isfinite(w)) {
			std::cerr
				<< "[AI Brain] Refusing to save corrupt checkpoint: weights contain NaN or Inf.\n";
			return false;
		}
	}
	for (float b : host_bias) {
		if (!std::isfinite(b)) {
			std::cerr
				<< "[AI Brain] Refusing to save corrupt checkpoint: biases contain NaN or Inf.\n";
			return false;
		}
	}
	for (float e_val : host_embeddings) {
		if (!std::isfinite(e_val)) {
			std::cerr
				<< "[AI Brain] Refusing to save corrupt checkpoint: embeddings contain NaN or Inf.\n";
			return false;
		}
	}

	// 4. Atomic write: write to temporary file first (.tmp)
	std::filesystem::path target_p(path);
	if (!target_p.parent_path().empty()) {
		std::error_code dir_ec;
		std::filesystem::create_directories(target_p.parent_path(), dir_ec);
	}

	std::string temp_path = path + ".tmp";
	std::ofstream out(temp_path, std::ios::binary | std::ios::trunc);
	if (!out.is_open()) {
		std::cerr << "[AI Brain] Failed to open temporary file for writing: " << temp_path
			  << "\n";
		return false;
	}

	out.write(reinterpret_cast<const char*>(&header), sizeof(header));
	out.write(reinterpret_cast<const char*>(host_weights.data()),
		  header.weight_count * sizeof(float));
	out.write(reinterpret_cast<const char*>(host_bias.data()),
		  header.bias_count * sizeof(float));
	out.write(reinterpret_cast<const char*>(host_embeddings.data()),
		  header.vocab_size * header.embedding_dim * sizeof(float));

	out.flush();
	if (!out.good()) {
		std::cerr << "[AI Brain] Stream error while writing weights to: " << temp_path
			  << "\n";
		out.close();
		std::error_code ec;
		std::filesystem::remove(temp_path, ec);
		return false;
	}
	out.close();

	// 5. Atomically replace the target file
	std::error_code ec;
	if (std::filesystem::exists(path, ec)) {
		std::filesystem::remove(path, ec);
	}
	std::filesystem::rename(temp_path, path, ec);
	if (ec) {
		ec.clear();
		std::filesystem::copy_file(
			temp_path, path, std::filesystem::copy_options::overwrite_existing, ec);
		std::filesystem::remove(temp_path, ec);
		if (ec) {
			std::cerr << "[AI Brain] Failed to rename/copy " << temp_path << " to "
				  << path << ": " << ec.message() << "\n";
			return false;
		}
	}

	std::cout << "[AI Brain] Saved " << header.weight_count << " weights, " << header.bias_count
		  << " biases, and " << (header.vocab_size * header.embedding_dim)
		  << " embeddings to " << path << "\n"
		  << std::flush;
	return true;
}

bool AI::load_weights(const std::string& path) {
	std::ifstream in(path, std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "[AI Brain] Failed to open weight file: " << path << "\n";
		return false;
	}

	AxiomWeightHeader header {};
	in.read(reinterpret_cast<char*>(&header), sizeof(header));
	if (in.gcount() != static_cast<std::streamsize>(sizeof(header))) {
		std::cerr << "[AI Brain] Incomplete or corrupt header in weight file: " << path
			  << "\n";
		return false;
	}

	// Validate magic bytes
	if (std::memcmp(header.magic, "AXIOM_V1", 8) != 0) {
		std::cerr << "[AI Brain] Invalid magic header in " << path
			  << " (expected 'AXIOM_V1')\n";
		return false;
	}

	// Validate model dimensions
	if (header.input_dimension != input_dimension ||
	    header.output_dimension != output_dimension) {
		std::cerr << "[AI Brain] Dimension mismatch in " << path << ": file has ("
			  << header.input_dimension << " -> " << header.output_dimension
			  << "), model expects (" << input_dimension << " -> " << output_dimension
			  << ")\n";
		return false;
	}

	if (header.weight_count != input_dimension * output_dimension ||
	    header.bias_count != output_dimension) {
		std::cerr << "[AI Brain] Element count mismatch in " << path << "\n";
		return false;
	}

	if (header.embedding_dim != 0 && header.embedding_dim != output_dimension) {
		std::cerr << "[AI Brain] Embedding dimension mismatch in " << path << ": file has "
			  << header.embedding_dim << ", model expects " << output_dimension << "\n";
		return false;
	}

	std::vector<float> loaded_weights(header.weight_count);
	std::vector<float> loaded_bias(header.bias_count);
	std::vector<float> loaded_embeddings;

	in.read(reinterpret_cast<char*>(loaded_weights.data()),
		header.weight_count * sizeof(float));
	if (in.gcount() != static_cast<std::streamsize>(header.weight_count * sizeof(float))) {
		std::cerr << "[AI Brain] Incomplete weight data in " << path << "\n";
		return false;
	}

	in.read(reinterpret_cast<char*>(loaded_bias.data()), header.bias_count * sizeof(float));
	if (in.gcount() != static_cast<std::streamsize>(header.bias_count * sizeof(float))) {
		std::cerr << "[AI Brain] Incomplete bias data in " << path << "\n";
		return false;
	}

	if (header.vocab_size > 0 && header.embedding_dim > 0) {
		size_t embedding_count = header.vocab_size * header.embedding_dim;
		loaded_embeddings.resize(embedding_count);
		in.read(reinterpret_cast<char*>(loaded_embeddings.data()),
			embedding_count * sizeof(float));
		if (in.gcount() != static_cast<std::streamsize>(embedding_count * sizeof(float))) {
			std::cerr << "[AI Brain] Incomplete embedding data in " << path << "\n";
			return false;
		}
	}

	for (float w : loaded_weights) {
		if (!std::isfinite(w)) {
			std::cerr << "[AI Brain] Corrupt weights in " << path
				  << ": contains NaN or Inf. Rejecting load.\n";
			return false;
		}
	}
	for (float b : loaded_bias) {
		if (!std::isfinite(b)) {
			std::cerr << "[AI Brain] Corrupt biases in " << path
				  << ": contains NaN or Inf. Rejecting load.\n";
			return false;
		}
	}
	for (float e_val : loaded_embeddings) {
		if (!std::isfinite(e_val)) {
			std::cerr << "[AI Brain] Corrupt embeddings in " << path
				  << ": contains NaN or Inf. Rejecting load.\n";
			return false;
		}
	}

	// Atomic update of host parameters
	host_weights = std::move(loaded_weights);
	host_bias = std::move(loaded_bias);
	if (!loaded_embeddings.empty()) {
		vocab_size = header.vocab_size;
		embedding_dim = header.embedding_dim;
		host_embeddings = std::move(loaded_embeddings);
	}

	// Synchronize and push directly to GPU VRAM
	if (!upload_weights()) {
		std::cerr << "[AI Brain] Failed to upload loaded weights to GPU VRAM.\n";
		return false;
	}

	std::cout << "[AI Brain] Successfully loaded weights and embeddings from " << path
		  << " and synced to GPU.\n"
		  << std::flush;
	return true;
}

bool AI::predict(const std::vector<std::bfloat16_t>& context_window,
		 std::vector<float>& output_prediction) {
	if (context_window.size() != input_dimension) {
		std::cerr << "[AI Brain] Input context dimension (" << context_window.size()
			  << ") does not match model in_dim (" << input_dimension << ").\n";
		return false;
	}

	synchronize_stream(&gpu_portion, 0);

	std::vector<std::bfloat16_t> dummy_target(output_dimension,
						  static_cast<std::bfloat16_t>(0.0f));

	if (!upload_to_gpu(context_window.data(),
			   context_window.size(),
			   dummy_target.data(),
			   dummy_target.size(),
			   &gpu_portion,
			   0)) {
		std::cerr << "[AI Brain] Failed to upload context vector to GPU for prediction.\n";
		return false;
	}

	gpu_embeddings.active_vocab_size = active_vocab_size;
	const GpuEmbeddings* embeddings_pointer =
		(gpu_embeddings.d_table != nullptr) ? &gpu_embeddings : nullptr;
	if (!forward_pass_with_embeddings(&gpu_portion,
					  embeddings_pointer,
					  &gpu_weights,
					  &gpu_activations,
					  input_dimension,
					  output_dimension,
					  0)) {
		std::cerr << "[AI Brain] Forward pass failed during prediction.\n";
		return false;
	}

	synchronize_stream(&gpu_portion, 0);

	return read_prediction(output_prediction);
}

bool AI::predict_tokens(const std::vector<uint32_t>& context_ids,
			std::vector<float>& output_prediction) {
	const size_t context_token_count = input_dimension / embedding_dim;
	if (context_ids.size() != context_token_count) {
		std::cerr << "[AI Brain] Context token count (" << context_ids.size()
			  << ") does not match model window (" << context_token_count << ").\n";
		return false;
	}
	if (gpu_embeddings.d_table == nullptr) {
		std::cerr << "[AI Brain] GPU embedding table is not allocated.\n";
		return false;
	}

	synchronize_stream(&gpu_portion, 0);

	std::vector<std::bfloat16_t> dummy_target(output_dimension,
						  static_cast<std::bfloat16_t>(0.0f));

	// Same path as training: IDs cross PCIe, the GPU gathers the trained embeddings
	if (!upload_tokens_to_gpu(context_ids.data(),
				  context_ids.size(),
				  dummy_target.data(),
				  dummy_target.size(),
				  &gpu_portion,
				  0)) {
		std::cerr << "[AI Brain] Failed to upload token IDs to GPU for prediction.\n";
		return false;
	}

	gpu_embeddings.active_vocab_size = active_vocab_size;
	if (!forward_pass_with_embeddings(&gpu_portion,
					  &gpu_embeddings,
					  &gpu_weights,
					  &gpu_activations,
					  input_dimension,
					  output_dimension,
					  0)) {
		std::cerr << "[AI Brain] Forward pass failed during prediction.\n";
		return false;
	}

	synchronize_stream(&gpu_portion, 0);

	return read_prediction(output_prediction);
}

bool AI::read_prediction(std::vector<float>& output_prediction) {
	std::vector<std::bfloat16_t> output_activations(output_dimension);
	if (!download_output(&gpu_activations, output_activations.data(), output_dimension)) {
		std::cerr << "[AI Brain] Failed to download prediction activations from GPU.\n";
		return false;
	}

	uint32_t gpu_token = 0;
	if (download_pred_token(&gpu_activations, &gpu_token)) {
		last_predicted_token_id = gpu_token;
	} else {
		last_predicted_token_id = 0;
	}

	output_prediction.resize(output_dimension);
	for (size_t i = 0; i < output_dimension; ++i) {
		output_prediction[i] = static_cast<float>(output_activations[i]);
	}

	return true;
}

bool AI::generate(const std::string& prompt,
		  Parser& parser,
		  Input& input,
		  size_t max_new_tokens,
		  const SamplingConfig& config,
		  std::function<void(const std::string&)> token_callback) {
	gpu_embeddings.temperature = config.temperature;

	if (prompt.empty()) {
		return false;
	}

	std::vector<uint32_t> tokens = parser.tokenize_for_inference(input, prompt);
	if (tokens.empty()) {
		return false;
	}

	if (input.next_text_id > 4) {
		active_vocab_size = input.next_text_id;
	}

	const size_t context_token_count = input_dimension / embedding_dim;

	// Right-aligned: the newest token sits in the last slot, exactly as in training
	std::vector<uint32_t> context_ids = parser.build_context_ids(tokens, context_token_count);

	std::vector<float> prediction(output_dimension, 0.0f);

	for (size_t step = 0; step < max_new_tokens; ++step) {
		if (!predict_tokens(context_ids, prediction)) {
			return false;
		}

		uint32_t token_id = 0;

		if (config.temperature <= 1e-4f) {
			// Greedy Fallback: directly use on-device GPU argmax winner
			token_id = last_predicted_token_id;
			if (token_id == 0) {
				size_t vocab_sz = (active_vocab_size > 0) ? active_vocab_size : vocab_size;
				std::vector<float> probabilities(vocab_sz, 0.0f);
				if (download_probabilities(&gpu_activations, probabilities.data(), vocab_sz)) {
					float best_prob = -1.0f;
					for (uint32_t id = 1; id < vocab_sz; ++id) {
						if (probabilities[id] > best_prob) {
							best_prob = probabilities[id];
							token_id = id;
						}
					}
				}
			}
		} else {
			size_t vocab_sz = (active_vocab_size > 0) ? active_vocab_size : vocab_size;
			std::vector<float> probabilities(vocab_sz, 0.0f);
			if (download_probabilities(&gpu_activations, probabilities.data(), vocab_sz)) {
				std::vector<std::pair<float, uint32_t>> candidates;
				candidates.reserve(vocab_sz);
				for (uint32_t id = 1; id < vocab_sz; ++id) {
					if (probabilities[id] > 0.0f) {
						candidates.emplace_back(probabilities[id], id);
					}
				}

				if (!candidates.empty()) {
					// 1. Top-K Truncation: Isolate K highest probability tokens
					if (config.top_k > 0 &&
					    static_cast<size_t>(config.top_k) < candidates.size()) {
						std::partial_sort(
							candidates.begin(),
							candidates.begin() + config.top_k,
							candidates.end(),
							[](const auto& a, const auto& b) {
								return a.first > b.first;
							});
						candidates.resize(config.top_k);
					} else {
						std::sort(
							candidates.begin(),
							candidates.end(),
							[](const auto& a, const auto& b) {
								return a.first > b.first;
							});
					}

					// 2. Top-P (Nucleus) Truncation: Cutoff cumulative mass at P
					float cumulative = 0.0f;
					size_t nucleus_size = 0;
					for (const auto& c : candidates) {
						cumulative += c.first;
						nucleus_size++;
						if (cumulative >= config.top_p && nucleus_size >= 1) {
							break;
						}
					}
					candidates.resize(nucleus_size);

					// 3. Re-normalize & Categorical Random Draw
					float sum_prob = 0.0f;
					for (const auto& c : candidates) {
						sum_prob += c.first;
					}

					if (sum_prob > 0.0f) {
						std::mt19937_64 rng(config.seed != 0
									    ? (config.seed + step)
									    : std::random_device {}());
						std::uniform_real_distribution<float> dist(0.0f, sum_prob);
						float r = dist(rng);
						float running_sum = 0.0f;
						token_id = candidates.front().second;
						for (const auto& c : candidates) {
							running_sum += c.first;
							if (r <= running_sum) {
								token_id = c.second;
								break;
							}
						}
					}
				}
			}
		}

		if (token_id == 0) {
			token_id = last_predicted_token_id;
			if (token_id == 0 && !input.embedding_table.empty()) {
				token_id = parser.decode_token(input, prediction);
			}
			if (token_id == 0) {
				token_id = vocab::UNK;
			}
		}

		if (token_id == vocab::EOS) {
			if (step < 4) {
				token_id = vocab::UNK;
			} else {
				break;
			}
		}

		std::string token_string = parser.token_to_string(input, token_id);

		if (token_callback) {
			token_callback(token_string);
		}

		std::rotate(context_ids.begin(), context_ids.begin() + 1, context_ids.end());
		context_ids.back() = token_id;
	}

	return true;
}


bool AI::build_transformer_block(size_t hidden_dim, size_t heads, size_t ffn_dim) {
	if (hidden_dim == 0 || heads == 0 || ffn_dim == 0)
		return false;
	this->num_heads = heads;
	this->ffn_dimension = ffn_dim;
	this->enable_transformer = true;

	return allocate_transformer_weights(&gpu_weights, hidden_dim, ffn_dim);
}

bool AI::forward_transformer(std::vector<std::bfloat16_t>& hidden_states, size_t sequence_length) {
	if (hidden_states.empty() || sequence_length == 0 || gpu_weights.d_query_weights == nullptr)
		return false;
	size_t hidden_dim = hidden_states.size() / sequence_length;

	size_t bytes = hidden_states.size() * sizeof(std::bfloat16_t);
	void* d_temp = allocate_device_memory(bytes);
	if (!d_temp)
		return false;
	upload_device_buffer(d_temp, hidden_states.data(), bytes);

	bool ok = transformer_forward_block(d_temp, &gpu_weights, &gpu_activations, sequence_length,
					    hidden_dim, num_heads, ffn_dimension, 0);
	if (ok) {
		download_device_buffer(hidden_states.data(), d_temp, bytes);
	}
	free_device_memory(d_temp);
	return ok;
}

bool AI::backward_transformer(const std::vector<float>& grad_output,
			      std::vector<float>& grad_input,
			      const std::vector<std::bfloat16_t>& hidden_states,
			      size_t sequence_length,
			      float learning_rate) {
	if (grad_output.empty() || sequence_length == 0 || gpu_weights.d_query_weights == nullptr)
		return false;
	size_t hidden_dim = grad_output.size() / sequence_length;
	grad_input.resize(grad_output.size());

	size_t f_bytes = grad_output.size() * sizeof(float);
	size_t bf_bytes = hidden_states.size() * sizeof(std::bfloat16_t);

	void* d_grad_out = allocate_device_memory(f_bytes);
	void* d_grad_in = allocate_device_memory(f_bytes);
	void* d_hidden = allocate_device_memory(bf_bytes);

	if (!d_grad_out || !d_grad_in || !d_hidden) {
		if (d_grad_out) free_device_memory(d_grad_out);
		if (d_grad_in) free_device_memory(d_grad_in);
		if (d_hidden) free_device_memory(d_hidden);
		return false;
	}

	upload_device_buffer(d_grad_out, grad_output.data(), f_bytes);
	upload_device_buffer(d_hidden, hidden_states.data(), bf_bytes);

	bool ok = transformer_backward_block(d_grad_out, d_grad_in, d_hidden, &gpu_weights,
					     &gpu_activations, sequence_length, hidden_dim,
					     num_heads, ffn_dimension, learning_rate, 0);
	if (ok) {
		download_device_buffer(grad_input.data(), d_grad_in, f_bytes);
	}
	free_device_memory(d_grad_out);
	free_device_memory(d_grad_in);
	free_device_memory(d_hidden);
	return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4: PagedAttention KV-Cache & Block Management
// ─────────────────────────────────────────────────────────────────────────────

bool AI::build_paged_cache(size_t total_blocks, size_t block_size) {
	free_paged_cache_memory();
	size_t hidden_dim = embedding_dim;
	if (hidden_dim == 0)
		hidden_dim = 32;

	bool ok = allocate_paged_cache(&gpu_paged_cache, total_blocks, block_size, hidden_dim, num_heads);
	if (!ok)
		return false;

	block_manager = std::make_unique<PagedBlockManager>(total_blocks, block_size);
	return true;
}

void AI::free_paged_cache_memory() {
	free_paged_cache(&gpu_paged_cache);
	block_manager.reset();
}

bool AI::fork_sequence_context(const SequenceContext& parent, SequenceContext& child) {
	if (!block_manager)
		return false;
	child.sequence_id = parent.sequence_id + 1;
	child.sequence_length = parent.sequence_length;
	child.block_table = parent.block_table;
	for (int block_id : child.block_table) {
		block_manager->retain_physical_block(block_id);
	}
	return true;
}

bool AI::append_sequence_token(SequenceContext& sequence,
			       const std::vector<std::bfloat16_t>& key,
			       const std::vector<std::bfloat16_t>& value) {
	if (!block_manager || !gpu_paged_cache.d_key_blocks)
		return false;
	if (key.size() != gpu_paged_cache.hidden_dimension ||
	    value.size() != gpu_paged_cache.hidden_dimension)
		return false;

	size_t block_size = block_manager->get_block_size();
	size_t current_token_index = sequence.sequence_length;
	size_t offset = current_token_index % block_size;

	int target_block = -1;
	if (offset == 0) {
		target_block = block_manager->allocate_physical_block();
		if (target_block < 0)
			return false;
		sequence.block_table.push_back(target_block);
	} else {
		target_block = sequence.block_table.back();
		if (block_manager->get_reference_count(target_block) > 1) {
			int new_block = block_manager->allocate_physical_block();
			if (new_block < 0)
				return false;
			copy_physical_block(&gpu_paged_cache, target_block, new_block, offset, nullptr);
			block_manager->release_physical_block(target_block);
			sequence.block_table.back() = new_block;
			target_block = new_block;
		}
	}

	append_paged_kv(&gpu_paged_cache, target_block, offset, key.data(), value.data(), nullptr);
	sequence.sequence_length++;
	return true;
}

void AI::release_sequence_context(SequenceContext& sequence) {
	if (!block_manager)
		return;
	for (int block_id : sequence.block_table) {
		block_manager->release_physical_block(block_id);
	}
	sequence.block_table.clear();
	sequence.sequence_length = 0;
}

bool AI::forward_paged_attention(const SequenceContext& sequence,
				 const std::vector<std::bfloat16_t>& query,
				 std::vector<std::bfloat16_t>& output,
				 size_t query_length) {
	if (!gpu_paged_cache.d_key_blocks || sequence.block_table.empty())
		return false;
	if (query_length == 0 || sequence.sequence_length == 0)
		return false;
	if (query.size() != query_length * gpu_paged_cache.hidden_dimension)
		return false;

	size_t table_bytes = sequence.block_table.size() * sizeof(int);
	if (!upload_device_buffer(gpu_paged_cache.d_block_table, sequence.block_table.data(), table_bytes))
		return false;

	size_t query_bytes = query.size() * sizeof(std::bfloat16_t);
	void* d_q = gpu_activations.d_query ? gpu_activations.d_query : allocate_device_memory(query_bytes);
	void* d_out = gpu_activations.d_attention_output ? gpu_activations.d_attention_output : allocate_device_memory(query_bytes);

	if (!d_q || !d_out) {
		if (d_q && d_q != gpu_activations.d_query)
			free_device_memory(d_q);
		if (d_out && d_out != gpu_activations.d_attention_output)
			free_device_memory(d_out);
		return false;
	}

	upload_device_buffer(d_q, query.data(), query_bytes);

	bool ok = paged_causal_attention(
		d_q,
		&gpu_paged_cache,
		static_cast<const int*>(gpu_paged_cache.d_block_table),
		d_out,
		query_length,
		sequence.sequence_length,
		gpu_paged_cache.hidden_dimension,
		gpu_paged_cache.num_heads,
		nullptr);

	if (ok) {
		output.resize(query_length * gpu_paged_cache.hidden_dimension);
		download_device_buffer(output.data(), d_out, query_bytes);
	}

	if (d_q != gpu_activations.d_query)
		free_device_memory(d_q);
	if (d_out != gpu_activations.d_attention_output)
		free_device_memory(d_out);

	return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 5: Zero-Overhead Hardware Saturation via CUDA Graphs
// ─────────────────────────────────────────────────────────────────────────────

bool AI::build_training_graph(Optimizer& optimizer, Portion& portion) {
	if (input_dimension == 0 || output_dimension == 0)
		return false;

	int buffer_index = portion.buffer_index;
	synchronize_stream(&gpu_portion, buffer_index);
	optimizer.load_context_window(portion);

	if (portion.has_token_ids && portion.context_token_ids[buffer_index] != nullptr &&
	    gpu_embeddings.d_table != nullptr) {
		upload_tokens_to_gpu(portion.context_token_ids[buffer_index],
				     portion.context_window_size,
				     portion.target_tokens[buffer_index],
				     portion.target_elements,
				     &gpu_portion,
				     buffer_index);
	}

	float lr = optimizer.learning_rate;
	return capture_training_graph(&gpu_graph_execution, &gpu_portion, &gpu_embeddings,
				      &gpu_weights, &gpu_activations, input_dimension,
				      output_dimension, lr, buffer_index);
}

bool AI::launch_training_graph_step() {
	if (!gpu_graph_execution.is_instantiated)
		return false;
	return launch_training_graph(&gpu_graph_execution, nullptr);
}

void AI::free_training_graph_resources() {
	free_training_graph(&gpu_graph_execution);
}

float AI::benchmark_graph_launch_latency(int iterations) {
	if (!gpu_graph_execution.is_instantiated)
		return -1.0f;
	return benchmark_graph_latency(&gpu_graph_execution, nullptr, iterations);
}
