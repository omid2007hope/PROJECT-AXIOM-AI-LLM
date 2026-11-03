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

#ifdef __cplusplus
}
#endif

#endif // SWIGLU_CUH
