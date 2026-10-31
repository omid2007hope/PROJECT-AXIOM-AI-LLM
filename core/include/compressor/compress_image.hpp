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

