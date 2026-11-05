#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cmath>
#include <cstdint>
#include "paged_gqa.cuh"

__global__ void append_paged_kv_kernel(
	const __nv_bfloat16* __restrict__ d_k,
	const __nv_bfloat16* __restrict__ d_v,
	__nv_bfloat16* __restrict__ d_k_cache,
	__nv_bfloat16* __restrict__ d_v_cache,
	const int32_t* __restrict__ d_block_table,
	int token_pos,
	int num_kv_heads,
	int head_dim
) {
	int kv_head = blockIdx.x;
	int d       = threadIdx.x;
	if (kv_head >= num_kv_heads || d >= head_dim) return;

	int logical_block  = token_pos / KV_PAGE_BLOCK_SIZE;
	int block_offset   = token_pos % KV_PAGE_BLOCK_SIZE;
	int physical_block = d_block_table[logical_block];

	size_t cache_idx = ((static_cast<size_t>(physical_block) * KV_PAGE_BLOCK_SIZE + block_offset) * num_kv_heads + kv_head) * head_dim + d;
	size_t src_idx   = static_cast<size_t>(kv_head) * head_dim + d;

	d_k_cache[cache_idx] = d_k[src_idx];
	d_v_cache[cache_idx] = d_v[src_idx];
}

__global__ void paged_gqa_decode_kernel(
	const __nv_bfloat16* __restrict__ d_q,
	const __nv_bfloat16* __restrict__ d_k_cache,
	const __nv_bfloat16* __restrict__ d_v_cache,
	const int32_t* __restrict__ d_block_table,
	__nv_bfloat16* __restrict__ d_attn_out,
	int seq_len,
	int num_q_heads,
	int num_kv_heads,
	int head_dim,
	float scale
) {
	if (seq_len <= 0) return;
	extern __shared__ float s_logits[];
	int q_head    = blockIdx.x;
	int tid       = threadIdx.x;
	int gqa_ratio = num_q_heads / num_kv_heads;
	int kv_head   = q_head / gqa_ratio;

	const __nv_bfloat16* q_vec = d_q + q_head * head_dim;

	float local_max = -1e30f;
	for (int t = tid; t < seq_len; t += blockDim.x) {
		int phys_block = d_block_table[t / KV_PAGE_BLOCK_SIZE];
		int offset     = t % KV_PAGE_BLOCK_SIZE;
		const __nv_bfloat16* k_vec = d_k_cache + ((static_cast<size_t>(phys_block) * KV_PAGE_BLOCK_SIZE + offset) * num_kv_heads + kv_head) * head_dim;

		float dot = 0.0f;
		for (int d = 0; d < head_dim; ++d) {
			dot += __bfloat162float(q_vec[d]) * __bfloat162float(k_vec[d]);
		}
		float score = dot * scale;
		s_logits[t] = score;
		if (score > local_max) local_max = score;
	}

	__shared__ float s_reduce[256];
	s_reduce[tid] = local_max;
	__syncthreads();
	for (int s = blockDim.x >> 1; s > 0; s >>= 1) {
		if (tid < s && s_reduce[tid + s] > s_reduce[tid]) {
			s_reduce[tid] = s_reduce[tid + s];
		}
		__syncthreads();
	}
	float max_score = s_reduce[0];

	float local_sum = 0.0f;
	for (int t = tid; t < seq_len; t += blockDim.x) {
		float p = expf(s_logits[t] - max_score);
		s_logits[t] = p;
		local_sum += p;
	}
	s_reduce[tid] = local_sum;
	__syncthreads();
	for (int s = blockDim.x >> 1; s > 0; s >>= 1) {
		if (tid < s) s_reduce[tid] += s_reduce[tid + s];
		__syncthreads();
	}
	float inv_sum = 1.0f / (s_reduce[0] + 1e-8f);

	for (int t = tid; t < seq_len; t += blockDim.x) {
		s_logits[t] *= inv_sum;
	}
	__syncthreads();

	for (int d = tid; d < head_dim; d += blockDim.x) {
		float acc = 0.0f;
		for (int t = 0; t < seq_len; ++t) {
			int phys_block = d_block_table[t / KV_PAGE_BLOCK_SIZE];
			int offset     = t % KV_PAGE_BLOCK_SIZE;
			const __nv_bfloat16* v_vec = d_v_cache + ((static_cast<size_t>(phys_block) * KV_PAGE_BLOCK_SIZE + offset) * num_kv_heads + kv_head) * head_dim;
			acc += s_logits[t] * __bfloat162float(v_vec[d]);
		}
		d_attn_out[q_head * head_dim + d] = __float2bfloat16(acc);
	}
}

extern "C" CUDA_API void launch_append_paged_kv(
	const void* d_k,
	const void* d_v,
	void* d_k_cache,
	void* d_v_cache,
	const int32_t* d_block_table,
	int token_pos,
	int num_kv_heads,
	int head_dim,
	void* stream
) {
	append_paged_kv_kernel<<<num_kv_heads, head_dim, 0, static_cast<cudaStream_t>(stream)>>>(
		static_cast<const __nv_bfloat16*>(d_k),
		static_cast<const __nv_bfloat16*>(d_v),
		static_cast<__nv_bfloat16*>(d_k_cache),
		static_cast<__nv_bfloat16*>(d_v_cache),
		d_block_table,
		token_pos,
		num_kv_heads,
		head_dim
	);
}

extern "C" CUDA_API void launch_paged_gqa_decode(
	const void* d_q,
	const void* d_k_cache,
	const void* d_v_cache,
	const int32_t* d_block_table,
	void* d_attn_out,
	int seq_len,
	int num_q_heads,
	int num_kv_heads,
	int head_dim,
	float scale,
	void* stream
) {
	if (seq_len <= 0) return;
	static bool smem_configured = false;
	if (!smem_configured) {
		cudaFuncSetAttribute(paged_gqa_decode_kernel, cudaFuncAttributeMaxDynamicSharedMemorySize, 98304);
		smem_configured = true;
	}
	if (seq_len > 24000) seq_len = 24000;
	size_t smem_bytes = static_cast<size_t>(seq_len) * sizeof(float);
	if (smem_bytes < sizeof(float)) smem_bytes = sizeof(float);
	paged_gqa_decode_kernel<<<num_q_heads, 128, smem_bytes, static_cast<cudaStream_t>(stream)>>>(
		static_cast<const __nv_bfloat16*>(d_q),
		static_cast<const __nv_bfloat16*>(d_k_cache),
		static_cast<const __nv_bfloat16*>(d_v_cache),
		d_block_table,
		static_cast<__nv_bfloat16*>(d_attn_out),
		seq_len,
		num_q_heads,
		num_kv_heads,
		head_dim,
		scale
	);
}

