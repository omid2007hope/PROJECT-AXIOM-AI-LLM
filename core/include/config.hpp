// SPDX-License-Identifier: LicenseRef-Axiom-Proprietary
/*
 * Copyright (c) 2026 Omid Teimory. All Rights Reserved.
 *
 * PROPRIETARY SOURCE-AVAILABLE SOFTWARE — NOT OPEN SOURCE.
 * Permission to view, clone, or compile this file is granted strictly on the
 * condition precedent of full compliance with the root ./LICENSE file.
 * Copying snippets, modifying, creating derivative works, redistributing,
 * commercial/SaaS use, and AI/ML training (reserved under EU Directive
 * 2019/790 Art. 4(3) and § 42h(6) Austrian UrhG) are strictly prohibited.
 * Removal or alteration of this notice violates 17 U.S.C. § 1202 and § 90c UrhG.
 */

#ifndef INCLUDE_CONFIG_HPP
#define INCLUDE_CONFIG_HPP

#include <cstddef>
#include <cstdint>
#include <ios>
#include <ostream>

#include "shared_params.h"

class FormatGuard
{
public:
	explicit FormatGuard(std::ostream& stream)
	    : stream_(stream), flags_(stream.flags()), precision_(stream.precision())
	{
	}
	~FormatGuard()
	{
		stream_.flags(flags_);
		stream_.precision(precision_);
	}
	FormatGuard(const FormatGuard&) = delete;
	FormatGuard& operator=(const FormatGuard&) = delete;

private:
	std::ostream& stream_;
	std::ios_base::fmtflags flags_;
	std::streamsize precision_;
};

#pragma pack(push, 1)

struct AxiomWeightHeader
{
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

enum class AppMode
{
	Chat,
	Use,
	BulkTrain,
	Test,
	Benchmark,
	Server,
	Exit
};

struct Config
{
	static constexpr size_t ram_buffer_size = 4194304;
	static constexpr size_t token_dim = axiom::TOKEN_DIM;
	static constexpr size_t context_window_size = axiom::CONTEXT_WINDOW;
	static constexpr float learning_rate = 0.0006f;
	static constexpr size_t loss_report_interval = 1000;

	static constexpr float weight_min = -0.0033854167f;
	static constexpr float weight_max = 0.0033854167f;
	static constexpr uint32_t random_seed = 1337U;

	// Training & CLI defaults
	static constexpr size_t default_training_epochs = 4;
	static constexpr uint32_t default_min_frequency = 3;
	static constexpr double default_target_stop_loss = 2.8;

	// Optimization for test
	static constexpr size_t CHUNK_TOKENS = 65536;
	static constexpr size_t chunk_position = 0;
	static constexpr size_t tokens_written = 0;
	static constexpr size_t last_percentage = 0;

	// Demonstrate progress with bar & I/O buffering
	static constexpr size_t minimum_limit = 500000;
	static constexpr size_t io_buffer_bytes = 65536;
	static constexpr uint64_t progress_min_file_bytes = 5ULL * 1024ULL * 1024ULL;
	static constexpr uint64_t progress_step_bytes = 2ULL * 1024ULL * 1024ULL;
	static constexpr int progress_bar_width = 24;

	// Transformer, MoE, PagedAttention & Generation defaults
	static constexpr size_t default_training_layers = 4;
	static constexpr uint32_t axiom_model_magic = 0x4D4F4958U; // "XIOM"
	static constexpr uint32_t axiom_model_version = 2U;
	static constexpr size_t axiom_model_header_bytes = 64;
	static constexpr uint64_t xform_magic = 0x5458464DU; // "TXFM"
	static constexpr uint32_t num_layers = 28U;
	static constexpr float rms_norm_eps = 1e-6f;
	static constexpr float rope_theta = AXIOM_ROPE_THETA;
	static constexpr uint32_t max_seq_len_cap = 16384U;
	static constexpr size_t mmap_chunk_bytes = 256ULL * 1024ULL * 1024ULL;
	static constexpr size_t paged_total_blocks = axiom::PAGED_TOTAL_BLOCKS;
	static constexpr size_t paged_block_size = axiom::PAGED_BLOCK_SIZE;
	static constexpr size_t default_max_new_tokens = 128;
	static constexpr float default_temperature = 0.7f;
	static constexpr int default_top_k = 40;
	static constexpr float default_top_p = 0.9f;
	static constexpr size_t default_num_experts = 8;
	static constexpr int default_graph_benchmark_iterations = 100;

	// Transformer weights and  memory allocation in memory_allocation.cu
	// Brings back Transformer weights to initial random state
	static constexpr uint32_t transformer_randomizer_seed = axiom::TRANSFORMER_RANDOMIZER_SEED;
	// number of layers for Transformer weights random state
	static constexpr size_t transformer_weight_layers = axiom::TRANSFORMER_LAYER_COUNT;

	// Folder layout & files
	static constexpr const char* database_folder = "database";
	static constexpr const char* models_folder = "models";
	static constexpr const char* training_folder = "training";
	static constexpr const char* archive_folder = "archive";
	static constexpr const char* embedded_file = "embedded_data.bin";
	static constexpr const char* asset_folder = "asset_sample";
	static constexpr const char* weights_file = "weights.axiom";
	static constexpr const char* vocab_file = "vocab.axiom";
	static constexpr const char* llm_weights_file = "axiom.axiom";
	static constexpr const char* llm_weights_3b_file = "axiom3b.axiom";
	static constexpr const char* llm_vocab_file = "axiom_vocab.axiom";
};

namespace config
{

// Hyperparameters
constexpr size_t ram_buffer_size = Config::ram_buffer_size; // 4M tokens (~16.7 MB in RAM)
constexpr size_t token_dim = Config::token_dim;             // GPT-2 scale (Must be multiple of 64)
constexpr size_t context_window_size =
    Config::context_window_size; // Doubled to capture paragraph-level context

static_assert(ram_buffer_size % context_window_size == 0,
              "ram_buffer_size must be an exact multiple of context_window_size");

// AdamW Optimizer Settings
constexpr float learning_rate = Config::learning_rate; // Base AdamW rate (direct step size)

// How many training steps pass between two loss reports
constexpr size_t loss_report_interval =
    Config::loss_report_interval; // Increased to prevent CPU/GPU synchronization stalls

// Initial weights are drawn uniformly from [-sqrt(3/K), +sqrt(3/K)], K = fan-in =
// context_window_size * token_dim = 262144. That gives std 1/sqrt(K) = 1/512, so the
// RMS-normalized input produces unit-variance outputs. (1/sqrt(token_dim) ignores the
// 256x fan-in and explodes the logits.) sqrt(3)/512 = 0.0033854167f.
constexpr float weight_min = Config::weight_min;
constexpr float weight_max = Config::weight_max;
constexpr uint32_t random_seed = Config::random_seed;

// Training & CLI defaults
// Temporary for testing (original: Config::num_layers = 28)
constexpr size_t default_training_layers = Config::default_training_layers;
constexpr size_t default_training_epochs = Config::default_training_epochs;
constexpr uint32_t default_min_frequency = Config::default_min_frequency;
constexpr double default_target_stop_loss = Config::default_target_stop_loss;

// Streaming, buffering & progress display
constexpr size_t chunk_tokens = Config::CHUNK_TOKENS;
constexpr size_t minimum_limit = Config::minimum_limit;
constexpr size_t io_buffer_bytes = Config::io_buffer_bytes;
constexpr uint64_t progress_min_file_bytes = Config::progress_min_file_bytes;
constexpr uint64_t progress_step_bytes = Config::progress_step_bytes;
constexpr int progress_bar_width = Config::progress_bar_width;

// Transformer, MoE, PagedAttention & Generation defaults
constexpr uint32_t axiom_model_magic = Config::axiom_model_magic;
constexpr uint32_t axiom_model_version = Config::axiom_model_version;
constexpr size_t axiom_model_header_bytes = Config::axiom_model_header_bytes;
constexpr uint64_t xform_magic = Config::xform_magic;
constexpr uint32_t num_layers = Config::num_layers;
constexpr float rms_norm_eps = Config::rms_norm_eps;
constexpr float rope_theta = Config::rope_theta;
constexpr uint32_t max_seq_len_cap = Config::max_seq_len_cap;
constexpr size_t mmap_chunk_bytes = Config::mmap_chunk_bytes;
constexpr size_t paged_total_blocks = Config::paged_total_blocks;
constexpr size_t paged_block_size = Config::paged_block_size;
constexpr size_t default_max_new_tokens = Config::default_max_new_tokens;
constexpr float default_temperature = Config::default_temperature;
constexpr int default_top_k = Config::default_top_k;
constexpr float default_top_p = Config::default_top_p;
constexpr size_t default_num_experts = Config::default_num_experts;
constexpr int default_graph_benchmark_iterations = Config::default_graph_benchmark_iterations;

// Transformer weights and  memory allocation in memory_allocation.cu
// Brings back Transformer weights to initial random state
constexpr uint32_t default_transformer_randomizer_seed = axiom::TRANSFORMER_RANDOMIZER_SEED;
// number of layers for Transformer weights random state
constexpr size_t default_transformer_layer_count = axiom::TRANSFORMER_LAYER_COUNT;

// Folder layout
constexpr const char* database_folder = Config::database_folder;
constexpr const char* models_folder = Config::models_folder;
constexpr const char* training_folder = Config::training_folder;
constexpr const char* archive_folder = Config::archive_folder;
constexpr const char* embedded_file = Config::embedded_file;
constexpr const char* asset_folder = Config::asset_folder;
constexpr const char* weights_file = Config::weights_file;
constexpr const char* vocab_file = Config::vocab_file;
constexpr const char* llm_weights_file = Config::llm_weights_file;
constexpr const char* llm_weights_3b_file = Config::llm_weights_3b_file;
constexpr const char* llm_vocab_file = Config::llm_vocab_file;

} // namespace config

namespace vocab
{
constexpr uint32_t VOCAB_SIZE = axiom::VOCAB_SIZE;

// Slots [0 .. 3]: Special reserved tokens
constexpr uint32_t PAD = 0;
constexpr uint32_t UNK = 1;
constexpr uint32_t BOS = 2;
constexpr uint32_t EOS = 3;

// Slots [4 .. 64,511]: Text words, subwords, numbers, and mathematical symbols
constexpr uint32_t TEXT_START_ID = 4;
constexpr uint32_t TEXT_MAX_ID = 64511;

// Slots [64,512 .. 64,767]: Discrete visual bytes (256 slots for 0x00..0xFF)
constexpr uint32_t VISUAL_START_ID = 64512;
constexpr uint32_t VISUAL_END_ID = 64767;

// Slots [64,768 .. 65,023]: Discrete acoustic bytes (256 slots for 0x00..0xFF)
constexpr uint32_t ACOUSTIC_START_ID = 64768;
constexpr uint32_t ACOUSTIC_END_ID = 65023;

// Slots [65,024 .. 65,535]: Reserved expansion slots for future modality codebooks
constexpr uint32_t EXPANSION_START_ID = 65024;
constexpr uint32_t EXPANSION_MAX_ID = 65535;

// External BPE chat template / EOS token IDs
constexpr uint32_t ENDOFTEXT_TOKEN_ID = 151643;
constexpr uint32_t IM_START_TOKEN_ID = 151644;
constexpr uint32_t IM_END_TOKEN_ID = 151645;
} // namespace vocab

struct Elements
{
	size_t weight_elements = 0;
	size_t bias_elements = 0;
};

// What one AI::train() call did
enum class TrainStatus
{
	Trained, // one full forward + backward step ran
	NoData,  // the RAM buffer is exhausted: the caller must refill it
	Failed   // a CUDA step failed (already reported on stderr)
};

struct TrainingTelemetry
{
	double mean_loss = 0.0;
	double gradient_norm = 0.0;
	double activation_magnitude = 0.0;
	double dead_fraction = 0.0;
	size_t nonfinite_count = 0;
	size_t steps = 0;
};

struct SamplingConfig
{
	float temperature = Config::default_temperature;
	int top_k = Config::default_top_k;
	float top_p = Config::default_top_p;
	uint64_t seed = 0;
};

struct AxiomModelConfig
{
	uint32_t magic = Config::axiom_model_magic;
	uint32_t version = Config::axiom_model_version;
	uint32_t num_layers = Config::num_layers;
	uint32_t hidden_dim = static_cast<uint32_t>(axiom::TOKEN_DIM);
	uint32_t num_q_heads = static_cast<uint32_t>(axiom::NUM_HEADS);
	uint32_t num_kv_heads = static_cast<uint32_t>(axiom::NUM_HEADS);
	uint32_t head_dim = static_cast<uint32_t>(axiom::HEAD_DIM);
	uint32_t feedforward_network_dimension = static_cast<uint32_t>(axiom::FFN_DIM);
	uint32_t vocab_size = axiom::VOCAB_SIZE;
	uint32_t max_seq_len = static_cast<uint32_t>(axiom::CONTEXT_WINDOW);
	float rms_norm_eps = Config::rms_norm_eps;
	float rope_theta = Config::rope_theta;
};

struct TransformerLayerWeights
{
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
