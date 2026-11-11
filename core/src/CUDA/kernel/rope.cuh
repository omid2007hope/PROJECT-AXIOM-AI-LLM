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
