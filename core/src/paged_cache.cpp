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

#include "../include/ai.hpp"
#include "../src/CUDA/kernel/paged_gqa.cuh"
#include "../src/CUDA/transfer.cuh"
#include "../src/CUDA/util/memory_allocation.cuh"

#include <memory>
#include <vector>

bool AI::build_paged_cache(size_t total_blocks, size_t block_size) {
	free_paged_cache_memory();
	size_t hidden_dim = embedding_dim;
	if (hidden_dim == 0)
		hidden_dim = 32;

	bool ok = allocate_paged_cache(
		&gpu_paged_cache, total_blocks, block_size, hidden_dim, num_heads);
	if (!ok)
		return false;

	block_manager = std::make_unique<PagedBlockManager>(total_blocks, block_size);
	return true;
}

void AI::free_paged_cache_memory() {
	free_paged_cache(&gpu_paged_cache);
	block_manager.reset();
}

bool AI::fork_sequence_context(const SequenceContext& parent, SequenceContext& child) {
	if (!block_manager)
		return false;
	child.sequence_id = parent.sequence_id + 1;
	child.sequence_length = parent.sequence_length;
	child.block_table = parent.block_table;
	for (int block_id : child.block_table) {
		block_manager->retain_physical_block(block_id);
	}
	return true;
}

bool AI::append_sequence_token(SequenceContext& sequence,
			       const std::vector<std::bfloat16_t>& key,
			       const std::vector<std::bfloat16_t>& value) {
	if (!block_manager || !gpu_paged_cache.d_key_blocks)
		return false;
	if (key.size() != gpu_paged_cache.hidden_dimension ||
	    value.size() != gpu_paged_cache.hidden_dimension)
		return false;

	size_t block_size = block_manager->get_block_size();
	size_t current_token_index = sequence.sequence_length;
	size_t offset = current_token_index % block_size;

	int target_block = -1;
	if (offset == 0) {
		target_block = block_manager->allocate_physical_block();
		if (target_block < 0)
			return false;
		sequence.block_table.push_back(target_block);
	} else {
		if (sequence.block_table.empty())
			return false;
		target_block = sequence.block_table.back();
		if (block_manager->get_reference_count(target_block) > 1) {
			int new_block = block_manager->allocate_physical_block();
			if (new_block < 0)
				return false;
			copy_physical_block(
				&gpu_paged_cache, target_block, new_block, offset, nullptr);
			block_manager->release_physical_block(target_block);
			sequence.block_table.back() = new_block;
			target_block = new_block;
		}
	}

	append_paged_kv(&gpu_paged_cache, target_block, offset, key.data(), value.data(), nullptr);
	sequence.sequence_length++;
	return true;
}

void AI::release_sequence_context(SequenceContext& sequence) {
	if (!block_manager)
		return;
	for (int block_id : sequence.block_table) {
		block_manager->release_physical_block(block_id);
	}
	sequence.block_table.clear();
	sequence.sequence_length = 0;
}

bool AI::forward_paged_attention(const SequenceContext& sequence,
				 const std::vector<std::bfloat16_t>& query,
				 std::vector<std::bfloat16_t>& output,
				 size_t query_length) {
	if (!gpu_paged_cache.d_key_blocks || sequence.block_table.empty())
		return false;
	if (query_length == 0 || sequence.sequence_length == 0)
		return false;
	if (query.size() != query_length * gpu_paged_cache.hidden_dimension)
		return false;

	size_t table_bytes = sequence.block_table.size() * sizeof(int);
	if (!upload_device_buffer(
		    gpu_paged_cache.d_block_table, sequence.block_table.data(), table_bytes))
		return false;

	size_t query_bytes = query.size() * sizeof(std::bfloat16_t);
	void* d_q = gpu_activations.d_query ? gpu_activations.d_query
					    : allocate_device_memory(query_bytes);
	void* d_out = gpu_activations.d_attention_output ? gpu_activations.d_attention_output
							 : allocate_device_memory(query_bytes);

	if (!d_q || !d_out) {
		if (d_q && d_q != gpu_activations.d_query)
			free_device_memory(d_q);
		if (d_out && d_out != gpu_activations.d_attention_output)
			free_device_memory(d_out);
		return false;
	}

	upload_device_buffer(d_q, query.data(), query_bytes);

	bool ok = paged_causal_attention(d_q,
					 &gpu_paged_cache,
					 static_cast<const int*>(gpu_paged_cache.d_block_table),
					 d_out,
					 query_length,
					 sequence.sequence_length,
					 gpu_paged_cache.hidden_dimension,
					 gpu_paged_cache.num_heads,
					 nullptr);

	if (ok) {
		output.resize(query_length * gpu_paged_cache.hidden_dimension);
		download_device_buffer(output.data(), d_out, query_bytes);
	}

	if (d_q != gpu_activations.d_query)
		free_device_memory(d_q);
	if (d_out != gpu_activations.d_attention_output)
		free_device_memory(d_out);

	return ok;
}

