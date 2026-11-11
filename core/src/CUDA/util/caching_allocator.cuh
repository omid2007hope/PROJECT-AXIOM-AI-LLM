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
