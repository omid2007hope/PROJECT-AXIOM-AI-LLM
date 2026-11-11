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

#ifndef MOE_ROUTER_CUH
#define MOE_ROUTER_CUH

#ifdef __CUDACC__
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#endif
#include <cstddef>
#include <cstdint>

#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API bool forward_mixture_experts(void* hidden_states,
				      const GpuWeights* gpu_weights,
				      GpuActivations* gpu_activations,
				      size_t sequence_length,
				      size_t embedding_dim,
				      size_t feedforward_network_dimension,
				      void* stream);

CUDA_API bool download_expert_counts(const GpuActivations* gpu_activations,
				     int32_t* h_histogram,
				     size_t num_experts);

CUDA_API bool reset_expert_histogram(GpuActivations* gpu_activations,
				     size_t num_experts,
				     void* stream);

#ifdef __cplusplus
}
#endif

#endif // MOE_ROUTER_CUH
