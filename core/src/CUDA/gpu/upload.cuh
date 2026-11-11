/*
 * Project Axiom — Built from scratch. Engineered for the future.
 * Copyright (c) 2026 Omid Teimory. All rights reserved.
 */

#ifndef UPLOAD_CUH
#define UPLOAD_CUH

#include <cstdint>

#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API bool upload_to_gpu(const void* h_context,
			    size_t context_elements,
			    const void* h_target,
			    size_t target_elements,
			    GpuPortion* gpu,
			    int buffer_index);

CUDA_API bool upload_tokens_to_gpu(const uint32_t* h_token_ids,
				   size_t token_count,
				   const void* h_target,
				   size_t target_elements,
				   GpuPortion* gpu,
				   int buffer_index);

CUDA_API bool init_gpu_portion(GpuPortion* gpu, size_t max_batch_tokens, size_t hidden_dim);
CUDA_API void upload_tokens_async(GpuPortion* gpu, int stream_idx, const uint32_t* h_pinned_tokens, size_t count);

CUDA_API void synchronize_stream(GpuPortion* gpu, int buffer_index);

CUDA_API bool upload_ai_weights(const float* h_weights,
				size_t weight_elements,
				const float* h_bias,
				size_t bias_elements,
				GpuWeights* gpu);

#ifdef __cplusplus
}
#endif

#endif // UPLOAD_CUH
