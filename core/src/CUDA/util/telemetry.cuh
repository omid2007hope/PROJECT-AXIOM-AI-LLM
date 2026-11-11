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
