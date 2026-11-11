/*
 * Copyright (c) 2026 Omid Teimory. All Rights Reserved.
 *
 * NOTICE: This file contains proprietary source code licensed exclusively under the
 * terms of the LICENSE file in the root directory of this repository.
 * You may view, download, and compile this code solely to run the unmodified product.
 * Copying, pasting, modifying, redistributing, creating derivative works, or using
 * this code in any other project or AI training system is strictly prohibited.
 * Removal or alteration of this notice is a violation of 17 U.S.C. § 1202 and EU law.
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
