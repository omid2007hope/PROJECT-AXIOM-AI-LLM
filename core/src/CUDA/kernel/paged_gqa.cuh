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

#ifndef PAGED_GQA_CUH
#define PAGED_GQA_CUH

#ifdef __CUDACC__
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#endif
#include <cstddef>
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

CUDA_API bool allocate_paged_cache(
	GpuPagedCache* cache,
	size_t total_blocks,
	size_t block_size,
	size_t hidden_dim,
	size_t num_heads
);

CUDA_API void free_paged_cache(GpuPagedCache* cache);

CUDA_API bool copy_physical_block(
	const GpuPagedCache* cache,
	int src_block,
	int dst_block,
	size_t valid_tokens,
	void* stream
);

CUDA_API bool append_paged_kv(
	const GpuPagedCache* cache,
	int physical_block,
	size_t block_offset,
	const void* key_vec,
	const void* val_vec,
	void* stream
);

CUDA_API bool paged_causal_attention(
	const void* query,
	const GpuPagedCache* cache,
	const int* d_block_table,
	void* output,
	size_t query_length,
	size_t context_length,
	size_t hidden_dim,
	size_t num_heads,
	void* stream
);

#ifdef __cplusplus
}
#endif

#endif // PAGED_GQA_CUH
