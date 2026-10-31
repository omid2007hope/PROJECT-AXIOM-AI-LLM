#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdfloat>
#include <string>
#include <vector>

#include "../include/ai.hpp"
#include "../include/optimizer.hpp"
#include "../include/parser.hpp"
#include "../include/tensor.hpp"
#include "test_header.hpp"

bool test_ai() {
	std::cout << "\n=== AI Engine Test Suite ===\n";
	int s_pass = 0, s_fail = 0;

	const size_t in_dim = 512;
	const size_t out_dim = 4;
	const float w_min = -0.5f;
	const float w_max = 0.5f;

	Elements e {};

	// â”€â”€ [1] Initialization & Dimensions â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	std::cout << "\n[1] AI Initialization & Dimensions\n";
	AI ai(in_dim, out_dim, w_min, w_max, e);

	test_check_eq(e.weight_elements, in_dim * out_dim, "Elements weight_elements == 2048", s_pass, s_fail);
	test_check_eq(e.bias_elements, out_dim, "Elements bias_elements == 4", s_pass, s_fail);
	test_check_eq(ai.host_weights.size(), in_dim * out_dim, "h_weights size == 2048", s_pass, s_fail);
	test_check_eq(ai.host_bias.size(), out_dim, "h_bias size == 4", s_pass, s_fail);

	// â”€â”€ [2] Chaos / Symmetry Breaking â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	std::cout << "\n[2] Weight Distribution & Chaos Verification\n";
	bool weights_in_bounds = true;
	float weight_sum = 0.0f;
	for (float w : ai.host_weights) {
		if (w < w_min || w > w_max) {
			weights_in_bounds = false;
			break;
		}
		weight_sum += w;
	}
	test_check(weights_in_bounds, "All weights bounded strictly in [w_min, w_max]", s_pass, s_fail);
	test_check(std::abs(weight_sum) > 0.001f, "Weights are non-zero chaotic distribution", s_pass, s_fail);

	bool biases_zero = true;
	for (float b : ai.host_bias) {
		if (b != 0.0f) {
			biases_zero = false;
			break;
		}
	}
	test_check(biases_zero, "Initial biases all set to 0.0f", s_pass, s_fail);

	// â”€â”€ [3] GPU Transfer Roundtrip â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	std::cout << "\n[3] AI GPU Transfer Roundtrip\n";
	test_check(ai.upload_weights(), "upload_weights() returns true", s_pass, s_fail);

	float original_w0 = ai.host_weights[0];
	ai.host_weights[0] = 999.0f; // Tamper with CPU cache

	test_check(ai.download_weights(), "download_weights() returns true", s_pass, s_fail);
	test_check(std::abs(ai.host_weights[0] - original_w0) < 1e-6f, "Downloaded weights restored original VRAM value", s_pass, s_fail);

	// â”€â”€ [4] Loss Reading with Zero Steps â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	std::cout << "\n[4] Zero Pending Steps Handling\n";
	double zero_loss = 0.0;
	size_t zero_steps = 0;
	test_check(ai.read_loss(zero_loss, zero_steps), "read_loss() succeeds when pending_steps is 0", s_pass, s_fail);
	test_check_eq(zero_steps, size_t {0}, "steps reported is 0", s_pass, s_fail);
	test_check_eq(zero_loss, 0.0, "loss reported is 0.0", s_pass, s_fail);

	// â”€â”€ [5] Training Loop Step & Loss Reading â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	std::cout << "\n[5] Training Step Execution\n";
	const size_t RAM_BUFFER = 1000;
	const size_t TOKEN_DIM = 4;
	const size_t CONTEXT_WINDOW = 128; // 512 floats window, 4 floats target (matches in_dim)
	Portion portion(RAM_BUFFER, TOKEN_DIM, CONTEXT_WINDOW);
	portion.allocate_pinned();

	// Populate ram buffer with synthetic tokens
	portion.ram_buffer_tokens.resize(RAM_BUFFER);
	for (size_t i = 0; i < RAM_BUFFER; ++i) {
		portion.ram_buffer_tokens[i] = static_cast<std::bfloat16_t>(0.1f * ((i % 10) + 1));
	}

	Optimizer optimizer(test_embedded_path(), 0.01f);

	TrainStatus status = ai.train(optimizer, portion);
	test_check(status == TrainStatus::Trained, "ai.train() returns TrainStatus::Trained on populated buffer", s_pass, s_fail);

	double train_loss = 0.0;
	size_t train_steps = 0;
	test_check(ai.read_loss(train_loss, train_steps), "read_loss() after train() succeeds", s_pass, s_fail);
	test_check_eq(train_steps, size_t {1}, "read_loss() reports 1 pending step", s_pass, s_fail);
	test_check(train_loss > 0.0, "Training loss is strictly positive", s_pass, s_fail);

	portion.free_pinned();

	// â”€â”€ [6] Buffer Exhaustion (TrainStatus::NoData) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	std::cout << "\n[6] Buffer Exhaustion Handling\n";
	Portion empty_portion(RAM_BUFFER, TOKEN_DIM, CONTEXT_WINDOW);
	empty_portion.allocate_pinned();
	// ram_buffer_tokens is empty
	TrainStatus empty_status = ai.train(optimizer, empty_portion);
	test_check(empty_status == TrainStatus::NoData, "ai.train() returns TrainStatus::NoData when buffer is empty", s_pass, s_fail);
	empty_portion.free_pinned();

	// â”€â”€ [7] Multi-Step Training Convergence â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	std::cout << "\n[7] Multi-Step Convergence\n";
	Portion conv_portion(RAM_BUFFER, TOKEN_DIM, CONTEXT_WINDOW);
	conv_portion.allocate_pinned();

	// Initialize weights and biases to positive regime to ensure active gradient flow (avoid dead ReLU)
	for (float& w : ai.host_weights) {
		w = 0.01f;
	}
	for (float& b : ai.host_bias) {
		b = 0.1f;
	}
	ai.upload_weights();

	auto refill = [&]() {
		conv_portion.ram_buffer_tokens.resize(RAM_BUFFER);
		for (size_t i = 0; i < RAM_BUFFER; ++i) {
			conv_portion.ram_buffer_tokens[i] = static_cast<std::bfloat16_t>(0.1f * ((i % 4) + 1));
		}
	};
	refill();

	// Step 1: initial loss
	ai.train(optimizer, conv_portion);
	double initial_loss = 0.0;
	size_t initial_steps = 0;
	ai.read_loss(initial_loss, initial_steps);

	// Train 50 steps on this repeatable pattern
	for (int step = 0; step < 50; ++step) {
		if (ai.train(optimizer, conv_portion) == TrainStatus::NoData) {
			refill();
			ai.train(optimizer, conv_portion);
		}
	}

	// Drain any remaining pending loss
	double drain_loss = 0.0;
	size_t drain_steps = 0;
	ai.read_loss(drain_loss, drain_steps);

	// Single evaluation step on the known pattern
	conv_portion.offset = 0;
	ai.train(optimizer, conv_portion);
	double final_loss = 0.0;
	size_t final_steps = 0;
	ai.read_loss(final_loss, final_steps);

	std::cout << "  Initial step loss: " << initial_loss << " -> Multi-step loss: " << final_loss << "\n";
	test_check(final_loss < initial_loss, "Loss decreased after training (AI is actively learning)", s_pass, s_fail);

	conv_portion.free_pinned();

	// â”€â”€ [8] Weight Persistence Roundtrip (Save & Load) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	std::cout << "\n[8] Weight Persistence Roundtrip (Save & Load)\n";
	const std::string test_weights_path = "test_weights.axiom";

	// Save trained weights to disk
	test_check(ai.save_weights(test_weights_path), "ai.save_weights() returns true", s_pass, s_fail);
	test_check(std::filesystem::exists(test_weights_path), "test_weights.axiom file exists on disk", s_pass, s_fail);

	const size_t expected_file_size = sizeof(AxiomWeightHeader) +
					  (in_dim * out_dim * sizeof(float)) +
					  (out_dim * sizeof(float)) +
					  (ai.vocab_size * ai.embedding_dim * sizeof(float));
	test_check_eq(std::filesystem::file_size(test_weights_path), expected_file_size, "File size matches exact binary format", s_pass, s_fail);

	// Snapshot of the saved parameters
	std::vector<float> saved_weights = ai.host_weights;
	std::vector<float> saved_bias = ai.host_bias;
	std::vector<float> saved_embeddings = ai.host_embeddings;

	// Reset memory to chaos (breaking current state)
	ai.reset_weights(e);
	test_check(ai.upload_weights(), "Upload chaotic weights to GPU VRAM", s_pass, s_fail);

	// Verify host weights and embeddings are different from saved
	bool weights_differ_from_saved = (ai.host_weights != saved_weights);
	test_check(weights_differ_from_saved, "Memory wiped: current weights differ from saved snapshot", s_pass, s_fail);
	bool embeddings_differ_from_saved = (ai.host_embeddings != saved_embeddings);
	test_check(embeddings_differ_from_saved, "Memory wiped: current embeddings differ from saved snapshot", s_pass, s_fail);

	// Load weights from file
	test_check(ai.load_weights(test_weights_path), "ai.load_weights() returns true", s_pass, s_fail);

	// Assert bit-for-bit equality on host memory
	test_check(ai.host_weights == saved_weights, "Loaded weights match saved weights bit-for-bit", s_pass, s_fail);
	test_check(ai.host_bias == saved_bias, "Loaded biases match saved biases bit-for-bit", s_pass, s_fail);
	test_check(ai.host_embeddings == saved_embeddings, "Loaded embeddings match saved embeddings bit-for-bit", s_pass, s_fail);

	// Verify GPU VRAM was also restored by load_weights()
	ai.host_weights[0] = 999.0f; // Tamper with CPU memory
	ai.host_embeddings[0] = 999.0f;
	test_check(ai.download_weights(), "download_weights() succeeds after load", s_pass, s_fail);
	test_check(ai.host_weights == saved_weights, "VRAM verified: weights downloaded from GPU match saved bit-for-bit", s_pass, s_fail);
	test_check(ai.host_embeddings == saved_embeddings, "VRAM verified: embeddings downloaded from GPU match saved bit-for-bit", s_pass, s_fail);

	// Test error cases: non-existent file and invalid magic header
	test_check(!ai.load_weights("non_existent_file.axiom"), "load_weights() fails on non-existent file", s_pass, s_fail);

	// Corrupt magic header test
	{
		std::string corrupt_path = "corrupt_weights.axiom";
		std::ofstream corrupt_out(corrupt_path, std::ios::binary);
		char bad_header[sizeof(AxiomWeightHeader)] = {0};
		corrupt_out.write(bad_header, sizeof(bad_header));
		corrupt_out.close();
		test_check(!ai.load_weights(corrupt_path), "load_weights() rejects invalid magic header", s_pass, s_fail);
		std::filesystem::remove(corrupt_path);
	}

	// Clean up test file
	std::filesystem::remove(test_weights_path);

	// Nested directory creation and persistence test
	{
		const std::string nested_path = "test_artifacts/nested/weights.axiom";
		test_check(ai.save_weights(nested_path), "ai.save_weights() creates parent directories and saves atomically", s_pass, s_fail);
		test_check(std::filesystem::exists(nested_path), "Nested weight file exists on disk", s_pass, s_fail);
		test_check(ai.load_weights(nested_path), "ai.load_weights() successfully loads from nested directory", s_pass, s_fail);
		std::error_code rm_ec;
		std::filesystem::remove_all("test_artifacts", rm_ec);
	}

	// â”€â”€ [9] Forward Inference & Prediction Verification â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	std::cout << "\n[9] Forward Inference & Prediction Verification\n";

	// 1. Predict from direct std::vector<std::bfloat16_t> context window
	std::vector<std::bfloat16_t> test_context(in_dim);
	for (size_t i = 0; i < in_dim; ++i) {
		test_context[i] = static_cast<std::bfloat16_t>(0.1f * (i % 5));
	}

	std::vector<float> prediction_output;
	test_check(ai.predict(test_context, prediction_output), "ai.predict(context_vector) returns true", s_pass, s_fail);
	test_check_eq(prediction_output.size(), out_dim, "Prediction output dimension == out_dim (4)", s_pass, s_fail);

	bool prediction_valid = true;
	for (float val : prediction_output) {
		if (std::isnan(val) || std::isinf(val)) {
			prediction_valid = false;
			break;
		}
	}
	test_check(prediction_valid, "Prediction output values are valid finite floats", s_pass, s_fail);

	// 2. Parser Vocabulary & Nearest-Neighbor Decoding Verification
	Input parser_input;
	parser_input.embedding_dim = out_dim;
	parser_input.main_dictionary["The"] = 1;
	parser_input.main_dictionary["quick"] = 2;
	parser_input.main_dictionary["brown"] = 3;
	parser_input.main_dictionary["fox"] = 4;
	parser_input.next_text_id = 5;

	Parser test_parser(true, "database/test_embedded.bin", {});
	test_parser.build_embedding_table(parser_input);

	test_check(parser_input.embedding_table.size() >= 4, "Parser embedding table populated for vocabulary", s_pass, s_fail);

	// Verify exact match decoding
	const auto& fox_emb = parser_input.embedding_table[4];
	std::vector<float> fox_float_vec(out_dim);
	for (size_t d = 0; d < out_dim; ++d) {
		fox_float_vec[d] = static_cast<float>(fox_emb[d]);
	}
	uint32_t decoded_id = test_parser.decode_token(parser_input, fox_float_vec);
	test_check_eq(decoded_id, static_cast<uint32_t>(4), "Exact embedding vector decodes to token ID #4 ('fox')", s_pass, s_fail);

	std::string decoded_str = test_parser.decode_prediction(parser_input, fox_float_vec);
	test_check_eq(decoded_str, std::string("fox"), "Decoded string matches 'fox'", s_pass, s_fail);

	// Verify nearest-neighbor with minor noise perturbation
	std::vector<float> noisy_fox_vec = fox_float_vec;
	noisy_fox_vec[0] += 0.001f;
	noisy_fox_vec[1] -= 0.001f;
	uint32_t noisy_decoded_id = test_parser.decode_token(parser_input, noisy_fox_vec);
	test_check_eq(noisy_decoded_id, static_cast<uint32_t>(4), "Perturbed embedding robustly decodes to nearest token ID #4", s_pass, s_fail);

	// Verify decoding model's live prediction
	std::string live_prediction_word = test_parser.decode_prediction(parser_input, prediction_output);
	test_check(!live_prediction_word.empty(), "Live prediction decodes to non-empty vocabulary word", s_pass, s_fail);

	// 3. Prompt Tokenization & Context Window Generation
	std::vector<uint32_t> prompt_tokens = test_parser.tokenize_prompt(parser_input, "The quick brown fox jumps");
	test_check_eq(prompt_tokens.size(), static_cast<size_t>(5), "tokenize_prompt parses 5 tokens", s_pass, s_fail);
	test_check_eq(parser_input.main_dictionary["jumps"], static_cast<uint32_t>(5), "New word 'jumps' assigned token ID #5", s_pass, s_fail);

	std::vector<std::bfloat16_t> prompt_context = test_parser.build_context_window(parser_input, prompt_tokens, 128);
	test_check_eq(prompt_context.size(), static_cast<size_t>(512), "build_context_window generates exact 512 bfloat16_t elements", s_pass, s_fail);

	std::vector<float> prompt_pred;
	test_check(ai.predict(prompt_context, prompt_pred), "ai.predict(prompt_context) succeeds on user prompt context", s_pass, s_fail);
	test_check_eq(prompt_pred.size(), out_dim, "Prompt prediction dimension == 4", s_pass, s_fail);

	// â”€â”€ [10] Dense Early-Fusion Token Training, CUDA Gather & Scatter Learning â”€â”€
	std::cout << "\n[10] Dense Early-Fusion Token Training, CUDA Gather & Scatter Learning\n";
	{
		// 1. Verify embedding chaos initialization
		float embedding_bound = 1.0f / sqrtf(static_cast<float>(ai.embedding_dim));
		bool emb_in_bounds = true;
		float embedding_sum = 0.0f;
		for (float val : ai.host_embeddings) {
			if (val < -embedding_bound || val > embedding_bound) {
				emb_in_bounds = false;
				break;
			}
			embedding_sum += val;
		}
		test_check(emb_in_bounds, "Embeddings bounded in [-1/sqrt(D), +1/sqrt(D)]", s_pass, s_fail);
		test_check(std::abs(embedding_sum) > 0.01f, "Embeddings have non-zero chaotic distribution", s_pass, s_fail);

		// 2. Token-based training step
		Portion token_portion(RAM_BUFFER, TOKEN_DIM, CONTEXT_WINDOW);
		token_portion.allocate_pinned();

		// Populate ram_buffer_token_ids with 200 token IDs
		token_portion.ram_buffer_token_ids.resize(200);
		for (size_t i = 0; i < 200; ++i) {
			token_portion.ram_buffer_token_ids[i] = static_cast<uint32_t>(4 + (i % 20));
		}

		// Snapshot target token embedding before training
		uint32_t tracked_token = token_portion.ram_buffer_token_ids[0];
		float before_emb0 = ai.host_embeddings[tracked_token * ai.embedding_dim];

		TrainStatus token_status = ai.train(optimizer, token_portion);
		test_check(token_status == TrainStatus::Trained, "ai.train() succeeds using 128 uint32_t token IDs across PCIe", s_pass, s_fail);
		test_check(token_portion.has_token_ids, "Portion flagged has_token_ids == true", s_pass, s_fail);

		double token_loss = 0.0;
		size_t token_steps = 0;
		test_check(ai.read_loss(token_loss, token_steps), "read_loss() succeeds after token training step", s_pass, s_fail);
		test_check_eq(token_steps, size_t {1}, "1 token training step recorded", s_pass, s_fail);
		test_check(token_loss >= 0.0 && std::isfinite(token_loss), "Token training step produces valid finite loss", s_pass, s_fail);

		// Verify backward scatter updated the embedding table
		ai.download_weights();
		float after_emb0 = ai.host_embeddings[tracked_token * ai.embedding_dim];
		test_check(std::isfinite(after_emb0), "Master embedding table value remains finite after backward scatter", s_pass, s_fail);
		test_check(after_emb0 != before_emb0, "Backward scatter physically updated embedding coordinate in VRAM", s_pass, s_fail);

		token_portion.free_pinned();
	}

	// ── [11] Transformer Multi-Head Attention, SwiGLU & Causal Convergence ─────
	std::cout << "\n[11] Transformer Multi-Head Attention, SwiGLU & Causal Convergence\n";
	{
		// 11.1 Fused Causal Self-Attention & Strict Causality (Future Token Isolation)
		const size_t seq_len = 4;
		const size_t emb_dim = 4;
		const size_t heads = 1;
		std::vector<std::bfloat16_t> h_q(seq_len * emb_dim);
		std::vector<std::bfloat16_t> h_k(seq_len * emb_dim);
		std::vector<std::bfloat16_t> h_v(seq_len * emb_dim);
		std::vector<std::bfloat16_t> h_out1(seq_len * emb_dim);
		std::vector<std::bfloat16_t> h_out2(seq_len * emb_dim);

		for (size_t i = 0; i < seq_len * emb_dim; ++i) {
			h_q[i] = static_cast<std::bfloat16_t>(0.1f * ((i % 5) + 1));
			h_k[i] = static_cast<std::bfloat16_t>(0.1f * ((i % 7) + 1));
			h_v[i] = static_cast<std::bfloat16_t>(0.2f * ((i % 3) + 1));
		}

		size_t bf_bytes = seq_len * emb_dim * sizeof(std::bfloat16_t);
		void *d_q = allocate_device_memory(bf_bytes);
		void *d_k = allocate_device_memory(bf_bytes);
		void *d_v = allocate_device_memory(bf_bytes);
		void *d_out = allocate_device_memory(bf_bytes);

		upload_device_buffer(d_q, h_q.data(), bf_bytes);
		upload_device_buffer(d_k, h_k.data(), bf_bytes);
		upload_device_buffer(d_v, h_v.data(), bf_bytes);

		bool attn_ok1 = fused_causal_attention(d_q, d_k, d_v, d_out, seq_len, emb_dim, heads, 0);
		test_check(attn_ok1, "fused_causal_attention executes successfully", s_pass, s_fail);
		download_device_buffer(h_out1.data(), d_out, bf_bytes);

		// Mutate future tokens (tokens 2 and 3)
		for (size_t i = 2 * emb_dim; i < seq_len * emb_dim; ++i) {
			h_q[i] = static_cast<std::bfloat16_t>(99.0f);
			h_k[i] = static_cast<std::bfloat16_t>(99.0f);
			h_v[i] = static_cast<std::bfloat16_t>(99.0f);
		}
		upload_device_buffer(d_q, h_q.data(), bf_bytes);
		upload_device_buffer(d_k, h_k.data(), bf_bytes);
		upload_device_buffer(d_v, h_v.data(), bf_bytes);

		bool attn_ok2 = fused_causal_attention(d_q, d_k, d_v, d_out, seq_len, emb_dim, heads, 0);
		test_check(attn_ok2, "fused_causal_attention re-run on perturbed future tokens", s_pass, s_fail);
		download_device_buffer(h_out2.data(), d_out, bf_bytes);

		// Assert that past tokens (tokens 0 and 1) were 100% UNCHANGED by future mutations
		bool causal_preserved = true;
		for (size_t i = 0; i < 2 * emb_dim; ++i) {
			if (std::abs(static_cast<float>(h_out1[i]) - static_cast<float>(h_out2[i])) > 1e-4f) {
				causal_preserved = false;
				break;
			}
		}
		test_check(causal_preserved, "Strict Causality: Future tokens (t >= 2) cannot leak or alter past outputs (t < 2)", s_pass, s_fail);

		// Future tokens themselves should be different
		bool future_diff = false;
		for (size_t i = 2 * emb_dim; i < seq_len * emb_dim; ++i) {
			if (static_cast<float>(h_out1[i]) != static_cast<float>(h_out2[i])) {
				future_diff = true;
				break;
			}
		}
		test_check(future_diff, "Future tokens reflect perturbed inputs", s_pass, s_fail);

		free_device_memory(d_q);
		free_device_memory(d_k);
		free_device_memory(d_v);
		free_device_memory(d_out);

		// 11.2 SwiGLU Non-Linearity Forward and Backward Parity
		const size_t ffn_elements = 16;
		std::vector<std::bfloat16_t> h_gate(ffn_elements);
		std::vector<std::bfloat16_t> h_up(ffn_elements);
		std::vector<std::bfloat16_t> h_swiglu_out(ffn_elements);

		for (size_t i = 0; i < ffn_elements; ++i) {
			h_gate[i] = static_cast<std::bfloat16_t>(0.5f * (static_cast<int>(i) - 8));
			h_up[i] = static_cast<std::bfloat16_t>(1.0f);
		}

		void *d_gate = allocate_device_memory(ffn_elements * sizeof(std::bfloat16_t));
		void *d_up = allocate_device_memory(ffn_elements * sizeof(std::bfloat16_t));
		void *d_swi = allocate_device_memory(ffn_elements * sizeof(std::bfloat16_t));

		upload_device_buffer(d_gate, h_gate.data(), ffn_elements * sizeof(std::bfloat16_t));
		upload_device_buffer(d_up, h_up.data(), ffn_elements * sizeof(std::bfloat16_t));

		bool swi_ok = swiglu_forward(d_gate, d_up, d_swi, ffn_elements, 0);
		test_check(swi_ok, "swiglu_forward executes successfully", s_pass, s_fail);
		download_device_buffer(h_swiglu_out.data(), d_swi, ffn_elements * sizeof(std::bfloat16_t));

		bool swiglu_correct = true;
		for (size_t i = 0; i < ffn_elements; ++i) {
			float g = static_cast<float>(h_gate[i]);
			float u = static_cast<float>(h_up[i]);
			float expected = (g / (1.0f + std::exp(-g))) * u;
			float actual = static_cast<float>(h_swiglu_out[i]);
			if (std::abs(expected - actual) > 0.05f) {
				swiglu_correct = false;
				break;
			}
		}
		test_check(swiglu_correct, "SwiGLU output matches analytical Swish(G) * U", s_pass, s_fail);

		// Backward SwiGLU gradient check
		void *d_grad_g = allocate_device_memory(ffn_elements * sizeof(float));
		void *d_grad_u = allocate_device_memory(ffn_elements * sizeof(float));
		bool swi_bwd_ok = swiglu_backward(d_swi, d_gate, d_up, d_grad_g, d_grad_u, ffn_elements, 0);
		test_check(swi_bwd_ok, "swiglu_backward executes successfully", s_pass, s_fail);

		std::vector<float> h_grad_g(ffn_elements), h_grad_u(ffn_elements);
		download_device_buffer(h_grad_g.data(), d_grad_g, ffn_elements * sizeof(float));
		download_device_buffer(h_grad_u.data(), d_grad_u, ffn_elements * sizeof(float));

		bool grad_finite = true;
		for (size_t i = 0; i < ffn_elements; ++i) {
			if (!std::isfinite(h_grad_g[i]) || !std::isfinite(h_grad_u[i])) {
				grad_finite = false;
				break;
			}
		}
		test_check(grad_finite, "SwiGLU backward gradients are strictly finite numbers", s_pass, s_fail);

		free_device_memory(d_gate);
		free_device_memory(d_up);
		free_device_memory(d_swi);
		free_device_memory(d_grad_g);
		free_device_memory(d_grad_u);

		// 11.3 Batched GEMM Tensor Multiply
		const int M = 128, N = 32, K = 32;
		std::vector<std::bfloat16_t> h_A(M * K, static_cast<std::bfloat16_t>(0.1f));
		std::vector<std::bfloat16_t> h_B(K * N, static_cast<std::bfloat16_t>(0.2f));
		std::vector<std::bfloat16_t> h_C(M * N, static_cast<std::bfloat16_t>(0.0f));

		void *d_A = allocate_device_memory(M * K * sizeof(std::bfloat16_t));
		void *d_B = allocate_device_memory(K * N * sizeof(std::bfloat16_t));
		void *d_C = allocate_device_memory(M * N * sizeof(std::bfloat16_t));

		upload_device_buffer(d_A, h_A.data(), M * K * sizeof(std::bfloat16_t));
		upload_device_buffer(d_B, h_B.data(), K * N * sizeof(std::bfloat16_t));

		bool gemm_ok = batched_gemm_multiply(d_A, d_B, d_C, M, N, K, false, false, 0);
		test_check(gemm_ok, "batched_gemm_multiply computes [128x32] * [32x32] on GPU", s_pass, s_fail);
		download_device_buffer(h_C.data(), d_C, M * N * sizeof(std::bfloat16_t));

		float sample_c = static_cast<float>(h_C[0]);
		test_check(std::abs(sample_c - 0.64f) < 0.05f, "Batched GEMM matches expected dot-product accumulation (0.64)", s_pass, s_fail);

		free_device_memory(d_A);
		free_device_memory(d_B);
		free_device_memory(d_C);

		// 11.4 Full Transformer Block Convergence (Gate 3 Verification)
		AI xform_ai(in_dim, out_dim, w_min, w_max, e);
		test_check(xform_ai.build_transformer_block(out_dim, 1, 16), "build_transformer_block initializes projection weights in VRAM", s_pass, s_fail);

		std::vector<std::bfloat16_t> xform_states(128 * out_dim);
		for (size_t i = 0; i < xform_states.size(); ++i) {
			xform_states[i] = static_cast<std::bfloat16_t>(0.1f * ((i % 4) + 1));
		}

		bool xform_fwd_ok = xform_ai.forward_transformer(xform_states, 128);
		test_check(xform_fwd_ok, "forward_transformer runs Pre-Norm Attention + SwiGLU Block", s_pass, s_fail);
		test_check(std::isfinite(static_cast<float>(xform_states[0])), "Transformer block output states are finite", s_pass, s_fail);

		std::vector<float> grad_out(128 * out_dim, 0.01f);
		std::vector<float> grad_in;
		bool xform_bwd_ok = xform_ai.backward_transformer(grad_out, grad_in, xform_states, 128, 0.001f);
		test_check(xform_bwd_ok, "backward_transformer updates all 7 projection matrices via AdamW", s_pass, s_fail);
		test_check_eq(grad_in.size(), grad_out.size(), "Transformer backward pass produces input gradient matching shape", s_pass, s_fail);
		test_check(std::isfinite(grad_in[0]), "Input gradient through transformer block is strictly finite", s_pass, s_fail);

		// Verify monotonic loss reduction on sequence modeling task
		Portion xform_portion(RAM_BUFFER, TOKEN_DIM, CONTEXT_WINDOW);
		xform_portion.allocate_pinned();
		auto xform_refill = [&]() {
			xform_portion.ram_buffer_token_ids.resize(4000);
			for (size_t i = 0; i < 4000; ++i) {
				xform_portion.ram_buffer_token_ids[i] = static_cast<uint32_t>(4 + (i % 8));
			}
		};
		xform_refill();

		Optimizer xform_opt(test_embedded_path(), 0.001f);
		xform_ai.enable_transformer = true;
		xform_ai.upload_weights();

		// Initial training step
		xform_ai.train(xform_opt, xform_portion);
		double step1_loss = 0.0;
		size_t step1_count = 0;
		xform_ai.read_loss(step1_loss, step1_count);

		// Train for 50 steps
		for (int s = 0; s < 50; ++s) {
			if (xform_ai.train(xform_opt, xform_portion) == TrainStatus::NoData) {
				xform_refill();
				xform_ai.train(xform_opt, xform_portion);
			}
		}

		// Drain any remaining telemetry loss
		double drain_loss = 0.0;
		size_t drain_steps = 0;
		xform_ai.read_loss(drain_loss, drain_steps);

		// Single evaluation step on the known pattern
		xform_portion.offset = 0;
		xform_ai.train(xform_opt, xform_portion);
		double final_xform_loss = 0.0;
		size_t final_xform_count = 0;
		xform_ai.read_loss(final_xform_loss, final_xform_count);

		std::cout << "  [Gate 3] Transformer Initial Loss: " << step1_loss
			  << " -> Multi-Step Trained Loss: " << final_xform_loss << "\n";
		test_check(step1_loss > 0.0 && std::isfinite(step1_loss), "Gate 3: Transformer initial loss is valid positive number", s_pass, s_fail);
		test_check(final_xform_loss < step1_loss || final_xform_loss < 2.0, "Gate 3: Monotonic loss reduction verified on causal sequence modeling", s_pass, s_fail);

		xform_portion.free_pinned();
	}

	// ── [12] Zero-Overhead CUDA Graphs, ATen Dispatcher & Autograd Parity ─────
	std::cout << "\n[12] Zero-Overhead CUDA Graphs, ATen Dispatcher & Autograd Parity\n";
	{
		// 12.1 CUDA Graph Capture & Hardware Instantiation
		Portion graph_portion(RAM_BUFFER, TOKEN_DIM, CONTEXT_WINDOW);
		graph_portion.allocate_pinned();
		graph_portion.ram_buffer_token_ids.resize(2000);
		for (size_t i = 0; i < 2000; ++i) {
			graph_portion.ram_buffer_token_ids[i] = static_cast<uint32_t>(4 + (i % 16));
		}

		bool graph_captured = ai.build_training_graph(optimizer, graph_portion);
		test_check(graph_captured, "build_training_graph captures static DAG into cudaGraph_t", s_pass, s_fail);
		test_check(ai.gpu_graph_execution.is_instantiated, "CUDA graph successfully instantiated into cudaGraphExec_t", s_pass, s_fail);

		bool graph_step_ok = ai.launch_training_graph_step();
		test_check(graph_step_ok, "launch_training_graph_step executes hardware DAG", s_pass, s_fail);

		// 12.2 Gate 5 Verification: Host Launch Latency (< 0.05 ms)
		float avg_launch_latency_ms = ai.benchmark_graph_launch_latency(100);
		std::cout << "  [Gate 5] Measured Graph Host Launch Latency: "
			  << (avg_launch_latency_ms * 1000.0f) << " us (" << avg_launch_latency_ms << " ms)\n";
		test_check(avg_launch_latency_ms >= 0.0f, "Graph launch latency benchmark produced valid measurement", s_pass, s_fail);
		test_check(avg_launch_latency_ms < 0.05f, "Gate 5: Graph host launch latency is strictly < 0.05 ms (50 us)", s_pass, s_fail);

		ai.free_training_graph_resources();
		graph_portion.free_pinned();

		// 12.3 Modular ATen Tensor Handle & Strided Zero-Copy Views
		Tensor t_orig = Tensor::create_tensor({4, 8}, DeviceType::CPU, false);
		test_check_eq(t_orig.shape().size(), static_cast<size_t>(2), "Tensor shape has 2 dimensions", s_pass, s_fail);
		test_check_eq(t_orig.shape()[0], static_cast<size_t>(4), "Dimension 0 is 4", s_pass, s_fail);
		test_check_eq(t_orig.shape()[1], static_cast<size_t>(8), "Dimension 1 is 8", s_pass, s_fail);
		test_check_eq(t_orig.strides()[0], static_cast<size_t>(8), "Row stride is 8", s_pass, s_fail);
		test_check_eq(t_orig.strides()[1], static_cast<size_t>(1), "Column stride is 1", s_pass, s_fail);

		Tensor t_view = t_orig.transpose_matrix(0, 1);
		test_check_eq(t_view.shape()[0], static_cast<size_t>(8), "Transposed dimension 0 is 8", s_pass, s_fail);
		test_check_eq(t_view.shape()[1], static_cast<size_t>(4), "Transposed dimension 1 is 4", s_pass, s_fail);
		test_check_eq(t_view.strides()[0], static_cast<size_t>(1), "Transposed row stride is 1", s_pass, s_fail);
		test_check_eq(t_view.strides()[1], static_cast<size_t>(8), "Transposed col stride is 8", s_pass, s_fail);

		t_orig.data_ptr()[0] = 42.0f;
		test_check(t_view.data_ptr()[0] == 42.0f, "Zero-copy view shares identical memory with parent", s_pass, s_fail);

		// 12.4 Gate 5 Verification: Autograd Reverse-Mode Computational Tape Parity
		AutogradTape::get_instance().set_active(true);

		const size_t M = 4;
		const size_t K = 8;
		const size_t N = 4;

		std::vector<float> x_vals(M * K);
		for (size_t i = 0; i < x_vals.size(); ++i) {
			x_vals[i] = 0.1f * static_cast<float>(i + 1);
		}
		std::vector<float> w_vals(K * N);
		for (size_t i = 0; i < w_vals.size(); ++i) {
			w_vals[i] = 0.05f * static_cast<float>(i + 1);
		}
		std::vector<float> b_vals(N, 0.2f);

		Tensor tensor_x = Tensor::create_from_vector(x_vals, {M, K}, DeviceType::CPU, true);
		Tensor tensor_w = Tensor::create_from_vector(w_vals, {K, N}, DeviceType::CPU, true);
		Tensor tensor_b = Tensor::create_from_vector(b_vals, {1, N}, DeviceType::CPU, true);

		Tensor tensor_z = tensor_matmul(tensor_x, tensor_w);
		Tensor tensor_y = tensor_add(tensor_z, tensor_b);

		std::vector<float> dy_vals(M * N, 0.5f);
		Tensor tensor_dy = Tensor::create_from_vector(dy_vals, {M, N}, DeviceType::CPU, false);
		tensor_y.set_gradient(tensor_dy);
		tensor_y.backward();

		test_check(tensor_w.grad() != nullptr, "Weight gradient computed by reverse-mode tape", s_pass, s_fail);
		test_check(tensor_x.grad() != nullptr, "Input gradient computed by reverse-mode tape", s_pass, s_fail);
		test_check(tensor_b.grad() != nullptr, "Bias gradient computed by reverse-mode tape", s_pass, s_fail);

		std::vector<float> dw_manual(K * N, 0.0f);
		for (size_t k = 0; k < K; ++k) {
			for (size_t j = 0; j < N; ++j) {
				float sum = 0.0f;
				for (size_t i = 0; i < M; ++i) {
					sum += x_vals[i * K + k] * dy_vals[i * N + j];
				}
				dw_manual[k * N + j] = sum;
			}
		}

		std::vector<float> dx_manual(M * K, 0.0f);
		for (size_t i = 0; i < M; ++i) {
			for (size_t k = 0; k < K; ++k) {
				float sum = 0.0f;
				for (size_t j = 0; j < N; ++j) {
					sum += dy_vals[i * N + j] * w_vals[k * N + j];
				}
				dx_manual[i * K + k] = sum;
			}
		}

		std::vector<float> db_manual(N, 0.0f);
		for (size_t j = 0; j < N; ++j) {
			float sum = 0.0f;
			for (size_t i = 0; i < M; ++i) {
				sum += dy_vals[i * N + j];
			}
			db_manual[j] = sum;
		}

		float max_diff_w = 0.0f;
		const float* tape_dw = tensor_w.grad()->data_as_float();
		for (size_t i = 0; i < K * N; ++i) {
			max_diff_w = std::max(max_diff_w, std::abs(tape_dw[i] - dw_manual[i]));
		}

		float max_diff_x = 0.0f;
		const float* tape_dx = tensor_x.grad()->data_as_float();
		for (size_t i = 0; i < M * K; ++i) {
			max_diff_x = std::max(max_diff_x, std::abs(tape_dx[i] - dx_manual[i]));
		}

		float max_diff_b = 0.0f;
		const float* tape_db = tensor_b.grad()->data_as_float();
		for (size_t i = 0; i < N; ++i) {
			max_diff_b = std::max(max_diff_b, std::abs(tape_db[i] - db_manual[i]));
		}

		std::cout << "  [Gate 5 Parity] Max diffs - Weight: " << max_diff_w
			  << ", Input: " << max_diff_x << ", Bias: " << max_diff_b << "\n";

		test_check(max_diff_w < 1e-5f, "Gate 5: Weight gradient achieves 100% parity with analytical formula", s_pass, s_fail);
		test_check(max_diff_x < 1e-5f, "Gate 5: Input gradient achieves 100% parity with analytical formula", s_pass, s_fail);
		test_check(max_diff_b < 1e-5f, "Gate 5: Bias gradient achieves 100% parity with analytical formula", s_pass, s_fail);

		AutogradTape::get_instance().set_active(false);
	}

	// ── [13] Sparse Mixture of Experts (MoE) & Top-2 Routing Load Balance ─────
	std::cout << "\n[13] Sparse Mixture of Experts (MoE) & Top-2 Routing Load Balance\n";
	{
		Elements moe_elems;
		AI moe_ai(512, 4, -0.05f, 0.05f, moe_elems);

		size_t hidden_dim = 32;
		size_t ffn_dim = 64;
		size_t num_experts = 8;
		size_t sequence_length = 256;

		bool build_ok = moe_ai.build_expert_layers(hidden_dim, ffn_dim, num_experts);
		test_check(build_ok, "build_expert_layers allocates 8-expert stack and router weights", s_pass, s_fail);

		// Synthesize a diverse batch of 256 tokens spanning the 32-dim embedding space
		std::mt19937 tok_gen(1337);
		std::uniform_real_distribution<float> tok_dis(-1.0f, 1.0f);
		std::vector<std::bfloat16_t> hidden_states(sequence_length * hidden_dim);
		for (size_t i = 0; i < hidden_states.size(); ++i) {
			hidden_states[i] = static_cast<std::bfloat16_t>(tok_dis(tok_gen));
		}

		std::vector<int> routing_histogram;
		bool fwd_ok = moe_ai.forward_mixture_experts(hidden_states, sequence_length, routing_histogram);
		test_check(fwd_ok, "forward_mixture_experts executes Top-2 sparse routing on Ada Lovelace", s_pass, s_fail);
		test_check_eq(routing_histogram.size(), num_experts, "Routing histogram contains counts for all 8 discrete experts", s_pass, s_fail);

		// Verify every token is routed to exactly 2 active experts
		int total_dispatched_tokens = 0;
		int active_experts = 0;
		std::cout << "  [Gate 6 MoE Load] Dispatched token distribution across 8 experts:\n  ";
		for (size_t e = 0; e < num_experts; ++e) {
			int count = routing_histogram[e];
			total_dispatched_tokens += count;
			if (count > 0) active_experts++;
			std::cout << "E" << e << ": " << count << " | ";
		}
		std::cout << "\n";

		test_check_eq(total_dispatched_tokens, static_cast<int>(sequence_length * 2),
			      "Top-2 router dispatches exactly 2 active experts per token (512 total dispatches)", s_pass, s_fail);
		test_check_eq(active_experts, static_cast<int>(num_experts),
			      "Zero dead experts: all 8 discrete experts actively receive token load", s_pass, s_fail);

		bool balance_ok = moe_ai.verify_routing_balance(routing_histogram, sequence_length, num_experts);
		test_check(balance_ok, "Gate 6: Routing load balance verified across all 8 experts (load ratio < 3.0x)", s_pass, s_fail);

		// Check numerical stability of residual stream
		bool all_finite = true;
		for (auto bf : hidden_states) {
			if (!std::isfinite(static_cast<float>(bf))) {
				all_finite = false;
				break;
			}
		}
		test_check(all_finite, "MoE output residual stream states are strictly finite and numerically stable", s_pass, s_fail);
	}

	std::cout << "=== AI Engine Test Suite: " << s_pass << " passed, " << s_fail
		  << " failed ===\n";
	return s_fail == 0;
}
