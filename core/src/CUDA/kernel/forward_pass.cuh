#ifndef FORWARD_PASS_CUH
#define FORWARD_PASS_CUH

#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API bool forward_pass(const GpuPortion* gpu_portion,
			   const GpuWeights* gpu_weights,
			   GpuActivations* gpu_activations,
			   size_t input_dim,
			   size_t output_dim,
			   int buffer_index);

CUDA_API bool forward_pass_with_embeddings(const GpuPortion* gpu_portion,
					   const GpuEmbeddings* gpu_embeddings,
					   const GpuWeights* gpu_weights,
					   GpuActivations* gpu_activations,
					   size_t input_dim,
					   size_t output_dim,
					   int buffer_index);

CUDA_API bool run_cutlass_saturation_benchmark(int M,
					       int N,
					       int K,
					       int iterations,
					       double* out_tflops,
					       double* out_bandwidth_gb_s,
					       float* out_avg_ms);

CUDA_API bool run_vram_saturation_benchmark(size_t safety_headroom_bytes,
					    size_t* out_allocated_bytes,
					    double* out_bandwidth_gb_s,
					    float* out_elapsed_ms);

// Phase 3: Multi-Head Attention, SwiGLU & Transformer Forward Block
CUDA_API bool batched_gemm_multiply(const void* matrix_a,
				    const void* matrix_b,
				    void* matrix_c,
				    int rows_a,
				    int cols_b,
				    int common_dim,
				    bool transpose_a,
				    bool transpose_b,
				    void* stream);

CUDA_API bool fused_causal_attention(const void* query,
				     const void* key,
				     const void* value,
				     void* output,
				     size_t sequence_length,
				     size_t embedding_dim,
				     size_t num_heads,
				     void* stream);

CUDA_API bool swiglu_forward(const void* gate,
			     const void* up,
			     void* output,
			     size_t total_elements,
			     void* stream);

CUDA_API bool transformer_forward_block(void* hidden_states,
					const GpuWeights* gpu_weights,
					GpuActivations* gpu_activations,
					size_t sequence_length,
					size_t embedding_dim,
					size_t num_heads,
					size_t ffn_dim,
					void* stream);

#ifdef __cplusplus
}
#endif

#endif // FORWARD_PASS_CUH
