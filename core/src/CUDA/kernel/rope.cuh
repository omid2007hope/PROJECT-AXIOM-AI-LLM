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

#ifndef ROPE_CUH
#define ROPE_CUH

#ifdef __CUDACC__
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#endif
#include <cstdint>
#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API void launch_rope_inplace_bf16(
	void* d_q,
	void* d_k,
	const int32_t* d_pos,
	int num_tokens,
	int num_q_heads,
	int num_kv_heads,
	int head_dim,
	float rope_theta,
	void* stream
);

#ifdef __cplusplus
}
#endif

#endif // ROPE_CUH
