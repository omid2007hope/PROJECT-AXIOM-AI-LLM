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

#include "../include/ai.hpp"
#include "../include/optimizer.hpp"
#include "../src/CUDA/transfer.cuh"

#include <vector>

bool AI::build_training_graph(Optimizer& optimizer, Portion& portion) {
	if (input_elements_x_total == 0 || target_elements_y_total == 0)
		return false;

	int buffer_index = portion.buffer_index;
	synchronize_stream(&gpu_portion, buffer_index);
	optimizer.load_context_window(portion);

	if (portion.has_token_ids && portion.context_token_ids[buffer_index] != nullptr &&
	    gpu_embeddings.d_table != nullptr) {
		upload_tokens_to_gpu(portion.context_token_ids[buffer_index],
				     portion.context_window_size,
				     portion.target_tokens[buffer_index],
				     portion.target_elements,
				     &gpu_portion,
				     buffer_index);
	}

	float lr = optimizer.learning_rate;
	return capture_training_graph(&gpu_graph_execution,
				      &gpu_portion,
				      &gpu_embeddings,
				      &gpu_weights,
				      &gpu_activations,
				      input_elements_x_total,
				      target_elements_y_total,
				      lr,
				      buffer_index);
}

bool AI::launch_training_graph_step() {
	if (!gpu_graph_execution.is_instantiated)
		return false;
	return launch_training_graph(&gpu_graph_execution, nullptr);
}

void AI::free_training_graph_resources() {
	free_training_graph(&gpu_graph_execution);
}

float AI::benchmark_graph_launch_latency(int iterations) {
	if (!gpu_graph_execution.is_instantiated)
		return -1.0f;
	return benchmark_graph_latency(&gpu_graph_execution, nullptr, iterations);
}

