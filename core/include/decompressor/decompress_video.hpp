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