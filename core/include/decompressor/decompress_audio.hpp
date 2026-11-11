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