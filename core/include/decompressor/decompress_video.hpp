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

#ifndef include_decompress_video
#define include_decompress_video

#include <cstdint>
#include <filesystem>
#include <vector>

struct DecodedVideo {
	int width = 0;
	int height = 0;
	int frame_count = 0;
	double fps = 0.0;
	std::vector<uint8_t> raw_frames {}; // Sequential RGB frame pixel bytes
	bool is_valid = false;
};

class DecompressVideo {
public:
	// Master video decoder
	static DecodedVideo decode_video(const std::filesystem::path& file_path);

	// Individual format decoders
	static DecodedVideo decompress_mp4(const std::filesystem::path& file_path);
	static DecodedVideo decompress_mkv(const std::filesystem::path& file_path);
};

#endif // include_decompress_video