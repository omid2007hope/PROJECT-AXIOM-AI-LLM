/*
Isolate the forward and backward pass - feed then fake false emmbeding data - check the out put live - loss drop per 1000 step - if drops - no abnormality - anomalies - pass green - then we find that forward pass and backward pass in isolation work perfectly which makes the question that why when compile the full app the loss is not okay!
*/

#include <vector>

#include "../../../src/CUDA/kernel/backward_pass.cu"
#include "../../../src/CUDA/kernel/forward_pass.cu"
#include "../../src/CUDA/util/gpu_types.cuh"



struct TestGpuPortion {
	void* d_token_ids[2] = {nullptr, nullptr};
	void* d_context_window[2] = {nullptr, nullptr};
	void* d_target[2] = {nullptr, nullptr};
	void* d_dx[2] = {nullptr, nullptr};

	size_t context_ptr = sizeof(d_context_window) / sizeof(d_context_window[0]);
	size_t target_ptr = sizeof(d_target) / sizeof(d_target[0]);

	void* stream[2] = {nullptr, nullptr};

	void* compute_done[2] = {nullptr, nullptr};
	bool compute_done_recorded[2] = {false, false};

	bool token_ids_active[2] = {false, false};

	size_t token_count = 0;
	size_t context_elements = 0;
	size_t target_elements = 0;

	void* d_hidden_state[2] = {nullptr, nullptr};
	void* d_loss_scalar[2] = {nullptr, nullptr};
	float* h_pinned_loss[2] = {nullptr, nullptr};
	void* loss_ready_event[2] = {nullptr, nullptr};

	size_t max_batch_tokens = 0;
	bool is_allocated = false;
};

struct TestGpuEmbeddings {
	void* d_table = nullptr;
	void* d_table_master = nullptr;
	size_t vocab_size = 0;
	size_t embedding_dim = 0;
	size_t active_vocab_size = 0;

	void* d_m = nullptr;
	void* d_v = nullptr;

	float temperature = 1.0f;
};

struct TestGpuWeights {
	void* d_weights = nullptr;
	void* d_bias = nullptr;

	void* d_weights_master = nullptr;
	void* d_bias_master = nullptr;

	void* d_weights_m = nullptr;
	void* d_weights_v = nullptr;
	void* d_bias_m = nullptr;
	void* d_bias_v = nullptr;

	size_t weight_elements = 0;
	size_t bias_elements = 0;
	uint64_t step_count = 0;

	void* d_query_weights = nullptr;
	void* d_key_weights = nullptr;
	void* d_value_weights = nullptr;
	void* d_output_weights = nullptr;

	void* d_gate_weights = nullptr;
	void* d_up_weights = nullptr;
	void* d_down_weights = nullptr;

	void* d_query_master = nullptr;
	void* d_key_master = nullptr;
	void* d_value_master = nullptr;
	void* d_output_master = nullptr;
	void* d_gate_master = nullptr;
	void* d_up_master = nullptr;
	void* d_down_master = nullptr;

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

	bool enable_moe = false;
	size_t expert_count = 0;
	void* d_router_weights = nullptr;
	void* d_router_master = nullptr;
	void* d_router_m = nullptr;
	void* d_router_v = nullptr;

	void* d_expert_gate = nullptr;
	void* d_expert_up = nullptr;
	void* d_expert_down = nullptr;

	void* d_expert_gate_master = nullptr;
	void* d_expert_up_master = nullptr;
	void* d_expert_down_master = nullptr;

	void* d_expert_gate_m = nullptr;
	void* d_expert_gate_v = nullptr;
	void* d_expert_up_m = nullptr;
	void* d_expert_up_v = nullptr;
	void* d_expert_down_m = nullptr;
	void* d_expert_down_v = nullptr;
};

struct TestGpuActivations {
	void* d_output = nullptr;
	size_t output_elements = 0;

	void* d_loss = nullptr;

	void* d_inv_rms = nullptr;
	void* d_probs = nullptr;
	size_t vocab_elements = 0;
	void* d_pred_id = nullptr;
	void* d_dz = nullptr;

	void* h_pinned_loss = nullptr;
	void* loss_event = nullptr;
	bool loss_pending = false;

	void* d_query = nullptr;
	void* d_key = nullptr;
	void* d_value = nullptr;
	void* d_attention_output = nullptr;
	void* d_intermediate_gate = nullptr;
	void* d_intermediate_up = nullptr;
	void* d_intermediate_swiglu = nullptr;
	void* d_ffn_output = nullptr;
	void* d_residual_mid = nullptr;
	void* d_residual_out = nullptr;
	void* d_normalized_attention = nullptr;
	void* d_normalized_feedforward = nullptr;
	void* d_attention_scale = nullptr;
	void* d_feedforward_scale = nullptr;
	size_t xform_seq_elements = 0;
	size_t xform_ffn_elements = 0;

	void* d_router_logits = nullptr;
	void* d_expert_first = nullptr;
	void* d_expert_second = nullptr;
	void* d_weight_first = nullptr;
	void* d_weight_second = nullptr;
	void* d_expert_histogram = nullptr;
};

// Const test struct instances
const TestGpuPortion test_gpu_portion{};
const TestGpuEmbeddings test_gpu_embeddings{};
const TestGpuWeights test_gpu_weights{};
const TestGpuActivations test_gpu_activations{};

// Dimension & indexing parameters
const size_t test_input_dim = 0;
const size_t test_output_dim = 0;
const int test_buffer_index = 0;
const size_t test_sequence_length = 0;
const size_t test_embedding_dim = 0;
const size_t test_num_heads = 0;
const size_t test_feedforward_network_dimension = 0;
const size_t test_total_elements = 0;
const size_t test_num_tokens = 0;
const size_t test_n = 0;
const uint32_t test_batch_tokens = 0;
const uint32_t test_hidden_dim = 0;
const uint32_t test_vocab_size = 0;

// GEMM & benchmark parameters
const int test_M = 0;
const int test_N = 0;
const int test_K = 0;
const int test_iterations = 0;
const int test_rows_a = 0;
const int test_cols_b = 0;
const int test_common_dim = 0;
const size_t test_safety_headroom_bytes = 0;
double* const test_out_tflops = nullptr;
double* const test_out_bandwidth_gb_s = nullptr;
float* const test_out_avg_ms = nullptr;
size_t* const test_out_allocated_bytes = nullptr;
float* const test_out_elapsed_ms = nullptr;

// Control & optimizer flags / scalars
const bool test_gather_embeddings = true;
const bool test_transpose_a = false;
const bool test_transpose_b = false;
const bool test_use_adamw = false;
const bool test_scatter_embeddings = true;
const float test_learning_rate = 0.0f;

// Device buffer & stream pointers
void* const test_stream = nullptr;
const void* const test_matrix_a = nullptr;
const void* const test_matrix_b = nullptr;
void* const test_matrix_c = nullptr;
const void* const test_query = nullptr;
const void* const test_key = nullptr;
const void* const test_value = nullptr;
void* const test_output = nullptr;
const void* const test_gate = nullptr;
const void* const test_up = nullptr;
void* const test_hidden_states = nullptr;
const uint32_t* const test_d_token_ids = nullptr;
const void* const test_d_table = nullptr;
void* const test_d_output = nullptr;
const void* const test_d_x = nullptr;
const void* const test_d_w = nullptr;
const void* const test_d_b = nullptr;
void* const test_d_y = nullptr;
const void* const test_d_normed_x = nullptr;
const void* const test_d_lm_head_weight = nullptr;
float* const test_d_out_logits = nullptr;
float* const test_d_logits_and_probs = nullptr;
uint32_t* const test_d_pred_id = nullptr;
const void* const test_d_dx = nullptr;
const void* const test_grad_output = nullptr;
void* const test_grad_query = nullptr;
void* const test_grad_key = nullptr;
void* const test_grad_value = nullptr;
void* const test_grad_gate = nullptr;
void* const test_grad_up = nullptr;
void* const test_grad_input = nullptr;



static bool test_forward() {

	CUDA_API bool forward_pass(const GpuPortion* gpu_portion, const GpuWeights* gpu_weights,
							   GpuActivations* gpu_activations, size_t input_dim, size_t output_dim,
							   int buffer_index);

	CUDA_API bool forward_pass_with_embeddings(
		const GpuPortion* gpu_portion, const GpuEmbeddings* gpu_embeddings,
		const GpuWeights* gpu_weights, GpuActivations* gpu_activations, size_t input_dim,
		size_t output_dim, int buffer_index, bool gather_embeddings = true);

	CUDA_API bool run_cutlass_saturation_benchmark(int M, int N, int K, int iterations,
												   double* out_tflops, double* out_bandwidth_gb_s,
												   float* out_avg_ms);

	CUDA_API bool run_vram_saturation_benchmark(size_t safety_headroom_bytes,
												size_t* out_allocated_bytes,
												double* out_bandwidth_gb_s, float* out_elapsed_ms);

	// Phase 3: Multi-Head Attention, SwiGLU & Transformer Forward Block
	CUDA_API bool batched_gemm_multiply(const void* matrix_a, const void* matrix_b, void* matrix_c,
										int rows_a, int cols_b, int common_dim, bool transpose_a,
										bool transpose_b, void* stream);

	CUDA_API bool fused_causal_attention(const void* query, const void* key, const void* value,
										 void* output, size_t sequence_length, size_t embedding_dim,
										 size_t num_heads, void* stream);

	CUDA_API bool swiglu_forward(const void* gate, const void* up, void* output,
								 size_t total_elements, void* stream);

	CUDA_API bool transformer_forward_block(void* hidden_states, const GpuWeights* gpu_weights,
											GpuActivations* gpu_activations, size_t sequence_length,
											size_t embedding_dim, size_t num_heads,
											size_t feedforward_network_dimension, void* stream);


	CUDA_API void launch_gather_embeddings_batched(
		const uint32_t* d_token_ids, const void* d_table, void* d_output, uint32_t batch_tokens,
		uint32_t hidden_dim, uint32_t vocab_size, void* stream);

	CUDA_API void launch_cutlass_gemm_bf16(int M, int N, int K, const void* d_x, const void* d_w,
										   const void* d_b, void* d_y, void* stream);

	CUDA_API void launch_tied_logits_bf16(const void* d_normed_x, const void* d_lm_head_weight,
										  float* d_out_logits, int M, int vocab_size,
										  int hidden_dim, void* stream);

	CUDA_API void launch_softmax_argmax(float* d_logits_and_probs, uint32_t* d_pred_id, size_t n,
										void* stream);
};

static bool test_backward() {
	CUDA_API bool backward_pass(const GpuPortion* gpu_portion, GpuWeights* gpu_weights,
								const GpuActivations* gpu_activations, size_t input_dim,
								size_t output_dim, float learning_rate, int buffer_index);

	CUDA_API bool backward_pass_with_embeddings(
		const GpuPortion* gpu_portion, GpuEmbeddings* gpu_embeddings, GpuWeights* gpu_weights,
		const GpuActivations* gpu_activations, size_t input_dim, size_t output_dim,
		float learning_rate, int buffer_index, bool use_adamw, bool scatter_embeddings = true);

	CUDA_API bool backward_scatter_embeddings(const GpuPortion* gpu_portion,
											  GpuEmbeddings* gpu_embeddings, const void* d_dx,
											  size_t num_tokens, size_t embedding_dim,
											  float learning_rate, int buffer_index, void* stream);

	// Phase 3: Multi-Head Attention, SwiGLU & Transformer Backward Block
	CUDA_API bool fused_causal_backward(const void* grad_output, const void* query, const void* key,
										const void* value, void* grad_query, void* grad_key,
										void* grad_value, size_t sequence_length,
										size_t embedding_dim, size_t num_heads, void* stream);

	CUDA_API bool swiglu_backward(const void* grad_output, const void* gate, const void* up,
								  void* grad_gate, void* grad_up, size_t total_elements,
								  void* stream);

	CUDA_API bool transformer_backward_block(
		const void* grad_output, void* grad_input, const void* hidden_states,
		GpuWeights* gpu_weights, const GpuActivations* gpu_activations, size_t sequence_length,
		size_t embedding_dim, size_t num_heads, size_t feedforward_network_dimension,
		float learning_rate, void* stream);
};

static bool pass() {
	test_forward();
	test_backward();
};