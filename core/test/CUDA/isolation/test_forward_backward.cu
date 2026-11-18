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

/*
Isolate the forward and backward pass - feed then fake false emmbeding data - check the out put live - loss drop per 1000 step - if drops - no abnormality - anomalies - pass green - then we find that forward pass and backward pass in isolation work perfectly which makes the question that why when compile the full app the loss is not okay!
*/

#include <vector>

#include "../../../src/CUDA/kernel/backward_pass.cu"
#include "../../../src/CUDA/kernel/forward_pass.cu"
#include "../../src/CUDA/util/gpu_types.cuh"



struct TestGpuPortion {
	void* d_token_ids[2] = {nullptr, nullptr};
	void* d_context_window[2] = {nullptr, nullptr};
	void* d_target[2] = {nullptr, nullptr};
	void* d_dx[2] = {nullptr, nullptr};

	size_t context_ptr = sizeof(d_context_window) / sizeof(d_context_window[0]);
	size_t target_ptr = sizeof(d_target) / sizeof(d_target[0]);

	void* stream[2] = {nullptr, nullptr};

	void* compute_done[2] = {nullptr, nullptr};
	bool compute_done_recorded[2] = {false, false};

	bool token_ids_active[2] = {false, false};

	size_t token_count = 0;
	size_t context_elements = 0;
	size_t target_elements = 0;

	void* d_hidden_state[2] = {nullptr, nullptr};
	void* d_loss_scalar[2] = {nullptr, nullptr};
	float* h_pinned_loss[2] = {nullptr, nullptr};
	void* loss_ready_event[2] = {nullptr, nullptr};

	size_t max_batch_tokens = 0;
	bool is_allocated = false;
};

struct TestGpuEmbeddings {
	void* d_table = nullptr;
	void* d_table_master = nullptr;
	size_t vocab_size = 0;
	size_t embedding_dim = 0;
	size_t active_vocab_size = 0;

	void* d_m = nullptr;
	void* d_v = nullptr;

	float temperature = 1.0f;
};

struct TestGpuWeights {
	void* d_weights = nullptr;
	void* d_bias = nullptr;

	void* d_weights_master = nullptr;
	void* d_bias_master = nullptr;

	void* d_weights_m = nullptr;
	void* d_weights_v = nullptr;
	void* d_bias_m = nullptr;
	void* d_bias_v = nullptr;

	size_t weight_elements = 0;
	size_t bias_elements = 0;
	uint64_t step_count = 0;

	void* d_query_weights = nullptr;
	void* d_key_weights = nullptr;
	void* d_value_weights = nullptr;
	void* d_output_weights = nullptr;

	void* d_gate_weights = nullptr;
	void* d_up_weights = nullptr;
	void* d_down_weights = nullptr;

	void* d_query_master = nullptr;
	void* d_key_master = nullptr;
	void* d_value_master = nullptr;
	void* d_output_master = nullptr;
	void* d_gate_master = nullptr;
	void* d_up_master = nullptr;
	void* d_down_master = nullptr;

	void* d_query_m = nullptr;
	void* d_query_v = nullptr;
	void* d_key_m = nullptr;
	void* d_key_v = nullptr;
	void* d_value_m = nullptr;
	void* d_value_v = nullptr;
	void* d_output_m = nullptr;
	void* d_output_v = nullptr;
	void* d_gate_m = nullptr;
	void* d_gate_v = nullptr;
	void* d_up_m = nullptr;
	void* d_up_v = nullptr;
	void* d_down_m = nullptr;
	void* d_down_v = nullptr;

	size_t transformer_dim = 0;
	size_t transformer_ffn = 0;

	bool enable_moe = false;
	size_t expert_count = 0;
	void* d_router_weights = nullptr;
	void* d_router_master = nullptr;
	void* d_router_m = nullptr;
	void* d_router_v = nullptr;

	void* d_expert_gate = nullptr;
	void* d_expert_up = nullptr;
	void* d_expert_down = nullptr;

	void* d_expert_gate_master = nullptr;
	void* d_expert_up_master = nullptr;
	void* d_expert_down_master = nullptr;

	void* d_expert_gate_m = nullptr;
	void* d_expert_gate_v = nullptr;
	void* d_expert_up_m = nullptr;
	void* d_expert_up_v = nullptr;
	void* d_expert_down_m = nullptr;
	void* d_expert_down_v = nullptr;
};

struct TestGpuActivations {
	void* d_output = nullptr;
	size_t output_elements = 0;

	void* d_loss = nullptr;

	void* d_inv_rms = nullptr;
	void* d_probs = nullptr;
	size_t vocab_elements = 0;
	void* d_pred_id = nullptr;
	void* d_dz = nullptr;

	void* h_pinned_loss = nullptr;
	void* loss_event = nullptr;
	bool loss_pending = false;

	void* d_query = nullptr;
	void* d_key = nullptr;
	void* d_value = nullptr;
	void* d_attention_output = nullptr;
	void* d_intermediate_gate = nullptr;
	void* d_intermediate_up = nullptr;
	void* d_intermediate_swiglu = nullptr;
	void* d_ffn_output = nullptr;
	void* d_residual_mid = nullptr;
	void* d_residual_out = nullptr;
	void* d_normalized_attention = nullptr;
	void* d_normalized_feedforward = nullptr;
	void* d_attention_scale = nullptr;
	void* d_feedforward_scale = nullptr;
	size_t xform_seq_elements = 0;
	size_t xform_ffn_elements = 0;

	void* d_router_logits = nullptr;
	void* d_expert_first = nullptr;
	void* d_expert_second = nullptr;
	void* d_weight_first = nullptr;
	void* d_weight_second = nullptr;
	void* d_expert_histogram = nullptr;
};

// Const test struct instances
const TestGpuPortion test_gpu_portion {};
const TestGpuEmbeddings test_gpu_embeddings {};
const TestGpuWeights test_gpu_weights {};
const TestGpuActivations test_gpu_activations {};

// Dimension & indexing parameters
const size_t test_input_dim = 0;
const size_t test_output_dim = 0;
const int test_buffer_index = 0;
const size_t test_sequence_length = 0;
const size_t test_embedding_dim = 0;
const size_t test_num_heads = 0;
const size_t test_feedforward_network_dimension = 0;
const size_t test_total_elements = 0;
const size_t test_num_tokens = 0;
const size_t test_n = 0;
const uint32_t test_batch_tokens = 0;
const uint32_t test_hidden_dim = 0;
const uint32_t test_vocab_size = 0;

// GEMM & benchmark parameters
const int test_M = 0;
const int test_N = 0;
const int test_K = 0;
const int test_iterations = 0;
const int test_rows_a = 0;
const int test_cols_b = 0;
const int test_common_dim = 0;
const size_t test_safety_headroom_bytes = 0;
double* const test_out_tflops = nullptr;
double* const test_out_bandwidth_gb_s = nullptr;
float* const test_out_avg_ms = nullptr;
size_t* const test_out_allocated_bytes = nullptr;
float* const test_out_elapsed_ms = nullptr;

// Control & optimizer flags / scalars
const bool test_gather_embeddings = true;
const bool test_transpose_a = false;
const bool test_transpose_b = false;
const bool test_use_adamw = false;
const bool test_scatter_embeddings = true;
const float test_learning_rate = 0.0f;

// Device buffer & stream pointers
void* const test_stream = nullptr;
const void* const test_matrix_a = nullptr;
const void* const test_matrix_b = nullptr;
void* const test_matrix_c = nullptr;
const void* const test_query = nullptr;
const void* const test_key = nullptr;
const void* const test_value = nullptr;
void* const test_output = nullptr;
const void* const test_gate = nullptr;
const void* const test_up = nullptr;
void* const test_hidden_states = nullptr;
const uint32_t* const test_d_token_ids = nullptr;
const void* const test_d_table = nullptr;
void* const test_d_output = nullptr;
const void* const test_d_x = nullptr;
const void* const test_d_w = nullptr;
const void* const test_d_b = nullptr;
void* const test_d_y = nullptr;
const void* const test_d_normed_x = nullptr;
const void* const test_d_lm_head_weight = nullptr;
float* const test_d_out_logits = nullptr;
float* const test_d_logits_and_probs = nullptr;
uint32_t* const test_d_pred_id = nullptr;
const void* const test_d_dx = nullptr;
const void* const test_grad_output = nullptr;
void* const test_grad_query = nullptr;
void* const test_grad_key = nullptr;
void* const test_grad_value = nullptr;
void* const test_grad_gate = nullptr;
void* const test_grad_up = nullptr;
void* const test_grad_input = nullptr;

// Switches
bool call_forward_pass = true;
bool call_forward_pass_with_embeddings = true;
bool call_run_cutlass_saturation_benchmark = true;
bool call_run_vram_saturation_benchmark = true;
bool call_batched_gemm_multiply = true;
bool call_fused_causal_attention = true;
bool call_swiglu_forward = true;
bool call_transformer_forward_block = true;
bool call_launch_gather_embeddings_batched = true;
bool call_launch_cutlass_gemm_bf16 = true;
bool call_launch_tied_logits_bf16 = true;
bool call_launch_softmax_argmax = true;

bool call_backward_pass = true;
bool call_backward_pass_with_embeddings = true;
bool call_backward_scatter_embeddings = true;
bool call_fused_causal_backward = true;
bool call_swiglu_backward = true;
bool call_transformer_backward_block = true;

static bool test_forward() {
	if (call_forward_pass == true) {
		forward_pass(reinterpret_cast<const GpuPortion*>(&test_gpu_portion),
					 reinterpret_cast<const GpuWeights*>(&test_gpu_weights),
					 reinterpret_cast<GpuActivations*>(
						 const_cast<TestGpuActivations*>(&test_gpu_activations)),
					 test_input_dim, test_output_dim, test_buffer_index);
	};


	if (call_forward_pass_with_embeddings == true) {
		forward_pass_with_embeddings(
			reinterpret_cast<const GpuPortion*>(&test_gpu_portion),
			reinterpret_cast<const GpuEmbeddings*>(&test_gpu_embeddings),
			reinterpret_cast<const GpuWeights*>(&test_gpu_weights),
			reinterpret_cast<GpuActivations*>(
				const_cast<TestGpuActivations*>(&test_gpu_activations)),
			test_input_dim, test_output_dim, test_buffer_index, test_gather_embeddings);
	}

	if (call_run_cutlass_saturation_benchmark == true) {
		run_cutlass_saturation_benchmark(test_M, test_N, test_K, test_iterations, test_out_tflops,
										 test_out_bandwidth_gb_s, test_out_avg_ms);
	}

	if (call_run_vram_saturation_benchmark == true) {
		run_vram_saturation_benchmark(test_safety_headroom_bytes, test_out_allocated_bytes,
									  test_out_bandwidth_gb_s, test_out_elapsed_ms);
	}

	// Phase 3: Multi-Head Attention, SwiGLU & Transformer Forward Block
	if (call_batched_gemm_multiply == true) {
		batched_gemm_multiply(test_matrix_a, test_matrix_b, test_matrix_c, test_rows_a,
							  test_cols_b, test_common_dim, test_transpose_a, test_transpose_b,
							  test_stream);
	}

	if (call_fused_causal_attention == true) {
		fused_causal_attention(test_query, test_key, test_value, test_output,
							   test_sequence_length, test_embedding_dim, test_num_heads,
							   test_stream);
	}

	if (call_swiglu_forward == true) {
		swiglu_forward(test_gate, test_up, test_output, test_total_elements, test_stream);
	}

	if (call_transformer_forward_block == true) {
		transformer_forward_block(
			test_hidden_states, reinterpret_cast<const GpuWeights*>(&test_gpu_weights),
			reinterpret_cast<GpuActivations*>(
				const_cast<TestGpuActivations*>(&test_gpu_activations)),
			test_sequence_length, test_embedding_dim, test_num_heads,
			test_feedforward_network_dimension, test_stream);
	}

	if (call_launch_gather_embeddings_batched == true) {
		launch_gather_embeddings_batched(test_d_token_ids, test_d_table, test_d_output,
										 test_batch_tokens, test_hidden_dim, test_vocab_size,
										 test_stream);
	}

	if (call_launch_cutlass_gemm_bf16 == true) {
		launch_cutlass_gemm_bf16(test_M, test_N, test_K, test_d_x, test_d_w, test_d_b, test_d_y,
								 test_stream);
	}

	if (call_launch_tied_logits_bf16 == true) {
		launch_tied_logits_bf16(test_d_normed_x, test_d_lm_head_weight, test_d_out_logits, test_M,
								static_cast<int>(test_vocab_size),
								static_cast<int>(test_hidden_dim), test_stream);
	}

	if (call_launch_softmax_argmax == true) {
		launch_softmax_argmax(test_d_logits_and_probs, test_d_pred_id, test_n, test_stream);
	}

	return true;
}

static bool test_backward() {
	if (call_backward_pass == true) {
		backward_pass(
			reinterpret_cast<const GpuPortion*>(&test_gpu_portion),
			reinterpret_cast<GpuWeights*>(const_cast<TestGpuWeights*>(&test_gpu_weights)),
			reinterpret_cast<const GpuActivations*>(&test_gpu_activations), test_input_dim,
			test_output_dim, test_learning_rate, test_buffer_index);
	}

	if (call_backward_pass_with_embeddings == true) {
		backward_pass_with_embeddings(
			reinterpret_cast<const GpuPortion*>(&test_gpu_portion),
			reinterpret_cast<GpuEmbeddings*>(
				const_cast<TestGpuEmbeddings*>(&test_gpu_embeddings)),
			reinterpret_cast<GpuWeights*>(const_cast<TestGpuWeights*>(&test_gpu_weights)),
			reinterpret_cast<const GpuActivations*>(&test_gpu_activations), test_input_dim,
			test_output_dim, test_learning_rate, test_buffer_index, test_use_adamw,
			test_scatter_embeddings);
	}

	if (call_backward_scatter_embeddings == true) {
		backward_scatter_embeddings(
			reinterpret_cast<const GpuPortion*>(&test_gpu_portion),
			reinterpret_cast<GpuEmbeddings*>(
				const_cast<TestGpuEmbeddings*>(&test_gpu_embeddings)),
			test_d_dx, test_num_tokens, test_embedding_dim, test_learning_rate,
			test_buffer_index, test_stream);
	}

	// Phase 3: Multi-Head Attention, SwiGLU & Transformer Backward Block
	if (call_fused_causal_backward == true) {
		fused_causal_backward(test_grad_output, test_query, test_key, test_value, test_grad_query,
							  test_grad_key, test_grad_value, test_sequence_length,
							  test_embedding_dim, test_num_heads, test_stream);
	}

	if (call_swiglu_backward == true) {
		swiglu_backward(test_grad_output, test_gate, test_up, test_grad_gate, test_grad_up,
						test_total_elements, test_stream);
	}

	if (call_transformer_backward_block == true) {
		transformer_backward_block(
			test_grad_output, test_grad_input, test_hidden_states,
			reinterpret_cast<GpuWeights*>(const_cast<TestGpuWeights*>(&test_gpu_weights)),
			reinterpret_cast<const GpuActivations*>(&test_gpu_activations), test_sequence_length,
			test_embedding_dim, test_num_heads, test_feedforward_network_dimension,
			test_learning_rate, test_stream);
	}

	return true;
}

static bool pass() {
	return test_forward() && test_backward();
}