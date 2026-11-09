#include "../include/ai.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <windows.h>

#include "../include/parser.hpp"
#include "../include/transfer.hpp"
#include "CUDA/kernel/forward_pass.cuh"
#include "CUDA/kernel/paged_gqa.cuh"
#include "CUDA/kernel/rmsnorm.cuh"
#include "CUDA/kernel/rope.cuh"
#include "CUDA/kernel/swiglu.cuh"
#include "CUDA/util/memory_allocation.cuh"

AI::AI()
    : input_elements_x_total(512), target_elements_y_total(4), weight_min(-0.1f), weight_max(0.1f) {
	vocab_size = 151936;
	embedding_dim = 1536;
	compute_stream_ = create_cuda_stream();
}


AI::AI(size_t input_elements_x_total,
       size_t target_elements_y_total,
       float weight_min,
       float weight_max,
       Elements& elements)
    : input_elements_x_total(input_elements_x_total),
      target_elements_y_total(target_elements_y_total),
      weight_min(weight_min),
      weight_max(weight_max) {
	// input_elements_x_total = token * dim = 512
	// target_elements_y_total = target * dim = 4

	// 512 * 4 = 2048
	elements.weight_elements = input_elements_x_total * target_elements_y_total;

	// target_elements_y_total = 4
	elements.bias_elements = target_elements_y_total;

	host_weights.resize(elements.weight_elements);
	host_bias.resize(elements.bias_elements);

	vocab_size = 65536;
	embedding_dim = target_elements_y_total;
	if (embedding_dim == 0)
		embedding_dim = 4;

	host_embeddings.resize(vocab_size * embedding_dim);

	reset_weights(elements);
	if (allocate_gpu_embeddings(&gpu_embeddings, vocab_size, embedding_dim)) {
		upload_embeddings(host_embeddings.data(), &gpu_embeddings);
	}
	compute_stream_ = create_cuda_stream();
}

void AI::reset_weights(Elements& elements) {
	reset_weights(elements, false);
}

void AI::reset_weights(Elements& elements, bool verbose) {
	elements.weight_elements = input_elements_x_total * target_elements_y_total;
	elements.bias_elements = target_elements_y_total;

	// Initialize with Chaos (Symmetry Breaking) using deterministic seed progression
	std::mt19937 gen(random_seed++);

	// Uniform distribution between weight_min and weight_max
	std::uniform_real_distribution<float> dis(weight_min, weight_max);

	for (size_t i = 0; i < elements.weight_elements; ++i) {
		host_weights[i] = dis(gen);
	}

	for (size_t i = 0; i < elements.bias_elements; ++i) {
		// Biases can often safely start at 0.0, because the chaotic weights
		// will do the heavy lifting to break the symmetry on the first pass.
		host_bias[i] = 0.0f;
	}

	float bound = 1.0f / sqrtf(static_cast<float>(embedding_dim));
	std::uniform_real_distribution<float> dis_emb(-bound, bound);
	for (float& val : host_embeddings) {
		val = dis_emb(gen);
	}
	std::fill(
		host_embeddings.begin(), host_embeddings.begin() + embedding_dim, 0.0f); // PAD row

	if (gpu_embeddings.d_table_master != nullptr) {
		upload_embeddings(host_embeddings.data(), &gpu_embeddings);
	}

	if (verbose) {
		std::cout << "[AI Brain] Weights, Biases, and Embeddings reset to chaotic state.\n"
				  << std::flush;
	}
}

AI::~AI() {
	free_training_graph_resources();
	free_paged_cache_memory();
	free_gpu(&gpu_portion);
	free_gpu_weights(&gpu_weights);
	free_gpu_activations(&gpu_activations);
	free_gpu_embeddings(&gpu_embeddings);

	if (d_weights_arena_) {
		free_device_memory(d_weights_arena_);
		d_weights_arena_ = nullptr;
	}
	if (d_curr_token_) {
		free_device_memory(d_curr_token_);
		d_curr_token_ = nullptr;
	}
	if (d_curr_pos_) {
		free_device_memory(d_curr_pos_);
		d_curr_pos_ = nullptr;
	}
	if (d_residual_stream_) {
		free_device_memory(d_residual_stream_);
		d_residual_stream_ = nullptr;
	}
	if (d_normed_x_) {
		free_device_memory(d_normed_x_);
		d_normed_x_ = nullptr;
	}
	if (d_q_) {
		free_device_memory(d_q_);
		d_q_ = nullptr;
	}
	if (d_k_) {
		free_device_memory(d_k_);
		d_k_ = nullptr;
	}
	if (d_v_) {
		free_device_memory(d_v_);
		d_v_ = nullptr;
	}
	if (d_attn_out_) {
		free_device_memory(d_attn_out_);
		d_attn_out_ = nullptr;
	}
	if (d_proj_tmp_) {
		free_device_memory(d_proj_tmp_);
		d_proj_tmp_ = nullptr;
	}
	if (d_gate_buf_) {
		free_device_memory(d_gate_buf_);
		d_gate_buf_ = nullptr;
	}
	if (d_up_buf_) {
		free_device_memory(d_up_buf_);
		d_up_buf_ = nullptr;
	}
	if (d_act_buf_) {
		free_device_memory(d_act_buf_);
		d_act_buf_ = nullptr;
	}
	if (d_pred_id_) {
		free_device_memory(d_pred_id_);
		d_pred_id_ = nullptr;
	}
	if (d_logits_) {
		free_device_memory(d_logits_);
		d_logits_ = nullptr;
	}
	if (d_block_table_) {
		free_device_memory(d_block_table_);
		d_block_table_ = nullptr;
	}
	for (void* ptr : d_k_cache_) {
		if (ptr)
			free_device_memory(ptr);
	}
	for (void* ptr : d_v_cache_) {
		if (ptr)
			free_device_memory(ptr);
	}
	d_k_cache_.clear();
	d_v_cache_.clear();
	if (compute_stream_) {
		destroy_cuda_stream(compute_stream_);
		compute_stream_ = nullptr;
	}
}

bool AI::upload_weights() {
	bool ok = Transfer::upload_weights(
		host_weights, host_bias, pending_steps, gpu_weights, gpu_activations);
	if (!ok)
		return false;

	if (gpu_embeddings.d_table != nullptr && !host_embeddings.empty()) {
		if (!Transfer::upload_embeddings(host_embeddings, gpu_embeddings)) {
			return false;
		}
	}
	return true;
}

bool AI::download_weights() {
	bool ok = Transfer::download_weights(host_weights, host_bias, gpu_weights);
	if (!ok)
		return false;

	if (gpu_embeddings.d_table_master != nullptr && !host_embeddings.empty()) {
		if (!Transfer::download_embeddings(host_embeddings, gpu_embeddings)) {
			return false;
		}
	}
	return true;
}

bool AI::load_weights(const std::string& path) {
	std::ifstream in(path, std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "[AI Brain] Failed to open weight file: " << path << "\n";
		return false;
	}

	AxiomWeightHeader header {};
	in.read(reinterpret_cast<char*>(&header), sizeof(header));
	if (in.gcount() != static_cast<std::streamsize>(sizeof(header))) {
		std::cerr << "[AI Brain] Incomplete or corrupt header in weight file: " << path
			  << "\n";
		return false;
	}

	// Validate magic bytes
	if (std::memcmp(header.magic, "AXIOM_V1", 8) != 0) {
		std::cerr << "[AI Brain] Invalid magic header in " << path
			  << " (expected 'AXIOM_V1')\n";
		return false;
	}

	// Validate model dimensions
	if (header.input_elements_x_total != input_elements_x_total ||
	    header.target_elements_y_total != target_elements_y_total) {
		std::cerr << "[AI Brain] Dimension mismatch in " << path << ": file has ("
			  << header.input_elements_x_total << " -> "
			  << header.target_elements_y_total << "), model expects ("
			  << input_elements_x_total << " -> " << target_elements_y_total << ")\n";
		return false;
	}

	if (header.weight_count != input_elements_x_total * target_elements_y_total ||
	    header.bias_count != target_elements_y_total) {
		std::cerr << "[AI Brain] Element count mismatch in " << path << "\n";
		return false;
	}

	if (header.embedding_dim != 0 && header.embedding_dim != target_elements_y_total) {
		std::cerr << "[AI Brain] Embedding dimension mismatch in " << path << ": file has "
			  << header.embedding_dim << ", model expects " << target_elements_y_total
			  << "\n";
		return false;
	}

	std::vector<float> loaded_weights(header.weight_count);
	std::vector<float> loaded_bias(header.bias_count);
	std::vector<float> loaded_embeddings;

	in.read(reinterpret_cast<char*>(loaded_weights.data()),
		header.weight_count * sizeof(float));
	if (in.gcount() != static_cast<std::streamsize>(header.weight_count * sizeof(float))) {
		std::cerr << "[AI Brain] Incomplete weight data in " << path << "\n";
		return false;
	}

	in.read(reinterpret_cast<char*>(loaded_bias.data()), header.bias_count * sizeof(float));
	if (in.gcount() != static_cast<std::streamsize>(header.bias_count * sizeof(float))) {
		std::cerr << "[AI Brain] Incomplete bias data in " << path << "\n";
		return false;
	}

	if (header.vocab_size > 0 && header.embedding_dim > 0) {
		size_t embedding_count = header.vocab_size * header.embedding_dim;
		loaded_embeddings.resize(embedding_count);
		in.read(reinterpret_cast<char*>(loaded_embeddings.data()),
			embedding_count * sizeof(float));
		if (in.gcount() != static_cast<std::streamsize>(embedding_count * sizeof(float))) {
			std::cerr << "[AI Brain] Incomplete embedding data in " << path << "\n";
			return false;
		}
	}

	for (float w : loaded_weights) {
		if (!std::isfinite(w)) {
			std::cerr << "[AI Brain] Corrupt weights in " << path
				  << ": contains NaN or Inf. Rejecting load.\n";
			return false;
		}
	}
	for (float b : loaded_bias) {
		if (!std::isfinite(b)) {
			std::cerr << "[AI Brain] Corrupt biases in " << path
				  << ": contains NaN or Inf. Rejecting load.\n";
			return false;
		}
	}
	for (float e_val : loaded_embeddings) {
		if (!std::isfinite(e_val)) {
			std::cerr << "[AI Brain] Corrupt embeddings in " << path
				  << ": contains NaN or Inf. Rejecting load.\n";
			return false;
		}
	}

	// Atomic update of host parameters
	host_weights = std::move(loaded_weights);
	host_bias = std::move(loaded_bias);
	if (!loaded_embeddings.empty()) {
		vocab_size = header.vocab_size;
		embedding_dim = header.embedding_dim;
		host_embeddings = std::move(loaded_embeddings);
	}

	// Synchronize and push directly to GPU VRAM
	if (!upload_weights()) {
		std::cerr << "[AI Brain] Failed to upload loaded weights to GPU VRAM.\n";
		return false;
	}

	std::cout << "[AI Brain] Successfully loaded weights and embeddings from " << path
		  << " and synced to GPU.\n"
		  << std::flush;
	return true;
}


bool AI::build_transformer_block(size_t hidden_dim,
				 size_t heads,
				 size_t feedforward_network_dimension) {
	if (hidden_dim == 0 || heads == 0 || feedforward_network_dimension == 0)
		return false;
	this->num_heads = heads;
	this->ffn_dimension = feedforward_network_dimension;
	this->enable_transformer = true;

	return allocate_transformer_weights(
		&gpu_weights, hidden_dim, feedforward_network_dimension);
}

bool AI::forward_transformer(std::vector<std::bfloat16_t>& hidden_states, size_t sequence_length) {
	if (hidden_states.empty() || sequence_length == 0 || gpu_weights.d_query_weights == nullptr)
		return false;
	size_t hidden_dim = hidden_states.size() / sequence_length;

	size_t bytes = hidden_states.size() * sizeof(std::bfloat16_t);
	void* d_temp = allocate_device_memory(bytes);
	if (!d_temp)
		return false;
	upload_device_buffer(d_temp, hidden_states.data(), bytes);

	bool ok = transformer_forward_block(d_temp,
					    &gpu_weights,
					    &gpu_activations,
					    sequence_length,
					    hidden_dim,
					    num_heads,
					    ffn_dimension,
					    0);
	if (ok) {
		download_device_buffer(hidden_states.data(), d_temp, bytes);
	}
	free_device_memory(d_temp);
	return ok;
}

bool AI::backward_transformer(const std::vector<float>& grad_output,
			      std::vector<float>& grad_input,
			      const std::vector<std::bfloat16_t>& hidden_states,
			      size_t sequence_length,
			      float learning_rate) {
	if (grad_output.empty() || sequence_length == 0 || gpu_weights.d_query_weights == nullptr)
		return false;
	size_t hidden_dim = grad_output.size() / sequence_length;
	grad_input.resize(grad_output.size());

	size_t f_bytes = grad_output.size() * sizeof(float);
	size_t bf_bytes = hidden_states.size() * sizeof(std::bfloat16_t);

	void* d_grad_out = allocate_device_memory(f_bytes);
	void* d_grad_in = allocate_device_memory(f_bytes);
	void* d_hidden = allocate_device_memory(bf_bytes);

	if (!d_grad_out || !d_grad_in || !d_hidden) {
		if (d_grad_out)
			free_device_memory(d_grad_out);
		if (d_grad_in)
			free_device_memory(d_grad_in);
		if (d_hidden)
			free_device_memory(d_hidden);
		return false;
	}

	upload_device_buffer(d_grad_out, grad_output.data(), f_bytes);
	upload_device_buffer(d_hidden, hidden_states.data(), bf_bytes);

	bool ok = transformer_backward_block(d_grad_out,
					     d_grad_in,
					     d_hidden,
					     &gpu_weights,
					     &gpu_activations,
					     sequence_length,
					     hidden_dim,
					     num_heads,
					     ffn_dimension,
					     learning_rate,
					     0);
	if (ok) {
		download_device_buffer(grad_input.data(), d_grad_in, f_bytes);
	}
	free_device_memory(d_grad_out);
	free_device_memory(d_grad_in);
	free_device_memory(d_hidden);
	return ok;
}

bool AI::build_expert_layers(size_t hidden_dim,
			     size_t feedforward_network_dimension,
			     size_t num_experts) {
	if (hidden_dim == 0 || feedforward_network_dimension == 0 || num_experts == 0)
		return false;
	this->ffn_dimension = feedforward_network_dimension;
	return allocate_expert_weights(
		&gpu_weights, hidden_dim, feedforward_network_dimension, num_experts);
}

bool AI::forward_mixture_experts(std::vector<std::bfloat16_t>& hidden_states,
				 size_t sequence_length,
				 std::vector<int>& routing_histogram) {
	if (hidden_states.empty() || sequence_length == 0 ||
	    gpu_weights.d_router_weights == nullptr)
		return false;

	size_t hidden_dim = hidden_states.size() / sequence_length;
	size_t bytes = hidden_states.size() * sizeof(std::bfloat16_t);

	void* d_temp = allocate_device_memory(bytes);
	if (!d_temp)
		return false;
	upload_device_buffer(d_temp, hidden_states.data(), bytes);

	bool ok = ::forward_mixture_experts(d_temp,
					    &gpu_weights,
					    &gpu_activations,
					    sequence_length,
					    hidden_dim,
					    ffn_dimension,
					    0);
	if (ok) {
		download_device_buffer(hidden_states.data(), d_temp, bytes);
		routing_histogram.resize(gpu_weights.expert_count, 0);
		download_expert_counts(
			&gpu_activations, routing_histogram.data(), gpu_weights.expert_count);
	}
	free_device_memory(d_temp);
	return ok;
}

bool AI::verify_routing_balance(const std::vector<int>& routing_histogram,
				size_t total_tokens,
				size_t num_experts) {
	if (routing_histogram.size() != num_experts || total_tokens == 0 || num_experts == 0)
		return false;

	int sum_tokens = 0;
	int min_tokens = routing_histogram[0];
	int max_tokens = routing_histogram[0];

	for (int count : routing_histogram) {
		sum_tokens += count;
		if (count < min_tokens)
			min_tokens = count;
		if (count > max_tokens)
			max_tokens = count;
	}

	// Each token is routed to exactly 2 experts
	if (sum_tokens != static_cast<int>(total_tokens * 2))
		return false;

	// Zero dead experts allowed
	if (min_tokens <= 0)
		return false;

	// Maximum load imbalance ratio must be within reasonable bound (< 3.0x average)
	float avg_load = static_cast<float>(sum_tokens) / static_cast<float>(num_experts);
	float max_ratio = static_cast<float>(max_tokens) / avg_load;

	return max_ratio < 3.0f;
}

bool AI::load_vocabulary(const std::string& vocab_path) {
	std::ifstream file(vocab_path, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "[Axiom Engine] Failed to open vocab file: " << vocab_path << "\n";
		return false;
	}
	uint32_t count = 0;
	if (!file.read(reinterpret_cast<char*>(&count), sizeof(uint32_t)) || count > 2000000U) {
		return false;
	}
	axiom_vocab_.clear();
	axiom_reverse_vocab_.clear();
	axiom_vocab_.reserve(count);
	axiom_reverse_vocab_.reserve(count);

	for (uint32_t i = 0; i < count; ++i) {
		uint32_t token_id = 0;
		uint32_t str_len = 0;
		if (!file.read(reinterpret_cast<char*>(&token_id), sizeof(uint32_t)))
			break;
		if (!file.read(reinterpret_cast<char*>(&str_len), sizeof(uint32_t)) ||
		    str_len > 1048576U)
			break;
		std::string token_str(str_len, '\0');
		if (str_len > 0) {
			if (!file.read(&token_str[0], str_len))
				break;
		}
		axiom_vocab_[token_str] = token_id;
		axiom_reverse_vocab_[token_id] = token_str;
	}
	std::cout << "[Axiom Engine] Successfully loaded " << axiom_vocab_.size()
		  << " vocabulary entries.\n";
	return !axiom_vocab_.empty();
}

bool AI::load_axiom_model_mmap(const char* filepath) {
	HANDLE hFile = CreateFileA(filepath,
				   GENERIC_READ,
				   FILE_SHARE_READ,
				   nullptr,
				   OPEN_EXISTING,
				   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
				   nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		std::cerr << "[Axiom Engine] Cannot open model file: " << filepath << "\n";
		return false;
	}

	LARGE_INTEGER file_size;
	GetFileSizeEx(hFile, &file_size);
	size_t total_bytes = static_cast<size_t>(file_size.QuadPart);
	if (total_bytes <= 64) {
		CloseHandle(hFile);
		return false;
	}

	HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (!hMap) {
		CloseHandle(hFile);
		return false;
	}

	const uint8_t* mapped_base =
		static_cast<const uint8_t*>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0));
	if (!mapped_base) {
		CloseHandle(hMap);
		CloseHandle(hFile);
		return false;
	}

	std::memcpy(&config_, mapped_base, sizeof(AxiomModelConfig));
	if (config_.magic != 0x4D4F4958U || config_.num_layers == 0 || config_.hidden_dim == 0 ||
	    config_.head_dim == 0 || config_.num_q_heads == 0 || config_.num_kv_heads == 0 ||
	    config_.feedforward_network_dimension == 0 || config_.vocab_size == 0 ||
	    config_.max_seq_len == 0) {
		std::cerr << "[Axiom Engine] Invalid .axiom model header in: " << filepath << "\n";
		UnmapViewOfFile(mapped_base);
		CloseHandle(hMap);
		CloseHandle(hFile);
		return false;
	}
	if (config_.max_seq_len > 16384) {
		config_.max_seq_len = 16384;
	}
	const uint8_t* host_weights_ptr = mapped_base + 64;
	size_t weights_bytes = total_bytes - 64;

	const size_t D = config_.hidden_dim;
	const size_t Qd = config_.num_q_heads * config_.head_dim;
	const size_t KVd = config_.num_kv_heads * config_.head_dim;
	const size_t FFN = config_.feedforward_network_dimension;
	const size_t V = config_.vocab_size;
	const size_t per_layer_elements = D + (Qd * D) + Qd + (KVd * D) + KVd + (KVd * D) + KVd +
					  (D * Qd) + D + (FFN * D) + (FFN * D) + (D * FFN);
	const size_t min_required_elements =
		(V * D) + (static_cast<size_t>(config_.num_layers) * per_layer_elements) + D;
	if (weights_bytes < min_required_elements * sizeof(uint16_t)) {
		std::cerr << "[Axiom Engine] Truncated .axiom weights payload in: " << filepath
			  << "\n";
		UnmapViewOfFile(mapped_base);
		CloseHandle(hMap);
		CloseHandle(hFile);
		return false;
	}

	if (d_weights_arena_) {
		free_device_memory(d_weights_arena_);
		d_weights_arena_ = nullptr;
	}

	d_weights_arena_ = allocate_device_memory(weights_bytes);
	if (!d_weights_arena_) {
		std::cerr << "[Axiom Engine] Failed to allocate device memory for weights arena ("
			  << (weights_bytes >> 20) << " MB)\n";
		UnmapViewOfFile(mapped_base);
		CloseHandle(hMap);
		CloseHandle(hFile);
		return false;
	}

	const size_t chunk_bytes = 256 * 1024 * 1024;
	for (size_t offset = 0; offset < weights_bytes; offset += chunk_bytes) {
		size_t to_copy = std::min(chunk_bytes, weights_bytes - offset);
		if (!upload_device_buffer(static_cast<uint8_t*>(d_weights_arena_) + offset,
					  host_weights_ptr + offset,
					  to_copy)) {
			std::cerr
				<< "[Axiom Engine] Failed to upload weights chunk to VRAM at offset "
				<< (offset >> 20) << " MB.\n";
			UnmapViewOfFile(mapped_base);
			CloseHandle(hMap);
			CloseHandle(hFile);
			return false;
		}
	}

	UnmapViewOfFile(mapped_base);
	CloseHandle(hMap);
	CloseHandle(hFile);

	const uint16_t* cursor = static_cast<const uint16_t*>(d_weights_arena_);

	d_embed_tokens_ = cursor;
	cursor += V * D;
	layers_.resize(config_.num_layers);

	for (uint32_t l = 0; l < config_.num_layers; ++l) {
		layers_[l].d_input_layernorm = cursor;
		cursor += D;
		layers_[l].d_q_proj_weight = cursor;
		cursor += Qd * D;
		layers_[l].d_q_proj_bias = cursor;
		cursor += Qd;
		layers_[l].d_k_proj_weight = cursor;
		cursor += KVd * D;
		layers_[l].d_k_proj_bias = cursor;
		cursor += KVd;
		layers_[l].d_v_proj_weight = cursor;
		cursor += KVd * D;
		layers_[l].d_v_proj_bias = cursor;
		cursor += KVd;
		layers_[l].d_o_proj_weight = cursor;
		cursor += D * Qd;
		layers_[l].d_post_attn_layernorm = cursor;
		cursor += D;
		layers_[l].d_gate_proj_weight = cursor;
		cursor += FFN * D;
		layers_[l].d_up_proj_weight = cursor;
		cursor += FFN * D;
		layers_[l].d_down_proj_weight = cursor;
		cursor += D * FFN;
	}

	d_final_norm_weight_ = cursor;
	cursor += D;
	const size_t untied_elements = min_required_elements + (V * D);
	d_lm_head_weight_ =
		(weights_bytes >= untied_elements * sizeof(uint16_t)) ? cursor : d_embed_tokens_;

	// Allocate working activation buffers
	if (d_curr_token_)
		free_device_memory(d_curr_token_);
	if (d_curr_pos_)
		free_device_memory(d_curr_pos_);
	if (d_residual_stream_)
		free_device_memory(d_residual_stream_);
	if (d_normed_x_)
		free_device_memory(d_normed_x_);
	if (d_q_)
		free_device_memory(d_q_);
	if (d_k_)
		free_device_memory(d_k_);
	if (d_v_)
		free_device_memory(d_v_);
	if (d_attn_out_)
		free_device_memory(d_attn_out_);
	if (d_proj_tmp_)
		free_device_memory(d_proj_tmp_);
	if (d_gate_buf_)
		free_device_memory(d_gate_buf_);
	if (d_up_buf_)
		free_device_memory(d_up_buf_);
	if (d_act_buf_)
		free_device_memory(d_act_buf_);
	if (d_pred_id_)
		free_device_memory(d_pred_id_);
	if (d_logits_)
		free_device_memory(d_logits_);

	d_curr_token_ = allocate_device_memory(sizeof(uint32_t));
	d_curr_pos_ = allocate_device_memory(sizeof(int32_t));
	d_pred_id_ = allocate_device_memory(sizeof(uint32_t));
	d_logits_ = allocate_device_memory(config_.vocab_size * sizeof(float));
	d_residual_stream_ = allocate_device_memory(D * sizeof(uint16_t));
	d_normed_x_ = allocate_device_memory(D * sizeof(uint16_t));
	d_q_ = allocate_device_memory(Qd * sizeof(uint16_t));
	d_k_ = allocate_device_memory(KVd * sizeof(uint16_t));
	d_v_ = allocate_device_memory(KVd * sizeof(uint16_t));
	d_attn_out_ = allocate_device_memory(Qd * sizeof(uint16_t));
	d_proj_tmp_ = allocate_device_memory(std::max(D, Qd) * sizeof(uint16_t));
	d_gate_buf_ = allocate_device_memory(FFN * sizeof(uint16_t));
	d_up_buf_ = allocate_device_memory(FFN * sizeof(uint16_t));
	d_act_buf_ = allocate_device_memory(FFN * sizeof(uint16_t));

	// Allocate Paged KV Cache and physical block table
	int num_blocks = static_cast<int>((config_.max_seq_len + 15) / 16);
	if (num_blocks <= 0)
		num_blocks = 512;
	std::vector<int32_t> h_block_table(num_blocks);
	for (int i = 0; i < num_blocks; ++i) {
		h_block_table[i] = i;
	}
	if (d_block_table_)
		free_device_memory(d_block_table_);
	d_block_table_ = allocate_device_memory(num_blocks * sizeof(int32_t));
	upload_device_buffer(d_block_table_, h_block_table.data(), num_blocks * sizeof(int32_t));

	for (void* ptr : d_k_cache_)
		if (ptr)
			free_device_memory(ptr);
	for (void* ptr : d_v_cache_)
		if (ptr)
			free_device_memory(ptr);
	d_k_cache_.resize(config_.num_layers, nullptr);
	d_v_cache_.resize(config_.num_layers, nullptr);

	if (!d_curr_token_ || !d_curr_pos_ || !d_pred_id_ || !d_logits_ || !d_residual_stream_ ||
	    !d_normed_x_ || !d_q_ || !d_k_ || !d_v_ || !d_attn_out_ || !d_proj_tmp_ ||
	    !d_gate_buf_ || !d_up_buf_ || !d_act_buf_ || !d_block_table_) {
		std::cerr << "[Axiom Engine] Out of VRAM: Activation buffer allocation failed.\n";
		return false;
	}

	size_t kv_cache_bytes = static_cast<size_t>(config_.max_seq_len) * config_.num_kv_heads *
				config_.head_dim * sizeof(uint16_t);
	for (uint32_t l = 0; l < config_.num_layers; ++l) {
		d_k_cache_[l] = allocate_device_memory(kv_cache_bytes);
		d_v_cache_[l] = allocate_device_memory(kv_cache_bytes);
		if (!d_k_cache_[l] || !d_v_cache_[l]) {
			std::cerr
				<< "[Axiom Engine] Out of VRAM: KV cache allocation failed at layer "
				<< l << "\n";
			return false;
		}
	}

	cached_tokens_.clear();

	std::cout << "[Axiom Engine] Loaded model: " << config_.num_layers
		  << " layers, hidden_dim=" << config_.hidden_dim
		  << ", feedforward_network_dimension=" << config_.feedforward_network_dimension
		  << " (" << (weights_bytes >> 20) << " MB VRAM)\n";
	return true;
}

void AI::forward_transformer_step(int token_id, int token_pos, float* d_out_logits, void* stream) {
	const uint32_t D = config_.hidden_dim;
	const uint32_t Qd = config_.num_q_heads * config_.head_dim;
	const uint32_t KVd = config_.num_kv_heads * config_.head_dim;
	const uint32_t FFN = config_.feedforward_network_dimension;

	uint32_t u_token_id = static_cast<uint32_t>(token_id);
	int32_t s_token_pos = static_cast<int32_t>(token_pos);
	upload_device_buffer(d_curr_token_, &u_token_id, sizeof(uint32_t));
	upload_device_buffer(d_curr_pos_, &s_token_pos, sizeof(int32_t));

	launch_gather_embeddings_batched(static_cast<const uint32_t*>(d_curr_token_),
					 d_embed_tokens_,
					 d_residual_stream_,
					 1,
					 D,
					 config_.vocab_size,
					 stream);

	for (uint32_t l = 0; l < config_.num_layers; ++l) {
		const auto& w = layers_[l];

		launch_rmsnorm_bf16(d_residual_stream_,
				    w.d_input_layernorm,
				    d_normed_x_,
				    1,
				    D,
				    config_.rms_norm_eps,
				    stream);

		launch_cutlass_gemm_bf16(
			1, Qd, D, d_normed_x_, w.d_q_proj_weight, w.d_q_proj_bias, d_q_, stream);
		launch_cutlass_gemm_bf16(
			1, KVd, D, d_normed_x_, w.d_k_proj_weight, w.d_k_proj_bias, d_k_, stream);
		launch_cutlass_gemm_bf16(
			1, KVd, D, d_normed_x_, w.d_v_proj_weight, w.d_v_proj_bias, d_v_, stream);

		launch_rope_inplace_bf16(d_q_,
					 d_k_,
					 static_cast<const int32_t*>(d_curr_pos_),
					 1,
					 config_.num_q_heads,
					 config_.num_kv_heads,
					 config_.head_dim,
					 config_.rope_theta,
					 stream);

		launch_append_paged_kv(d_k_,
				       d_v_,
				       d_k_cache_[l],
				       d_v_cache_[l],
				       static_cast<const int32_t*>(d_block_table_),
				       token_pos,
				       config_.num_kv_heads,
				       config_.head_dim,
				       stream);

		float scale = 1.0f / std::sqrt(static_cast<float>(config_.head_dim));
		launch_paged_gqa_decode(d_q_,
					d_k_cache_[l],
					d_v_cache_[l],
					static_cast<const int32_t*>(d_block_table_),
					d_attn_out_,
					token_pos + 1,
					config_.num_q_heads,
					config_.num_kv_heads,
					config_.head_dim,
					scale,
					stream);

		launch_cutlass_gemm_bf16(
			1, D, Qd, d_attn_out_, w.d_o_proj_weight, nullptr, d_proj_tmp_, stream);
		launch_residual_add_bf16(d_residual_stream_, d_proj_tmp_, D, stream);

		launch_rmsnorm_bf16(d_residual_stream_,
				    w.d_post_attn_layernorm,
				    d_normed_x_,
				    1,
				    D,
				    config_.rms_norm_eps,
				    stream);

		launch_cutlass_gemm_bf16(
			1, FFN, D, d_normed_x_, w.d_gate_proj_weight, nullptr, d_gate_buf_, stream);
		launch_cutlass_gemm_bf16(
			1, FFN, D, d_normed_x_, w.d_up_proj_weight, nullptr, d_up_buf_, stream);
		launch_swiglu_bf16(d_gate_buf_, d_up_buf_, d_act_buf_, FFN, stream);
		launch_cutlass_gemm_bf16(
			1, D, FFN, d_act_buf_, w.d_down_proj_weight, nullptr, d_proj_tmp_, stream);
		launch_residual_add_bf16(d_residual_stream_, d_proj_tmp_, D, stream);
	}

	launch_rmsnorm_bf16(d_residual_stream_,
			    d_final_norm_weight_,
			    d_normed_x_,
			    1,
			    D,
			    config_.rms_norm_eps,
			    stream);
	if (d_out_logits) {
		launch_tied_logits_bf16(d_normed_x_,
					d_lm_head_weight_,
					d_out_logits,
					1,
					config_.vocab_size,
					D,
					stream);
	}
}

bool AI::generate(const std::string& prompt,
		  size_t max_new_tokens,
		  float temperature,
		  int top_k,
		  float top_p,
		  std::function<void(const std::string&)> token_callback) {
	std::lock_guard<std::mutex> lock(inference_mutex_);
	if (!d_weights_arena_ || axiom_vocab_.empty()) {
		std::cerr << "[Axiom Engine] Cannot generate: model or vocabulary not loaded.\n";
		return false;
	}

	std::vector<uint32_t> prompt_tokens = Parser::tokenize_exact_code(prompt, axiom_vocab_);
	if (prompt_tokens.empty()) {
		return false;
	}

	// Reserve room for generation while allowing full VRAM context window (up to max_seq_len)
	const size_t reserve_tokens =
		std::min<size_t>(max_new_tokens + 64, config_.max_seq_len / 2);
	const size_t max_allowed_prompt = (config_.max_seq_len > reserve_tokens)
						  ? (config_.max_seq_len - reserve_tokens)
						  : (config_.max_seq_len / 2);

	if (prompt_tokens.size() > max_allowed_prompt) {
		size_t excess = prompt_tokens.size() - max_allowed_prompt;
		// Keep the head (system instructions + tool schema) and the tail (active file context + user query)
		const size_t head_keep = max_allowed_prompt / 3;
		prompt_tokens.erase(prompt_tokens.begin() + head_keep,
				    prompt_tokens.begin() + head_keep + excess);
	}

	size_t logits_bytes = config_.vocab_size * sizeof(float);
	float* d_logits = static_cast<float*>(d_logits_);
	bool allocated_local_logits = false;
	if (!d_logits) {
		d_logits = static_cast<float*>(allocate_device_memory(logits_bytes));
		allocated_local_logits = true;
	}
	if (!d_logits)
		return false;

	std::vector<float> h_logits(config_.vocab_size);

	// KV Prefix Cache: reuse already-prefilled tokens across multi-turn requests
	size_t prefix_match = 0;
	const size_t max_prefix = std::min(cached_tokens_.size(), prompt_tokens.size());
	while (prefix_match < max_prefix &&
	       cached_tokens_[prefix_match] == prompt_tokens[prefix_match]) {
		++prefix_match;
	}
	// Always re-run at least the final prompt token to produce fresh d_logits
	if (prefix_match >= prompt_tokens.size() && !prompt_tokens.empty()) {
		prefix_match = prompt_tokens.size() - 1;
	}

	// 1. Prefill only the uncached suffix of the prompt
	for (size_t i = prefix_match; i < prompt_tokens.size(); ++i) {
		float* target_logits = (i + 1 == prompt_tokens.size()) ? d_logits : nullptr;
		forward_transformer_step(
			prompt_tokens[i], static_cast<int>(i), target_logits, compute_stream_);
	}
	sync_cuda_stream(compute_stream_);

	cached_tokens_ = prompt_tokens;

	int current_pos = static_cast<int>(prompt_tokens.size());
	std::random_device rd;
	std::mt19937 rng(rd());
	std::vector<std::pair<float, uint32_t>> candidates;
	std::string generated_so_far;

	// 2. Autoregressive decode
	for (size_t step = 0; step < max_new_tokens; ++step) {
		if (current_pos >= static_cast<int>(config_.max_seq_len))
			break;

		uint32_t next_token = 0;
		if (temperature <= 0.01f && d_pred_id_) {
			launch_softmax_argmax(d_logits,
					      static_cast<uint32_t*>(d_pred_id_),
					      config_.vocab_size,
					      compute_stream_);
			sync_cuda_stream(compute_stream_);
			download_device_buffer(&h_pred_id_, d_pred_id_, sizeof(uint32_t));
			next_token = h_pred_id_;
			if (step == 0 && (next_token == 151643 || next_token == 151644 ||
					  next_token == 151645)) {
				download_device_buffer(h_logits.data(), d_logits, logits_bytes);
				float best_val = -1e30f;
				for (uint32_t v = 0; v < config_.vocab_size; ++v) {
					if (v >= 151643 && v <= 151645)
						continue;
					if (std::isfinite(h_logits[v]) && h_logits[v] > best_val) {
						best_val = h_logits[v];
						next_token = v;
					}
				}
			}
		} else {
			download_device_buffer(h_logits.data(), d_logits, logits_bytes);
			if (step == 0 && config_.vocab_size > 151645) {
				h_logits[151643] = -1e30f;
				h_logits[151644] = -1e30f;
				h_logits[151645] = -1e30f;
			}
			float inv_temp = 1.0f / std::max(temperature, 0.01f);
			candidates.clear();

			// Fast threshold filter: track running max to avoid sorting all 151,936 logits
			float max_scaled = -1e30f;
			for (uint32_t v = 0; v < config_.vocab_size; ++v) {
				if (std::isfinite(h_logits[v])) {
					float scaled = h_logits[v] * inv_temp;
					if (scaled > max_scaled)
						max_scaled = scaled;
				}
			}
			// Any logit more than 20.0 below max_scaled has exp(-20) < 2e-9 probability
			const float cutoff_logit = max_scaled - 20.0f;
			for (uint32_t v = 0; v < config_.vocab_size; ++v) {
				if (std::isfinite(h_logits[v])) {
					float scaled = h_logits[v] * inv_temp;
					if (scaled >= cutoff_logit) {
						candidates.emplace_back(scaled, v);
					}
				}
			}

			if (candidates.empty()) {
				next_token = 0;
			} else {
				const size_t effective_k =
					(top_k > 0) ? std::min(static_cast<size_t>(top_k),
							       candidates.size())
						    : std::min<size_t>(64, candidates.size());
				if (effective_k < candidates.size()) {
					std::partial_sort(candidates.begin(),
							  candidates.begin() + effective_k,
							  candidates.end(),
							  [](const auto& a, const auto& b) {
								  return a.first > b.first;
							  });
					candidates.resize(effective_k);
				} else {
					std::sort(candidates.begin(),
						  candidates.end(),
						  [](const auto& a, const auto& b) {
							  return a.first > b.first;
						  });
				}

				float max_val = candidates[0].first;
				float sum_exp = 0.0f;
				for (auto& c : candidates) {
					c.first = std::exp(c.first - max_val);
					sum_exp += c.first;
				}
				if (sum_exp > 0.0f && top_p > 0.0f && top_p < 1.0f) {
					float cumulative = 0.0f;
					size_t cutoff = candidates.size();
					for (size_t idx = 0; idx < candidates.size(); ++idx) {
						cumulative += candidates[idx].first / sum_exp;
						if (cumulative >= top_p) {
							cutoff = idx + 1;
							break;
						}
					}
					candidates.resize(cutoff);
					sum_exp = 0.0f;
					for (const auto& c : candidates) {
						sum_exp += c.first;
					}
				}
				std::uniform_real_distribution<float> dist(0.0f, sum_exp);
				float r = dist(rng);
				float accum = 0.0f;
				next_token = candidates[0].second;
				for (const auto& c : candidates) {
					accum += c.first;
					if (accum >= r) {
						next_token = c.second;
						break;
					}
				}
			}
		}

		// Check EOS (<|endoftext|> = 151643, <|im_start|> = 151644, <|im_end|> = 151645)
		if (next_token == 151643 || next_token == 151644 || next_token == 151645) {
			break;
		}

		cached_tokens_.push_back(next_token);

		auto it = axiom_reverse_vocab_.find(next_token);
		if (it != axiom_reverse_vocab_.end()) {
			generated_so_far += it->second;
			if (token_callback)
				token_callback(it->second);
		}

		forward_transformer_step(next_token, current_pos, d_logits, compute_stream_);
		sync_cuda_stream(compute_stream_);
		current_pos++;
	}

	if (allocated_local_logits) {
		free_device_memory(d_logits);
	}
	return true;
}
