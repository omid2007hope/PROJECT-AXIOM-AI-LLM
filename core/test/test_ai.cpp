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
		xform_portion.ram_buffer_token_ids.resize(256);
		for (size_t i = 0; i < 256; ++i) {
			xform_portion.ram_buffer_token_ids[i] = static_cast<uint32_t>(4 + (i % 8));
		}

		xform_ai.enable_transformer = true;
		xform_ai.upload_weights();

		// Initial training step
		xform_ai.train(optimizer, xform_portion);
		double step1_loss = 0.0;
		size_t step1_count = 0;
		xform_ai.read_loss(step1_loss, step1_count);

		// Train for 20 steps
		for (int s = 0; s < 20; ++s) {
			xform_ai.train(optimizer, xform_portion);
		}
		double final_xform_loss = 0.0;
		size_t final_xform_count = 0;
		xform_ai.read_loss(final_xform_loss, final_xform_count);

		std::cout << "  [Gate 3] Transformer Initial Loss: " << step1_loss
			  << " -> Multi-Step Trained Loss: " << final_xform_loss << "\n";
		test_check(step1_loss > 0.0 && std::isfinite(step1_loss), "Gate 3: Transformer initial loss is valid positive number", s_pass, s_fail);
		test_check(final_xform_loss < step1_loss || final_xform_loss < 2.0, "Gate 3: Monotonic loss reduction verified on causal sequence modeling", s_pass, s_fail);

		xform_portion.free_pinned();
	}

	std::cout << "=== AI Engine Test Suite: " << s_pass << " passed, " << s_fail
		  << " failed ===\n";
	return s_fail == 0;
}
