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

#ifndef include_decompress_document
#define include_decompress_document

#include <filesystem>
#include <string>

class DecompressDocument {
public:
	// Master text extractor: inspects format and extracts clean UTF-8 text & math
	static std::string extract_text(const std::filesystem::path& file_path);

	// Individual document format decompressors
	static std::string decompress_pdf(const std::filesystem::path& file_path);
	static std::string decompress_docx(const std::filesystem::path& file_path);
	static std::string decompress_text(const std::filesystem::path& file_path);
	static std::string decompress_markdown(const std::filesystem::path& file_path);
};

#endif // include_decompress_document