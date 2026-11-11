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

#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include "rmsnorm.cuh"

__inline__ __device__ float warp_reduce_sum(float val) {
	#pragma unroll
	for (int offset = 16; offset > 0; offset >>= 1) {
		val += __shfl_xor_sync(0xffffffff, val, offset);
	}
	return val;
}

__global__ void rmsnorm_bf16_kernel(
	const __nv_bfloat16* __restrict__ d_in,
	const __nv_bfloat16* __restrict__ d_weight,
	__nv_bfloat16* __restrict__ d_out,
	int hidden_dim,
	float eps
) {
	int row = blockIdx.x;
	int tid = threadIdx.x;

	const __nv_bfloat16* row_in = d_in + row * hidden_dim;
	__nv_bfloat16* row_out      = d_out + row * hidden_dim;

	float sum_sq = 0.0f;
	for (int i = tid; i < hidden_dim; i += blockDim.x) {
		float v = __bfloat162float(row_in[i]);
		sum_sq += v * v;
	}
	sum_sq = warp_reduce_sum(sum_sq);

	__shared__ float s_warp_sums[32];
	int lane = tid & 31;
	int wid  = tid >> 5;

	if (lane == 0) s_warp_sums[wid] = sum_sq;
	__syncthreads();

	if (wid == 0) {
		float block_sum = (tid < (blockDim.x >> 5)) ? s_warp_sums[lane] : 0.0f;
		block_sum = warp_reduce_sum(block_sum);
		if (tid == 0) {
			s_warp_sums[0] = rsqrtf(block_sum / static_cast<float>(hidden_dim) + eps);
		}
	}
	__syncthreads();

	float inv_rms = s_warp_sums[0];
	for (int i = tid; i < hidden_dim; i += blockDim.x) {
		float v = __bfloat162float(row_in[i]);
		float w = __bfloat162float(d_weight[i]);
		row_out[i] = __float2bfloat16(v * inv_rms * w);
	}
}

__global__ void residual_add_bf16_kernel(
	__nv_bfloat16* __restrict__ d_residual,
	const __nv_bfloat16* __restrict__ d_proj,
	int total_elements
) {
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float r = __bfloat162float(d_residual[idx]);
		float p = __bfloat162float(d_proj[idx]);
		d_residual[idx] = __float2bfloat16(r + p);
	}
}

extern "C" CUDA_API void launch_rmsnorm_bf16(
	const void* d_in,
	const void* d_weight,
	void* d_out,
	int num_tokens,
	int hidden_dim,
	float eps,
	void* stream
) {
	int threads = (hidden_dim >= 512) ? 256 : 128;
	rmsnorm_bf16_kernel<<<num_tokens, threads, 0, static_cast<cudaStream_t>(stream)>>>(
		static_cast<const __nv_bfloat16*>(d_in),
		static_cast<const __nv_bfloat16*>(d_weight),
		static_cast<__nv_bfloat16*>(d_out),
		hidden_dim,
		eps
	);
}

extern "C" CUDA_API void launch_residual_add_bf16(
	void* d_residual_stream,
	const void* d_proj_tmp,
	int hidden_dim,
	void* stream
) {
	int threads = 256;
	int blocks = (hidden_dim + threads - 1) / threads;
	residual_add_bf16_kernel<<<blocks, threads, 0, static_cast<cudaStream_t>(stream)>>>(
		static_cast<__nv_bfloat16*>(d_residual_stream),
		static_cast<const __nv_bfloat16*>(d_proj_tmp),
		hidden_dim
	);
}

