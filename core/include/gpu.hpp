#ifndef include_gpu
#define include_gpu

#include <float.h>
#include <memory>

#include "../include/ai.hpp"
#include "../include/optimizer.hpp"
#include "../include/parser.hpp"
#include "../include/training.hpp"
#include "../src/CUDA/kernel/forward_pass.cuh"
#include "../src/CUDA/kernel/paged_gqa.cuh"
#include "../src/CUDA/kernel/rmsnorm.cuh"
#include "../src/CUDA/kernel/rope.cuh"
#include "../src/CUDA/kernel/swiglu.cuh"
#include "../src/CUDA/transfer.cuh"
#include "../src/CUDA/util/memory_allocation.cuh"
#include "paged_cache.hpp"


// GPU Memory State (The pointers for the VRAM)
GpuWeights gpu_weights;
GpuActivations gpu_activations;
GpuPortion gpu_portion; // Reused across


// iterations to prevent reallocation
GpuEmbeddings gpu_embeddings;
GpuPagedCache gpu_paged_cache;
std::unique_ptr<PagedBlockManager> block_manager;
GpuGraphExecution gpu_graph_execution;

class GPU {
public:
	// Method to move memory to GPU
	bool upload_weights();

	// Method to copy the trained weights back into host_weights / host_bias
	bool download_weights();
};

#endif // include_gpu
