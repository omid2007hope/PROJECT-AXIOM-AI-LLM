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

