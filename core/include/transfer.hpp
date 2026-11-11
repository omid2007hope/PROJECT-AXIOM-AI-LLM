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

#ifndef include_transfer
#define include_transfer

#include <cstddef>
#include <vector>

#include "../src/CUDA/transfer.cuh"
#include "config.hpp"

class Transfer {
public:
	// Method to move memory to GPU
	static bool upload_weights(const std::vector<float>& host_weights,
				   const std::vector<float>& host_bias,
				   size_t& pending_steps,
				   GpuWeights& gpu_weights,
				   GpuActivations& gpu_activations);

	// Method to move embeddings to GPU
	static bool upload_embeddings(const std::vector<float>& host_embeddings,
				      GpuEmbeddings& gpu_embeddings);

	// Method to copy the trained weights back into host_weights / host_bias
	static bool download_weights(std::vector<float>& host_weights,
				     std::vector<float>& host_bias,
				     const GpuWeights& gpu_weights);

	// Method to copy trained embeddings back into host_embeddings
	static bool download_embeddings(std::vector<float>& host_embeddings,
					const GpuEmbeddings& gpu_embeddings);
};

#endif