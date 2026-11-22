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

#include "../include/parser.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../include/decompressor/decompress_audio.hpp"
#include "../include/decompressor/decompress_document.hpp"
#include "../include/decompressor/decompress_image.hpp"
#include "../include/decompressor/decompress_video.hpp"
#include "../include/Identifier.hpp"



// Data Streaming (or Batching).

/*
Turn raw data samples into the parsed input vector;
Now isolated from the neural network entirely.
*/

// *************
// Tokenization phase
// *************

static std::string normalize_ligatures(const std::string& text) {
	std::string result;
	result.reserve(text.size());
	for (size_t i = 0; i < text.size();) {
		unsigned char c = static_cast<unsigned char>(text[i]);
		if (c == 0xEF && i + 2 < text.size()) {
			unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
			unsigned char c2 = static_cast<unsigned char>(text[i + 2]);
			if (c1 == 0xAC) {
				if (c2 == 0x80) { result += "ff"; i += 3; continue; }
				if (c2 == 0x81) { result += "fi"; i += 3; continue; }
				if (c2 == 0x82) { result += "fl"; i += 3; continue; }
				if (c2 == 0x83) { result += "ffi"; i += 3; continue; }
				if (c2 == 0x84) { result += "ffl"; i += 3; continue; }
				if (c2 == 0x85) { result += "ft"; i += 3; continue; }
				if (c2 == 0x86) { result += "st"; i += 3; continue; }
			}
		}
		if (c == 0xE2 && i + 2 < text.size()) {
			unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
			unsigned char c2 = static_cast<unsigned char>(text[i + 2]);
			if (c1 == 0x80) {
				if (c2 >= 0x98 && c2 <= 0x9B) { result += '\''; i += 3; continue; }
				if (c2 >= 0x9C && c2 <= 0x9F) { result += '"'; i += 3; continue; }
				if (c2 >= 0x90 && c2 <= 0x95) { result += '-'; i += 3; continue; }
				if (c2 >= 0x80 && c2 <= 0x8B) { result += ' '; i += 3; continue; }
				if (c2 == 0xA6) { result += "..."; i += 3; continue; }
			}
		}
		if (c == 0xC2 && i + 1 < text.size()) {
			unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
			if (c1 == 0xA0) { result += ' '; i += 2; continue; }
		}
		if (c == 0x91 || c == 0x92) { result += '\''; ++i; continue; }
		if (c == 0x93 || c == 0x94) { result += '"'; ++i; continue; }
		if (c == 0x96 || c == 0x97 || c == 0xAD) { result += '-'; ++i; continue; }
		if (c == 0xA0) { result += ' '; ++i; continue; }
		if (c == 0x85) { result += "..."; ++i; continue; }
		result += static_cast<char>(c);
		++i;
	}
	return result;
}

// Cleans raw token strings: normalizes ligatures, lowercases ASCII and strips leading/trailing punctuation
static std::string sanitize_word(const std::string& raw) {
	if (raw.empty())
		return "";
	if (raw.size() >= 3 && raw.front() == '<' && raw.back() == '>') {
		return raw;
	}
	std::string normalized = normalize_ligatures(raw);
	size_t start = 0;
	while (start < normalized.size() && (std::ispunct(static_cast<unsigned char>(normalized[start])))) {
		++start;
	}
	size_t end = normalized.size();
	while (end > start && (std::ispunct(static_cast<unsigned char>(normalized[end - 1])))) {
		--end;
	}
	if (start >= end)
		return "";
	std::string clean = normalized.substr(start, end - start);
	for (char& c : clean) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return clean;
}

// MOCK DISCRETE TOKENIZER (Gemini / Chameleon Early Fusion style)
std::vector<uint32_t> Parser::tokenization(Input& input, Test& test) {

	// *************
	// Developer mode : on
	// Data comes from test structure
	// *************

	if (developer_mode == true) {
		// 1. Text Tokenization (IDs 1 - 9999)
		for (const std::string& word : test.test_input) {
			std::string clean = sanitize_word(word);
			if (!clean.empty()) {
				add_assign_increase(
					clean, input.main_dictionary, input.next_text_id, input);
			}
		}

		// 3. Image Tokenization (IDs 20000 - 29999)
		for (const std::string& file : test.data_image) {
			tokenize_from_disk(
				file, input.image_dictionary, input.next_image_id, input);
		}

		// 4. Video Tokenization (IDs 30000 - 39999)
		for (const std::string& file : test.data_video) {
			tokenize_from_disk(
				file, input.video_dictionary, input.next_video_id, input);
		}

		// 5. Voice/Audio Tokenization (IDs 40000 - 49999)
		for (const std::string& file : test.data_voice) {
			tokenize_from_disk(
				file, input.voice_dictionary, input.next_voice_id, input);
		}

		// *************
		// Developer mode : off
		// Data comes from input structure
		// *************

	} else {
		// Production Mode: Dynamically Tokenize User's UI Data
		for (const std::string& word : input.user_input) {
			std::string clean = sanitize_word(word);
			if (!clean.empty()) {
				add_assign_increase(
					clean, input.main_dictionary, input.next_text_id, input);
			}
		}

		for (const std::string& path : input.user_image_paths) {
			tokenize_from_disk(
				path, input.image_dictionary, input.next_image_id, input);
		}

		for (const std::string& path : input.user_video_paths) {
			tokenize_from_disk(
				path, input.video_dictionary, input.next_video_id, input);
		}

		for (const std::string& path : input.user_voice_paths) {
			tokenize_from_disk(
				path, input.voice_dictionary, input.next_voice_id, input);
		}
	}


	return input.tokenized_data;
}

// *************
// Embedding phase
// *************

// EMBEDDINGS: Convert global token IDs to a flat dense vector
// Streams directly to disk in fixed-size chunks to eliminate RAM saturation
// and SSD page thrashing on massive datasets (hundreds of millions of tokens).
std::vector<std::bfloat16_t> Parser::embedding(Input& input, Test& test) {
	size_t chunk_position = Config::chunk_position;
	size_t tokens_written = Config::tokens_written;
	size_t last_percentage = Config::last_percentage;

	build_embedding_table(input);

	if (!embedded_path.parent_path().empty()) {
		std::error_code ec;
		std::filesystem::create_directories(embedded_path.parent_path(), ec);
	}

	std::ofstream out(embedded_path, std::ios::binary | std::ios::trunc);

	if (!out) {
		std::cerr << "[!] Fatal: Failed to open " << embedded_path
			  << " for writing embeddings.\n";

		return {};
	}

	// Keep embeddings in RAM only when within both token and byte budgets
	const size_t total_embed_bytes =
		input.tokenized_data.size() * config::token_dim * sizeof(std::bfloat16_t);
	const bool keep_in_ram =
		(input.tokenized_data.size() <= config::ram_buffer_size) &&
		(total_embed_bytes <= config::mmap_chunk_bytes);

	if (keep_in_ram == true) {
		input.embedded_data.resize(input.tokenized_data.size() * config::token_dim);
	} else {
		input.embedded_data.clear();
		input.embedded_data.shrink_to_fit();
	}

	std::vector<std::bfloat16_t> chunk(config::chunk_tokens * config::token_dim);
	std::vector<std::bfloat16_t> current_vec(config::token_dim);

	const bool show_progress = (input.tokenized_data.size() >= config::minimum_limit);

	for (size_t i = 0; i < input.tokenized_data.size(); ++i) {

		// each single token
		const uint32_t id = input.tokenized_data[i];

		// Insure it creates a new unique ID each time
		if (id != input.last_token) {
			input.last_token = id;
			auto it = input.embedding_table.find(id);
			if (it != input.embedding_table.end()) {
				current_vec = it->second;
			} else {
				float bound =
					1.0f /
					std::sqrt(static_cast<float>(config::token_dim));
				for (size_t d = 0; d < config::token_dim; ++d) {
					uint32_t h = id * 0x45d9f3b +
						     static_cast<uint32_t>(d + 1) * 0x119de1f3;
					h = ((h >> 16) ^ h) * 0x45d9f3b;
					h = ((h >> 16) ^ h);
					float norm = static_cast<float>(h) /
						     static_cast<float>(0xFFFFFFFFU);
					current_vec[d] = static_cast<std::bfloat16_t>(
						-bound + 2.0f * bound * norm);
				}
			}
		}

		if (keep_in_ram == true) {
			for (size_t d = 0; d < config::token_dim; ++d) {

				input.embedded_data[i * config::token_dim + d] = current_vec[d];
			}
		}

		for (size_t d = 0; d < config::token_dim; ++d) {
			chunk[chunk_position++] = current_vec[d];
		}

		if (chunk_position == chunk.size()) {
			out.write(reinterpret_cast<const char*>(chunk.data()),
				  chunk.size() * sizeof(std::bfloat16_t));
			tokens_written += config::chunk_tokens;
			chunk_position = 0;

			if (show_progress) {
				const size_t pct =
					(tokens_written * 100) / input.tokenized_data.size();
				if (pct != last_percentage) {
					last_percentage = pct;
					constexpr int bar_w = config::progress_bar_width;
					const int filled = static_cast<int>((pct / 100.0) * bar_w);
					std::string bar = "[";
					for (int b = 0; b < bar_w; ++b) {
						if (b < filled)
							bar += '=';
						else if (b == filled)
							bar += '>';
						else
							bar += ' ';
					}
					bar += "]";
					std::cout << "\r\033[K  [..] Synthesizing continuous embeddings "
						  << bar << " " << pct << "% ("
						  << tokens_written << "/"
						  << input.tokenized_data.size() << " tokens)"
						  << std::flush;
				}
			}
		}
	}

	// std::ofstream out(embedded_path, std::ios::binary | std::ios::trunc)
	if (chunk_position > 0) {
		out.write(reinterpret_cast<const char*>(chunk.data()),
			  chunk_position * sizeof(std::bfloat16_t));
		tokens_written += (chunk_position / config::token_dim);
	}
	out.close();

	if (show_progress) {
		const uintmax_t bytes =
			input.tokenized_data.size() * config::token_dim * sizeof(std::bfloat16_t);
		std::cout
			<< "\r\033[K  [OK] Continuous embeddings synthesized: "
			<< input.tokenized_data.size() << " tokens (" << format_file_size(bytes)
			<< ") streamed to disk.\n"
			<< std::flush;
	}

	return input.embedded_data;
}

void Parser::build_embedding_table(Input& input) {
	const size_t total_unique = input.main_dictionary.size() + input.image_dictionary.size() +
				    input.video_dictionary.size() + input.voice_dictionary.size();
	if (total_unique > 0) {
		input.embedding_table.reserve(total_unique);
	}

	float bound =
		1.0f /
		std::sqrt(static_cast<float>(config::token_dim));
	auto ensure_embedding = [&](uint32_t id) {
		if (input.embedding_table.find(id) == input.embedding_table.end()) {
			std::vector<std::bfloat16_t> vec(config::token_dim);
			for (uint32_t d = 0; d < config::token_dim; ++d) {
				uint32_t h =
					id * 0x45d9f3b + static_cast<uint32_t>(d + 1) * 0x119de1f3;
				h = ((h >> 16) ^ h) * 0x45d9f3b;
				h = ((h >> 16) ^ h);
				float norm =
					static_cast<float>(h) / static_cast<float>(0xFFFFFFFFU);
				vec[d] = static_cast<std::bfloat16_t>(-bound + 2.0f * bound * norm);
			}
			input.embedding_table[id] = std::move(vec);
		}
	};

	for (const auto& [_, id] : input.main_dictionary)
		ensure_embedding(id);
	for (const auto& [_, id] : input.image_dictionary)
		ensure_embedding(id);
	for (const auto& [_, id] : input.video_dictionary)
		ensure_embedding(id);
	for (const auto& [_, id] : input.voice_dictionary)
		ensure_embedding(id);
}

uint32_t Parser::decode_token(const Input& input, const std::vector<float>& embedding_vector) {
	if (input.embedding_table.empty()) {
		return 0;
	}

	// Rebuild the contiguous mirror whenever the table has grown or shrunk
	if (input.decode_ids.size() != input.embedding_table.size()) {
		input.decode_ids.clear();
		input.decode_vectors.clear();
		input.decode_ids.reserve(input.embedding_table.size());
		input.decode_vectors.reserve(input.embedding_table.size() * config::token_dim);

		for (const auto& [id, vec] : input.embedding_table) {
			input.decode_ids.push_back(id);
			for (size_t d = 0; d < config::token_dim; ++d) {
				input.decode_vectors.push_back(
					d < vec.size() ? static_cast<float>(vec[d]) : 0.0f);
			}
		}
	}

	const size_t compared_dims = std::min(config::token_dim, embedding_vector.size());

	for (size_t d = 0; d < compared_dims; ++d) {
		if (!std::isfinite(embedding_vector[d])) {
			return 0;
		}
	}

	float y_norm_sq = 0.0f;
	for (size_t d = 0; d < compared_dims; ++d) {
		y_norm_sq += embedding_vector[d] * embedding_vector[d];
	}
	float y_inv_norm = 1.0f / (std::sqrt(y_norm_sq) + 1e-8f);

	const float* row = input.decode_vectors.data();

	uint32_t best_id = 0;
	float best_sim = -1e30f;

	for (size_t i = 0; i < input.decode_ids.size(); ++i, row += config::token_dim) {
		uint32_t id = input.decode_ids[i];
		if (id == vocab::PAD) {
			continue;
		}

		float dot = 0.0f;
		float e_norm_sq = 0.0f;
		for (size_t d = 0; d < compared_dims; ++d) {
			dot += embedding_vector[d] * row[d];
			e_norm_sq += row[d] * row[d];
		}
		float cosine_sim = dot * y_inv_norm / (std::sqrt(e_norm_sq) + 1e-8f);
		if (cosine_sim > best_sim) {
			best_sim = cosine_sim;
			best_id = id;
		}
	}

	if (best_id == 0 && !input.decode_ids.empty()) {
		best_id = input.decode_ids[0];
	}

	return best_id;
}

std::string Parser::token_to_string(const Input& input, uint32_t token_id) {
	if (token_id == vocab::PAD) {
		return "<PAD>";
	}
	if (token_id == vocab::UNK) {
		return "<UNK>";
	}
	if (token_id == vocab::BOS) {
		return "<BOS>";
	}
	if (token_id == vocab::EOS) {
		return "<EOS>";
	}

	// 1. Text & Number Modality (Main Dictionary: 4 to 64,511)
	if (token_id >= vocab::TEXT_START_ID && token_id <= vocab::TEXT_MAX_ID) {
		auto it = input.reverse_main_dictionary.find(token_id);
		if (it != input.reverse_main_dictionary.end()) {
			return it->second;
		}
		for (const auto& [word, id] : input.main_dictionary) {
			if (id == token_id)
				return word;
		}
		return "[Token #" + std::to_string(token_id) + "]";
	}

	// 2. Visual Modality (Images & Video frames: 64,512 to 64,767)
	if (token_id >= vocab::VISUAL_START_ID && token_id <= vocab::VISUAL_END_ID) {
		uint8_t byte_val = static_cast<uint8_t>(token_id - vocab::VISUAL_START_ID);
		for (const auto& [b, id] : input.video_dictionary) {
			if (id == token_id) {
				return "[Video byte: " +
				       std::to_string(static_cast<int>(byte_val)) + "]";
			}
		}
		return "[Image byte: " + std::to_string(static_cast<int>(byte_val)) + "]";
	}

	// 3. Acoustic Modality (Voice & Audio waveforms: 64,768 to 65,023)
	if (token_id >= vocab::ACOUSTIC_START_ID && token_id <= vocab::ACOUSTIC_END_ID) {
		uint8_t byte_val = static_cast<uint8_t>(token_id - vocab::ACOUSTIC_START_ID);
		return "[Voice byte: " + std::to_string(static_cast<int>(byte_val)) + "]";
	}

	// 4. Reserved Expansion Slots (65,024 to 65,535)
	if (token_id >= vocab::EXPANSION_START_ID && token_id <= vocab::EXPANSION_MAX_ID) {
		return "[Expansion #" + std::to_string(token_id) + "]";
	}

	return "[Unknown #" + std::to_string(token_id) + "]";
}

std::string Parser::decode_prediction(const Input& input,
				      const std::vector<float>& embedding_vector) {
	uint32_t token_id = decode_token(input, embedding_vector);
	if (token_id == 0)
		return "[Empty Vocabulary]";
	return token_to_string(input, token_id);
}

std::vector<uint32_t> Parser::tokenize_prompt(Input& input, const std::string& prompt) {
	std::vector<uint32_t> tokens;
	std::istringstream iss(prompt);
	std::string word;
	while (iss >> word) {
		std::string clean = sanitize_word(word);
		if (clean.empty())
			continue;

		auto it = input.main_dictionary.find(clean);
		if (it == input.main_dictionary.end()) {
			it = input.main_dictionary.find(word);
		}

		// last element of main == new data
		if (it == input.main_dictionary.end()) {

			// check if we didn't pass the limit
			if (input.next_text_id <= vocab::TEXT_MAX_ID) {
				uint32_t new_id = input.next_text_id++;

				// sanitized_word in main_Dictionary assign it a id
				input.main_dictionary[clean] = new_id;

				// id in reverse_main_Dictionary assign it, it's exact data
				input.reverse_main_dictionary[new_id] = clean;
				// put all in tokens
				tokens.push_back(new_id);
			} else {
				input.main_dictionary[clean] = vocab::UNK;
				tokens.push_back(vocab::UNK);
			}
		} else {
			tokens.push_back(it->second);
		}
	}
	build_embedding_table(input);
	return tokens;
}

std::vector<uint32_t> Parser::tokenize_for_inference(const Input& input,
						     const std::string& prompt) {
	std::vector<uint32_t> tokens;
	std::istringstream iss(prompt);
	std::string word;
	while (iss >> word) {
		const std::string clean = sanitize_word(word);
		if (clean.empty())
			continue;

		auto it = input.main_dictionary.find(clean);
		if (it == input.main_dictionary.end()) {
			it = input.main_dictionary.find(word);
		}

		// Unknown words map to <UNK>: the dictionary and ID counter stay untouched
		tokens.push_back(it != input.main_dictionary.end() ? it->second : vocab::UNK);
	}
	return tokens;
}

std::vector<uint32_t> Parser::build_context_ids(const std::vector<uint32_t>& tokens,
						size_t context_tokens) {
	std::vector<uint32_t> context_ids(context_tokens, vocab::PAD);
	const size_t copy_count = std::min(tokens.size(), context_tokens);
	std::copy(tokens.end() - copy_count, tokens.end(), context_ids.end() - copy_count);
	return context_ids;
}

std::vector<std::bfloat16_t> Parser::build_context_window(const Input& input,
							  const std::vector<uint32_t>& tokens,
							  size_t context_tokens) {
	std::vector<std::bfloat16_t> window(context_tokens * config::token_dim,
					    static_cast<std::bfloat16_t>(0.0f));
	if (tokens.empty())
		return window;

	size_t count = std::min(tokens.size(), context_tokens);
	size_t start_token_idx = context_tokens - count;

	for (size_t i = 0; i < count; ++i) {
		uint32_t token_id = tokens[tokens.size() - count + i];
		auto it = input.embedding_table.find(token_id);
		if (it != input.embedding_table.end()) {
			for (size_t d = 0; d < config::token_dim; ++d) {
				window[(start_token_idx + i) * config::token_dim + d] =
					(d < it->second.size())
						? it->second[d]
						: static_cast<std::bfloat16_t>(0.0f);
			}
		}
	}
	return window;
}

static void print_progress_bar(const std::string& filename,
			       const std::string& action,
			       uintmax_t current_bytes,
			       uintmax_t total_bytes) {
	const double pct = (total_bytes > 0) ? std::min(100.0,
							(static_cast<double>(current_bytes) /
							 static_cast<double>(total_bytes)) *
								100.0)
					     : 100.0;
	constexpr int bar_width = config::progress_bar_width;
	const int filled = static_cast<int>((pct / 100.0) * bar_width);

	std::string bar = "[";
	for (int i = 0; i < bar_width; ++i) {
		if (i < filled) {
			bar += '=';
		} else if (i == filled && filled < bar_width) {
			bar += '>';
		} else {
			bar += ' ';
		}
	}
	bar += "]";

	std::cout << "\r\033[K  [..] " << action << " " << filename << " " << bar << " " << std::fixed
		  << std::setprecision(1) << pct << "% (" << format_file_size(current_bytes)
		  << " / " << format_file_size(total_bytes) << ")" << std::flush;
}

enum class BulkModality { Document, Image, Video, Audio, Binary };

static BulkModality classify_bulk_modality(const std::filesystem::path& file_path,
					   const IdentifiedFile& id_file,
					   std::string& lowercase_ext) {
	lowercase_ext = file_path.extension().string();
	for (char& c : lowercase_ext) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}

	const bool is_text_ext =
		(lowercase_ext == ".txt" || lowercase_ext == ".text" || lowercase_ext == ".md" ||
		 lowercase_ext == ".markdown" || lowercase_ext == ".json" ||
		 lowercase_ext == ".csv" || lowercase_ext == ".log" || lowercase_ext == ".c" ||
		 lowercase_ext == ".cpp" || lowercase_ext == ".h" || lowercase_ext == ".hpp" ||
		 lowercase_ext == ".cu" || lowercase_ext == ".cuh" || lowercase_ext == ".py" ||
		 lowercase_ext == ".html" || lowercase_ext == ".htm" || lowercase_ext == ".xml" ||
		 lowercase_ext == ".yaml" || lowercase_ext == ".yml" || lowercase_ext == ".ini" ||
		 lowercase_ext == ".cfg" || lowercase_ext == ".conf" || lowercase_ext == ".sql" ||
		 lowercase_ext == ".rs" || lowercase_ext == ".go" || lowercase_ext == ".java" ||
		 lowercase_ext == ".ts" || lowercase_ext == ".js" || lowercase_ext == ".sh" ||
		 lowercase_ext == ".bat" || lowercase_ext == ".ps1");

	if (id_file.modality == ModalityCategory::DOCUMENT || is_text_ext ||
	    lowercase_ext == ".pdf" || lowercase_ext == ".docx") {
		return BulkModality::Document;
	}
	if (id_file.modality == ModalityCategory::IMAGE || lowercase_ext == ".jpg" ||
	    lowercase_ext == ".jpeg" || lowercase_ext == ".png" || lowercase_ext == ".bmp" ||
	    lowercase_ext == ".webp" || lowercase_ext == ".gif" || lowercase_ext == ".tiff" ||
	    lowercase_ext == ".tif") {
		return BulkModality::Image;
	}
	if (id_file.modality == ModalityCategory::VIDEO || lowercase_ext == ".mp4" ||
	    lowercase_ext == ".mkv" || lowercase_ext == ".avi" || lowercase_ext == ".mov" ||
	    lowercase_ext == ".webm" || lowercase_ext == ".wmv") {
		return BulkModality::Video;
	}
	if (id_file.modality == ModalityCategory::AUDIO || lowercase_ext == ".ogg" ||
	    lowercase_ext == ".wav" || lowercase_ext == ".mp3" || lowercase_ext == ".flac" ||
	    lowercase_ext == ".aac" || lowercase_ext == ".m4a" || lowercase_ext == ".wma" ||
	    lowercase_ext == ".opus") {
		return BulkModality::Audio;
	}
	return BulkModality::Binary;
}

static bool copy_token_range(std::ifstream& in_file,
			     size_t count,
			     std::vector<uint32_t>& scratch,
			     std::ofstream* out_file,
			     std::vector<uint32_t>* out_vec) {
	size_t remaining = count;
	while (remaining > 0) {
		const size_t batch = std::min<size_t>(scratch.size(), remaining);
		in_file.read(reinterpret_cast<char*>(scratch.data()),
			     static_cast<std::streamsize>(batch * sizeof(uint32_t)));
		if (static_cast<size_t>(in_file.gcount()) != batch * sizeof(uint32_t)) {
			return false;
		}
		if (out_file) {
			out_file->write(reinterpret_cast<const char*>(scratch.data()),
					static_cast<std::streamsize>(batch * sizeof(uint32_t)));
			if (!(*out_file)) {
				return false;
			}
		}
		if (out_vec) {
			out_vec->insert(out_vec->end(), scratch.begin(), scratch.begin() + batch);
		}
		remaining -= batch;
	}
	return true;
}

size_t Parser::bulk_tokenize_directory(const std::filesystem::path& dir_path,
				       Input& input,
				       size_t max_files,
				       uint32_t min_frequency,
				       const std::filesystem::path& ocean_path,
				       bool shuffle_files,
				       uint32_t shuffle_seed,
				       size_t* total_tokens) {
	last_bulk_files.clear();
	if (total_tokens) {
		*total_tokens = 0;
	}

	std::error_code ec;
	if (!std::filesystem::exists(dir_path, ec) ||
	    !std::filesystem::is_directory(dir_path, ec)) {
		std::cerr << "[!] Invalid or non-existent asset directory: " << dir_path << "\n";
		return 0;
	}

	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path, ec)) {
		if (entry.is_regular_file(ec)) {
			std::filesystem::path rel = entry.path().lexically_relative(dir_path);
			bool in_archive = false;
			for (const auto& part : rel) {
				if (part == "archive") {
					in_archive = true;
					break;
				}
			}
			if (in_archive) {
				continue;
			}
			std::string fname = entry.path().filename().string();
			if (!fname.empty() && fname.front() != '.') {
				files.push_back(entry.path());
			}
		}
	}

	if (files.empty()) {
		std::cout << "[!] No files found in " << dir_path << "\n";
		return 0;
	}

	std::sort(files.begin(), files.end());
	if (shuffle_files && files.size() > 1) {
		std::mt19937 file_rng(shuffle_seed);
		std::shuffle(files.begin(), files.end(), file_rng);
	}
	std::cout << "\n[+] Discovered " << files.size() << " asset file(s) in "
		  << dir_path.string() << ":\n";

	// Phase 1.3: Frequency filtering pass (when min_frequency > 1)
	if (min_frequency > 1) {
		input.main_dictionary.clear();
		input.reverse_main_dictionary.clear();
		input.next_text_id = vocab::TEXT_START_ID;

		std::unordered_map<std::string, uint32_t> word_counts;
		size_t scan_count = 0;
		for (const auto& fpath : files) {
			if (max_files > 0 && scan_count >= max_files) {
				break;
			}
			++scan_count;
			std::string ext;
			IdentifiedFile id_file = Identifier::identify(fpath);
			if (classify_bulk_modality(fpath, id_file, ext) == BulkModality::Document) {
				std::string extracted_text =
					DecompressDocument::extract_text(fpath);
				std::istringstream iss(extracted_text);
				std::string word;
				while (iss >> word) {
					std::string clean = sanitize_word(word);
					if (!clean.empty()) {
						word_counts[clean]++;
					}
				}
			}
		}

		std::vector<std::pair<std::string, uint32_t>> eligible_words;
		eligible_words.reserve(word_counts.size());
		for (const auto& [word, count] : word_counts) {
			if (count >= min_frequency &&
			    input.main_dictionary.find(word) == input.main_dictionary.end()) {
				eligible_words.emplace_back(word, count);
			}
		}

		std::sort(eligible_words.begin(),
			  eligible_words.end(),
			  [](const auto& a, const auto& b) {
				  if (a.second != b.second)
					  return a.second > b.second;
				  return a.first < b.first;
			  });

		size_t admitted_count = 0;
		for (const auto& [word, count] : eligible_words) {
			if (input.next_text_id <= vocab::TEXT_MAX_ID) {
				input.main_dictionary[word] = input.next_text_id;
				input.reverse_main_dictionary[input.next_text_id] = word;
				input.next_text_id += 1;
				++admitted_count;
			} else {
				break;
			}
		}

		if (developer_mode || !eligible_words.empty()) {
			std::cout << "  [+] Frequency Filter (count >= " << min_frequency
				  << "): " << admitted_count << " terms admitted into vocabulary ("
				  << (word_counts.size() - admitted_count)
				  << " sub-threshold/overflow mapped to <UNK>)\n";
		}
	}

	struct StreamSinkGuard {
		Parser& parser;
		bool active = false;
		~StreamSinkGuard() {
			if (active) {
				parser.flush_ocean_buffer();
				if (parser.active_ocean_stream_.is_open()) {
					parser.active_ocean_stream_.close();
				}
				parser.active_ocean_buffer_.clear();
				parser.streaming_to_disk_ = false;
			}
		}
	} sink_guard {*this, false};

	active_ocean_buffer_.clear();
	active_ocean_count_ = 0;
	streaming_to_disk_ = false;
	stream_write_failed_ = false;

	if (!ocean_path.empty()) {
		if (!ocean_path.parent_path().empty()) {
			std::error_code dir_ec;
			std::filesystem::create_directories(ocean_path.parent_path(), dir_ec);
		}
		active_ocean_stream_.open(ocean_path, std::ios::binary | std::ios::trunc);
		if (!active_ocean_stream_) {
			std::cerr << "[!] Failed to open staging ocean file for writing: "
				  << ocean_path.string() << "\n";
			return 0;
		}
		active_ocean_buffer_.reserve(config::chunk_tokens * 4);
		streaming_to_disk_ = true;
		sink_guard.active = true;
	}

	size_t files_processed = 0;
	size_t eos_delimiters = 0;
	const size_t total_files =
		(max_files > 0 && max_files < files.size()) ? max_files : files.size();

	auto report_file_done = [&](const std::filesystem::path& fpath,
				    uintmax_t fsize,
				    const std::string& modality_tag,
				    size_t added_tokens) {
		++files_processed;
		last_bulk_files.push_back(fpath);

		if (developer_mode) {
			std::cout << "  [OK] " << fpath.filename().string() << " ("
				  << format_file_size(fsize) << ") -> " << modality_tag << ": "
				  << added_tokens << " tokens mapped\n";
		} else {
			const double pct =
				(total_files > 0) ? std::min(100.0,
							     (static_cast<double>(files_processed) *
							      100.0 / total_files))
						  : 100.0;
			constexpr int bar_w = 24;
			int filled = static_cast<int>((pct / 100.0) * bar_w);
			if (filled > bar_w)
				filled = bar_w;
			std::string bar = "[";
			for (int b = 0; b < bar_w; ++b) {
				if (b < filled)
					bar += '=';
				else if (b == filled && filled < bar_w)
					bar += '>';
				else
					bar += ' ';
			}
			bar += "]";

			std::string fname = fpath.filename().string();
			if (fname.size() > 20)
				fname = fname.substr(0, 17) + "...";

			{
				FormatGuard fmt(std::cout);
				std::cout << "\r\033[K  [..] Ingesting assets " << bar << " " << std::fixed
					  << std::setprecision(1) << pct << "% (" << files_processed << "/"
					  << total_files << " files | " << active_token_count(input)
					  << " tokens | " << fname << ")   " << std::flush;
			}
		}
	};

	for (const auto& fpath : files) {
		if (max_files > 0 && files_processed >= max_files) {
			break;
		}
		if (stream_write_failed_) {
			break;
		}

		IdentifiedFile id_file = Identifier::identify(fpath);
		std::string ext;
		const BulkModality modality = classify_bulk_modality(fpath, id_file, ext);

		uintmax_t fsize = std::filesystem::file_size(fpath, ec);
		if (ec)
			fsize = 0;

		const size_t initial_tokens = active_token_count(input);

		if (modality == BulkModality::Document) {
			// Phase 1.2: Sanitize Document Ingestion & Eliminate PDF Byte Streaming
			// Extract clean text from PDF, DOCX, TXT, MD so linguistic terms enter main_dictionary
			std::string extracted_text = DecompressDocument::extract_text(fpath);
			std::istringstream iss(extracted_text);
			std::string word;
			while (iss >> word) {
				std::string clean = sanitize_word(word);
				if (!clean.empty()) {
					if (min_frequency > 1) {
						auto it = input.main_dictionary.find(clean);
						if (it != input.main_dictionary.end() &&
						    it->second <= vocab::TEXT_MAX_ID) {
							emit_token(input, it->second);
						} else {
							emit_token(input, vocab::UNK);
						}
					} else {
						add_assign_increase(clean,
								    input.main_dictionary,
								    input.next_text_id,
								    input);
					}
				}
			}
			const size_t added = active_token_count(input) - initial_tokens;
			std::string tag = (ext == ".pdf")
						  ? "PDF Document"
						  : ((ext == ".docx") ? "DOCX Document" : "Text");
			report_file_done(fpath, fsize, tag, added);
		} else if (modality == BulkModality::Image || modality == BulkModality::Video ||
			   modality == BulkModality::Audio) {
			std::ifstream in(fpath, std::ios::binary);
			if (!in) {
				std::cerr << "  [!] Failed to open " << fpath.filename().string()
					  << "\n";
				continue;
			}
			auto& dict = (modality == BulkModality::Image)
					     ? input.image_dictionary
					     : ((modality == BulkModality::Video)
							? input.video_dictionary
							: input.voice_dictionary);
			uint32_t& next_id = (modality == BulkModality::Image)
						    ? input.next_image_id
						    : ((modality == BulkModality::Video)
							       ? input.next_video_id
							       : input.next_voice_id);
			const char* bar_label = (modality == BulkModality::Image)
							? "Ingesting Image"
							: ((modality == BulkModality::Video)
								   ? "Tokenizing Video"
								   : "Ingesting Audio");
			const char* done_label = (modality == BulkModality::Image)
							 ? "Image"
							 : ((modality == BulkModality::Video)
								    ? "Video"
								    : "Voice/Audio");

			char buf[config::io_buffer_bytes];
			uintmax_t bytes_read = 0;
			uintmax_t last_progress_bytes = 0;
			const bool show_progress = (fsize >= config::progress_min_file_bytes);

			while (in.read(buf, sizeof(buf)) || in.gcount() > 0) {
				std::streamsize n = in.gcount();
				for (std::streamsize i = 0; i < n; ++i) {
					add_assign_increase(static_cast<uint8_t>(buf[i]),
							    dict,
							    next_id,
							    input);
				}
				bytes_read += static_cast<uintmax_t>(n);
				if (show_progress &&
				    (bytes_read - last_progress_bytes >= config::progress_step_bytes ||
				     bytes_read >= fsize)) {
					last_progress_bytes = bytes_read;
					print_progress_bar(fpath.filename().string(),
							   bar_label,
							   bytes_read,
							   fsize);
				}
			}
			const size_t added = active_token_count(input) - initial_tokens;
			if (show_progress) {
				std::cout << "\r\033[K";
			}
			report_file_done(fpath, fsize, done_label, added);
		} else {
			// Phase 1.3: Non-media binary assets are NOT streamed into image_dictionary
			report_file_done(fpath, fsize, "Binary Asset (Skipped Raw Streaming)", 0);
		}

		// Mark document/asset boundary so unrelated files never bleed across a context window
		if (active_token_count(input) > initial_tokens) {
			emit_token(input, vocab::EOS);
			++eos_delimiters;
		}
	}

	if (streaming_to_disk_) {
		flush_ocean_buffer();
		active_ocean_stream_.close();
		if (!active_ocean_stream_) {
			stream_write_failed_ = true;
		}
	}

	const size_t final_token_count = active_token_count(input);
	if (stream_write_failed_) {
		std::cerr << "[!] Write failure while streaming ingested tokens to "
			  << ocean_path.string() << "\n";
		std::error_code rm_ec;
		std::filesystem::remove(ocean_path, rm_ec);
		return 0;
	}

	if (total_tokens) {
		*total_tokens = final_token_count;
	}

	if (!developer_mode && files_processed > 0) {
		std::cout << "\r\033[K  [OK] Ingesting assets [========================] 100.0% ("
			  << files_processed << "/" << total_files << " files | "
			  << final_token_count << " tokens";
		if (eos_delimiters > 0) {
			std::cout << " | " << eos_delimiters << " <EOS> delimiters";
		}
		std::cout << ")\n" << std::flush;
	}

	return files_processed;
}

bool Parser::prepare_training_ocean(const std::filesystem::path& staging_path,
				    const std::filesystem::path& ocean_path,
				    size_t context_window,
				    size_t pool_capacity,
				    uint32_t shuffle_seed,
				    std::vector<uint32_t>& validation_tokens,
				    size_t& train_tokens_out) const {
	validation_tokens.clear();
	train_tokens_out = 0;

	std::error_code ec;
	const uintmax_t staging_bytes = std::filesystem::file_size(staging_path, ec);
	if (ec || staging_bytes < sizeof(uint32_t)) {
		std::cerr << "[!] Staging ocean missing or empty: " << staging_path.string() << "\n";
		return false;
	}
	size_t total_tokens = static_cast<size_t>(staging_bytes / sizeof(uint32_t));

	// Repeat tiny corpora in-place so continuous autoregressive training has >= 1 window
	const size_t min_training_tokens = context_window + 1;
	if (total_tokens < min_training_tokens) {
		std::cout << "[..] Dataset sequence (" << total_tokens
			  << " tokens) is shorter than context window (" << context_window
			  << " tokens). Repeating sequence to enable continuous autoregressive training...\n";
		std::vector<uint32_t> tiny_tokens(total_tokens);
		{
			std::ifstream tiny_in(staging_path, std::ios::binary);
			if (!tiny_in) {
				return false;
			}
			tiny_in.read(reinterpret_cast<char*>(tiny_tokens.data()),
				     static_cast<std::streamsize>(total_tokens * sizeof(uint32_t)));
			if (static_cast<size_t>(tiny_in.gcount()) != total_tokens * sizeof(uint32_t)) {
				return false;
			}
		}
		const size_t target_tokens = context_window * 8;
		std::vector<uint32_t> expanded;
		expanded.reserve(target_tokens);
		while (expanded.size() < target_tokens) {
			for (size_t i = 0; i < tiny_tokens.size() && expanded.size() < target_tokens;
			     ++i) {
				expanded.push_back(tiny_tokens[i]);
			}
			if (expanded.size() < target_tokens && !expanded.empty() &&
			    expanded.back() != vocab::EOS) {
				expanded.push_back(vocab::EOS);
			}
		}
		if (!save_token_ocean(ocean_path, expanded)) {
			return false;
		}
		std::filesystem::remove(staging_path, ec);
		train_tokens_out = expanded.size();
		return true;
	}

	const std::filesystem::path raw_train_path = ocean_path.string() + ".raw.tmp";
	auto cleanup_temps = [&]() {
		std::error_code rm_ec;
		std::filesystem::remove(raw_train_path, rm_ec);
	};

	const size_t stream_batch_tokens = config::chunk_tokens * 4;
	std::vector<uint32_t> scratch(stream_batch_tokens);
	const bool has_validation_corpus = (total_tokens >= context_window * 4);
	size_t train_count = 0;

	if (has_validation_corpus) {
		const size_t min_val_tokens = context_window + 1;
		const size_t val_budget =
			std::max(min_val_tokens, std::min<size_t>(total_tokens / 10, pool_capacity));
		if (total_tokens > val_budget + min_val_tokens) {
			const size_t max_blocks = total_tokens / (min_val_tokens * 2);
			const size_t num_blocks = std::max<size_t>(1, std::min<size_t>(10, max_blocks));
			const size_t seg_len = total_tokens / num_blocks;
			const size_t block_val_len = std::min(
				seg_len - min_val_tokens, std::max(min_val_tokens, val_budget / num_blocks));

			std::ifstream staging_in(staging_path, std::ios::binary);
			std::ofstream raw_out(raw_train_path, std::ios::binary | std::ios::trunc);
			if (!staging_in || !raw_out) {
				cleanup_temps();
				return false;
			}

			validation_tokens.reserve(num_blocks * (block_val_len + 1));

			for (size_t b = 0; b < num_blocks; ++b) {
				const size_t seg_begin = b * seg_len;
				const size_t seg_end =
					(b + 1 == num_blocks) ? total_tokens : (b + 1) * seg_len;
				const size_t val_begin =
					seg_begin + (seg_end - seg_begin - block_val_len) / 2;
				const size_t val_end = val_begin + block_val_len;

				if (!copy_token_range(staging_in,
						      val_begin - seg_begin,
						      scratch,
						      &raw_out,
						      nullptr)) {
					cleanup_temps();
					return false;
				}
				train_count += (val_begin - seg_begin);

				const uint32_t seam_eos = vocab::EOS;
				raw_out.write(reinterpret_cast<const char*>(&seam_eos), sizeof(uint32_t));
				if (!raw_out) {
					cleanup_temps();
					return false;
				}
				++train_count;

				if (!validation_tokens.empty()) {
					validation_tokens.push_back(vocab::EOS);
				}
				if (!copy_token_range(staging_in,
						      val_end - val_begin,
						      scratch,
						      nullptr,
						      &validation_tokens)) {
					cleanup_temps();
					return false;
				}
				if (!copy_token_range(staging_in,
						      seg_end - val_end,
						      scratch,
						      &raw_out,
						      nullptr)) {
					cleanup_temps();
					return false;
				}
				train_count += (seg_end - val_end);
			}

			raw_out.close();
			staging_in.close();
			if (!raw_out) {
				cleanup_temps();
				return false;
			}
			std::filesystem::remove(staging_path, ec);

			std::cout << "[OK] Stratified validation split (" << num_blocks
				  << " blocks): " << train_count << " train tokens | "
				  << validation_tokens.size() << " held-out validation tokens.\n";
		}
	}

	std::filesystem::path source_train_path = raw_train_path;
	if (train_count == 0) {
		train_count = total_tokens;
		source_train_path = staging_path;
	}

	// Single-pool corpus (<= pool_capacity): Pool 1 holds 100% of data and shuffles globally in RAM
	if (train_count <= pool_capacity) {
		std::filesystem::remove(ocean_path, ec);
		ec.clear();
		std::filesystem::rename(source_train_path, ocean_path, ec);
		if (ec) {
			ec.clear();
			std::filesystem::copy_file(
				source_train_path,
				ocean_path,
				std::filesystem::copy_options::overwrite_existing,
				ec);
			std::filesystem::remove(source_train_path, ec);
			if (ec) {
				cleanup_temps();
				return false;
			}
		}
		train_tokens_out = train_count;
		const uintmax_t ocean_bytes =
			static_cast<uintmax_t>(train_count) * sizeof(uint32_t);
		std::cout << "[OK] Token ocean streamed to disk: " << train_count << " tokens ("
			  << format_file_size(ocean_bytes) << ") -> "
			  << ocean_path.filename().string() << "\n"
			  << std::flush;
		return true;
	}

	// Multi-pool corpus (> pool_capacity): globally shuffle aligned blocks across the Ocean on disk
	const size_t unit_align = std::max<size_t>(1, context_window);
	size_t block_tokens = (config::chunk_tokens >= unit_align)
				      ? ((config::chunk_tokens / unit_align) * unit_align)
				      : unit_align;
	if (block_tokens == 0) {
		block_tokens = unit_align;
	}
	const size_t num_full_blocks = train_count / block_tokens;
	const size_t tail_tokens = train_count % block_tokens;

	std::vector<uint32_t> block_order(num_full_blocks);
	std::iota(block_order.begin(), block_order.end(), 0U);
	std::mt19937 block_rng(shuffle_seed);
	std::shuffle(block_order.begin(), block_order.end(), block_rng);

	std::ifstream train_in(source_train_path, std::ios::binary);
	std::ofstream ocean_out(ocean_path, std::ios::binary | std::ios::trunc);
	if (!train_in || !ocean_out) {
		cleanup_temps();
		return false;
	}

	std::vector<uint32_t> block_buf(block_tokens);
	const std::streamsize block_bytes = static_cast<std::streamsize>(block_tokens * sizeof(uint32_t));
	for (uint32_t block_idx : block_order) {
		const std::streamoff offset_bytes =
			static_cast<std::streamoff>(static_cast<uint64_t>(block_idx) * block_tokens * sizeof(uint32_t));
		train_in.seekg(offset_bytes, std::ios::beg);
		train_in.read(reinterpret_cast<char*>(block_buf.data()), block_bytes);
		if (train_in.gcount() != block_bytes) {
			ocean_out.close();
			std::filesystem::remove(ocean_path, ec);
			cleanup_temps();
			return false;
		}
		ocean_out.write(reinterpret_cast<const char*>(block_buf.data()), block_bytes);
		if (!ocean_out) {
			ocean_out.close();
			std::filesystem::remove(ocean_path, ec);
			cleanup_temps();
			return false;
		}
	}

	if (tail_tokens > 0) {
		const std::streamoff tail_offset =
			static_cast<std::streamoff>(static_cast<uint64_t>(num_full_blocks) * block_tokens * sizeof(uint32_t));
		train_in.seekg(tail_offset, std::ios::beg);
		if (!copy_token_range(train_in, tail_tokens, scratch, &ocean_out, nullptr)) {
			ocean_out.close();
			std::filesystem::remove(ocean_path, ec);
			cleanup_temps();
			return false;
		}
	}

	train_in.close();
	ocean_out.close();
	if (!ocean_out) {
		std::filesystem::remove(ocean_path, ec);
		cleanup_temps();
		return false;
	}
	std::filesystem::remove(source_train_path, ec);
	cleanup_temps();

	train_tokens_out = train_count;
	const uintmax_t ocean_bytes = static_cast<uintmax_t>(train_count) * sizeof(uint32_t);
	std::cout << "[OK] Globally block-shuffled token ocean (" << num_full_blocks << " x "
		  << block_tokens << "-token blocks): " << train_count << " tokens ("
		  << format_file_size(ocean_bytes) << ") -> " << ocean_path.filename().string()
		  << "\n"
		  << std::flush;
	return true;
}

bool Parser::save_token_ocean(const std::filesystem::path& target_path,
			      const std::vector<uint32_t>& token_stream) const {
	if (token_stream.empty()) {
		std::cerr << "[!] Cannot write empty token ocean to " << target_path.string()
			  << "\n";
		return false;
	}

	if (!target_path.parent_path().empty()) {
		std::error_code dir_error;
		std::filesystem::create_directories(target_path.parent_path(), dir_error);
	}

	std::ofstream output_file(target_path, std::ios::binary | std::ios::trunc);
	if (!output_file) {
		std::cerr << "[!] Fatal: Failed to open " << target_path.string()
			  << " for writing token ocean.\n";
		return false;
	}

	const size_t total_tokens = token_stream.size();
	size_t written_tokens = 0;
	while (written_tokens < total_tokens) {
		const size_t batch_count =
			std::min<size_t>(config::chunk_tokens, total_tokens - written_tokens);
		output_file.write(
			reinterpret_cast<const char*>(token_stream.data() + written_tokens),
			static_cast<std::streamsize>(batch_count * sizeof(uint32_t)));
		if (!output_file) {
			std::cerr << "[!] Write failure while streaming token ocean to "
				  << target_path.string() << "\n";
			output_file.close();
			std::error_code remove_error;
			std::filesystem::remove(target_path, remove_error);
			return false;
		}
		written_tokens += batch_count;
	}

	output_file.close();
	if (!output_file) {
		std::cerr << "[!] Failed to flush token ocean to " << target_path.string() << "\n";
		std::error_code remove_error;
		std::filesystem::remove(target_path, remove_error);
		return false;
	}

	const uintmax_t total_bytes =
		static_cast<uintmax_t>(total_tokens) * sizeof(uint32_t);
	std::cout << "[OK] Token ocean streamed to disk: " << total_tokens << " tokens ("
		  << format_file_size(total_bytes) << ") -> " << target_path.filename().string()
		  << "\n"
		  << std::flush;
	return true;
}

bool Parser::save_vocabulary(const std::filesystem::path& filepath, const Input& input) {
	if (input.main_dictionary.empty()) {
		if (std::filesystem::exists(filepath)) {
			std::cout << "[i] Preserving existing vocabulary at "
				  << filepath.filename().string()
				  << " (no text tokens in current dataset).\n";
			return true;
		}
		std::cout << "[i] Skipping vocabulary serialization: text dictionary is empty.\n";
		return true;
	}

	// 1. Write to .tmp to prevent corruption during an aborted save
	std::filesystem::path tmp_filepath = filepath.string() + ".tmp";
	if (!tmp_filepath.parent_path().empty()) {
		std::error_code ec;
		std::filesystem::create_directories(tmp_filepath.parent_path(), ec);
	}
	std::ofstream out(tmp_filepath, std::ios::binary);
	if (!out) {
		std::cerr << "[!] Failed to open temporary vocabulary file for writing: "
			  << tmp_filepath.string() << "\n";
		return false;
	}

	// 2. Write Magic Header (Axiom Vocab Key: 'A', 'X', 'V', 'K')
	const char magic[4] = {'A', 'X', 'V', 'K'};
	out.write(magic, 4);

	// 3. Write Dictionary Size
	uint64_t vocab_size = static_cast<uint64_t>(input.main_dictionary.size());
	out.write(reinterpret_cast<const char*>(&vocab_size), sizeof(uint64_t));

	// 4. Stream Key-Value Pairs [Length] [Bytes] [ID]
	for (const auto& [word, id] : input.main_dictionary) {
		uint32_t word_len = static_cast<uint32_t>(word.size());
		out.write(reinterpret_cast<const char*>(&word_len), sizeof(uint32_t));
		out.write(word.data(), word_len);
		out.write(reinterpret_cast<const char*>(&id), sizeof(uint32_t));
	}

	out.close();
	if (!out) {
		std::cerr << "[!] Write failure during vocabulary serialization.\n";
		std::error_code rm_ec;
		std::filesystem::remove(tmp_filepath, rm_ec);
		return false;
	}

	// 5. Atomic Rename (.tmp -> target)
	std::error_code ec;
	std::filesystem::rename(tmp_filepath, filepath, ec);
	if (ec) {
		ec.clear();
		std::filesystem::copy_file(tmp_filepath,
					   filepath,
					   std::filesystem::copy_options::overwrite_existing,
					   ec);
		std::filesystem::remove(tmp_filepath, ec);
	}

	std::cout << "[OK] Serialized " << vocab_size << " text tokens to "
		  << filepath.filename().string() << "\n"
		  << std::flush;
	return true;
}

bool Parser::load_vocabulary(const std::filesystem::path& filepath, Input& input) {
	std::ifstream in(filepath, std::ios::binary);
	if (!in) {
		std::cerr << "[!] Failed to open vocabulary file: " << filepath.string() << "\n";
		return false;
	}

	// 1. Validate Magic Header
	char magic[4] = {};
	in.read(magic, 4);
	if (in.gcount() != 4 || magic[0] != 'A' || magic[1] != 'X' || magic[2] != 'V' ||
	    magic[3] != 'K') {
		std::cerr << "[!] Fatal: Invalid magic bytes in vocab file: "
			  << filepath.filename().string() << " (expected 'AXVK')\n";
		return false;
	}

	// 2. Read Size and Allocate
	uint64_t vocab_size = 0;
	in.read(reinterpret_cast<char*>(&vocab_size), sizeof(uint64_t));
	if (vocab_size > 2000000ULL)
		return false;
	if (!in) {
		std::cerr << "[!] Fatal: Corrupt vocabulary size header in "
			  << filepath.filename().string() << "\n";
		return false;
	}

	if (vocab_size == 0) {
		std::cout << "[i] Vocabulary file " << filepath.filename().string()
			  << " contains 0 text tokens; preserving initialized vocabulary.\n";
		in.close();
		return true;
	}

	input.main_dictionary.clear();
	input.reverse_main_dictionary.clear();
	input.main_dictionary.reserve(vocab_size);
	input.reverse_main_dictionary.reserve(vocab_size);

	// 3. Reconstruct Dictionaries
	for (uint64_t i = 0; i < vocab_size; ++i) {
		uint32_t word_len = 0;
		in.read(reinterpret_cast<char*>(&word_len), sizeof(uint32_t));
		if (word_len > 10485760U)
			return false;
		if (!in) {
			std::cerr << "[!] Incomplete vocabulary entry length at index " << i
				  << "\n";
			return false;
		}

		std::string word(word_len, '\0');
		in.read(word.data(), word_len);
		if (!in) {
			std::cerr << "[!] Incomplete vocabulary word data at index " << i << "\n";
			return false;
		}

		uint32_t id = 0;
		in.read(reinterpret_cast<char*>(&id), sizeof(uint32_t));
		if (!in) {
			std::cerr << "[!] Incomplete vocabulary token ID at index " << i << "\n";
			return false;
		}

		input.main_dictionary[word] = id;
		input.reverse_main_dictionary[id] = word;
		if (id >= input.next_text_id) {
			input.next_text_id = id + 1;
		}
	}

	in.close();
	build_embedding_table(input);
	std::cout << "[OK] Loaded " << vocab_size << " vocabulary tokens from "
		  << filepath.filename().string() << "\n"
		  << std::flush;
	return true;
}

void Parser::init_reserved_vocabulary(Input& input) {
	static const char* kX86Primitives[] = {
		"rax",		"rbx",	      "rcx",	      "rdx",	      "rsi",	  "rdi",
		"rbp",		"rsp",	      "r8",	      "r9",	      "r10",	  "r11",
		"r12",		"r13",	      "r14",	      "r15",	      "xmm0",	  "xmm1",
		"xmm2",		"xmm3",	      "xmm4",	      "xmm5",	      "xmm6",	  "xmm7",
		"ymm0",		"ymm1",	      "ymm2",	      "ymm3",	      "zmm0",	  "zmm1",
		"zmm2",		"zmm3",	      "mov",	      "lea",	      "add",	  "sub",
		"imul",		"idiv",	      "xor",	      "and",	      "or",	  "shl",
		"shr",		"cmp",	      "test",	      "je",	      "jne",	  "jg",
		"jl",		"jmp",	      "call",	      "ret",	      "push",	  "pop",
		"vmovups",	"vmovaps",    "vfmadd231ps",  "vbroadcastss", "vxorps",	  "vaddps",
		"section",	".text",      ".data",	      "global",	      "[intent]", "[var]",
		"<|im_start|>", "<|im_end|>", "<|endoftext|>"};

	uint32_t next_id = 4;
	for (const char* token_str : kX86Primitives) {
		input.main_dictionary[token_str] = next_id;
		input.reverse_main_dictionary[next_id] = token_str;
		++next_id;
	}
	if (next_id > input.next_text_id) {
		input.next_text_id = next_id;
	}
}

std::vector<uint32_t> Parser::tokenize_exact_code(const Input& input,
						  const std::string& raw_prompt) const {
	std::vector<uint32_t> out_ids;
	out_ids.reserve(raw_prompt.size() / 2);

	size_t pos = 0;
	const size_t n = raw_prompt.size();
	while (pos < n) {
		size_t match_len = std::min<size_t>(32, n - pos);
		uint32_t matched_id = 1; // Default <UNK>
		bool found = false;

		for (; match_len >= 1; --match_len) {
			std::string_view candidate(raw_prompt.data() + pos, match_len);
			auto it = input.main_dictionary.find(std::string(candidate));
			if (it != input.main_dictionary.end()) {
				matched_id = it->second;
				found = true;
				break;
			}
		}
		out_ids.push_back(matched_id);
		pos += found ? match_len : 1;
	}
	return out_ids;
}

std::vector<uint32_t> Parser::tokenize_exact_code(
	const std::string& raw_prompt, const std::unordered_map<std::string, uint32_t>& vocab) {
	std::vector<uint32_t> out_ids;
	out_ids.reserve(raw_prompt.size() / 2);
	size_t pos = 0;
	const size_t n = raw_prompt.size();
	while (pos < n) {
		size_t match_len = std::min<size_t>(32, n - pos);
		uint32_t matched_id = 1;
		bool found = false;
		for (; match_len >= 1; --match_len) {
			std::string_view candidate(raw_prompt.data() + pos, match_len);
			auto it = vocab.find(std::string(candidate));
			if (it != vocab.end()) {
				if (it->second == vocab::ENDOFTEXT_TOKEN_ID) {
					continue;
				}
				if (it->second == vocab::IM_START_TOKEN_ID) {
					bool at_line_start =
						(pos == 0 || raw_prompt[pos - 1] == '\n');
					bool valid_role =
						(pos + 19 <= n &&
						 raw_prompt.compare(pos + 12, 7, "system\n") ==
							 0) ||
						(pos + 17 <= n &&
						 raw_prompt.compare(pos + 12, 5, "user\n") == 0) ||
						(pos + 22 <= n &&
						 raw_prompt.compare(pos + 12, 10, "assistant\n") ==
							 0);
					if (!at_line_start || !valid_role) {
						continue;
					}
				}
				if (it->second == vocab::IM_END_TOKEN_ID) {
					bool valid_end =
						(pos + 10 < n && raw_prompt[pos + 10] == '\n') &&
						(pos + 11 == n ||
						 (pos + 23 <= n &&
						  raw_prompt.compare(
							  pos + 11, 12, "<|im_start|>") == 0));
					if (!valid_end) {
						continue;
					}
				}
				matched_id = it->second;
				found = true;
				break;
			}
		}
		out_ids.push_back(matched_id);
		pos += found ? match_len : 1;
	}
	return out_ids;
}
