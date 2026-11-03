#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cmath>
#include <cstdint>
#include "rope.cuh"

__global__ void rope_inplace_bf16_kernel(
	__nv_bfloat16* __restrict__ d_q,
	__nv_bfloat16* __restrict__ d_k,
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
	int pos = d_pos[token_idx];
	int total_pairs = (num_q_heads + num_kv_heads) * half_dim;

	for (int idx = threadIdx.x; idx < total_pairs; idx += blockDim.x) {
		int head_idx = idx / half_dim;
		int pair_idx = idx % half_dim;

		float exponent = static_cast<float>(2 * pair_idx) / static_cast<float>(head_dim);
		float freq = 1.0f / powf(rope_theta, exponent);
		float angle = static_cast<float>(pos) * freq;
		float cos_val, sin_val;
		__sincosf(angle, &sin_val, &cos_val);

		if (head_idx < num_q_heads) {
			__nv_bfloat16* q_head = d_q + token_idx * (num_q_heads * head_dim) + head_idx * head_dim;
			float q0 = __bfloat162float(q_head[pair_idx]);
			float q1 = __bfloat162float(q_head[pair_idx + half_dim]);
			q_head[pair_idx]            = __float2bfloat16(q0 * cos_val - q1 * sin_val);
			q_head[pair_idx + half_dim] = __float2bfloat16(q1 * cos_val + q0 * sin_val);
		} else {
			int kv_head_idx = head_idx - num_q_heads;
			__nv_bfloat16* k_head = d_k + token_idx * (num_kv_heads * head_dim) + kv_head_idx * head_dim;
			float k0 = __bfloat162float(k_head[pair_idx]);
			float k1 = __bfloat162float(k_head[pair_idx + half_dim]);
			k_head[pair_idx]            = __float2bfloat16(k0 * cos_val - k1 * sin_val);
			k_head[pair_idx + half_dim] = __float2bfloat16(k1 * cos_val + k0 * sin_val);
		}
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
	rope_inplace_bf16_kernel<<<num_tokens, 256, 0, static_cast<cudaStream_t>(stream)>>>(
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

