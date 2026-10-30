#include <cuda_bf16.h>
#include <cuda_runtime.h>

#include "../util/caching_allocator.cuh"
#include "../util/cuda_utils.cuh"
#include "backward_pass.cuh"

// =============================================================================
// =============================================================================
// Milestone 3: Discrete Autoregressive Token Classification Backward Pass
//
// 1. Cross-Entropy Loss: L = -log(P_{target})
// 2. Exact bounded logit gradient: dL/dz_i = P_i - 1[i == target]  in [-1.0, +1.0]
// 3. RMSNorm backpropagation scaling: dx_j = inv_rms * (dL/d_x_hat_j)
// =============================================================================

__global__ static void compute_loss_and_dz_kernel(
	const float* __restrict__ probs,
	const __nv_bfloat16* __restrict__ y,
	const __nv_bfloat16* __restrict__ target,
	const __nv_bfloat16* __restrict__ emb_table,
	float* __restrict__ dz,
	float* __restrict__ loss,
	size_t out_dim,
	size_t vocab_size,
	size_t emb_dim,
	bool is_token_training) {

	size_t row = blockIdx.x * (size_t)blockDim.x + threadIdx.x;

	if (is_token_training && probs != nullptr) {
		uint32_t target_tok_id = *reinterpret_cast<const uint32_t*>(target);
		if (target_tok_id >= vocab_size) {
			target_tok_id = 0;
		}

		if (row == 0) {
			float p_target = probs[target_tok_id];
			float ce_loss = -logf(fmaxf(p_target, 1e-12f));
			if (isfinite(ce_loss)) {
				atomicAdd(loss, ce_loss);
			}
		}

		if (row < out_dim) {
			float dz_val = 0.0f;
			if (emb_table != nullptr && emb_dim == out_dim) {
				for (size_t v = 0; v < vocab_size; ++v) {
					float p = probs[v];
					if (p > 1e-7f || v == target_tok_id) {
						float grad_logit = p - (v == target_tok_id ? 1.0f : 0.0f);
						float e_val = __bfloat162float(emb_table[v * emb_dim + row]);
						dz_val += grad_logit * e_val;
					}
				}
			} else {
				if (row < vocab_size) {
					dz_val = probs[row] - (row == target_tok_id ? 1.0f : 0.0f);
				}
			}

			dz_val = fmaxf(-1.0f, fminf(1.0f, dz_val));
			dz[row] = dz_val;
		}
	} else {
		if (row < out_dim) {
			float y_val = __bfloat162float(y[row]);
			float t_val = __bfloat162float(target[row]);
			float dy = y_val - t_val;
			if (isnan(dy) || isinf(dy)) {
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
				atomicAdd(loss, mse_loss);
			}
		}
	}
}

__global__ static void backward_pass_kernel(float* __restrict__ W_master,
					    __nv_bfloat16* __restrict__ W,
					    float* __restrict__ b_master,
					    __nv_bfloat16* __restrict__ b,
					    const __nv_bfloat16* __restrict__ x,
					    const float* __restrict__ dz_vector,
					    size_t in_dim,
					    size_t out_dim,
					    float learning_rate) {

	size_t i = blockIdx.x * (size_t)blockDim.x + threadIdx.x;

	if (i < out_dim * in_dim) {
		size_t row = i / in_dim;
		size_t col = i % in_dim;

		float dz = dz_vector[row];

		float x_val = __bfloat162float(x[col]);
		if (isnan(x_val) || isinf(x_val)) {
			x_val = 0.0f;
		}

		float w_val = W_master[i] - learning_rate * dz * x_val;
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
__global__ static void backward_pass_adamw_kernel(float* __restrict__ W_master,
						  __nv_bfloat16* __restrict__ W,
						  float* __restrict__ W_m,
						  float* __restrict__ W_v,
						  float* __restrict__ b_master,
						  __nv_bfloat16* __restrict__ b,
						  float* __restrict__ b_m,
						  float* __restrict__ b_v,
						  const __nv_bfloat16* __restrict__ x,
						  const float* __restrict__ dz_vector,
						  size_t in_dim,
						  size_t out_dim,
						  float learning_rate,
						  uint64_t step,
						  float beta1,
						  float beta2,
						  float eps,
						  float weight_decay) {

	size_t i = blockIdx.x * (size_t)blockDim.x + threadIdx.x;

	if (i < out_dim * in_dim) {
		size_t row = i / in_dim;
		size_t col = i % in_dim;

		float dz = dz_vector[row];

		float x_val = __bfloat162float(x[col]);
		if (isnan(x_val) || isinf(x_val)) {
			x_val = 0.0f;
		}

		float g = dz * x_val;
		float m = beta1 * W_m[i] + (1.0f - beta1) * g;
		float v = beta2 * W_v[i] + (1.0f - beta2) * g * g;
		W_m[i] = m;
		W_v[i] = v;

		float step_f = static_cast<float>(step);
		float bc1 = 1.0f - powf(beta1, step_f);
		float bc2 = 1.0f - powf(beta2, step_f);
		float m_hat = m / (bc1 > 1e-7f ? bc1 : 1.0f);
		float v_hat = v / (bc2 > 1e-7f ? bc2 : 1.0f);

		float update = m_hat / (sqrtf(v_hat) + eps) + weight_decay * W_master[i];
		float w_val = W_master[i] - learning_rate * update;

		if (isfinite(w_val)) {
			w_val = fmaxf(-10.0f, fminf(10.0f, w_val));
			W_master[i] = w_val;
			W[i] = __float2bfloat16(w_val);
		}

		if (col == 0) {
			float bg = dz;
			float bm = beta1 * b_m[row] + (1.0f - beta1) * bg;
			float bv = beta2 * b_v[row] + (1.0f - beta2) * bg * bg;
			b_m[row] = bm;
			b_v[row] = bv;

			float bm_hat = bm / (bc1 > 1e-7f ? bc1 : 1.0f);
			float bv_hat = bv / (bc2 > 1e-7f ? bc2 : 1.0f);
			float b_update = bm_hat / (sqrtf(bv_hat) + eps);
			float b_val = b_master[row] - learning_rate * b_update;

			if (isfinite(b_val)) {
				b_val = fmaxf(-10.0f, fminf(10.0f, b_val));
				b_master[row] = b_val;
				b[row] = __float2bfloat16(b_val);
			}
		}
	}
}

// Phase 5: Backward Scatter Kernel & Input Gradient Calculation
__global__ static void backward_scatter_embeddings_kernel(
	const uint32_t* __restrict__ token_ids,
	float* __restrict__ table_master,
	__nv_bfloat16* __restrict__ table,
	const float* __restrict__ W_master,
	const float* __restrict__ dz_vector,
	const float* __restrict__ inv_rms_ptr,
	size_t num_tokens,
	size_t embedding_dim,
	size_t vocab_size,
	size_t out_dim,
	float learning_rate) {

	size_t in_dim = num_tokens * embedding_dim;
	size_t j = blockIdx.x * (size_t)blockDim.x + threadIdx.x;

	if (j < in_dim) {
		float dx_j = 0.0f;
		for (size_t row = 0; row < out_dim; ++row) {
			float dz = dz_vector[row];
			float w = W_master[row * in_dim + j];
			dx_j += dz * w;
		}

		// Milestone 3: Scale gradient through RMSNorm
		float inv_rms = (inv_rms_ptr && isfinite(*inv_rms_ptr)) ? *inv_rms_ptr : 1.0f;
		dx_j *= inv_rms;
		dx_j = fmaxf(-1.0f, fminf(1.0f, dx_j));

		// Accumulate gradient into FP32 master table with atomicAdd
		size_t token_idx = j / embedding_dim;
		size_t dim_idx = j % embedding_dim;
		uint32_t tok_id = token_ids[token_idx];

		// PAD (ID 0) is a fixed zero vector and never learns
		if (tok_id != 0 && tok_id < vocab_size) {
			size_t table_idx = tok_id * embedding_dim + dim_idx;
			atomicAdd(&table_master[table_idx], -learning_rate * dx_j);

			float new_val = table_master[table_idx];
			if (isfinite(new_val)) {
				new_val = fmaxf(-5.0f, fminf(5.0f, new_val));
				table_master[table_idx] = new_val;
				table[table_idx] = __float2bfloat16(new_val);
			}
		}
	}
}

extern "C" CUDA_API bool backward_pass_with_embeddings(const GpuPortion* gpu_portion,
						      GpuEmbeddings* gpu_embeddings,
						      GpuWeights* gpu_weights,
						      const GpuActivations* gpu_activations,
						      size_t input_dim,
						      size_t output_dim,
						      float learning_rate,
						      int buffer_index,
						      bool use_adamw) {

	// Safe Guard
	if (!gpu_portion || !gpu_weights || !gpu_activations ||
	    !gpu_portion->d_context_window[buffer_index] || !gpu_portion->d_target[buffer_index] ||
	    !gpu_weights->d_weights || !gpu_weights->d_bias || !gpu_weights->d_weights_master ||
	    !gpu_weights->d_bias_master || !gpu_activations->d_output || !gpu_activations->d_loss ||
	    input_dim == 0 || output_dim == 0) {
		return false;
	}

	cudaStream_t stream = (cudaStream_t)gpu_portion->stream[buffer_index];
	gpu_weights->step_count += 1;

	// Lazy allocation of AdamW momentum & variance buffers if requested
	if (use_adamw && (!gpu_weights->d_weights_m || !gpu_weights->d_weights_v)) {
		size_t w_elems = input_dim * output_dim;
		size_t b_elems = output_dim;
		gpu_weights->d_weights_m = allocate_vram(w_elems * sizeof(float), stream);
		gpu_weights->d_weights_v = allocate_vram(w_elems * sizeof(float), stream);
		gpu_weights->d_bias_m = allocate_vram(b_elems * sizeof(float), stream);
		gpu_weights->d_bias_v = allocate_vram(b_elems * sizeof(float), stream);
		if (gpu_weights->d_weights_m && gpu_weights->d_weights_v &&
		    gpu_weights->d_bias_m && gpu_weights->d_bias_v) {
			cudaMemsetAsync(gpu_weights->d_weights_m, 0, w_elems * sizeof(float), stream);
			cudaMemsetAsync(gpu_weights->d_weights_v, 0, w_elems * sizeof(float), stream);
			cudaMemsetAsync(gpu_weights->d_bias_m, 0, b_elems * sizeof(float), stream);
			cudaMemsetAsync(gpu_weights->d_bias_v, 0, b_elems * sizeof(float), stream);
		} else {
			use_adamw = false;
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
	size_t vocab_sz = (gpu_embeddings && gpu_embeddings->d_table)
				  ? (gpu_embeddings->active_vocab_size > 0 ? gpu_embeddings->active_vocab_size
									   : gpu_embeddings->vocab_size)
				  : output_dim;
	size_t emb_dim = (gpu_embeddings && gpu_embeddings->d_table) ? gpu_embeddings->embedding_dim : output_dim;
	const __nv_bfloat16* emb_tbl = (gpu_embeddings && gpu_embeddings->d_table)
					       ? static_cast<const __nv_bfloat16*>(gpu_embeddings->d_table)
					       : nullptr;

	int dz_threads = 256;
	int dz_blocks = static_cast<int>((output_dim + dz_threads - 1) / dz_threads);
	compute_loss_and_dz_kernel<<<dz_blocks, dz_threads, 0, stream>>>(
		static_cast<const float*>(gpu_activations->d_probs),
		static_cast<const __nv_bfloat16*>(gpu_activations->d_output),
		static_cast<const __nv_bfloat16*>(gpu_portion->d_target[buffer_index]),
		emb_tbl,
		static_cast<float*>(non_const_act->d_dz),
		static_cast<float*>(non_const_act->d_loss),
		output_dim,
		vocab_sz,
		emb_dim,
		is_token_training);

	if (!CUDA_CHECK(cudaGetLastError()))
		return false;

	GpuActivations* non_const_act_event = const_cast<GpuActivations*>(gpu_activations);
	if (!non_const_act_event->loss_event) {
		cudaEventCreateWithFlags(
			(cudaEvent_t*)&non_const_act_event->loss_event,
			cudaEventDisableTiming);
	}
	if (non_const_act_event->loss_event) {
		cudaEventRecord((cudaEvent_t)non_const_act_event->loss_event, stream);
	}

	Grid grid(input_dim * output_dim);

	if (use_adamw && gpu_weights->d_weights_m && gpu_weights->d_weights_v) {
		constexpr float beta1 = 0.9f;
		constexpr float beta2 = 0.999f;
		constexpr float eps = 1e-8f;
		constexpr float weight_decay = 0.01f;

		backward_pass_adamw_kernel<<<grid.block, grid.thread, grid.index, stream>>>(
			static_cast<float*>(gpu_weights->d_weights_master),
			static_cast<__nv_bfloat16*>(gpu_weights->d_weights),
			static_cast<float*>(gpu_weights->d_weights_m),
			static_cast<float*>(gpu_weights->d_weights_v),
			static_cast<float*>(gpu_weights->d_bias_master),
			static_cast<__nv_bfloat16*>(gpu_weights->d_bias),
			static_cast<float*>(gpu_weights->d_bias_m),
			static_cast<float*>(gpu_weights->d_bias_v),
			static_cast<const __nv_bfloat16*>(gpu_portion->d_context_window[buffer_index]),
			static_cast<const float*>(non_const_act->d_dz),
			input_dim,
			output_dim,
			learning_rate,
			gpu_weights->step_count,
			beta1,
			beta2,
			eps,
			weight_decay);
	} else {
		backward_pass_kernel<<<grid.block, grid.thread, grid.index, stream>>>(
			static_cast<float*>(gpu_weights->d_weights_master),
			static_cast<__nv_bfloat16*>(gpu_weights->d_weights),
			static_cast<float*>(gpu_weights->d_bias_master),
			static_cast<__nv_bfloat16*>(gpu_weights->d_bias),
			static_cast<const __nv_bfloat16*>(gpu_portion->d_context_window[buffer_index]),
			static_cast<const float*>(non_const_act->d_dz),
			input_dim,
			output_dim,
			learning_rate);
	}

	if (!CUDA_CHECK(cudaGetLastError()))
		return false;

	// Phase 5: Backward Scatter Kernel for learnable embedding matrix
	if (gpu_portion->token_ids_active[buffer_index] && gpu_embeddings &&
	    gpu_embeddings->d_table_master && gpu_embeddings->d_table) {
		size_t num_tok = gpu_portion->token_count > 0 ? gpu_portion->token_count : (input_dim / emb_dim);
		int threads = 256;
		int blocks = static_cast<int>((input_dim + threads - 1) / threads);

		backward_scatter_embeddings_kernel<<<blocks, threads, 0, stream>>>(
			static_cast<const uint32_t*>(gpu_portion->d_token_ids[buffer_index]),
			static_cast<float*>(gpu_embeddings->d_table_master),
			static_cast<__nv_bfloat16*>(gpu_embeddings->d_table),
			static_cast<const float*>(gpu_weights->d_weights_master),
			static_cast<const float*>(non_const_act->d_dz),
			static_cast<const float*>(gpu_activations->d_inv_rms),
			num_tok,
			emb_dim,
			gpu_embeddings->vocab_size,
			output_dim,
			learning_rate);

		if (!CUDA_CHECK(cudaGetLastError()))
			return false;
	}

	// Releases the next step's forward pass (see forward_pass)
	return CUDA_CHECK(cudaEventRecord((cudaEvent_t)gpu_portion->compute_done[buffer_index], stream));
}

extern "C" CUDA_API bool backward_pass(const GpuPortion* gpu_portion,
				       GpuWeights* gpu_weights,
				       const GpuActivations* gpu_activations,
				       size_t input_dim,
				       size_t output_dim,
				       float learning_rate,
				       int buffer_index) {
	return backward_pass_with_embeddings(gpu_portion,
					    nullptr,
					    gpu_weights,
					    gpu_activations,
					    input_dim,
					    output_dim,
					    learning_rate,
					    buffer_index,
					    false);
}


// =============================================================================
// Phase 3: Multi-Head Attention, SwiGLU & Transformer Backward Block
// =============================================================================

__global__ static void fused_causal_backward_kernel(
	const __nv_bfloat16* __restrict__ grad_output,
	const __nv_bfloat16* __restrict__ query,
	const __nv_bfloat16* __restrict__ key,
	const __nv_bfloat16* __restrict__ value,
	float* __restrict__ grad_query,
	float* __restrict__ grad_key,
	float* __restrict__ grad_value,
	size_t sequence_length,
	size_t embedding_dim,
	size_t num_heads,
	size_t head_dim,
	float scale) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	size_t total_queries = sequence_length * num_heads;
	if (idx >= total_queries)
		return;

	size_t i = idx / num_heads; // query token index
	size_t h = idx % num_heads; // head index

	size_t head_offset = h * head_dim;
	const __nv_bfloat16* q_vec = query + i * embedding_dim + head_offset;
	const __nv_bfloat16* do_vec = grad_output + i * embedding_dim + head_offset;

	// 1. Recompute attention scores S_{i, j} and softmax weights P_{i, j} for j <= i
	float max_score = -1e30f;
	float scores[128];
	if (sequence_length > 128)
		return;

	for (size_t j = 0; j <= i; ++j) {
		const __nv_bfloat16* k_vec = key + j * embedding_dim + head_offset;
		float dot = 0.0f;
		for (size_t d = 0; d < head_dim && d < 64; ++d) {
			dot += __bfloat162float(q_vec[d]) * __bfloat162float(k_vec[d]);
		}
		float score = dot * scale;
		scores[j] = score;
		if (score > max_score) {
			max_score = score;
		}
	}

	float sum_exp = 0.0f;
	float probs[128];
	for (size_t j = 0; j <= i; ++j) {
		float exp_s = expf(scores[j] - max_score);
		probs[j] = exp_s;
		sum_exp += exp_s;
	}
	float inv_sum = 1.0f / (sum_exp > 1e-12f ? sum_exp : 1.0f);
	for (size_t j = 0; j <= i; ++j) {
		probs[j] *= inv_sum;
	}

	// 2. Compute dP_{i, j} and D_i
	float dP[128];
	float D_i = 0.0f;
	for (size_t j = 0; j <= i; ++j) {
		const __nv_bfloat16* v_vec = value + j * embedding_dim + head_offset;
		float dp_val = 0.0f;
		for (size_t d = 0; d < head_dim && d < 64; ++d) {
			float do_val = __bfloat162float(do_vec[d]);
			float v_val = __bfloat162float(v_vec[d]);
			dp_val += do_val * v_val;

			atomicAdd(&grad_value[j * embedding_dim + head_offset + d], probs[j] * do_val);
		}
		dP[j] = dp_val;
		D_i += probs[j] * dp_val;
	}

	// 3. Compute dS_{i, j} and accumulate into dQ_{i, d} and dK_{j, d}
	float dq_local[64] = {0.0f};
	for (size_t j = 0; j <= i; ++j) {
		float dS = probs[j] * (dP[j] - D_i) * scale;
		const __nv_bfloat16* k_vec = key + j * embedding_dim + head_offset;

		for (size_t d = 0; d < head_dim && d < 64; ++d) {
			float k_val = __bfloat162float(k_vec[d]);
			float q_val = __bfloat162float(q_vec[d]);

			dq_local[d] += dS * k_val;
			atomicAdd(&grad_key[j * embedding_dim + head_offset + d], dS * q_val);
		}
	}

	for (size_t d = 0; d < head_dim && d < 64; ++d) {
		atomicAdd(&grad_query[i * embedding_dim + head_offset + d], dq_local[d]);
	}
}

extern "C" CUDA_API bool fused_causal_backward(const void* grad_output,
					       const void* query,
					       const void* key,
					       const void* value,
					       void* grad_query,
					       void* grad_key,
					       void* grad_value,
					       size_t sequence_length,
					       size_t embedding_dim,
					       size_t num_heads,
					       void* stream) {
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
		static_cast<const __nv_bfloat16*>(grad_output),
		static_cast<const __nv_bfloat16*>(query),
		static_cast<const __nv_bfloat16*>(key),
		static_cast<const __nv_bfloat16*>(value),
		static_cast<float*>(grad_query),
		static_cast<float*>(grad_key),
		static_cast<float*>(grad_value),
		sequence_length,
		embedding_dim,
		num_heads,
		head_dim,
		scale);

	return CUDA_CHECK(cudaGetLastError());
}

__global__ static void swiglu_backward_kernel(
	const __nv_bfloat16* __restrict__ grad_output,
	const __nv_bfloat16* __restrict__ gate,
	const __nv_bfloat16* __restrict__ up,
	float* __restrict__ grad_gate,
	float* __restrict__ grad_up,
	size_t total_elements) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float dh = __bfloat162float(grad_output[idx]);
		float g = __bfloat162float(gate[idx]);
		float u = __bfloat162float(up[idx]);

		float sig = 1.0f / (1.0f + expf(-g));
		float swish = g * sig;
		float swish_prime = sig * (1.0f + g * (1.0f - sig));

		grad_up[idx] = dh * swish;
		grad_gate[idx] = dh * u * swish_prime;
	}
}

extern "C" CUDA_API bool swiglu_backward(const void* grad_output,
					 const void* gate,
					 const void* up,
					 void* grad_gate,
					 void* grad_up,
					 size_t total_elements,
					 void* stream) {
	if (!grad_output || !gate || !up || !grad_gate || !grad_up || total_elements == 0) {
		return false;
	}

	cudaStream_t s = (cudaStream_t)stream;
	int threads = 256;
	int blocks = static_cast<int>((total_elements + threads - 1) / threads);

	swiglu_backward_kernel<<<blocks, threads, 0, s>>>(
		static_cast<const __nv_bfloat16*>(grad_output),
		static_cast<const __nv_bfloat16*>(gate),
		static_cast<const __nv_bfloat16*>(up),
		static_cast<float*>(grad_gate),
		static_cast<float*>(grad_up),
		total_elements);

	return CUDA_CHECK(cudaGetLastError());
}

__global__ static void batched_weight_gradient_kernel(
	const float* __restrict__ grad_out,
	const __nv_bfloat16* __restrict__ input,
	float* __restrict__ grad_W,
	int S, int OutDim, int InDim) {

	int in_col = blockIdx.x * blockDim.x + threadIdx.x;
	int out_row = blockIdx.y * blockDim.y + threadIdx.y;

	if (out_row < OutDim && in_col < InDim) {
		float sum = 0.0f;
		for (int s = 0; s < S; ++s) {
			float dz = grad_out[s * OutDim + out_row];
			float x = __bfloat162float(input[s * InDim + in_col]);
			sum += dz * x;
		}
		grad_W[out_row * InDim + in_col] = sum;
	}
}

__global__ static void batched_input_gradient_kernel(
	const float* __restrict__ grad_out,
	const float* __restrict__ W_master,
	float* __restrict__ grad_X,
	int S, int OutDim, int InDim) {

	int s = blockIdx.y * blockDim.y + threadIdx.y;
	int in_col = blockIdx.x * blockDim.x + threadIdx.x;

	if (s < S && in_col < InDim) {
		float sum = 0.0f;
		for (int o = 0; o < OutDim; ++o) {
			float dz = grad_out[s * OutDim + o];
			float w = W_master[o * InDim + in_col];
			sum += dz * w;
		}
		grad_X[s * InDim + in_col] = sum;
	}
}

__global__ static void fused_matrix_adamw_kernel(
	float* __restrict__ W_master,
	__nv_bfloat16* __restrict__ W,
	float* __restrict__ W_m,
	float* __restrict__ W_v,
	const float* __restrict__ grad_W,
	size_t total_elements,
	float learning_rate,
	uint64_t step,
	float beta1,
	float beta2,
	float eps,
	float weight_decay) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float g = grad_W[idx];
		if (!isfinite(g))
			g = 0.0f;

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

__global__ static void float_to_bf16_grad_kernel(
	const float* __restrict__ src,
	__nv_bfloat16* __restrict__ dst,
	size_t total_elements) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		dst[idx] = __float2bfloat16(src[idx]);
	}
}

extern "C" CUDA_API bool transformer_backward_block(const void* grad_output,
						    void* grad_input,
						    const void* hidden_states,
						    GpuWeights* gpu_weights,
						    const GpuActivations* gpu_activations,
						    size_t sequence_length,
						    size_t embedding_dim,
						    size_t num_heads,
						    size_t ffn_dim,
						    float learning_rate,
						    void* stream) {
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

	float* d_grad_swiglu_float = static_cast<float*>(allocate_vram(total_seq_ffn * sizeof(float), s));
	float* d_grad_gate_float = static_cast<float*>(allocate_vram(total_seq_ffn * sizeof(float), s));
	float* d_grad_up_float = static_cast<float*>(allocate_vram(total_seq_ffn * sizeof(float), s));
	float* d_grad_mid_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_attn_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_q_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_k_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));
	float* d_grad_v_float = static_cast<float*>(allocate_vram(total_seq_dim * sizeof(float), s));

	__nv_bfloat16* d_grad_bf16_temp = static_cast<__nv_bfloat16*>(allocate_vram(total_seq_ffn * sizeof(__nv_bfloat16), s));

	if (!d_grad_down_W || !d_grad_gate_W || !d_grad_up_W || !d_grad_out_W ||
	    !d_grad_q_W || !d_grad_k_W || !d_grad_v_W || !d_grad_swiglu_float ||
	    !d_grad_gate_float || !d_grad_up_float || !d_grad_mid_float ||
	    !d_grad_attn_float || !d_grad_q_float || !d_grad_k_float || !d_grad_v_float ||
	    !d_grad_bf16_temp) {
		return false;
	}

	dim3 block_dim(16, 16);

	// 1. Parameter gradient for W_down: [D x F] = dX_out^T * SwiGLU
	dim3 grid_down((F + block_dim.x - 1) / block_dim.x, (D + block_dim.y - 1) / block_dim.y);
	batched_weight_gradient_kernel<<<grid_down, block_dim, 0, s>>>(
		static_cast<const float*>(grad_output),
		static_cast<const __nv_bfloat16*>(gpu_activations->d_intermediate_swiglu),
		d_grad_down_W,
		S, D, F);

	// 2. Input gradient into SwiGLU: [S x F] = dX_out * W_down
	dim3 grid_swiglu((F + block_dim.x - 1) / block_dim.x, (S + block_dim.y - 1) / block_dim.y);
	batched_input_gradient_kernel<<<grid_swiglu, block_dim, 0, s>>>(
		static_cast<const float*>(grad_output),
		static_cast<const float*>(gpu_weights->d_down_master),
		d_grad_swiglu_float,
		S, D, F);

	// Convert d_grad_swiglu_float to bf16 for swiglu_backward
	int conv_threads = 256;
	int conv_blocks = static_cast<int>((total_seq_ffn + conv_threads - 1) / conv_threads);
	float_to_bf16_grad_kernel<<<conv_blocks, conv_threads, 0, s>>>(
		d_grad_swiglu_float, d_grad_bf16_temp, total_seq_ffn);

	// 3. SwiGLU backward -> d_grad_gate_float and d_grad_up_float
	swiglu_backward(d_grad_bf16_temp,
			gpu_activations->d_intermediate_gate,
			gpu_activations->d_intermediate_up,
			d_grad_gate_float,
			d_grad_up_float,
			total_seq_ffn,
			s);

	// 4. Parameter gradients for W_gate and W_up: [F x D]
	dim3 grid_gate_up((D + block_dim.x - 1) / block_dim.x, (F + block_dim.y - 1) / block_dim.y);
	batched_weight_gradient_kernel<<<grid_gate_up, block_dim, 0, s>>>(
		d_grad_gate_float,
		static_cast<const __nv_bfloat16*>(gpu_activations->d_residual_mid),
		d_grad_gate_W,
		S, F, D);
	batched_weight_gradient_kernel<<<grid_gate_up, block_dim, 0, s>>>(
		d_grad_up_float,
		static_cast<const __nv_bfloat16*>(gpu_activations->d_residual_mid),
		d_grad_up_W,
		S, F, D);

	// 5. Backprop into mid activations: [S x D]
	dim3 grid_mid((D + block_dim.x - 1) / block_dim.x, (S + block_dim.y - 1) / block_dim.y);
	batched_input_gradient_kernel<<<grid_mid, block_dim, 0, s>>>(
		d_grad_gate_float,
		static_cast<const float*>(gpu_weights->d_gate_master),
		d_grad_mid_float,
		S, F, D);

	// 6. Parameter gradient for W_output: [D x D] = d_grad_mid^T * Attn_Out
	dim3 grid_d_d((D + block_dim.x - 1) / block_dim.x, (D + block_dim.y - 1) / block_dim.y);
	batched_weight_gradient_kernel<<<grid_d_d, block_dim, 0, s>>>(
		d_grad_mid_float,
		static_cast<const __nv_bfloat16*>(gpu_activations->d_attention_output),
		d_grad_out_W,
		S, D, D);

	// 7. Backprop into Attention output: [S x D]
	batched_input_gradient_kernel<<<grid_mid, block_dim, 0, s>>>(
		d_grad_mid_float,
		static_cast<const float*>(gpu_weights->d_output_master),
		d_grad_attn_float,
		S, D, D);

	// Convert d_grad_attn_float to bf16
	int conv_mid_blocks = static_cast<int>((total_seq_dim + conv_threads - 1) / conv_threads);
	float_to_bf16_grad_kernel<<<conv_mid_blocks, conv_threads, 0, s>>>(
		d_grad_attn_float, d_grad_bf16_temp, total_seq_dim);

	// 8. Fused Causal Attention backward -> d_grad_q, d_grad_k, d_grad_v
	fused_causal_backward(d_grad_bf16_temp,
			      gpu_activations->d_query,
			      gpu_activations->d_key,
			      gpu_activations->d_value,
			      d_grad_q_float,
			      d_grad_k_float,
			      d_grad_v_float,
			      sequence_length,
			      embedding_dim,
			      num_heads,
			      s);

	// 9. Parameter gradients for W_Q, W_K, W_V: [D x D]
	batched_weight_gradient_kernel<<<grid_d_d, block_dim, 0, s>>>(
		d_grad_q_float,
		static_cast<const __nv_bfloat16*>(hidden_states),
		d_grad_q_W,
		S, D, D);
	batched_weight_gradient_kernel<<<grid_d_d, block_dim, 0, s>>>(
		d_grad_k_float,
		static_cast<const __nv_bfloat16*>(hidden_states),
		d_grad_k_W,
		S, D, D);
	batched_weight_gradient_kernel<<<grid_d_d, block_dim, 0, s>>>(
		d_grad_v_float,
		static_cast<const __nv_bfloat16*>(hidden_states),
		d_grad_v_W,
		S, D, D);

	// 10. Update all 7 transformer parameter matrices using Fused AdamW
	uint64_t step = gpu_weights->step_count;
	constexpr float beta1 = 0.9f;
	constexpr float beta2 = 0.999f;
	constexpr float eps = 1e-8f;
	constexpr float weight_decay = 0.01f;

	auto apply_adamw = [&](float* master, __nv_bfloat16* bf16_w, float* m, float* v,
			       const float* g, size_t elements) {
		int threads = 256;
		int blocks = static_cast<int>((elements + threads - 1) / threads);
		fused_matrix_adamw_kernel<<<blocks, threads, 0, s>>>(
			master, bf16_w, m, v, g, elements,
			learning_rate, step, beta1, beta2, eps, weight_decay);
	};

	apply_adamw(static_cast<float*>(gpu_weights->d_down_master),
		    static_cast<__nv_bfloat16*>(gpu_weights->d_down_weights),
		    static_cast<float*>(gpu_weights->d_down_m),
		    static_cast<float*>(gpu_weights->d_down_v),
		    d_grad_down_W, D * F);

	apply_adamw(static_cast<float*>(gpu_weights->d_gate_master),
		    static_cast<__nv_bfloat16*>(gpu_weights->d_gate_weights),
		    static_cast<float*>(gpu_weights->d_gate_m),
		    static_cast<float*>(gpu_weights->d_gate_v),
		    d_grad_gate_W, F * D);

	apply_adamw(static_cast<float*>(gpu_weights->d_up_master),
		    static_cast<__nv_bfloat16*>(gpu_weights->d_up_weights),
		    static_cast<float*>(gpu_weights->d_up_m),
		    static_cast<float*>(gpu_weights->d_up_v),
		    d_grad_up_W, F * D);

	apply_adamw(static_cast<float*>(gpu_weights->d_output_master),
		    static_cast<__nv_bfloat16*>(gpu_weights->d_output_weights),
		    static_cast<float*>(gpu_weights->d_output_m),
		    static_cast<float*>(gpu_weights->d_output_v),
		    d_grad_out_W, D * D);

	apply_adamw(static_cast<float*>(gpu_weights->d_query_master),
		    static_cast<__nv_bfloat16*>(gpu_weights->d_query_weights),
		    static_cast<float*>(gpu_weights->d_query_m),
		    static_cast<float*>(gpu_weights->d_query_v),
		    d_grad_q_W, D * D);

	apply_adamw(static_cast<float*>(gpu_weights->d_key_master),
		    static_cast<__nv_bfloat16*>(gpu_weights->d_key_weights),
		    static_cast<float*>(gpu_weights->d_key_m),
		    static_cast<float*>(gpu_weights->d_key_v),
		    d_grad_k_W, D * D);

	apply_adamw(static_cast<float*>(gpu_weights->d_value_master),
		    static_cast<__nv_bfloat16*>(gpu_weights->d_value_weights),
		    static_cast<float*>(gpu_weights->d_value_m),
		    static_cast<float*>(gpu_weights->d_value_v),
		    d_grad_v_W, D * D);

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
	free_vram(d_grad_attn_float, s);
	free_vram(d_grad_q_float, s);
	free_vram(d_grad_k_float, s);
	free_vram(d_grad_v_float, s);
	free_vram(d_grad_bf16_temp, s);

	return CUDA_CHECK(cudaGetLastError());
}
