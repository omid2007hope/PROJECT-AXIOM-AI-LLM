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

#include <cassert>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "../include/parser.hpp"
#include "test_header.hpp"

// ─── Minimal assertion helper ────────────────────────────────────────────────
static int s_pass = 0, s_fail = 0;

static void check(bool cond, const char* label) {
	if (cond) {
		std::cout << "  PASS [" << label << "]\n";
		++s_pass;
	} else {
		std::cerr << "  FAIL [" << label << "]\n";
		++s_fail;
	}
}

template <typename A, typename B>
static void check_eq(A actual, B expected, const char* label) {
	if (static_cast<B>(actual) == expected) {
		std::cout << "  PASS [" << label << "]: " << actual << "\n";
		++s_pass;
	} else {
		std::cerr << "  FAIL [" << label << "]: got " << actual << ", expected " << expected
			  << "\n";
		++s_fail;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
bool test_embeddings() {
	std::cout << "\n=== Embedding Test Suite ===\n";

	Input input;
	Test test = make_test_data();
	Parser parser(true, test_embedded_path(), test_asset_dirs());



	// ── [1] Tokenization produces non-zero output ────────────────────────────
	std::cout << "\n[1] Tokenization\n";
	std::vector<uint32_t> tokens = parser.tokenization(input, test);
	check(tokens.size() > 0, "tokenized_data non-empty");
	check_eq(
		tokens.size(), input.tokenized_data.size(), "return value == input.tokenized_data");

	// Main dictionary: text words and numbers unified
	check(input.main_dictionary.size() > 0, "main_dictionary populated");

	// Token ID ranges must not overlap between modalities
	uint32_t max_main = 0;
	for (auto& [k, v] : input.main_dictionary)
		max_main = std::max(max_main, v);
	uint32_t min_image = UINT32_MAX;
	for (auto& [k, v] : input.image_dictionary)
		min_image = std::min(min_image, v);
	check(max_main < min_image, "main IDs < image IDs (no overlap)");

	// ── [2] Embedding returns correct size ───────────────────────────────────
	std::cout << "\n[2] Embedding size\n";
	std::vector<std::bfloat16_t> emb = parser.embedding(input, test);
	size_t expected_size = input.tokenized_data.size() * config::token_dim;
	const bool in_ram = !emb.empty();
	if (in_ram) {
		check_eq(emb.size(), expected_size, "embedded.size == tokens * token_dim");
		check_eq(input.embedded_data.size(), expected_size, "input.embedded_data synced");
	} else {
		check_eq(input.embedded_data.size(), size_t {0}, "streamed large embedding dataset without OOM");
	}

	std::cout << "\n=== Embedded Data Test Result ===\n";
	std::vector<std::bfloat16_t> preview_emb;
	if (in_ram) {
		size_t n = std::min<size_t>(emb.size(), 1000);
		preview_emb.assign(emb.begin(), emb.begin() + static_cast<std::ptrdiff_t>(n));
	} else {
		std::ifstream disk_in(test_embedded_path(), std::ios::binary);
		if (disk_in.is_open()) {
			size_t n = std::min<size_t>(expected_size, 1000);
			preview_emb.resize(n);
			disk_in.read(reinterpret_cast<char*>(preview_emb.data()),
				     static_cast<std::streamsize>(n * sizeof(std::bfloat16_t)));
			size_t read_elems =
				static_cast<size_t>(disk_in.gcount()) / sizeof(std::bfloat16_t);
			preview_emb.resize(read_elems);
		}
	}
	std::cout << "Successfully generated "
		  << (in_ram ? input.embedded_data.size() : expected_size) << " elements!"
		  << (in_ram ? "\n" : " (streamed to disk)\n");
	std::cout << "First " << preview_emb.size() << ": ";
	for (size_t i = 0; i < preview_emb.size(); ++i) {
		std::cout << preview_emb[i] << " ";
	}
	std::cout << "\n";

	// ── [3] Embedding table has one entry per unique token ───────────────────
	std::cout << "\n[3] Embedding table\n";
	size_t unique_tokens = input.main_dictionary.size() +
			       input.image_dictionary.size() + input.video_dictionary.size() +
			       input.voice_dictionary.size();
	// embedding_table must be <= unique_tokens (only seen tokens get an entry)
	check(input.embedding_table.size() <= unique_tokens,
	      "embedding_table entries <= unique token count");
	check(input.embedding_table.size() > 0, "embedding_table non-empty");

	// Each entry must have exactly token_dim values
	bool all_dim_correct = true;
	for (auto& [id, vec] : input.embedding_table) {
		if (vec.size() != config::token_dim) {
			all_dim_correct = false;
			break;
		}
	}
	check(all_dim_correct, "every embedding vector has token_dim elements");

	// ── [4] BF16 precision: values are in [-1, 1] and non-zero ──────────────
	std::cout << "\n[4] BF16 value range\n";
	bool all_bounded = true, any_nonzero = false;
	if (in_ram) {
		for (auto v : emb) {
			double d = static_cast<double>(v);
			if (d < -1.5 || d > 1.5) {
				all_bounded = false;
			}
			if (d != 0.0) {
				any_nonzero = true;
			}
		}
	} else {
		for (const auto& [id, vec] : input.embedding_table) {
			for (auto v : vec) {
				double d = static_cast<double>(v);
				if (d < -1.5 || d > 1.5) {
					all_bounded = false;
				}
				if (d != 0.0) {
					any_nonzero = true;
				}
			}
		}
	}
	check(all_bounded, "all BF16 values in [-1.5, 1.5] (sin-based init)");
	check(any_nonzero, "at least one non-zero embedding value");

	// ── [5] BF16 type size ───────────────────────────────────────────────────
	std::cout << "\n[5] BF16 type size\n";
	check_eq(sizeof(std::bfloat16_t), size_t {2}, "sizeof(bfloat16_t) == 2 bytes");

	// ── [6] embedded_data.bin written to disk ───────────────────────────────
	std::cout << "\n[6] Disk output\n";
	std::filesystem::path bin = test_embedded_path();
	check(std::filesystem::exists(bin), "database/embedded_data.bin exists");
	if (std::filesystem::exists(bin)) {
		size_t file_bytes = std::filesystem::file_size(bin);
		size_t expected_bytes = expected_size * sizeof(std::bfloat16_t);
		check_eq(file_bytes,
			 expected_bytes,
			 "file size == embedded.size * sizeof(bfloat16_t)");
	}

	// ── [7] Re-run embedding is idempotent (small in-RAM slice; avoids multi-GB rewrite)
	std::cout << "\n[7] Idempotency\n";
	std::filesystem::path temp_bin = test_core_dir() / "test_temp_idempotent.bin";
	auto make_small = []() {
		Test small_test;
		small_test.test_input = {"The", "quick", "brown", "fox", "jumps", "over", "the", "lazy", "dog"};
		return small_test;
	};
	Input input_a, input_b;
	Test test_a = make_small();
	Test test_b = make_small();
	Parser parser_a(true, temp_bin, test_asset_dirs());
	Parser parser_b(true, temp_bin, test_asset_dirs());
	std::vector<uint32_t> tok_a = parser_a.tokenization(input_a, test_a);
	std::vector<uint32_t> tok_b = parser_b.tokenization(input_b, test_b);
	std::vector<std::bfloat16_t> emb_a = parser_a.embedding(input_a, test_a);
	std::vector<std::bfloat16_t> emb_b = parser_b.embedding(input_b, test_b);
	std::error_code rm_ec;
	std::filesystem::remove(temp_bin, rm_ec);

	check(tok_a == tok_b && !tok_a.empty(), "second run produces identical non-empty tokens");
	check(!emb_a.empty() && emb_a.size() == emb_b.size(),
	      "second run produces same non-empty embedding size");
	bool same_values = !emb_a.empty() && (emb_a.size() == emb_b.size()) &&
			   (input_a.embedding_table.size() == input_b.embedding_table.size());
	for (size_t i = 0; i < emb_a.size() && same_values; ++i) {
		if (emb_a[i] != emb_b[i]) {
			same_values = false;
		}
	}
	for (const auto& [id, vec] : input_a.embedding_table) {
		if (!same_values)
			break;
		auto it = input_b.embedding_table.find(id);
		if (it == input_b.embedding_table.end() || it->second != vec) {
			same_values = false;
		}
	}
	check(same_values, "deterministic: same embedding values on re-run");

	std::cout << "\n=== Embedding Test Suite: " << s_pass << " passed, " << s_fail
		  << " failed ===\n";
	int fails = s_fail;
	s_pass = s_fail = 0; // reset for next suite
	return fails == 0;
}
