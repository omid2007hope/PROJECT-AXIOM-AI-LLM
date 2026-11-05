#ifndef INCLUDE_AI_HPP
#define INCLUDE_AI_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <stdfloat>
#include <string>
#include <vector>

#include "../src/CUDA/transfer.cuh"
#include "optimizer.hpp"
#include "paged_cache.hpp"

#pragma pack(push, 1)

struct AxiomWeightHeader {
	char magic[8] = {'A', 'X', 'I', 'O', 'M', '_', 'V', '1'};
	uint64_t input_elements_x_total = 0;
	uint64_t target_elements_y_total = 0;
	uint64_t weight_count = 0;
	uint64_t bias_count = 0;
	uint64_t vocab_size = 0;
	uint64_t embedding_dim = 0;
};

#pragma pack(pop)

static_assert(sizeof(AxiomWeightHeader) == 56, "AxiomWeightHeader must be exactly 56 bytes");

struct Elements {
	size_t weight_elements = 0;
	size_t bias_elements = 0;
};

// What one AI::train() call did
enum class TrainStatus {
	Trained, // one full forward + backward step ran
	NoData,	 // the RAM buffer is exhausted: the caller must refill it
	Failed	 // a CUDA step failed (already reported on stderr)
};

struct SamplingConfig {
	float temperature = 0.7f;
	int top_k = 40;
	float top_p = 0.9f;
	uint64_t seed = 0;
};

struct AxiomModelConfig {
	uint32_t magic          = 0x4D4F4958; // "XIOM"
	uint32_t version        = 2;
	uint32_t num_layers     = 28;
	uint32_t hidden_dim     = 1536;
	uint32_t num_q_heads    = 12;
	uint32_t num_kv_heads   = 2;
	uint32_t head_dim       = 128;
	uint32_t ffn_dim        = 8960;
	uint32_t vocab_size     = 151936;
	uint32_t max_seq_len    = 8192;
	float    rms_norm_eps   = 1e-6f;
	float    rope_theta     = 1000000.0f;
};

struct TransformerLayerWeights {
	const void* d_input_layernorm     = nullptr;
	const void* d_q_proj_weight       = nullptr;
	const void* d_q_proj_bias         = nullptr;
	const void* d_k_proj_weight       = nullptr;
	const void* d_k_proj_bias         = nullptr;
	const void* d_v_proj_weight       = nullptr;
	const void* d_v_proj_bias         = nullptr;
	const void* d_o_proj_weight       = nullptr;
	const void* d_post_attn_layernorm = nullptr;
	const void* d_gate_proj_weight    = nullptr;
	const void* d_up_proj_weight      = nullptr;
	const void* d_down_proj_weight    = nullptr;
};

class Parser;
struct Input;

class AI {
private:
	size_t input_elements_x_total;
	size_t target_elements_y_total;

	// Bounds of the uniform distribution used to initialize the weights
	float weight_min;
	float weight_max;

	// Training steps launched since the last read_loss()
	size_t pending_steps = 0;

	// Copies the last forward pass output and the GPU-argmax token to the host
	bool read_prediction(std::vector<float>& output_prediction);

public:
	AI(const AI&) = delete;

	AI& operator=(const AI&) = delete;

	size_t vocab_size = 65536;
	size_t embedding_dim = 4;
	size_t active_vocab_size = 0;

	// Milestone 3: Discrete Autoregressive Token Classification
	uint32_t last_predicted_token_id = 0;

	// Phase 3: Transformer Multi-Head Attention & SwiGLU FFN
	bool enable_transformer = false;
	size_t num_heads = 4;
	size_t ffn_dimension = 64;

	// CPU Memory (High precision float for initialization and updates)
	std::vector<float> host_weights;
	std::vector<float> host_bias;
	std::vector<float> host_embeddings;

	// GPU Memory State (The pointers for the VRAM)
	GpuWeights gpu_weights;
	GpuActivations gpu_activations;
	GpuPortion gpu_portion; // Reused across
	// iterations to prevent reallocation
	GpuEmbeddings gpu_embeddings;
	GpuPagedCache gpu_paged_cache;
	std::unique_ptr<PagedBlockManager> block_manager;
	GpuGraphExecution gpu_graph_execution;

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

	// The Training Loop Orchestrator
	TrainStatus train(Optimizer& optimizer, Portion& portion);

	// Mean training loss (0.5 * squared error per step, measured before each update)
	// over the steps since the previous call. `steps` receives how many that was.
	// Waits for the GPU to finish, so call it once per report interval, not per step.
	bool read_loss(double& mean_loss, size_t& steps);

	// Weight persistence methods
	bool save_weights(const std::string& path = "weights.axiom");

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

	// ── Qwen2.5 / Multi-Layer Stacked Pipeline ──────────────────────────────────
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

	std::unordered_map<std::string, uint32_t> qwen_vocab_;
	std::unordered_map<uint32_t, std::string> qwen_reverse_vocab_;
	std::vector<uint32_t> cached_tokens_;
	mutable std::mutex inference_mutex_;

	bool load_vocabulary(const std::string& vocab_path);
	bool load_axiom_model_mmap(const char* filepath);
	bool has_transformer_model() const noexcept { return d_weights_arena_ != nullptr; }
	void forward_transformer_step(int token_id, int token_pos, float* d_out_logits, void* stream);
	bool generate(const std::string& prompt,
		      size_t max_new_tokens,
		      float temperature,
		      int top_k,
		      float top_p,
		      std::function<void(const std::string&)> token_callback);
};

#endif
