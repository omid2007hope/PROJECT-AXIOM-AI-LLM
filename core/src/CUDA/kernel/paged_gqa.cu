#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cmath>
#include <cstdint>
#include "../util/cuda_utils.cuh"
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

__global__ static void paged_causal_attention_kernel(
	const __nv_bfloat16* __restrict__ query,
	const __nv_bfloat16* __restrict__ key_blocks,
	const __nv_bfloat16* __restrict__ value_blocks,
	const int* __restrict__ block_table,
	__nv_bfloat16* __restrict__ output,
	size_t query_length,
	size_t context_length,
	size_t block_size,
	size_t hidden_dimension,
	size_t num_heads,
	size_t head_dimension,
	float scale
) {
	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	size_t total_queries = query_length * num_heads;
	if (idx >= total_queries)
		return;

	size_t q_idx = idx / num_heads;
	size_t h = idx % num_heads;
	size_t head_offset = h * head_dimension;

	const __nv_bfloat16* q_vec = query + q_idx * hidden_dimension + head_offset;
	size_t max_attend_j = (query_length == 1) ? (context_length - 1) : q_idx;

	float max_score = -1e30f;
	float sum_exp = 0.0f;
	float accumulator[64] = {0.0f};

	for (size_t j = 0; j <= max_attend_j; ++j) {
		size_t logical_block = j / block_size;
		int physical_block = block_table[logical_block];
		if (physical_block < 0)
			continue;
		size_t token_offset = j % block_size;

		size_t block_stride = physical_block * (block_size * hidden_dimension) +
				      token_offset * hidden_dimension + head_offset;
		const __nv_bfloat16* k_vec = key_blocks + block_stride;

		float dot = 0.0f;
		for (size_t d = 0; d < head_dimension && d < 64; ++d) {
			dot += __bfloat162float(q_vec[d]) * __bfloat162float(k_vec[d]);
		}
		float score = dot * scale;

		float new_max = fmaxf(max_score, score);
		float exp_prev = expf(max_score - new_max);
		float exp_curr = expf(score - new_max);

		sum_exp = sum_exp * exp_prev + exp_curr;
		max_score = new_max;

		const __nv_bfloat16* v_vec = value_blocks + block_stride;
		for (size_t d = 0; d < head_dimension && d < 64; ++d) {
			accumulator[d] = accumulator[d] * exp_prev + exp_curr * __bfloat162float(v_vec[d]);
		}
	}

	float inv_sum = 1.0f / (sum_exp > 1e-12f ? sum_exp : 1.0f);
	__nv_bfloat16* out_vec = output + q_idx * hidden_dimension + head_offset;
	for (size_t d = 0; d < head_dimension && d < 64; ++d) {
		float val = accumulator[d] * inv_sum;
		out_vec[d] = __float2bfloat16(val);
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

extern "C" CUDA_API bool allocate_paged_cache(
	GpuPagedCache* cache,
	size_t total_blocks,
	size_t block_size,
	size_t hidden_dim,
	size_t num_heads
) {
	if (!cache || total_blocks == 0 || block_size == 0 || hidden_dim == 0 || num_heads == 0)
		return false;

	free_paged_cache(cache);

	cache->total_blocks = total_blocks;
	cache->block_size = block_size;
	cache->hidden_dimension = hidden_dim;
	cache->num_heads = num_heads;

	size_t block_elements = block_size * hidden_dim;
	size_t total_elements = total_blocks * block_elements;
	size_t buffer_bytes = total_elements * sizeof(__nv_bfloat16);

	if (!CUDA_CHECK(cudaMalloc(&cache->d_key_blocks, buffer_bytes)))
		return false;
	if (!CUDA_CHECK(cudaMalloc(&cache->d_value_blocks, buffer_bytes)))
		return false;

	size_t table_bytes = 1024 * sizeof(int);
	if (!CUDA_CHECK(cudaMalloc(&cache->d_block_table, table_bytes)))
		return false;

	cudaMemset(cache->d_key_blocks, 0, buffer_bytes);
	cudaMemset(cache->d_value_blocks, 0, buffer_bytes);
	cudaMemset(cache->d_block_table, 0xFF, table_bytes);

	return true;
}

extern "C" CUDA_API void free_paged_cache(GpuPagedCache* cache) {
	if (!cache)
		return;

	if (cache->d_key_blocks) {
		cudaFree(cache->d_key_blocks);
		cache->d_key_blocks = nullptr;
	}
	if (cache->d_value_blocks) {
		cudaFree(cache->d_value_blocks);
		cache->d_value_blocks = nullptr;
	}
	if (cache->d_block_table) {
		cudaFree(cache->d_block_table);
		cache->d_block_table = nullptr;
	}

	cache->total_blocks = 0;
	cache->block_size = 0;
	cache->hidden_dimension = 0;
	cache->num_heads = 0;
}

extern "C" CUDA_API bool copy_physical_block(
	const GpuPagedCache* cache,
	int src_block,
	int dst_block,
	size_t valid_tokens,
	void* stream
) {
	if (!cache || !cache->d_key_blocks || !cache->d_value_blocks)
		return false;
	if (src_block < 0 || dst_block < 0)
		return false;
	if (static_cast<size_t>(src_block) >= cache->total_blocks ||
	    static_cast<size_t>(dst_block) >= cache->total_blocks)
		return false;

	if (valid_tokens == 0)
		return true;
	if (valid_tokens > cache->block_size)
		valid_tokens = cache->block_size;

	cudaStream_t s = static_cast<cudaStream_t>(stream);
	size_t block_elements = cache->block_size * cache->hidden_dimension;
	size_t copy_elements = valid_tokens * cache->hidden_dimension;
	size_t copy_bytes = copy_elements * sizeof(__nv_bfloat16);

	__nv_bfloat16* k_base = static_cast<__nv_bfloat16*>(cache->d_key_blocks);
	__nv_bfloat16* v_base = static_cast<__nv_bfloat16*>(cache->d_value_blocks);

	const __nv_bfloat16* src_k = k_base + src_block * block_elements;
	__nv_bfloat16* dst_k = k_base + dst_block * block_elements;

	const __nv_bfloat16* src_v = v_base + src_block * block_elements;
	__nv_bfloat16* dst_v = v_base + dst_block * block_elements;

	cudaMemcpyAsync(dst_k, src_k, copy_bytes, cudaMemcpyDeviceToDevice, s);
	cudaMemcpyAsync(dst_v, src_v, copy_bytes, cudaMemcpyDeviceToDevice, s);

	return CUDA_CHECK(cudaGetLastError());
}

extern "C" CUDA_API bool append_paged_kv(
	const GpuPagedCache* cache,
	int physical_block,
	size_t block_offset,
	const void* key_vec,
	const void* val_vec,
	void* stream
) {
	if (!cache || !cache->d_key_blocks || !cache->d_value_blocks)
		return false;
	if (physical_block < 0 || static_cast<size_t>(physical_block) >= cache->total_blocks)
		return false;
	if (block_offset >= cache->block_size || !key_vec || !val_vec)
		return false;

	cudaStream_t s = static_cast<cudaStream_t>(stream);
	size_t block_elements = cache->block_size * cache->hidden_dimension;
	size_t element_offset = physical_block * block_elements + block_offset * cache->hidden_dimension;
	size_t copy_bytes = cache->hidden_dimension * sizeof(__nv_bfloat16);

	__nv_bfloat16* k_base = static_cast<__nv_bfloat16*>(cache->d_key_blocks);
	__nv_bfloat16* v_base = static_cast<__nv_bfloat16*>(cache->d_value_blocks);

	cudaMemcpyAsync(k_base + element_offset, key_vec, copy_bytes, cudaMemcpyDefault, s);
	cudaMemcpyAsync(v_base + element_offset, val_vec, copy_bytes, cudaMemcpyDefault, s);

	return CUDA_CHECK(cudaGetLastError());
}

extern "C" CUDA_API bool paged_causal_attention(
	const void* query,
	const GpuPagedCache* cache,
	const int* d_block_table,
	void* output,
	size_t query_length,
	size_t context_length,
	size_t hidden_dim,
	size_t num_heads,
	void* stream
) {
	if (!query || !cache || !cache->d_key_blocks || !cache->d_value_blocks ||
	    !d_block_table || !output || query_length == 0 || context_length == 0 ||
	    hidden_dim == 0 || num_heads == 0)
		return false;

	size_t head_dim = hidden_dim / num_heads;
	if (head_dim == 0 || head_dim > 64)
		return false;

	float scale = 1.0f / sqrtf(static_cast<float>(head_dim));
	size_t total_queries = query_length * num_heads;
	int threads = 128;
	int blocks = static_cast<int>((total_queries + threads - 1) / threads);
	cudaStream_t s = static_cast<cudaStream_t>(stream);

	paged_causal_attention_kernel<<<blocks, threads, 0, s>>>(
		static_cast<const __nv_bfloat16*>(query),
		static_cast<const __nv_bfloat16*>(cache->d_key_blocks),
		static_cast<const __nv_bfloat16*>(cache->d_value_blocks),
		d_block_table,
		static_cast<__nv_bfloat16*>(output),
		query_length,
		context_length,
		cache->block_size,
		hidden_dim,
		num_heads,
		head_dim,
		scale
	);

	return CUDA_CHECK(cudaGetLastError());
}
