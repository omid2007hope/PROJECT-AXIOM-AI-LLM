/*
 * Project Axiom — Built from scratch. Engineered for the future.
 * Copyright (c) 2026 Omid Teimory. All rights reserved.
 */

#ifndef ROPE_CUH
#define ROPE_CUH

#ifdef __CUDACC__
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#endif
#include <cstdint>
#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API void launch_rope_inplace_bf16(
	void* d_q,
	void* d_k,
	const int32_t* d_pos,
	int num_tokens,
	int num_q_heads,
	int num_kv_heads,
	int head_dim,
	float rope_theta,
	void* stream
);

#ifdef __cplusplus
}
#endif

#endif // ROPE_CUH
