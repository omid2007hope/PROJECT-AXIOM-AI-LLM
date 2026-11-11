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

#ifndef BACKWARD_PASS_CUH
#define BACKWARD_PASS_CUH

#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API bool backward_pass(const GpuPortion* gpu_portion,
			    GpuWeights* gpu_weights,
			    const GpuActivations* gpu_activations,
			    size_t input_dim,
			    size_t output_dim,
			    float learning_rate,
			    int buffer_index);

CUDA_API bool backward_pass_with_embeddings(const GpuPortion* gpu_portion,
					    GpuEmbeddings* gpu_embeddings,
					    GpuWeights* gpu_weights,
					    const GpuActivations* gpu_activations,
					    size_t input_dim,
					    size_t output_dim,
					    float learning_rate,
					    int buffer_index,
					    bool use_adamw,
					    bool scatter_embeddings = true);

CUDA_API bool backward_scatter_embeddings(const GpuPortion* gpu_portion,
					  GpuEmbeddings* gpu_embeddings,
					  const void* d_dx,
					  size_t num_tokens,
					  size_t embedding_dim,
					  float learning_rate,
					  int buffer_index,
					  void* stream);

// Phase 3: Multi-Head Attention, SwiGLU & Transformer Backward Block
CUDA_API bool fused_causal_backward(const void* grad_output,
				    const void* query,
				    const void* key,
				    const void* value,
				    void* grad_query,
				    void* grad_key,
				    void* grad_value,
				    size_t sequence_length,
				    size_t embedding_dim,
				    size_t num_heads,
				    void* stream);

CUDA_API bool swiglu_backward(const void* grad_output,
			      const void* gate,
			      const void* up,
			      void* grad_gate,
			      void* grad_up,
			      size_t total_elements,
			      void* stream);

CUDA_API bool transformer_backward_block(const void* grad_output,
					 void* grad_input,
					 const void* hidden_states,
					 GpuWeights* gpu_weights,
					 const GpuActivations* gpu_activations,
					 size_t sequence_length,
					 size_t embedding_dim,
					 size_t num_heads,
					 size_t feedforward_network_dimension,
					 float learning_rate,
					 void* stream);

#ifdef __cplusplus
}
#endif

#endif // BACKWARD_PASS_CUH
