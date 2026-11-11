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

#ifndef CUDA_TELEMETRY_CUH
#define CUDA_TELEMETRY_CUH

#include "gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

// Fetches GPU VRAM usage.
CUDA_API void get_gpu_telemetry(size_t* free_mem, size_t* total_mem);

// Fetches detailed GPU hardware specifications and theoretical capabilities.
CUDA_API void get_hardware_specs(char* name_buf, int name_buf_len,
                                 int* sm_major, int* sm_minor,
                                 int* bus_width_bits, int* mem_clock_khz,
                                 double* peak_bandwidth_gb_s, double* peak_dense_tflops);

// Hardware-level CUDA Event Profiling API
CUDA_API void* create_cuda_event();
CUDA_API void destroy_cuda_event(void* event);
CUDA_API void record_cuda_event(void* event, void* stream);
CUDA_API void sync_cuda_event(void* event);
CUDA_API float elapsed_cuda_events_ms(void* start_event, void* stop_event);

#ifdef __cplusplus
}
#endif

#endif // CUDA_TELEMETRY_CUH
