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

#include <chrono>
#include <cuda_runtime.h>

#include "../util/cuda_utils.cuh"
#include "backward_pass.cuh"
#include "cuda_graph_capture.cuh"
#include "forward_pass.cuh"

extern "C" CUDA_API bool capture_training_graph(GpuGraphExecution* graph_engine,
					     const GpuPortion* gpu_portion,
					     GpuEmbeddings* gpu_embeddings,
					     GpuWeights* gpu_weights,
					     GpuActivations* gpu_activations,
					     size_t input_dim,
					     size_t output_dim,
					     float learning_rate,
					     int buffer_index) {
	if (!graph_engine || !gpu_portion || !gpu_weights || !gpu_activations ||
	    input_dim == 0 || output_dim == 0) {
		return false;
	}

	free_training_graph(graph_engine);

	cudaStream_t stream = (cudaStream_t)gpu_portion->stream[buffer_index];
	if (!stream) {
		return false;
	}
	graph_engine->stream = stream;

	// Warmup execution: ensures all internal static structures and memory pools
	// are allocated prior to stream capture, fulfilling CUDA graph capture constraints.
	forward_pass_with_embeddings(gpu_portion, gpu_embeddings, gpu_weights,
				     gpu_activations, input_dim, output_dim, buffer_index);
	backward_pass_with_embeddings(gpu_portion, gpu_embeddings, gpu_weights,
				      gpu_activations, input_dim, output_dim,
				      learning_rate, buffer_index, true);
	cudaStreamSynchronize(stream);

	// Begin stream capture mode
	if (!CUDA_CHECK(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal))) {
		return false;
	}

	// Capture forward pass
	if (!forward_pass_with_embeddings(gpu_portion, gpu_embeddings, gpu_weights,
					  gpu_activations, input_dim, output_dim, buffer_index)) {
		cudaGraph_t failed_graph = nullptr;
		cudaStreamEndCapture(stream, &failed_graph);
		if (failed_graph) cudaGraphDestroy(failed_graph);
		return false;
	}

	// Capture backward pass
	if (!backward_pass_with_embeddings(gpu_portion, gpu_embeddings, gpu_weights,
					   gpu_activations, input_dim, output_dim,
					   learning_rate, buffer_index, true)) {
		cudaGraph_t failed_graph = nullptr;
		cudaStreamEndCapture(stream, &failed_graph);
		if (failed_graph) cudaGraphDestroy(failed_graph);
		return false;
	}

	cudaGraph_t captured_graph = nullptr;
	if (!CUDA_CHECK(cudaStreamEndCapture(stream, &captured_graph))) {
		if (captured_graph) cudaGraphDestroy(captured_graph);
		return false;
	}

	cudaGraphExec_t instantiated_exec = nullptr;
	if (!CUDA_CHECK(cudaGraphInstantiate(&instantiated_exec, captured_graph, nullptr, nullptr, 0))) {
		cudaGraphDestroy(captured_graph);
		return false;
	}

	graph_engine->graph = captured_graph;
	graph_engine->graph_exec = instantiated_exec;
	graph_engine->is_instantiated = true;
	return true;
}

extern "C" CUDA_API bool launch_training_graph(GpuGraphExecution* graph_engine, void* stream) {
	if (!graph_engine || !graph_engine->is_instantiated || !graph_engine->graph_exec) {
		return false;
	}
	cudaStream_t s = stream ? (cudaStream_t)stream : (cudaStream_t)graph_engine->stream;
	return CUDA_CHECK(cudaGraphLaunch((cudaGraphExec_t)graph_engine->graph_exec, s));
}

extern "C" CUDA_API void free_training_graph(GpuGraphExecution* graph_engine) {
	if (!graph_engine)
		return;

	if (graph_engine->graph_exec) {
		cudaGraphExecDestroy((cudaGraphExec_t)graph_engine->graph_exec);
		graph_engine->graph_exec = nullptr;
	}
	if (graph_engine->graph) {
		cudaGraphDestroy((cudaGraph_t)graph_engine->graph);
		graph_engine->graph = nullptr;
	}
	graph_engine->is_instantiated = false;
	graph_engine->launch_latency_ms = 0.0f;
}

extern "C" CUDA_API float benchmark_graph_latency(GpuGraphExecution* graph_engine, void* stream, int iterations) {
	if (!graph_engine || !graph_engine->is_instantiated || !graph_engine->graph_exec || iterations <= 0) {
		return -1.0f;
	}
	cudaStream_t s = stream ? (cudaStream_t)stream : (cudaStream_t)graph_engine->stream;

	// Warmup launch
	cudaGraphLaunch((cudaGraphExec_t)graph_engine->graph_exec, s);
	cudaStreamSynchronize(s);

	// Measure host launch latency (CPU time spent dispatching graph to queue)
	auto start_time = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < iterations; ++i) {
		cudaGraphLaunch((cudaGraphExec_t)graph_engine->graph_exec, s);
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	cudaStreamSynchronize(s);

	double total_elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
	float avg_latency_ms = static_cast<float>(total_elapsed_ms / iterations);
	graph_engine->launch_latency_ms = avg_latency_ms;
	return avg_latency_ms;
}

