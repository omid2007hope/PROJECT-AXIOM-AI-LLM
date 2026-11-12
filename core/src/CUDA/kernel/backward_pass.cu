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

#include "../util/caching_allocator.cuh"
#include "../util/cuda_utils.cuh"
#include "backward_pass.cuh"
#include "forward_pass.cuh"
#include "swiglu.cuh"

// =============================================================================
// =============================================================================
// Milestone 3: Discrete Autoregressive Token Classification Backward Pass
//
// 1. Cross-Entropy Loss: L = -log(P_{target})
// 2. Exact bounded logit gradient: dL/dz_i = P_i - 1[i == target]  in [-1.0, +1.0]
// 3. RMSNorm backpropagation scaling: dx_j = inv_rms * (dL/d_x_hat_j)
// =============================================================================

__global__ static void compute_loss_and_dz_kernel(
	const float* __restrict__ probs, const __nv_bfloat16* __restrict__ y,
	const __nv_bfloat16* __restrict__ target, const __nv_bfloat16* __restrict__ emb_table,
	float* __restrict__ dz,
	float* __restrict__ loss, // telemetry block, see TelemetrySlot
	size_t out_dim, size_t vocab_size, size_t emb_dim, bool is_token_training,
	float inv_temp = 1.0f) {

	size_t row = blockIdx.x * (size_t)blockDim.x + threadIdx.x;

	// Output activation health: necrosis (|y| ~ 0), saturation (y^2) and NaN/Inf
	if (row < out_dim) {
		float activation = __bfloat162float(y[row]);
		if (!isfinite(activation)) {
			atomicAdd(&loss[telemetry_nonfinite], 1.0f);
		} else {
			if (fabsf(activation) < 1e-6f) {
				atomicAdd(&loss[telemetry_dead_output], 1.0f);
			}
			atomicAdd(&loss[telemetry_activation_square], activation * activation);
		}
	}

	if (is_token_training && probs != nullptr) {
		uint32_t target_tok_id = *reinterpret_cast<const uint32_t*>(target);
		if (target_tok_id >= vocab_size) {
			if (row == 0) {
				atomicAdd(&loss[telemetry_nonfinite], 1.0f);
			}
			if (row < out_dim) {
				dz[row] = 0.0f;
			}
			return;
		}

		if (row == 0) {
			float p_target = probs[target_tok_id];
			float ce_loss = -logf(fmaxf(p_target, 1e-35f));
			if (isfinite(ce_loss)) {
				atomicAdd(&loss[telemetry_loss], ce_loss);
			} else {
				atomicAdd(&loss[telemetry_nonfinite], 1.0f);
			}
		}

		if (row < out_dim) {
			float dz_val = 0.0f;
			if (emb_table != nullptr && emb_dim == out_dim) {
				float expected_embedding = 0.0f;
				for (size_t v = 0; v < vocab_size; ++v) {
					float prob_v = probs[v];
					expected_embedding +=
						prob_v * __bfloat162float(emb_table[v * emb_dim + row]);
				}
				float target_embedding =
					__bfloat162float(emb_table[target_tok_id * emb_dim + row]);
				float inverse_scale =
					1.0f / sqrtf(static_cast<float>(emb_dim > 0 ? emb_dim : 1));
				dz_val = inv_temp * (expected_embedding - target_embedding) * inverse_scale;
			} else {
				if (row < vocab_size) {
					dz_val = probs[row] - (row == target_tok_id ? 1.0f : 0.0f);
				}
			}

			if (isfinite(dz_val)) {
				atomicAdd(&loss[telemetry_gradient_square], dz_val * dz_val);
			} else {
				atomicAdd(&loss[telemetry_nonfinite], 1.0f);
				dz_val = 0.0f;
			}
			dz_val = fmaxf(-10.0f, fminf(10.0f, dz_val));
			dz[row] = dz_val;
		}
	} else {
		if (row < out_dim) {
			float y_val = __bfloat162float(y[row]);
			float t_val = __bfloat162float(target[row]);
			float dy = y_val - t_val;
			if (isfinite(dy)) {
				atomicAdd(&loss[telemetry_gradient_square], dy * dy);
			} else {
				atomicAdd(&loss[telemetry_nonfinite], 1.0f);
				dy = 0.0f;
			}
			float dz_val = fmaxf(-1.0f, fminf(1.0f, dy));
			dz[row] = dz_val;

			if (row == 0) {
				float mse_loss = 0.0f;
				for (size_t r = 0; r < out_dim; ++r) {
					float y_r = __bfloat162float(y[r]);
					float t_r = __bfloat162float(target[r]);
					float d = y_r - t_r;
					if (isfinite(d)) {
						mse_loss += 0.5f * d * d;
					}
				}
				atomicAdd(&loss[telemetry_loss], mse_loss);
			}
		}
	}
}

__global__ static void compute_cross_entropy_and_dz_kernel(
	__nv_bfloat16* __restrict__ logits_and_grad,
	const uint32_t* __restrict__ token_ids,
	const __nv_bfloat16* __restrict__ target,
	float* __restrict__ loss,
	size_t S,
	size_t vocab_size,
	float inv_temp) {

	size_t s = blockIdx.x;
	if (s >= S)
		return;

	uint32_t target_tok_id = (s + 1 < S && token_ids != nullptr)
								 ? token_ids[s + 1]
								 : (target ? *reinterpret_cast<const uint32_t*>(target) : 0);
	if (target_tok_id >= vocab_size)
		target_tok_id = 0;

	size_t row_offset = s * vocab_size;
	__nv_bfloat16* row = logits_and_grad + row_offset;

	__shared__ float s_max[32];
	__shared__ float s_sum[32];

	float local_max = -1e30f;
	for (size_t v = threadIdx.x; v < vocab_size; v += blockDim.x) {
		float val = __bfloat162float(row[v]);
		if (isfinite(val) && val > local_max) {
			local_max = val;
		}
	}

	for (int offset = 16; offset > 0; offset /= 2) {
		local_max = fmaxf(local_max, __shfl_down_sync(0xffffffff, local_max, offset));
	}

	int lane = threadIdx.x % 32;
	int wid = threadIdx.x / 32;
	if (lane == 0) {
		s_max[wid] = local_max;
	}
	__syncthreads();

	if (wid == 0) {
		float warp_max = (lane < (blockDim.x / 32)) ? s_max[lane] : -1e30f;
		for (int offset = 16; offset > 0; offset /= 2) {
			warp_max = fmaxf(warp_max, __shfl_down_sync(0xffffffff, warp_max, offset));
		}
		if (lane == 0) {
			s_max[0] = warp_max;
		}
	}
	__syncthreads();

	float global_max = s_max[0];

	float local_sum = 0.0f;
	for (size_t v = threadIdx.x; v < vocab_size; v += blockDim.x) {
		float val = __bfloat162float(row[v]);
		float exp_val = expf(fmaxf(-50.0f, fminf(50.0f, (val - global_max) * inv_temp)));
		local_sum += exp_val;
	}

	for (int offset = 16; offset > 0; offset /= 2) {
		local_sum += __shfl_down_sync(0xffffffff, local_sum, offset);
	}
	if (lane == 0) {
		s_sum[wid] = local_sum;
	}
	__syncthreads();

	if (wid == 0) {
		float warp_sum = (lane < (blockDim.x / 32)) ? s_sum[lane] : 0.0f;
		for (int offset = 16; offset > 0; offset /= 2) {
			warp_sum += __shfl_down_sync(0xffffffff, warp_sum, offset);
		}
		if (lane == 0) {
			s_sum[0] = warp_sum > 1e-12f ? warp_sum : 1.0f;
		}
	}
	__syncthreads();

	float inv_sum = 1.0f / s_sum[0];
	float seq_scale = 1.0f / static_cast<float>(S > 0 ? S : 1);

	for (size_t v = threadIdx.x; v < vocab_size; v += blockDim.x) {
		float val = __bfloat162float(row[v]);
		float exp_val = expf(fmaxf(-50.0f, fminf(50.0f, (val - global_max) * inv_temp)));
		float prob = exp_val * inv_sum;

		if (v == target_tok_id) {
			float ce_loss = -logf(fmaxf(prob, 1e-35f));
			if (isfinite(ce_loss)) {
				atomicAdd(&loss[telemetry_loss], ce_loss * seq_scale);
			} else {
				atomicAdd(&loss[telemetry_nonfinite], 1.0f);
			}
		}

		float target_indicator = (v == target_tok_id) ? 1.0f : 0.0f;
		float dz = seq_scale * inv_temp * (prob - target_indicator);
		dz = fmaxf(-10.0f, fminf(10.0f, dz));
		row[v] = __float2bfloat16(dz);
		local_sum += dz * dz;
	}

	for (int offset = 16; offset > 0; offset /= 2) {
		local_sum += __shfl_down_sync(0xffffffff, local_sum, offset);
	}
	if (lane == 0) {
		s_sum[wid] = local_sum;
	}
	__syncthreads();

	if (wid == 0) {
		float warp_sq = (lane < (blockDim.x / 32)) ? s_sum[lane] : 0.0f;
		for (int offset = 16; offset > 0; offset /= 2) {
			warp_sq += __shfl_down_sync(0xffffffff, warp_sq, offset);
		}
		if (lane == 0 && loss) {
			atomicAdd(&loss[telemetry_gradient_square], warp_sq);
		}
	}
}

__global__ static void scale_dh_to_dx_kernel(
	const __nv_bfloat16* __restrict__ dh,
	float* __restrict__ dx,
	size_t total_elements,
	float scale = 1.0f) {

	size_t i = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (i < total_elements) {
		float val = __bfloat162float(dh[i]) * scale;
		val = isfinite(val) ? fmaxf(-10.0f, fminf(10.0f, val)) : 0.0f;
		dx[i] = val;
	}
}

__global__ static void backward_pass_kernel(
	float* __restrict__ W_master, __nv_bfloat16* __restrict__ W, float* __restrict__ b_master,
	__nv_bfloat16* __restrict__ b, const __nv_bfloat16* __restrict__ x,
	const float* __restrict__ dz_vector, size_t in_dim, size_t out_dim, float learning_rate) {

	size_t i = blockIdx.x * (size_t)blockDim.x + threadIdx.x;

	if (i < out_dim * in_dim) {
		size_t row = i / in_dim;
		size_t col = i % in_dim;

		float dz = dz_vector[row];

		float x_val = __bfloat162float(x[col]);
		if (isnan(x_val) || isinf(x_val)) {
			x_val = 0.0f;
		}

		float weight_lr = learning_rate;

		float w_val = W_master[i] - weight_lr * dz * x_val;
		if (isfinite(w_val)) {
			w_val = fmaxf(-10.0f, fminf(10.0f, w_val));
			W_master[i] = w_val;
			W[i] = __float2bfloat16(w_val);
		}

		if (col == 0) {
			float b_val = b_master[row] - learning_rate * dz;
			if (isfinite(b_val)) {
				b_val = fmaxf(-10.0f, fminf(10.0f, b_val));
				b_master[row] = b_val;
				b[row] = __float2bfloat16(b_val);
			}
		}
	}
}

// Phase 6.2: Fused AdamW Kernel in CUDA
__global__ static void backward_pass_adamw_kernel(
	float* __restrict__ W_master, __nv_bfloat16* __restrict__ W, float* __restrict__ W_m,
	float* __restrict__ W_v, float* __restrict__ b_master, __nv_bfloat16* __restrict__ b,
	float* __restrict__ b_m, float* __restrict__ b_v, const __nv_bfloat16* __restrict__ x,
	const float* __restrict__ dz_vector, size_t in_dim, size_t out_dim, float learning_rate,
	uint64_t step, float beta1, float beta2, float eps, float weight_decay) {

	size_t i = blockIdx.x * (size_t)blockDim.x + threadIdx.x;

	if (i < out_dim * in_dim) {
		size_t row = i / in_dim;
		size_t col = i % in_dim;

		float dz = dz_vector[row];

		float x_val = __bfloat162float(x[col]);
		if (isnan(x_val) || isinf(x_val)) {
			x_val = 0.0f;
		}

		float in_dim_f = static_cast<float>(in_dim > 0 ? in_dim : 1);
		float weight_lr = learning_rate / sqrtf(in_dim_f);

		if (W_m != nullptr && W_v != nullptr) {
			float weight_grad = dz * x_val;
			if (!isfinite(weight_grad))
				weight_grad = 0.0f;
			weight_grad = fmaxf(-1.0f, fminf(1.0f, weight_grad));

			float mom = beta1 * W_m[i] + (1.0f - beta1) * weight_grad;
			float var = beta2 * W_v[i] + (1.0f - beta2) * weight_grad * weight_grad;
			W_m[i] = mom;
			W_v[i] = var;

			float step_f = static_cast<float>(step);
			float bc1 = 1.0f - powf(beta1, step_f);
			float bc2 = 1.0f - powf(beta2, step_f);
			float mom_hat = mom / (bc1 > 1e-7f ? bc1 : 1.0f);
			float var_hat = var / (bc2 > 1e-7f ? bc2 : 1.0f);

			float update = mom_hat / (sqrtf(var_hat) + eps) + weight_decay * W_master[i];
			float w_val = W_master[i] - weight_lr * update;

			if (isfinite(w_val)) {
				w_val = fmaxf(-10.0f, fminf(10.0f, w_val));
				W_master[i] = w_val;
				W[i] = __float2bfloat16(w_val);
			}
		} else {
			float update = dz * x_val + weight_decay * W_master[i];
			float w_val = W_master[i] - weight_lr * update;

			if (isfinite(w_val)) {
				w_val = fmaxf(-10.0f, fminf(10.0f, w_val));
				W_master[i] = w_val;
				W[i] = __float2bfloat16(w_val);
			}
		}

		if (col == 0) {
			float eff_beta1 = beta1;
			float step_f = static_cast<float>(step);
			float bc1 = 1.0f - powf(eff_beta1, step_f);
			float bc2 = 1.0f - powf(beta2, step_f);
			float out_dim_f = static_cast<float>(out_dim > 0 ? out_dim : 1);
			float rms_floor = 1.0f / sqrtf(out_dim_f);
			float bg = dz;
			float bm = eff_beta1 * b_m[row] + (1.0f - eff_beta1) * bg;
			float bv = beta2 * b_v[row] + (1.0f - beta2) * bg * bg;
			b_m[row] = bm;
			b_v[row] = bv;

			float bm_hat = bm / (bc1 > 1e-7f ? bc1 : 1.0f);
			float bv_hat = bv / (bc2 > 1e-7f ? bc2 : 1.0f);
			float bias_lr = learning_rate;
			float b_update =
				bm_hat / (sqrtf(bv_hat) + rms_floor + eps) + weight_decay * b_master[row];
			float b_val = b_master[row] - bias_lr * b_update;

			if (isfinite(b_val)) {
				b_val = fmaxf(-10.0f, fminf(10.0f, b_val));
				b_master[row] = b_val;
				b[row] = __float2bfloat16(b_val);
			}
		}
	}
}

__global__ static void compute_input_gradient(const float* __restrict__ W_master,
											  const float* __restrict__ dz_vector,
											  float* __restrict__ dx_out, size_t in_dim,
											  size_t out_dim) {
	size_t j = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (j < in_dim) {
		float grad_j = 0.0f;
		for (size_t row = 0; row < out_dim; ++row) {
			grad_j += dz_vector[row] * W_master[row * in_dim + j];
		}
		dx_out[j] = isfinite(grad_j) ? grad_j : 0.0f;
	}
}

__global__ static void backward_rms_norm(const __nv_bfloat16* __restrict__ x_hat,
										 const float* __restrict__ inv_rms_ptr,
										 float* __restrict__ dx_inout,
										 size_t S,
										 size_t D) {
	size_t s = blockIdx.x;
	if (s >= S)
		return;

	size_t offset = s * D;
	__shared__ float s_sum[32];
	float dot_sum = 0.0f;

	for (size_t k = threadIdx.x; k < D; k += blockDim.x) {
		float g_k = dx_inout[offset + k];
		float x_k = __bfloat162float(x_hat[offset + k]);
		if (isfinite(g_k) && isfinite(x_k)) {
			dot_sum += g_k * x_k;
		}
	}

	for (int shift = 16; shift > 0; shift /= 2) {
		dot_sum += __shfl_down_sync(0xffffffff, dot_sum, shift);
	}

	int lane = threadIdx.x % 32;
	int wid = threadIdx.x / 32;
	if (lane == 0) {
		s_sum[wid] = dot_sum;
	}
	__syncthreads();

	if (wid == 0) {
		float warp_sum = (lane < (blockDim.x / 32)) ? s_sum[lane] : 0.0f;
		for (int shift = 16; shift > 0; shift /= 2) {
			warp_sum += __shfl_down_sync(0xffffffff, warp_sum, shift);
		}
		if (lane == 0) {
			s_sum[0] = warp_sum / static_cast<float>(D > 0 ? D : 1);
		}
	}
	__syncthreads();

	float mean_dot = s_sum[0];
	float inv_rms = (inv_rms_ptr && isfinite(inv_rms_ptr[s])) ? inv_rms_ptr[s] : 1.0f;

	for (size_t j = threadIdx.x; j < D; j += blockDim.x) {
		float g_j = dx_inout[offset + j];
		float x_j = __bfloat162float(x_hat[offset + j]);
		float dx_j = inv_rms * (g_j - x_j * mean_dot);
		dx_j = isfinite(dx_j) ? fmaxf(-10.0f, fminf(10.0f, dx_j)) : 0.0f;
		dx_inout[offset + j] = dx_j;
	}
}

__global__ static void update_output_head(const float* __restrict__ probs,
										  const __nv_bfloat16* __restrict__ y,
										  const __nv_bfloat16* __restrict__ target,
										  float* __restrict__ table_master,
										  __nv_bfloat16* __restrict__ table, size_t vocab_size,
										  size_t emb_dim, float learning_rate) {
	size_t v = blockIdx.x;
	if (v == 0 || v >= vocab_size) {
		return;
	}

	uint32_t target_tok_id = *reinterpret_cast<const uint32_t*>(target);
	if (target_tok_id >= vocab_size) {
		return;
	}

	float p = probs[v];
	if (p <= 1e-35f && v != target_tok_id) {
		return;
	}

	float grad_logit = p - (v == target_tok_id ? 1.0f : 0.0f);
	float emb_dim_f = static_cast<float>(emb_dim > 0 ? emb_dim : 1);
	float head_lr = (4.0f * learning_rate) / sqrtf(emb_dim_f);
	float bound_limit = 4.0f / sqrtf(emb_dim_f);
	size_t row_offset = v * emb_dim;

	__shared__ float s_y_sq[32];
	float local_y_sq = 0.0f;
	for (size_t d = threadIdx.x; d < emb_dim; d += blockDim.x) {
		float y_val = __bfloat162float(y[d]);
		if (isfinite(y_val)) {
			local_y_sq += y_val * y_val;
		}
	}
	for (int offset = 16; offset > 0; offset /= 2) {
		local_y_sq += __shfl_down_sync(0xffffffff, local_y_sq, offset);
	}
	int lane = threadIdx.x % 32;
	int wid = threadIdx.x / 32;
	if (lane == 0) {
		s_y_sq[wid] = local_y_sq;
	}
	__syncthreads();
	if (wid == 0) {
		float warp_sq = (lane < (blockDim.x / 32)) ? s_y_sq[lane] : 0.0f;
		for (int offset = 16; offset > 0; offset /= 2) {
			warp_sq += __shfl_down_sync(0xffffffff, warp_sq, offset);
		}
		if (lane == 0) {
			float mean_y_sq = warp_sq / emb_dim_f;
			s_y_sq[0] = fmaxf(1.0f, sqrtf(mean_y_sq));
		}
	}
	__syncthreads();
	float rms_y = s_y_sq[0];

	float token_weight_decay = 1e-5f / static_cast<float>(vocab_size > 0 ? vocab_size : 1);

	for (size_t d = threadIdx.x; d < emb_dim; d += blockDim.x) {
		float y_val = __bfloat162float(y[d]);
		if (isfinite(y_val)) {
			float norm_y = y_val / rms_y;
			float grad_e = fmaxf(-10.0f, fminf(10.0f, grad_logit * norm_y));
			size_t table_idx = row_offset + d;
			float cur_val = table_master[table_idx];
			float new_val = cur_val - head_lr * (grad_e + token_weight_decay * cur_val);
			if (isfinite(new_val)) {
				new_val = fmaxf(-bound_limit, fminf(bound_limit, new_val));
				table_master[table_idx] = new_val;
				table[table_idx] = __float2bfloat16(new_val);
			}
		}
	}
}

// Phase 5: Backward Scatter Kernel & Input Gradient Calculation
__global__ static void backward_scatter_embeddings_kernel(
	const uint32_t* __restrict__ token_ids, float* __restrict__ table_master,
	const float* __restrict__ dx_in, size_t num_tokens, size_t embedding_dim, size_t vocab_size,
	float learning_rate, bool update_table) {

	size_t in_dim = num_tokens * embedding_dim;
	size_t j = blockIdx.x * (size_t)blockDim.x + threadIdx.x;

	if (j < in_dim && update_table) {
		size_t token_idx = j / embedding_dim;
		size_t dim_idx = j % embedding_dim;
		uint32_t tok_id = token_ids[token_idx];

		// PAD (ID 0) is a fixed zero vector and never learns
		if (tok_id != 0 && tok_id < vocab_size) {
			size_t table_idx = tok_id * embedding_dim + dim_idx;
			float inv_sqrt_tokens =
				1.0f / sqrtf(static_cast<float>(num_tokens > 0 ? num_tokens : 1));
			float delta = -learning_rate * inv_sqrt_tokens * dx_in[j];
			if (isfinite(delta)) {
				atomicAdd(&table_master[table_idx], delta);
			}
		}
	}
}

__global__ static void sync_scattered_embeddings(const uint32_t* __restrict__ token_ids,
												 float* __restrict__ table_master,
												 __nv_bfloat16* __restrict__ table,
												 size_t num_tokens, size_t embedding_dim,
												 size_t vocab_size) {
	size_t in_dim = num_tokens * embedding_dim;
	size_t j = blockIdx.x * (size_t)blockDim.x + threadIdx.x;

	if (j < in_dim) {
		size_t token_idx = j / embedding_dim;
		size_t dim_idx = j % embedding_dim;
		uint32_t tok_id = token_ids[token_idx];

		if (tok_id != 0 && tok_id < vocab_size) {
			float bound_limit =
				4.0f / sqrtf(static_cast<float>(embedding_dim > 0 ? embedding_dim : 1));
			size_t table_idx = tok_id * embedding_dim + dim_idx;
			float val = table_master[table_idx];
			if (!isfinite(val)) {
				val = 0.0f;
				table_master[table_idx] = val;
			} else if (val > bound_limit) {
				val = bound_limit;
				table_master[table_idx] = val;
			} else if (val < -bound_limit) {
				val = -bound_limit;
				table_master[table_idx] = val;
			}
			table[table_idx] = __float2bfloat16(val);
		}
	}
}

extern "C" CUDA_API bool backward_scatter_embeddings(
	const GpuPortion* gpu_portion, GpuEmbeddings* gpu_embeddings, const void* d_dx,
	size_t num_tokens, size_t embedding_dim, float learning_rate, int buffer_index, void* stream) {

	if (!gpu_portion || !gpu_embeddings || !gpu_embeddings->d_table ||
		!gpu_embeddings->d_table_master || !d_dx || buffer_index < 0 || buffer_index >= 2 ||
		!gpu_portion->d_token_ids[buffer_index] || num_tokens == 0 || embedding_dim == 0) {
		return false;
	}

	cudaStream_t s = stream ? static_cast<cudaStream_t>(stream)
							: static_cast<cudaStream_t>(gpu_portion->stream[buffer_index]);
	size_t input_dim = num_tokens * embedding_dim;
	int threads = 256;
	int blocks = static_cast<int>((input_dim + threads - 1) / threads);
	bool update_table = true;

	backward_scatter_embeddings_kernel<<<blocks, threads, 0, s>>>(
		static_cast<const uint32_t*>(gpu_portion->d_token_ids[buffer_index]),
		static_cast<float*>(gpu_embeddings->d_table_master), static_cast<const float*>(d_dx),
		num_tokens, embedding_dim, gpu_embeddings->vocab_size, learning_rate, update_table);

	sync_scattered_embeddings<<<blocks, threads, 0, s>>>(
		static_cast<const uint32_t*>(gpu_portion->d_token_ids[buffer_index]),
		static_cast<float*>(gpu_embeddings->d_table_master),
		static_cast<__nv_bfloat16*>(gpu_embeddings->d_table), num_tokens, embedding_dim,
		gpu_embeddings->vocab_size);

	return CUDA_CHECK(cudaGetLastError());
}

extern "C" CUDA_API bool backward_pass_with_embeddings(
	const GpuPortion* gpu_portion, GpuEmbeddings* gpu_embeddings, GpuWeights* gpu_weights,
	const GpuActivations* gpu_activations, size_t input_dim, size_t output_dim, float learning_rate,
	int buffer_index, bool use_adamw, bool scatter_embeddings) {

	// Safe Guard
	if (!gpu_portion || !gpu_weights || !gpu_activations || buffer_index < 0 || buffer_index >= 2 ||
		!gpu_portion->d_context_window[buffer_index] || !gpu_portion->d_target[buffer_index] ||
		!gpu_weights->d_weights || !gpu_weights->d_bias || !gpu_weights->d_weights_master ||
		!gpu_weights->d_bias_master || !gpu_activations->d_output || !gpu_activations->d_loss ||
		input_dim == 0 || output_dim == 0) {
		return false;
	}

	cudaStream_t stream = (cudaStream_t)gpu_portion->stream[buffer_index];
	gpu_weights->step_count += 1;

	// Lazy allocation of AdamW weight & bias momentum & variance buffers if requested
	if (use_adamw && (!gpu_weights->d_bias_m || !gpu_weights->d_bias_v ||
			  !gpu_weights->d_weights_m || !gpu_weights->d_weights_v)) {
		size_t b_elems = output_dim;
		if (!gpu_weights->d_bias_m)
			gpu_weights->d_bias_m = allocate_vram(b_elems * sizeof(float), stream);
		if (!gpu_weights->d_bias_v)
			gpu_weights->d_bias_v = allocate_vram(b_elems * sizeof(float), stream);
		if (gpu_weights->d_bias_m && gpu_weights->d_bias_v) {
			cudaMemsetAsync(gpu_weights->d_bias_m, 0, b_elems * sizeof(float), stream);
			cudaMemsetAsync(gpu_weights->d_bias_v, 0, b_elems * sizeof(float), stream);
			gpu_weights->bias_elements = b_elems;
		}

		size_t w_elems = input_dim * output_dim;
		if (!gpu_weights->d_weights_m)
			gpu_weights->d_weights_m = allocate_vram(w_elems * sizeof(float), stream);
		if (!gpu_weights->d_weights_v)
			gpu_weights->d_weights_v = allocate_vram(w_elems * sizeof(float), stream);
		if (gpu_weights->d_weights_m && gpu_weights->d_weights_v) {
			cudaMemsetAsync(gpu_weights->d_weights_m, 0, w_elems * sizeof(float), stream);
			cudaMemsetAsync(gpu_weights->d_weights_v, 0, w_elems * sizeof(float), stream);
			gpu_weights->weight_elements = w_elems;
		}
	}

	// Milestone 3: Lazy allocation of dz gradient buffer
	GpuActivations* non_const_act = const_cast<GpuActivations*>(gpu_activations);
	if (!non_const_act->d_dz) {
		non_const_act->d_dz = allocate_vram(output_dim * sizeof(float), stream);
		if (!non_const_act->d_dz)
			return false;
	}

	bool is_token_training = gpu_portion->token_ids_active[buffer_index];
	size_t vocab_sz =
		(gpu_embeddings && gpu_embeddings->d_table)
			? (gpu_embeddings->active_vocab_size > 0 ? gpu_embeddings->active_vocab_size
													 : gpu_embeddings->vocab_size)
			: output_dim;
	size_t emb_dim =
		(gpu_embeddings && gpu_embeddings->d_table) ? gpu_embeddings->embedding_dim : output_dim;
	const __nv_bfloat16* emb_tbl = (gpu_embeddings && gpu_embeddings->d_table)
									   ? static_cast<const __nv_bfloat16*>(gpu_embeddings->d_table)
									   : nullptr;

	float inv_temp = (gpu_embeddings && gpu_embeddings->temperature > 1e-4f)
						 ? (1.0f / gpu_embeddings->temperature)
						 : 1.0f;

	GpuPortion* non_const_gpu = const_cast<GpuPortion*>(gpu_portion);
	bool has_embeddings = gpu_portion->token_ids_active[buffer_index] && gpu_embeddings &&
						  gpu_embeddings->d_table_master && gpu_embeddings->d_table;
	bool is_transformer_readout = (gpu_weights->d_query_weights != nullptr && input_dim > output_dim);

	if (is_transformer_readout && is_token_training && emb_tbl != nullptr) {
		size_t sequence_length =
			gpu_portion->token_count > 0 ? gpu_portion->token_count : (input_dim / emb_dim);
		size_t logits_count = sequence_length * vocab_sz;

		if (!non_const_gpu->d_dx[buffer_index]) {
			non_const_gpu->d_dx[buffer_index] = allocate_vram(input_dim * sizeof(float), stream);
			if (!non_const_gpu->d_dx[buffer_index])
				return false;
		}

		void* d_seq_logits = allocate_vram(logits_count * sizeof(__nv_bfloat16), stream);
		if (!d_seq_logits)
			return false;

		if (!batched_gemm_multiply(
				gpu_portion->d_context_window[buffer_index], emb_tbl, d_seq_logits,
				static_cast<int>(sequence_length), static_cast<int>(vocab_sz),
				static_cast<int>(emb_dim), false, true, stream)) {
			free_vram(d_seq_logits, stream);
			return false;
		}

		float logit_scale = inv_temp / sqrtf(static_cast<float>(emb_dim > 0 ? emb_dim : 1));
		int grid_s = static_cast<int>(sequence_length);
		int threads_v = 1024;
		compute_cross_entropy_and_dz_kernel<<<grid_s, threads_v, 0, stream>>>(
			static_cast<__nv_bfloat16*>(d_seq_logits),
			static_cast<const uint32_t*>(gpu_portion->d_token_ids[buffer_index]),
			static_cast<const __nv_bfloat16*>(gpu_portion->d_target[buffer_index]),
			static_cast<float*>(non_const_act->d_loss), sequence_length, vocab_sz, logit_scale);

		if (!CUDA_CHECK(cudaGetLastError())) {
			free_vram(d_seq_logits, stream);
			return false;
		}

		size_t dh_elements = sequence_length * emb_dim;
		void* d_dh_bf16 = allocate_vram(dh_elements * sizeof(__nv_bfloat16), stream);
		if (!d_dh_bf16) {
			free_vram(d_seq_logits, stream);
			return false;
		}

		if (!batched_gemm_multiply(
				d_seq_logits, emb_tbl, d_dh_bf16, static_cast<int>(sequence_length),
				static_cast<int>(emb_dim), static_cast<int>(vocab_sz), false, false, stream)) {
			free_vram(d_dh_bf16, stream);
			free_vram(d_seq_logits, stream);
			return false;
		}

		free_vram(d_seq_logits, stream);

		int dx_threads = 256;
		int dx_blocks = static_cast<int>((dh_elements + dx_threads - 1) / dx_threads);
		scale_dh_to_dx_kernel<<<dx_blocks, dx_threads, 0, stream>>>(
			static_cast<const __nv_bfloat16*>(d_dh_bf16),
			static_cast<float*>(non_const_gpu->d_dx[buffer_index]), dh_elements, 1.0f);

		size_t final_token_offset = (sequence_length > 0 ? sequence_length - 1 : 0) * emb_dim;
		cudaMemcpyAsync(non_const_act->d_dz,
						static_cast<float*>(non_const_gpu->d_dx[buffer_index]) + final_token_offset,
						output_dim * sizeof(float), cudaMemcpyDeviceToDevice, stream);

		free_vram(d_dh_bf16, stream);

		GpuActivations* non_const_act_event = const_cast<GpuActivations*>(gpu_activations);
		if (!non_const_act_event->loss_event) {
			cudaEventCreateWithFlags((cudaEvent_t*)&non_const_act_event->loss_event,
									 cudaEventDisableTiming);
		}
		if (non_const_act_event->loss_event) {
			cudaEventRecord((cudaEvent_t)non_const_act_event->loss_event, stream);
		}
	} else {
		int dz_threads = 256;
		int dz_blocks = static_cast<int>((output_dim + dz_threads - 1) / dz_threads);
		compute_loss_and_dz_kernel<<<dz_blocks, dz_threads, 0, stream>>>(
			static_cast<const float*>(gpu_activations->d_probs),
			static_cast<const __nv_bfloat16*>(gpu_activations->d_output),
			static_cast<const __nv_bfloat16*>(gpu_portion->d_target[buffer_index]), emb_tbl,
			static_cast<float*>(non_const_act->d_dz), static_cast<float*>(non_const_act->d_loss),
			output_dim, vocab_sz, emb_dim, is_token_training, inv_temp);

		if (!CUDA_CHECK(cudaGetLastError()))
			return false;

		GpuActivations* non_const_act_event = const_cast<GpuActivations*>(gpu_activations);
		if (!non_const_act_event->loss_event) {
			cudaEventCreateWithFlags((cudaEvent_t*)&non_const_act_event->loss_event,
									 cudaEventDisableTiming);
		}
		if (non_const_act_event->loss_event) {
			cudaEventRecord((cudaEvent_t)non_const_act_event->loss_event, stream);
		}

		if (is_transformer_readout) {
			if (!non_const_gpu->d_dx[buffer_index]) {
				non_const_gpu->d_dx[buffer_index] = allocate_vram(input_dim * sizeof(float), stream);
				if (!non_const_gpu->d_dx[buffer_index])
					return false;
			}
			size_t final_token_offset = input_dim - output_dim;
			cudaMemsetAsync(non_const_gpu->d_dx[buffer_index], 0, input_dim * sizeof(float), stream);
			cudaMemcpyAsync(static_cast<float*>(non_const_gpu->d_dx[buffer_index]) + final_token_offset,
							non_const_act->d_dz, output_dim * sizeof(float),
							cudaMemcpyDeviceToDevice, stream);
		} else if (has_embeddings || non_const_gpu->d_dx[buffer_index]) {
			if (!non_const_gpu->d_dx[buffer_index]) {
				non_const_gpu->d_dx[buffer_index] = allocate_vram(input_dim * sizeof(float), stream);
				if (!non_const_gpu->d_dx[buffer_index])
					return false;
			}
			int dx_threads = 256;
			int dx_blocks = static_cast<int>((input_dim + dx_threads - 1) / dx_threads);
			compute_input_gradient<<<dx_blocks, dx_threads, 0, stream>>>(
				static_cast<const float*>(gpu_weights->d_weights_master),
				static_cast<const float*>(non_const_act->d_dz),
				static_cast<float*>(non_const_gpu->d_dx[buffer_index]), input_dim, output_dim);

			if (gpu_embeddings && gpu_embeddings->d_table) {
				size_t emb_dim_sz =
					gpu_embeddings->embedding_dim > 0 ? gpu_embeddings->embedding_dim : AXIOM_TOKEN_DIM;
				size_t num_tok = gpu_portion->token_count > 0 ? gpu_portion->token_count
															  : (input_dim / emb_dim_sz);
				int norm_threads = static_cast<int>(std::min<size_t>(1024, emb_dim_sz));
				backward_rms_norm<<<static_cast<int>(num_tok), norm_threads, 0, stream>>>(
					static_cast<const __nv_bfloat16*>(gpu_portion->d_context_window[buffer_index]),
					static_cast<const float*>(gpu_activations->d_inv_rms),
					static_cast<float*>(non_const_gpu->d_dx[buffer_index]), num_tok, emb_dim_sz);
			}
			if (!CUDA_CHECK(cudaGetLastError()))
				return false;
		}
	}

	// Phase 4: Fused AdamW weight and bias update
	if (!is_transformer_readout && output_dim > 0 && input_dim > 0) {
		Grid grid(input_dim * output_dim);

		if (use_adamw && gpu_weights->d_bias_m && gpu_weights->d_bias_v) {
			constexpr float beta1 = 0.9f;
			constexpr float beta2 = 0.999f;
			constexpr float eps = 1e-8f;
			constexpr float weight_decay = 1e-5f;

			backward_pass_adamw_kernel<<<grid.block, grid.thread, grid.index, stream>>>(
				static_cast<float*>(gpu_weights->d_weights_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_weights),
				static_cast<float*>(gpu_weights->d_weights_m),
				static_cast<float*>(gpu_weights->d_weights_v),
				static_cast<float*>(gpu_weights->d_bias_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_bias),
				static_cast<float*>(gpu_weights->d_bias_m), static_cast<float*>(gpu_weights->d_bias_v),
				static_cast<const __nv_bfloat16*>(gpu_portion->d_context_window[buffer_index]),
				static_cast<const float*>(non_const_act->d_dz), input_dim, output_dim, learning_rate,
				gpu_weights->step_count, beta1, beta2, eps, weight_decay);
		} else {
			backward_pass_kernel<<<grid.block, grid.thread, grid.index, stream>>>(
				static_cast<float*>(gpu_weights->d_weights_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_weights),
				static_cast<float*>(gpu_weights->d_bias_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_bias),
				static_cast<const __nv_bfloat16*>(gpu_portion->d_context_window[buffer_index]),
				static_cast<const float*>(non_const_act->d_dz), input_dim, output_dim, learning_rate);
		}

		if (!CUDA_CHECK(cudaGetLastError()))
			return false;
	}

	// Phase 5: Output Head & Backward Scatter Kernel for learnable embedding matrix
	if (has_embeddings) {
		if (is_token_training && gpu_activations->d_probs && emb_dim == output_dim && vocab_sz > 0) {
			int head_threads = 256;
			int head_blocks = static_cast<int>(vocab_sz);
			update_output_head<<<head_blocks, head_threads, 0, stream>>>(
				static_cast<const float*>(gpu_activations->d_probs),
				static_cast<const __nv_bfloat16*>(gpu_activations->d_output),
				static_cast<const __nv_bfloat16*>(gpu_portion->d_target[buffer_index]),
				static_cast<float*>(gpu_embeddings->d_table_master),
				static_cast<__nv_bfloat16*>(gpu_embeddings->d_table), vocab_sz, emb_dim,
				learning_rate);
		}

		if (scatter_embeddings && !is_transformer_readout) {

		size_t num_tok =
			gpu_portion->token_count > 0 ? gpu_portion->token_count : (input_dim / emb_dim);
		int threads = 256;
		int blocks = static_cast<int>((input_dim + threads - 1) / threads);
		bool update_table = true;

		backward_scatter_embeddings_kernel<<<blocks, threads, 0, stream>>>(
			static_cast<const uint32_t*>(gpu_portion->d_token_ids[buffer_index]),
			static_cast<float*>(gpu_embeddings->d_table_master),
			static_cast<const float*>(non_const_gpu->d_dx[buffer_index]), num_tok, emb_dim,
			gpu_embeddings->vocab_size, learning_rate, update_table);

		sync_scattered_embeddings<<<blocks, threads, 0, stream>>>(
			static_cast<const uint32_t*>(gpu_portion->d_token_ids[buffer_index]),
			static_cast<float*>(gpu_embeddings->d_table_master),
			static_cast<__nv_bfloat16*>(gpu_embeddings->d_table), num_tok, emb_dim,
			gpu_embeddings->vocab_size);

		if (!CUDA_CHECK(cudaGetLastError()))
			return false;
		}
	}

	// Releases the next step's forward pass (see forward_pass)
	cudaStreamCaptureStatus capture_status = cudaStreamCaptureStatusNone;
	if (stream) {
		cudaError_t cap_err = cudaStreamIsCapturing(stream, &capture_status);
		if (cap_err != cudaSuccess) {
			cudaGetLastError();
			capture_status = cudaStreamCaptureStatusNone;
		}
	}
	if (capture_status == cudaStreamCaptureStatusNone && gpu_portion->compute_done[buffer_index]) {
		const_cast<GpuPortion*>(gpu_portion)->compute_done_recorded[buffer_index] = true;
		return CUDA_CHECK(
			cudaEventRecord((cudaEvent_t)gpu_portion->compute_done[buffer_index], stream));
	}
	return true;
}

extern "C" CUDA_API bool backward_pass(const GpuPortion* gpu_portion, GpuWeights* gpu_weights,
									   const GpuActivations* gpu_activations, size_t input_dim,
									   size_t output_dim, float learning_rate, int buffer_index) {
	return backward_pass_with_embeddings(gpu_portion, nullptr, gpu_weights, gpu_activations,
										 input_dim, output_dim, learning_rate, buffer_index, false,
										 true);
}


// =============================================================================
// Phase 3: Multi-Head Attention, SwiGLU & Transformer Backward Block
// =============================================================================

__global__ static void fused_causal_backward_kernel(
	const __nv_bfloat16* __restrict__ grad_output, const __nv_bfloat16* __restrict__ query,
	const __nv_bfloat16* __restrict__ key, const __nv_bfloat16* __restrict__ value,
	float* __restrict__ grad_query, float* __restrict__ grad_key, float* __restrict__ grad_value,
	size_t sequence_length, size_t embedding_dim, size_t num_heads, size_t head_dim, float scale) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	size_t total_queries = sequence_length * num_heads;
	if (idx >= total_queries)
		return;

	size_t i = idx / num_heads; // query token index
	size_t h = idx % num_heads; // head index

	size_t head_offset = h * head_dim;
	const __nv_bfloat16* q_vec = query + i * embedding_dim + head_offset;
	const __nv_bfloat16* do_vec = grad_output + i * embedding_dim + head_offset;

	float q_local[64];
	float do_local[64];
	float dq_local[64] = {0.0f};
	for (size_t d = 0; d < head_dim && d < 64; ++d) {
		q_local[d] = __bfloat162float(q_vec[d]);
		do_local[d] = __bfloat162float(do_vec[d]);
	}

	// Pass 1: Online softmax statistics (max_score, sum_exp, and weighted dP sum D_i)
	float max_score = -1e30f;
	float sum_exp = 0.0f;
	float weighted_dp = 0.0f;

	for (size_t j = 0; j <= i; ++j) {
		const __nv_bfloat16* k_vec = key + j * embedding_dim + head_offset;
		const __nv_bfloat16* v_vec = value + j * embedding_dim + head_offset;
		float dot = 0.0f;
		float dp_val = 0.0f;
		for (size_t d = 0; d < head_dim && d < 64; ++d) {
			dot += q_local[d] * __bfloat162float(k_vec[d]);
			dp_val += do_local[d] * __bfloat162float(v_vec[d]);
		}
		float score = dot * scale;
		float new_max = fmaxf(max_score, score);
		float exp_prev = expf(max_score - new_max);
		float exp_curr = expf(score - new_max);

		sum_exp = sum_exp * exp_prev + exp_curr;
		weighted_dp = weighted_dp * exp_prev + exp_curr * dp_val;
		max_score = new_max;
	}

	float inv_sum = 1.0f / (sum_exp > 1e-12f ? sum_exp : 1.0f);
	float D_i = weighted_dp * inv_sum;

	// Pass 2: Recompute probabilities and accumulate dV, dQ, and dK for arbitrary sequence_length
	for (size_t j = 0; j <= i; ++j) {
		const __nv_bfloat16* k_vec = key + j * embedding_dim + head_offset;
		const __nv_bfloat16* v_vec = value + j * embedding_dim + head_offset;
		float dot = 0.0f;
		float dp_val = 0.0f;
		for (size_t d = 0; d < head_dim && d < 64; ++d) {
			dot += q_local[d] * __bfloat162float(k_vec[d]);
			dp_val += do_local[d] * __bfloat162float(v_vec[d]);
		}
		float prob = expf(dot * scale - max_score) * inv_sum;
		float dS = prob * (dp_val - D_i) * scale;

		for (size_t d = 0; d < head_dim && d < 64; ++d) {
			float k_val = __bfloat162float(k_vec[d]);
			atomicAdd(&grad_value[j * embedding_dim + head_offset + d], prob * do_local[d]);
			dq_local[d] += dS * k_val;
			atomicAdd(&grad_key[j * embedding_dim + head_offset + d], dS * q_local[d]);
		}
	}

	for (size_t d = 0; d < head_dim && d < 64; ++d) {
		atomicAdd(&grad_query[i * embedding_dim + head_offset + d], dq_local[d]);
	}
}

extern "C" CUDA_API bool fused_causal_backward(const void* grad_output, const void* query,
											   const void* key, const void* value, void* grad_query,
											   void* grad_key, void* grad_value,
											   size_t sequence_length, size_t embedding_dim,
											   size_t num_heads, void* stream) {
	if (!grad_output || !query || !key || !value || !grad_query || !grad_key || !grad_value ||
		sequence_length == 0 || embedding_dim == 0 || num_heads == 0) {
		return false;
	}

	size_t head_dim = embedding_dim / num_heads;
	if (head_dim == 0 || head_dim > 64) {
		return false;
	}

	cudaStream_t s = (cudaStream_t)stream;
	size_t total_elements = sequence_length * embedding_dim;
	cudaMemsetAsync(grad_query, 0, total_elements * sizeof(float), s);
	cudaMemsetAsync(grad_key, 0, total_elements * sizeof(float), s);
	cudaMemsetAsync(grad_value, 0, total_elements * sizeof(float), s);

	float scale = 1.0f / sqrtf(static_cast<float>(head_dim));
	size_t total_queries = sequence_length * num_heads;
	int threads = 128;
	int blocks = static_cast<int>((total_queries + threads - 1) / threads);

	fused_causal_backward_kernel<<<blocks, threads, 0, s>>>(
		static_cast<const __nv_bfloat16*>(grad_output), static_cast<const __nv_bfloat16*>(query),
		static_cast<const __nv_bfloat16*>(key), static_cast<const __nv_bfloat16*>(value),
		static_cast<float*>(grad_query), static_cast<float*>(grad_key),
		static_cast<float*>(grad_value), sequence_length, embedding_dim, num_heads, head_dim,
		scale);

	return CUDA_CHECK(cudaGetLastError());
}

__global__ static void add_residual_gradient(float* __restrict__ d_target,
											 const float* __restrict__ d_source,
											 size_t total_elements) {
	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float sum = d_target[idx] + d_source[idx];
		d_target[idx] = isfinite(sum) ? fmaxf(-10.0f, fminf(10.0f, sum)) : 0.0f;
	}
}

__global__ static void accumulate_ffn_residual_kernel(float* __restrict__ d_grad_mid,
													  const float* __restrict__ grad_out,
													  const float* __restrict__ d_grad_up,
													  size_t total_elements) {
	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float out_g = grad_out[idx];
		float gate_g = d_grad_mid[idx];
		float up_g = d_grad_up[idx];
		float sum = out_g + gate_g + up_g;
		d_grad_mid[idx] = isfinite(sum) ? fmaxf(-10.0f, fminf(10.0f, sum)) : 0.0f;
	}
}

__global__ static void accumulate_attention_residual(float* __restrict__ d_grad_mid,
													 const float* __restrict__ grad_q_in,
													 const float* __restrict__ grad_k_in,
													 const float* __restrict__ grad_v_in,
													 size_t total_elements) {
	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float sum = d_grad_mid[idx] + grad_q_in[idx] + grad_k_in[idx] + grad_v_in[idx];
		d_grad_mid[idx] = isfinite(sum) ? fmaxf(-10.0f, fminf(10.0f, sum)) : 0.0f;
	}
}

__global__ static void batched_weight_gradient_kernel(const float* __restrict__ grad_out,
													  const __nv_bfloat16* __restrict__ input,
													  float* __restrict__ grad_W, int S, int in_dim,
													  int out_dim) {

	__shared__ float tile_x[16][16];
	__shared__ float tile_g[16][16];

	int o = blockIdx.x * 16 + threadIdx.x;
	int i = blockIdx.y * 16 + threadIdx.y;

	float sum = 0.0f;
	for (int base_s = 0; base_s < S; base_s += 16) {
		int s_k = base_s + threadIdx.x;
		int s_g = base_s + threadIdx.y;

		tile_x[threadIdx.y][threadIdx.x] =
			(i < in_dim && s_k < S) ? __bfloat162float(input[s_k * in_dim + i]) : 0.0f;
		tile_g[threadIdx.y][threadIdx.x] =
			(o < out_dim && s_g < S) ? grad_out[s_g * out_dim + o] : 0.0f;

		__syncthreads();

#pragma unroll
		for (int k = 0; k < 16; ++k) {
			sum += tile_x[threadIdx.y][k] * tile_g[k][threadIdx.x];
		}

		__syncthreads();
	}

	if (i < in_dim && o < out_dim) {
		float inv_s = 1.0f / static_cast<float>(S > 0 ? S : 1);
		grad_W[i * out_dim + o] = isfinite(sum) ? (sum * inv_s) : 0.0f;
	}
}

__global__ static void batched_input_gradient_kernel(const float* __restrict__ grad_out,
													 const float* __restrict__ W_master,
													 float* __restrict__ grad_X, int S, int in_dim,
													 int out_dim) {

	__shared__ float tile_g[16][16];
	__shared__ float tile_w[16][16];

	int i = blockIdx.x * 16 + threadIdx.x;
	int s = blockIdx.y * 16 + threadIdx.y;

	float sum = 0.0f;
	for (int base_o = 0; base_o < out_dim; base_o += 16) {
		int o_g = base_o + threadIdx.x;
		int o_w = base_o + threadIdx.y;

		tile_g[threadIdx.y][threadIdx.x] =
			(s < S && o_g < out_dim) ? grad_out[s * out_dim + o_g] : 0.0f;
		tile_w[threadIdx.y][threadIdx.x] =
			(i < in_dim && o_w < out_dim) ? W_master[i * out_dim + o_w] : 0.0f;

		__syncthreads();

#pragma unroll
		for (int k = 0; k < 16; ++k) {
			sum += tile_g[threadIdx.y][k] * tile_w[k][threadIdx.x];
		}

		__syncthreads();
	}

	if (s < S && i < in_dim) {
		grad_X[s * in_dim + i] = isfinite(sum) ? fmaxf(-10.0f, fminf(10.0f, sum)) : 0.0f;
	}
}

__global__ static void fused_matrix_adamw_kernel(
	float* __restrict__ W_master, __nv_bfloat16* __restrict__ W, float* __restrict__ W_m,
	float* __restrict__ W_v, const float* __restrict__ grad_W, size_t total_elements,
	float learning_rate, uint64_t step, float beta1, float beta2, float eps, float weight_decay) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float g = grad_W[idx];
		if (!isfinite(g))
			g = 0.0f;
		g = fmaxf(-1.0f, fminf(1.0f, g));

		float m = beta1 * W_m[idx] + (1.0f - beta1) * g;
		float v = beta2 * W_v[idx] + (1.0f - beta2) * g * g;
		W_m[idx] = m;
		W_v[idx] = v;

		float step_f = static_cast<float>(step);
		float bc1 = 1.0f - powf(beta1, step_f);
		float bc2 = 1.0f - powf(beta2, step_f);
		float m_hat = m / (bc1 > 1e-7f ? bc1 : 1.0f);
		float v_hat = v / (bc2 > 1e-7f ? bc2 : 1.0f);

		float update = m_hat / (sqrtf(v_hat) + eps) + weight_decay * W_master[idx];
		float w_val = W_master[idx] - learning_rate * update;

		if (isfinite(w_val)) {
			w_val = fmaxf(-10.0f, fminf(10.0f, w_val));
			W_master[idx] = w_val;
			W[idx] = __float2bfloat16(w_val);
		}
	}
}

__global__ static void float_to_bf16_grad_kernel(const float* __restrict__ src,
												 __nv_bfloat16* __restrict__ dst,
												 size_t total_elements) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		dst[idx] = __float2bfloat16(src[idx]);
	}
}

extern "C" CUDA_API bool transformer_backward_block(
	const void* grad_output, void* grad_input, const void* hidden_states, GpuWeights* gpu_weights,
	const GpuActivations* gpu_activations, size_t sequence_length, size_t embedding_dim,
	size_t num_heads, size_t ffn_dim, float learning_rate, void* stream) {
	if (!grad_output || !gpu_weights || !gpu_activations || sequence_length == 0 ||
		embedding_dim == 0 || num_heads == 0 || ffn_dim == 0) {
		return false;
	}

	cudaStream_t s = (cudaStream_t)stream;
	int S = static_cast<int>(sequence_length);
	int D = static_cast<int>(embedding_dim);
	int F = static_cast<int>(ffn_dim);
	size_t total_seq_dim = sequence_length * embedding_dim;
	size_t total_seq_ffn = sequence_length * ffn_dim;

	// Allocate temporary float gradient scratchpads
	float* d_grad_down_W = static_cast<float*>(allocate_vram(D * F * sizeof(float), s));
	float* d_grad_gate_W = static_cast<float*>(allocate_vram(F * D * sizeof(float), s));
	float* d_grad_up_W = static_cast<float*>(allocate_vram(F * D * sizeof(float), s));
	float* d_grad_out_W = static_cast<float*>(allocate_vram(D * D * sizeof(float), s));
	float* d_grad_q_W = static_cast<float*>(allocate_vram(D * D * sizeof(float), s));
	float* d_grad_k_W = static_cast<float*>(allocate_vram(D * D * sizeof(float), s));
	float* d_grad_v_W = static_cast<float*>(allocate_vram(D * D * sizeof(float), s));

	float* d_grad_swiglu_float =
		static_cast<float*>(allocate_vram(total_seq_ffn * sizeof(float), s));
	float* d_grad_gate_float = static_cast<float*>(allocate_vram(total_seq_ffn * sizeof(float), s));
	float* d_grad_up_float = static_cast<float*>(allocate_vram(total_seq_ffn * sizeof(float), s));
	float* d_grad_mid_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_ffn_up_in = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_attn_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_q_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_k_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_v_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_proj_q_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_proj_k_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_proj_v_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_attn_accum = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));

	size_t max_temp_elems = (total_seq_ffn > total_seq_dim) ? total_seq_ffn : total_seq_dim;
	__nv_bfloat16* d_grad_bf16_temp =
		static_cast<__nv_bfloat16*>(allocate_vram(max_temp_elems * sizeof(__nv_bfloat16), s));

	if (!d_grad_down_W || !d_grad_gate_W || !d_grad_up_W || !d_grad_out_W || !d_grad_q_W ||
		!d_grad_k_W || !d_grad_v_W || !d_grad_swiglu_float || !d_grad_gate_float ||
		!d_grad_up_float || !d_grad_mid_float || !d_grad_ffn_up_in || !d_grad_attn_float ||
		!d_grad_q_float || !d_grad_k_float || !d_grad_v_float || !d_grad_proj_q_float ||
		!d_grad_proj_k_float || !d_grad_proj_v_float || !d_grad_attn_accum || !d_grad_bf16_temp) {
		free_vram(d_grad_down_W, s);
		free_vram(d_grad_gate_W, s);
		free_vram(d_grad_up_W, s);
		free_vram(d_grad_out_W, s);
		free_vram(d_grad_q_W, s);
		free_vram(d_grad_k_W, s);
		free_vram(d_grad_v_W, s);
		free_vram(d_grad_swiglu_float, s);
		free_vram(d_grad_gate_float, s);
		free_vram(d_grad_up_float, s);
		free_vram(d_grad_mid_float, s);
		free_vram(d_grad_ffn_up_in, s);
		free_vram(d_grad_attn_float, s);
		free_vram(d_grad_q_float, s);
		free_vram(d_grad_k_float, s);
		free_vram(d_grad_v_float, s);
		free_vram(d_grad_proj_q_float, s);
		free_vram(d_grad_proj_k_float, s);
		free_vram(d_grad_proj_v_float, s);
		free_vram(d_grad_attn_accum, s);
		free_vram(d_grad_bf16_temp, s);
		return false;
	}

	dim3 block_dim(16, 16);

	// 1. Parameter gradient for W_down: [F x D] = SwiGLU^T * dX_out
	dim3 grid_down((D + block_dim.x - 1) / block_dim.x, (F + block_dim.y - 1) / block_dim.y);
	batched_weight_gradient_kernel<<<grid_down, block_dim, 0, s>>>(
		static_cast<const float*>(grad_output),
		static_cast<const __nv_bfloat16*>(gpu_activations->d_intermediate_swiglu), d_grad_down_W, S,
		F, D);

	// 2. Input gradient into SwiGLU: [S x F] = dX_out * W_down^T
	dim3 grid_swiglu((F + block_dim.x - 1) / block_dim.x, (S + block_dim.y - 1) / block_dim.y);
	batched_input_gradient_kernel<<<grid_swiglu, block_dim, 0, s>>>(
		static_cast<const float*>(grad_output),
		static_cast<const float*>(gpu_weights->d_down_master), d_grad_swiglu_float, S, F, D);

	// Convert d_grad_swiglu_float to bf16 for swiglu_backward
	int conv_threads = 256;
	int conv_blocks = static_cast<int>((total_seq_ffn + conv_threads - 1) / conv_threads);
	float_to_bf16_grad_kernel<<<conv_blocks, conv_threads, 0, s>>>(d_grad_swiglu_float,
																   d_grad_bf16_temp, total_seq_ffn);

	// 3. SwiGLU backward -> d_grad_gate_float and d_grad_up_float
	swiglu_backward(d_grad_bf16_temp, gpu_activations->d_intermediate_gate,
					gpu_activations->d_intermediate_up, d_grad_gate_float, d_grad_up_float,
					total_seq_ffn, s);

	// 4. Parameter gradients for W_gate and W_up: [D x F]
	dim3 grid_gate_up((F + block_dim.x - 1) / block_dim.x, (D + block_dim.y - 1) / block_dim.y);
	batched_weight_gradient_kernel<<<grid_gate_up, block_dim, 0, s>>>(
		d_grad_gate_float,
		static_cast<const __nv_bfloat16*>(gpu_activations->d_normalized_feedforward),
		d_grad_gate_W, S, D, F);
	batched_weight_gradient_kernel<<<grid_gate_up, block_dim, 0, s>>>(
		d_grad_up_float,
		static_cast<const __nv_bfloat16*>(gpu_activations->d_normalized_feedforward),
		d_grad_up_W, S, D, F);

	// 5. Backprop into mid activations: [S x D] (SwiGLU Gate + SwiGLU Up + Residual Highway)
	dim3 grid_mid((D + block_dim.x - 1) / block_dim.x, (S + block_dim.y - 1) / block_dim.y);
	batched_input_gradient_kernel<<<grid_mid, block_dim, 0, s>>>(
		d_grad_gate_float, static_cast<const float*>(gpu_weights->d_gate_master), d_grad_mid_float,
		S, D, F);
	batched_input_gradient_kernel<<<grid_mid, block_dim, 0, s>>>(
		d_grad_up_float, static_cast<const float*>(gpu_weights->d_up_master), d_grad_ffn_up_in, S,
		D, F);
	int res_threads = 256;
	int res_blocks = static_cast<int>((total_seq_dim + res_threads - 1) / res_threads);
	add_residual_gradient<<<res_blocks, res_threads, 0, s>>>(
		d_grad_mid_float, d_grad_ffn_up_in, total_seq_dim);

	backward_rms_norm<<<S, 1024, 0, s>>>(
		static_cast<const __nv_bfloat16*>(gpu_activations->d_normalized_feedforward),
		static_cast<const float*>(gpu_activations->d_feedforward_scale),
		d_grad_mid_float, S, D);

	add_residual_gradient<<<res_blocks, res_threads, 0, s>>>(
		d_grad_mid_float, static_cast<const float*>(grad_output), total_seq_dim);

	// 6. Parameter gradient for W_output: [D x D] = Attn_Out^T * d_grad_mid
	dim3 grid_d_d((D + block_dim.x - 1) / block_dim.x, (D + block_dim.y - 1) / block_dim.y);
	batched_weight_gradient_kernel<<<grid_d_d, block_dim, 0, s>>>(
		d_grad_mid_float, static_cast<const __nv_bfloat16*>(gpu_activations->d_attention_output),
		d_grad_out_W, S, D, D);

	// 7. Backprop into Attention output: [S x D]
	batched_input_gradient_kernel<<<grid_mid, block_dim, 0, s>>>(
		d_grad_mid_float, static_cast<const float*>(gpu_weights->d_output_master),
		d_grad_attn_float, S, D, D);

	// Convert d_grad_attn_float to bf16
	int conv_mid_blocks = static_cast<int>((total_seq_dim + conv_threads - 1) / conv_threads);
	float_to_bf16_grad_kernel<<<conv_mid_blocks, conv_threads, 0, s>>>(
		d_grad_attn_float, d_grad_bf16_temp, total_seq_dim);

	// 8. Fused Causal Attention backward -> d_grad_q, d_grad_k, d_grad_v
	if (!fused_causal_backward(d_grad_bf16_temp, gpu_activations->d_query, gpu_activations->d_key,
							  gpu_activations->d_value, d_grad_q_float, d_grad_k_float, d_grad_v_float,
							  sequence_length, embedding_dim, num_heads, s)) {
		free_vram(d_grad_down_W, s);
		free_vram(d_grad_gate_W, s);
		free_vram(d_grad_up_W, s);
		free_vram(d_grad_out_W, s);
		free_vram(d_grad_q_W, s);
		free_vram(d_grad_k_W, s);
		free_vram(d_grad_v_W, s);
		free_vram(d_grad_swiglu_float, s);
		free_vram(d_grad_gate_float, s);
		free_vram(d_grad_up_float, s);
		free_vram(d_grad_mid_float, s);
		free_vram(d_grad_ffn_up_in, s);
		free_vram(d_grad_attn_float, s);
		free_vram(d_grad_q_float, s);
		free_vram(d_grad_k_float, s);
		free_vram(d_grad_v_float, s);
		free_vram(d_grad_proj_q_float, s);
		free_vram(d_grad_proj_k_float, s);
		free_vram(d_grad_proj_v_float, s);
		free_vram(d_grad_attn_accum, s);
		free_vram(d_grad_bf16_temp, s);
		return false;
	}

	// 9. Parameter gradients for W_Q, W_K, W_V: [D x D]
	batched_weight_gradient_kernel<<<grid_d_d, block_dim, 0, s>>>(
		d_grad_q_float, static_cast<const __nv_bfloat16*>(gpu_activations->d_normalized_attention),
		d_grad_q_W, S, D, D);
	batched_weight_gradient_kernel<<<grid_d_d, block_dim, 0, s>>>(
		d_grad_k_float, static_cast<const __nv_bfloat16*>(gpu_activations->d_normalized_attention),
		d_grad_k_W, S, D, D);
	batched_weight_gradient_kernel<<<grid_d_d, block_dim, 0, s>>>(
		d_grad_v_float, static_cast<const __nv_bfloat16*>(gpu_activations->d_normalized_attention),
		d_grad_v_W, S, D, D);

	// Backpropagate dQ, dK, dV through W_Q^T, W_K^T, W_V^T into dedicated proj buffers
	batched_input_gradient_kernel<<<grid_mid, block_dim, 0, s>>>(
		d_grad_q_float, static_cast<const float*>(gpu_weights->d_query_master),
		d_grad_proj_q_float, S, D, D);
	batched_input_gradient_kernel<<<grid_mid, block_dim, 0, s>>>(
		d_grad_k_float, static_cast<const float*>(gpu_weights->d_key_master),
		d_grad_proj_k_float, S, D, D);
	batched_input_gradient_kernel<<<grid_mid, block_dim, 0, s>>>(
		d_grad_v_float, static_cast<const float*>(gpu_weights->d_value_master),
		d_grad_proj_v_float, S, D, D);

	cudaMemsetAsync(d_grad_attn_accum, 0, total_seq_dim * sizeof(float), s);
	accumulate_attention_residual<<<res_blocks, res_threads, 0, s>>>(
		d_grad_attn_accum, d_grad_proj_q_float, d_grad_proj_k_float, d_grad_proj_v_float, total_seq_dim);

	backward_rms_norm<<<S, 1024, 0, s>>>(
		static_cast<const __nv_bfloat16*>(gpu_activations->d_normalized_attention),
		static_cast<const float*>(gpu_activations->d_attention_scale),
		d_grad_attn_accum, S, D);

	add_residual_gradient<<<res_blocks, res_threads, 0, s>>>(
		d_grad_mid_float, d_grad_attn_accum, total_seq_dim);

	// 10. Update all 7 transformer parameter matrices using Fused AdamW
	gpu_weights->step_count += 1;
	uint64_t step = gpu_weights->step_count;
	constexpr float beta1 = 0.9f;
	constexpr float beta2 = 0.999f;
	constexpr float eps = 1e-8f;
	constexpr float weight_decay = 1e-5f;

	auto apply_adamw = [&](float* master, __nv_bfloat16* bf16_w, float* m, float* v, const float* g,
						   size_t elements) {
		int threads = 256;
		int blocks = static_cast<int>((elements + threads - 1) / threads);
		fused_matrix_adamw_kernel<<<blocks, threads, 0, s>>>(master, bf16_w, m, v, g, elements,
															 learning_rate, step, beta1, beta2, eps,
															 weight_decay);
	};

	apply_adamw(static_cast<float*>(gpu_weights->d_down_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_down_weights),
				static_cast<float*>(gpu_weights->d_down_m),
				static_cast<float*>(gpu_weights->d_down_v), d_grad_down_W, D * F);

	apply_adamw(static_cast<float*>(gpu_weights->d_gate_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_gate_weights),
				static_cast<float*>(gpu_weights->d_gate_m),
				static_cast<float*>(gpu_weights->d_gate_v), d_grad_gate_W, F * D);

	apply_adamw(static_cast<float*>(gpu_weights->d_up_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_up_weights),
				static_cast<float*>(gpu_weights->d_up_m), static_cast<float*>(gpu_weights->d_up_v),
				d_grad_up_W, F * D);

	apply_adamw(static_cast<float*>(gpu_weights->d_output_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_output_weights),
				static_cast<float*>(gpu_weights->d_output_m),
				static_cast<float*>(gpu_weights->d_output_v), d_grad_out_W, D * D);

	apply_adamw(static_cast<float*>(gpu_weights->d_query_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_query_weights),
				static_cast<float*>(gpu_weights->d_query_m),
				static_cast<float*>(gpu_weights->d_query_v), d_grad_q_W, D * D);

	apply_adamw(static_cast<float*>(gpu_weights->d_key_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_key_weights),
				static_cast<float*>(gpu_weights->d_key_m),
				static_cast<float*>(gpu_weights->d_key_v), d_grad_k_W, D * D);

	apply_adamw(static_cast<float*>(gpu_weights->d_value_master),
				static_cast<__nv_bfloat16*>(gpu_weights->d_value_weights),
				static_cast<float*>(gpu_weights->d_value_m),
				static_cast<float*>(gpu_weights->d_value_v), d_grad_v_W, D * D);

	// 11. Accumulate input gradient into grad_input if requested
	if (grad_input) {
		cudaMemcpyAsync(grad_input, d_grad_mid_float, total_seq_dim * sizeof(float),
						cudaMemcpyDeviceToDevice, s);
	}

	// Free temporary scratchpads deterministically
	free_vram(d_grad_down_W, s);
	free_vram(d_grad_gate_W, s);
	free_vram(d_grad_up_W, s);
	free_vram(d_grad_out_W, s);
	free_vram(d_grad_q_W, s);
	free_vram(d_grad_k_W, s);
	free_vram(d_grad_v_W, s);
	free_vram(d_grad_swiglu_float, s);
	free_vram(d_grad_gate_float, s);
	free_vram(d_grad_up_float, s);
	free_vram(d_grad_mid_float, s);
	free_vram(d_grad_ffn_up_in, s);
	free_vram(d_grad_attn_float, s);
	free_vram(d_grad_q_float, s);
	free_vram(d_grad_k_float, s);
	free_vram(d_grad_v_float, s);
	free_vram(d_grad_proj_q_float, s);
	free_vram(d_grad_proj_k_float, s);
	free_vram(d_grad_proj_v_float, s);
	free_vram(d_grad_attn_accum, s);
	free_vram(d_grad_bf16_temp, s);

	return CUDA_CHECK(cudaGetLastError());
}
