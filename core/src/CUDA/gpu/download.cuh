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

CUDA_API bool download_transformer_weights(const GpuWeights* gpu,
					   float* h_q,
					   float* h_k,
					   float* h_v,
					   float* h_o,
					   float* h_gate,
					   float* h_up,
					   float* h_down,
					   size_t dim,
					   size_t ffn);

CUDA_API void enqueue_loss_download(GpuPortion* gpu, int stream_idx);
CUDA_API bool poll_loss_nonblocking(GpuPortion* gpu, int stream_idx, float* out_loss);


#ifdef __cplusplus
}
#endif

#endif // DOWNLOAD_CUH
