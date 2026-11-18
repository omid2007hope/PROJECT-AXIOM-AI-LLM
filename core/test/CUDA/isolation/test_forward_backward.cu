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

#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#include "../../../src/CUDA/kernel/backward_pass.cu"
#include "../../../src/CUDA/kernel/forward_pass.cu"
#include "../../../src/CUDA/util/gpu_types.cuh"



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

// Self-contained host & device test data helpers (no external inputs)
static void* test_create_stream() {
	cudaStream_t s = nullptr;
	cudaStreamCreate(&s);
	return s;
}

static void* test_create_event() {
	cudaEvent_t e = nullptr;
	cudaEventCreate(&e);
	return e;
}

static void* test_alloc_zero(size_t bytes) {
	void* ptr = nullptr;
	cudaMalloc(&ptr, bytes);
	if (ptr) {
		cudaMemset(ptr, 0, bytes);
	}
	return ptr;
}

static void* test_alloc_bf16(size_t count, float base_val) {
	std::vector<__nv_bfloat16> host(count);
	for (size_t i = 0; i < count; ++i) {
		float v = base_val * (1.0f + 0.01f * static_cast<float>(i % 17) - 0.08f);
		host[i] = __float2bfloat16(v);
	}
	void* ptr = nullptr;
	cudaMalloc(&ptr, count * sizeof(__nv_bfloat16));
	if (ptr) {
		cudaMemcpy(ptr, host.data(), count * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice);
	}
	return ptr;
}

static float* test_alloc_f32(size_t count, float base_val) {
	std::vector<float> host(count);
	for (size_t i = 0; i < count; ++i) {
		host[i] = base_val * (1.0f + 0.01f * static_cast<float>(i % 17) - 0.08f);
	}
	float* ptr = nullptr;
	cudaMalloc(&ptr, count * sizeof(float));
	if (ptr) {
		cudaMemcpy(ptr, host.data(), count * sizeof(float), cudaMemcpyHostToDevice);
	}
	return ptr;
}

static uint32_t* test_alloc_token_ids(size_t count, uint32_t start_id, uint32_t vocab_sz) {
	std::vector<uint32_t> host(count);
	for (size_t i = 0; i < count; ++i) {
		host[i] = (start_id + static_cast<uint32_t>(i)) % vocab_sz;
	}
	uint32_t* ptr = nullptr;
	cudaMalloc(&ptr, count * sizeof(uint32_t));
	if (ptr) {
		cudaMemcpy(ptr, host.data(), count * sizeof(uint32_t), cudaMemcpyHostToDevice);
	}
	return ptr;
}

static void* test_alloc_target_token(size_t elem_count, uint32_t target_id) {
	std::vector<__nv_bfloat16> host(elem_count, __float2bfloat16(0.0f));
	if (elem_count * sizeof(__nv_bfloat16) >= sizeof(uint32_t)) {
		std::memcpy(host.data(), &target_id, sizeof(uint32_t));
	}
	void* ptr = nullptr;
	cudaMalloc(&ptr, elem_count * sizeof(__nv_bfloat16));
	if (ptr) {
		cudaMemcpy(ptr, host.data(), elem_count * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice);
	}
	return ptr;
}

// Dimension & indexing parameters (must be initialized before struct factories)
const size_t test_input_dim = 256;
const size_t test_output_dim = 64;
const int test_buffer_index = 0;
const size_t test_sequence_length = 4;
const size_t test_embedding_dim = 64;
const size_t test_num_heads = 4;
const size_t test_feedforward_network_dimension = 256;
const size_t test_total_elements = 1024;
const size_t test_num_tokens = 4;
const size_t test_n = 32;
const uint32_t test_batch_tokens = 4;
const uint32_t test_hidden_dim = 64;
const uint32_t test_vocab_size = 32;

// GEMM & benchmark parameters
const int test_M = 4;
const int test_N = 64;
const int test_K = 256;
const int test_iterations = 5;
const int test_rows_a = 4;
const int test_cols_b = 64;
const int test_common_dim = 64;
const size_t test_safety_headroom_bytes = 64ULL * 1024ULL * 1024ULL;
static double test_out_tflops_storage = 0.0;
static double test_out_bandwidth_gb_s_storage = 0.0;
static float test_out_avg_ms_storage = 0.0f;
static size_t test_out_allocated_bytes_storage = 0;
static float test_out_elapsed_ms_storage = 0.0f;
double* const test_out_tflops = &test_out_tflops_storage;
double* const test_out_bandwidth_gb_s = &test_out_bandwidth_gb_s_storage;
float* const test_out_avg_ms = &test_out_avg_ms_storage;
size_t* const test_out_allocated_bytes = &test_out_allocated_bytes_storage;
float* const test_out_elapsed_ms = &test_out_elapsed_ms_storage;

// Control & optimizer flags / scalars
const bool test_gather_embeddings = true;
const bool test_transpose_a = false;
const bool test_transpose_b = false;
const bool test_use_adamw = true;
const bool test_scatter_embeddings = true;
const float test_learning_rate = 0.006f;

// Device buffer & stream pointers
void* const test_stream = test_create_stream();
const void* const test_matrix_a = test_alloc_bf16(test_rows_a * test_common_dim, 0.02f);
const void* const test_matrix_b = test_alloc_bf16(test_common_dim * test_cols_b, 0.02f);
void* const test_matrix_c = test_alloc_zero(test_rows_a * test_cols_b * sizeof(__nv_bfloat16));
const void* const test_query = test_alloc_bf16(test_sequence_length * test_embedding_dim, 0.02f);
const void* const test_key = test_alloc_bf16(test_sequence_length * test_embedding_dim, 0.02f);
const void* const test_value = test_alloc_bf16(test_sequence_length * test_embedding_dim, 0.02f);
void* const test_output = test_alloc_zero(test_sequence_length * test_embedding_dim * sizeof(__nv_bfloat16));
const void* const test_gate = test_alloc_bf16(test_total_elements, 0.02f);
const void* const test_up = test_alloc_bf16(test_total_elements, 0.02f);
void* const test_hidden_states = test_alloc_bf16(test_sequence_length * test_embedding_dim, 0.02f);
const uint32_t* const test_d_token_ids = test_alloc_token_ids(test_batch_tokens, 1, test_vocab_size);
const void* const test_d_table = test_alloc_bf16(test_vocab_size * test_hidden_dim, 0.02f);
void* const test_d_output = test_alloc_zero(test_batch_tokens * test_hidden_dim * sizeof(__nv_bfloat16));
const void* const test_d_x = test_alloc_bf16(test_M * test_K, 0.02f);
const void* const test_d_w = test_alloc_bf16(test_N * test_K, 0.003f);
const void* const test_d_b = test_alloc_bf16(test_N, 0.0f);
void* const test_d_y = test_alloc_zero(test_M * test_N * sizeof(__nv_bfloat16));
const void* const test_d_normed_x = test_alloc_bf16(test_M * test_hidden_dim, 0.02f);
const void* const test_d_lm_head_weight = test_alloc_bf16(test_vocab_size * test_hidden_dim, 0.02f);
float* const test_d_out_logits = test_alloc_f32(test_M * test_vocab_size, 0.0f);
float* const test_d_logits_and_probs = test_alloc_f32(test_n, 0.1f);
uint32_t* const test_d_pred_id = test_alloc_token_ids(1, 0, test_vocab_size);
const void* const test_d_dx = test_alloc_f32(test_num_tokens * test_embedding_dim, 0.01f);
const void* const test_grad_output = test_alloc_bf16(test_total_elements, 0.01f);
void* const test_grad_query = test_alloc_zero(test_sequence_length * test_embedding_dim * sizeof(__nv_bfloat16));
void* const test_grad_key = test_alloc_zero(test_sequence_length * test_embedding_dim * sizeof(__nv_bfloat16));
void* const test_grad_value = test_alloc_zero(test_sequence_length * test_embedding_dim * sizeof(__nv_bfloat16));
void* const test_grad_gate = test_alloc_zero(test_total_elements * sizeof(__nv_bfloat16));
void* const test_grad_up = test_alloc_zero(test_total_elements * sizeof(__nv_bfloat16));
void* const test_grad_input = test_alloc_zero(test_sequence_length * test_embedding_dim * sizeof(__nv_bfloat16));

// Populated const test struct instances
static TestGpuPortion make_test_gpu_portion() {
	TestGpuPortion p;
	p.d_token_ids[0] = test_alloc_token_ids(test_num_tokens, 1, test_vocab_size);
	p.d_token_ids[1] = test_alloc_token_ids(test_num_tokens, 2, test_vocab_size);
	p.d_context_window[0] = test_alloc_bf16(test_input_dim, 0.02f);
	p.d_context_window[1] = test_alloc_bf16(test_input_dim, 0.02f);
	p.d_target[0] = test_alloc_target_token(test_output_dim, 5);
	p.d_target[1] = test_alloc_target_token(test_output_dim, 6);
	p.d_dx[0] = test_alloc_zero(test_input_dim * sizeof(float));
	p.d_dx[1] = test_alloc_zero(test_input_dim * sizeof(float));
	p.stream[0] = test_stream;
	p.stream[1] = test_create_stream();
	p.compute_done[0] = test_create_event();
	p.compute_done[1] = test_create_event();
	p.token_ids_active[0] = true;
	p.token_ids_active[1] = true;
	p.token_count = test_num_tokens;
	p.context_elements = test_input_dim;
	p.target_elements = test_output_dim;
	p.max_batch_tokens = test_num_tokens;
	p.is_allocated = true;
	return p;
}

static TestGpuEmbeddings make_test_gpu_embeddings() {
	TestGpuEmbeddings e;
	size_t elems = test_vocab_size * test_embedding_dim;
	e.d_table = test_alloc_bf16(elems, 0.02f);
	e.d_table_master = test_alloc_f32(elems, 0.02f);
	e.vocab_size = test_vocab_size;
	e.embedding_dim = test_embedding_dim;
	e.active_vocab_size = test_vocab_size;
	e.d_m = test_alloc_zero(elems * sizeof(float));
	e.d_v = test_alloc_zero(elems * sizeof(float));
	e.temperature = 1.0f;
	return e;
}

static TestGpuWeights make_test_gpu_weights() {
	TestGpuWeights w;
	size_t w_elems = test_input_dim * test_output_dim;
	size_t b_elems = test_output_dim;
	size_t qkv_elems = test_embedding_dim * test_embedding_dim;
	size_t ffn_elems = test_embedding_dim * test_feedforward_network_dimension;

	w.d_weights = test_alloc_bf16(w_elems, 0.003f);
	w.d_bias = test_alloc_bf16(b_elems, 0.0f);
	w.d_weights_master = test_alloc_f32(w_elems, 0.003f);
	w.d_bias_master = test_alloc_f32(b_elems, 0.0f);
	w.d_weights_m = test_alloc_zero(w_elems * sizeof(float));
	w.d_weights_v = test_alloc_zero(w_elems * sizeof(float));
	w.d_bias_m = test_alloc_zero(b_elems * sizeof(float));
	w.d_bias_v = test_alloc_zero(b_elems * sizeof(float));
	w.weight_elements = w_elems;
	w.bias_elements = b_elems;

	w.d_query_weights = test_alloc_bf16(qkv_elems, 0.02f);
	w.d_key_weights = test_alloc_bf16(qkv_elems, 0.02f);
	w.d_value_weights = test_alloc_bf16(qkv_elems, 0.02f);
	w.d_output_weights = test_alloc_bf16(qkv_elems, 0.02f);
	w.d_gate_weights = test_alloc_bf16(ffn_elems, 0.02f);
	w.d_up_weights = test_alloc_bf16(ffn_elems, 0.02f);
	w.d_down_weights = test_alloc_bf16(ffn_elems, 0.02f);

	w.d_query_master = test_alloc_f32(qkv_elems, 0.02f);
	w.d_key_master = test_alloc_f32(qkv_elems, 0.02f);
	w.d_value_master = test_alloc_f32(qkv_elems, 0.02f);
	w.d_output_master = test_alloc_f32(qkv_elems, 0.02f);
	w.d_gate_master = test_alloc_f32(ffn_elems, 0.02f);
	w.d_up_master = test_alloc_f32(ffn_elems, 0.02f);
	w.d_down_master = test_alloc_f32(ffn_elems, 0.02f);

	w.d_query_m = test_alloc_zero(qkv_elems * sizeof(float));
	w.d_query_v = test_alloc_zero(qkv_elems * sizeof(float));
	w.d_key_m = test_alloc_zero(qkv_elems * sizeof(float));
	w.d_key_v = test_alloc_zero(qkv_elems * sizeof(float));
	w.d_value_m = test_alloc_zero(qkv_elems * sizeof(float));
	w.d_value_v = test_alloc_zero(qkv_elems * sizeof(float));
	w.d_output_m = test_alloc_zero(qkv_elems * sizeof(float));
	w.d_output_v = test_alloc_zero(qkv_elems * sizeof(float));
	w.d_gate_m = test_alloc_zero(ffn_elems * sizeof(float));
	w.d_gate_v = test_alloc_zero(ffn_elems * sizeof(float));
	w.d_up_m = test_alloc_zero(ffn_elems * sizeof(float));
	w.d_up_v = test_alloc_zero(ffn_elems * sizeof(float));
	w.d_down_m = test_alloc_zero(ffn_elems * sizeof(float));
	w.d_down_v = test_alloc_zero(ffn_elems * sizeof(float));

	w.transformer_dim = test_embedding_dim;
	w.transformer_ffn = test_feedforward_network_dimension;
	return w;
}

static TestGpuActivations make_test_gpu_activations() {
	TestGpuActivations a;
	size_t seq_elems = test_sequence_length * test_embedding_dim;
	size_t ffn_elems = test_sequence_length * test_feedforward_network_dimension;

	a.d_output = test_alloc_zero(test_output_dim * sizeof(__nv_bfloat16));
	a.output_elements = test_output_dim;
	a.d_loss = test_alloc_zero(telemetry_slot_count * sizeof(float));
	a.d_inv_rms = test_alloc_f32(test_num_tokens, 1.0f);
	a.d_probs = test_alloc_zero(test_vocab_size * sizeof(float));
	a.vocab_elements = test_vocab_size;
	a.d_pred_id = test_alloc_zero(sizeof(uint32_t));
	a.d_dz = test_alloc_zero(test_output_dim * sizeof(float));

	a.d_query = test_alloc_zero(seq_elems * sizeof(__nv_bfloat16));
	a.d_key = test_alloc_zero(seq_elems * sizeof(__nv_bfloat16));
	a.d_value = test_alloc_zero(seq_elems * sizeof(__nv_bfloat16));
	a.d_attention_output = test_alloc_zero(seq_elems * sizeof(__nv_bfloat16));
	a.d_intermediate_gate = test_alloc_zero(ffn_elems * sizeof(__nv_bfloat16));
	a.d_intermediate_up = test_alloc_zero(ffn_elems * sizeof(__nv_bfloat16));
	a.d_intermediate_swiglu = test_alloc_zero(ffn_elems * sizeof(__nv_bfloat16));
	a.d_ffn_output = test_alloc_zero(seq_elems * sizeof(__nv_bfloat16));
	a.d_residual_mid = test_alloc_zero(seq_elems * sizeof(__nv_bfloat16));
	a.d_residual_out = test_alloc_zero(seq_elems * sizeof(__nv_bfloat16));
	a.d_normalized_attention = test_alloc_zero(seq_elems * sizeof(__nv_bfloat16));
	a.d_normalized_feedforward = test_alloc_zero(seq_elems * sizeof(__nv_bfloat16));
	a.d_attention_scale = test_alloc_f32(test_sequence_length, 1.0f);
	a.d_feedforward_scale = test_alloc_f32(test_sequence_length, 1.0f);
	a.xform_seq_elements = seq_elems;
	a.xform_ffn_elements = ffn_elems;
	return a;
}

const TestGpuPortion test_gpu_portion = make_test_gpu_portion();
const TestGpuEmbeddings test_gpu_embeddings = make_test_gpu_embeddings();
const TestGpuWeights test_gpu_weights = make_test_gpu_weights();
const TestGpuActivations test_gpu_activations = make_test_gpu_activations();

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
		if (!forward_pass(reinterpret_cast<const GpuPortion*>(&test_gpu_portion),
						  reinterpret_cast<const GpuWeights*>(&test_gpu_weights),
						  reinterpret_cast<GpuActivations*>(
							  const_cast<TestGpuActivations*>(&test_gpu_activations)),
						  test_input_dim, test_output_dim, test_buffer_index)) {
			std::cerr << "[FAIL] forward_pass returned false\n";
			return false;
		}
	}

	if (call_forward_pass_with_embeddings == true) {
		if (!forward_pass_with_embeddings(
				reinterpret_cast<const GpuPortion*>(&test_gpu_portion),
				reinterpret_cast<const GpuEmbeddings*>(&test_gpu_embeddings),
				reinterpret_cast<const GpuWeights*>(&test_gpu_weights),
				reinterpret_cast<GpuActivations*>(
					const_cast<TestGpuActivations*>(&test_gpu_activations)),
				test_input_dim, test_output_dim, test_buffer_index, test_gather_embeddings)) {
			std::cerr << "[FAIL] forward_pass_with_embeddings returned false\n";
			return false;
		}
	}

	if (call_run_cutlass_saturation_benchmark == true) {
		if (!run_cutlass_saturation_benchmark(test_M, test_N, test_K, test_iterations,
											  test_out_tflops, test_out_bandwidth_gb_s,
											  test_out_avg_ms)) {
			std::cerr << "[FAIL] run_cutlass_saturation_benchmark returned false\n";
			return false;
		}
	}

	if (call_run_vram_saturation_benchmark == true) {
		if (!run_vram_saturation_benchmark(test_safety_headroom_bytes, test_out_allocated_bytes,
										   test_out_bandwidth_gb_s, test_out_elapsed_ms)) {
			std::cerr << "[FAIL] run_vram_saturation_benchmark returned false\n";
			return false;
		}
	}

	// Phase 3: Multi-Head Attention, SwiGLU & Transformer Forward Block
	if (call_batched_gemm_multiply == true) {
		if (!batched_gemm_multiply(test_matrix_a, test_matrix_b, test_matrix_c, test_rows_a,
								   test_cols_b, test_common_dim, test_transpose_a, test_transpose_b,
								   test_stream)) {
			std::cerr << "[FAIL] batched_gemm_multiply returned false\n";
			return false;
		}
	}

	if (call_fused_causal_attention == true) {
		if (!fused_causal_attention(test_query, test_key, test_value, test_output,
									test_sequence_length, test_embedding_dim, test_num_heads,
									test_stream)) {
			std::cerr << "[FAIL] fused_causal_attention returned false\n";
			return false;
		}
	}

	if (call_swiglu_forward == true) {
		if (!swiglu_forward(test_gate, test_up, test_output, test_total_elements, test_stream)) {
			std::cerr << "[FAIL] swiglu_forward returned false\n";
			return false;
		}
	}

	if (call_transformer_forward_block == true) {
		if (!transformer_forward_block(
				test_hidden_states, reinterpret_cast<const GpuWeights*>(&test_gpu_weights),
				reinterpret_cast<GpuActivations*>(
					const_cast<TestGpuActivations*>(&test_gpu_activations)),
				test_sequence_length, test_embedding_dim, test_num_heads,
				test_feedforward_network_dimension, test_stream)) {
			std::cerr << "[FAIL] transformer_forward_block returned false\n";
			return false;
		}
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
		if (!backward_pass(
				reinterpret_cast<const GpuPortion*>(&test_gpu_portion),
				reinterpret_cast<GpuWeights*>(const_cast<TestGpuWeights*>(&test_gpu_weights)),
				reinterpret_cast<const GpuActivations*>(&test_gpu_activations), test_input_dim,
				test_output_dim, test_learning_rate, test_buffer_index)) {
			std::cerr << "[FAIL] backward_pass returned false\n";
			return false;
		}
	}

	if (call_backward_pass_with_embeddings == true) {
		if (!backward_pass_with_embeddings(
				reinterpret_cast<const GpuPortion*>(&test_gpu_portion),
				reinterpret_cast<GpuEmbeddings*>(
					const_cast<TestGpuEmbeddings*>(&test_gpu_embeddings)),
				reinterpret_cast<GpuWeights*>(const_cast<TestGpuWeights*>(&test_gpu_weights)),
				reinterpret_cast<const GpuActivations*>(&test_gpu_activations), test_input_dim,
				test_output_dim, test_learning_rate, test_buffer_index, test_use_adamw,
				test_scatter_embeddings)) {
			std::cerr << "[FAIL] backward_pass_with_embeddings returned false\n";
			return false;
		}
	}

	if (call_backward_scatter_embeddings == true) {
		if (!backward_scatter_embeddings(
				reinterpret_cast<const GpuPortion*>(&test_gpu_portion),
				reinterpret_cast<GpuEmbeddings*>(
					const_cast<TestGpuEmbeddings*>(&test_gpu_embeddings)),
				test_d_dx, test_num_tokens, test_embedding_dim, test_learning_rate,
				test_buffer_index, test_stream)) {
			std::cerr << "[FAIL] backward_scatter_embeddings returned false\n";
			return false;
		}
	}

	// Phase 3: Multi-Head Attention, SwiGLU & Transformer Backward Block
	if (call_fused_causal_backward == true) {
		if (!fused_causal_backward(test_grad_output, test_query, test_key, test_value,
								   test_grad_query, test_grad_key, test_grad_value,
								   test_sequence_length, test_embedding_dim, test_num_heads,
								   test_stream)) {
			std::cerr << "[FAIL] fused_causal_backward returned false\n";
			return false;
		}
	}

	if (call_swiglu_backward == true) {
		if (!swiglu_backward(test_grad_output, test_gate, test_up, test_grad_gate, test_grad_up,
							 test_total_elements, test_stream)) {
			std::cerr << "[FAIL] swiglu_backward returned false\n";
			return false;
		}
	}

	if (call_transformer_backward_block == true) {
		if (!transformer_backward_block(
				test_grad_output, test_grad_input, test_hidden_states,
				reinterpret_cast<GpuWeights*>(const_cast<TestGpuWeights*>(&test_gpu_weights)),
				reinterpret_cast<const GpuActivations*>(&test_gpu_activations),
				test_sequence_length, test_embedding_dim, test_num_heads,
				test_feedforward_network_dimension, test_learning_rate, test_stream)) {
			std::cerr << "[FAIL] transformer_backward_block returned false\n";
			return false;
		}
	}

	return true;
}

static bool pass() {
	return test_forward() && test_backward();
}

static bool read_and_reset_telemetry(float out_telemetry[telemetry_slot_count]) {
	cudaError_t sync_err = cudaDeviceSynchronize();
	if (sync_err != cudaSuccess) {
		std::cerr << "[FAIL] cudaDeviceSynchronize: " << cudaGetErrorString(sync_err) << "\n";
		return false;
	}
	const size_t bytes = telemetry_slot_count * sizeof(float);
	if (cudaMemcpy(out_telemetry, test_gpu_activations.d_loss, bytes, cudaMemcpyDeviceToHost) !=
		cudaSuccess) {
		std::cerr << "[FAIL] cudaMemcpy telemetry failed\n";
		return false;
	}
	cudaMemset(test_gpu_activations.d_loss, 0, bytes);
	return true;
}

int main() {
	constexpr int total_steps = 3000;
	constexpr int report_interval = 100;
	constexpr int checkpoint_interval = 1000;

	float prev_checkpoint_loss = -1.0f;
	bool all_healthy = true;

	std::cout << "[ISOLATION] Starting closed-loop forward/backward test (" << total_steps
			  << " steps)\n";

	for (int step = 1; step <= total_steps; ++step) {
		if (!pass()) {
			std::cerr << "[FAIL] Execution failed at step " << step << "\n";
			return 1;
		}

		if (step % report_interval == 0) {
			float telemetry[telemetry_slot_count] = {0.0f};
			if (!read_and_reset_telemetry(telemetry)) {
				return 1;
			}

			const float avg_loss = telemetry[telemetry_loss] / static_cast<float>(report_interval);
			const float grad_rms = std::sqrt(
				telemetry[telemetry_gradient_square] /
				static_cast<float>(report_interval * test_output_dim));
			const float act_rms = std::sqrt(
				telemetry[telemetry_activation_square] /
				static_cast<float>(report_interval * test_output_dim));
			const float dead_out = telemetry[telemetry_dead_output];
			const float nonfinite = telemetry[telemetry_nonfinite];

			std::cout << "[step " << step << "] avg_loss=" << avg_loss << " grad_rms=" << grad_rms
					  << " act_rms=" << act_rms << " dead_out=" << dead_out
					  << " nonfinite=" << nonfinite << "\n";

			if (!std::isfinite(avg_loss) || !std::isfinite(grad_rms) || nonfinite > 0.0f) {
				std::cerr << "[ANOMALY] Non-finite or invalid telemetry at step " << step << "\n";
				all_healthy = false;
				break;
			}

			if (step % checkpoint_interval == 0) {
				if (prev_checkpoint_loss >= 0.0f && avg_loss >= prev_checkpoint_loss) {
					std::cerr << "[ANOMALY] Loss did not drop at step " << step << " (prev="
							  << prev_checkpoint_loss << ", curr=" << avg_loss << ")\n";
					all_healthy = false;
				} else {
					std::cout << "[CHECKPOINT " << step << "] loss=" << avg_loss << " (OK)\n";
				}
				prev_checkpoint_loss = avg_loss;
			}
		}
	}

	if (all_healthy) {
		std::cout << "[PASS GREEN] Closed-loop forward/backward passed without anomalies.\n";
		return 0;
	}
	std::cout << "[FAIL RED] Anomaly detected in isolated forward/backward loop.\n";
	return 1;
}