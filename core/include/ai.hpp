#ifndef INCLUDE_AI_HPP
#define INCLUDE_AI_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <stdfloat>
#include <string>
#include <vector>

#include "../include/config.hpp"
#include "../include/cpu.hpp"
#include "../include/gpu.hpp"

struct TransformerLayerWeights {
	const void* d_input_layernorm = nullptr;
	const void* d_q_proj_weight = nullptr;
	const void* d_q_proj_bias = nullptr;
	const void* d_k_proj_weight = nullptr;
	const void* d_k_proj_bias = nullptr;
	const void* d_v_proj_weight = nullptr;
	const void* d_v_proj_bias = nullptr;
	const void* d_o_proj_weight = nullptr;
	const void* d_post_attn_layernorm = nullptr;
	const void* d_gate_proj_weight = nullptr;
	const void* d_up_proj_weight = nullptr;
	const void* d_down_proj_weight = nullptr;
};

class Parser;
struct Input;

class AI {
public:
	AI(const AI&) = delete;

	AI& operator=(const AI&) = delete;


	// Default constructor for standalone inference/serving
	AI();

	// Constructor: Takes dimensions and initializes with Chaos (Randomness)
	// inside [weight_min, weight_max]
	AI(size_t input_elements_x_total,
	   size_t target_elements_y_total,
	   float weight_min,
	   float weight_max,
	   Elements& elements);

	~AI();

	// Method to move memory to GPU
	bool upload_weights();

	// Method to copy the trained weights back into host_weights / host_bias
	bool download_weights();

	// Method to reset all parameters/weights back to chaotic state
	void reset_weights(Elements& elements);


	bool load_weights(const std::string& path = "weights.axiom");

	// Forward Inference Engine (no gradient updates)
	bool predict(const std::vector<std::bfloat16_t>& context_window,
		     std::vector<float>& output_prediction);

	// Forward Inference from token IDs (right-aligned, PAD = 0): the GPU gathers the
	// trained embeddings, so inference sees the same inputs as training.
	bool predict_tokens(const std::vector<uint32_t>& context_ids,
			    std::vector<float>& output_prediction);

	// Autoregressive generation loop with real-time streaming
	bool generate(const std::string& prompt,
		      Parser& parser,
		      Input& input,
		      size_t max_new_tokens = 128,
		      const SamplingConfig& config = {},
		      std::function<void(const std::string&)> token_callback = nullptr);

	// Phase 3: Transformer Block Construction and Execution
	bool build_transformer_block(size_t hidden_dim, size_t heads = 4, size_t ffn_dim = 64);

	bool forward_transformer(std::vector<std::bfloat16_t>& hidden_states,
				 size_t sequence_length);

	bool backward_transformer(const std::vector<float>& grad_output,
				  std::vector<float>& grad_input,
				  const std::vector<std::bfloat16_t>& hidden_states,
				  size_t sequence_length,
				  float learning_rate);

	// Phase 4: PagedAttention KV-Cache & Block Management
	bool build_paged_cache(size_t total_blocks = 256, size_t block_size = 16);
	void free_paged_cache_memory();
	bool fork_sequence_context(const SequenceContext& parent, SequenceContext& child);
	bool append_sequence_token(SequenceContext& sequence,
				   const std::vector<std::bfloat16_t>& key,
				   const std::vector<std::bfloat16_t>& value);
	void release_sequence_context(SequenceContext& sequence);
	bool forward_paged_attention(const SequenceContext& sequence,
				     const std::vector<std::bfloat16_t>& query,
				     std::vector<std::bfloat16_t>& output,
				     size_t query_length);

	// Phase 5: Zero-Overhead Hardware Saturation via CUDA Graphs
	bool build_training_graph(Optimizer& optimizer, Portion& portion);
	bool launch_training_graph_step();
	void free_training_graph_resources();
	float benchmark_graph_launch_latency(int iterations = 100);

	// Phase 6: Sparse Mixture of Experts (MoE) Routing
	bool build_expert_layers(size_t hidden_dim, size_t ffn_dim, size_t num_experts = 8);
	bool forward_mixture_experts(std::vector<std::bfloat16_t>& hidden_states,
				     size_t sequence_length,
				     std::vector<int>& routing_histogram);
	bool verify_routing_balance(const std::vector<int>& routing_histogram,
				    size_t total_tokens,
				    size_t num_experts = 8);

	// ── Axiom Native Stacked Pipeline ───────────────────────────────────────────
	AxiomModelConfig config_;
	std::vector<TransformerLayerWeights> layers_;

	void* d_weights_arena_ = nullptr;
	const void* d_embed_tokens_ = nullptr;
	const void* d_final_norm_weight_ = nullptr;
	const void* d_lm_head_weight_ = nullptr;

	void* d_curr_token_ = nullptr;
	void* d_curr_pos_ = nullptr;
	void* d_residual_stream_ = nullptr;
	void* d_normed_x_ = nullptr;
	void* d_q_ = nullptr;
	void* d_k_ = nullptr;
	void* d_v_ = nullptr;
	void* d_attn_out_ = nullptr;
	void* d_proj_tmp_ = nullptr;
	void* d_gate_buf_ = nullptr;
	void* d_up_buf_ = nullptr;
	void* d_act_buf_ = nullptr;
	void* d_block_table_ = nullptr;
	void* d_pred_id_ = nullptr;
	void* d_logits_ = nullptr;
	uint32_t h_pred_id_ = 0;

	std::vector<void*> d_k_cache_;
	std::vector<void*> d_v_cache_;

	void* compute_stream_ = nullptr;

	std::unordered_map<std::string, uint32_t> axiom_vocab_;
	std::unordered_map<uint32_t, std::string> axiom_reverse_vocab_;
	std::vector<uint32_t> cached_tokens_;
	mutable std::mutex inference_mutex_;

	bool load_vocabulary(const std::string& vocab_path);
	bool load_axiom_model_mmap(const char* filepath);
	bool has_transformer_model() const noexcept {
		return d_weights_arena_ != nullptr;
	}
	void forward_transformer_step(int token_id,
				      int token_pos,
				      float* d_out_logits,
				      void* stream);
	bool generate(const std::string& prompt,
		      size_t max_new_tokens,
		      float temperature,
		      int top_k,
		      float top_p,
		      std::function<void(const std::string&)> token_callback);
};

#endif
