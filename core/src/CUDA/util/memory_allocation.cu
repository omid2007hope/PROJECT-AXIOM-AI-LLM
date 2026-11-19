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
#include <cstdio>
#include <cstdlib>
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#include <random>

#include "caching_allocator.cuh"
#include "cuda_utils.cuh"
#include "memory_allocation.cuh"

__global__ static void float_to_bf16_array_kernel(const float* __restrict__ src,
						  __nv_bfloat16* __restrict__ dst,
						  size_t n) {
	size_t i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n) {
		dst[i] = __float2bfloat16(src[i]);
	}
}

extern "C" CUDA_API void free_gpu(GpuPortion* gpu) {
	if (!gpu)
		return;
	for (size_t i = 0; i < gpu->context_ptr; ++i) {
		if (gpu->d_context_window[i]) {
			free_vram(gpu->d_context_window[i], gpu->stream[i]);
			gpu->d_context_window[i] = nullptr;
		}
		if (gpu->d_token_ids[i]) {
			free_vram(gpu->d_token_ids[i], gpu->stream[i]);
			gpu->d_token_ids[i] = nullptr;
		}
		if (gpu->d_dx[i]) {
			free_vram(gpu->d_dx[i], gpu->stream[i]);
			gpu->d_dx[i] = nullptr;
		}
		if (gpu->d_hidden_state[i]) {
			free_vram(gpu->d_hidden_state[i], gpu->stream[i]);
			gpu->d_hidden_state[i] = nullptr;
		}
		if (gpu->d_loss_scalar[i]) {
			free_vram(gpu->d_loss_scalar[i], gpu->stream[i]);
			gpu->d_loss_scalar[i] = nullptr;
		}
		if (gpu->h_pinned_loss[i]) {
			cudaFreeHost(gpu->h_pinned_loss[i]);
			gpu->h_pinned_loss[i] = nullptr;
		}
	}
	gpu->context_elements = 0;
	gpu->token_count = 0;
	gpu->token_ids_active[0] = false;
	gpu->token_ids_active[1] = false;

	for (size_t i = 0; i < gpu->target_ptr; ++i) {
		if (gpu->d_target[i]) {
			free_vram(gpu->d_target[i], gpu->stream[i]);
			gpu->d_target[i] = nullptr;
		}
	}
	gpu->target_elements = 0;

	for (size_t i = 0; i < gpu->context_ptr; ++i) {
		if (gpu->stream[i]) {
			cudaStreamDestroy((cudaStream_t)gpu->stream[i]);
			gpu->stream[i] = nullptr;
		}
		if (gpu->compute_done[i]) {
			cudaEventDestroy((cudaEvent_t)gpu->compute_done[i]);
			gpu->compute_done[i] = nullptr;
		}
		if (gpu->loss_ready_event[i]) {
			cudaEventDestroy((cudaEvent_t)gpu->loss_ready_event[i]);
			gpu->loss_ready_event[i] = nullptr;
		}
		gpu->compute_done_recorded[i] = false;
	}
	gpu->is_allocated = false;
}

extern "C" CUDA_API void free_legacy_gpu_weights(GpuWeights* gpu) {
	if (!gpu)
		return;
	if (gpu->d_weights) {
		cudaFree(gpu->d_weights);
		gpu->d_weights = nullptr;
	}
	if (gpu->d_bias) {
		cudaFree(gpu->d_bias);
		gpu->d_bias = nullptr;
	}
	if (gpu->d_weights_master) {
		cudaFree(gpu->d_weights_master);
		gpu->d_weights_master = nullptr;
	}
	if (gpu->d_bias_master) {
		cudaFree(gpu->d_bias_master);
		gpu->d_bias_master = nullptr;
	}
	if (gpu->d_weights_m) {
		free_vram(gpu->d_weights_m, 0);
		gpu->d_weights_m = nullptr;
	}
	if (gpu->d_weights_v) {
		free_vram(gpu->d_weights_v, 0);
		gpu->d_weights_v = nullptr;
	}
	if (gpu->d_bias_m) {
		free_vram(gpu->d_bias_m, 0);
		gpu->d_bias_m = nullptr;
	}
	if (gpu->d_bias_v) {
		free_vram(gpu->d_bias_v, 0);
		gpu->d_bias_v = nullptr;
	}
	gpu->weight_elements = 0;
	gpu->bias_elements = 0;
}

extern "C" CUDA_API void free_gpu_weights(GpuWeights* gpu) {
	if (!gpu)
		return;
	free_legacy_gpu_weights(gpu);

	// Free Transformer weights
	void** xform_weights[] = {
		&gpu->d_query_weights,	&gpu->d_key_weights,   &gpu->d_value_weights,
		&gpu->d_output_weights, &gpu->d_gate_weights,  &gpu->d_up_weights,
		&gpu->d_down_weights,	&gpu->d_query_master,  &gpu->d_key_master,
		&gpu->d_value_master,	&gpu->d_output_master, &gpu->d_gate_master,
		&gpu->d_up_master,	&gpu->d_down_master,   &gpu->d_query_m,
		&gpu->d_query_v,	&gpu->d_key_m,	       &gpu->d_key_v,
		&gpu->d_value_m,	&gpu->d_value_v,       &gpu->d_output_m,
		&gpu->d_output_v,	&gpu->d_gate_m,	       &gpu->d_gate_v,
		&gpu->d_up_m,		&gpu->d_up_v,	       &gpu->d_down_m,
		&gpu->d_down_v};
	for (void** ptr : xform_weights) {
		if (*ptr) {
			cudaFree(*ptr);
			*ptr = nullptr;
		}
	}

	free_expert_weights(gpu);

	gpu->transformer_dim = 0;
	gpu->transformer_ffn = 0;
	gpu->weight_elements = 0;
	gpu->bias_elements = 0;
	gpu->step_count = 0;
}

extern "C" CUDA_API bool allocate_gpu_embeddings(GpuEmbeddings* gpu_emb,
						 size_t vocab_size,
						 size_t embedding_dim) {
	if (!gpu_emb || vocab_size == 0 || embedding_dim == 0)
		return false;

	gpu_emb->vocab_size = vocab_size;
	gpu_emb->embedding_dim = embedding_dim;
	size_t total_elements = vocab_size * embedding_dim;

	// Free any previous allocations
	free_gpu_embeddings(gpu_emb);

	// 1. Allocate bf16 device table (fast Tensor Core / gather reads)
	if (!CUDA_CHECK(cudaMalloc(&gpu_emb->d_table, total_elements * sizeof(__nv_bfloat16)))) {
		free_gpu_embeddings(gpu_emb);
		return false;
	}

	// 2. Allocate FP32 master table (high-precision accumulation)
	if (!CUDA_CHECK(cudaMalloc(&gpu_emb->d_table_master, total_elements * sizeof(float)))) {
		free_gpu_embeddings(gpu_emb);
		return false;
	}

	// 3. Allocate AdamW moment buffers
	if (!CUDA_CHECK(cudaMalloc(&gpu_emb->d_m, total_elements * sizeof(float))) ||
	    !CUDA_CHECK(cudaMalloc(&gpu_emb->d_v, total_elements * sizeof(float))) ||
	    !CUDA_CHECK(cudaMemset(gpu_emb->d_m, 0, total_elements * sizeof(float))) ||
	    !CUDA_CHECK(cudaMemset(gpu_emb->d_v, 0, total_elements * sizeof(float)))) {
		free_gpu_embeddings(gpu_emb);
		return false;
	}

	// 4. Chaos initialization in [-1/sqrt(D), +1/sqrt(D)]
	float bound = 1.0f / sqrtf(static_cast<float>(embedding_dim));
	float* h_chaos = static_cast<float*>(malloc(total_elements * sizeof(float)));
	if (!h_chaos) {
		free_gpu_embeddings(gpu_emb);
		return false;
	}

	std::mt19937 gen(1337);
	std::uniform_real_distribution<float> dis(-bound, bound);
	for (size_t i = 0; i < total_elements; ++i) {
		h_chaos[i] = dis(gen);
	}
	std::fill(h_chaos, h_chaos + embedding_dim, 0.0f); // PAD row

	bool copy_ok = CUDA_CHECK(cudaMemcpy(gpu_emb->d_table_master,
					     h_chaos,
					     total_elements * sizeof(float),
					     cudaMemcpyHostToDevice));
	free(h_chaos);
	if (!copy_ok) {
		free_gpu_embeddings(gpu_emb);
		return false;
	}

	int threads = 256;
	int blocks = static_cast<int>((total_elements + threads - 1) / threads);
	float_to_bf16_array_kernel<<<blocks, threads>>>(
		static_cast<const float*>(gpu_emb->d_table_master),
		static_cast<__nv_bfloat16*>(gpu_emb->d_table),
		total_elements);
	if (!CUDA_CHECK(cudaGetLastError()) || !CUDA_CHECK(cudaDeviceSynchronize())) {
		free_gpu_embeddings(gpu_emb);
		return false;
	}

	return true;
}

extern "C" CUDA_API void free_gpu_embeddings(GpuEmbeddings* gpu_emb) {
	if (!gpu_emb)
		return;
	if (gpu_emb->d_table) {
		cudaFree(gpu_emb->d_table);
		gpu_emb->d_table = nullptr;
	}
	if (gpu_emb->d_table_master) {
		cudaFree(gpu_emb->d_table_master);
		gpu_emb->d_table_master = nullptr;
	}
	if (gpu_emb->d_m) {
		cudaFree(gpu_emb->d_m);
		gpu_emb->d_m = nullptr;
	}
	if (gpu_emb->d_v) {
		cudaFree(gpu_emb->d_v);
		gpu_emb->d_v = nullptr;
	}
}

extern "C" CUDA_API bool upload_embeddings(const float* h_embeddings, GpuEmbeddings* gpu_emb) {
	if (!h_embeddings || !gpu_emb || !gpu_emb->d_table_master || !gpu_emb->d_table)
		return false;

	size_t total_elements = gpu_emb->vocab_size * gpu_emb->embedding_dim;
	if (total_elements == 0)
		return true;
	if (!CUDA_CHECK(cudaMemcpy(gpu_emb->d_table_master,
				   h_embeddings,
				   total_elements * sizeof(float),
				   cudaMemcpyHostToDevice))) {
		return false;
	}

	int threads = 256;
	int blocks = static_cast<int>((total_elements + threads - 1) / threads);
	float_to_bf16_array_kernel<<<blocks, threads>>>(
		static_cast<const float*>(gpu_emb->d_table_master),
		static_cast<__nv_bfloat16*>(gpu_emb->d_table),
		total_elements);
	return CUDA_CHECK(cudaGetLastError());
}

extern "C" CUDA_API bool download_embeddings(float* h_embeddings, const GpuEmbeddings* gpu_emb) {
	if (!h_embeddings || !gpu_emb || !gpu_emb->d_table_master)
		return false;

	size_t total_elements = gpu_emb->vocab_size * gpu_emb->embedding_dim;
	return CUDA_CHECK(cudaMemcpy(h_embeddings,
				     gpu_emb->d_table_master,
				     total_elements * sizeof(float),
				     cudaMemcpyDeviceToHost));
}

extern "C" CUDA_API void free_gpu_activations(GpuActivations* gpu) {
	if (!gpu)
		return;
	if (gpu->d_output) {
		free_vram(gpu->d_output, 0);
		gpu->d_output = nullptr;
	}
	if (gpu->d_loss) {
		free_vram(gpu->d_loss, 0);
		gpu->d_loss = nullptr;
	}
	if (gpu->d_inv_rms) {
		free_vram(gpu->d_inv_rms, 0);
		gpu->d_inv_rms = nullptr;
	}
	if (gpu->d_probs) {
		free_vram(gpu->d_probs, 0);
		gpu->d_probs = nullptr;
	}
	if (gpu->d_pred_id) {
		free_vram(gpu->d_pred_id, 0);
		gpu->d_pred_id = nullptr;
	}
	if (gpu->d_dz) {
		free_vram(gpu->d_dz, 0);
		gpu->d_dz = nullptr;
	}
	if (gpu->h_pinned_loss) {
		pinned_free(gpu->h_pinned_loss);
		gpu->h_pinned_loss = nullptr;
	}
	if (gpu->loss_event) {
		cudaEventDestroy((cudaEvent_t)gpu->loss_event);
		gpu->loss_event = nullptr;
	}

	// Free Transformer intermediate activations
	void** xform_acts[] = {&gpu->d_query,
			       &gpu->d_key,
			       &gpu->d_value,
			       &gpu->d_attention_output,
			       &gpu->d_intermediate_gate,
			       &gpu->d_intermediate_up,
			       &gpu->d_intermediate_swiglu,
			       &gpu->d_ffn_output,
			       &gpu->d_residual_mid,
			       &gpu->d_residual_out,
			       &gpu->d_normalized_attention,
			       &gpu->d_normalized_feedforward,
			       &gpu->d_attention_scale,
			       &gpu->d_feedforward_scale,
			       &gpu->d_router_logits,
			       &gpu->d_expert_first,
			       &gpu->d_expert_second,
			       &gpu->d_weight_first,
			       &gpu->d_weight_second,
			       &gpu->d_expert_histogram};
	for (void** ptr : xform_acts) {
		if (*ptr) {
			free_vram(*ptr, 0);
			*ptr = nullptr;
		}
	}

	gpu->loss_pending = false;
	gpu->output_elements = 0;
	gpu->vocab_elements = 0;
	gpu->xform_seq_elements = 0;
	gpu->xform_ffn_elements = 0;

	void** persistent_scratch[] = {
		&gpu->d_seq_logits, &gpu->d_dh_bf16, &gpu->d_grad_table, &gpu->d_bwd_scratch};
	for (void** ptr : persistent_scratch) {
		if (*ptr) {
			cudaFree(*ptr);
			*ptr = nullptr;
		}
	}
	gpu->logits_capacity = 0;
	gpu->dh_capacity = 0;
	gpu->grad_table_capacity = 0;
	gpu->bwd_scratch_bytes = 0;
}

extern "C" CUDA_API void* pinned_alloc(size_t bytes) {
	void* ptr = nullptr;
	cudaError_t err = cudaHostAlloc(&ptr, bytes, cudaHostAllocWriteCombined);
	if (err != cudaSuccess) {
		fprintf(stderr,
			"[CUDA] pinned_alloc failed (%zu bytes): %s\n",
			bytes,
			cudaGetErrorString(err));
		return nullptr;
	}
	return ptr;
}

extern "C" CUDA_API void pinned_free(void* ptr) {
	if (ptr)
		cudaFreeHost(ptr);
}


extern "C" CUDA_API void* allocate_device_memory(size_t bytes) {
	void* ptr = nullptr;
	if (cudaMalloc(&ptr, bytes) == cudaSuccess) {
		return ptr;
	}
	if (cudaMallocManaged(&ptr, bytes) == cudaSuccess) {
		return ptr;
	}
	return nullptr;
}

extern "C" CUDA_API void free_device_memory(void* ptr) {
	if (ptr) {
		cudaFree(ptr);
	}
}

extern "C" CUDA_API bool upload_device_buffer(void* dst, const void* src, size_t bytes) {
	if (!dst || !src || bytes == 0)
		return false;
	return CUDA_CHECK(cudaMemcpy(dst, src, bytes, cudaMemcpyHostToDevice));
}

extern "C" CUDA_API bool download_device_buffer(void* dst, const void* src, size_t bytes) {
	if (!dst || !src || bytes == 0)
		return false;
	return CUDA_CHECK(cudaMemcpy(dst, src, bytes, cudaMemcpyDeviceToHost));
}

extern "C" CUDA_API bool register_host_memory(void* ptr, size_t bytes) {
	if (!ptr || bytes == 0)
		return false;
	return CUDA_CHECK(cudaHostRegister(ptr, bytes, cudaHostRegisterReadOnly));
}

extern "C" CUDA_API void unregister_host_memory(void* ptr) {
	if (ptr)
		cudaHostUnregister(ptr);
}

extern "C" CUDA_API void* create_cuda_stream() {
	cudaStream_t stream = nullptr;
	if (cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking) == cudaSuccess) {
		return stream;
	}
	return nullptr;
}

extern "C" CUDA_API void destroy_cuda_stream(void* stream) {
	if (stream)
		cudaStreamDestroy(static_cast<cudaStream_t>(stream));
}

extern "C" CUDA_API void sync_cuda_stream(void* stream) {
	if (stream)
		cudaStreamSynchronize(static_cast<cudaStream_t>(stream));
}

extern "C" CUDA_API bool upload_device_buffer_async(void* dst,
						    const void* src,
						    size_t bytes,
						    void* stream) {
	if (!dst || !src || bytes == 0)
		return false;
	return CUDA_CHECK(cudaMemcpyAsync(
		dst, src, bytes, cudaMemcpyHostToDevice, static_cast<cudaStream_t>(stream)));
}

extern "C" CUDA_API bool download_device_buffer_async(void* dst,
						      const void* src,
						      size_t bytes,
						      void* stream) {
	if (!dst || !src || bytes == 0)
		return false;
	return CUDA_CHECK(cudaMemcpyAsync(
		dst, src, bytes, cudaMemcpyDeviceToHost, static_cast<cudaStream_t>(stream)));
}

static bool init_weight_matrix(void** d_w,
			       void** d_master,
			       void** d_m,
			       void** d_v,
			       size_t elements,
			       std::mt19937& gen,
			       std::uniform_real_distribution<float>& dis) {
	if (*d_w) { cudaFree(*d_w); *d_w = nullptr; }
	if (*d_master) { cudaFree(*d_master); *d_master = nullptr; }
	if (*d_m) { cudaFree(*d_m); *d_m = nullptr; }
	if (*d_v) { cudaFree(*d_v); *d_v = nullptr; }

	const size_t bf16_bytes = elements * sizeof(__nv_bfloat16);
	const size_t float_bytes = elements * sizeof(float);
	if (!CUDA_CHECK(cudaMalloc(d_w, bf16_bytes)) ||
	    !CUDA_CHECK(cudaMalloc(d_master, float_bytes)) ||
	    !CUDA_CHECK(cudaMalloc(d_m, float_bytes)) ||
	    !CUDA_CHECK(cudaMalloc(d_v, float_bytes)) ||
	    !CUDA_CHECK(cudaMemset(*d_m, 0, float_bytes)) ||
	    !CUDA_CHECK(cudaMemset(*d_v, 0, float_bytes))) {
		return false;
	}

	std::vector<float> init_w(elements);
	for (size_t i = 0; i < elements; ++i) {
		init_w[i] = dis(gen);
	}
	if (!CUDA_CHECK(cudaMemcpy(*d_master, init_w.data(), float_bytes, cudaMemcpyHostToDevice))) {
		return false;
	}

	int threads = 256;
	int blocks = static_cast<int>((elements + threads - 1) / threads);
	float_to_bf16_array_kernel<<<blocks, threads>>>(
		static_cast<const float*>(*d_master),
		static_cast<__nv_bfloat16*>(*d_w),
		elements);
	return CUDA_CHECK(cudaGetLastError());
}

extern "C" CUDA_API bool allocate_transformer_weights(GpuWeights* gpu,
						      size_t hidden_dim,
						      size_t feedforward_network_dimension) {
	if (!gpu || hidden_dim == 0 || feedforward_network_dimension == 0)
		return false;
	gpu->transformer_dim = hidden_dim;
	gpu->transformer_ffn = feedforward_network_dimension;

	std::mt19937 gen(42);
	float bound = 1.0f / sqrtf(static_cast<float>(hidden_dim));
	std::uniform_real_distribution<float> dis(-bound, bound);

	const size_t dd_elems = hidden_dim * hidden_dim;
	const size_t df_elems = hidden_dim * feedforward_network_dimension;

	bool ok =
		init_weight_matrix(&gpu->d_query_weights, &gpu->d_query_master, &gpu->d_query_m, &gpu->d_query_v, dd_elems, gen, dis) &&
		init_weight_matrix(&gpu->d_key_weights, &gpu->d_key_master, &gpu->d_key_m, &gpu->d_key_v, dd_elems, gen, dis) &&
		init_weight_matrix(&gpu->d_value_weights, &gpu->d_value_master, &gpu->d_value_m, &gpu->d_value_v, dd_elems, gen, dis) &&
		init_weight_matrix(&gpu->d_output_weights, &gpu->d_output_master, &gpu->d_output_m, &gpu->d_output_v, dd_elems, gen, dis) &&
		init_weight_matrix(&gpu->d_gate_weights, &gpu->d_gate_master, &gpu->d_gate_m, &gpu->d_gate_v, df_elems, gen, dis) &&
		init_weight_matrix(&gpu->d_up_weights, &gpu->d_up_master, &gpu->d_up_m, &gpu->d_up_v, df_elems, gen, dis) &&
		init_weight_matrix(&gpu->d_down_weights, &gpu->d_down_master, &gpu->d_down_m, &gpu->d_down_v, df_elems, gen, dis);

	return ok && CUDA_CHECK(cudaDeviceSynchronize());
}

extern "C" CUDA_API bool allocate_expert_weights(GpuWeights* gpu,
						 size_t hidden_dim,
						 size_t feedforward_network_dimension,
						 size_t num_experts) {
	if (!gpu || hidden_dim == 0 || feedforward_network_dimension == 0 || num_experts == 0)
		return false;

	gpu->enable_moe = true;
	gpu->expert_count = num_experts;
	gpu->transformer_dim = hidden_dim;
	gpu->transformer_ffn = feedforward_network_dimension;

	size_t router_elements = hidden_dim * num_experts;
	size_t ffn_expert_elements = num_experts * hidden_dim * feedforward_network_dimension;

	std::mt19937 gen(1337);
	float bound = 1.0f / sqrtf(static_cast<float>(hidden_dim));
	std::uniform_real_distribution<float> dis(-bound, bound);

	bool ok =
		init_weight_matrix(&gpu->d_router_weights, &gpu->d_router_master, &gpu->d_router_m, &gpu->d_router_v, router_elements, gen, dis) &&
		init_weight_matrix(&gpu->d_expert_gate, &gpu->d_expert_gate_master, &gpu->d_expert_gate_m, &gpu->d_expert_gate_v, ffn_expert_elements, gen, dis) &&
		init_weight_matrix(&gpu->d_expert_up, &gpu->d_expert_up_master, &gpu->d_expert_up_m, &gpu->d_expert_up_v, ffn_expert_elements, gen, dis) &&
		init_weight_matrix(&gpu->d_expert_down, &gpu->d_expert_down_master, &gpu->d_expert_down_m, &gpu->d_expert_down_v, ffn_expert_elements, gen, dis);

	return ok && CUDA_CHECK(cudaDeviceSynchronize());
}

extern "C" CUDA_API void free_expert_weights(GpuWeights* gpu) {
	if (!gpu)
		return;

	void** moe_weights[] = {&gpu->d_router_weights,
				&gpu->d_router_master,
				&gpu->d_router_m,
				&gpu->d_router_v,
				&gpu->d_expert_gate,
				&gpu->d_expert_gate_master,
				&gpu->d_expert_gate_m,
				&gpu->d_expert_gate_v,
				&gpu->d_expert_up,
				&gpu->d_expert_up_master,
				&gpu->d_expert_up_m,
				&gpu->d_expert_up_v,
				&gpu->d_expert_down,
				&gpu->d_expert_down_master,
				&gpu->d_expert_down_m,
				&gpu->d_expert_down_v};
	for (void** ptr : moe_weights) {
		if (*ptr) {
			cudaFree(*ptr);
			*ptr = nullptr;
		}
	}
	gpu->enable_moe = false;
	gpu->expert_count = 0;
}
