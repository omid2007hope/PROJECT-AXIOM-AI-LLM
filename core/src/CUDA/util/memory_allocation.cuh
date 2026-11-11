/*
 * Project Axiom — Built from scratch. Engineered for the future.
 * Copyright (c) 2026 Omid Teimory. All rights reserved.
 */

#ifndef MEMORY_ALLOCATION_CUH
#define MEMORY_ALLOCATION_CUH

#include "gpu_types.cuh"

#ifdef __cplusplus
extern "C" {
#endif

CUDA_API void free_gpu(GpuPortion* gpu);
CUDA_API void free_gpu_weights(GpuWeights* gpu);
CUDA_API void free_gpu_activations(GpuActivations* gpu);
CUDA_API bool allocate_gpu_embeddings(GpuEmbeddings* gpu_emb,
				      size_t vocab_size,
				      size_t embedding_dim);
CUDA_API void free_gpu_embeddings(GpuEmbeddings* gpu_emb);
CUDA_API bool upload_embeddings(const float* h_embeddings, GpuEmbeddings* gpu_emb);
CUDA_API bool download_embeddings(float* h_embeddings, const GpuEmbeddings* gpu_emb);

CUDA_API void* pinned_alloc(size_t bytes);
CUDA_API void pinned_free(void* ptr);

CUDA_API void* allocate_device_memory(size_t bytes);
CUDA_API void free_device_memory(void* ptr);
CUDA_API bool upload_device_buffer(void* dst, const void* src, size_t bytes);
CUDA_API bool download_device_buffer(void* dst, const void* src, size_t bytes);
CUDA_API bool register_host_memory(void* ptr, size_t bytes);
CUDA_API void unregister_host_memory(void* ptr);
CUDA_API void* create_cuda_stream();
CUDA_API void destroy_cuda_stream(void* stream);
CUDA_API void sync_cuda_stream(void* stream);
CUDA_API bool upload_device_buffer_async(void* dst, const void* src, size_t bytes, void* stream);
CUDA_API bool download_device_buffer_async(void* dst, const void* src, size_t bytes, void* stream);
CUDA_API bool allocate_transformer_weights(GpuWeights* gpu,
					   size_t hidden_dim,
					   size_t feedforward_network_dimension);
CUDA_API bool allocate_expert_weights(GpuWeights* gpu,
				      size_t hidden_dim,
				      size_t feedforward_network_dimension,
				      size_t num_experts);
CUDA_API void free_expert_weights(GpuWeights* gpu);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_ALLOCATION_CUH
