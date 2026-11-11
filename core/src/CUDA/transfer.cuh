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

#ifndef CUDA_TRANSFER_CUH
#define CUDA_TRANSFER_CUH

// This file is now an umbrella header that includes all the modularized CUDA APIs.
// CPU-side code can still simply #include "transfer.cuh" and gain access to all structs and functions.

#include "gpu/download.cuh"
#include "gpu/upload.cuh"
#include "kernel/backward_pass.cuh"
#include "kernel/cuda_graph_capture.cuh"
#include "kernel/forward_pass.cuh"
#include "kernel/moe_router.cuh"
#include "kernel/paged_gqa.cuh"
#include "kernel/rmsnorm.cuh"
#include "kernel/rope.cuh"
#include "kernel/swiglu.cuh"
#include "util/caching_allocator.cuh"
#include "util/gpu_types.cuh"
#include "util/memory_allocation.cuh"
#include "util/telemetry.cuh"

#endif // CUDA_TRANSFER_CUH
