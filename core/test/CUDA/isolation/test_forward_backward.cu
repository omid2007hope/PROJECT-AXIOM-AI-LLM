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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#include "../../../include/shared_params.h"
#include "../../../src/CUDA/gpu/download.cuh"
#include "../../../src/CUDA/kernel/backward_pass.cuh"
#include "../../../src/CUDA/kernel/forward_pass.cuh"
#include "../../../src/CUDA/kernel/swiglu.cuh"
#include "../../../src/CUDA/util/gpu_types.cuh"
#include "../../../src/CUDA/util/memory_allocation.cuh"

// Scale profile selector:
//   0 = Canonical Production Max (shared_params.h: V=65536, S=256, D=1024, H=16, FFN=4096)
//   1 = Experimental 4096-Dim    (V=32768, S=8, D=4096, H=64, FFN=16384)
#ifndef AXIOM_ISOLATION_PROFILE
#define AXIOM_ISOLATION_PROFILE 0
#endif

// Mirror structs matching gpu_types.cuh field-for-field
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

// Compile-time ABI parity checks against gpu_types.cuh
static_assert(sizeof(TestGpuPortion) == sizeof(GpuPortion), "TestGpuPortion size mismatch");
static_assert(alignof(TestGpuPortion) == alignof(GpuPortion), "TestGpuPortion align mismatch");
static_assert(sizeof(TestGpuEmbeddings) == sizeof(GpuEmbeddings),
			  "TestGpuEmbeddings size mismatch");
static_assert(alignof(TestGpuEmbeddings) == alignof(GpuEmbeddings),
			  "TestGpuEmbeddings align mismatch");
static_assert(sizeof(TestGpuWeights) == sizeof(GpuWeights), "TestGpuWeights size mismatch");
static_assert(alignof(TestGpuWeights) == alignof(GpuWeights), "TestGpuWeights align mismatch");
static_assert(sizeof(TestGpuActivations) == sizeof(GpuActivations),
			  "TestGpuActivations size mismatch");
static_assert(alignof(TestGpuActivations) == alignof(GpuActivations),
			  "TestGpuActivations align mismatch");

// Deterministic RAII tracker for all test CUDA allocations, streams, and events
struct TestResourceRegistry {
	std::vector<void*> device_ptrs;
	std::vector<cudaStream_t> streams;
	std::vector<cudaEvent_t> events;

	~TestResourceRegistry() {
		cudaDeviceSynchronize();
		for (void* ptr : device_ptrs) {
			if (ptr) {
				cudaFree(ptr);
			}
		}
		for (cudaEvent_t ev : events) {
			if (ev) {
				cudaEventDestroy(ev);
			}
		}
		for (cudaStream_t st : streams) {
			if (st) {
				cudaStreamDestroy(st);
			}
		}
	}
};

static TestResourceRegistry& test_registry() {
	static TestResourceRegistry registry;
	return registry;
}

static void check_cuda_or_abort(cudaError_t err, const char* context) {
	if (err != cudaSuccess) {
		std::cerr << "[FATAL CUDA] " << context << ": " << cudaGetErrorString(err) << "\n";
		std::exit(1);
	}
}

static void* test_create_stream() {
	cudaStream_t s = nullptr;
	check_cuda_or_abort(cudaStreamCreate(&s), "cudaStreamCreate");
	test_registry().streams.push_back(s);
	return s;
}

static void* test_create_event() {
	cudaEvent_t e = nullptr;
	check_cuda_or_abort(cudaEventCreateWithFlags(&e, cudaEventDisableTiming),
						"cudaEventCreateWithFlags");
	test_registry().events.push_back(e);
	return e;
}

static void* test_alloc_zero(size_t bytes) {
	void* ptr = nullptr;
	check_cuda_or_abort(cudaMalloc(&ptr, bytes), "cudaMalloc(zero)");
	check_cuda_or_abort(cudaMemset(ptr, 0, bytes), "cudaMemset(zero)");
	test_registry().device_ptrs.push_back(ptr);
	return ptr;
}

static void* test_alloc_bf16_uniform(size_t count, float bound, uint32_t seed) {
	void* ptr = nullptr;
	check_cuda_or_abort(cudaMalloc(&ptr, count * sizeof(__nv_bfloat16)), "cudaMalloc(bf16)");
	std::mt19937 rng(seed);
	std::uniform_real_distribution<float> dist(-bound, bound);
	constexpr size_t chunk_cap = 8ULL * 1024ULL * 1024ULL;
	std::vector<__nv_bfloat16> host(std::min(count, chunk_cap));
	for (size_t offset = 0; offset < count; offset += chunk_cap) {
		size_t chunk = std::min(chunk_cap, count - offset);
		for (size_t i = 0; i < chunk; ++i) {
			host[i] = __float2bfloat16(dist(rng));
		}
		check_cuda_or_abort(
			cudaMemcpy(static_cast<__nv_bfloat16*>(ptr) + offset, host.data(),
					   chunk * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice),
			"cudaMemcpy(bf16)");
	}
	test_registry().device_ptrs.push_back(ptr);
	return ptr;
}

static void test_alloc_paired_weights(size_t count, float bound, uint32_t seed,
									  void** out_bf16, void** out_f32,
									  size_t zero_prefix_elems = 0) {
	check_cuda_or_abort(cudaMalloc(out_bf16, count * sizeof(__nv_bfloat16)),
						"cudaMalloc(paired_bf16)");
	check_cuda_or_abort(cudaMalloc(out_f32, count * sizeof(float)), "cudaMalloc(paired_f32)");
	std::mt19937 rng(seed);
	std::uniform_real_distribution<float> dist(-bound, bound);
	constexpr size_t chunk_cap = 8ULL * 1024ULL * 1024ULL;
	std::vector<float> host_f32(std::min(count, chunk_cap));
	std::vector<__nv_bfloat16> host_bf16(std::min(count, chunk_cap));
	for (size_t offset = 0; offset < count; offset += chunk_cap) {
		size_t chunk = std::min(chunk_cap, count - offset);
		for (size_t i = 0; i < chunk; ++i) {
			size_t global_idx = offset + i;
			float val = (global_idx < zero_prefix_elems) ? 0.0f : dist(rng);
			host_f32[i] = val;
			host_bf16[i] = __float2bfloat16(val);
		}
		check_cuda_or_abort(cudaMemcpy(static_cast<__nv_bfloat16*>(*out_bf16) + offset,
									   host_bf16.data(), chunk * sizeof(__nv_bfloat16),
									   cudaMemcpyHostToDevice),
							"cudaMemcpy(paired_bf16)");
		check_cuda_or_abort(cudaMemcpy(static_cast<float*>(*out_f32) + offset, host_f32.data(),
									   chunk * sizeof(float), cudaMemcpyHostToDevice),
							"cudaMemcpy(paired_f32)");
	}
	test_registry().device_ptrs.push_back(*out_bf16);
	test_registry().device_ptrs.push_back(*out_f32);
}

static float* test_alloc_f32_fill(size_t count, float fill_val) {
	std::vector<float> host(count, fill_val);
	float* ptr = nullptr;
	check_cuda_or_abort(cudaMalloc(&ptr, count * sizeof(float)), "cudaMalloc(f32)");
	check_cuda_or_abort(cudaMemcpy(ptr, host.data(), count * sizeof(float), cudaMemcpyHostToDevice),
						"cudaMemcpy(f32)");
	test_registry().device_ptrs.push_back(ptr);
	return ptr;
}

// Packs 0x3D803D80 per 32-bit word: valid ~0.06256f as float AND 0.0625f in both bf16 halves
static void* test_alloc_dual_grad_buffer(size_t float_count) {
	std::vector<uint32_t> host(float_count, 0x3D803D80u);
	void* ptr = nullptr;
	check_cuda_or_abort(cudaMalloc(&ptr, float_count * sizeof(uint32_t)), "cudaMalloc(dual_grad)");
	check_cuda_or_abort(
		cudaMemcpy(ptr, host.data(), float_count * sizeof(uint32_t), cudaMemcpyHostToDevice),
		"cudaMemcpy(dual_grad)");
	test_registry().device_ptrs.push_back(ptr);
	return ptr;
}

static uint32_t* test_alloc_token_ids(size_t count, uint32_t start_id, uint32_t vocab_sz) {
	std::vector<uint32_t> host(count);
	for (size_t i = 0; i < count; ++i) {
		uint32_t id = (start_id + static_cast<uint32_t>(i)) % vocab_sz;
		host[i] = (id == 0) ? 1u : id; // Keep token IDs non-PAD
	}
	uint32_t* ptr = nullptr;
	check_cuda_or_abort(cudaMalloc(&ptr, count * sizeof(uint32_t)), "cudaMalloc(token_ids)");
	check_cuda_or_abort(
		cudaMemcpy(ptr, host.data(), count * sizeof(uint32_t), cudaMemcpyHostToDevice),
		"cudaMemcpy(token_ids)");
	test_registry().device_ptrs.push_back(ptr);
	return ptr;
}

static void* test_alloc_target_token(size_t elem_count, uint32_t target_id) {
	std::vector<__nv_bfloat16> host(elem_count, __float2bfloat16(0.0f));
	if (elem_count * sizeof(__nv_bfloat16) >= sizeof(uint32_t)) {
		std::memcpy(host.data(), &target_id, sizeof(uint32_t));
	}
	void* ptr = nullptr;
	check_cuda_or_abort(cudaMalloc(&ptr, elem_count * sizeof(__nv_bfloat16)),
						"cudaMalloc(target_token)");
	check_cuda_or_abort(
		cudaMemcpy(ptr, host.data(), elem_count * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice),
		"cudaMemcpy(target_token)");
	test_registry().device_ptrs.push_back(ptr);
	return ptr;
}

// Dimension & indexing parameters (consistent across all 18 kernels)
#if AXIOM_ISOLATION_PROFILE == 1
// Experimental 4096-dim stress scale (Llama-class hidden dim = 4096, 64 heads, FFN = 16384)
const uint32_t test_vocab_size = 32768U;
const size_t test_sequence_length = 8U;
const size_t test_embedding_dim = 4096U;
const size_t test_num_heads = 64U; // head_dim = 4096 / 64 = 64 <= 64
const size_t test_feedforward_network_dimension = 16384U;
#else
// Canonical Production Max (matching core/include/shared_params.h)
const uint32_t test_vocab_size = axiom::VOCAB_SIZE;              // 65536
const size_t test_sequence_length = axiom::CONTEXT_WINDOW;       // 256
const size_t test_embedding_dim = axiom::TOKEN_DIM;              // 1024
const size_t test_num_heads = axiom::NUM_HEADS;                  // 16 (head_dim = 64)
const size_t test_feedforward_network_dimension = axiom::FFN_DIM;// 4096
#endif

const size_t test_input_dim = test_sequence_length * test_embedding_dim;
const size_t test_output_dim = test_embedding_dim;
const int test_buffer_index = 0;
const size_t test_total_elements = test_sequence_length * test_feedforward_network_dimension;
const size_t test_num_tokens = test_sequence_length;
const size_t test_n = test_vocab_size;
const uint32_t test_batch_tokens = static_cast<uint32_t>(test_sequence_length);
const uint32_t test_hidden_dim = static_cast<uint32_t>(test_embedding_dim);

// GEMM & benchmark parameters
const int test_M = static_cast<int>(test_sequence_length);
const int test_N = static_cast<int>(test_output_dim);
const int test_K = static_cast<int>(test_input_dim);
const int test_iterations = 2;
const int test_rows_a = static_cast<int>(test_sequence_length);
const int test_cols_b = static_cast<int>(test_output_dim);
const int test_common_dim = static_cast<int>(test_embedding_dim);
const size_t test_safety_headroom_bytes = 256ULL * 1024ULL * 1024ULL;
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

// Switches: unfused core active by default (fused/block wrappers disabled for isolation)
bool call_launch_gather_embeddings_batched = false;
bool call_transformer_forward_block = false;
bool call_forward_pass_with_embeddings = true;
bool call_forward_pass = false;
bool call_run_cutlass_saturation_benchmark = false;
bool call_run_vram_saturation_benchmark = false;
bool call_batched_gemm_multiply = false;
bool call_fused_causal_attention = false;
bool call_swiglu_forward = false;
bool call_launch_cutlass_gemm_bf16 = false;
bool call_launch_tied_logits_bf16 = false;
bool call_launch_softmax_argmax = false;

bool call_backward_pass_with_embeddings = true;
bool call_transformer_backward_block = false;
bool call_backward_scatter_embeddings = false;
bool call_backward_pass = false;
bool call_fused_causal_backward = false;
bool call_swiglu_backward = false;

// Control & optimizer flags / scalars
const bool test_gather_embeddings =
	!(call_transformer_forward_block || call_launch_gather_embeddings_batched);
const bool test_transpose_a = false;
const bool test_transpose_b = false;
const bool test_use_adamw = true;
const bool test_scatter_embeddings = true;
const float test_learning_rate = 0.006f;

// Populated const test struct instances (created first so closed-loop pointers can alias them)
static TestGpuEmbeddings make_test_gpu_embeddings() {
	TestGpuEmbeddings e;
	size_t elems = test_vocab_size * test_embedding_dim;
	float emb_bound = 1.0f / std::sqrt(static_cast<float>(test_embedding_dim));
	test_alloc_paired_weights(elems, emb_bound, 1337u, &e.d_table, &e.d_table_master,
							  test_embedding_dim);
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
	float lin_bound = std::sqrt(3.0f / static_cast<float>(test_input_dim));
	float qkv_bound = 1.0f / std::sqrt(static_cast<float>(test_embedding_dim));
	float ffn_bound = 1.0f / std::sqrt(static_cast<float>(test_feedforward_network_dimension));

	test_alloc_paired_weights(w_elems, lin_bound, 2001u, &w.d_weights, &w.d_weights_master);
	test_alloc_paired_weights(b_elems, 0.0f, 2002u, &w.d_bias, &w.d_bias_master);
	w.d_weights_m = test_alloc_zero(w_elems * sizeof(float));
	w.d_weights_v = test_alloc_zero(w_elems * sizeof(float));
	w.d_bias_m = test_alloc_zero(b_elems * sizeof(float));
	w.d_bias_v = test_alloc_zero(b_elems * sizeof(float));
	w.weight_elements = w_elems;
	w.bias_elements = b_elems;

	if (call_transformer_forward_block || call_transformer_backward_block) {
		test_alloc_paired_weights(qkv_elems, qkv_bound, 3001u, &w.d_query_weights,
								  &w.d_query_master);
		test_alloc_paired_weights(qkv_elems, qkv_bound, 3002u, &w.d_key_weights, &w.d_key_master);
		test_alloc_paired_weights(qkv_elems, qkv_bound, 3003u, &w.d_value_weights,
								  &w.d_value_master);
		test_alloc_paired_weights(qkv_elems, qkv_bound, 3004u, &w.d_output_weights,
								  &w.d_output_master);
		test_alloc_paired_weights(ffn_elems, qkv_bound, 3005u, &w.d_gate_weights, &w.d_gate_master);
		test_alloc_paired_weights(ffn_elems, qkv_bound, 3006u, &w.d_up_weights, &w.d_up_master);
		test_alloc_paired_weights(ffn_elems, ffn_bound, 3007u, &w.d_down_weights, &w.d_down_master);

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
	}

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
	a.d_inv_rms = test_alloc_f32_fill(test_num_tokens, 1.0f);
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
	a.d_attention_scale = test_alloc_f32_fill(test_sequence_length, 1.0f);
	a.d_feedforward_scale = test_alloc_f32_fill(test_sequence_length, 1.0f);
	a.xform_seq_elements = seq_elems;
	a.xform_ffn_elements = ffn_elems;
	return a;
}

void* const test_stream = test_create_stream();
const TestGpuEmbeddings test_gpu_embeddings = make_test_gpu_embeddings();
const TestGpuWeights test_gpu_weights = make_test_gpu_weights();
const TestGpuActivations test_gpu_activations = make_test_gpu_activations();

static TestGpuPortion make_test_gpu_portion() {
	TestGpuPortion p;
	p.d_token_ids[0] = test_alloc_token_ids(test_num_tokens, 1, test_vocab_size);
	p.d_token_ids[1] = test_alloc_token_ids(test_num_tokens, 2, test_vocab_size);
	p.d_context_window[0] = test_alloc_bf16_uniform(test_input_dim, 0.05f, 4001u);
	p.d_context_window[1] = test_alloc_bf16_uniform(test_input_dim, 0.05f, 4002u);
	p.d_target[0] = test_alloc_target_token(test_output_dim, 5);
	p.d_target[1] = test_alloc_target_token(test_output_dim, 6);
	p.d_dx[0] = test_alloc_dual_grad_buffer(test_total_elements);
	p.d_dx[1] = test_alloc_dual_grad_buffer(test_total_elements);
	p.stream[0] = test_stream;
	p.stream[1] = test_create_stream();
	p.compute_done[0] = test_create_event();
	p.compute_done[1] = test_create_event();
	p.token_ids_active[0] = true;
	p.token_ids_active[1] = true;
	p.token_count = test_num_tokens;
	p.context_elements = test_input_dim;
	p.target_elements = test_output_dim;
	p.d_hidden_state[0] = test_alloc_bf16_uniform(test_input_dim, 0.05f, 4003u);
	p.d_hidden_state[1] = test_alloc_bf16_uniform(test_input_dim, 0.05f, 4004u);
	p.max_batch_tokens = test_num_tokens;
	p.is_allocated = true;
	return p;
}

const TestGpuPortion test_gpu_portion = make_test_gpu_portion();

// Device buffer pointers wired directly into the closed-loop GPU state
const void* const test_matrix_a = test_alloc_bf16_uniform(test_rows_a * test_common_dim, 0.05f, 5001u);
const void* const test_matrix_b = test_alloc_bf16_uniform(test_common_dim * test_cols_b, 0.05f, 5002u);
void* const test_matrix_c = test_alloc_zero(test_rows_a * test_cols_b * sizeof(__nv_bfloat16));
const void* const test_query = test_alloc_bf16_uniform(test_sequence_length * test_embedding_dim, 0.05f, 5003u);
const void* const test_key = test_alloc_bf16_uniform(test_sequence_length * test_embedding_dim, 0.05f, 5004u);
const void* const test_value = test_alloc_bf16_uniform(test_sequence_length * test_embedding_dim, 0.05f, 5005u);
void* const test_output = test_alloc_zero(test_total_elements * sizeof(__nv_bfloat16));
const void* const test_gate = test_alloc_bf16_uniform(test_total_elements, 0.05f, 5006u);
const void* const test_up = test_alloc_bf16_uniform(test_total_elements, 0.05f, 5007u);
void* const test_hidden_states = test_gpu_portion.d_context_window[test_buffer_index];
const uint32_t* const test_d_token_ids =
	static_cast<const uint32_t*>(test_gpu_portion.d_token_ids[test_buffer_index]);
const void* const test_d_table = test_gpu_embeddings.d_table;
void* const test_d_output = test_gpu_portion.d_context_window[test_buffer_index];
const void* const test_d_x = test_alloc_bf16_uniform(test_M * test_K, 0.05f, 5008u);
const void* const test_d_w = test_alloc_bf16_uniform(test_N * test_K, 0.05f, 5009u);
const void* const test_d_b = test_alloc_zero(test_N * sizeof(__nv_bfloat16));
void* const test_d_y = test_alloc_zero(test_M * test_N * sizeof(__nv_bfloat16));
const void* const test_d_normed_x = test_alloc_bf16_uniform(test_M * test_hidden_dim, 0.05f, 5010u);
const void* const test_d_lm_head_weight = test_gpu_embeddings.d_table;
float* const test_d_out_logits = test_alloc_f32_fill(test_M * test_vocab_size, 0.0f);
float* const test_d_logits_and_probs = test_alloc_f32_fill(test_n, 0.1f);
uint32_t* const test_d_pred_id = static_cast<uint32_t*>(test_gpu_activations.d_pred_id);
const void* const test_d_dx = test_gpu_portion.d_dx[test_buffer_index];
const void* const test_grad_output = test_gpu_portion.d_dx[test_buffer_index];
void* const test_grad_query = test_alloc_zero(test_sequence_length * test_embedding_dim * sizeof(float));
void* const test_grad_key = test_alloc_zero(test_sequence_length * test_embedding_dim * sizeof(float));
void* const test_grad_value = test_alloc_zero(test_sequence_length * test_embedding_dim * sizeof(float));
void* const test_grad_gate = test_alloc_zero(test_total_elements * sizeof(float));
void* const test_grad_up = test_alloc_zero(test_total_elements * sizeof(float));
void* const test_grad_input = test_gpu_portion.d_dx[test_buffer_index];

// Switch bindings moved above make_test_gpu_weights() so de-fused vs block mode configures automatically.

static bool test_forward() {
	if (call_launch_gather_embeddings_batched == true) {
		launch_gather_embeddings_batched(test_d_token_ids, test_d_table, test_d_output,
										 test_batch_tokens, test_hidden_dim, test_vocab_size,
										 test_stream);
		if (cudaGetLastError() != cudaSuccess) {
			std::cerr << "[FAIL] launch_gather_embeddings_batched launch failed\n";
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

	// Phase 3: Standalone Multi-Head Attention & SwiGLU checks
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

	if (call_launch_cutlass_gemm_bf16 == true) {
		launch_cutlass_gemm_bf16(test_M, test_N, test_K, test_d_x, test_d_w, test_d_b, test_d_y,
								 test_stream);
		if (cudaGetLastError() != cudaSuccess) {
			std::cerr << "[FAIL] launch_cutlass_gemm_bf16 launch failed\n";
			return false;
		}
	}

	if (call_launch_tied_logits_bf16 == true) {
		launch_tied_logits_bf16(test_d_normed_x, test_d_lm_head_weight, test_d_out_logits, test_M,
								static_cast<int>(test_vocab_size),
								static_cast<int>(test_hidden_dim), test_stream);
		if (cudaGetLastError() != cudaSuccess) {
			std::cerr << "[FAIL] launch_tied_logits_bf16 launch failed\n";
			return false;
		}
	}

	if (call_launch_softmax_argmax == true) {
		launch_softmax_argmax(test_d_logits_and_probs, test_d_pred_id, test_n, test_stream);
		if (cudaGetLastError() != cudaSuccess) {
			std::cerr << "[FAIL] launch_softmax_argmax launch failed\n";
			return false;
		}
	}

	return true;
}

static bool test_backward() {
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

	// Phase 3: Standalone Multi-Head Attention & SwiGLU backward checks
	if (call_fused_causal_backward == true) {
		cudaMemsetAsync(test_grad_query, 0, test_sequence_length * test_embedding_dim * sizeof(float),
						static_cast<cudaStream_t>(test_stream));
		cudaMemsetAsync(test_grad_key, 0, test_sequence_length * test_embedding_dim * sizeof(float),
						static_cast<cudaStream_t>(test_stream));
		cudaMemsetAsync(test_grad_value, 0, test_sequence_length * test_embedding_dim * sizeof(float),
						static_cast<cudaStream_t>(test_stream));
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

	return true;
}

static bool pass() {
	return test_forward() && test_backward();
}

static bool read_and_reset_telemetry(float out_telemetry[telemetry_slot_count]) {
	cudaError_t sync_err = cudaStreamSynchronize(static_cast<cudaStream_t>(test_stream));
	if (sync_err != cudaSuccess) {
		std::cerr << "[FAIL] cudaStreamSynchronize: " << cudaGetErrorString(sync_err) << "\n";
		return false;
	}
	const size_t bytes = telemetry_slot_count * sizeof(float);
	if (cudaMemcpy(out_telemetry, test_gpu_activations.d_loss, bytes, cudaMemcpyDeviceToHost) !=
		cudaSuccess) {
		std::cerr << "[FAIL] cudaMemcpy telemetry failed\n";
		return false;
	}
	if (cudaMemset(test_gpu_activations.d_loss, 0, bytes) != cudaSuccess) {
		std::cerr << "[FAIL] cudaMemset telemetry reset failed\n";
		return false;
	}
	return true;
}

int main() {
	constexpr int total_steps = 3000;
	constexpr int report_interval = 100;
	constexpr int checkpoint_interval = 1000;
	constexpr float sweet_spot_ceiling = 3.0f;
	constexpr float min_1k_drop = 0.05f;
	constexpr float spike_tolerance = 0.5f;

	float step1_loss = -1.0f;
	float prev_report_loss = -1.0f;
	float prev_checkpoint_loss = -1.0f;
	float prev_checkpoint_grad = -1.0f;
	std::vector<std::pair<int, float>> checkpoint_losses;
	bool all_healthy = true;
	const char* red_flag_reason = nullptr;

	size_t free_vram = 0;
	size_t total_vram = 0;
	cudaMemGetInfo(&free_vram, &total_vram);

	std::cout << std::fixed << std::setprecision(6);
	std::cout << "[ISOLATION] Profile=" << AXIOM_ISOLATION_PROFILE
			  << " | mode=" << (call_transformer_forward_block ? "TRANSFORMER_BLOCK" : "UNFUSED_CORE")
			  << " | steps=" << total_steps << " | vocab=" << test_vocab_size
			  << " | seq=" << test_sequence_length << " | emb_dim=" << test_embedding_dim
			  << " | heads=" << test_num_heads << " | ffn=" << test_feedforward_network_dimension
			  << " | in_dim=" << test_input_dim << "\n";
	std::cout << "[VRAM] Free: " << (free_vram / (1024ULL * 1024ULL))
			  << " MiB / Total: " << (total_vram / (1024ULL * 1024ULL)) << " MiB\n"
			  << std::flush;

	for (int step = 1; step <= total_steps; ++step) {
		if (!pass()) {
			std::cerr << "[RED FLAG] CUDA kernel execution returned false at step " << step << "\n";
			return 1;
		}

		if (step == 1 || step % report_interval == 0) {
			float telemetry[telemetry_slot_count] = {0.0f};
			if (!read_and_reset_telemetry(telemetry)) {
				return 1;
			}

			const int window_steps =
				(step == 1) ? 1 : (step == report_interval ? report_interval - 1 : report_interval);
			uint32_t pred_id = 0;
			cudaMemcpy(&pred_id, test_gpu_activations.d_pred_id, sizeof(uint32_t),
					   cudaMemcpyDeviceToHost);

			const float avg_loss = telemetry[telemetry_loss] / static_cast<float>(window_steps);
			const float grad_l2 =
				std::sqrt(telemetry[telemetry_gradient_square] / static_cast<float>(window_steps));
			const float dead_out = telemetry[telemetry_dead_output];
			const float nonfinite = telemetry[telemetry_nonfinite];

			if (step == 1) {
				step1_loss = avg_loss;
			}

			std::cout << "[step " << std::setw(4) << step << "] live loss: " << avg_loss
					  << " | grad_l2=" << grad_l2 << " | pred_id=" << pred_id
					  << " | dead_out=" << dead_out << " | nonfinite=" << nonfinite << "\n"
					  << std::flush;

			if (!std::isfinite(avg_loss) || !std::isfinite(grad_l2) || nonfinite > 0.0f ||
				avg_loss <= 0.0f || grad_l2 <= 0.0f) {
				red_flag_reason = "Non-finite, zero-loss, or zero-gradient telemetry detected";
				std::cerr << "[RED FLAG] " << red_flag_reason << " at step " << step << "\n";
				all_healthy = false;
				break;
			}

			if (prev_report_loss > 0.0f && avg_loss > prev_report_loss + spike_tolerance) {
				red_flag_reason = "Loss spiked upward (backward/forward CUDA math unstable)";
				std::cerr << "[RED FLAG] " << red_flag_reason << " at step " << step << " ("
						  << prev_report_loss << " -> " << avg_loss << ")\n";
				all_healthy = false;
				break;
			}
			prev_report_loss = avg_loss;

			if (step % checkpoint_interval == 0) {
				const float prev_ref =
					(prev_checkpoint_loss < 0.0f) ? step1_loss : prev_checkpoint_loss;
				const float drop = prev_ref - avg_loss;
				checkpoint_losses.push_back({step, avg_loss});

				if (avg_loss >= prev_ref) {
					red_flag_reason = "Loss increased or flatlined at 1000-step checkpoint";
					std::cerr << "[RED FLAG] " << red_flag_reason << " (step " << step
							  << ": prev=" << prev_ref << ", curr=" << avg_loss << ")\n";
					all_healthy = false;
					break;
				}
				if (avg_loss > sweet_spot_ceiling && drop < min_1k_drop) {
					red_flag_reason = "Loss drop stalled above target sweet spot [1.0, 3.0]";
					std::cerr << "[RED FLAG] " << red_flag_reason << " (step " << step
							  << ": drop=" << drop << " < " << min_1k_drop << ", loss=" << avg_loss
							  << ")\n";
					all_healthy = false;
					break;
				}
				if (prev_checkpoint_grad > 0.0f &&
					std::abs(grad_l2 - prev_checkpoint_grad) < 1e-6f &&
					avg_loss > sweet_spot_ceiling) {
					red_flag_reason = "Gradient norm frozen across 1000 steps while loss > 3.0";
					std::cerr << "[RED FLAG] " << red_flag_reason << " (step " << step
							  << ": grad_l2=" << grad_l2 << ", loss=" << avg_loss << ")\n";
					all_healthy = false;
					break;
				}

				std::cout << "[CHECKPOINT " << step << "] live loss: " << avg_loss
						  << " (delta=" << -drop << ") (OK)\n"
						  << std::flush;
				prev_checkpoint_loss = avg_loss;
				prev_checkpoint_grad = grad_l2;
			}
		}
	}

	if (all_healthy &&
		(checkpoint_losses.empty() || prev_checkpoint_loss > sweet_spot_ceiling)) {
		red_flag_reason =
			"Final loss failed to converge into sweet spot [1.0, 3.0] — backward/forward CUDA code has a math problem";
		std::cerr << "[RED FLAG] " << red_flag_reason << " (final=" << prev_checkpoint_loss
				  << ", sweet_spot<=" << sweet_spot_ceiling << ")\n";
		all_healthy = false;
	}

	std::cout << "\n--- 1000-Step Live Loss Summary ---\n";
	std::cout << "  step    1 | loss: " << step1_loss << "\n";
	for (const auto& entry : checkpoint_losses) {
		std::cout << "  step " << std::setw(4) << entry.first << " | loss: " << entry.second
				  << "\n";
	}

	if (all_healthy) {
		std::cout << "[PASS GREEN] Closed-loop forward/backward converged (" << step1_loss
				  << " -> " << prev_checkpoint_loss << ") into sweet spot <= "
				  << sweet_spot_ceiling << " without anomalies.\n";
		return 0;
	}
	std::cout << "[FAIL RED] RED FLAG: "
			  << (red_flag_reason ? red_flag_reason : "Backward/forward CUDA code problem")
			  << "\n";
	return 1;
}
