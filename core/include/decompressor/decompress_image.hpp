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

#ifndef include_decompress_image
#define include_decompress_image

#include <cstdint>
#include <filesystem>
#include <vector>

struct DecodedImage {
	int width = 0;
	int height = 0;
	int channels = 0;               // 3 = RGB, 1 = Grayscale, 4 = RGBA
	std::vector<uint8_t> pixels {}; // Raw RGB pixel bytes [0x00 .. 0xFF]
	bool is_valid = false;
};

class DecompressImage {
public:
	// Master image decoder
	static DecodedImage decode_image(const std::filesystem::path& file_path);

	// Individual format decoders
	static DecodedImage decompress_png(const std::filesystem::path& file_path);
	static DecodedImage decompress_jpeg(const std::filesystem::path& file_path);
	static DecodedImage decompress_bmp(const std::filesystem::path& file_path);
	static DecodedImage decompress_webp(const std::filesystem::path& file_path);
};

#endif // include_decompress_image