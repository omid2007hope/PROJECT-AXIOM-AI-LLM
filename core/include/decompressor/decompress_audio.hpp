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

#ifndef include_decompress_audio
#define include_decompress_audio

#include <cstdint>
#include <filesystem>
#include <vector>

struct DecodedAudio {
	int sample_rate = 0;
	int channels = 0;
	int bits_per_sample = 16;
	std::vector<uint8_t> pcm_bytes {}; // Raw linear PCM waveform samples [0x00 .. 0xFF]
	bool is_valid = false;
};

class DecompressAudio {
public:
	// Master audio decoder
	static DecodedAudio decode_audio(const std::filesystem::path& file_path);

	// Individual format decoders
	static DecodedAudio decompress_wav(const std::filesystem::path& file_path);
	static DecodedAudio decompress_ogg(const std::filesystem::path& file_path);
	static DecodedAudio decompress_mp3(const std::filesystem::path& file_path);
};

#endif // include_decompress_audio