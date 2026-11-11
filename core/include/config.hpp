/*
 * Copyright (c) 2026 Omid Teimory. All Rights Reserved.
 *
 * NOTICE: This file contains proprietary source code licensed exclusively under the
 * terms of the LICENSE file in the root directory of this repository.
 * You may view, download, and compile this code solely to run the unmodified product.
 * Copying, pasting, modifying, redistributing, creating derivative works, or using
 * this code in any other project or AI training system is strictly prohibited.
 * Removal or alteration of this notice is a violation of 17 U.S.C. § 1202 and EU law.
 */

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

enum class AppMode { Chat, Use, Train, BulkTrain, Test, Benchmark, Server, Exit };

struct Config {
	static constexpr size_t ram_buffer_size = 4194304;
	static constexpr size_t token_dim = 1024;
	static constexpr size_t context_window_size = 256;
	static constexpr float learning_rate = 0.001f;
	static constexpr size_t loss_report_interval = 1000;

	static constexpr float weight_min = -0.0033854167f;
	static constexpr float weight_max = 0.0033854167f;

	// Folder layout & files
	static constexpr const char* database_folder = "database";
	static constexpr const char* models_folder = "models";
	static constexpr const char* training_folder = "training";
	static constexpr const char* embedded_file = "embedded_data.bin";
	static constexpr const char* asset_folder = "asset_sample";
	static constexpr const char* weights_file = "weights.axiom";
	static constexpr const char* vocab_file = "vocab.axiom";
};

namespace config {

// Hyperparameters
constexpr size_t ram_buffer_size = Config::ram_buffer_size; // 4M tokens (~16.7 MB in RAM)
constexpr size_t token_dim = Config::token_dim;			// GPT-2 scale (Must be multiple of 64)
constexpr size_t context_window_size = Config::context_window_size; // Doubled to capture paragraph-level context

// AdamW Optimizer Settings
constexpr float learning_rate = Config::learning_rate; // Base AdamW rate (fan-in scaled inside CUDA kernels)

// How many training steps pass between two loss reports
constexpr size_t loss_report_interval = Config::loss_report_interval; // Increased to prevent CPU/GPU synchronization stalls

// Initial weights are drawn uniformly from [-sqrt(3/K), +sqrt(3/K)], K = fan-in =
// context_window_size * token_dim = 262144. That gives std 1/sqrt(K) = 1/512, so the
// RMS-normalized input produces unit-variance outputs. (1/sqrt(token_dim) ignores the
// 256x fan-in and explodes the logits.) sqrt(3)/512 = 0.0033854167f.
constexpr float weight_min = Config::weight_min;
constexpr float weight_max = Config::weight_max;

// Folder layout
constexpr const char* database_folder = Config::database_folder;
constexpr const char* models_folder = Config::models_folder;
constexpr const char* training_folder = Config::training_folder;
constexpr const char* embedded_file = Config::embedded_file;
constexpr const char* asset_folder = Config::asset_folder;
constexpr const char* weights_file = Config::weights_file;
constexpr const char* vocab_file = Config::vocab_file;

} // namespace config


namespace vocab {
constexpr uint32_t VOCAB_SIZE = 65536;

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
} // namespace vocab

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
	uint32_t feedforward_network_dimension = 8960;
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
