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
#include <cstdint>
#include "rope.cuh"

__device__ __forceinline__ float load_scalar(const __nv_bfloat16* p) {
	return __bfloat162float(*p);
}

__device__ __forceinline__ float load_scalar(const float* p) {
	return *p;
}

__device__ __forceinline__ void store_scalar(__nv_bfloat16* p, float val) {
	*p = __float2bfloat16(val);
}

__device__ __forceinline__ void store_scalar(float* p, float val) {
	*p = val;
}

template <typename ScalarT, int Sign>
__global__ void rope_rotate_kernel(
	ScalarT* __restrict__ d_q,
	ScalarT* __restrict__ d_k,
	const int32_t* __restrict__ d_pos,
	int num_tokens,
	int num_q_heads,
	int num_kv_heads,
	int head_dim,
	float rope_theta
) {
	int token_idx = blockIdx.x;
	if (token_idx >= num_tokens) return;

	int half_dim = head_dim >> 1;
	int pos = d_pos ? d_pos[token_idx] : token_idx;
	int total_pairs = (num_q_heads + num_kv_heads) * half_dim;

	for (int idx = threadIdx.x; idx < total_pairs; idx += blockDim.x) {
		int head_idx = idx / half_dim;
		int pair_idx = idx % half_dim;

		float exponent = static_cast<float>(2 * pair_idx) / static_cast<float>(head_dim);
		float freq = 1.0f / powf(rope_theta, exponent);
		float angle = static_cast<float>(pos) * freq;
		float cos_val, sin_val;
		__sincosf(angle, &sin_val, &cos_val);
		float sin_eff = static_cast<float>(Sign) * sin_val;

		ScalarT* head_ptr = (head_idx < num_q_heads)
			? (d_q + static_cast<size_t>(token_idx) * (num_q_heads * head_dim) + head_idx * head_dim)
			: (d_k + static_cast<size_t>(token_idx) * (num_kv_heads * head_dim) + (head_idx - num_q_heads) * head_dim);

		float v0 = load_scalar(&head_ptr[pair_idx]);
		float v1 = load_scalar(&head_ptr[pair_idx + half_dim]);
		store_scalar(&head_ptr[pair_idx],            v0 * cos_val - v1 * sin_eff);
		store_scalar(&head_ptr[pair_idx + half_dim], v1 * cos_val + v0 * sin_eff);
	}
}

extern "C" CUDA_API void launch_rope_inplace_bf16(
	void* d_q,
	void* d_k,
	const int32_t* d_pos,
	int num_tokens,
	int num_q_heads,
	int num_kv_heads,
	int head_dim,
	float rope_theta,
	void* stream
) {
	if (!d_q || !d_k || num_tokens <= 0 || num_q_heads <= 0 || num_kv_heads <= 0 ||
	    head_dim < 2 || (head_dim & 1) != 0) {
		return;
	}
	rope_rotate_kernel<__nv_bfloat16, 1><<<num_tokens, 256, 0, static_cast<cudaStream_t>(stream)>>>(
		static_cast<__nv_bfloat16*>(d_q),
		static_cast<__nv_bfloat16*>(d_k),
		d_pos,
		num_tokens,
		num_q_heads,
		num_kv_heads,
		head_dim,
		rope_theta
	);
}

extern "C" CUDA_API void launch_rope_sequence(
	void* d_q,
	void* d_k,
	int num_tokens,
	int num_heads,
	int head_dim,
	float rope_theta,
	void* stream
) {
	launch_rope_inplace_bf16(
		d_q, d_k, nullptr, num_tokens, num_heads, num_heads, head_dim, rope_theta, stream);
}

extern "C" CUDA_API void launch_rope_backward(
	void* d_grad_q,
	void* d_grad_k,
	int num_tokens,
	int num_heads,
	int head_dim,
	float rope_theta,
	void* stream
) {
	if (!d_grad_q || !d_grad_k || num_tokens <= 0 || num_heads <= 0 ||
	    head_dim < 2 || (head_dim & 1) != 0) {
		return;
	}
	rope_rotate_kernel<float, -1><<<num_tokens, 256, 0, static_cast<cudaStream_t>(stream)>>>(
		static_cast<float*>(d_grad_q),
		static_cast<float*>(d_grad_k),
		nullptr,
		num_tokens,
		num_heads,
		num_heads,
		head_dim,
		rope_theta
	);
}

