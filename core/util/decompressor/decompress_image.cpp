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

#include "decompressor/decompress_image.hpp"
#include "Identifier.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

DecodedImage DecompressImage::decompress_bmp(const std::filesystem::path& file_path) {
	DecodedImage img;
	std::ifstream in(file_path, std::ios::binary);
	if (!in) {
		return img;
	}

	uint8_t file_header[14];
	in.read(reinterpret_cast<char*>(file_header), 14);
	if (in.gcount() < 14 || file_header[0] != 'B' || file_header[1] != 'M') {
		return img;
	}

	uint32_t pixel_offset = *reinterpret_cast<uint32_t*>(&file_header[10]);

	uint8_t info_header[40];
	in.read(reinterpret_cast<char*>(info_header), 40);
	if (in.gcount() < 40) {
		return img;
	}

	uint32_t dib_size = *reinterpret_cast<uint32_t*>(&info_header[0]);
	int32_t width = *reinterpret_cast<int32_t*>(&info_header[4]);
	int32_t height = *reinterpret_cast<int32_t*>(&info_header[8]);
	uint16_t bpp = *reinterpret_cast<uint16_t*>(&info_header[14]);
	uint32_t compression = *reinterpret_cast<uint32_t*>(&info_header[16]);

	// Only uncompressed BITMAPINFOHEADER-or-later pixel data is decoded: RLE/bitfield
	// payloads or OS/2 headers would be misread as pixels
	if (dib_size < 40 || compression != 0 || width <= 0 || width > 32768 || height == 0 ||
	    height == INT32_MIN || (bpp != 24 && bpp != 32 && bpp != 8)) {
		return img;
	}

	bool top_down = (height < 0);
	height = (height < 0) ? -height : height;
	if (height > 32768) {
		return img;
	}

	// The pixel array must fit inside the file, otherwise the header is lying about the
	// size and would force a huge allocation
	const size_t row_stride = ((static_cast<size_t>(width) * bpp + 31) / 32) * 4;
	const std::uintmax_t file_size = std::filesystem::file_size(file_path);
	if (pixel_offset < 54 || pixel_offset > file_size ||
	    row_stride * static_cast<size_t>(height) > file_size - pixel_offset) {
		return img;
	}

	img.width = width;
	img.height = height;
	img.channels = 3;

	in.seekg(pixel_offset);
	if (!in) {
		return img;
	}

	std::vector<uint8_t> row_buffer(row_stride);
	img.pixels.resize(static_cast<size_t>(width) * height * 3);

	for (int y = 0; y < height; ++y) {
		in.read(reinterpret_cast<char*>(row_buffer.data()), row_stride);
		if (in.gcount() < static_cast<std::streamsize>(row_stride)) {
			break;
		}

		int target_y = top_down ? y : (height - 1 - y);
		size_t out_row_start = static_cast<size_t>(target_y) * width * 3;

		if (bpp == 24) {
			for (int x = 0; x < width; ++x) {
				uint8_t b = row_buffer[x * 3];
				uint8_t g = row_buffer[x * 3 + 1];
				uint8_t r = row_buffer[x * 3 + 2];
				img.pixels[out_row_start + x * 3] = r;
				img.pixels[out_row_start + x * 3 + 1] = g;
				img.pixels[out_row_start + x * 3 + 2] = b;
			}
		} else if (bpp == 32) {
			for (int x = 0; x < width; ++x) {
				uint8_t b = row_buffer[x * 4];
				uint8_t g = row_buffer[x * 4 + 1];
				uint8_t r = row_buffer[x * 4 + 2];
				img.pixels[out_row_start + x * 3] = r;
				img.pixels[out_row_start + x * 3 + 1] = g;
				img.pixels[out_row_start + x * 3 + 2] = b;
			}
		} else if (bpp == 8) {
			for (int x = 0; x < width; ++x) {
				uint8_t gray = row_buffer[x];
				img.pixels[out_row_start + x * 3] = gray;
				img.pixels[out_row_start + x * 3 + 1] = gray;
				img.pixels[out_row_start + x * 3 + 2] = gray;
			}
		}
	}

	img.is_valid = true;
	return img;
}

DecodedImage DecompressImage::decompress_jpeg(const std::filesystem::path& file_path) {
	DecodedImage img;
	std::ifstream in(file_path, std::ios::binary);
	if (!in) {
		return img;
	}

	std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)),
				    std::istreambuf_iterator<char>());
	if (buffer.size() < 4 || buffer[0] != 0xFF || buffer[1] != 0xD8) {
		return img;
	}

	img.channels = 3;

	// Parse JPEG markers to find SOF0/SOF2 (Start Of Frame) for exact resolution
	size_t pos = 2;
	const size_t len = buffer.size();

	while (pos + 4 < len) {
		if (buffer[pos] != 0xFF) {
			pos++;
			continue;
		}

		uint8_t marker = buffer[pos + 1];
		pos += 2;

		// Skip fill bytes and standalone markers without length payload
		if (marker == 0xFF || marker == 0x00 || marker == 0x01 ||
		    (marker >= 0xD0 && marker <= 0xD8)) {
			continue;
		}

		// End of Image
		if (marker == 0xD9) {
			break;
		}

		// SOF0 (Baseline) or SOF2 (Progressive)
		if (marker == 0xC0 || marker == 0xC2) {
			if (pos + 6 < len) {
				pos += 2; // skip length
				pos += 1; // skip precision
				img.height = (buffer[pos] << 8) | buffer[pos + 1];
				img.width = (buffer[pos + 2] << 8) | buffer[pos + 3];
				break;
			}
		}

		// Variable length marker
		if (pos + 2 <= len) {
			uint16_t block_len = (buffer[pos] << 8) | buffer[pos + 1];
			if (block_len < 2) {
				break;
			}
			pos += block_len;
		} else {
			break;
		}
	}

	// A JPEG without a Start-Of-Frame marker has no image dimensions and is not decodable
	if (img.width <= 0 || img.height <= 0) {
		return DecodedImage {};
	}

	// Store raw image scan data (pixels / quantized DCT coefficients)
	img.pixels = std::move(buffer);
	img.is_valid = true;
	return img;
}

DecodedImage DecompressImage::decompress_png(const std::filesystem::path& file_path) {
	DecodedImage img;
	std::ifstream in(file_path, std::ios::binary);
	if (!in) {
		return img;
	}

	std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)),
				    std::istreambuf_iterator<char>());
	if (buffer.size() < 24) {
		return img;
	}

	// Verify PNG signature
	const uint8_t png_sig[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
	if (std::memcmp(buffer.data(), png_sig, 8) != 0) {
		return img;
	}

	// Parse IHDR chunk (always at offset 8): a valid PNG starts with a 13-byte IHDR
	const uint32_t ihdr_length = (static_cast<uint32_t>(buffer[8]) << 24) | (buffer[9] << 16) |
				     (buffer[10] << 8) | buffer[11];
	if (ihdr_length != 13 || buffer[12] != 'I' || buffer[13] != 'H' || buffer[14] != 'D' ||
	    buffer[15] != 'R') {
		return img;
	}
	const uint32_t png_width = (static_cast<uint32_t>(buffer[16]) << 24) | (buffer[17] << 16) |
				   (buffer[18] << 8) | buffer[19];
	const uint32_t png_height = (static_cast<uint32_t>(buffer[20]) << 24) | (buffer[21] << 16) |
				    (buffer[22] << 8) | buffer[23];
	if (png_width == 0 || png_height == 0 || png_width > 0x7FFFFFFFu || png_height > 0x7FFFFFFFu) {
		return img;
	}
	img.width = static_cast<int>(png_width);
	img.height = static_cast<int>(png_height);
	img.channels = 3;

	img.pixels = std::move(buffer);
	img.is_valid = true;
	return img;
}

DecodedImage DecompressImage::decompress_webp(const std::filesystem::path& file_path) {
	DecodedImage img;
	std::ifstream in(file_path, std::ios::binary);
	if (!in) {
		return img;
	}

	std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)),
				    std::istreambuf_iterator<char>());
	if (buffer.size() < 30 || std::memcmp(buffer.data(), "RIFF", 4) != 0 ||
	    std::memcmp(buffer.data() + 8, "WEBP", 4) != 0) {
		return img;
	}

	img.channels = 3;
	img.pixels = std::move(buffer);
	img.is_valid = true;
	return img;
}

DecodedImage DecompressImage::decode_image(const std::filesystem::path& file_path) {
	IdentifiedFile info = Identifier::identify(file_path);
	switch (info.format) {
	case FileFormat::BMP:
		return decompress_bmp(file_path);
	case FileFormat::PNG:
		return decompress_png(file_path);
	case FileFormat::WEBP:
		return decompress_webp(file_path);
	case FileFormat::JPEG:
	default:
		return decompress_jpeg(file_path);
	}
}
