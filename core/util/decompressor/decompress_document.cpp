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

#include "decompressor/decompress_document.hpp"
#include "Identifier.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <zlib.h>

static bool is_valid_token(const std::string& text) {
	if (text.empty()) {
		return false;
	}
	size_t alphanumeric_count = 0;
	size_t alphabetic_count = 0;
	size_t non_space_count = 0;
	for (unsigned char character : text) {
		if (character == 0 ||
		    (character < 32 && character != '\n' && character != '\r' && character != '\t') ||
		    character == 127) {
			return false;
		}
		if (!std::isspace(character)) {
			non_space_count++;
			if (std::isalnum(character)) {
				alphanumeric_count++;
			}
			if (std::isalpha(character)) {
				alphabetic_count++;
			}
		}
	}
	if (non_space_count == 0) {
		return false;
	}
	// Reject binary noise fragments like "9$k#" or "a!~"
	return (alphanumeric_count * 10 >= non_space_count * 7) &&
	       (alphabetic_count > 0 || alphanumeric_count == non_space_count);
}

static std::string inflate_stream_data(const char* compressed_data, size_t compressed_size) {
	if (!compressed_data || compressed_size == 0) {
		return "";
	}

	auto run_inflate = [&](int window_bits) -> std::string {
		z_stream stream {};
		stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed_data));
		stream.avail_in = static_cast<uInt>(compressed_size);

		if (inflateInit2(&stream, window_bits) != Z_OK) {
			return "";
		}

		std::string output;
		char chunk[16384];
		constexpr size_t max_inflated_bytes = 64 * 1024 * 1024;
		int status = Z_OK;

		while (status == Z_OK) {
			stream.next_out = reinterpret_cast<Bytef*>(chunk);
			stream.avail_out = sizeof(chunk);
			status = inflate(&stream, Z_NO_FLUSH);
			if (status != Z_OK && status != Z_STREAM_END && status != Z_BUF_ERROR) {
				inflateEnd(&stream);
				return "";
			}
			size_t produced = sizeof(chunk) - stream.avail_out;
			if (produced > 0) {
				if (output.size() + produced > max_inflated_bytes) {
					inflateEnd(&stream);
					return "";
				}
				output.append(chunk, produced);
			}
			if (status == Z_STREAM_END || (produced == 0 && stream.avail_in == 0)) {
				break;
			}
		}

		inflateEnd(&stream);
		return output;
	};

	std::string inflated = run_inflate(MAX_WBITS);
	if (inflated.empty()) {
		inflated = run_inflate(-MAX_WBITS);
	}
	return inflated;
}

static bool is_image_dictionary(const std::string& dictionary) {
	return dictionary.find("/Subtype /Image") != std::string::npos ||
	       dictionary.find("/Subtype/Image") != std::string::npos ||
	       dictionary.find("/DCTDecode") != std::string::npos ||
	       dictionary.find("/JBIG2Decode") != std::string::npos ||
	       dictionary.find("/JPXDecode") != std::string::npos ||
	       dictionary.find("/CCITTFaxDecode") != std::string::npos ||
	       dictionary.find("/FontFile") != std::string::npos ||
	       dictionary.find("/Metadata") != std::string::npos;
}

// Reads one PDF literal string starting at raw[position] == '(' and leaves position just
// past the matching ')'. Handles nesting, \n \r \t \b \f, \ddd octal and line continuations.
static std::string read_pdf_literal(const std::string& raw, size_t& position, size_t limit) {
	std::string literal;
	int depth = 1;
	++position;
	while (position < limit && depth > 0) {
		const char current = raw[position];
		if (current == '\\' && position + 1 < limit) {
			++position;
			const char escaped = raw[position];
			if (escaped >= '0' && escaped <= '7') {
				int code = 0;
				int digits = 0;
				while (digits < 3 && position < limit && raw[position] >= '0' && raw[position] <= '7') {
					code = code * 8 + (raw[position] - '0');
					++position;
					++digits;
				}
				literal += static_cast<char>(code & 0xFF);
				continue;
			}
			switch (escaped) {
			case 'n': literal += '\n'; break;
			case 'r': literal += '\r'; break;
			case 't': literal += '\t'; break;
			case 'b': literal += '\b'; break;
			case 'f': literal += '\f'; break;
			case '\r':
				if (position + 1 < limit && raw[position + 1] == '\n') {
					++position;
				}
				break;
			case '\n': break;
			default: literal += escaped; break;
			}
		} else if (current == '(') {
			++depth;
			literal += '(';
		} else if (current == ')') {
			--depth;
			if (depth > 0) {
				literal += ')';
			}
		} else {
			literal += current;
		}
		++position;
	}

	// Map Windows-1252 punctuation to ASCII and drop other high bytes so that raw
	// font-encoded bytes never reach the tokenizer as invalid UTF-8
	std::string normalized;
	normalized.reserve(literal.size());
	for (unsigned char byte : literal) {
		if (byte < 128) {
			normalized += static_cast<char>(byte);
		} else if (byte == 0x91 || byte == 0x92) {
			normalized += '\'';
		} else if (byte == 0x93 || byte == 0x94) {
			normalized += '"';
		} else if (byte == 0x96 || byte == 0x97 || byte == 0xAD) {
			normalized += '-';
		} else if (byte == 0x95) {
			normalized += '*';
		} else if (byte == 0x85) {
			normalized += "...";
		} else if (byte == 0xA0) {
			normalized += ' ';
		}
	}
	return normalized;
}

static void extract_text_runs(const std::string& raw, std::ostringstream& extracted) {
	const size_t length = raw.size();
	size_t position = 0;

	while (position < length) {
		size_t begin_text = raw.find("BT", position);
		if (begin_text == std::string::npos) {
			break;
		}

		bool valid_before =
			(begin_text == 0 ||
			 std::isspace(static_cast<unsigned char>(raw[begin_text - 1])));
		bool valid_after =
			(begin_text + 2 < length &&
			 std::isspace(static_cast<unsigned char>(raw[begin_text + 2])));
		if (!valid_before || !valid_after) {
			position = begin_text + 2;
			continue;
		}

		size_t end_text = begin_text + 2;
		bool found_end = false;
		while (end_text < length) {
			end_text = raw.find("ET", end_text);
			if (end_text == std::string::npos || (end_text - begin_text) > 65536) {
				break;
			}
			bool end_valid_before =
				std::isspace(static_cast<unsigned char>(raw[end_text - 1])) ||
				raw[end_text - 1] == ')' || raw[end_text - 1] == ']' ||
				raw[end_text - 1] == '>';
			bool end_valid_after =
				(end_text + 2 == length ||
				 std::isspace(static_cast<unsigned char>(raw[end_text + 2])));
			if (end_valid_before && end_valid_after) {
				found_end = true;
				break;
			}
			end_text += 2;
		}

		if (!found_end) {
			position = begin_text + 2;
			continue;
		}

		bool is_binary_chunk = false;
		for (size_t index = begin_text; index < end_text; ++index) {
			unsigned char character = static_cast<unsigned char>(raw[index]);
			if (character == 0 ||
			    (character < 32 && character != '\n' && character != '\r' &&
			     character != '\t')) {
				is_binary_chunk = true;
				break;
			}
		}
		if (is_binary_chunk) {
			position = begin_text + 2;
			continue;
		}

		size_t scan_index = begin_text + 2;
		while (scan_index < end_text) {
			if (raw[scan_index] == '(') {
				std::string literal = read_pdf_literal(raw, scan_index, end_text);
				if (is_valid_token(literal)) {
					extracted << literal << " ";
				}
			} else if (raw[scan_index] == '[') {
				// TJ array: literals are glyph runs of one word; only a large negative
				// kerning value (below -200) is a real word gap
				std::string joined;
				++scan_index;
				while (scan_index < end_text && raw[scan_index] != ']') {
					if (raw[scan_index] == '(') {
						joined += read_pdf_literal(raw, scan_index, end_text);
					} else if (raw[scan_index] == '-' || raw[scan_index] == '+' ||
						   raw[scan_index] == '.' ||
						   std::isdigit(static_cast<unsigned char>(raw[scan_index]))) {
						char* number_end = nullptr;
						const double kerning =
							std::strtod(raw.c_str() + scan_index, &number_end);
						const size_t consumed = static_cast<size_t>(
							number_end - (raw.c_str() + scan_index));
						if (consumed == 0) {
							++scan_index;
						} else {
							scan_index += consumed;
							if (kerning < -200.0) {
								joined += ' ';
							}
						}
					} else {
						++scan_index;
					}
				}
				if (is_valid_token(joined)) {
					extracted << joined << " ";
				}
			} else {
				scan_index++;
			}
		}
		position = end_text + 2;
	}
}

// Subset fonts with custom encodings turn real text into shifted glyph codes that still look
// like words ("obk^jfkd"). Natural Latin-script text puts about 70% of its letters in
// {e,t,a,o,i,n,s,h,r}; a stream of at least 60 letters below 50% is a mis-encoded stream.
static bool has_natural_letters(const std::string& text) {
	size_t letter_count = 0;
	size_t common_count = 0;
	for (unsigned char character : text) {
		if (!std::isalpha(character)) {
			continue;
		}
		++letter_count;
		switch (std::tolower(character)) {
		case 'e': case 't': case 'a': case 'o': case 'i':
		case 'n': case 's': case 'h': case 'r':
			++common_count;
			break;
		default:
			break;
		}
	}
	return letter_count < 60 || common_count * 100 >= letter_count * 50;
}

// Extracts the text of one PDF content stream, dropping it whole if it is mis-encoded
static void extract_pdf_text(const std::string& raw, std::ostringstream& extracted) {
	std::ostringstream stream_text;
	extract_text_runs(raw, stream_text);
	const std::string text = stream_text.str();
	if (has_natural_letters(text)) {
		extracted << text;
	}
}

// True when the bytes are plausible text: no NUL and at most 1% control characters
static bool looks_like_text(const std::string& content) {
	size_t control_bytes = 0;
	for (unsigned char character : content) {
		if (character == 0) {
			return false;
		}
		if ((character < 32 && character != '\t' && character != '\n' && character != '\r' &&
		     character != '\f') ||
		    character == 127) {
			++control_bytes;
		}
	}
	return control_bytes * 100 <= content.size();
}

std::string DecompressDocument::decompress_text(const std::filesystem::path& file_path) {
	std::ifstream in(file_path, std::ios::binary);
	if (!in) {
		return "";
	}

	std::ostringstream ss;
	ss << in.rdbuf();
	std::string content = ss.str();

	// Binary data renamed to a text extension must never reach the text tokenizer
	if (!looks_like_text(content)) {
		return "";
	}

	// Strip UTF-8 BOM if present
	if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
	    static_cast<unsigned char>(content[1]) == 0xBB &&
	    static_cast<unsigned char>(content[2]) == 0xBF) {
		content.erase(0, 3);
	}

	return content;
}

std::string DecompressDocument::decompress_markdown(const std::filesystem::path& file_path) {
	return decompress_text(file_path);
}

std::string DecompressDocument::decompress_pdf(const std::filesystem::path& file_path) {
	std::ifstream in(file_path, std::ios::binary);
	if (!in) {
		return "";
	}

	std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	if (raw.empty()) {
		return "";
	}

	std::ostringstream extracted;
	const size_t length = raw.size();
	size_t cursor = 0;

	while (cursor < length) {
		size_t stream_position = raw.find("stream", cursor);
		if (stream_position == std::string::npos) {
			extract_pdf_text(raw.substr(cursor), extracted);
			break;
		}

		// Verify "stream" is a standalone PDF keyword followed by EOL
		bool valid_keyword_before =
			(stream_position == 0 ||
			 std::isspace(static_cast<unsigned char>(raw[stream_position - 1])) ||
			 raw[stream_position - 1] == '>');
		size_t data_start = stream_position + 6;
		bool valid_keyword_after = false;
		if (data_start < length) {
			if (raw[data_start] == '\r' && data_start + 1 < length &&
			    raw[data_start + 1] == '\n') {
				data_start += 2;
				valid_keyword_after = true;
			} else if (raw[data_start] == '\n' || raw[data_start] == '\r') {
				data_start += 1;
				valid_keyword_after = true;
			}
		}

		if (!valid_keyword_before || !valid_keyword_after) {
			if (stream_position + 6 > cursor) {
				extract_pdf_text(raw.substr(cursor, stream_position + 6 - cursor),
						 extracted);
			}
			cursor = stream_position + 6;
			continue;
		}

		if (stream_position > cursor) {
			extract_pdf_text(raw.substr(cursor, stream_position - cursor), extracted);
		}

		size_t endstream_position = raw.find("endstream", data_start);
		if (endstream_position == std::string::npos) {
			break;
		}

		size_t dictionary_lookback = (stream_position > 1024) ? (stream_position - 1024) : 0;
		size_t object_position = raw.rfind("obj", stream_position);
		if (object_position != std::string::npos && object_position > dictionary_lookback) {
			dictionary_lookback = object_position;
		}
		std::string dictionary =
			raw.substr(dictionary_lookback, stream_position - dictionary_lookback);

		if (!is_image_dictionary(dictionary)) {
			size_t data_end = endstream_position;
			if (data_end > data_start && raw[data_end - 1] == '\n') {
				data_end--;
			}
			if (data_end > data_start && raw[data_end - 1] == '\r') {
				data_end--;
			}

			if (dictionary.find("/FlateDecode") != std::string::npos) {
				if (data_end > data_start) {
					std::string inflated = inflate_stream_data(
						raw.data() + data_start, data_end - data_start);
					if (!inflated.empty()) {
						extract_pdf_text(inflated, extracted);
					}
				}
			} else if (dictionary.find("/Filter") == std::string::npos) {
				if (data_end > data_start) {
					extract_pdf_text(
						raw.substr(data_start, data_end - data_start),
						extracted);
				}
			}
		}

		cursor = endstream_position + 9;
	}

	return extracted.str();
}

static void extract_docx_xml(const std::string& xml, std::ostringstream& extracted) {
	const size_t length = xml.size();
	for (size_t index = 0; index + 4 < length; ++index) {
		if (xml[index] == '<' && xml[index + 1] == 'w' && xml[index + 2] == ':' &&
		    xml[index + 3] == 'p' &&
		    (xml[index + 4] == '>' || xml[index + 4] == ' ' || xml[index + 4] == '/')) {
			extracted << "\n";
		}

		if (xml[index] == '<' && xml[index + 1] == 'w' && xml[index + 2] == ':' &&
		    xml[index + 3] == 't' && (xml[index + 4] == '>' || xml[index + 4] == ' ')) {
			size_t tag_close = index + 4;
			while (tag_close < length && xml[tag_close] != '>') {
				tag_close++;
			}
			if (tag_close < length && xml[tag_close] == '>') {
				size_t content_start = tag_close + 1;
				size_t end_tag = content_start;
				while (end_tag + 5 < length) {
					if (xml[end_tag] == '<' && xml[end_tag + 1] == '/' &&
					    xml[end_tag + 2] == 'w' && xml[end_tag + 3] == ':' &&
					    xml[end_tag + 4] == 't' && xml[end_tag + 5] == '>') {
						break;
					}
					end_tag++;
				}
				if (end_tag > content_start) {
					extracted << xml.substr(content_start, end_tag - content_start)
						  << " ";
				}
				index = end_tag + 5;
			}
		}
	}
}

std::string DecompressDocument::decompress_docx(const std::filesystem::path& file_path) {
	std::ifstream in(file_path, std::ios::binary);
	if (!in) {
		return "";
	}

	std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	if (raw.empty()) {
		return "";
	}

	std::ostringstream extracted;
	const size_t length = raw.size();

	// 1. Parse ZIP local file headers (PK\x03\x04) and inflate compressed XML entries
	size_t position = 0;
	while (position + 30 <= length) {
		size_t signature = raw.find("PK\x03\x04", position);
		if (signature == std::string::npos || signature + 30 > length) {
			break;
		}
		const auto* header = reinterpret_cast<const unsigned char*>(raw.data() + signature);
		uint16_t method = static_cast<uint16_t>(header[8] | (header[9] << 8));
		uint32_t compressed_size = static_cast<uint32_t>(
			header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24));
		uint16_t name_length = static_cast<uint16_t>(header[26] | (header[27] << 8));
		uint16_t extra_length = static_cast<uint16_t>(header[28] | (header[29] << 8));

		size_t data_offset = signature + 30 + name_length + extra_length;
		if (name_length == 0 || name_length > 512 || data_offset > length) {
			position = signature + 4;
			continue;
		}

		std::string entry_name = raw.substr(signature + 30, name_length);
		size_t next_signature = raw.find("PK", data_offset);
		size_t payload_length = compressed_size;
		if (payload_length == 0 || data_offset + payload_length > length) {
			payload_length = (next_signature != std::string::npos)
						 ? (next_signature - data_offset)
						 : (length - data_offset);
		}

		if (entry_name.find("word/") != std::string::npos &&
		    entry_name.find(".xml") != std::string::npos && payload_length > 0) {
			if (method == 8) {
				std::string xml =
					inflate_stream_data(raw.data() + data_offset, payload_length);
				if (!xml.empty()) {
					extract_docx_xml(xml, extracted);
				}
			} else if (method == 0) {
				extract_docx_xml(raw.substr(data_offset, payload_length), extracted);
			}
		}

		position = (compressed_size > 0 && data_offset + compressed_size <= length)
				   ? (data_offset + compressed_size)
				   : (data_offset + 1);
	}

	// 2. Fallback for uncompressed/synthetic XML buffers containing <w:t>
	if (extracted.str().empty() && raw.find("<w:t") != std::string::npos) {
		extract_docx_xml(raw, extracted);
	}

	return extracted.str();
}

std::string DecompressDocument::extract_text(const std::filesystem::path& file_path) {
	IdentifiedFile info = Identifier::identify(file_path);
	switch (info.format) {
	case FileFormat::PDF:
		return decompress_pdf(file_path);
	case FileFormat::DOCX:
		return decompress_docx(file_path);
	case FileFormat::MD:
		return decompress_markdown(file_path);
	case FileFormat::TXT:
	case FileFormat::JSON:
	case FileFormat::CSV:
	case FileFormat::LOG:
	case FileFormat::CODE:
		return decompress_text(file_path);
	default:
		return "";
	}
}
