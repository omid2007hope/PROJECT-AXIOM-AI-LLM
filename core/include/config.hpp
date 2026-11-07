#ifndef INCLUDE_CONFIG_HPP
#define INCLUDE_CONFIG_HPP

#include <cstddef>
#include <cstdint>

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

struct TrainingTelemetry {
	double mean_loss = 0.0;
	double gradient_norm = 0.0;
	double activation_magnitude = 0.0;
	double dead_fraction = 0.0;
	size_t nonfinite_count = 0;
	size_t steps = 0;
};

struct SamplingConfig {
	float temperature = 0.7f;
	int top_k = 40;
	float top_p = 0.9f;
	uint64_t seed = 0;
};

struct AxiomModelConfig {
	uint32_t magic = 0x4D4F4958; // "XIOM"
	uint32_t version = 2;
	uint32_t num_layers = 28;
	uint32_t hidden_dim = 1536;
	uint32_t num_q_heads = 12;
	uint32_t num_kv_heads = 2;
	uint32_t head_dim = 128;
	uint32_t ffn_dim = 8960;
	uint32_t vocab_size = 151936;
	uint32_t max_seq_len = 8192;
	float rms_norm_eps = 1e-6f;
	float rope_theta = 1000000.0f;
};

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

#endif // INCLUDE_CONFIG_HPP
