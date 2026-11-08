#include "decompressor/decompress_document.hpp"
#include "Identifier.hpp"

#include <algorithm>
#include <cctype>
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

static void extract_pdf_text(const std::string& raw, std::ostringstream& extracted) {
	const size_t n = raw.size();
	size_t pos = 0;

	while (pos < n) {
		size_t bt = raw.find("BT", pos);
		if (bt == std::string::npos) {
			break;
		}

		bool valid_before = (bt == 0 || std::isspace(static_cast<unsigned char>(raw[bt - 1])));
		bool valid_after = (bt + 2 < n && std::isspace(static_cast<unsigned char>(raw[bt + 2])));
		if (!valid_before || !valid_after) {
			pos = bt + 2;
			continue;
		}

		size_t et = bt + 2;
		bool found_et = false;
		while (et < n) {
			et = raw.find("ET", et);
			if (et == std::string::npos || (et - bt) > 65536) {
				break;
			}
			bool et_before = std::isspace(static_cast<unsigned char>(raw[et - 1])) ||
					 raw[et - 1] == ')' || raw[et - 1] == ']' || raw[et - 1] == '>';
			bool et_after = (et + 2 == n || std::isspace(static_cast<unsigned char>(raw[et + 2])));
			if (et_before && et_after) {
				found_et = true;
				break;
			}
			et += 2;
		}

		if (!found_et) {
			pos = bt + 2;
			continue;
		}

		bool is_binary_chunk = false;
		for (size_t k = bt; k < et; ++k) {
			unsigned char c = static_cast<unsigned char>(raw[k]);
			if (c == 0 || (c < 32 && c != '\n' && c != '\r' && c != '\t')) {
				is_binary_chunk = true;
				break;
			}
		}
		if (is_binary_chunk) {
			pos = bt + 2;
			continue;
		}

		size_t j = bt + 2;
		while (j < et) {
			if (raw[j] == '(') {
				j++;
				std::string literal;
				int depth = 1;
				while (j < et && depth > 0) {
					if (raw[j] == '\\' && j + 1 < et) {
						j++;
						switch (raw[j]) {
						case 'n': literal += '\n'; break;
						case 'r': literal += '\r'; break;
						case 't': literal += '\t'; break;
						default: literal += raw[j]; break;
						}
					} else if (raw[j] == '(') {
						depth++;
						literal += '(';
					} else if (raw[j] == ')') {
						depth--;
						if (depth > 0) {
							literal += ')';
						}
					} else {
						literal += raw[j];
					}
					j++;
				}
				if (is_valid_token(literal)) {
					extracted << literal << " ";
				}
			} else {
				j++;
			}
		}
		pos = et + 2;
	}
}

std::string DecompressDocument::decompress_text(const std::filesystem::path& file_path) {
	std::ifstream in(file_path, std::ios::binary);
	if (!in) {
		return "";
	}

	std::ostringstream ss;
	ss << in.rdbuf();
	std::string content = ss.str();

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
	const size_t n = raw.size();
	size_t cursor = 0;

	while (cursor < n) {
		size_t stream_pos = raw.find("stream", cursor);
		if (stream_pos == std::string::npos) {
			extract_pdf_text(raw.substr(cursor), extracted);
			break;
		}

		// Verify "stream" is a standalone PDF keyword followed by EOL
		bool valid_kw_before =
			(stream_pos == 0 ||
			 std::isspace(static_cast<unsigned char>(raw[stream_pos - 1])) ||
			 raw[stream_pos - 1] == '>');
		size_t data_start = stream_pos + 6;
		bool valid_kw_after = false;
		if (data_start < n) {
			if (raw[data_start] == '\r' && data_start + 1 < n &&
			    raw[data_start + 1] == '\n') {
				data_start += 2;
				valid_kw_after = true;
			} else if (raw[data_start] == '\n' || raw[data_start] == '\r') {
				data_start += 1;
				valid_kw_after = true;
			}
		}

		if (!valid_kw_before || !valid_kw_after) {
			if (stream_pos + 6 > cursor) {
				extract_pdf_text(raw.substr(cursor, stream_pos + 6 - cursor),
						 extracted);
			}
			cursor = stream_pos + 6;
			continue;
		}

		if (stream_pos > cursor) {
			extract_pdf_text(raw.substr(cursor, stream_pos - cursor), extracted);
		}

		size_t endstream_pos = raw.find("endstream", data_start);
		if (endstream_pos == std::string::npos) {
			break;
		}

		size_t dict_lookback = (stream_pos > 1024) ? (stream_pos - 1024) : 0;
		size_t obj_pos = raw.rfind("obj", stream_pos);
		if (obj_pos != std::string::npos && obj_pos > dict_lookback) {
			dict_lookback = obj_pos;
		}
		std::string dictionary = raw.substr(dict_lookback, stream_pos - dict_lookback);

		if (!is_image_dictionary(dictionary)) {
			size_t data_end = endstream_pos;
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

		cursor = endstream_pos + 9;
	}

	return extracted.str();
}

static void extract_docx_xml(const std::string& xml, std::ostringstream& extracted) {
	const size_t n = xml.size();
	for (size_t i = 0; i + 4 < n; ++i) {
		if (xml[i] == '<' && xml[i + 1] == 'w' && xml[i + 2] == ':' && xml[i + 3] == 'p' &&
		    (xml[i + 4] == '>' || xml[i + 4] == ' ' || xml[i + 4] == '/')) {
			extracted << "\n";
		}

		if (xml[i] == '<' && xml[i + 1] == 'w' && xml[i + 2] == ':' && xml[i + 3] == 't' &&
		    (xml[i + 4] == '>' || xml[i + 4] == ' ')) {
			size_t tag_close = i + 4;
			while (tag_close < n && xml[tag_close] != '>') {
				tag_close++;
			}
			if (tag_close < n && xml[tag_close] == '>') {
				size_t content_start = tag_close + 1;
				size_t end_tag = content_start;
				while (end_tag + 5 < n) {
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
				i = end_tag + 5;
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
	const size_t n = raw.size();

	// 1. Parse ZIP local file headers (PK\x03\x04) and inflate compressed XML entries
	size_t pos = 0;
	while (pos + 30 <= n) {
		size_t sig = raw.find("PK\x03\x04", pos);
		if (sig == std::string::npos || sig + 30 > n) {
			break;
		}
		const auto* hdr = reinterpret_cast<const unsigned char*>(raw.data() + sig);
		uint16_t method = static_cast<uint16_t>(hdr[8] | (hdr[9] << 8));
		uint32_t comp_size = static_cast<uint32_t>(
			hdr[18] | (hdr[19] << 8) | (hdr[20] << 16) | (hdr[21] << 24));
		uint16_t name_len = static_cast<uint16_t>(hdr[26] | (hdr[27] << 8));
		uint16_t extra_len = static_cast<uint16_t>(hdr[28] | (hdr[29] << 8));

		size_t data_offset = sig + 30 + name_len + extra_len;
		if (name_len == 0 || name_len > 512 || data_offset > n) {
			pos = sig + 4;
			continue;
		}

		std::string entry_name = raw.substr(sig + 30, name_len);
		size_t next_sig = raw.find("PK", data_offset);
		size_t payload_len = comp_size;
		if (payload_len == 0 || data_offset + payload_len > n) {
			payload_len = (next_sig != std::string::npos) ? (next_sig - data_offset)
								      : (n - data_offset);
		}

		if (entry_name.find("word/") != std::string::npos &&
		    entry_name.find(".xml") != std::string::npos && payload_len > 0) {
			if (method == 8) {
				std::string xml =
					inflate_stream_data(raw.data() + data_offset, payload_len);
				if (!xml.empty()) {
					extract_docx_xml(xml, extracted);
				}
			} else if (method == 0) {
				extract_docx_xml(raw.substr(data_offset, payload_len), extracted);
			}
		}

		pos = (comp_size > 0 && data_offset + comp_size <= n) ? (data_offset + comp_size)
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
