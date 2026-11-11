/*
 * Project Axiom — Built from scratch. Engineered for the future.
 * Copyright (c) 2026 Omid Teimory. All rights reserved.
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
