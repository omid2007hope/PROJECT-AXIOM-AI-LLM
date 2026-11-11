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

#include <cuda_bf16.h>
#include <cuda_runtime.h>

#include "../util/cuda_utils.cuh"
#include "../util/memory_allocation.cuh"
#include "download.cuh"

extern "C" CUDA_API bool download_from_gpu(void* h_context, size_t context_elements, void* h_target,
										   size_t target_elements, const GpuPortion* gpu,
										   int buffer_index) {
	if (!gpu || buffer_index < 0 || buffer_index >= 2)
		return false;

	if (h_context && gpu->d_context_window[buffer_index] &&
		context_elements == gpu->context_elements) {
		if (!CUDA_CHECK(cudaMemcpyAsync(h_context, gpu->d_context_window[buffer_index],
										context_elements * sizeof(__nv_bfloat16),
										cudaMemcpyDeviceToHost,
										(cudaStream_t)gpu->stream[buffer_index])))
			return false;
	}

	if (h_target && gpu->d_target[buffer_index] && target_elements == gpu->target_elements) {
		if (!CUDA_CHECK(cudaMemcpyAsync(
				h_target, gpu->d_target[buffer_index], target_elements * sizeof(__nv_bfloat16),
				cudaMemcpyDeviceToHost, (cudaStream_t)gpu->stream[buffer_index])))
			return false;
	}

	if (gpu->stream[buffer_index]) {
		if (!CUDA_CHECK(cudaStreamSynchronize((cudaStream_t)gpu->stream[buffer_index])))
			return false;
	}

	return true;
}

// Phase 2.2: Non-Blocking Loss & Activation Telemetry via pinned slot and stream event query
extern "C" CUDA_API bool download_telemetry(GpuActivations* gpu_activations, float* h_telemetry) {
	if (!gpu_activations || !h_telemetry)
		return false;

	const size_t telemetry_bytes = telemetry_slot_count * sizeof(float);
	for (size_t i = 0; i < telemetry_slot_count; ++i) {
		// rest values
		h_telemetry[i] = 0.0f;
	}

	if (!gpu_activations->d_loss)
		return true;

	if (!gpu_activations->h_pinned_loss) {
		// allocate this many byte for telemetry
		gpu_activations->h_pinned_loss = pinned_alloc(telemetry_bytes);

		// verify allocation
		if (!gpu_activations->h_pinned_loss)
			return false;

		float* pinned_slots = static_cast<float*>(gpu_activations->h_pinned_loss);
		for (size_t i = 0; i < telemetry_slot_count; ++i) {

			// rest values
			pinned_slots[i] = 0.0f;
		}
	}

	if (gpu_activations->loss_event) {
		cudaError_t status = cudaEventQuery((cudaEvent_t)gpu_activations->loss_event);
		if (status == cudaErrorNotReady) {
			if (!CUDA_CHECK(cudaEventSynchronize((cudaEvent_t)gpu_activations->loss_event)))
				return false;
		} else if (status != cudaSuccess) {
			return false;
		}
	}

	if (!CUDA_CHECK(cudaMemcpy(gpu_activations->h_pinned_loss, gpu_activations->d_loss,
							   telemetry_bytes, cudaMemcpyDeviceToHost)))
		return false;

	const float* pinned_slots = static_cast<const float*>(gpu_activations->h_pinned_loss);

	for (size_t i = 0; i < telemetry_slot_count; ++i) {
		h_telemetry[i] = pinned_slots[i];
	}
	return CUDA_CHECK(cudaMemset(gpu_activations->d_loss, 0, telemetry_bytes));
}

extern "C" CUDA_API bool download_loss(GpuActivations* gpu_activations, float* h_loss_sum) {
	if (!gpu_activations || !h_loss_sum)
		return false;

	float slots[telemetry_slot_count] = {};

	if (!download_telemetry(gpu_activations, slots))
		return false;

	*h_loss_sum = slots[telemetry_loss];
	return true;
}

extern "C" CUDA_API bool poll_loss_telemetry(GpuActivations* gpu_activations, float* h_loss_sum,
											 bool* is_ready) {
	if (!gpu_activations || !h_loss_sum || !is_ready)
		return false;

	*is_ready = false;
	*h_loss_sum = 0.0f;

	if (!gpu_activations->d_loss) {
		*is_ready = true;
		return true;
	}

	const size_t telemetry_bytes = telemetry_slot_count * sizeof(float);
	if (!gpu_activations->h_pinned_loss) {
		gpu_activations->h_pinned_loss = pinned_alloc(telemetry_bytes);
		if (!gpu_activations->h_pinned_loss)
			return false;
		*static_cast<float*>(gpu_activations->h_pinned_loss) = 0.0f;
	}

	if (gpu_activations->loss_event) {
		cudaError_t status = cudaEventQuery((cudaEvent_t)gpu_activations->loss_event);
		if (status == cudaErrorNotReady) {
			*is_ready = false;
			return true;
		} else if (status != cudaSuccess) {
			return false;
		}
	}

	if (!CUDA_CHECK(cudaMemcpy(gpu_activations->h_pinned_loss, gpu_activations->d_loss,
							   sizeof(float), cudaMemcpyDeviceToHost)))
		return false;

	*h_loss_sum = *static_cast<float*>(gpu_activations->h_pinned_loss);
	CUDA_CHECK(cudaMemset(gpu_activations->d_loss, 0, telemetry_bytes));
	*is_ready = true;
	return true;
}

extern "C" CUDA_API bool download_output(const GpuActivations* gpu_activations, void* h_output,
										 size_t output_elements) {
	if (!gpu_activations || !h_output || !gpu_activations->d_output ||
		output_elements != gpu_activations->output_elements)
		return false;

	return CUDA_CHECK(cudaMemcpy(h_output, gpu_activations->d_output,
								 output_elements * sizeof(__nv_bfloat16), cudaMemcpyDeviceToHost));
}

extern "C" CUDA_API bool download_pred_token(const GpuActivations* gpu_activations,
											 uint32_t* h_pred_id) {
	if (!gpu_activations || !h_pred_id || !gpu_activations->d_pred_id)
		return false;

	return CUDA_CHECK(cudaMemcpy(h_pred_id, gpu_activations->d_pred_id, sizeof(uint32_t),
								 cudaMemcpyDeviceToHost));
}

extern "C" CUDA_API bool download_ai_weights(const GpuWeights* gpu, float* h_weights,
											 size_t weight_elements, float* h_bias,
											 size_t bias_elements) {
	if (!gpu || !h_weights || !h_bias || !gpu->d_weights_master || !gpu->d_bias_master ||
		weight_elements != gpu->weight_elements || bias_elements != gpu->bias_elements)
		return false;

	if (!CUDA_CHECK(cudaDeviceSynchronize()))
		return false;

	// The FP32 master copies are the true weights; the bf16 ones are just their rounding
	if (!CUDA_CHECK(cudaMemcpy(h_weights, gpu->d_weights_master, weight_elements * sizeof(float),
							   cudaMemcpyDeviceToHost)))
		return false;

	return CUDA_CHECK(cudaMemcpy(h_bias, gpu->d_bias_master, bias_elements * sizeof(float),
								 cudaMemcpyDeviceToHost));
}

extern "C" CUDA_API bool download_probabilities(const GpuActivations* gpu_activations,
												float* h_probs, size_t count) {
	if (!gpu_activations || !h_probs || !gpu_activations->d_probs)
		return false;
	if (count > gpu_activations->vocab_elements)
		return false;
	return CUDA_CHECK(cudaMemcpy(h_probs, gpu_activations->d_probs, count * sizeof(float),
								 cudaMemcpyDeviceToHost));
}

extern "C" CUDA_API void enqueue_loss_download(GpuPortion* gpu, int stream_idx) {
	if (!gpu || stream_idx < 0 || stream_idx >= 2 || !gpu->h_pinned_loss[stream_idx] ||
		!gpu->d_loss_scalar[stream_idx] || !gpu->stream[stream_idx])
		return;
	cudaMemcpyAsync(gpu->h_pinned_loss[stream_idx], gpu->d_loss_scalar[stream_idx], sizeof(float),
					cudaMemcpyDeviceToHost, static_cast<cudaStream_t>(gpu->stream[stream_idx]));
	if (gpu->loss_ready_event[stream_idx]) {
		cudaEventRecord(static_cast<cudaEvent_t>(gpu->loss_ready_event[stream_idx]),
						static_cast<cudaStream_t>(gpu->stream[stream_idx]));
	}
}

extern "C" CUDA_API bool poll_loss_nonblocking(GpuPortion* gpu, int stream_idx, float* out_loss) {
	if (!gpu || !out_loss || stream_idx < 0 || stream_idx >= 2 || !gpu->h_pinned_loss[stream_idx] ||
		!gpu->loss_ready_event[stream_idx])
		return false;
	if (cudaEventQuery(static_cast<cudaEvent_t>(gpu->loss_ready_event[stream_idx])) ==
		cudaSuccess) {
		*out_loss = *(gpu->h_pinned_loss[stream_idx]);
		return true;
	}
	return false;
}
