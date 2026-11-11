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