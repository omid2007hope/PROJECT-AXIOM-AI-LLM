#ifndef include_config
#define include_config

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

size_t vocab_size = 65536;
size_t embedding_dim = 4;
size_t active_vocab_size = 0;
uint32_t random_seed = 1337U;

// Milestone 3: Discrete Autoregressive Token Classification
uint32_t last_predicted_token_id = 0;

// Phase 3: Transformer Multi-Head Attention & SwiGLU FFN
bool enable_transformer = false;
size_t num_heads = 4;
size_t ffn_dimension = 64;

size_t input_elements_x_total = 0;
size_t target_elements_y_total = 0;

// Bounds of the uniform distribution used to initialize the weights
float weight_min = 0;
float weight_max = 0;

// Training steps launched since the last read_loss()
size_t pending_steps = 0;

// Copies the last forward pass output and the GPU-argmax token to the host
bool read_prediction(std::vector<float>& output_prediction);

#endif // include_config