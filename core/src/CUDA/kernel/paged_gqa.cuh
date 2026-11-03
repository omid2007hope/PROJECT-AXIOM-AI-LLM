#ifndef PAGED_GQA_CUH
#define PAGED_GQA_CUH

#ifdef __CUDACC__
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#endif
#include <cstdint>
#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

constexpr int KV_PAGE_BLOCK_SIZE = 16;

CUDA_API void launch_append_paged_kv(
	const void* d_k,
	const void* d_v,
	void* d_k_cache,
	void* d_v_cache,
	const int32_t* d_block_table,
	int token_pos,
	int num_kv_heads,
	int head_dim,
	void* stream
);

CUDA_API void launch_paged_gqa_decode(
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
);

#ifdef __cplusplus
}
#endif

#endif // PAGED_GQA_CUH
