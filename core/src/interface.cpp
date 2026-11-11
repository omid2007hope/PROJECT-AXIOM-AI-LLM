/*
 * Copyright (c) 2026 Omid Teimory. All Rights Reserved.
 *
 * NOTICE: This file contains proprietary source code licensed exclusively under the
 * terms of the LICENSE file in the root directory of this repository.
 * You may view, download, and compile this code solely to run the unmodified product.
 * Copying, pasting, modifying, redistributing, creating derivative works, or using
 * this code in any other project or AI training system is strictly prohibited.
 * Removal or alteration of this notice is a violation of 17 U.S.C. § 1202 and EU law.
 */

#include "../include/ai.hpp"
#include "../include/parser.hpp"
#include "../src/CUDA/transfer.cuh"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <unordered_set>

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
	if (enable_transformer && gpu_weights.d_query_weights != nullptr &&
	    gpu_portion.d_context_window[0] != nullptr) {
		const size_t model_dim = embedding_dim > 0 ? embedding_dim : target_elements_y_total;
		const size_t sequence_length = context_ids.size();
		size_t heads = (num_heads > 0 && model_dim % num_heads == 0) ? num_heads : 1;
		if (model_dim / heads > 64 && model_dim % 64 == 0) {
			heads = model_dim / 64;
		}
		const size_t feedforward_network_dimension =
			(ffn_dimension > 0 && gpu_weights.transformer_dim == model_dim) ? ffn_dimension
																			: (model_dim * 4);

		launch_gather_embeddings_batched(
			static_cast<const uint32_t*>(gpu_portion.d_token_ids[0]), gpu_embeddings.d_table,
			gpu_portion.d_context_window[0], static_cast<uint32_t>(sequence_length),
			static_cast<uint32_t>(model_dim), static_cast<uint32_t>(vocab_size),
			gpu_portion.stream[0]);

		if (!transformer_forward_block(gpu_portion.d_context_window[0], &gpu_weights,
									   &gpu_activations, sequence_length, model_dim, heads,
									   feedforward_network_dimension, gpu_portion.stream[0])) {
			std::cerr << "[AI Brain] Transformer forward block failed during prediction.\n";
			return false;
		}

		gpu_portion.token_ids_active[0] = false;
		const bool fwd_ok = forward_pass_with_embeddings(
			&gpu_portion, &gpu_embeddings, &gpu_weights, &gpu_activations,
			input_elements_x_total, target_elements_y_total, 0);
		gpu_portion.token_ids_active[0] = true;
		if (!fwd_ok) {
			std::cerr << "[AI Brain] Forward pass failed during prediction.\n";
			return false;
		}
	} else if (!forward_pass_with_embeddings(&gpu_portion,
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

		// Repetition penalty window: tracks recently emitted tokens to prevent absorbing Markov loops
		std::unordered_set<uint32_t> recent_tokens;
		const size_t lookback = std::min<size_t>(context_ids.size(), 64);
		for (size_t k = context_ids.size() - lookback; k < context_ids.size(); ++k) {
			if (context_ids[k] != 0 && context_ids[k] != vocab::PAD) {
				recent_tokens.insert(context_ids[k]);
			}
		}
		constexpr float repetition_penalty = 1.35f;

		if (config.temperature <= 1e-4f) {
			size_t vocab_sz = (active_vocab_size > 0) ? active_vocab_size : vocab_size;
			std::vector<float> probabilities(vocab_sz, 0.0f);
			if (download_probabilities(&gpu_activations, probabilities.data(), vocab_sz)) {
				float best_prob = -1.0f;
				for (uint32_t id = 1; id < vocab_sz; ++id) {
					float p = probabilities[id];
					if (recent_tokens.find(id) != recent_tokens.end()) {
						p /= repetition_penalty;
					}
					if (p > best_prob) {
						best_prob = p;
						token_id = id;
					}
				}
			}
			if (token_id == 0) {
				token_id = last_predicted_token_id;
			}
		} else {
			size_t vocab_sz = (active_vocab_size > 0) ? active_vocab_size : vocab_size;
			std::vector<float> probabilities(vocab_sz, 0.0f);
			if (download_probabilities(
				    &gpu_activations, probabilities.data(), vocab_sz)) {
				std::vector<std::pair<float, uint32_t>> candidates;
				candidates.reserve(vocab_sz);
				for (uint32_t id = 1; id < vocab_sz; ++id) {
					float p = probabilities[id];
					if (p > 0.0f) {
						if (recent_tokens.find(id) != recent_tokens.end()) {
							p /= repetition_penalty;
						}
						candidates.emplace_back(p, id);
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
