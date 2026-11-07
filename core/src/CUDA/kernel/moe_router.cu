#include <cmath>
#include <cstdint>
#include <cuda_bf16.h>
#include <cuda_runtime.h>

#include "../util/caching_allocator.cuh"
#include "../util/cuda_utils.cuh"
#include "moe_router.cuh"

__device__ __forceinline__ float silu_activation(float x) {
	return x / (1.0f + expf(-x));
}

__global__ static void top2_router_kernel(const __nv_bfloat16* __restrict__ hidden_states,
					  const __nv_bfloat16* __restrict__ router_weights,
					  float* __restrict__ router_logits,
					  int32_t* __restrict__ expert_first,
					  int32_t* __restrict__ expert_second,
					  float* __restrict__ weight_first,
					  float* __restrict__ weight_second,
					  int32_t* __restrict__ expert_histogram,
					  int sequence_length,
					  int hidden_dim,
					  int num_experts) {
	int token_idx = blockIdx.x * blockDim.x + threadIdx.x;
	if (token_idx >= sequence_length)
		return;

	const __nv_bfloat16* x = hidden_states + token_idx * hidden_dim;
	float logits[16];
	int n = (num_experts > 16) ? 16 : num_experts;

	// 1. Compute dot product x . W_router[:, e]
	for (int e = 0; e < n; ++e) {
		float dot = 0.0f;
		for (int d = 0; d < hidden_dim; ++d) {
			dot += __bfloat162float(x[d]) *
			       __bfloat162float(router_weights[d * num_experts + e]);
		}
		logits[e] = dot;
		if (router_logits) {
			router_logits[token_idx * num_experts + e] = dot;
		}
	}

	// 2. Softmax over experts
	float max_logit = logits[0];
	for (int e = 1; e < n; ++e) {
		if (logits[e] > max_logit) {
			max_logit = logits[e];
		}
	}

	float sum_exp = 0.0f;
	float probs[16];
	for (int e = 0; e < n; ++e) {
		probs[e] = expf(logits[e] - max_logit);
		sum_exp += probs[e];
	}
	float inv_sum = 1.0f / (sum_exp + 1e-9f);
	for (int e = 0; e < n; ++e) {
		probs[e] *= inv_sum;
	}

	// 3. Find top-1 and top-2
	int top1 = 0;
	float prob1 = probs[0];
	for (int e = 1; e < n; ++e) {
		if (probs[e] > prob1) {
			prob1 = probs[e];
			top1 = e;
		}
	}

	int top2 = (top1 == 0) ? 1 : 0;
	float prob2 = probs[top2];
	for (int e = 0; e < n; ++e) {
		if (e != top1 && probs[e] > prob2) {
			prob2 = probs[e];
			top2 = e;
		}
	}

	// 4. Renormalize top-2 weights
	float sum_top2 = prob1 + prob2 + 1e-9f;
	float w1 = prob1 / sum_top2;
	float w2 = prob2 / sum_top2;

	expert_first[token_idx] = top1;
	expert_second[token_idx] = top2;
	weight_first[token_idx] = w1;
	weight_second[token_idx] = w2;

	if (expert_histogram) {
		atomicAdd(&expert_histogram[top1], 1);
		atomicAdd(&expert_histogram[top2], 1);
	}
}

__global__ static void moe_top2_dispatch_kernel(const __nv_bfloat16* __restrict__ hidden_states,
						const __nv_bfloat16* __restrict__ expert_gate,
						const __nv_bfloat16* __restrict__ expert_up,
						const __nv_bfloat16* __restrict__ expert_down,
						const int32_t* __restrict__ expert_first,
						const int32_t* __restrict__ expert_second,
						const float* __restrict__ weight_first,
						const float* __restrict__ weight_second,
						__nv_bfloat16* __restrict__ ffn_output,
						int sequence_length,
						int hidden_dim,
						int feedforward_network_dimension,
						int num_experts) {
	int token_idx = blockIdx.x;
	if (token_idx >= sequence_length)
		return;

	extern __shared__ float s_swiglu[]; // size: 2 * feedforward_network_dimension floats
	float* s_h1 = s_swiglu;
	float* s_h2 = s_swiglu + feedforward_network_dimension;

	int e1 = expert_first[token_idx];
	int e2 = expert_second[token_idx];
	float w1 = weight_first[token_idx];
	float w2 = weight_second[token_idx];

	const __nv_bfloat16* x_t = hidden_states + token_idx * hidden_dim;

	// Pointer offsets for expert e1
	size_t df_offset1 = static_cast<size_t>(e1) * (hidden_dim * feedforward_network_dimension);
	size_t fd_offset1 = static_cast<size_t>(e1) * (feedforward_network_dimension * hidden_dim);
	const __nv_bfloat16* gate1 = expert_gate + df_offset1;
	const __nv_bfloat16* up1 = expert_up + df_offset1;
	const __nv_bfloat16* down1 = expert_down + fd_offset1;

	// Pointer offsets for expert e2
	size_t df_offset2 = static_cast<size_t>(e2) * (hidden_dim * feedforward_network_dimension);
	size_t fd_offset2 = static_cast<size_t>(e2) * (feedforward_network_dimension * hidden_dim);
	const __nv_bfloat16* gate2 = expert_gate + df_offset2;
	const __nv_bfloat16* up2 = expert_up + df_offset2;
	const __nv_bfloat16* down2 = expert_down + fd_offset2;

	// Phase A: Compute SwiGLU intermediate values for e1 and e2
	for (int f = threadIdx.x; f < feedforward_network_dimension; f += blockDim.x) {
		float g1 = 0.0f, u1 = 0.0f;
		float g2 = 0.0f, u2 = 0.0f;

		for (int d = 0; d < hidden_dim; ++d) {
			float x_val = __bfloat162float(x_t[d]);
			g1 += x_val *
			      __bfloat162float(gate1[d * feedforward_network_dimension + f]);
			u1 += x_val * __bfloat162float(up1[d * feedforward_network_dimension + f]);

			g2 += x_val *
			      __bfloat162float(gate2[d * feedforward_network_dimension + f]);
			u2 += x_val * __bfloat162float(up2[d * feedforward_network_dimension + f]);
		}

		s_h1[f] = silu_activation(g1) * u1;
		s_h2[f] = silu_activation(g2) * u2;
	}

	__syncthreads();

	// Phase B: Down projection and weighted combination
	__nv_bfloat16* out_t = ffn_output + token_idx * hidden_dim;
	for (int d = threadIdx.x; d < hidden_dim; d += blockDim.x) {
		float out1 = 0.0f;
		float out2 = 0.0f;

		for (int f = 0; f < feedforward_network_dimension; ++f) {
			out1 += s_h1[f] * __bfloat162float(down1[f * hidden_dim + d]);
			out2 += s_h2[f] * __bfloat162float(down2[f * hidden_dim + d]);
		}

		float combined = w1 * out1 + w2 * out2;
		out_t[d] = __float2bfloat16(combined);
	}
}

__global__ static void moe_tensor_add_inplace_kernel(__nv_bfloat16* __restrict__ dst,
						     const __nv_bfloat16* __restrict__ src,
						     size_t total_elements) {

	size_t idx = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
	if (idx < total_elements) {
		float d = __bfloat162float(dst[idx]);
		float s = __bfloat162float(src[idx]);
		dst[idx] = __float2bfloat16(d + s);
	}
}

extern "C" CUDA_API bool forward_mixture_experts(void* hidden_states,
						 const GpuWeights* gpu_weights,
						 GpuActivations* gpu_activations,
						 size_t sequence_length,
						 size_t embedding_dim,
						 size_t feedforward_network_dimension,
						 void* stream) {
	if (!hidden_states || !gpu_weights || !gpu_activations || sequence_length == 0 ||
	    embedding_dim == 0 || feedforward_network_dimension == 0 ||
	    !gpu_weights->d_router_weights) {
		return false;
	}

	cudaStream_t s = (cudaStream_t)stream;
	size_t total_seq_dim = sequence_length * embedding_dim;
	size_t num_experts = gpu_weights->expert_count;

	// Allocate intermediate activations using BFC caching allocator
	if (!gpu_activations->d_router_logits) {
		gpu_activations->d_router_logits =
			allocate_vram(sequence_length * num_experts * sizeof(float), s);
		gpu_activations->d_expert_first =
			allocate_vram(sequence_length * sizeof(int32_t), s);
		gpu_activations->d_expert_second =
			allocate_vram(sequence_length * sizeof(int32_t), s);
		gpu_activations->d_weight_first = allocate_vram(sequence_length * sizeof(float), s);
		gpu_activations->d_weight_second =
			allocate_vram(sequence_length * sizeof(float), s);
		gpu_activations->d_expert_histogram =
			allocate_vram(num_experts * sizeof(int32_t), s);
		if (s) {
			cudaMemsetAsync(gpu_activations->d_expert_histogram,
					0,
					num_experts * sizeof(int32_t),
					s);
		} else {
			cudaMemset(gpu_activations->d_expert_histogram,
				   0,
				   num_experts * sizeof(int32_t));
		}
	}

	if (!gpu_activations->d_ffn_output) {
		gpu_activations->d_ffn_output =
			allocate_vram(total_seq_dim * sizeof(__nv_bfloat16), s);
	}
	if (!gpu_activations->d_residual_out) {
		gpu_activations->d_residual_out =
			allocate_vram(total_seq_dim * sizeof(__nv_bfloat16), s);
	}

	if (!gpu_activations->d_router_logits || !gpu_activations->d_expert_first ||
	    !gpu_activations->d_expert_second || !gpu_activations->d_weight_first ||
	    !gpu_activations->d_weight_second || !gpu_activations->d_expert_histogram ||
	    !gpu_activations->d_ffn_output || !gpu_activations->d_residual_out) {
		return false;
	}

	// 1. Save residual copy of input
	cudaMemcpyAsync(gpu_activations->d_residual_out,
			hidden_states,
			total_seq_dim * sizeof(__nv_bfloat16),
			cudaMemcpyDeviceToDevice,
			s);

	// 2. Route tokens to top-2 experts
	int router_threads = 128;
	int router_blocks =
		static_cast<int>((sequence_length + router_threads - 1) / router_threads);
	top2_router_kernel<<<router_blocks, router_threads, 0, s>>>(
		static_cast<const __nv_bfloat16*>(hidden_states),
		static_cast<const __nv_bfloat16*>(gpu_weights->d_router_weights),
		static_cast<float*>(gpu_activations->d_router_logits),
		static_cast<int32_t*>(gpu_activations->d_expert_first),
		static_cast<int32_t*>(gpu_activations->d_expert_second),
		static_cast<float*>(gpu_activations->d_weight_first),
		static_cast<float*>(gpu_activations->d_weight_second),
		static_cast<int32_t*>(gpu_activations->d_expert_histogram),
		static_cast<int>(sequence_length),
		static_cast<int>(embedding_dim),
		static_cast<int>(num_experts));

	// 3. Dispatch tokens exclusively to top 2 active experts
	size_t shared_mem_bytes = 2 * feedforward_network_dimension * sizeof(float);
	moe_top2_dispatch_kernel<<<static_cast<int>(sequence_length), 128, shared_mem_bytes, s>>>(
		static_cast<const __nv_bfloat16*>(hidden_states),
		static_cast<const __nv_bfloat16*>(gpu_weights->d_expert_gate),
		static_cast<const __nv_bfloat16*>(gpu_weights->d_expert_up),
		static_cast<const __nv_bfloat16*>(gpu_weights->d_expert_down),
		static_cast<const int32_t*>(gpu_activations->d_expert_first),
		static_cast<const int32_t*>(gpu_activations->d_expert_second),
		static_cast<const float*>(gpu_activations->d_weight_first),
		static_cast<const float*>(gpu_activations->d_weight_second),
		static_cast<__nv_bfloat16*>(gpu_activations->d_ffn_output),
		static_cast<int>(sequence_length),
		static_cast<int>(embedding_dim),
		static_cast<int>(feedforward_network_dimension),
		static_cast<int>(num_experts));

	// 4. Final Residual Stream: X_out = X_mid + MoE_out
	int add_threads = 256;
	int add_blocks = static_cast<int>((total_seq_dim + add_threads - 1) / add_threads);
	moe_tensor_add_inplace_kernel<<<add_blocks, add_threads, 0, s>>>(
		static_cast<__nv_bfloat16*>(gpu_activations->d_residual_out),
		static_cast<const __nv_bfloat16*>(gpu_activations->d_ffn_output),
		total_seq_dim);

	// Write X_out back to hidden_states
	cudaMemcpyAsync(hidden_states,
			gpu_activations->d_residual_out,
			total_seq_dim * sizeof(__nv_bfloat16),
			cudaMemcpyDeviceToDevice,
			s);

	return CUDA_CHECK(cudaGetLastError());
}

extern "C" CUDA_API bool download_expert_counts(const GpuActivations* gpu_activations,
						int32_t* h_histogram,
						size_t num_experts) {
	if (!gpu_activations || !gpu_activations->d_expert_histogram || !h_histogram ||
	    num_experts == 0) {
		return false;
	}
	return CUDA_CHECK(cudaMemcpy(h_histogram,
				     gpu_activations->d_expert_histogram,
				     num_experts * sizeof(int32_t),
				     cudaMemcpyDeviceToHost));
}

extern "C" CUDA_API bool reset_expert_histogram(GpuActivations* gpu_activations,
						size_t num_experts,
						void* stream) {
	if (!gpu_activations || !gpu_activations->d_expert_histogram || num_experts == 0) {
		return false;
	}
	cudaStream_t s = (cudaStream_t)stream;
	if (s) {
		return CUDA_CHECK(cudaMemsetAsync(
			gpu_activations->d_expert_histogram, 0, num_experts * sizeof(int32_t), s));
	} else {
		return CUDA_CHECK(cudaMemset(
			gpu_activations->d_expert_histogram, 0, num_experts * sizeof(int32_t)));
	}
}
