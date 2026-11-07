#ifndef MOE_ROUTER_CUH
#define MOE_ROUTER_CUH

#ifdef __CUDACC__
#include <cuda_bf16.h>
#include <cuda_runtime.h>
#endif
#include <cstddef>
#include <cstdint>
#include "../util/gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API bool forward_mixture_experts(void* hidden_states,
				      const GpuWeights* gpu_weights,
				      GpuActivations* gpu_activations,
				      size_t sequence_length,
				      size_t embedding_dim,
				      size_t ffn_dim,
				      void* stream);

CUDA_API bool download_expert_counts(const GpuActivations* gpu_activations,
				     int32_t* h_histogram,
				     size_t num_experts);

CUDA_API bool reset_expert_histogram(GpuActivations* gpu_activations,
				     size_t num_experts,
				     void* stream);

#ifdef __cplusplus
}
#endif

#endif // MOE_ROUTER_CUH

