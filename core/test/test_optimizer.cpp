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

#include <iostream>

#include "../include/config.hpp"
#include "../include/optimizer.hpp"
#include "../include/parser.hpp"
#include "test_header.hpp"

// ─── Minimal assertion helper ────────────────────────────────────────────────
static int s_pass = 0, s_fail = 0;

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
static void check(bool cond, const char* label) {
	if (cond) {
		std::cout << "  PASS [" << label << "]\n";
		++s_pass;
	} else {
		std::cerr << "  FAIL [" << label << "]\n";
		++s_fail;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
bool test_optimizer() {
	std::cout << "\n=== Optimizer Test Suite ===\n";

	// Dynamic configuration sourced directly from Config
	Config c;
	const size_t TOKEN_DIM = c.token_dim;
	const size_t RAM_BUFFER = c.ram_buffer_size;
	const size_t CONTEXT_WINDOW = c.context_window_size;
	const size_t WINDOW_ELEMENTS = CONTEXT_WINDOW * TOKEN_DIM;
	const size_t TARGET_ELEMENTS = 1 * TOKEN_DIM;

	// Expected windows per chunk: floor((RAM_BUFFER - WINDOW_ELEMENTS - TARGET_ELEMENTS) / TOKEN_DIM) + 1
	const size_t EXPECTED_WINDOWS = (RAM_BUFFER - WINDOW_ELEMENTS - TARGET_ELEMENTS) / TOKEN_DIM + 1;

	// ── [1] Initial Portion state ────────────────────────────────────────────
	std::cout << "\n[1] Initial Portion state\n";
	Portion portion(RAM_BUFFER, TOKEN_DIM, CONTEXT_WINDOW);
	check_eq(portion.ram_buffer_tokens.size(), size_t {0}, "ram_buffer empty");
	// Pinned buffers: nullptr until allocate_pinned() is called
	check(portion.context_window_tokens[0] == nullptr, "context_window_tokens == nullptr");
	check(portion.target_tokens[0] == nullptr, "target_tokens == nullptr");
	check(portion.buffer_in_use.load() == false, "buffer_in_use == false");
	check(portion.context_in_use.load() == false, "context_in_use == false");

	check_eq(portion.all_elements, WINDOW_ELEMENTS, "all_elements derived");
	check_eq(portion.target_elements, TARGET_ELEMENTS, "target_elements derived");

	// Allocate the pinned host buffers (cudaHostAllocWriteCombined)
	bool pinned_ok = portion.allocate_pinned();
	check(pinned_ok, "allocate_pinned() succeeded");
	check(portion.context_window_tokens[0] != nullptr, "context_window_tokens allocated");
	check(portion.target_tokens[0] != nullptr, "target_tokens allocated");

	Optimizer optimizer(test_embedded_path(), c.learning_rate);

	// ── [2] disk_to_ram — first load ─────────────────────────────────────────
	std::cout << "\n[2] disk_to_ram — first load\n";
	optimizer.disk_to_ram(portion);
	check_eq(portion.ram_buffer_tokens.size(), RAM_BUFFER, "ram_buffer filled to RAM_BUFFER");
	check(portion.buffer_in_use.load() == false, "buffer lock released after load");

	// ── [3] load_context_window — slide one window ──────────────────────────
	std::cout << "\n[3] load_context_window — slide " << EXPECTED_WINDOWS << " windows\n";
	optimizer.load_context_window(portion);

	// After the call the pinned buffers must be filled
	// (we check element count via the Portion size fields, not .size())
	check(portion.context_window_tokens[0] != nullptr, "context_window_tokens filled");
	check(portion.target_tokens[0] != nullptr, "target_tokens filled");
	// Validate first value is a valid (non-NaN) BF16 by casting to float
	{
		float v = static_cast<float>(portion.context_window_tokens[0][0]);
		check(v == v, "context_window_tokens[0] is not NaN");
	}
	check(portion.buffer_in_use.load() == false, "buffer lock released after windows");
	check(portion.context_in_use.load() == false, "context lock released after windows");

	// ── [4] disk_to_ram — second load (carry-forward) ────────────────────────
	std::cout << "\n[4] disk_to_ram — second load (carry-forward)\n";
	// ram_buffer_tokens still holds the old 100 elements.
	// disk_to_ram must: read carry (last WINDOW_ELEMENTS=16), clear, prepend carry,
	// then fill so total == RAM_BUFFER again.
	check_eq(portion.ram_buffer_tokens.size(),
		 RAM_BUFFER,
		 "buffer still holds data (not cleared by load_context_window)");
	optimizer.disk_to_ram(portion);
	check_eq(portion.ram_buffer_tokens.size(),
		 RAM_BUFFER,
		 "ram_buffer refilled to RAM_BUFFER on 2nd call");
	check(portion.buffer_in_use.load() == false, "buffer lock released 2nd load");

	// ── [5] Carry-forward correctness ────────────────────────────────────────
	std::cout << "\n[5] Carry-forward sanity\n";
	check(portion.ram_buffer_tokens.size() == RAM_BUFFER, "carry prepended + disk fill = RAM_BUFFER");
	// Every element at the carry positions should be a valid BF16 value
	bool carry_nonzero = false;
	for (size_t i = 0; i < WINDOW_ELEMENTS; ++i) {
		if (static_cast<double>(portion.ram_buffer_tokens[i]) != 0.0) {
			carry_nonzero = true;
			break;
		}
	}
	check(carry_nonzero, "carry-forward region contains non-zero values");

	// ── [6] Lock safety — buffer_in_use blocks disk_to_ram ───────────────────
	std::cout << "\n[6] Lock safety\n";
	{
		Portion locked(RAM_BUFFER, TOKEN_DIM, CONTEXT_WINDOW);
		locked.buffer_in_use.store(true, std::memory_order_relaxed);
		Optimizer opt2(test_embedded_path(), c.learning_rate);
		opt2.disk_to_ram(locked); // must be a no-op
		check_eq(locked.ram_buffer_tokens.size(),
			 size_t {0},
			 "disk_to_ram skipped when buffer_in_use=true");
	}
	{
		Portion locked(RAM_BUFFER, TOKEN_DIM, CONTEXT_WINDOW);
		locked.context_in_use.store(true, std::memory_order_relaxed);
		Optimizer opt3(test_embedded_path(), c.learning_rate);
		opt3.disk_to_ram(locked); // must be a no-op
		check_eq(locked.ram_buffer_tokens.size(),
			 size_t {0},
			 "disk_to_ram skipped when context_in_use=true");
	}

	// ── [7] Full 3-cycle training loop ───────────────────────────────────────
	std::cout << "\n[7] Full training cycle — 3 consecutive cycles\n";
	for (int cycle = 0; cycle < 3; ++cycle) {
		optimizer.disk_to_ram(portion);
		size_t buffer_size = portion.ram_buffer_tokens.size();
		optimizer.load_context_window(portion);

		std::cout << "  cycle " << cycle + 1 << ": ram=" << buffer_size
			  << " → ctx_elems=" << portion.all_elements
			  << " tgt_elems=" << portion.target_elements << "\n";

		check(portion.context_window_tokens[0] != nullptr, "ctx filled");
		check(portion.target_tokens[0] != nullptr, "tgt filled");
		check(portion.buffer_in_use.load() == false, "buffer unlocked after cycle");
		check(portion.context_in_use.load() == false, "context unlocked after cycle");
	}

	// ── Cleanup ───────────────────────────────────────────────────────────────
	portion.free_pinned();
	check(portion.context_window_tokens[0] == nullptr, "context_window_tokens freed");
	check(portion.target_tokens[0] == nullptr, "target_tokens freed");

	// ── [8] Lock-free SPSC Ring Buffer & Async Prefetch ───────────────────────
	std::cout << "\n[8] Lock-free SPSC Ring Buffer & Async Prefetch\n";
	{
		ChunkRingBuffer<4> ring_buffer;
		check(ring_buffer.can_produce(), "SPSC ring buffer initially can produce");
		check(!ring_buffer.can_consume(), "SPSC ring buffer initially empty");
		check_eq(ring_buffer.available_chunks(), size_t {0}, "0 available chunks");

		// Produce 1 chunk
		ChunkSlot& prod_slot = ring_buffer.get_producer_slot();
		prod_slot.elements = 100;
		ring_buffer.push_produced();

		check(ring_buffer.can_consume(), "SPSC ring buffer can consume after push");
		check_eq(ring_buffer.available_chunks(), size_t {1}, "1 available chunk");

		// Consume chunk
		ChunkSlot& cons_slot = ring_buffer.get_consumer_slot();
		check_eq(cons_slot.elements, size_t {100}, "Consumed slot has 100 elements");
		ring_buffer.pop_consumed();

		check(!ring_buffer.can_consume(), "SPSC ring buffer empty after pop");
		check_eq(ring_buffer.available_chunks(), size_t {0}, "0 available chunks after pop");

		// Test Optimizer prefetch controls
		Optimizer prefetch_opt(test_embedded_path(), c.learning_rate);
		Portion test_portion(RAM_BUFFER, TOKEN_DIM, CONTEXT_WINDOW);
		check(!prefetch_opt.is_prefetch_running(), "Prefetch not running initially");
		prefetch_opt.start_prefetch(test_portion);
		check(prefetch_opt.is_prefetch_running(), "Prefetch running after start_prefetch");
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		prefetch_opt.stop_prefetch();
		check(!prefetch_opt.is_prefetch_running(), "Prefetch stopped after stop_prefetch");
	}

	// ── [9] Token-ID Ocean -> Pool -> Glass Streaming Cycle ──────────────────
	std::cout << "\n[9] Token-ID Ocean -> Pool -> Glass Streaming Cycle\n";
	{
		const std::filesystem::path ocean_path =
			test_core_dir() / config::database_folder / "test_token_ocean.bin";
		constexpr size_t ocean_tokens = 1500;
		constexpr size_t pool_tokens = 600;
		std::vector<uint32_t> source_tokens(ocean_tokens);
		for (size_t i = 0; i < ocean_tokens; ++i) {
			source_tokens[i] = static_cast<uint32_t>(1000 + i);
		}

		Parser ocean_parser(true, ocean_path, test_asset_dirs());
		check(ocean_parser.save_token_ocean(ocean_path, source_tokens),
		      "save_token_ocean() writes uint32_t ocean to disk");
		std::error_code size_error;
		check_eq(std::filesystem::file_size(ocean_path, size_error),
			 static_cast<uintmax_t>(ocean_tokens * sizeof(uint32_t)),
			 "token ocean file size is exactly 4 bytes per token");

		Portion token_portion(pool_tokens, TOKEN_DIM, CONTEXT_WINDOW);
		check(token_portion.allocate_pinned(), "token portion pinned allocation succeeded");

		Optimizer token_opt(ocean_path, c.learning_rate);
		token_opt.set_token_mode(true);
		check(token_opt.is_token_mode(), "optimizer token mode enabled");

		// Pool 1: tokens [1000 .. 1599]
		token_opt.disk_to_ram(token_portion);
		check_eq(token_portion.ram_buffer_token_ids.size(), pool_tokens,
			 "Pool 1 filled to pool_tokens (600)");
		check_eq(token_portion.ram_buffer_token_ids.front(), uint32_t {1000},
			 "Pool 1 begins at ocean token 0");

		// Drain every Glass from Pool 1 and verify contiguous context -> target alignment
		size_t pool1_glasses = 0;
		bool alignment_valid = true;
		while (token_opt.load_context_window(token_portion)) {
			++pool1_glasses;
			const uint32_t* ctx = token_portion.context_token_ids[token_portion.buffer_index];
			const uint32_t target_id =
				*reinterpret_cast<const uint32_t*>(token_portion.target_tokens[token_portion.buffer_index]);
			if (!token_portion.has_token_ids || ctx[CONTEXT_WINDOW - 1] + 1U != target_id) {
				alignment_valid = false;
			}
		}
		check(pool1_glasses == 2, "Pool 1 yielded 2 non-overlapping 256-token glasses");
		check(alignment_valid, "Every glass has contiguous context and exact next-token target");
		check_eq(token_portion.offset, size_t {0}, "Pool 1 offset reset to 0 after exhaustion");

		// Pool 2: 256 seam carry [1344..1599] + 344 new [1600..1943]
		token_opt.disk_to_ram(token_portion);
		check_eq(token_portion.ram_buffer_token_ids.size(), pool_tokens,
			 "Pool 2 filled to pool_tokens with seam carry");
		check_eq(token_portion.ram_buffer_token_ids[0], uint32_t {1344},
			 "Pool 2 starts with 256-token seam carry");
		check_eq(token_portion.ram_buffer_token_ids[CONTEXT_WINDOW], uint32_t {1600},
			 "Pool 2 continues seamlessly at token 600");

		// Pool 3: 256 seam [1688..1943] + 344 new [1944..2287]
		token_opt.disk_to_ram(token_portion);
		check_eq(token_portion.ram_buffer_token_ids.size(), pool_tokens,
			 "Pool 3 filled to pool_tokens");

		// Pool 4 (Ocean tail): 256 seam [2032..2287] + 212 remaining [2288..2499] = 468
		token_opt.disk_to_ram(token_portion);
		check_eq(token_portion.ram_buffer_token_ids.size(), size_t {468},
			 "Pool 4 holds ocean tail with seam carry (468 tokens)");
		check_eq(token_portion.ram_buffer_token_ids.back(), uint32_t {2499},
			 "Pool 4 ends on final ocean token");
		check(token_portion.pool_at_epoch_end, "Pool 4 flagged pool_at_epoch_end == true");
		size_t pool4_glasses = 0;
		bool tail_token_covered = false;
		while (token_opt.load_context_window(token_portion)) {
			++pool4_glasses;
			const uint32_t target_id =
				*reinterpret_cast<const uint32_t*>(token_portion.target_tokens[token_portion.buffer_index]);
			if (target_id == uint32_t {2499}) {
				tail_token_covered = true;
			}
		}
		check_eq(pool4_glasses, size_t {2}, "Pool 4 yields 2 glasses including right-aligned EOF window");
		check(tail_token_covered, "Final ocean token (2499) is explicitly covered at epoch EOF");

		// Pool 5 (Epoch 2 wrap-around): clean restart at token 0 without cross-epoch seam
		token_opt.disk_to_ram(token_portion);
		check_eq(token_portion.ram_buffer_token_ids.size(), pool_tokens,
			 "Pool 5 wraps cleanly to Epoch 2 with 600 tokens");
		check_eq(token_portion.ram_buffer_token_ids.front(), uint32_t {1000},
			 "Epoch 2 starts cleanly at token 0 (no cross-epoch seam bleed)");

		// Async SPSC Prefetch + reset_stream() mid-Ocean rewind check
		token_opt.start_prefetch(token_portion);
		check(token_opt.is_prefetch_running(), "Token prefetch worker started");
		token_opt.disk_to_ram(token_portion);
		check_eq(token_portion.ram_buffer_token_ids.size(), pool_tokens,
			 "Async prefetched token Pool 2 consumed cleanly");
		check_eq(token_portion.ram_buffer_token_ids[0], uint32_t {1344},
			 "Async prefetched token Pool 2 preserved seam carry");
		token_opt.reset_stream(token_portion, true);
		check(token_opt.is_prefetch_running(), "reset_stream preserves prefetch state");
		check_eq(token_portion.ram_buffer_token_ids.front(), uint32_t {1000},
			 "reset_stream rewinds mid-Ocean stream cleanly to token 0");
		token_opt.stop_prefetch();
		token_opt.close_stream();

		// Tiny ocean (< context_window + 1) repeat without EOF->BOF cursor leak
		const std::filesystem::path tiny_ocean_path =
			test_core_dir() / config::database_folder / "test_tiny_ocean.bin";
		const std::vector<uint32_t> tiny_tokens = {11, 12, 13, 14, 15};
		check(ocean_parser.save_token_ocean(tiny_ocean_path, tiny_tokens),
		      "save_token_ocean() writes tiny (<257 token) ocean");
		Optimizer tiny_opt(tiny_ocean_path, c.learning_rate);
		tiny_opt.set_token_mode(true);
		tiny_opt.disk_to_ram(token_portion);
		check(token_portion.ram_buffer_token_ids.size() >= CONTEXT_WINDOW + 1,
		      "Tiny ocean expanded to >= 1 full context window");
		check(token_portion.pool_at_epoch_end, "Tiny ocean flagged pool_at_epoch_end == true");
		bool has_eos_sep = false;
		for (size_t i = 5; i < token_portion.ram_buffer_token_ids.size(); ++i) {
			if (token_portion.ram_buffer_token_ids[i] == vocab::EOS) {
				has_eos_sep = true;
				break;
			}
		}
		check(has_eos_sep, "Tiny ocean repeat inserts EOS delimiters between wraps");
		tiny_opt.disk_to_ram(token_portion);
		check_eq(token_portion.ram_buffer_token_ids.front(), uint32_t {11},
			 "Tiny ocean Epoch 2 restarts cleanly at token 0");
		tiny_opt.close_stream();
		std::filesystem::remove(tiny_ocean_path, size_error);

		// Exact chunk-size multiple Ocean wrap-around check
		const std::vector<uint32_t> exact_tokens(source_tokens.begin(),
							 source_tokens.begin() + pool_tokens);
		check(ocean_parser.save_token_ocean(ocean_path, exact_tokens),
		      "save_token_ocean() writes exact-multiple ocean (600 tokens)");
		Optimizer exact_opt(ocean_path, c.learning_rate);
		exact_opt.set_token_mode(true);
		exact_opt.disk_to_ram(token_portion);
		check_eq(token_portion.ram_buffer_token_ids.size(), pool_tokens,
			 "Exact-multiple ocean Pool 1 has 600 tokens");
		exact_opt.disk_to_ram(token_portion);
		check_eq(token_portion.ram_buffer_token_ids.size(), pool_tokens,
			 "Exact-multiple ocean Pool 2 wraps cleanly to 600 tokens");
		check_eq(token_portion.ram_buffer_token_ids.front(), uint32_t {1000},
			 "Exact-multiple ocean Epoch 2 restarts at token 0");
		exact_opt.close_stream();

		token_portion.free_pinned();
		std::error_code remove_error;
		check(std::filesystem::remove(ocean_path, remove_error),
		      "Temporary test token ocean removed cleanly after stream close");
	}

	std::cout << "\n=== Optimizer Test Suite: " << s_pass << " passed, " << s_fail
		  << " failed ===\n";
	int fails = s_fail;
	s_pass = s_fail = 0;
	return fails == 0;
}
