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

