#include <algorithm>
#include <cstdio>
#include <cuda_runtime.h>
#include <mutex>
#include <vector>

#include "caching_allocator.cuh"
#include "cuda_utils.cuh"

struct MemoryBlock {
	uint8_t* address = nullptr;
	size_t size = 0;
	bool is_free = true;
	MemoryBlock* previous_block = nullptr;
	MemoryBlock* next_block = nullptr;
};

class CachingAllocator {
private:
	uint8_t* arena_base = nullptr;
	size_t arena_size = 0;
	size_t allocated_bytes = 0;
	std::vector<MemoryBlock*> block_list;
	std::mutex allocator_mutex;

	static constexpr size_t DEFAULT_ARENA_SIZE = 64 * 1024 * 1024; // 64 MB VRAM Arena
	static constexpr size_t MEMORY_ALIGNMENT = 256; // 256-byte Tensor Core alignment
	static constexpr size_t MIN_SPLIT_SIZE = 256;

	bool initialize_arena(size_t request_size) {
		if (arena_base != nullptr) {
			return true;
		}

		size_t target_size =
			(request_size > DEFAULT_ARENA_SIZE) ? request_size : DEFAULT_ARENA_SIZE;
		cudaError_t err = cudaMalloc(&arena_base, target_size);
		if (err != cudaSuccess) {
			// Fallback to smaller 16 MB arena if GPU VRAM is heavily constrained
			target_size = 16 * 1024 * 1024;
			err = cudaMalloc(&arena_base, target_size);
			if (err != cudaSuccess) {
				arena_base = nullptr;
				arena_size = 0;
				return false;
			}
		}

		arena_size = target_size;
		MemoryBlock* root = new MemoryBlock();
		root->address = arena_base;
		root->size = arena_size;
		root->is_free = true;
		root->previous_block = nullptr;
		root->next_block = nullptr;
		block_list.push_back(root);
		return true;
	}

public:
	CachingAllocator() = default;

	~CachingAllocator() {
		purge();
	}

	void* allocate(size_t bytes, cudaStream_t stream) {
		if (bytes == 0) {
			return nullptr;
		}

		size_t aligned_bytes = (bytes + MEMORY_ALIGNMENT - 1) & ~(MEMORY_ALIGNMENT - 1);
		std::lock_guard<std::mutex> lock(allocator_mutex);

		if (arena_base == nullptr) {
			initialize_arena(aligned_bytes);
		}

		if (arena_base != nullptr) {
			MemoryBlock* best_fit = nullptr;
			for (MemoryBlock* block : block_list) {
				if (block->is_free && block->size >= aligned_bytes) {
					if (!best_fit || block->size < best_fit->size) {
						best_fit = block;
						if (best_fit->size == aligned_bytes) {
							break;
						}
					}
				}
			}

			if (best_fit != nullptr) {
				if (best_fit->size >= aligned_bytes + MIN_SPLIT_SIZE) {
					MemoryBlock* remaining_block = new MemoryBlock();
					remaining_block->address =
						best_fit->address + aligned_bytes;
					remaining_block->size = best_fit->size - aligned_bytes;
					remaining_block->is_free = true;
					remaining_block->previous_block = best_fit;
					remaining_block->next_block = best_fit->next_block;

					if (best_fit->next_block != nullptr) {
						best_fit->next_block->previous_block =
							remaining_block;
					}
					best_fit->next_block = remaining_block;
					best_fit->size = aligned_bytes;
					block_list.push_back(remaining_block);
				}

				best_fit->is_free = false;
				allocated_bytes += best_fit->size;
				return best_fit->address;
			}
		}

		// Fallback to stream-ordered asynchronous memory pool
		void* async_ptr = nullptr;
		cudaError_t err = cudaMallocAsync(&async_ptr, aligned_bytes, stream);
		if (err == cudaSuccess) {
			return async_ptr;
		}

		return nullptr;
	}

	void deallocate(void* ptr, cudaStream_t stream) {
		if (ptr == nullptr) {
			return;
		}

		std::lock_guard<std::mutex> lock(allocator_mutex);

		uint8_t* byte_ptr = static_cast<uint8_t*>(ptr);
		if (arena_base != nullptr && byte_ptr >= arena_base &&
		    byte_ptr < arena_base + arena_size) {
			for (MemoryBlock* block : block_list) {
				if (block->address == byte_ptr) {
					if (block->is_free) {
						return; // already freed
					}
					block->is_free = true;
					allocated_bytes -= block->size;

					// Coalesce with next block if free
					if (block->next_block != nullptr &&
					    block->next_block->is_free) {
						MemoryBlock* next = block->next_block;
						block->size += next->size;
						block->next_block = next->next_block;
						if (next->next_block != nullptr) {
							next->next_block->previous_block = block;
						}
						auto it = std::find(
							block_list.begin(), block_list.end(), next);
						if (it != block_list.end()) {
							block_list.erase(it);
						}
						delete next;
					}

					// Coalesce with previous block if free
					if (block->previous_block != nullptr &&
					    block->previous_block->is_free) {
						MemoryBlock* prev = block->previous_block;
						prev->size += block->size;
						prev->next_block = block->next_block;
						if (block->next_block != nullptr) {
							block->next_block->previous_block = prev;
						}
						auto it = std::find(block_list.begin(),
								    block_list.end(),
								    block);
						if (it != block_list.end()) {
							block_list.erase(it);
						}
						delete block;
					}

					return;
				}
			}
			return;
		}

		// Fallback deallocation through stream-ordered memory pool
		cudaError_t free_err = cudaFreeAsync(ptr, stream);
		if (free_err != cudaSuccess) {
			cudaGetLastError(); // Clear sticky error if pointer was not from stream pool
		}
	}

	void purge() {
		std::lock_guard<std::mutex> lock(allocator_mutex);
		for (MemoryBlock* block : block_list) {
			delete block;
		}
		block_list.clear();

		if (arena_base != nullptr) {
			cudaFree(arena_base);
			arena_base = nullptr;
		}
		arena_size = 0;
		allocated_bytes = 0;
	}

	bool is_allocated(void* ptr) {
		std::lock_guard<std::mutex> lock(allocator_mutex);
		uint8_t* byte_ptr = static_cast<uint8_t*>(ptr);
		if (arena_base != nullptr && byte_ptr >= arena_base &&
		    byte_ptr < arena_base + arena_size) {
			for (MemoryBlock* block : block_list) {
				if (block->address == byte_ptr) {
					return !block->is_free;
				}
			}
		}
		return false;
	}

	size_t get_arena_size() {
		std::lock_guard<std::mutex> lock(allocator_mutex);
		return arena_size;
	}

	size_t get_allocated_vram() {
		std::lock_guard<std::mutex> lock(allocator_mutex);
		return allocated_bytes;
	}
};

static CachingAllocator global_caching_allocator;

extern "C" CUDA_API void* allocate_vram(size_t bytes, void* stream) {
	return global_caching_allocator.allocate(bytes, static_cast<cudaStream_t>(stream));
}

extern "C" CUDA_API void free_vram(void* ptr, void* stream) {
	global_caching_allocator.deallocate(ptr, static_cast<cudaStream_t>(stream));
}

extern "C" CUDA_API void purge_vram_cache() {
	global_caching_allocator.purge();
}

extern "C" CUDA_API bool is_vram_allocated(void* ptr) {
	return global_caching_allocator.is_allocated(ptr);
}

extern "C" CUDA_API size_t get_arena_size() {
	return global_caching_allocator.get_arena_size();
}

extern "C" CUDA_API size_t get_allocated_vram() {
	return global_caching_allocator.get_allocated_vram();
}
