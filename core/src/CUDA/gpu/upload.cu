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

#include <cuda_bf16.h>
#include <cuda_runtime.h>

#include "../util/caching_allocator.cuh"
#include "../util/cuda_utils.cuh"
#include "upload.cuh"

extern "C" CUDA_API bool upload_to_gpu(const void* h_context,
				       size_t context_elements,
				       const void* h_target,
				       size_t target_elements,
				       GpuPortion* gpu,
				       int buffer_index) {
	if (!gpu || !h_context || !h_target || context_elements == 0 || target_elements == 0 ||
	    buffer_index < 0 || buffer_index >= 2) {
		fprintf(stderr, "Invalid memory parameters in upload_to_gpu\n");
		return false;
	}

	// ── Stream and Event Initialization ─────────────────────────────────────────
	for (size_t i = 0; i < gpu->context_ptr; ++i) {
		if (gpu->stream[i] == nullptr) {
			CUDA_CHECK(cudaStreamCreateWithFlags((cudaStream_t*)&gpu->stream[i],
							     cudaStreamNonBlocking));
		}

		if (gpu->compute_done[i] == nullptr) {
			if (!CUDA_CHECK(cudaEventCreateWithFlags((cudaEvent_t*)&gpu->compute_done[i],
								 cudaEventDisableTiming)))
				return false;
		}
	}

	// ── Context Window (X) — Userspace BFC Caching Allocator & Stream Memory Pool
	if (gpu->context_elements != context_elements || !gpu->d_context_window[buffer_index]) {
		for (size_t i = 0; i < gpu->context_ptr; ++i) {
			if (gpu->d_context_window[i]) {
				free_vram(gpu->d_context_window[i], gpu->stream[i]);
				gpu->d_context_window[i] = nullptr;
			}
			gpu->d_context_window[i] =
				allocate_vram(context_elements * sizeof(__nv_bfloat16), gpu->stream[i]);
			if (!gpu->d_context_window[i])
				return false;
		}
		gpu->context_elements = context_elements;
	}
	gpu->token_ids_active[buffer_index] = false; // float context: nothing to gather

	if (!CUDA_CHECK(cudaMemcpyAsync(gpu->d_context_window[buffer_index],
					h_context,
					context_elements * sizeof(__nv_bfloat16),
					cudaMemcpyHostToDevice,
					(cudaStream_t)gpu->stream[buffer_index])))
		return false;

	// ── Target (Y) — Userspace BFC Caching Allocator & Stream Memory Pool ───────
	if (gpu->target_elements != target_elements || !gpu->d_target[buffer_index]) {
		for (size_t i = 0; i < gpu->target_ptr; ++i) {
			if (gpu->d_target[i]) {
				free_vram(gpu->d_target[i], gpu->stream[i]);
				gpu->d_target[i] = nullptr;
			}
			gpu->d_target[i] =
				allocate_vram(target_elements * sizeof(__nv_bfloat16), gpu->stream[i]);
			if (!gpu->d_target[i])
				return false;
		}
		gpu->target_elements = target_elements;
	}

	if (!CUDA_CHECK(cudaMemcpyAsync(gpu->d_target[buffer_index],
					h_target,
					target_elements * sizeof(__nv_bfloat16),
					cudaMemcpyHostToDevice,
					(cudaStream_t)gpu->stream[buffer_index])))
		return false;

	return true;
}

extern "C" CUDA_API bool upload_tokens_to_gpu(const uint32_t* h_token_ids,
					       size_t token_count,
					       const void* h_target,
					       size_t target_elements,
					       GpuPortion* gpu,
					       int buffer_index) {
	if (!gpu || !h_token_ids || !h_target || token_count == 0 || target_elements == 0 ||
	    buffer_index < 0 || buffer_index >= 2) {
		fprintf(stderr, "Invalid memory parameters in upload_tokens_to_gpu\n");
		return false;
	}

	// ── Stream and Event Initialization ─────────────────────────────────────────
	for (size_t i = 0; i < gpu->context_ptr; ++i) {
		if (gpu->stream[i] == nullptr) {
			CUDA_CHECK(cudaStreamCreateWithFlags((cudaStream_t*)&gpu->stream[i],
							     cudaStreamNonBlocking));
		}

		if (gpu->compute_done[i] == nullptr) {
			if (!CUDA_CHECK(cudaEventCreateWithFlags((cudaEvent_t*)&gpu->compute_done[i],
								 cudaEventDisableTiming)))
				return false;
		}
	}

	// ── Token IDs — Userspace BFC Caching Allocator & Stream Memory Pool ────────
	if (gpu->token_count != token_count || !gpu->d_token_ids[buffer_index]) {
		for (size_t i = 0; i < gpu->context_ptr; ++i) {
			if (gpu->d_token_ids[i]) {
				free_vram(gpu->d_token_ids[i], gpu->stream[i]);
				gpu->d_token_ids[i] = nullptr;
			}
			gpu->d_token_ids[i] =
				allocate_vram(token_count * sizeof(uint32_t), gpu->stream[i]);
			if (!gpu->d_token_ids[i])
				return false;
		}
		gpu->token_count = token_count;
	}
	gpu->token_ids_active[buffer_index] = true;

	if (!CUDA_CHECK(cudaMemcpyAsync(gpu->d_token_ids[buffer_index],
					h_token_ids,
					token_count * sizeof(uint32_t),
					cudaMemcpyHostToDevice,
					(cudaStream_t)gpu->stream[buffer_index])))
		return false;

	// ── Target (Y) — Userspace BFC Caching Allocator & Stream Memory Pool ───────
	if (gpu->target_elements != target_elements || !gpu->d_target[buffer_index]) {
		for (size_t i = 0; i < gpu->target_ptr; ++i) {
			if (gpu->d_target[i]) {
				free_vram(gpu->d_target[i], gpu->stream[i]);
				gpu->d_target[i] = nullptr;
			}
			gpu->d_target[i] =
				allocate_vram(target_elements * sizeof(__nv_bfloat16), gpu->stream[i]);
			if (!gpu->d_target[i])
				return false;
		}
		gpu->target_elements = target_elements;
	}

	if (!CUDA_CHECK(cudaMemcpyAsync(gpu->d_target[buffer_index],
					h_target,
					target_elements * sizeof(__nv_bfloat16),
					cudaMemcpyHostToDevice,
					(cudaStream_t)gpu->stream[buffer_index])))
		return false;

	return true;
}

__global__ static void float_to_bf16_kernel(const float* __restrict__ src,
					    __nv_bfloat16* __restrict__ dst,
					    size_t n) {
	size_t i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i < n)
		dst[i] = __float2bfloat16(src[i]);
}

extern "C" CUDA_API bool upload_ai_weights(const float* h_weights,
					   size_t weight_elements,
					   const float* h_bias,
					   size_t bias_elements,
					   GpuWeights* gpu) {

	if (!gpu || !h_weights || !h_bias || weight_elements == 0 || bias_elements == 0) {
		fprintf(stderr, "Invalid parameters in upload_ai_weights\n");
		return false;
	}

	// A reset can arrive mid-training: drain every in-flight kernel first so none of
	// them is still updating the weights while they are overwritten below.
	if (!CUDA_CHECK(cudaDeviceSynchronize()))
		return false;

	// ── Weights ───────────────────────────────────────────────────────────────
	if (gpu->weight_elements != weight_elements) {
		if (gpu->d_weights) {
			if (!CUDA_CHECK(cudaFree(gpu->d_weights)))
				return false;
			gpu->d_weights = nullptr;
		}
		if (gpu->d_weights_master) {
			if (!CUDA_CHECK(cudaFree(gpu->d_weights_master)))
				return false;
			gpu->d_weights_master = nullptr;
		}
		if (!CUDA_CHECK(
			    cudaMalloc(&gpu->d_weights, weight_elements * sizeof(__nv_bfloat16))))
			return false;
		if (!CUDA_CHECK(cudaMalloc(&gpu->d_weights_master, weight_elements * sizeof(float))))
			return false;
		gpu->weight_elements = weight_elements;
	}

	if (!CUDA_CHECK(cudaMemcpy(gpu->d_weights_master,
				   h_weights,
				   weight_elements * sizeof(float),
				   cudaMemcpyHostToDevice)))
		return false;

	int threads = 256;
	int blocks = static_cast<int>((weight_elements + threads - 1) / threads);
	cudaGetLastError();
	float_to_bf16_kernel<<<blocks, threads>>>(static_cast<const float*>(gpu->d_weights_master),
						  static_cast<__nv_bfloat16*>(gpu->d_weights),
						  weight_elements);
	if (!CUDA_CHECK(cudaGetLastError()))
		return false;

	// ── Bias ──────────────────────────────────────────────────────────────────
	if (gpu->bias_elements != bias_elements) {
		if (gpu->d_bias) {
			if (!CUDA_CHECK(cudaFree(gpu->d_bias)))
				return false;
			gpu->d_bias = nullptr;
		}
		if (gpu->d_bias_master) {
			if (!CUDA_CHECK(cudaFree(gpu->d_bias_master)))
				return false;
			gpu->d_bias_master = nullptr;
		}
		if (!CUDA_CHECK(cudaMalloc(&gpu->d_bias, bias_elements * sizeof(__nv_bfloat16))))
			return false;
		if (!CUDA_CHECK(cudaMalloc(&gpu->d_bias_master, bias_elements * sizeof(float))))
			return false;
		gpu->bias_elements = bias_elements;
	}

	if (!CUDA_CHECK(cudaMemcpy(
		    gpu->d_bias_master, h_bias, bias_elements * sizeof(float), cudaMemcpyHostToDevice)))
		return false;

	blocks = static_cast<int>((bias_elements + threads - 1) / threads);
	cudaGetLastError();
	float_to_bf16_kernel<<<blocks, threads>>>(static_cast<const float*>(gpu->d_bias_master),
						  static_cast<__nv_bfloat16*>(gpu->d_bias),
						  bias_elements);
	if (!CUDA_CHECK(cudaGetLastError()))
		return false;

	// Make the bf16 copies visible to the training streams
	return CUDA_CHECK(cudaDeviceSynchronize());
}

extern "C" CUDA_API bool upload_transformer_weights(GpuWeights* gpu,
													 const float* h_q,
													 const float* h_k,
													 const float* h_v,
													 const float* h_o,
													 const float* h_gate,
													 const float* h_up,
													 const float* h_down,
													 size_t dim,
													 size_t ffn) {
	if (!gpu || !h_q || !h_k || !h_v || !h_o || !h_gate || !h_up || !h_down ||
		!gpu->d_query_master || !gpu->d_key_master || !gpu->d_value_master ||
		!gpu->d_output_master || !gpu->d_gate_master || !gpu->d_up_master ||
		!gpu->d_down_master || gpu->transformer_dim != dim || gpu->transformer_ffn != ffn) {
		return false;
	}

	const size_t dd_elements = dim * dim;
	const size_t df_elements = dim * ffn;
	const size_t dd_bytes = dd_elements * sizeof(float);
	const size_t df_bytes = df_elements * sizeof(float);

	if (!CUDA_CHECK(cudaMemcpy(gpu->d_query_master, h_q, dd_bytes, cudaMemcpyHostToDevice)))
		return false;
	if (!CUDA_CHECK(cudaMemcpy(gpu->d_key_master, h_k, dd_bytes, cudaMemcpyHostToDevice)))
		return false;
	if (!CUDA_CHECK(cudaMemcpy(gpu->d_value_master, h_v, dd_bytes, cudaMemcpyHostToDevice)))
		return false;
	if (!CUDA_CHECK(cudaMemcpy(gpu->d_output_master, h_o, dd_bytes, cudaMemcpyHostToDevice)))
		return false;
	if (!CUDA_CHECK(cudaMemcpy(gpu->d_gate_master, h_gate, df_bytes, cudaMemcpyHostToDevice)))
		return false;
	if (!CUDA_CHECK(cudaMemcpy(gpu->d_up_master, h_up, df_bytes, cudaMemcpyHostToDevice)))
		return false;
	if (!CUDA_CHECK(cudaMemcpy(gpu->d_down_master, h_down, df_bytes, cudaMemcpyHostToDevice)))
		return false;

	constexpr int threads = 256;
	const int dd_blocks = static_cast<int>((dd_elements + threads - 1) / threads);
	const int df_blocks = static_cast<int>((df_elements + threads - 1) / threads);

	cudaGetLastError();
	float_to_bf16_kernel<<<dd_blocks, threads>>>(static_cast<const float*>(gpu->d_query_master),
												 static_cast<__nv_bfloat16*>(gpu->d_query_weights),
												 dd_elements);
	float_to_bf16_kernel<<<dd_blocks, threads>>>(static_cast<const float*>(gpu->d_key_master),
												 static_cast<__nv_bfloat16*>(gpu->d_key_weights),
												 dd_elements);
	float_to_bf16_kernel<<<dd_blocks, threads>>>(static_cast<const float*>(gpu->d_value_master),
												 static_cast<__nv_bfloat16*>(gpu->d_value_weights),
												 dd_elements);
	float_to_bf16_kernel<<<dd_blocks, threads>>>(static_cast<const float*>(gpu->d_output_master),
												 static_cast<__nv_bfloat16*>(gpu->d_output_weights),
												 dd_elements);

	float_to_bf16_kernel<<<df_blocks, threads>>>(static_cast<const float*>(gpu->d_gate_master),
												 static_cast<__nv_bfloat16*>(gpu->d_gate_weights),
												 df_elements);
	float_to_bf16_kernel<<<df_blocks, threads>>>(static_cast<const float*>(gpu->d_up_master),
												 static_cast<__nv_bfloat16*>(gpu->d_up_weights),
												 df_elements);
	float_to_bf16_kernel<<<df_blocks, threads>>>(static_cast<const float*>(gpu->d_down_master),
												 static_cast<__nv_bfloat16*>(gpu->d_down_weights),
												 df_elements);

	if (!CUDA_CHECK(cudaGetLastError()))
		return false;

	return CUDA_CHECK(cudaDeviceSynchronize());
}

extern "C" CUDA_API void synchronize_stream(GpuPortion* gpu, int buffer_index) {
	if (gpu && buffer_index >= 0 && buffer_index < 2 && gpu->stream[buffer_index]) {
		CUDA_CHECK(cudaStreamSynchronize((cudaStream_t)gpu->stream[buffer_index]));
	}
}

extern "C" CUDA_API bool init_gpu_portion(GpuPortion* gpu, size_t max_batch_tokens, size_t hidden_dim) {
	if (!gpu) return false;
	if (gpu->is_allocated) return true;
	gpu->max_batch_tokens = max_batch_tokens;

	for (int s = 0; s < 2; ++s) {
		if (!gpu->stream[s]) {
			cudaStreamCreateWithFlags(reinterpret_cast<cudaStream_t*>(&gpu->stream[s]), cudaStreamNonBlocking);
		}
		if (!gpu->loss_ready_event[s]) {
			cudaEventCreateWithFlags(reinterpret_cast<cudaEvent_t*>(&gpu->loss_ready_event[s]), cudaEventDisableTiming);
		}

		if (!gpu->d_token_ids[s])
			gpu->d_token_ids[s] = allocate_vram(max_batch_tokens * sizeof(uint32_t), gpu->stream[s]);
		if (!gpu->d_context_window[s])
			gpu->d_context_window[s] = allocate_vram(max_batch_tokens * hidden_dim * sizeof(__nv_bfloat16), gpu->stream[s]);
		if (!gpu->d_hidden_state[s])
			gpu->d_hidden_state[s] = allocate_vram(max_batch_tokens * hidden_dim * sizeof(__nv_bfloat16), gpu->stream[s]);
		if (!gpu->d_loss_scalar[s])
			gpu->d_loss_scalar[s] = static_cast<float*>(allocate_vram(sizeof(float), gpu->stream[s]));

		if (!gpu->h_pinned_loss[s]) {
			cudaHostAlloc(reinterpret_cast<void**>(&gpu->h_pinned_loss[s]), sizeof(float), cudaHostAllocPortable);
			if (gpu->h_pinned_loss[s]) *(gpu->h_pinned_loss[s]) = 0.0f;
		}
	}
	gpu->is_allocated = true;
	return true;
}

extern "C" CUDA_API void upload_tokens_async(GpuPortion* gpu, int stream_idx, const uint32_t* h_pinned_tokens, size_t count) {
	if (!gpu || !h_pinned_tokens || count == 0 || stream_idx < 0 || stream_idx >= 2 || !gpu->d_token_ids[stream_idx]) return;
	cudaMemcpyAsync(
		gpu->d_token_ids[stream_idx],
		h_pinned_tokens,
		count * sizeof(uint32_t),
		cudaMemcpyHostToDevice,
		static_cast<cudaStream_t>(gpu->stream[stream_idx])
	);
}
