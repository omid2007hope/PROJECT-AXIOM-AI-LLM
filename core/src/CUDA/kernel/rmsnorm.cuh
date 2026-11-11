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

#ifndef RMSNORM_CUH
#define RMSNORM_CUH

#ifdef __CUDACC__
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#endif
#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API void launch_rmsnorm_bf16(
	const void* d_in,
	const void* d_weight,
	void* d_out,
	int num_tokens,
	int hidden_dim,
	float eps,
	void* stream
);

CUDA_API void launch_residual_add_bf16(
	void* d_residual_stream,
	const void* d_proj_tmp,
	int hidden_dim,
	void* stream
);

#ifdef __cplusplus
}
#endif

#endif // RMSNORM_CUH
