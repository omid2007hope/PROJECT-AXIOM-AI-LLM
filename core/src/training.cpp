// SPDX-License-Identifier: LicenseRef-Axiom-Proprietary
/*
 * Copyright (c) 2026 Omid Teimory. All Rights Reserved.
 *
 * PROPRIETARY SOURCE-AVAILABLE SOFTWARE — NOT OPEN SOURCE.
 * Permission to view, clone, or compile this file is granted strictly on the
 * condition precedent of full compliance with the root ./LICENSE file.
 * Copying snippets, modifying, creating derivative works, redistributing,
 * commercial/SaaS use, and AI/ML training (reserved under EU Directive
 * 2019/790 Art. 4(3) and § 42h(6) Austrian UrhG) are strictly prohibited.
 * Removal or alteration of this notice violates 17 U.S.C. § 1202 and § 90c UrhG.
 */

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "../include/ai.hpp"
#include "../include/optimizer.hpp"
#include "../src/CUDA/transfer.cuh"

TrainStatus AI::train(Optimizer& optimizer, Portion& portion) {
	// Sync the stream to ensure the GPU is done with this buffer before we overwrite it.
	synchronize_stream(&gpu_portion, portion.buffer_index);

	bool has_data = optimizer.load_context_window(portion);
	if (!has_data) {
		return TrainStatus::NoData;
	}

	int buffer_index = portion.buffer_index;
	float learning_rate = optimizer.learning_rate;

	if (portion.has_token_ids && portion.context_token_ids[buffer_index] != nullptr &&
		gpu_embeddings.d_table != nullptr) {
		const size_t sequence_length = std::min<size_t>(portion.context_window_size, 2048);

		// Transfer token IDs across PCIe per context window
		if (!upload_tokens_to_gpu(portion.context_token_ids[buffer_index], sequence_length,
								  portion.target_tokens[buffer_index], portion.target_elements,
								  &gpu_portion, buffer_index)) {
			std::cerr << "[AI Brain] Failed to upload tokens to GPU.\n";
			return TrainStatus::Failed;
		}

		gpu_embeddings.active_vocab_size = active_vocab_size;

		if (enable_transformer) {
			synchronize_stream(&gpu_portion, 1 - buffer_index);

			const size_t model_dim = (embedding_dim > 0) ? embedding_dim : ((target_elements_y_total > 0) ? target_elements_y_total : axiom::TOKEN_DIM);
			size_t heads = (num_heads > 0 && model_dim % num_heads == 0) ? num_heads : axiom::NUM_HEADS;
			if (model_dim / heads > axiom::MAX_HEAD_DIM && model_dim % axiom::MAX_HEAD_DIM == 0) {
				heads = model_dim / axiom::MAX_HEAD_DIM;
			}
			const size_t feedforward_network_dimension =
				(ffn_dimension > 0 && gpu_weights.transformer_dim == model_dim) ? ffn_dimension
																				: axiom::FFN_DIM;

			if (gpu_weights.d_query_weights == nullptr ||
				gpu_weights.transformer_dim != model_dim) {
				if (!build_transformer_block(model_dim, heads, feedforward_network_dimension)) {
					std::cerr << "[AI Brain] Failed to build transformer block.\n";
					return TrainStatus::Failed;
				}
			}

			void* stream = gpu_portion.stream[buffer_index];
			const size_t sequence_elements = sequence_length * model_dim;

			for (size_t i = 0; i < gpu_portion.context_ptr; ++i) {
				if (gpu_portion.context_elements != input_elements_x_total ||
					!gpu_portion.d_context_window[i]) {
					if (gpu_portion.d_context_window[i]) {
						free_vram(gpu_portion.d_context_window[i], gpu_portion.stream[i]);
					}
					gpu_portion.d_context_window[i] = allocate_vram(
						input_elements_x_total * sizeof(std::bfloat16_t), gpu_portion.stream[i]);
				}
			}
			gpu_portion.context_elements = input_elements_x_total;

			if (!gpu_portion.d_hidden_state[buffer_index]) {
				gpu_portion.d_hidden_state[buffer_index] =
					allocate_vram(sequence_elements * sizeof(std::bfloat16_t), stream);
			}
			if (!gpu_portion.d_dx[buffer_index]) {
				gpu_portion.d_dx[buffer_index] =
					allocate_vram(sequence_elements * sizeof(float), stream);
			}
			if (!gpu_portion.d_context_window[buffer_index] ||
				!gpu_portion.d_hidden_state[buffer_index] || !gpu_portion.d_dx[buffer_index]) {
				std::cerr << "[AI Brain] Failed to allocate sequence buffers in VRAM.\n";
				return TrainStatus::Failed;
			}

			// Gather [M = S x D] token embeddings into context and saved hidden_state buffers
			launch_gather_embeddings_batched(
				static_cast<const uint32_t*>(gpu_portion.d_token_ids[buffer_index]),
				gpu_embeddings.d_table, gpu_portion.d_context_window[buffer_index],
				static_cast<uint32_t>(sequence_length), static_cast<uint32_t>(model_dim),
				static_cast<uint32_t>(vocab_size), stream);
			launch_gather_embeddings_batched(
				static_cast<const uint32_t*>(gpu_portion.d_token_ids[buffer_index]),
				gpu_embeddings.d_table, gpu_portion.d_hidden_state[buffer_index],
				static_cast<uint32_t>(sequence_length), static_cast<uint32_t>(model_dim),
				static_cast<uint32_t>(vocab_size), stream);

			// Sequence-parallel Transformer Forward Block (M = S)
			if (!transformer_forward_block(gpu_portion.d_context_window[buffer_index], &gpu_weights,
										   &gpu_activations, sequence_length, model_dim, heads,
										   feedforward_network_dimension, stream)) {
				std::cerr << "[AI Brain] Transformer forward block failed.\n";
				return TrainStatus::Failed;
			}

			// Readout forward projection: gather_embeddings is false because d_context_window has transformed states
			const bool fwd_ok = forward_pass_with_embeddings(
				&gpu_portion, &gpu_embeddings, &gpu_weights, &gpu_activations,
				input_elements_x_total, target_elements_y_total, buffer_index, false /* gather_embeddings */);
			if (!fwd_ok) {
				std::cerr << "[AI Brain] Forward pass with embeddings failed.\n";
				return TrainStatus::Failed;
			}

			// Readout loss, telemetry, and upstream gradient d_dx computation.
			// scatter_embeddings is set to true so output head updates and embedding matrix learns
			if (!backward_pass_with_embeddings(&gpu_portion, &gpu_embeddings, &gpu_weights,
											   &gpu_activations, input_elements_x_total,
											   target_elements_y_total, learning_rate, buffer_index,
											   true /* use_adamw */, true /* scatter_embeddings */)) {
				std::cerr << "[AI Brain] Backward pass with embeddings failed.\n";
				return TrainStatus::Failed;
			}

			// Sequence-parallel Transformer Backward Block (M = S)
			// Backpropagates d_dx (readout gradient) through SwiGLU and Attention into d_dx (input embedding gradient)
			if (!transformer_backward_block(gpu_portion.d_dx[buffer_index],
											gpu_portion.d_dx[buffer_index],
											gpu_portion.d_hidden_state[buffer_index], &gpu_weights,
											&gpu_activations, sequence_length, model_dim, heads,
											feedforward_network_dimension, learning_rate, stream)) {
				std::cerr << "[AI Brain] Transformer backward block failed.\n";
				return TrainStatus::Failed;
			}

			// Scatter the true post-transformer input gradient into the embedding table
			if (!backward_scatter_embeddings(&gpu_portion, &gpu_embeddings,
											 gpu_portion.d_dx[buffer_index], sequence_length,
											 model_dim, learning_rate, buffer_index, stream)) {
				std::cerr << "[AI Brain] Backward scatter embeddings failed.\n";
				return TrainStatus::Failed;
			}
		} else {
			// Launch CUDA gather kernel + CUTLASS EVT GEMM forward pass
			if (!forward_pass_with_embeddings(&gpu_portion, &gpu_embeddings, &gpu_weights,
											  &gpu_activations, input_elements_x_total,
											  target_elements_y_total, buffer_index)) {
				std::cerr << "[AI Brain] Forward pass with embeddings failed.\n";
				return TrainStatus::Failed;
			}

			// Launch CUDA fused AdamW + backward scatter embedding update
			if (!backward_pass_with_embeddings(&gpu_portion, &gpu_embeddings, &gpu_weights,
											   &gpu_activations, input_elements_x_total,
											   target_elements_y_total, learning_rate, buffer_index,
											   true /* use_adamw */)) {
				std::cerr << "[AI Brain] Backward pass with embeddings failed.\n";
				return TrainStatus::Failed;
			}
		}
	} else {
		// Fallback for float-based training
		if (!upload_to_gpu(portion.context_window_tokens[buffer_index], portion.all_elements,
						   portion.target_tokens[buffer_index], portion.target_elements,
						   &gpu_portion, buffer_index)) {
			std::cerr << "[AI Brain] Failed to upload portion to GPU.\n";
			return TrainStatus::Failed;
		}

		if (!forward_pass(&gpu_portion, &gpu_weights, &gpu_activations, input_elements_x_total,
						  target_elements_y_total, buffer_index)) {
			std::cerr << "[AI Brain] Forward pass failed.\n";
			return TrainStatus::Failed;
		}

		if (!backward_pass(&gpu_portion, &gpu_weights, &gpu_activations, input_elements_x_total,
						   target_elements_y_total, learning_rate, buffer_index)) {
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
			std::cerr << "[AI Brain] Failed to download weights from GPU before saving.\n";
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
		std::cerr << "[AI Brain] Failed to open temporary file for writing: " << temp_path << "\n";
		return false;
	}

	out.write(reinterpret_cast<const char*>(&header), sizeof(header));
	out.write(reinterpret_cast<const char*>(host_weights.data()),
			  header.weight_count * sizeof(float));
	out.write(reinterpret_cast<const char*>(host_bias.data()), header.bias_count * sizeof(float));
	out.write(reinterpret_cast<const char*>(host_embeddings.data()),
			  header.vocab_size * header.embedding_dim * sizeof(float));

	// If transformer block is allocated, serialize transformer attention & SwiGLU weights
	const bool has_transformer = (enable_transformer && gpu_weights.d_query_weights != nullptr &&
								  gpu_weights.transformer_dim > 0 && gpu_weights.transformer_ffn > 0);
	if (has_transformer) {
		const size_t xform_dim = gpu_weights.transformer_dim;
		const size_t xform_ffn = gpu_weights.transformer_ffn;
		const size_t dd_elements = xform_dim * xform_dim;
		const size_t df_elements = xform_dim * xform_ffn;

		std::vector<float> host_q(dd_elements);
		std::vector<float> host_k(dd_elements);
		std::vector<float> host_v(dd_elements);
		std::vector<float> host_o(dd_elements);
		std::vector<float> host_gate(df_elements);
		std::vector<float> host_up(df_elements);
		std::vector<float> host_down(df_elements);

		if (!download_transformer_weights(&gpu_weights,
										  host_q.data(), host_k.data(), host_v.data(), host_o.data(),
										  host_gate.data(), host_up.data(), host_down.data(),
										  xform_dim, xform_ffn)) {
			std::cerr << "[AI Brain] Failed to download transformer weights from GPU.\n";
			out.close();
			std::error_code ec;
			std::filesystem::remove(temp_path, ec);
			return false;
		}

		auto check_finite = [](const std::vector<float>& vec) {
			for (float val : vec) {
				if (!std::isfinite(val)) return false;
			}
			return true;
		};

		if (!check_finite(host_q) || !check_finite(host_k) || !check_finite(host_v) ||
			!check_finite(host_o) || !check_finite(host_gate) || !check_finite(host_up) ||
			!check_finite(host_down)) {
			std::cerr << "[AI Brain] Refusing to save corrupt checkpoint: transformer weights contain NaN or Inf.\n";
			out.close();
			std::error_code ec;
			std::filesystem::remove(temp_path, ec);
			return false;
		}

		const uint64_t xform_magic = 0x5458464DU;
		const uint64_t xform_dim_u64 = xform_dim;
		const uint64_t xform_ffn_u64 = xform_ffn;

		out.write(reinterpret_cast<const char*>(&xform_magic), sizeof(xform_magic));
		out.write(reinterpret_cast<const char*>(&xform_dim_u64), sizeof(xform_dim_u64));
		out.write(reinterpret_cast<const char*>(&xform_ffn_u64), sizeof(xform_ffn_u64));

		out.write(reinterpret_cast<const char*>(host_q.data()), dd_elements * sizeof(float));
		out.write(reinterpret_cast<const char*>(host_k.data()), dd_elements * sizeof(float));
		out.write(reinterpret_cast<const char*>(host_v.data()), dd_elements * sizeof(float));
		out.write(reinterpret_cast<const char*>(host_o.data()), dd_elements * sizeof(float));
		out.write(reinterpret_cast<const char*>(host_gate.data()), df_elements * sizeof(float));
		out.write(reinterpret_cast<const char*>(host_up.data()), df_elements * sizeof(float));
		out.write(reinterpret_cast<const char*>(host_down.data()), df_elements * sizeof(float));
	}

	out.flush();
	if (!out.good()) {
		std::cerr << "[AI Brain] Stream error while writing weights to: " << temp_path << "\n";
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
		std::filesystem::copy_file(temp_path, path,
								   std::filesystem::copy_options::overwrite_existing, ec);
		std::filesystem::remove(temp_path, ec);
		if (ec) {
			std::cerr << "[AI Brain] Failed to rename/copy " << temp_path << " to " << path << ": "
					  << ec.message() << "\n";
			return false;
		}
	}

	std::cout << "[AI Brain] Saved " << header.weight_count << " weights, " << header.bias_count
			  << " biases, and " << (header.vocab_size * header.embedding_dim) << " embeddings to "
			  << path << "\n"
			  << std::flush;
	return true;
}

bool AI::read_loss(double& mean_loss, size_t& steps) {
	TrainingTelemetry telemetry {};
	if (!read_telemetry(telemetry)) {
		return false;
	}
	mean_loss = telemetry.mean_loss;
	steps = telemetry.steps;
	return true;
}

bool AI::read_telemetry(TrainingTelemetry& telemetry) {
	// index:  0 1 2 3 4
	float slots[telemetry_slot_count] = {};

	// unable to download data from GPU
	if (!download_telemetry(&gpu_activations, slots)) {
		return false;
	}

	telemetry.steps = pending_steps;
	pending_steps = 0;

	const double step_count = (telemetry.steps > 0) ? static_cast<double>(telemetry.steps) : 1.0;

	const double output_count =
		static_cast<double>((telemetry.steps > 0 ? telemetry.steps : 1) *
							(target_elements_y_total > 0 ? target_elements_y_total : 1));

	telemetry.mean_loss =
		(telemetry.steps > 0) ? static_cast<double>(slots[telemetry_loss]) / step_count : 0.0;

	telemetry.gradient_norm =
		std::sqrt(static_cast<double>(slots[telemetry_gradient_square]) / step_count);
	telemetry.nonfinite_count = static_cast<size_t>(slots[telemetry_nonfinite]);
	telemetry.dead_fraction = static_cast<double>(slots[telemetry_dead_output]) / output_count;
	telemetry.activation_magnitude =
		std::sqrt(static_cast<double>(slots[telemetry_activation_square]) / output_count);

	return true;
}

bool AI::evaluate_validation(const std::vector<uint32_t>& validation_tokens, size_t window_size,
							 double& validation_loss) {
	if (window_size == 0 || validation_tokens.size() <= window_size ||
		gpu_embeddings.d_table == nullptr) {
		return false;
	}

	synchronize_stream(&gpu_portion, 0);
	synchronize_stream(&gpu_portion, 1);

	const size_t total_windows = validation_tokens.size() - window_size;
	const size_t max_windows = std::min<size_t>(8, total_windows);
	const size_t stride = (total_windows > max_windows) ? (total_windows / max_windows) : 1;
	const size_t vocab_bound = (active_vocab_size > 0) ? active_vocab_size : vocab_size;

	std::vector<std::bfloat16_t> dummy_target(target_elements_y_total,
											  static_cast<std::bfloat16_t>(0.0f));
	std::vector<float> probabilities(vocab_bound, 0.0f);

	double total_loss = 0.0;
	size_t evaluated = 0;

	gpu_embeddings.active_vocab_size = active_vocab_size;

	if (enable_transformer) {
		const size_t model_dim = embedding_dim > 0 ? embedding_dim : target_elements_y_total;
		size_t heads = (num_heads > 0 && model_dim % num_heads == 0) ? num_heads : 1;
		if (model_dim / heads > 64 && model_dim % 64 == 0) {
			heads = model_dim / 64;
		}
		const size_t feedforward_network_dimension =
			(ffn_dimension > 0 && gpu_weights.transformer_dim == model_dim) ? ffn_dimension
																			: (model_dim * 4);

		if (gpu_weights.d_query_weights == nullptr || gpu_weights.transformer_dim != model_dim ||
			gpu_weights.transformer_ffn != feedforward_network_dimension) {
			if (!build_transformer_block(model_dim, heads, feedforward_network_dimension)) {
				return false;
			}
		}

		if (gpu_portion.d_context_window[0] == nullptr ||
			gpu_portion.context_elements != input_elements_x_total) {
			if (gpu_portion.d_context_window[0]) {
				free_vram(gpu_portion.d_context_window[0], gpu_portion.stream[0]);
			}
			gpu_portion.d_context_window[0] = allocate_vram(
				input_elements_x_total * sizeof(std::bfloat16_t), gpu_portion.stream[0]);
			if (!gpu_portion.d_context_window[0]) {
				return false;
			}
			gpu_portion.context_elements = input_elements_x_total;
		}
	}

	for (size_t idx = 0; idx < max_windows; ++idx) {
		const size_t start = idx * stride;
		if (start + window_size >= validation_tokens.size()) {
			break;
		}
		const uint32_t target_id = validation_tokens[start + window_size];
		if (target_id == 0 || target_id >= vocab_bound) {
			continue;
		}

		if (!upload_tokens_to_gpu(validation_tokens.data() + start, window_size,
								  dummy_target.data(), dummy_target.size(), &gpu_portion, 0)) {
			return false;
		}

		if (enable_transformer && gpu_weights.d_query_weights != nullptr &&
			gpu_portion.d_context_window[0] != nullptr) {
			const size_t model_dim = (embedding_dim > 0) ? embedding_dim : ((target_elements_y_total > 0) ? target_elements_y_total : axiom::TOKEN_DIM);
			size_t heads = (num_heads > 0 && model_dim % num_heads == 0) ? num_heads : axiom::NUM_HEADS;
			if (model_dim / heads > axiom::MAX_HEAD_DIM && model_dim % axiom::MAX_HEAD_DIM == 0) {
				heads = model_dim / axiom::MAX_HEAD_DIM;
			}
			const size_t feedforward_network_dimension =
				(ffn_dimension > 0 && gpu_weights.transformer_dim == model_dim) ? ffn_dimension
																				: axiom::FFN_DIM;

			launch_gather_embeddings_batched(
				static_cast<const uint32_t*>(gpu_portion.d_token_ids[0]), gpu_embeddings.d_table,
				gpu_portion.d_context_window[0], static_cast<uint32_t>(window_size),
				static_cast<uint32_t>(model_dim), static_cast<uint32_t>(vocab_size),
				gpu_portion.stream[0]);

			if (!transformer_forward_block(gpu_portion.d_context_window[0], &gpu_weights,
										   &gpu_activations, window_size, model_dim, heads,
										   feedforward_network_dimension, gpu_portion.stream[0])) {
				return false;
			}

			const bool fwd_ok = forward_pass_with_embeddings(
				&gpu_portion, &gpu_embeddings, &gpu_weights, &gpu_activations,
				input_elements_x_total, target_elements_y_total, 0, false /* gather_embeddings */);
			if (!fwd_ok) {
				return false;
			}
		} else if (!forward_pass_with_embeddings(&gpu_portion, &gpu_embeddings, &gpu_weights,
												 &gpu_activations, input_elements_x_total,
												 target_elements_y_total, 0)) {
			return false;
		}

		synchronize_stream(&gpu_portion, 0);

		if (!download_probabilities(&gpu_activations, probabilities.data(), vocab_bound)) {
			return false;
		}

		const uint32_t safe_target = (target_id < vocab_bound) ? target_id : 0U;
		const float target_prob = std::max(probabilities[safe_target], 1e-35f);
		total_loss += static_cast<double>(-std::log(target_prob));
		++evaluated;
	}

	if (evaluated == 0) {
		return false;
	}

	validation_loss = total_loss / static_cast<double>(evaluated);
	return true;
}
