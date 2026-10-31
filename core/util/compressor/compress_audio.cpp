#include "../../include/compressor/compress_audio.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

#include "../../include/parser.hpp"

std::vector<uint8_t> CompressAudio::tokens_to_pcm(const std::vector<uint32_t>& tokens) {
	std::vector<uint8_t> pcm_bytes;
	pcm_bytes.reserve(tokens.size());

	for (uint32_t tok_id : tokens) {
		if (tok_id >= vocab::ACOUSTIC_START_ID && tok_id <= vocab::ACOUSTIC_END_ID) {
			pcm_bytes.push_back(static_cast<uint8_t>(tok_id - vocab::ACOUSTIC_START_ID));
		} else {
			pcm_bytes.push_back(0);
		}
	}

	return pcm_bytes;
}

std::vector<uint32_t> CompressAudio::pcm_to_tokens(const std::vector<uint8_t>& pcm_bytes) {
	std::vector<uint32_t> tokens;
	tokens.reserve(pcm_bytes.size());

	for (uint8_t b : pcm_bytes) {
		tokens.push_back(vocab::ACOUSTIC_START_ID + static_cast<uint32_t>(b));
	}

	return tokens;
}

bool CompressAudio::pcm_to_wav(const std::vector<uint8_t>& pcm_bytes,
			       int sample_rate,
			       int channels,
			       int bits_per_sample,
			       const std::filesystem::path& file_path) {
	if (sample_rate <= 0 || channels <= 0 || bits_per_sample <= 0 || pcm_bytes.empty()) {
		return false;
	}

	if (!file_path.parent_path().empty()) {
		std::error_code error_code;
		std::filesystem::create_directories(file_path.parent_path(), error_code);
	}

	std::ofstream out(file_path, std::ios::binary);
	if (!out) {
		return false;
	}

	uint32_t pcm_data_size = static_cast<uint32_t>(pcm_bytes.size());
	uint32_t riff_chunk_size = 36 + pcm_data_size;

	// 1. RIFF chunk descriptor
	out.write("RIFF", 4);
	out.write(reinterpret_cast<const char*>(&riff_chunk_size), 4);
	out.write("WAVE", 4);

	// 2. fmt subchunk
	out.write("fmt ", 4);
	uint32_t fmt_subchunk_size = 16;
	uint16_t audio_format = 1; // Uncompressed PCM
	uint16_t num_channels = static_cast<uint16_t>(channels);
	uint32_t samples_per_sec = static_cast<uint32_t>(sample_rate);
	uint32_t bytes_per_sec = samples_per_sec * num_channels * (bits_per_sample / 8);
	uint16_t block_align = static_cast<uint16_t>(num_channels * (bits_per_sample / 8));
	uint16_t bits_per_samp = static_cast<uint16_t>(bits_per_sample);

	out.write(reinterpret_cast<const char*>(&fmt_subchunk_size), 4);
	out.write(reinterpret_cast<const char*>(&audio_format), 2);
	out.write(reinterpret_cast<const char*>(&num_channels), 2);
	out.write(reinterpret_cast<const char*>(&samples_per_sec), 4);
	out.write(reinterpret_cast<const char*>(&bytes_per_sec), 4);
	out.write(reinterpret_cast<const char*>(&block_align), 2);
	out.write(reinterpret_cast<const char*>(&bits_per_samp), 2);

	// 3. data subchunk
	out.write("data", 4);
	out.write(reinterpret_cast<const char*>(&pcm_data_size), 4);
	out.write(reinterpret_cast<const char*>(pcm_bytes.data()), pcm_data_size);

	return out.good();
}

bool CompressAudio::tokens_to_wav(const std::vector<uint32_t>& tokens,
				  int sample_rate,
				  int channels,
				  int bits_per_sample,
				  const std::filesystem::path& file_path) {
	std::vector<uint8_t> pcm_bytes = tokens_to_pcm(tokens);
	return pcm_to_wav(pcm_bytes, sample_rate, channels, bits_per_sample, file_path);
}

