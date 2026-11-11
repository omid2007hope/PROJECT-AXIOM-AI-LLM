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

#ifndef include_compress_image
#define include_compress_image

#include <cstdint>
#include <filesystem>
#include <vector>

class CompressImage {
public:
	// Encodes RGB pixel byte array into a standards-compliant 24-bit uncompressed BMP file
	static bool pixels_to_bmp(const std::vector<uint8_t>& pixels,
				  int width,
				  int height,
				  const std::filesystem::path& file_path);

	// Decodes discrete visual tokens ([64,512..64,767]) to RGB pixels and serializes to BMP
	static bool tokens_to_bmp(const std::vector<uint32_t>& tokens,
				  int width,
				  int height,
				  const std::filesystem::path& file_path);

	// Translates discrete visual token IDs ([64,512..64,767]) back to raw RGB bytes [0..255]
	static std::vector<uint8_t> tokens_to_pixels(const std::vector<uint32_t>& tokens);

	// Translates raw RGB pixel bytes [0..255] to discrete visual token IDs ([64,512..64,767])
	static std::vector<uint32_t> pixels_to_tokens(const std::vector<uint8_t>& pixels);
};

#endif // include_compress_image

