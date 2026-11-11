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

