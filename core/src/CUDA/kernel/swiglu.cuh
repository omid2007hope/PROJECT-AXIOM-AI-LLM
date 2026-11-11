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

#ifndef SWIGLU_CUH
#define SWIGLU_CUH

#ifdef __CUDACC__
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#endif
#include <cstddef>
#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API void launch_swiglu_bf16(
	const void* d_gate,
	const void* d_up,
	void* d_out,
	size_t total_elements,
	void* stream
);

CUDA_API bool swiglu_forward(
	const void* gate,
	const void* up,
	void* output,
	size_t total_elements,
	void* stream
);

CUDA_API bool swiglu_backward(
	const void* grad_output,
	const void* gate,
	const void* up,
	void* grad_gate,
	void* grad_up,
	size_t total_elements,
	void* stream
);

#ifdef __cplusplus
}
#endif

#endif // SWIGLU_CUH
