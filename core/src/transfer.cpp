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

#include "../include/transfer.hpp"

#include "../src/CUDA/transfer.cuh"


// **********
// Upload
// **********

bool Transfer::upload_weights(const std::vector<float>& host_weights,
			      const std::vector<float>& host_bias,
			      size_t& pending_steps,
			      GpuWeights& gpu_weights,
			      GpuActivations& gpu_activations) {
	// Pass the data down to the CUDA C-interface
	if (!upload_ai_weights(host_weights.data(),
			       host_weights.size(),
			       host_bias.data(),
			       host_bias.size(),
			       &gpu_weights)) {
		return false;
	}

	// Whatever loss was measured against the previous weights no longer describes
	// this network, so start a fresh measurement.
	float stale_loss = 0.0f;
	download_loss(&gpu_activations, &stale_loss);
	pending_steps = 0;

	return true;
}

bool Transfer::upload_embeddings(const std::vector<float>& host_embeddings,
				 GpuEmbeddings& gpu_embeddings) {
	return ::upload_embeddings(host_embeddings.data(), &gpu_embeddings);
}

// **********
// Download
// **********

bool Transfer::download_weights(std::vector<float>& host_weights,
				std::vector<float>& host_bias,
				const GpuWeights& gpu_weights) {
	return download_ai_weights(&gpu_weights,
				   host_weights.data(),
				   host_weights.size(),
				   host_bias.data(),
				   host_bias.size());
}

bool Transfer::download_embeddings(std::vector<float>& host_embeddings,
				   const GpuEmbeddings& gpu_embeddings) {
	return ::download_embeddings(host_embeddings.data(), &gpu_embeddings);
}