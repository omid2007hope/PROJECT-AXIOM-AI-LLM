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

#ifndef CUDA_GRAPH_CAPTURE_CUH
#define CUDA_GRAPH_CAPTURE_CUH

#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif
#include <cstddef>
#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API bool capture_training_graph(GpuGraphExecution* graph_engine,
				     const GpuPortion* gpu_portion,
				     GpuEmbeddings* gpu_embeddings,
				     GpuWeights* gpu_weights,
				     GpuActivations* gpu_activations,
				     size_t input_dim,
				     size_t output_dim,
				     float learning_rate,
				     int buffer_index);

CUDA_API bool launch_training_graph(GpuGraphExecution* graph_engine, void* stream);

CUDA_API void free_training_graph(GpuGraphExecution* graph_engine);

CUDA_API float benchmark_graph_latency(GpuGraphExecution* graph_engine, void* stream, int iterations);

#ifdef __cplusplus
}
#endif

#endif // CUDA_GRAPH_CAPTURE_CUH

