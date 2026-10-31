#include "../../include/compressor/compress_image.hpp"

#include <cmath>
#include <fstream>
#include <iostream>

#include "../../include/parser.hpp"

std::vector<uint8_t> CompressImage::tokens_to_pixels(const std::vector<uint32_t>& tokens) {
	std::vector<uint8_t> pixels;
	pixels.reserve(tokens.size());

	for (uint32_t tok_id : tokens) {
		if (tok_id >= vocab::VISUAL_START_ID && tok_id <= vocab::VISUAL_END_ID) {
			pixels.push_back(static_cast<uint8_t>(tok_id - vocab::VISUAL_START_ID));
		} else {
			pixels.push_back(0);
		}
	}

	return pixels;
}

std::vector<uint32_t> CompressImage::pixels_to_tokens(const std::vector<uint8_t>& pixels) {
	std::vector<uint32_t> tokens;
	tokens.reserve(pixels.size());

	for (uint8_t b : pixels) {
		tokens.push_back(vocab::VISUAL_START_ID + static_cast<uint32_t>(b));
	}

	return tokens;
}

bool CompressImage::pixels_to_bmp(const std::vector<uint8_t>& pixels,
				  int width,
				  int height,
				  const std::filesystem::path& file_path) {
	if (width <= 0 || height <= 0 || pixels.size() < static_cast<size_t>(width * height * 3)) {
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

	// 24-bit BMP requires rows to be padded to multiples of 4 bytes
	size_t row_stride = ((static_cast<size_t>(width) * 24 + 31) / 32) * 4;
	size_t pad_bytes = row_stride - (static_cast<size_t>(width) * 3);
	uint32_t image_size = static_cast<uint32_t>(row_stride * height);
	uint32_t pixel_offset = 54;
	uint32_t file_size = pixel_offset + image_size;

	// File Header (14 bytes)
	uint8_t file_header[14] = {'B', 'M', 0};
	*reinterpret_cast<uint32_t*>(&file_header[2]) = file_size;
	*reinterpret_cast<uint32_t*>(&file_header[10]) = pixel_offset;
	out.write(reinterpret_cast<const char*>(file_header), 14);

	// DIB Info Header (40 bytes, BITMAPINFOHEADER)
	uint8_t info_header[40] = {0};
	*reinterpret_cast<uint32_t*>(&info_header[0]) = 40;
	*reinterpret_cast<int32_t*>(&info_header[4]) = width;
	*reinterpret_cast<int32_t*>(&info_header[8]) = height; // Positive = bottom-up
	*reinterpret_cast<uint16_t*>(&info_header[12]) = 1;     // Planes
	*reinterpret_cast<uint16_t*>(&info_header[14]) = 24;    // 24-bit RGB
	*reinterpret_cast<uint32_t*>(&info_header[16]) = 0;     // BI_RGB uncompressed
	*reinterpret_cast<uint32_t*>(&info_header[20]) = image_size;
	*reinterpret_cast<int32_t*>(&info_header[24]) = 2835;   // 72 DPI
	*reinterpret_cast<int32_t*>(&info_header[28]) = 2835;
	out.write(reinterpret_cast<const char*>(info_header), 40);

	// BMP stores rows bottom-up in BGR pixel order
	std::vector<uint8_t> row_buffer(row_stride, 0);

	for (int y = height - 1; y >= 0; --y) {
		size_t in_row_start = static_cast<size_t>(y) * width * 3;
		for (int x = 0; x < width; ++x) {
			uint8_t r = pixels[in_row_start + x * 3 + 0];
			uint8_t g = pixels[in_row_start + x * 3 + 1];
			uint8_t b = pixels[in_row_start + x * 3 + 2];

			row_buffer[x * 3 + 0] = b; // Blue
			row_buffer[x * 3 + 1] = g; // Green
			row_buffer[x * 3 + 2] = r; // Red
		}
		// Zero-fill padding
		for (size_t p = 0; p < pad_bytes; ++p) {
			row_buffer[width * 3 + p] = 0;
		}
		out.write(reinterpret_cast<const char*>(row_buffer.data()), row_stride);
	}

	return out.good();
}

bool CompressImage::tokens_to_bmp(const std::vector<uint32_t>& tokens,
				  int width,
				  int height,
				  const std::filesystem::path& file_path) {
	std::vector<uint8_t> pixels = tokens_to_pixels(tokens);
	return pixels_to_bmp(pixels, width, height, file_path);
}

