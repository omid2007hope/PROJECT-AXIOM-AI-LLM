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