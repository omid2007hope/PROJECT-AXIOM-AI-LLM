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
					    bool use_adamw);

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
					 size_t ffn_dim,
					 float learning_rate,
					 void* stream);

#ifdef __cplusplus
}
#endif

#endif // BACKWARD_PASS_CUH
