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

#ifndef GPU_KERNEL_CUH
#define GPU_KERNEL_CUH

#include <cstddef>

struct Grid {
	int thread;
	int block;
	int index;

	inline Grid(size_t output_dim) : thread(256), index(0) {
		block = static_cast<int>((output_dim + thread - 1) / thread);
	}
};

#endif // GPU_KERNEL_CUH
