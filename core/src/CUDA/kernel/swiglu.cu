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

__global__ static void swiglu_backward_kernel(
	const __nv_bfloat16* __restrict__ grad_output,
	const __nv_bfloat16* __restrict__ gate,
	const __nv_bfloat16* __restrict__ up,
	float* __restrict__ grad_gate,
	float* __restrict__ grad_up,
	size_t total_elements
) {
	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float dh = __bfloat162float(grad_output[idx]);
		float g = __bfloat162float(gate[idx]);
		float u = __bfloat162float(up[idx]);

		float sig = 1.0f / (1.0f + expf(-g));
		float swish = g * sig;
		float swish_prime = sig + g * sig * (1.0f - sig);

		grad_up[idx] = dh * swish;
		grad_gate[idx] = dh * u * swish_prime;
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

extern "C" CUDA_API bool swiglu_forward(
	const void* gate,
	const void* up,
	void* output,
	size_t total_elements,
	void* stream
) {
	if (!gate || !up || !output || total_elements == 0) {
		return false;
	}

	int threads = 256;
	int blocks = static_cast<int>((total_elements + threads - 1) / threads);
	cudaStream_t s = static_cast<cudaStream_t>(stream);

	swiglu_bf16_kernel<<<blocks, threads, 0, s>>>(
		static_cast<const __nv_bfloat16*>(gate),
		static_cast<const __nv_bfloat16*>(up),
		static_cast<__nv_bfloat16*>(output),
		total_elements
	);

	return cudaGetLastError() == cudaSuccess;
}

extern "C" CUDA_API bool swiglu_backward(
	const void* grad_output,
	const void* gate,
	const void* up,
	void* grad_gate,
	void* grad_up,
	size_t total_elements,
	void* stream
) {
	if (!grad_output || !gate || !up || !grad_gate || !grad_up || total_elements == 0) {
		return false;
	}

	cudaStream_t s = static_cast<cudaStream_t>(stream);
	int threads = 256;
	int blocks = static_cast<int>((total_elements + threads - 1) / threads);

	swiglu_backward_kernel<<<blocks, threads, 0, s>>>(
		static_cast<const __nv_bfloat16*>(grad_output),
		static_cast<const __nv_bfloat16*>(gate),
		static_cast<const __nv_bfloat16*>(up),
		static_cast<float*>(grad_gate),
		static_cast<float*>(grad_up),
		total_elements
	);

	return cudaGetLastError() == cudaSuccess;
}
