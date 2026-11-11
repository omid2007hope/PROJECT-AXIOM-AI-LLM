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

#ifndef DOWNLOAD_CUH
#define DOWNLOAD_CUH

#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API bool download_from_gpu(void* h_context,
				size_t context_elements,
				void* h_target,
				size_t target_elements,
				const GpuPortion* gpu,
				int buffer_index);

// Reads and clears the loss accumulator: *h_loss_sum receives the sum of
// 0.5 * (y - t)^2 over every training step since the previous call.
CUDA_API bool download_loss(GpuActivations* gpu_activations, float* h_loss_sum);

// Reads and clears all telemetry_slot_count floats from d_loss into h_telemetry.
CUDA_API bool download_telemetry(GpuActivations* gpu_activations, float* h_telemetry);

// Non-blocking query of the loss accumulator into pinned host memory.
CUDA_API bool poll_loss_telemetry(GpuActivations* gpu_activations, float* h_loss_sum, bool* is_ready);

// Copies the latest forward-pass output (bfloat16) to the host.
CUDA_API bool download_output(const GpuActivations* gpu_activations,
			      void* h_output,
			      size_t output_elements);

// Copies the latest GPU argmax token prediction (uint32_t) to the host.
CUDA_API bool download_pred_token(const GpuActivations* gpu_activations, uint32_t* h_pred_id);

// Copies the FP32 master weights and biases to the host.
CUDA_API bool download_ai_weights(const GpuWeights* gpu,
				  float* h_weights,
				  size_t weight_elements,
				  float* h_bias,
				  size_t bias_elements);

CUDA_API bool download_probabilities(const GpuActivations* gpu_activations,
				     float* h_probs,
				     size_t count);

CUDA_API void enqueue_loss_download(GpuPortion* gpu, int stream_idx);
CUDA_API bool poll_loss_nonblocking(GpuPortion* gpu, int stream_idx, float* out_loss);


#ifdef __cplusplus
}
#endif

#endif // DOWNLOAD_CUH
