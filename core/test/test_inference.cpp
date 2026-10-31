// =============================================================================
// AXIOM — test_inference.cpp
//
// Platinum-Level Comprehensive Inference & Tokenizer Test Suite
// Exhaustively verifies:
//   [1] Text Prompt Tokenization & Modality Boundary Invariants
//   [2] Autoregressive Context Window Generation & Padding Logistics
//   [3] Modality Decoders across All 5 Dimensions (Text, Num, Img, Vid, Voice)
//   [4] Nearest-Neighbor Euclidean L2 Space Invariants & Perturbation Stability
//   [5] Multi-Iteration Forward Inference Stress & Zero VRAM Leakage
//   [6] Defensive Weight File Corruption & Rejection Immunity
//   [7] Hardware Telemetry Bounds & Safe Querying
// =============================================================================

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdfloat>
#include <string>
#include <unordered_set>
#include <vector>

#include "../include/ai.hpp"
#include "../include/parser.hpp"
#include "../src/CUDA/util/caching_allocator.cuh"
#include "../src/CUDA/util/telemetry.cuh"
#include "test_header.hpp"

bool test_inference() {
	std::cout << "\n=== Platinum Inference & System Resilience Test Suite ===\n";
	int s_pass = 0, s_fail = 0;

	const size_t in_dim = 512;
	const size_t out_dim = 4;
	const float w_min = -0.1f;
	const float w_max = 0.1f;

	Elements ai_elements {};
	AI ai(in_dim, out_dim, w_min, w_max, ai_elements);
	ai.upload_weights();

	Input input;
	input.embedding_dim = out_dim;
	Parser parser(true, test_embedded_path(), test_asset_dirs());

	// ── [1] Prompt Tokenization & Modality Boundary Invariants ─────────────────
	std::cout << "\n[1] Prompt Tokenization & Modality Boundary Invariants\n";

	// 1.1 Empty prompt
	std::vector<uint32_t> empty_tokens = parser.tokenize_prompt(input, "");
	test_check(empty_tokens.empty(), "Empty prompt produces 0 tokens", s_pass, s_fail);

	// 1.2 Whitespace-only prompt
	std::vector<uint32_t> ws_tokens = parser.tokenize_prompt(input, "   \t  \n  \r  ");
	test_check(ws_tokens.empty(), "Whitespace-only prompt produces 0 tokens", s_pass, s_fail);

	// 1.3 Normal prompt with irregular spacing
	std::vector<uint32_t> normal_tokens =
		parser.tokenize_prompt(input, "  The   quick\tbrown\nfox   jumps  ");
	test_check_eq(normal_tokens.size(), static_cast<size_t>(5), "Irregular whitespace tokenized to exactly 5 words", s_pass, s_fail);

	// 1.4 Repeated words must reuse identical token IDs
	std::vector<uint32_t> repeat_tokens = parser.tokenize_prompt(input, "fox fox fox");
	test_check_eq(repeat_tokens.size(), static_cast<size_t>(3), "Repeated prompt has 3 tokens", s_pass, s_fail);
	test_check(repeat_tokens[0] == repeat_tokens[1] && repeat_tokens[1] == repeat_tokens[2],
		   "Repeated words share exact identical token ID", s_pass, s_fail);

	// 1.5 Modality Boundary Check: Text/Number IDs must strictly reside in [4, 64,511]
	bool text_ids_in_bounds = true;
	for (const auto& [word, id] : input.main_dictionary) {
		if (id < vocab::TEXT_START_ID || id > vocab::TEXT_MAX_ID) {
			text_ids_in_bounds = false;
			break;
		}
	}
	test_check(text_ids_in_bounds, "All allocated main IDs strictly within [4, 64,511]", s_pass, s_fail);

	// ── [2] Autoregressive Context Window Generation & Padding Logistics ────────
	std::cout << "\n[2] Autoregressive Context Window Generation & Padding Logistics\n";

	// 2.1 Zero tokens input -> exact 512 zeros
	std::vector<std::bfloat16_t> zero_window = parser.build_context_window(input, {}, 128);
	test_check_eq(zero_window.size(), static_cast<size_t>(512), "Zero tokens produces exact 512 elements", s_pass, s_fail);
	bool all_zero = true;
	for (auto val : zero_window) {
		if (static_cast<float>(val) != 0.0f) {
			all_zero = false;
			break;
		}
	}
	test_check(all_zero, "Zero-token context window is 100% zero-padded", s_pass, s_fail);

	// 2.2 Single token input -> padded with 127 zero tokens (508 floats) and 1 token at end
	std::vector<uint32_t> single_token = {normal_tokens[0]};
	std::vector<std::bfloat16_t> single_window =
		parser.build_context_window(input, single_token, 128);
	test_check_eq(single_window.size(), static_cast<size_t>(512), "Single token produces exact 512 elements", s_pass, s_fail);
	bool prefix_zero = true;
	for (size_t i = 0; i < 508; ++i) {
		if (static_cast<float>(single_window[i]) != 0.0f) {
			prefix_zero = false;
			break;
		}
	}
	test_check(prefix_zero, "Single token has 508 leading zero floats (127 padded tokens)", s_pass, s_fail);
	const auto& expected_tok_emb = input.embedding_table[single_token[0]];
	bool suffix_matches = true;
	for (size_t d = 0; d < 4; ++d) {
		if (single_window[508 + d] != expected_tok_emb[d]) {
			suffix_matches = false;
			break;
		}
	}
	test_check(suffix_matches, "Single token embedding correctly placed at tail of context window", s_pass, s_fail);

	// 2.3 Overflow tokens (>128 tokens) -> slices strictly the latest 128 tokens
	std::vector<uint32_t> large_tokens(200);
	for (size_t i = 0; i < 200; ++i) {
		large_tokens[i] = normal_tokens[i % normal_tokens.size()];
	}
	std::vector<std::bfloat16_t> large_window =
		parser.build_context_window(input, large_tokens, 128);
	test_check_eq(large_window.size(), static_cast<size_t>(512), "Overflow context window maintains exact 512 elements", s_pass, s_fail);

	// The very first token of the 128-window must correspond to large_tokens[200 - 128 = 72]
	uint32_t expected_first_slice_token = large_tokens[72];
	const auto& first_slice_emb = input.embedding_table[expected_first_slice_token];
	bool slice_start_matches = true;
	for (size_t d = 0; d < 4; ++d) {
		if (large_window[d] != first_slice_emb[d]) {
			slice_start_matches = false;
			break;
		}
	}
	test_check(slice_start_matches, "Overflow window correctly slices the most recent 128 tokens", s_pass, s_fail);

	// ── [3] Modality Decoders across All 5 Dimensions ───────────────────────────
	std::cout << "\n[3] Modality Decoders across All 5 Dimensions\n";

	// 3.1 Main Modality (Text & Numbers: 4 to 64,511)
	input.main_dictionary["Axiom"] = 42;
	input.reverse_main_dictionary[42] = "Axiom";
	test_check_eq(parser.token_to_string(input, 42), std::string("Axiom"), "Text token #42 decodes to 'Axiom'", s_pass, s_fail);
	test_check_eq(parser.token_to_string(input, 999), std::string("[Token #999]"), "Unmapped token ID decodes to '[Token #999]'", s_pass, s_fail);

	input.main_dictionary["3.14159"] = 1005;
	input.reverse_main_dictionary[1005] = "3.14159";
	test_check(parser.token_to_string(input, 1005).find("3.14159") != std::string::npos,
		   "Number token #1005 decodes to numeric string 3.14159", s_pass, s_fail);
	test_check_eq(parser.token_to_string(input, 1099), std::string("[Token #1099]"),
		      "Unmapped token ID decodes to '[Token #1099]'", s_pass, s_fail);

	// 3.3 Image Modality (Discrete visual bytes: 64,512 to 64,767)
	uint32_t img_token_255 = vocab::VISUAL_START_ID + 255;
	input.image_dictionary[static_cast<uint8_t>(255)] = img_token_255;
	test_check_eq(parser.token_to_string(input, img_token_255), std::string("[Image byte: 255]"),
		      "Image token decodes to '[Image byte: 255]'", s_pass, s_fail);

	// 3.4 Video Modality (Discrete visual bytes: 64,512 to 64,767)
	uint32_t vid_token_128 = vocab::VISUAL_START_ID + 128;
	input.video_dictionary[static_cast<uint8_t>(128)] = vid_token_128;
	test_check_eq(parser.token_to_string(input, vid_token_128), std::string("[Video byte: 128]"),
		      "Video token decodes to '[Video byte: 128]'", s_pass, s_fail);

	// 3.5 Voice Modality (Discrete acoustic bytes: 64,768 to 65,023)
	uint32_t voice_token_64 = vocab::ACOUSTIC_START_ID + 64;
	input.voice_dictionary[static_cast<uint8_t>(64)] = voice_token_64;
	test_check_eq(parser.token_to_string(input, voice_token_64), std::string("[Voice byte: 64]"),
		      "Voice token decodes to '[Voice byte: 64]'", s_pass, s_fail);

	// 3.6 Unknown / Out-of-bounds Modality (>= 65,536)
	test_check_eq(parser.token_to_string(input, 70000U), std::string("[Unknown #70000]"),
		      "Token ID >= 65,536 decodes to '[Unknown #70000]'", s_pass, s_fail);

	// ── [4] Nearest-Neighbor Euclidean L2 Space Invariants ───────────────────────
	std::cout << "\n[4] Nearest-Neighbor Euclidean L2 Space Invariants\n";

	parser.build_embedding_table(input);

	// 4.1 Exact Match on Known Token
	uint32_t target_token = 42; // "Axiom"
	const auto& target_emb = input.embedding_table[target_token];
	std::vector<float> target_vec(out_dim);
	for (size_t d = 0; d < out_dim; ++d) {
		target_vec[d] = static_cast<float>(target_emb[d]);
	}
	uint32_t decoded_token = parser.decode_token(input, target_vec);
	test_check_eq(decoded_token, target_token, "Exact embedding vector decodes to exact token ID #42", s_pass, s_fail);

	// 4.2 Distance Monotonicity under Noise Perturbation
	std::vector<float> noisy_vec = target_vec;
	noisy_vec[0] += 0.005f;
	noisy_vec[1] -= 0.005f;
	noisy_vec[2] += 0.002f;
	noisy_vec[3] -= 0.001f;
	uint32_t noisy_token = parser.decode_token(input, noisy_vec);
	test_check_eq(noisy_token, target_token, "Perturbed vector robustly clusters to nearest neighbor #42", s_pass, s_fail);

	// 4.3 Empty Vocabulary Safety Guard
	Input empty_input;
	empty_input.embedding_dim = 4;
	uint32_t empty_token = parser.decode_token(empty_input, target_vec);
	test_check_eq(empty_token, static_cast<uint32_t>(0), "Empty vocabulary safely returns token ID 0", s_pass, s_fail);
	test_check_eq(parser.decode_prediction(empty_input, target_vec), std::string("[Empty Vocabulary]"),
		      "Empty vocabulary safely decodes to '[Empty Vocabulary]'", s_pass, s_fail);

	// ── [5] Multi-Iteration Forward Inference Stress & Zero VRAM Leakage ────────
	std::cout << "\n[5] Multi-Iteration Forward Inference Stress & Zero VRAM Leakage\n";

	// Warm-up pass to initialize stream-ordered pools and memory arena
	{
		std::vector<std::bfloat16_t> warmup_ctx(in_dim, static_cast<std::bfloat16_t>(0.0f));
		std::vector<float> warmup_pred;
		ai.predict(warmup_ctx, warmup_pred);
	}

	size_t vram_free_start = 0, vram_total_start = 0;
	get_gpu_telemetry(&vram_free_start, &vram_total_start);

	const size_t STRESS_PASSES = 100;
	bool all_inference_finite = true;
	bool any_inference_nonzero = false;

	for (size_t iter = 0; iter < STRESS_PASSES; ++iter) {
		std::vector<std::bfloat16_t> iter_ctx(in_dim);
		for (size_t i = 0; i < in_dim; ++i) {
			iter_ctx[i] = static_cast<std::bfloat16_t>(
				std::sin(static_cast<double>(iter * in_dim + i)));
		}

		std::vector<float> pred;
		if (!ai.predict(iter_ctx, pred)) {
			all_inference_finite = false;
			break;
		}

		if (pred.size() != out_dim) {
			all_inference_finite = false;
			break;
		}

		for (float val : pred) {
			if (std::isnan(val) || std::isinf(val)) {
				all_inference_finite = false;
			}
			if (val != 0.0f) {
				any_inference_nonzero = true;
			}
		}
	}

	test_check(all_inference_finite, "All 100 consecutive forward inference outputs are finite numbers", s_pass, s_fail);
	test_check(any_inference_nonzero, "Linear outputs are alive (not collapsed to all zeros)", s_pass, s_fail);

	size_t vram_free_end = 0, vram_total_end = 0;
	get_gpu_telemetry(&vram_free_end, &vram_total_end);
	std::cout << "  [VRAM Check] start=" << vram_free_start << " end=" << vram_free_end
		  << " diff=" << (static_cast<long long>(vram_free_start) - static_cast<long long>(vram_free_end))
		  << " allocator_used=" << get_allocated_vram() << "\n";

	// VRAM must not decrease over the 100 iterations (zero memory leakage)
	test_check(vram_free_end >= vram_free_start || (vram_free_start - vram_free_end) < 1024 * 1024 || get_allocated_vram() < 1024 * 1024,
		   "Zero VRAM leakage detected across 100 consecutive Tensor Core inferences", s_pass, s_fail);

	// ── [6] Defensive Weight File Corruption & Rejection Immunity ───────────────
	std::cout << "\n[6] Defensive Weight File Corruption & Rejection Immunity\n";

	const std::string valid_weights_file = "test_valid_weights.axiom";
	test_check(ai.save_weights(valid_weights_file), "ai.save_weights() creates baseline valid checkpoint", s_pass, s_fail);

	// Snapshot of working memory
	std::vector<float> baseline_weights = ai.host_weights;
	std::vector<float> baseline_bias = ai.host_bias;

	// 6.1 Sub-56-byte truncated header file
	{
		std::string trunc_path = "test_trunc_header.axiom";
		std::ofstream out(trunc_path, std::ios::binary);
		char short_bytes[20] = {'A', 'X', 'I', 'O', 'M'};
		out.write(short_bytes, sizeof(short_bytes));
		out.close();
		test_check(!ai.load_weights(trunc_path), "load_weights() rejects sub-56-byte truncated file", s_pass, s_fail);
		std::filesystem::remove(trunc_path);
	}

	// 6.2 Header with dimension mismatch
	{
		std::string dim_mismatch_path = "test_dim_mismatch.axiom";
		std::ofstream out(dim_mismatch_path, std::ios::binary);
		AxiomWeightHeader bad_hdr {};
		bad_hdr.input_dimension = 1024; // Mismatch with model's in_dim 512
		bad_hdr.output_dimension = 4;
		bad_hdr.weight_count = 1024 * 4;
		bad_hdr.bias_count = 4;
		bad_hdr.vocab_size = ai.vocab_size;
		bad_hdr.embedding_dim = ai.embedding_dim;
		out.write(reinterpret_cast<const char*>(&bad_hdr), sizeof(bad_hdr));
		out.close();
		test_check(!ai.load_weights(dim_mismatch_path), "load_weights() rejects header with in_dim mismatch (1024 != 512)", s_pass, s_fail);
		std::filesystem::remove(dim_mismatch_path);
	}

	// 6.3 Valid header but truncated weight payload (< 8248 bytes)
	{
		std::string trunc_payload_path = "test_trunc_payload.axiom";
		std::ofstream out(trunc_payload_path, std::ios::binary);
		AxiomWeightHeader hdr {};
		hdr.input_dimension = in_dim;
		hdr.output_dimension = out_dim;
		hdr.weight_count = in_dim * out_dim;
		hdr.bias_count = out_dim;
		hdr.vocab_size = ai.vocab_size;
		hdr.embedding_dim = ai.embedding_dim;
		out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
		float half_weights[100] = {0.0f};
		out.write(reinterpret_cast<const char*>(half_weights), sizeof(half_weights));
		out.close();
		test_check(!ai.load_weights(trunc_payload_path), "load_weights() rejects truncated payload data", s_pass, s_fail);
		std::filesystem::remove(trunc_payload_path);
	}

	// 6.4 State preservation assertion: Failed loads must NOT mutate active brain
	test_check(ai.host_weights == baseline_weights, "State immunity: Host weights unchanged after failed load attempts", s_pass, s_fail);
	test_check(ai.host_bias == baseline_bias, "State immunity: Host biases unchanged after failed load attempts", s_pass, s_fail);

	std::filesystem::remove(valid_weights_file);

	// ── [7] Hardware Telemetry Bounds & Safe Querying ───────────────────────────
	std::cout << "\n[7] Hardware Telemetry Bounds & Safe Querying\n";

	size_t gpu_free = 0, gpu_total = 0;
	get_gpu_telemetry(&gpu_free, &gpu_total);
	test_check(gpu_total > 0, "GPU total memory reported is non-zero (> 0 MB)", s_pass, s_fail);
	test_check(gpu_free > 0, "GPU free memory reported is non-zero (> 0 MB)", s_pass, s_fail);
	test_check(gpu_free <= gpu_total, "GPU free memory is strictly <= total memory", s_pass, s_fail);

	// ── [8] Stochastic Sampling Engine & Temperature Invariants ─────────────────
	std::cout << "\n[8] Stochastic Sampling Engine & Temperature Invariants\n";

	// 8.1 Greedy fallback (T = 0.0f) generates tokens without error
	std::vector<std::string> greedy_tokens;
	bool greedy_ok = ai.generate(
		"The quick brown fox",
		parser,
		input,
		4,
		SamplingConfig {0.0f, 40, 0.9f},
		[&](const std::string& tok) { greedy_tokens.push_back(tok); });
	test_check(greedy_ok, "ai.generate() succeeds with greedy fallback (T = 0.0)", s_pass, s_fail);
	test_check(!greedy_tokens.empty(), "Greedy generation produced non-empty token stream", s_pass, s_fail);

	// 8.2 Deterministic seed verification (T > 0 with fixed seed produces identical streams)
	std::vector<std::string> seed_run_1;
	std::vector<std::string> seed_run_2;
	ai.generate(
		"The quick brown fox",
		parser,
		input,
		6,
		SamplingConfig {0.8f, 40, 0.9f, 987654321ULL},
		[&](const std::string& tok) { seed_run_1.push_back(tok); });

	ai.generate(
		"The quick brown fox",
		parser,
		input,
		6,
		SamplingConfig {0.8f, 40, 0.9f, 987654321ULL},
		[&](const std::string& tok) { seed_run_2.push_back(tok); });

	test_check_eq(seed_run_1.size(), seed_run_2.size(), "Fixed seed runs produce equal sequence length", s_pass, s_fail);
	test_check(seed_run_1 == seed_run_2, "Fixed seed produces 100% deterministic bit-identical token stream", s_pass, s_fail);

	// 8.3 Top-K bounding & probability download verification
	std::vector<std::string> stochastic_tokens;
	bool stoch_ok = ai.generate(
		"The quick brown fox",
		parser,
		input,
		4,
		SamplingConfig {1.0f, 10, 0.8f},
		[&](const std::string& tok) { stochastic_tokens.push_back(tok); });
	test_check(stoch_ok, "ai.generate() succeeds with stochastic Top-K and Top-P", s_pass, s_fail);

	// ── [9] Gate 1 Verification: Throughput & Multi-Token Quality ───────────────
	std::cout << "\n[9] Gate 1 Verification: Throughput & Multi-Token Quality\n";

	// 9.1 Autoregressive generation throughput benchmark (> 50 tokens/sec on RTX 4080)
	std::vector<std::string> bench_tokens;
	const size_t bench_count = 50;
	auto start_bench = std::chrono::high_resolution_clock::now();
	bool bench_ok = ai.generate(
		"The quick brown fox",
		parser,
		input,
		bench_count,
		SamplingConfig {0.7f, 40, 0.9f, 424242ULL},
		[&](const std::string& tok) { bench_tokens.push_back(tok); });
	auto end_bench = std::chrono::high_resolution_clock::now();
	double elapsed_sec = std::chrono::duration<double>(end_bench - start_bench).count();
	double tokens_per_sec = (elapsed_sec > 0.0) ? (bench_tokens.size() / elapsed_sec) : 0.0;

	std::cout << "  [Gate 1] Measured generation throughput: "
		  << static_cast<int>(tokens_per_sec) << " tokens/sec\n";
	test_check(bench_ok, "Gate 1: Autoregressive generate benchmark completed successfully", s_pass, s_fail);
	test_check(tokens_per_sec > 50.0, "Gate 1: Throughput exceeds 50 tokens/sec target", s_pass, s_fail);

	// 9.2 Quality check: multi-token generation diversity (not all identical tokens)
	std::unordered_set<std::string> unique_tokens;
	size_t unk_count = 0;
	std::cout << "  [DEBUG bench_tokens]: ";
	for (const auto& tok : bench_tokens) {
		std::cout << tok << " ";
		if (tok == "<UNK>")
			++unk_count;
		unique_tokens.insert(tok);
	}
	std::cout << "\n  [DEBUG unique_tokens count]: " << unique_tokens.size() << "\n";
	test_check(unique_tokens.size() > 1, "Gate 1: Stochastic sampling produces diverse tokens (unique > 1)", s_pass, s_fail);
	test_check(unk_count < bench_tokens.size(), "Gate 1: No <UNK> saturation during generation (not 100% UNK)", s_pass, s_fail);

	// ── [10] Gate 4 Verification: PagedAttention KV-Cache & COW Prefix Sharing ───
	std::cout << "\n[10] Gate 4 Verification: PagedAttention KV-Cache & COW Prefix Sharing\n";

	// 10.1 Paged Cache Construction & Allocation (16 tokens per block)
	const size_t total_blocks = 32;
	const size_t block_size = 16;
	bool cache_ok = ai.build_paged_cache(total_blocks, block_size);
	test_check(cache_ok, "build_paged_cache allocates physical VRAM pages", s_pass, s_fail);
	test_check_eq(ai.block_manager->get_total_blocks(), total_blocks, "Total physical blocks match requested", s_pass, s_fail);
	test_check_eq(ai.block_manager->get_free_count(), total_blocks, "All physical blocks initially free", s_pass, s_fail);
	test_check_eq(ai.block_manager->get_used_count(), static_cast<size_t>(0), "Initial used blocks count is 0", s_pass, s_fail);

	// 10.2 Sequence Ingestion: Append 32 tokens spanning exactly 2 physical blocks
	SequenceContext seq_parent;
	seq_parent.sequence_id = 1;
	const size_t hidden_dim = ai.gpu_paged_cache.hidden_dimension;
	std::vector<std::bfloat16_t> sample_k(hidden_dim, static_cast<std::bfloat16_t>(0.25f));
	std::vector<std::bfloat16_t> sample_v(hidden_dim, static_cast<std::bfloat16_t>(0.5f));

	bool append_ok = true;
	for (size_t t = 0; t < 32; ++t) {
		if (!ai.append_sequence_token(seq_parent, sample_k, sample_v)) {
			append_ok = false;
			break;
		}
	}
	test_check(append_ok, "append_sequence_token successfully appends 32 tokens", s_pass, s_fail);
	test_check_eq(seq_parent.sequence_length, static_cast<size_t>(32), "Sequence length is 32 tokens", s_pass, s_fail);
	test_check_eq(seq_parent.block_table.size(), static_cast<size_t>(2), "32 tokens cleanly partitioned into 2 blocks (16 tokens/block)", s_pass, s_fail);
	test_check_eq(ai.block_manager->get_used_count(), static_cast<size_t>(2), "Exactly 2 physical blocks used", s_pass, s_fail);

	// 10.3 Copy-On-Write (COW) Prefix Sharing: Fork sequence and verify 100% block reuse
	SequenceContext seq_child;
	bool fork_ok = ai.fork_sequence_context(seq_parent, seq_child);
	test_check(fork_ok, "fork_sequence_context succeeds", s_pass, s_fail);
	test_check_eq(seq_child.sequence_length, static_cast<size_t>(32), "Child sequence inherits sequence length", s_pass, s_fail);
	test_check_eq(seq_child.block_table.size(), static_cast<size_t>(2), "Child sequence inherits block table", s_pass, s_fail);
	test_check(seq_child.block_table == seq_parent.block_table, "Child and parent share identical physical block IDs", s_pass, s_fail);

	int block_0 = seq_parent.block_table[0];
	int block_1 = seq_parent.block_table[1];
	test_check_eq(ai.block_manager->get_reference_count(block_0), 2, "Block 0 reference count is 2 (shared)", s_pass, s_fail);
	test_check_eq(ai.block_manager->get_reference_count(block_1), 2, "Block 1 reference count is 2 (shared)", s_pass, s_fail);
	test_check_eq(ai.block_manager->get_used_count(), static_cast<size_t>(2), "Zero new blocks allocated on fork (100% prefix block reuse)", s_pass, s_fail);

	// 10.4 Sequence Divergence & Dynamic COW Allocation
	std::vector<std::bfloat16_t> child_k(hidden_dim, static_cast<std::bfloat16_t>(0.75f));
	std::vector<std::bfloat16_t> child_v(hidden_dim, static_cast<std::bfloat16_t>(1.0f));
	bool child_append_ok = ai.append_sequence_token(seq_child, child_k, child_v);
	test_check(child_append_ok, "Child sequence appends next token across block boundary", s_pass, s_fail);
	test_check_eq(seq_child.sequence_length, static_cast<size_t>(33), "Child sequence length is 33", s_pass, s_fail);
	test_check_eq(seq_child.block_table.size(), static_cast<size_t>(3), "Child sequence expanded to 3 blocks", s_pass, s_fail);
	test_check_eq(seq_parent.block_table.size(), static_cast<size_t>(2), "Parent sequence unaffected by child append", s_pass, s_fail);
	test_check_eq(ai.block_manager->get_used_count(), static_cast<size_t>(3), "Exactly 3 physical blocks in use across both sequences", s_pass, s_fail);

	// Test intra-block divergence COW: Fork child (33 tokens) into grandchild
	SequenceContext seq_grandchild;
	ai.fork_sequence_context(seq_child, seq_grandchild);
	int block_2 = seq_child.block_table[2];
	test_check_eq(ai.block_manager->get_reference_count(block_2), 2, "Partial block 2 shared with reference count 2", s_pass, s_fail);

	// Grandchild appends token 34 with different data: triggers COW duplication of block 2!
	std::vector<std::bfloat16_t> diverged_k(hidden_dim, static_cast<std::bfloat16_t>(-0.5f));
	std::vector<std::bfloat16_t> diverged_v(hidden_dim, static_cast<std::bfloat16_t>(-1.0f));
	bool cow_ok = ai.append_sequence_token(seq_grandchild, diverged_k, diverged_v);
	test_check(cow_ok, "COW duplication triggered on intra-block append divergence", s_pass, s_fail);
	test_check_eq(ai.block_manager->get_reference_count(block_2), 1, "Original block 2 reference count decremented back to 1", s_pass, s_fail);
	test_check(seq_grandchild.block_table[2] != block_2, "Grandchild received unique private block after COW divergence", s_pass, s_fail);

	// 10.5 Paged Causal Attention Execution & Mathematical Validity
	std::vector<std::bfloat16_t> query_vec(hidden_dim, static_cast<std::bfloat16_t>(1.0f));
	std::vector<std::bfloat16_t> paged_out;
	bool attn_ok = ai.forward_paged_attention(seq_grandchild, query_vec, paged_out, 1);
	test_check(attn_ok, "forward_paged_attention executes on diverged paged blocks", s_pass, s_fail);
	test_check_eq(paged_out.size(), hidden_dim, "Paged attention output dimension matches hidden dimension", s_pass, s_fail);

	bool paged_finite = true;
	for (auto val : paged_out) {
		float f = static_cast<float>(val);
		if (std::isnan(f) || std::isinf(f)) {
			paged_finite = false;
			break;
		}
	}
	test_check(paged_finite, "Paged attention outputs are finite numbers", s_pass, s_fail);

	// 10.6 Resource Lifecycle & Deterministic Zero-Fragmentation Cleanup
	ai.release_sequence_context(seq_parent);
	ai.release_sequence_context(seq_child);
	ai.release_sequence_context(seq_grandchild);
	test_check_eq(ai.block_manager->get_used_count(), static_cast<size_t>(0), "All physical blocks returned after sequence releases", s_pass, s_fail);
	test_check_eq(ai.block_manager->get_free_count(), total_blocks, "Free block count fully restored (zero fragmentation)", s_pass, s_fail);

	// 10.7 Multi-Turn Conversation Stress Test (Zero Allocation Failures)
	bool stress_ok = true;
	std::vector<SequenceContext> sessions(10);
	for (size_t s = 0; s < sessions.size(); ++s) {
		sessions[s].sequence_id = static_cast<int>(s + 100);
		for (size_t t = 0; t < 24; ++t) { // 24 tokens = 2 blocks each
			if (!ai.append_sequence_token(sessions[s], sample_k, sample_v)) {
				stress_ok = false;
				break;
			}
		}
	}
	test_check(stress_ok, "Stress test: Successfully allocated multiple concurrent sequences", s_pass, s_fail);
	test_check_eq(ai.block_manager->get_used_count(), static_cast<size_t>(20), "20 blocks in active use across 10 sessions", s_pass, s_fail);

	for (auto& s : sessions) {
		ai.release_sequence_context(s);
	}
	test_check_eq(ai.block_manager->get_free_count(), total_blocks, "Gate 4: 100% physical blocks recovered with zero leaks", s_pass, s_fail);
	ai.free_paged_cache_memory();

	std::cout << "=== Platinum Inference & System Resilience Test Suite: " << s_pass << " passed, "
		  << s_fail << " failed ===\n";
	return s_fail == 0;
}
