/*
 * Project Axiom — Built from scratch. Engineered for the future.
 * Copyright (c) 2026 Omid Teimory. All rights reserved.
 */

#ifndef RMSNORM_CUH
#define RMSNORM_CUH

#ifdef __CUDACC__
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#endif
#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API void launch_rmsnorm_bf16(
	const void* d_in,
	const void* d_weight,
	void* d_out,
	int num_tokens,
	int hidden_dim,
	float eps,
	void* stream
);

CUDA_API void launch_residual_add_bf16(
	void* d_residual_stream,
	const void* d_proj_tmp,
	int hidden_dim,
	void* stream
);

#ifdef __cplusplus
}
#endif

#endif // RMSNORM_CUH
