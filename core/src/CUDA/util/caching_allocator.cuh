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

#ifndef CACHING_ALLOCATOR_CUH
#define CACHING_ALLOCATOR_CUH

#include <cstddef>
#include <cstdint>

#include "gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

// Allocates memory using the userspace Best-Fit with Coalescing (BFC) Caching Allocator
// or stream-ordered async memory pools without triggering device-wide synchronization barriers.
CUDA_API void* allocate_vram(size_t bytes, void* stream);

// Deallocates memory previously allocated by allocate_vram, immediately coalescing
// adjacent free blocks in userspace.
CUDA_API void free_vram(void* ptr, void* stream);

// Releases the entire pre-allocated arena back to the CUDA driver on shutdown or reset.
CUDA_API void purge_vram_cache();

// Query allocation state and telemetry
CUDA_API bool is_vram_allocated(void* ptr);
CUDA_API size_t get_arena_size();
CUDA_API size_t get_allocated_vram();

#ifdef __cplusplus
}
#endif

#endif // CACHING_ALLOCATOR_CUH
