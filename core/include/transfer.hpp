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