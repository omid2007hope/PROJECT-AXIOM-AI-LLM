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

#ifndef include_compress_audio
#define include_compress_audio

#include <cstdint>
#include <filesystem>
#include <vector>

class CompressAudio {
public:
	// Encodes linear PCM samples into a standards-compliant RIFF/WAVE container
	static bool pcm_to_wav(const std::vector<uint8_t>& pcm_bytes,
			       int sample_rate,
			       int channels,
			       int bits_per_sample,
			       const std::filesystem::path& file_path);

	// Decodes discrete acoustic tokens ([64,768..65,023]) to PCM samples and serializes to WAV
	static bool tokens_to_wav(const std::vector<uint32_t>& tokens,
				  int sample_rate,
				  int channels,
				  int bits_per_sample,
				  const std::filesystem::path& file_path);

	// Translates discrete acoustic token IDs ([64,768..65,023]) back to raw PCM bytes [0..255]
	static std::vector<uint8_t> tokens_to_pcm(const std::vector<uint32_t>& tokens);

	// Translates raw PCM sample bytes [0..255] to discrete acoustic token IDs ([64,768..65,023])
	static std::vector<uint32_t> pcm_to_tokens(const std::vector<uint8_t>& pcm_bytes);
};

#endif // include_compress_audio

