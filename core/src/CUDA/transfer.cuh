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
