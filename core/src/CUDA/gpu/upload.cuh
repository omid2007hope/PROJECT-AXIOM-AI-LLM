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

CUDA_API bool upload_transformer_weights(GpuWeights* gpu,
					 const float* h_q,
					 const float* h_k,
					 const float* h_v,
					 const float* h_o,
					 const float* h_gate,
					 const float* h_up,
					 const float* h_down,
					 size_t dim,
					 size_t ffn);

#ifdef __cplusplus
}
#endif

#endif // UPLOAD_CUH
