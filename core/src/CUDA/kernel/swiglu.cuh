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

#ifndef SWIGLU_CUH
#define SWIGLU_CUH

#ifdef __CUDACC__
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#endif
#include <cstddef>
#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API void launch_swiglu_bf16(
	const void* d_gate,
	const void* d_up,
	void* d_out,
	size_t total_elements,
	void* stream
);

CUDA_API bool swiglu_forward(
	const void* gate,
	const void* up,
	void* output,
	size_t total_elements,
	void* stream
);

CUDA_API bool swiglu_backward(
	const void* grad_output,
	const void* gate,
	const void* up,
	void* grad_gate,
	void* grad_up,
	size_t total_elements,
	void* stream
);

#ifdef __cplusplus
}
#endif

#endif // SWIGLU_CUH
