#include "../include/ai.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

#include "../include/parser.hpp"
#include "../include/transfer.hpp"
#include "CUDA/kernel/forward_pass.cuh"
#include "CUDA/kernel/rmsnorm.cuh"
#include "CUDA/kernel/rope.cuh"
#include "CUDA/kernel/swiglu.cuh"
#include "CUDA/kernel/paged_gqa.cuh"
#include "CUDA/util/memory_allocation.cuh"
#include <windows.h>

AI::AI()
    : input_elements_x_total(512),
      target_elements_y_total(4),
      weight_min(-0.1f),
      weight_max(0.1f) {
	vocab_size = 151936;
	embedding_dim = 1536;
	compute_stream_ = create_cuda_stream();
}


AI::AI(size_t input_elements_x_total,
       size_t target_elements_y_total,
       float weight_min,
       float weight_max,
       Elements& elements)
    : input_elements_x_total(input_elements_x_total),
      target_elements_y_total(target_elements_y_total),
      weight_min(weight_min),
      weight_max(weight_max) {

	// input_elements_x_total = token * dim = 512
	// target_elements_y_total = target * dim = 4

	// 512 * 4 = 2048
	elements.weight_elements = input_elements_x_total * target_elements_y_total;

	// target_elements_y_total = 4
	elements.bias_elements = target_elements_y_total;

	host_weights.resize(elements.weight_elements);
	host_bias.resize(elements.bias_elements);

	vocab_size = 65536;
	embedding_dim = target_elements_y_total;
	if (embedding_dim == 0)
		embedding_dim = 4;

	host_embeddings.resize(vocab_size * embedding_dim);

	reset_weights(elements);
	if (allocate_gpu_embeddings(&gpu_embeddings, vocab_size, embedding_dim)) {
		upload_embeddings(host_embeddings.data(), &gpu_embeddings);
	}
	compute_stream_ = create_cuda_stream();
}

void AI::reset_weights(Elements& elements) {
	elements.weight_elements = input_elements_x_total * target_elements_y_total;
	elements.bias_elements = target_elements_y_total;

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

	if (d_weights_arena_) { free_device_memory(d_weights_arena_); d_weights_arena_ = nullptr; }
	if (d_curr_token_) { free_device_memory(d_curr_token_); d_curr_token_ = nullptr; }
	if (d_curr_pos_) { free_device_memory(d_curr_pos_); d_curr_pos_ = nullptr; }
	if (d_residual_stream_) { free_device_memory(d_residual_stream_); d_residual_stream_ = nullptr; }
	if (d_normed_x_) { free_device_memory(d_normed_x_); d_normed_x_ = nullptr; }
	if (d_q_) { free_device_memory(d_q_); d_q_ = nullptr; }
	if (d_k_) { free_device_memory(d_k_); d_k_ = nullptr; }
	if (d_v_) { free_device_memory(d_v_); d_v_ = nullptr; }
	if (d_attn_out_) { free_device_memory(d_attn_out_); d_attn_out_ = nullptr; }
	if (d_proj_tmp_) { free_device_memory(d_proj_tmp_); d_proj_tmp_ = nullptr; }
	if (d_gate_buf_) { free_device_memory(d_gate_buf_); d_gate_buf_ = nullptr; }
	if (d_up_buf_) { free_device_memory(d_up_buf_); d_up_buf_ = nullptr; }
	if (d_act_buf_) { free_device_memory(d_act_buf_); d_act_buf_ = nullptr; }
	if (d_pred_id_) { free_device_memory(d_pred_id_); d_pred_id_ = nullptr; }
	if (d_logits_) { free_device_memory(d_logits_); d_logits_ = nullptr; }
	if (d_block_table_) { free_device_memory(d_block_table_); d_block_table_ = nullptr; }
	for (void* ptr : d_k_cache_) { if (ptr) free_device_memory(ptr); }
	for (void* ptr : d_v_cache_) { if (ptr) free_device_memory(ptr); }
	d_k_cache_.clear();
	d_v_cache_.clear();
	if (compute_stream_) { destroy_cuda_stream(compute_stream_); compute_stream_ = nullptr; }
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
						  input_elements_x_total,
						  target_elements_y_total,
						  buffer_index)) {
			std::cerr << "[AI Brain] Forward pass with embeddings failed.\n";
			return TrainStatus::Failed;
		}

		// Launch CUDA fused AdamW + backward scatter embedding update
		if (!backward_pass_with_embeddings(&gpu_portion,
						   &gpu_embeddings,
						   &gpu_weights,
						   &gpu_activations,
						   input_elements_x_total,
						   target_elements_y_total,
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
				  input_elements_x_total,
				  target_elements_y_total,
				  buffer_index)) {
			std::cerr << "[AI Brain] Forward pass failed.\n";
			return TrainStatus::Failed;
		}

		if (!backward_pass(&gpu_portion,
				   &gpu_weights,
				   &gpu_activations,
				   input_elements_x_total,
				   target_elements_y_total,
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
	header.input_elements_x_total = input_elements_x_total;
	header.target_elements_y_total = target_elements_y_total;
	header.weight_count = host_weights.size();
	header.bias_count = host_bias.size();
	header.vocab_size = vocab_size;
	header.embedding_dim = embedding_dim;

	if (header.weight_count != input_elements_x_total * target_elements_y_total ||
	    header.bias_count != target_elements_y_total) {
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
	if (header.input_elements_x_total != input_elements_x_total ||
	    header.target_elements_y_total != target_elements_y_total) {
		std::cerr << "[AI Brain] Dimension mismatch in " << path << ": file has ("
			  << header.input_elements_x_total << " -> "
			  << header.target_elements_y_total << "), model expects ("
			  << input_elements_x_total << " -> " << target_elements_y_total << ")\n";
		return false;
	}

	if (header.weight_count != input_elements_x_total * target_elements_y_total ||
	    header.bias_count != target_elements_y_total) {
		std::cerr << "[AI Brain] Element count mismatch in " << path << "\n";
		return false;
	}

	if (header.embedding_dim != 0 && header.embedding_dim != target_elements_y_total) {
		std::cerr << "[AI Brain] Embedding dimension mismatch in " << path << ": file has "
			  << header.embedding_dim << ", model expects " << target_elements_y_total
			  << "\n";
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
	if (context_window.size() != input_elements_x_total) {
		std::cerr << "[AI Brain] Input context dimension (" << context_window.size()
			  << ") does not match model in_dim (" << input_elements_x_total << ").\n";
		return false;
	}

	synchronize_stream(&gpu_portion, 0);

	std::vector<std::bfloat16_t> dummy_target(target_elements_y_total,
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
					  input_elements_x_total,
					  target_elements_y_total,
					  0)) {
		std::cerr << "[AI Brain] Forward pass failed during prediction.\n";
		return false;
	}

	synchronize_stream(&gpu_portion, 0);

	return read_prediction(output_prediction);
}

bool AI::predict_tokens(const std::vector<uint32_t>& context_ids,
			std::vector<float>& output_prediction) {
	const size_t context_token_count = input_elements_x_total / embedding_dim;
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

	std::vector<std::bfloat16_t> dummy_target(target_elements_y_total,
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
					  input_elements_x_total,
					  target_elements_y_total,
					  0)) {
		std::cerr << "[AI Brain] Forward pass failed during prediction.\n";
		return false;
	}

	synchronize_stream(&gpu_portion, 0);

	return read_prediction(output_prediction);
}

bool AI::read_prediction(std::vector<float>& output_prediction) {
	std::vector<std::bfloat16_t> output_activations(target_elements_y_total);
	if (!download_output(
		    &gpu_activations, output_activations.data(), target_elements_y_total)) {
		std::cerr << "[AI Brain] Failed to download prediction activations from GPU.\n";
		return false;
	}

	uint32_t gpu_token = 0;
	if (download_pred_token(&gpu_activations, &gpu_token)) {
		last_predicted_token_id = gpu_token;
	} else {
		last_predicted_token_id = 0;
	}

	output_prediction.resize(target_elements_y_total);
	for (size_t i = 0; i < target_elements_y_total; ++i) {
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

	const size_t context_token_count = input_elements_x_total / embedding_dim;

	// Right-aligned: the newest token sits in the last slot, exactly as in training
	std::vector<uint32_t> context_ids = parser.build_context_ids(tokens, context_token_count);

	std::vector<float> prediction(target_elements_y_total, 0.0f);

	for (size_t step = 0; step < max_new_tokens; ++step) {
		if (!predict_tokens(context_ids, prediction)) {
			return false;
		}

		uint32_t token_id = 0;

		if (config.temperature <= 1e-4f) {
			// Greedy Fallback: directly use on-device GPU argmax winner
			token_id = last_predicted_token_id;
			if (token_id == 0) {
				size_t vocab_sz =
					(active_vocab_size > 0) ? active_vocab_size : vocab_size;
				std::vector<float> probabilities(vocab_sz, 0.0f);
				if (download_probabilities(
					    &gpu_activations, probabilities.data(), vocab_sz)) {
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
			if (download_probabilities(
				    &gpu_activations, probabilities.data(), vocab_sz)) {
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
						std::partial_sort(candidates.begin(),
								  candidates.begin() + config.top_k,
								  candidates.end(),
								  [](const auto& a, const auto& b) {
									  return a.first > b.first;
								  });
						candidates.resize(config.top_k);
					} else {
						std::sort(candidates.begin(),
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
						if (cumulative >= config.top_p &&
						    nucleus_size >= 1) {
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
						std::mt19937_64 rng(
							config.seed != 0 ? (config.seed + step)
									 : std::random_device {}());
						std::uniform_real_distribution<float> dist(
							0.0f, sum_prob);
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

	bool ok = transformer_forward_block(d_temp,
					    &gpu_weights,
					    &gpu_activations,
					    sequence_length,
					    hidden_dim,
					    num_heads,
					    ffn_dimension,
					    0);
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
		if (d_grad_out)
			free_device_memory(d_grad_out);
		if (d_grad_in)
			free_device_memory(d_grad_in);
		if (d_hidden)
			free_device_memory(d_hidden);
		return false;
	}

	upload_device_buffer(d_grad_out, grad_output.data(), f_bytes);
	upload_device_buffer(d_hidden, hidden_states.data(), bf_bytes);

	bool ok = transformer_backward_block(d_grad_out,
					     d_grad_in,
					     d_hidden,
					     &gpu_weights,
					     &gpu_activations,
					     sequence_length,
					     hidden_dim,
					     num_heads,
					     ffn_dimension,
					     learning_rate,
					     0);
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

	bool ok = allocate_paged_cache(
		&gpu_paged_cache, total_blocks, block_size, hidden_dim, num_heads);
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
		if (sequence.block_table.empty())
			return false;
		target_block = sequence.block_table.back();
		if (block_manager->get_reference_count(target_block) > 1) {
			int new_block = block_manager->allocate_physical_block();
			if (new_block < 0)
				return false;
			copy_physical_block(
				&gpu_paged_cache, target_block, new_block, offset, nullptr);
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
	if (!upload_device_buffer(
		    gpu_paged_cache.d_block_table, sequence.block_table.data(), table_bytes))
		return false;

	size_t query_bytes = query.size() * sizeof(std::bfloat16_t);
	void* d_q = gpu_activations.d_query ? gpu_activations.d_query
					    : allocate_device_memory(query_bytes);
	void* d_out = gpu_activations.d_attention_output ? gpu_activations.d_attention_output
							 : allocate_device_memory(query_bytes);

	if (!d_q || !d_out) {
		if (d_q && d_q != gpu_activations.d_query)
			free_device_memory(d_q);
		if (d_out && d_out != gpu_activations.d_attention_output)
			free_device_memory(d_out);
		return false;
	}

	upload_device_buffer(d_q, query.data(), query_bytes);

	bool ok = paged_causal_attention(d_q,
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
	if (input_elements_x_total == 0 || target_elements_y_total == 0)
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
	return capture_training_graph(&gpu_graph_execution,
				      &gpu_portion,
				      &gpu_embeddings,
				      &gpu_weights,
				      &gpu_activations,
				      input_elements_x_total,
				      target_elements_y_total,
				      lr,
				      buffer_index);
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

bool AI::build_expert_layers(size_t hidden_dim, size_t ffn_dim, size_t num_experts) {
	if (hidden_dim == 0 || ffn_dim == 0 || num_experts == 0)
		return false;
	this->ffn_dimension = ffn_dim;
	return allocate_expert_weights(&gpu_weights, hidden_dim, ffn_dim, num_experts);
}

bool AI::forward_mixture_experts(std::vector<std::bfloat16_t>& hidden_states,
				 size_t sequence_length,
				 std::vector<int>& routing_histogram) {
	if (hidden_states.empty() || sequence_length == 0 ||
	    gpu_weights.d_router_weights == nullptr)
		return false;

	size_t hidden_dim = hidden_states.size() / sequence_length;
	size_t bytes = hidden_states.size() * sizeof(std::bfloat16_t);

	void* d_temp = allocate_device_memory(bytes);
	if (!d_temp)
		return false;
	upload_device_buffer(d_temp, hidden_states.data(), bytes);

	bool ok = ::forward_mixture_experts(d_temp,
					    &gpu_weights,
					    &gpu_activations,
					    sequence_length,
					    hidden_dim,
					    ffn_dimension,
					    0);
	if (ok) {
		download_device_buffer(hidden_states.data(), d_temp, bytes);
		routing_histogram.resize(gpu_weights.expert_count, 0);
		download_expert_counts(
			&gpu_activations, routing_histogram.data(), gpu_weights.expert_count);
	}
	free_device_memory(d_temp);
	return ok;
}

bool AI::verify_routing_balance(const std::vector<int>& routing_histogram,
				size_t total_tokens,
				size_t num_experts) {
	if (routing_histogram.size() != num_experts || total_tokens == 0 || num_experts == 0)
		return false;

	int sum_tokens = 0;
	int min_tokens = routing_histogram[0];
	int max_tokens = routing_histogram[0];

	for (int count : routing_histogram) {
		sum_tokens += count;
		if (count < min_tokens)
			min_tokens = count;
		if (count > max_tokens)
			max_tokens = count;
	}

	// Each token is routed to exactly 2 experts
	if (sum_tokens != static_cast<int>(total_tokens * 2))
		return false;

	// Zero dead experts allowed
	if (min_tokens <= 0)
		return false;

	// Maximum load imbalance ratio must be within reasonable bound (< 3.0x average)
	float avg_load = static_cast<float>(sum_tokens) / static_cast<float>(num_experts);
	float max_ratio = static_cast<float>(max_tokens) / avg_load;

	return max_ratio < 3.0f;
}

bool AI::load_vocabulary(const std::string& vocab_path) {
	std::ifstream file(vocab_path, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "[Axiom Engine] Failed to open vocab file: " << vocab_path << "\n";
		return false;
	}
	uint32_t count = 0;
	if (!file.read(reinterpret_cast<char*>(&count), sizeof(uint32_t)) || count > 2000000U) {
		return false;
	}
	qwen_vocab_.clear();
	qwen_reverse_vocab_.clear();
	qwen_vocab_.reserve(count);
	qwen_reverse_vocab_.reserve(count);

	for (uint32_t i = 0; i < count; ++i) {
		uint32_t token_id = 0;
		uint32_t str_len = 0;
		if (!file.read(reinterpret_cast<char*>(&token_id), sizeof(uint32_t))) break;
		if (!file.read(reinterpret_cast<char*>(&str_len), sizeof(uint32_t)) || str_len > 1048576U) break;
		std::string token_str(str_len, '\0');
		if (str_len > 0) {
			if (!file.read(&token_str[0], str_len)) break;
		}
		qwen_vocab_[token_str] = token_id;
		qwen_reverse_vocab_[token_id] = token_str;
	}
	std::cout << "[Axiom Engine] Successfully loaded " << qwen_vocab_.size() << " vocabulary entries.\n";
	return !qwen_vocab_.empty();
}

bool AI::load_axiom_model_mmap(const char* filepath) {
	HANDLE hFile = CreateFileA(
		filepath, GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr
	);
	if (hFile == INVALID_HANDLE_VALUE) {
		std::cerr << "[Axiom Engine] Cannot open model file: " << filepath << "\n";
		return false;
	}

	LARGE_INTEGER file_size;
	GetFileSizeEx(hFile, &file_size);
	size_t total_bytes = static_cast<size_t>(file_size.QuadPart);
	if (total_bytes <= 64) {
		CloseHandle(hFile);
		return false;
	}

	HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (!hMap) {
		CloseHandle(hFile);
		return false;
	}

	const uint8_t* mapped_base = static_cast<const uint8_t*>(
		MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)
	);
	if (!mapped_base) {
		CloseHandle(hMap);
		CloseHandle(hFile);
		return false;
	}

	std::memcpy(&config_, mapped_base, sizeof(AxiomModelConfig));
	if (config_.magic != 0x4D4F4958U || config_.num_layers == 0 || config_.hidden_dim == 0 ||
	    config_.head_dim == 0 || config_.num_q_heads == 0 || config_.num_kv_heads == 0 ||
	    config_.ffn_dim == 0 || config_.vocab_size == 0 || config_.max_seq_len == 0) {
		std::cerr << "[Axiom Engine] Invalid .axiom model header in: " << filepath << "\n";
		UnmapViewOfFile(mapped_base);
		CloseHandle(hMap);
		CloseHandle(hFile);
		return false;
	}
	if (config_.max_seq_len > 16384) {
		config_.max_seq_len = 16384;
	}
	const uint8_t* host_weights_ptr = mapped_base + 64;
	size_t weights_bytes = total_bytes - 64;

	const size_t D   = config_.hidden_dim;
	const size_t Qd  = config_.num_q_heads * config_.head_dim;
	const size_t KVd = config_.num_kv_heads * config_.head_dim;
	const size_t FFN = config_.ffn_dim;
	const size_t V   = config_.vocab_size;
	const size_t per_layer_elements =
		D + (Qd * D) + Qd + (KVd * D) + KVd + (KVd * D) + KVd + (D * Qd) + D + (FFN * D) + (FFN * D) + (D * FFN);
	const size_t min_required_elements = (V * D) + (static_cast<size_t>(config_.num_layers) * per_layer_elements) + D;
	if (weights_bytes < min_required_elements * sizeof(uint16_t)) {
		std::cerr << "[Axiom Engine] Truncated .axiom weights payload in: " << filepath << "\n";
		UnmapViewOfFile(mapped_base);
		CloseHandle(hMap);
		CloseHandle(hFile);
		return false;
	}

	if (d_weights_arena_) {
		free_device_memory(d_weights_arena_);
		d_weights_arena_ = nullptr;
	}

	d_weights_arena_ = allocate_device_memory(weights_bytes);
	if (!d_weights_arena_) {
		std::cerr << "[Axiom Engine] Failed to allocate device memory for weights arena (" << (weights_bytes >> 20) << " MB)\n";
		UnmapViewOfFile(mapped_base);
		CloseHandle(hMap);
		CloseHandle(hFile);
		return false;
	}

	const size_t chunk_bytes = 256 * 1024 * 1024;
	for (size_t offset = 0; offset < weights_bytes; offset += chunk_bytes) {
		size_t to_copy = std::min(chunk_bytes, weights_bytes - offset);
		if (!upload_device_buffer(static_cast<uint8_t*>(d_weights_arena_) + offset,
		                          host_weights_ptr + offset,
		                          to_copy)) {
			std::cerr << "[Axiom Engine] Failed to upload weights chunk to VRAM at offset "
			          << (offset >> 20) << " MB.\n";
			UnmapViewOfFile(mapped_base);
			CloseHandle(hMap);
			CloseHandle(hFile);
			return false;
		}
	}

	UnmapViewOfFile(mapped_base);
	CloseHandle(hMap);
	CloseHandle(hFile);

	const uint16_t* cursor = static_cast<const uint16_t*>(d_weights_arena_);

	d_embed_tokens_ = cursor; cursor += V * D;
	layers_.resize(config_.num_layers);

	for (uint32_t l = 0; l < config_.num_layers; ++l) {
		layers_[l].d_input_layernorm     = cursor; cursor += D;
		layers_[l].d_q_proj_weight       = cursor; cursor += Qd * D;
		layers_[l].d_q_proj_bias         = cursor; cursor += Qd;
		layers_[l].d_k_proj_weight       = cursor; cursor += KVd * D;
		layers_[l].d_k_proj_bias         = cursor; cursor += KVd;
		layers_[l].d_v_proj_weight       = cursor; cursor += KVd * D;
		layers_[l].d_v_proj_bias         = cursor; cursor += KVd;
		layers_[l].d_o_proj_weight       = cursor; cursor += D * Qd;
		layers_[l].d_post_attn_layernorm = cursor; cursor += D;
		layers_[l].d_gate_proj_weight    = cursor; cursor += FFN * D;
		layers_[l].d_up_proj_weight      = cursor; cursor += FFN * D;
		layers_[l].d_down_proj_weight    = cursor; cursor += D * FFN;
	}

	d_final_norm_weight_ = cursor; cursor += D;
	const size_t untied_elements = min_required_elements + (V * D);
	d_lm_head_weight_ = (weights_bytes >= untied_elements * sizeof(uint16_t)) ? cursor : d_embed_tokens_;

	// Allocate working activation buffers
	if (d_curr_token_)      free_device_memory(d_curr_token_);
	if (d_curr_pos_)        free_device_memory(d_curr_pos_);
	if (d_residual_stream_) free_device_memory(d_residual_stream_);
	if (d_normed_x_)        free_device_memory(d_normed_x_);
	if (d_q_)               free_device_memory(d_q_);
	if (d_k_)               free_device_memory(d_k_);
	if (d_v_)               free_device_memory(d_v_);
	if (d_attn_out_)        free_device_memory(d_attn_out_);
	if (d_proj_tmp_)        free_device_memory(d_proj_tmp_);
	if (d_gate_buf_)        free_device_memory(d_gate_buf_);
	if (d_up_buf_)          free_device_memory(d_up_buf_);
	if (d_act_buf_)         free_device_memory(d_act_buf_);
	if (d_pred_id_)         free_device_memory(d_pred_id_);
	if (d_logits_)          free_device_memory(d_logits_);

	d_curr_token_      = allocate_device_memory(sizeof(uint32_t));
	d_curr_pos_        = allocate_device_memory(sizeof(int32_t));
	d_pred_id_         = allocate_device_memory(sizeof(uint32_t));
	d_logits_          = allocate_device_memory(config_.vocab_size * sizeof(float));
	d_residual_stream_ = allocate_device_memory(D * sizeof(uint16_t));
	d_normed_x_        = allocate_device_memory(D * sizeof(uint16_t));
	d_q_               = allocate_device_memory(Qd * sizeof(uint16_t));
	d_k_               = allocate_device_memory(KVd * sizeof(uint16_t));
	d_v_               = allocate_device_memory(KVd * sizeof(uint16_t));
	d_attn_out_        = allocate_device_memory(Qd * sizeof(uint16_t));
	d_proj_tmp_        = allocate_device_memory(std::max(D, Qd) * sizeof(uint16_t));
	d_gate_buf_        = allocate_device_memory(FFN * sizeof(uint16_t));
	d_up_buf_          = allocate_device_memory(FFN * sizeof(uint16_t));
	d_act_buf_         = allocate_device_memory(FFN * sizeof(uint16_t));

	// Allocate Paged KV Cache and physical block table
	int num_blocks = static_cast<int>((config_.max_seq_len + 15) / 16);
	if (num_blocks <= 0) num_blocks = 512;
	std::vector<int32_t> h_block_table(num_blocks);
	for (int i = 0; i < num_blocks; ++i) {
		h_block_table[i] = i;
	}
	if (d_block_table_) free_device_memory(d_block_table_);
	d_block_table_ = allocate_device_memory(num_blocks * sizeof(int32_t));
	upload_device_buffer(d_block_table_, h_block_table.data(), num_blocks * sizeof(int32_t));

	for (void* ptr : d_k_cache_) if (ptr) free_device_memory(ptr);
	for (void* ptr : d_v_cache_) if (ptr) free_device_memory(ptr);
	d_k_cache_.resize(config_.num_layers, nullptr);
	d_v_cache_.resize(config_.num_layers, nullptr);

	if (!d_curr_token_ || !d_curr_pos_ || !d_pred_id_ || !d_logits_ || !d_residual_stream_ || !d_normed_x_ ||
	    !d_q_ || !d_k_ || !d_v_ || !d_attn_out_ || !d_proj_tmp_ ||
	    !d_gate_buf_ || !d_up_buf_ || !d_act_buf_ || !d_block_table_) {
		std::cerr << "[Axiom Engine] Out of VRAM: Activation buffer allocation failed.\n";
		return false;
	}

	size_t kv_cache_bytes = static_cast<size_t>(config_.max_seq_len) * config_.num_kv_heads * config_.head_dim * sizeof(uint16_t);
	for (uint32_t l = 0; l < config_.num_layers; ++l) {
		d_k_cache_[l] = allocate_device_memory(kv_cache_bytes);
		d_v_cache_[l] = allocate_device_memory(kv_cache_bytes);
		if (!d_k_cache_[l] || !d_v_cache_[l]) {
			std::cerr << "[Axiom Engine] Out of VRAM: KV cache allocation failed at layer " << l << "\n";
			return false;
		}
	}

	cached_tokens_.clear();

	std::cout << "[Axiom Engine] Loaded model: " << config_.num_layers << " layers, hidden_dim="
		  << config_.hidden_dim << ", ffn_dim=" << config_.ffn_dim << " ("
		  << (weights_bytes >> 20) << " MB VRAM)\n";
	return true;
}

void AI::forward_transformer_step(int token_id, int token_pos, float* d_out_logits, void* stream) {
	const uint32_t D   = config_.hidden_dim;
	const uint32_t Qd  = config_.num_q_heads * config_.head_dim;
	const uint32_t KVd = config_.num_kv_heads * config_.head_dim;
	const uint32_t FFN = config_.ffn_dim;

	uint32_t u_token_id = static_cast<uint32_t>(token_id);
	int32_t  s_token_pos = static_cast<int32_t>(token_pos);
	upload_device_buffer(d_curr_token_, &u_token_id, sizeof(uint32_t));
	upload_device_buffer(d_curr_pos_, &s_token_pos, sizeof(int32_t));

	launch_gather_embeddings_batched(
		static_cast<const uint32_t*>(d_curr_token_),
		d_embed_tokens_,
		d_residual_stream_,
		1, D, config_.vocab_size, stream
	);

	for (uint32_t l = 0; l < config_.num_layers; ++l) {
		const auto& w = layers_[l];

		launch_rmsnorm_bf16(d_residual_stream_, w.d_input_layernorm, d_normed_x_, 1, D, config_.rms_norm_eps, stream);

		launch_cutlass_gemm_bf16(1, Qd,  D, d_normed_x_, w.d_q_proj_weight, w.d_q_proj_bias, d_q_, stream);
		launch_cutlass_gemm_bf16(1, KVd, D, d_normed_x_, w.d_k_proj_weight, w.d_k_proj_bias, d_k_, stream);
		launch_cutlass_gemm_bf16(1, KVd, D, d_normed_x_, w.d_v_proj_weight, w.d_v_proj_bias, d_v_, stream);

		launch_rope_inplace_bf16(
			d_q_, d_k_, static_cast<const int32_t*>(d_curr_pos_), 1,
			config_.num_q_heads, config_.num_kv_heads, config_.head_dim, config_.rope_theta, stream
		);

		launch_append_paged_kv(
			d_k_, d_v_, d_k_cache_[l], d_v_cache_[l], static_cast<const int32_t*>(d_block_table_),
			token_pos, config_.num_kv_heads, config_.head_dim, stream
		);

		float scale = 1.0f / std::sqrt(static_cast<float>(config_.head_dim));
		launch_paged_gqa_decode(
			d_q_, d_k_cache_[l], d_v_cache_[l], static_cast<const int32_t*>(d_block_table_), d_attn_out_,
			token_pos + 1, config_.num_q_heads, config_.num_kv_heads, config_.head_dim, scale, stream
		);

		launch_cutlass_gemm_bf16(1, D, Qd, d_attn_out_, w.d_o_proj_weight, nullptr, d_proj_tmp_, stream);
		launch_residual_add_bf16(d_residual_stream_, d_proj_tmp_, D, stream);

		launch_rmsnorm_bf16(d_residual_stream_, w.d_post_attn_layernorm, d_normed_x_, 1, D, config_.rms_norm_eps, stream);

		launch_cutlass_gemm_bf16(1, FFN, D, d_normed_x_, w.d_gate_proj_weight, nullptr, d_gate_buf_, stream);
		launch_cutlass_gemm_bf16(1, FFN, D, d_normed_x_, w.d_up_proj_weight,   nullptr, d_up_buf_,   stream);
		launch_swiglu_bf16(d_gate_buf_, d_up_buf_, d_act_buf_, FFN, stream);
		launch_cutlass_gemm_bf16(1, D, FFN, d_act_buf_,  w.d_down_proj_weight, nullptr, d_proj_tmp_, stream);
		launch_residual_add_bf16(d_residual_stream_, d_proj_tmp_, D, stream);
	}

	launch_rmsnorm_bf16(d_residual_stream_, d_final_norm_weight_, d_normed_x_, 1, D, config_.rms_norm_eps, stream);
	if (d_out_logits) {
		launch_tied_logits_bf16(d_normed_x_, d_lm_head_weight_, d_out_logits, 1, config_.vocab_size, D, stream);
	}
}

bool AI::generate(const std::string& prompt,
		  size_t max_new_tokens,
		  float temperature,
		  int top_k,
		  float top_p,
		  std::function<void(const std::string&)> token_callback) {
	std::lock_guard<std::mutex> lock(inference_mutex_);
	if (!d_weights_arena_ || qwen_vocab_.empty()) {
		std::cerr << "[Axiom Engine] Cannot generate: model or vocabulary not loaded.\n";
		return false;
	}

	std::vector<uint32_t> prompt_tokens = Parser::tokenize_exact_code(prompt, qwen_vocab_);
	if (prompt_tokens.empty()) {
		return false;
	}

	// Reserve room for generation while allowing full VRAM context window (up to max_seq_len)
	const size_t reserve_tokens = std::min<size_t>(max_new_tokens + 64, config_.max_seq_len / 2);
	const size_t max_allowed_prompt = (config_.max_seq_len > reserve_tokens)
	                                      ? (config_.max_seq_len - reserve_tokens)
	                                      : (config_.max_seq_len / 2);

	if (prompt_tokens.size() > max_allowed_prompt) {
		size_t excess = prompt_tokens.size() - max_allowed_prompt;
		// Keep the head (system instructions + tool schema) and the tail (active file context + user query)
		const size_t head_keep = max_allowed_prompt / 3;
		prompt_tokens.erase(prompt_tokens.begin() + head_keep,
		                    prompt_tokens.begin() + head_keep + excess);
	}

	size_t logits_bytes = config_.vocab_size * sizeof(float);
	float* d_logits = static_cast<float*>(d_logits_);
	bool allocated_local_logits = false;
	if (!d_logits) {
		d_logits = static_cast<float*>(allocate_device_memory(logits_bytes));
		allocated_local_logits = true;
	}
	if (!d_logits) return false;

	std::vector<float> h_logits(config_.vocab_size);

	// KV Prefix Cache: reuse already-prefilled tokens across multi-turn Cline/ComfyUI requests
	size_t prefix_match = 0;
	const size_t max_prefix = std::min(cached_tokens_.size(), prompt_tokens.size());
	while (prefix_match < max_prefix && cached_tokens_[prefix_match] == prompt_tokens[prefix_match]) {
		++prefix_match;
	}
	// Always re-run at least the final prompt token to produce fresh d_logits
	if (prefix_match >= prompt_tokens.size() && !prompt_tokens.empty()) {
		prefix_match = prompt_tokens.size() - 1;
	}

	// 1. Prefill only the uncached suffix of the prompt
	for (size_t i = prefix_match; i < prompt_tokens.size(); ++i) {
		float* target_logits = (i + 1 == prompt_tokens.size()) ? d_logits : nullptr;
		forward_transformer_step(prompt_tokens[i], static_cast<int>(i), target_logits, compute_stream_);
	}
	sync_cuda_stream(compute_stream_);

	cached_tokens_ = prompt_tokens;

	int current_pos = static_cast<int>(prompt_tokens.size());
	std::random_device rd;
	std::mt19937 rng(rd());
	std::vector<std::pair<float, uint32_t>> candidates;
	std::string generated_so_far;

	// 2. Autoregressive decode
	for (size_t step = 0; step < max_new_tokens; ++step) {
		if (current_pos >= static_cast<int>(config_.max_seq_len)) break;

		uint32_t next_token = 0;
		if (temperature <= 0.01f && d_pred_id_) {
			launch_softmax_argmax(d_logits, static_cast<uint32_t*>(d_pred_id_), config_.vocab_size, compute_stream_);
			sync_cuda_stream(compute_stream_);
			download_device_buffer(&h_pred_id_, d_pred_id_, sizeof(uint32_t));
			next_token = h_pred_id_;
			if (step == 0 && (next_token == 151643 || next_token == 151644 || next_token == 151645)) {
				download_device_buffer(h_logits.data(), d_logits, logits_bytes);
				float best_val = -1e30f;
				for (uint32_t v = 0; v < config_.vocab_size; ++v) {
					if (v >= 151643 && v <= 151645) continue;
					if (std::isfinite(h_logits[v]) && h_logits[v] > best_val) {
						best_val = h_logits[v];
						next_token = v;
					}
				}
			}
		} else {
			download_device_buffer(h_logits.data(), d_logits, logits_bytes);
			if (step == 0 && config_.vocab_size > 151645) {
				h_logits[151643] = -1e30f;
				h_logits[151644] = -1e30f;
				h_logits[151645] = -1e30f;
			}
			float inv_temp = 1.0f / std::max(temperature, 0.01f);
			candidates.clear();

			// Fast threshold filter: track running max to avoid sorting all 151,936 logits
			float max_scaled = -1e30f;
			for (uint32_t v = 0; v < config_.vocab_size; ++v) {
				if (std::isfinite(h_logits[v])) {
					float scaled = h_logits[v] * inv_temp;
					if (scaled > max_scaled) max_scaled = scaled;
				}
			}
			// Any logit more than 20.0 below max_scaled has exp(-20) < 2e-9 probability
			const float cutoff_logit = max_scaled - 20.0f;
			for (uint32_t v = 0; v < config_.vocab_size; ++v) {
				if (std::isfinite(h_logits[v])) {
					float scaled = h_logits[v] * inv_temp;
					if (scaled >= cutoff_logit) {
						candidates.emplace_back(scaled, v);
					}
				}
			}

			if (candidates.empty()) {
				next_token = 0;
			} else {
				const size_t effective_k = (top_k > 0)
					? std::min(static_cast<size_t>(top_k), candidates.size())
					: std::min<size_t>(64, candidates.size());
				if (effective_k < candidates.size()) {
					std::partial_sort(candidates.begin(),
					                  candidates.begin() + effective_k,
					                  candidates.end(),
					                  [](const auto& a, const auto& b) { return a.first > b.first; });
					candidates.resize(effective_k);
				} else {
					std::sort(candidates.begin(),
					          candidates.end(),
					          [](const auto& a, const auto& b) { return a.first > b.first; });
				}

				float max_val = candidates[0].first;
				float sum_exp = 0.0f;
				for (auto& c : candidates) {
					c.first = std::exp(c.first - max_val);
					sum_exp += c.first;
				}
				if (sum_exp > 0.0f && top_p > 0.0f && top_p < 1.0f) {
					float cumulative = 0.0f;
					size_t cutoff = candidates.size();
					for (size_t idx = 0; idx < candidates.size(); ++idx) {
						cumulative += candidates[idx].first / sum_exp;
						if (cumulative >= top_p) {
							cutoff = idx + 1;
							break;
						}
					}
					candidates.resize(cutoff);
					sum_exp = 0.0f;
					for (const auto& c : candidates) {
						sum_exp += c.first;
					}
				}
				std::uniform_real_distribution<float> dist(0.0f, sum_exp);
				float r = dist(rng);
				float accum = 0.0f;
				next_token = candidates[0].second;
				for (const auto& c : candidates) {
					accum += c.first;
					if (accum >= r) {
						next_token = c.second;
						break;
					}
				}
			}
		}

		// Check EOS (<|endoftext|> = 151643, <|im_start|> = 151644, <|im_end|> = 151645)
		if (next_token == 151643 || next_token == 151644 || next_token == 151645) {
			break;
		}

		cached_tokens_.push_back(next_token);

		auto it = qwen_reverse_vocab_.find(next_token);
		if (it != qwen_reverse_vocab_.end()) {
			generated_so_far += it->second;
			if (token_callback) token_callback(it->second);

			// ONLY stop on complete outer XML tool closing tags for Cline
			if (generated_so_far.find("</replace_in_file>") != std::string::npos ||
			    generated_so_far.find("</write_to_file>") != std::string::npos ||
			    generated_so_far.find("</read_file>") != std::string::npos ||
			    generated_so_far.find("</execute_command>") != std::string::npos ||
			    generated_so_far.find("</search_files>") != std::string::npos ||
			    generated_so_far.find("</list_files>") != std::string::npos ||
			    generated_so_far.find("</attempt_completion>") != std::string::npos ||
			    generated_so_far.find("</ask_followup_question>") != std::string::npos) {
				// Feed the final token into KV cache before breaking so prefix cache stays valid
				forward_transformer_step(next_token, current_pos, nullptr, compute_stream_);
				sync_cuda_stream(compute_stream_);
				break;
			}
		}

		forward_transformer_step(next_token, current_pos, d_logits, compute_stream_);
		sync_cuda_stream(compute_stream_);
		current_pos++;
	}

	if (allocated_local_logits) {
		free_device_memory(d_logits);
	}
	return true;
}
