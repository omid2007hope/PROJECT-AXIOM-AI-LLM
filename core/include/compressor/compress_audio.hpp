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

