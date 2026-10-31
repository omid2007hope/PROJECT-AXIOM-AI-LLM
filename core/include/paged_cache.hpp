#ifndef PAGED_CACHE_HPP
#define PAGED_CACHE_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

// Logical sequence state mapping to physical VRAM blocks
struct SequenceContext {
	int sequence_id = 0;
	size_t sequence_length = 0;
	std::vector<int> block_table; // Maps logical block index to physical block ID
};

// Manages physical block allocation, reference counting, and Copy-On-Write
class PagedBlockManager {
public:
	explicit PagedBlockManager(size_t total_blocks = 256, size_t block_size = 16)
		: total_blocks_count(total_blocks),
		  block_token_capacity(block_size),
		  reference_counts(total_blocks, 0)
	{
		free_block_indices.reserve(total_blocks);
		for (int i = static_cast<int>(total_blocks) - 1; i >= 0; --i) {
			free_block_indices.push_back(i);
		}
	}

	int allocate_physical_block() {
		if (free_block_indices.empty()) {
			return -1; // Out of physical blocks
		}
		int block_id = free_block_indices.back();
		free_block_indices.pop_back();
		reference_counts[block_id] = 1;
		return block_id;
	}

	void retain_physical_block(int block_id) {
		if (block_id >= 0 && static_cast<size_t>(block_id) < total_blocks_count) {
			reference_counts[block_id]++;
		}
	}

	void release_physical_block(int block_id) {
		if (block_id >= 0 && static_cast<size_t>(block_id) < total_blocks_count) {
			if (reference_counts[block_id] > 0) {
				reference_counts[block_id]--;
				if (reference_counts[block_id] == 0) {
					free_block_indices.push_back(block_id);
				}
			}
		}
	}

	int get_reference_count(int block_id) const {
		if (block_id >= 0 && static_cast<size_t>(block_id) < total_blocks_count) {
			return reference_counts[block_id];
		}
		return 0;
	}

	size_t get_free_count() const {
		return free_block_indices.size();
	}

	size_t get_used_count() const {
		return total_blocks_count - free_block_indices.size();
	}

	size_t get_total_blocks() const {
		return total_blocks_count;
	}

	size_t get_block_size() const {
		return block_token_capacity;
	}

	void reset() {
		free_block_indices.clear();
		for (int i = static_cast<int>(total_blocks_count) - 1; i >= 0; --i) {
			free_block_indices.push_back(i);
		}
		std::fill(reference_counts.begin(), reference_counts.end(), 0);
	}

private:
	size_t total_blocks_count;
	size_t block_token_capacity;
	std::vector<int> free_block_indices;
	std::vector<int> reference_counts;
};

#endif // PAGED_CACHE_HPP

