#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cmath>
#include "swiglu.cuh"

__global__ void swiglu_bf16_kernel(
	const __nv_bfloat16* __restrict__ d_gate,
	const __nv_bfloat16* __restrict__ d_up,
	__nv_bfloat16* __restrict__ d_out,
	size_t total_elements
) {
	size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float g = __bfloat162float(d_gate[idx]);
		float u = __bfloat162float(d_up[idx]);
		float silu_g = g / (1.0f + expf(-g));
		d_out[idx] = __float2bfloat16(silu_g * u);
	}
}

extern "C" CUDA_API void launch_swiglu_bf16(
	const void* d_gate,
	const void* d_up,
	void* d_out,
	size_t total_elements,
	void* stream
) {
	int threads = 256;
	int blocks = static_cast<int>((total_elements + threads - 1) / threads);
	swiglu_bf16_kernel<<<blocks, threads, 0, static_cast<cudaStream_t>(stream)>>>(
		static_cast<const __nv_bfloat16*>(d_gate),
		static_cast<const __nv_bfloat16*>(d_up),
		static_cast<__nv_bfloat16*>(d_out),
		total_elements
	);
}

