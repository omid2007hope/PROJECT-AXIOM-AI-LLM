#ifndef GPU_TYPES_CUH
#define GPU_TYPES_CUH

#include <cstddef>
#include <cstdint>

#include "gpu_kernel.cuh"

#ifdef __cplusplus
extern "C" {
#endif

// GPU-side mirror of the context window and target that live in Portion.
// All pointers are device memory — do NOT dereference on the CPU.
// Uses void* to hide __nv_bfloat16 from the CPU compiler.
struct GpuPortion {
	void* d_token_ids[2] = {nullptr,
				nullptr}; // device ptr: uint32_t (128 token IDs across PCIe)
	void* d_context_window[2] = {nullptr, nullptr}; // device ptr: X  (128 tokens * dim)
	void* d_target[2] = {nullptr, nullptr};		// device ptr: Y  (1  token  * dim)
	void* d_dx[2] = {nullptr, nullptr}; // device ptr: float (dL/dx for input backprop)

	size_t context_ptr = sizeof(d_context_window) / sizeof(d_context_window[0]);
	size_t target_ptr = sizeof(d_target) / sizeof(d_target[0]);

	void* stream[2] = {nullptr, nullptr};

	// cudaEvent_t recorded when the backward pass of a buffer finishes. The next
	// step's forward pass waits on it, so the shared weights are updated strictly
	// in order while the PCIe uploads of the two buffers still overlap.
	void* compute_done[2] = {nullptr, nullptr};

	// true only while d_token_ids[i] holds the IDs of the step currently uploaded to buffer i
	bool token_ids_active[2] = {false, false};

	size_t token_count = 0;	     // total token count in context window (e.g. 128)
	size_t context_elements = 0; // total bfloat16 elements in X
	size_t target_elements = 0;  // total bfloat16 elements in Y
};

// GPU-side mirror of the learnable embedding matrix.
struct GpuEmbeddings {
	void* d_table = nullptr;	// device ptr to __nv_bfloat16 (vocab_size * embedding_dim)
	void* d_table_master = nullptr; // device ptr to float (vocab_size * embedding_dim)
	size_t vocab_size = 65536;
	size_t embedding_dim = 32;
	size_t active_vocab_size =
		0; // Milestone 3: Dynamic bound for active vocabulary in tied logits/softmax

	// AdamW first moment (m_t) and second moment (v_t) tensors
	void* d_m = nullptr; // device ptr to float (vocab_size * embedding_dim)
	void* d_v = nullptr; // device ptr to float (vocab_size * embedding_dim)

	float temperature = 1.0;
};


// GPU-side mirror of the AI weights and biases.
struct GpuWeights {
	void* d_weights = nullptr; // device ptr to __nv_bfloat16 (read by the Tensor Cores)
	void* d_bias = nullptr;	   // device ptr to __nv_bfloat16

	// FP32 master copies: the optimizer updates these and rounds the result into
	// d_weights / d_bias. A bf16-only update would lose steps smaller than its
	// 8-bit mantissa, which stalls learning.
	void* d_weights_master = nullptr; // device ptr to float
	void* d_bias_master = nullptr;	  // device ptr to float

	// AdamW first and second moment tensors for weights and biases
	void* d_weights_m = nullptr; // device ptr to float (weight_elements)
	void* d_weights_v = nullptr; // device ptr to float (weight_elements)
	void* d_bias_m = nullptr;    // device ptr to float (bias_elements)
	void* d_bias_v = nullptr;    // device ptr to float (bias_elements)

	size_t weight_elements = 0;
	size_t bias_elements = 0;
	uint64_t step_count = 0;

	// Phase 3: Transformer Multi-Head Attention & SwiGLU FFN Weights
	void* d_query_weights = nullptr;  // device ptr to __nv_bfloat16 (dim * dim)
	void* d_key_weights = nullptr;    // device ptr to __nv_bfloat16 (dim * dim)
	void* d_value_weights = nullptr;  // device ptr to __nv_bfloat16 (dim * dim)
	void* d_output_weights = nullptr; // device ptr to __nv_bfloat16 (dim * dim)

	void* d_gate_weights = nullptr; // device ptr to __nv_bfloat16 (dim * ffn_dim)
	void* d_up_weights = nullptr;   // device ptr to __nv_bfloat16 (dim * ffn_dim)
	void* d_down_weights = nullptr; // device ptr to __nv_bfloat16 (ffn_dim * dim)

	// FP32 Master copies for transformer weights
	void* d_query_master = nullptr;
	void* d_key_master = nullptr;
	void* d_value_master = nullptr;
	void* d_output_master = nullptr;
	void* d_gate_master = nullptr;
	void* d_up_master = nullptr;
	void* d_down_master = nullptr;

	// AdamW moment buffers for transformer weights
	void* d_query_m = nullptr;
	void* d_query_v = nullptr;
	void* d_key_m = nullptr;
	void* d_key_v = nullptr;
	void* d_value_m = nullptr;
	void* d_value_v = nullptr;
	void* d_output_m = nullptr;
	void* d_output_v = nullptr;
	void* d_gate_m = nullptr;
	void* d_gate_v = nullptr;
	void* d_up_m = nullptr;
	void* d_up_v = nullptr;
	void* d_down_m = nullptr;
	void* d_down_v = nullptr;

	size_t transformer_dim = 0;
	size_t transformer_ffn = 0;
};

// GPU-side activations buffer
struct GpuActivations {
	void* d_output = nullptr; // device ptr to __nv_bfloat16
	size_t output_elements = 0;

	// device ptr to float: running sum of loss over the steps since the
	// last download_loss(). Measured before each update, so it is the loss on a
	// sample the network has not trained on yet.
	void* d_loss = nullptr;

	// Milestone 3: Discrete Autoregressive Token Classification
	void* d_inv_rms = nullptr; // device ptr to float (RMS normalization factor)
	void* d_probs = nullptr;   // device ptr to float (Softmax probability distribution)
	size_t vocab_elements = 0; // capacity of d_probs
	void* d_pred_id = nullptr; // device ptr to uint32_t (GPU Argmax winning token ID)
	void* d_dz = nullptr;	   // device ptr to float (loss gradient dz across output neurons)

	// Phase 2: Non-blocking loss telemetry
	void* h_pinned_loss = nullptr; // pinned host float slot
	void* loss_event = nullptr;    // cudaEvent_t for async telemetry
	bool loss_pending = false;     // async transfer in flight flag

	// Phase 3: Transformer Block Intermediate Activations
	void* d_query = nullptr;               // device ptr to __nv_bfloat16 (seq * dim)
	void* d_key = nullptr;                 // device ptr to __nv_bfloat16 (seq * dim)
	void* d_value = nullptr;               // device ptr to __nv_bfloat16 (seq * dim)
	void* d_attention_output = nullptr;    // device ptr to __nv_bfloat16 (seq * dim)
	void* d_intermediate_gate = nullptr;   // device ptr to __nv_bfloat16 (seq * ffn_dim)
	void* d_intermediate_up = nullptr;     // device ptr to __nv_bfloat16 (seq * ffn_dim)
	void* d_intermediate_swiglu = nullptr; // device ptr to __nv_bfloat16 (seq * ffn_dim)
	void* d_ffn_output = nullptr;          // device ptr to __nv_bfloat16 (seq * dim)
	void* d_residual_mid = nullptr;        // device ptr to __nv_bfloat16 (seq * dim)
	void* d_residual_out = nullptr;        // device ptr to __nv_bfloat16 (seq * dim)
};

// Phase 4: PagedAttention KV-Cache Physical Page Pool
struct GpuPagedCache {
	void* d_key_blocks = nullptr;   // device ptr to __nv_bfloat16 (total_blocks * block_size * hidden_dim)
	void* d_value_blocks = nullptr; // device ptr to __nv_bfloat16 (total_blocks * block_size * hidden_dim)
	void* d_block_table = nullptr;  // device ptr to int (staged sequence block table)
	size_t total_blocks = 0;
	size_t block_size = 16;
	size_t hidden_dimension = 32;
	size_t num_heads = 4;
};

#ifdef BUILDING_CUDA_DLL
#define CUDA_API __declspec(dllexport)
#else
#define CUDA_API __declspec(dllimport)
#endif

#ifdef __cplusplus
}
#endif

#endif // GPU_TYPES_CUH
