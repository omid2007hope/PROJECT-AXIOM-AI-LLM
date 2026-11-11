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

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <zlib.h>

#include "../include/Identifier.hpp"
#include "../include/decompressor/decompress_audio.hpp"
#include "../include/decompressor/decompress_document.hpp"
#include "../include/decompressor/decompress_image.hpp"
#include "../include/decompressor/decompress_video.hpp"
#include "../include/compressor/compress_audio.hpp"
#include "../include/compressor/compress_image.hpp"
#include "../include/parser.hpp"
#include "test_header.hpp"

bool test_decompressor() {
	int pass_count = 0;
	int fail_count = 0;

	std::cout << "\n=== Multimodal Decompressor Test Suite ===\n\n";

	std::filesystem::path temp_dir = test_core_dir() / "test_temp_decom";
	std::filesystem::create_directories(temp_dir);
	std::filesystem::path asset_dir = test_asset_dirs()[0];

	// ─── [1] Document Decompressor (DecompressDocument) ──────────────────────────
	std::cout << "[1] Document Decompressor (DecompressDocument: TXT, MD, PDF, DOCX)\n";

	// 1.1 TXT / MD extraction
	{
		std::filesystem::path txt_file = temp_dir / "sample.txt";
		{
			std::ofstream out(txt_file);
			out << "Silicon native computation with Zero Overhead.";
		}
		std::string txt_content = DecompressDocument::decompress_text(txt_file);
		test_check(txt_content.find("Silicon") != std::string::npos, "decompress_text extracts raw text content", pass_count, fail_count);
		test_check(txt_content.find("Overhead") != std::string::npos, "decompress_text preserves all vocabulary words", pass_count, fail_count);
	}

	// 1.2 PDF text extraction (BT ... ET text operators)
	{
		std::filesystem::path pdf_file = temp_dir / "sample.pdf";
		{
			std::ofstream out(pdf_file, std::ios::binary);
			out << "%PDF-1.4\n"
			    << "1 0 obj << /Type /Catalog /Pages 2 0 R >> endobj\n"
			    << "2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj\n"
			    << "3 0 obj << /Type /Page /Parent 2 0 R >> endobj\n"
			    << "4 0 obj << /Length 50 >> stream\n"
			    << "BT\n"
			    << "/F1 12 Tf\n"
			    << "(Quantum Mechanics) Tj\n"
			    << "(Wavefunction) Tj\n"
			    << "ET\n"
			    << "endstream endobj\n"
			    << "xref\ntrailer << /Root 1 0 R >>\nstartxref\n%%EOF";
		}
		std::string pdf_content = DecompressDocument::decompress_pdf(pdf_file);
		test_check(pdf_content.find("Quantum Mechanics") != std::string::npos, "decompress_pdf extracts text from (text) Tj operators", pass_count, fail_count);
		test_check(pdf_content.find("Wavefunction") != std::string::npos, "decompress_pdf extracts multiple stream tokens", pass_count, fail_count);
	}

	// 1.3 DOCX text extraction (<w:t> XML tags)
	{
		std::filesystem::path docx_file = temp_dir / "sample.docx";
		{
			std::ofstream out(docx_file, std::ios::binary);
			// Simulated DOCX XML chunk
			out << "PK\x03\x04"
			    << "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
			    << "<w:body>"
			    << "<w:p><w:r><w:t>Gravitational Constant</w:t></w:r></w:p>"
			    << "<w:p><w:r><w:t>9.80665</w:t></w:r></w:p>"
			    << "</w:body></w:document>";
		}
		std::string docx_content = DecompressDocument::decompress_docx(docx_file);
		test_check(docx_content.find("Gravitational Constant") != std::string::npos, "decompress_docx extracts text enclosed in <w:t> tags", pass_count, fail_count);
		test_check(docx_content.find("9.80665") != std::string::npos, "decompress_docx extracts numerical and equation tokens", pass_count, fail_count);
	}

	// ─── [2] Image Decompressor (DecompressImage) ───────────────────────────────
	std::cout << "\n[2] Image Decompressor (DecompressImage: BMP, JPG, PNG)\n";

	// 2.1 Synthetic BMP 24-bit generation and bit-for-bit decode
	{
		std::filesystem::path bmp_file = temp_dir / "test_pattern.bmp";
		{
			std::ofstream out(bmp_file, std::ios::binary);
			uint32_t width = 4, height = 4;
			uint32_t row_stride = ((width * 24 + 31) / 32) * 4; // 12 bytes (4 pixels * 3 bytes, 0 pad)
			uint32_t pixel_offset = 54;
			uint32_t file_size = pixel_offset + row_stride * height;

			// File header (14 bytes)
			uint8_t file_hdr[14] = {'B', 'M'};
			*reinterpret_cast<uint32_t*>(&file_hdr[2]) = file_size;
			*reinterpret_cast<uint32_t*>(&file_hdr[10]) = pixel_offset;
			out.write(reinterpret_cast<char*>(file_hdr), 14);

			// Info header (40 bytes)
			uint8_t info_hdr[40] = {0};
			*reinterpret_cast<uint32_t*>(&info_hdr[0]) = 40;
			*reinterpret_cast<int32_t*>(&info_hdr[4]) = width;
			*reinterpret_cast<int32_t*>(&info_hdr[8]) = height;
			*reinterpret_cast<uint16_t*>(&info_hdr[12]) = 1;
			*reinterpret_cast<uint16_t*>(&info_hdr[14]) = 24; // 24-bit RGB
			out.write(reinterpret_cast<char*>(info_hdr), 40);

			// Pixel data: write 4x4 pure red pixels (BGR: 0x00, 0x00, 0xFF)
			std::vector<uint8_t> row(row_stride, 0);
			for (int x = 0; x < 4; ++x) {
				row[x * 3 + 0] = 0x00; // Blue
				row[x * 3 + 1] = 0x00; // Green
				row[x * 3 + 2] = 0xFF; // Red
			}
			for (int y = 0; y < 4; ++y) {
				out.write(reinterpret_cast<char*>(row.data()), row_stride);
			}
		}

		DecodedImage decoded = DecompressImage::decompress_bmp(bmp_file);
		test_check(decoded.is_valid, "decompress_bmp parses valid BMP", pass_count, fail_count);
		test_check_eq(decoded.width, 4, "decompress_bmp width is 4", pass_count, fail_count);
		test_check_eq(decoded.height, 4, "decompress_bmp height is 4", pass_count, fail_count);
		test_check_eq(decoded.channels, 3, "decompress_bmp channels is 3 (RGB)", pass_count, fail_count);
		test_check_eq(static_cast<int>(decoded.pixels[0]), 0xFF, "decompress_bmp pixel 0 is Red (0xFF)", pass_count, fail_count);
		test_check_eq(static_cast<int>(decoded.pixels[1]), 0x00, "decompress_bmp pixel 1 is Green (0x00)", pass_count, fail_count);
		test_check_eq(static_cast<int>(decoded.pixels[2]), 0x00, "decompress_bmp pixel 2 is Blue (0x00)", pass_count, fail_count);
	}

	// 2.2 Live Photo.jpeg asset decode
	{
		std::filesystem::path photo_path = asset_dir / "Photo.jpeg";
		if (std::filesystem::exists(photo_path)) {
			DecodedImage decoded = DecompressImage::decode_image(photo_path);
			test_check(decoded.is_valid, "decode_image successfully decodes Photo.jpeg", pass_count, fail_count);
			test_check(decoded.width > 0 && decoded.height > 0, "Photo.jpeg resolution extracted from SOF marker", pass_count, fail_count);
			test_check(!decoded.pixels.empty(), "Photo.jpeg pixel byte array populated", pass_count, fail_count);
		}
	}

	// ─── [3] Audio Decompressor (DecompressAudio) ───────────────────────────────
	std::cout << "\n[3] Audio Decompressor (DecompressAudio: WAV, OGG, MP3)\n";

	// 3.1 Synthetic WAV generation and PCM extraction
	{
		std::filesystem::path wav_file = temp_dir / "test_tone.wav";
		{
			std::ofstream out(wav_file, std::ios::binary);
			uint32_t pcm_data_size = 128;
			uint32_t riff_size = 36 + pcm_data_size;

			out.write("RIFF", 4);
			out.write(reinterpret_cast<char*>(&riff_size), 4);
			out.write("WAVE", 4);

			out.write("fmt ", 4);
			uint32_t fmt_size = 16;
			uint16_t audio_fmt = 1; // PCM
			uint16_t channels = 2;
			uint32_t sample_rate = 44100;
			uint32_t byte_rate = 44100 * 2 * 2;
			uint16_t block_align = 4;
			uint16_t bits = 16;
			out.write(reinterpret_cast<char*>(&fmt_size), 4);
			out.write(reinterpret_cast<char*>(&audio_fmt), 2);
			out.write(reinterpret_cast<char*>(&channels), 2);
			out.write(reinterpret_cast<char*>(&sample_rate), 4);
			out.write(reinterpret_cast<char*>(&byte_rate), 4);
			out.write(reinterpret_cast<char*>(&block_align), 2);
			out.write(reinterpret_cast<char*>(&bits), 2);

			out.write("data", 4);
			out.write(reinterpret_cast<char*>(&pcm_data_size), 4);
			std::vector<uint8_t> dummy_samples(pcm_data_size, 0x7F);
			out.write(reinterpret_cast<char*>(dummy_samples.data()), pcm_data_size);
		}

		DecodedAudio decoded = DecompressAudio::decompress_wav(wav_file);
		test_check(decoded.is_valid, "decompress_wav parses valid RIFF/WAVE container", pass_count, fail_count);
		test_check_eq(decoded.sample_rate, 44100, "decompress_wav sample rate is 44100 Hz", pass_count, fail_count);
		test_check_eq(decoded.channels, 2, "decompress_wav channels is 2 (Stereo)", pass_count, fail_count);
		test_check_eq(decoded.bits_per_sample, 16, "decompress_wav bits per sample is 16-bit", pass_count, fail_count);
		test_check_eq(decoded.pcm_bytes.size(), static_cast<size_t>(128), "decompress_wav extracted exact 128 raw PCM bytes", pass_count, fail_count);
	}

	// 3.2 Live Voice.ogg asset decode
	{
		std::filesystem::path voice_path = asset_dir / "Voice.ogg";
		if (std::filesystem::exists(voice_path)) {
			DecodedAudio decoded = DecompressAudio::decode_audio(voice_path);
			test_check(decoded.is_valid, "decode_audio successfully decodes Voice.ogg", pass_count, fail_count);
			test_check(decoded.sample_rate > 0, "Voice.ogg sample rate extracted", pass_count, fail_count);
			test_check(!decoded.pcm_bytes.empty(), "Voice.ogg linear PCM samples populated", pass_count, fail_count);
		}
	}

	// ─── [4] Video Decompressor (DecompressVideo) ───────────────────────────────
	std::cout << "\n[4] Video Decompressor (DecompressVideo: MP4, MKV)\n";

	// 4.1 Live Video.mp4 asset decode
	{
		std::filesystem::path video_path = asset_dir / "Video.mp4";
		if (std::filesystem::exists(video_path)) {
			DecodedVideo decoded = DecompressVideo::decode_video(video_path);
			test_check(decoded.is_valid, "decode_video successfully parses Video.mp4", pass_count, fail_count);
			test_check(decoded.width > 0 && decoded.height > 0, "Video.mp4 resolution extracted from MP4 atoms", pass_count, fail_count);
			test_check(!decoded.raw_frames.empty(), "Video.mp4 frame bytes extracted from mdat box", pass_count, fail_count);
		}
	}

	// ─── [5] Multimodal Output Codecs (CompressImage & CompressAudio) ────────────
	std::cout << "\n[5] Multimodal Output Codecs (CompressImage & CompressAudio)\n";

	// 5.1 Image Codec: tokens -> BMP -> DecompressImage bit-accurate roundtrip
	{
		int width = 8;
		int height = 8;
		std::vector<uint8_t> original_pixels(width * height * 3);
		for (size_t i = 0; i < original_pixels.size(); ++i) {
			original_pixels[i] = static_cast<uint8_t>((i * 17 + 31) % 256);
		}

		// Convert RGB bytes to discrete visual tokens [64,512..64,767]
		std::vector<uint32_t> visual_tokens = CompressImage::pixels_to_tokens(original_pixels);
		test_check_eq(visual_tokens.size(), original_pixels.size(), "pixels_to_tokens preserves element count", pass_count, fail_count);

		bool all_tokens_in_vocab = true;
		for (uint32_t tok : visual_tokens) {
			if (tok < vocab::VISUAL_START_ID || tok > vocab::VISUAL_END_ID) {
				all_tokens_in_vocab = false;
				break;
			}
		}
		test_check(all_tokens_in_vocab, "All visual tokens reside in designated visual range [64512..64767]", pass_count, fail_count);

		std::filesystem::path output_bmp = temp_dir / "encoded_output.bmp";
		bool encode_ok = CompressImage::tokens_to_bmp(visual_tokens, width, height, output_bmp);
		test_check(encode_ok, "tokens_to_bmp encodes valid 24-bit uncompressed BMP container", pass_count, fail_count);

		// Read back using DecompressImage and assert 100% bit-accurate pixel match
		DecodedImage decoded = DecompressImage::decompress_bmp(output_bmp);
		test_check(decoded.is_valid, "decompress_bmp successfully decodes serialized BMP container", pass_count, fail_count);
		test_check_eq(decoded.width, width, "Roundtrip image width matches original 8px", pass_count, fail_count);
		test_check_eq(decoded.height, height, "Roundtrip image height matches original 8px", pass_count, fail_count);
		test_check_eq(decoded.channels, 3, "Roundtrip image has 3 RGB channels", pass_count, fail_count);

		bool pixels_match = (decoded.pixels == original_pixels);
		test_check(pixels_match, "Gate 6: Visual token roundtrip achieves 100% bit-accurate pixel reconstruction", pass_count, fail_count);
	}

	// 5.2 Acoustic Codec: tokens -> WAV -> DecompressAudio bit-accurate roundtrip
	{
		int sample_rate = 44100;
		int channels = 2;
		int bits_per_sample = 16;
		size_t sample_count = 256;
		std::vector<uint8_t> original_pcm(sample_count);
		for (size_t i = 0; i < sample_count; ++i) {
			original_pcm[i] = static_cast<uint8_t>((i * 13 + 7) % 256);
		}

		// Convert linear PCM bytes to discrete acoustic tokens [64,768..65,023]
		std::vector<uint32_t> acoustic_tokens = CompressAudio::pcm_to_tokens(original_pcm);
		test_check_eq(acoustic_tokens.size(), original_pcm.size(), "pcm_to_tokens preserves element count", pass_count, fail_count);

		bool all_tokens_in_vocab = true;
		for (uint32_t tok : acoustic_tokens) {
			if (tok < vocab::ACOUSTIC_START_ID || tok > vocab::ACOUSTIC_END_ID) {
				all_tokens_in_vocab = false;
				break;
			}
		}
		test_check(all_tokens_in_vocab, "All acoustic tokens reside in designated acoustic range [64768..65023]", pass_count, fail_count);

		std::filesystem::path output_wav = temp_dir / "encoded_output.wav";
		bool encode_ok = CompressAudio::tokens_to_wav(acoustic_tokens, sample_rate, channels, bits_per_sample, output_wav);
		test_check(encode_ok, "tokens_to_wav encodes valid RIFF/WAVE audio container", pass_count, fail_count);

		// Read back using DecompressAudio and assert 100% bit-accurate PCM sample match
		DecodedAudio decoded = DecompressAudio::decompress_wav(output_wav);
		test_check(decoded.is_valid, "decompress_wav successfully decodes serialized WAV container", pass_count, fail_count);
		test_check_eq(decoded.sample_rate, sample_rate, "Roundtrip audio sample rate matches 44100 Hz", pass_count, fail_count);
		test_check_eq(decoded.channels, channels, "Roundtrip audio channels match Stereo (2)", pass_count, fail_count);
		test_check_eq(decoded.bits_per_sample, bits_per_sample, "Roundtrip audio bits per sample matches 16-bit", pass_count, fail_count);

		bool samples_match = (decoded.pcm_bytes == original_pcm);
		test_check(samples_match, "Gate 6: Acoustic token roundtrip achieves 100% bit-accurate PCM reconstruction", pass_count, fail_count);
	}

	// ─── [6] Hardening: noise rejection, malformed headers, identifier false positives ──
	std::cout << "\n[6] Hardening (scanned PDFs, binary-as-text, hostile headers)\n";

	auto write_file = [&](const std::filesystem::path& path, const std::string& bytes) {
		std::ofstream out(path, std::ios::binary);
		out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
	};
	auto deflate_bytes = [](const std::string& plain) {
		uLongf compressed_size = compressBound(static_cast<uLong>(plain.size()));
		std::string compressed(compressed_size, '\0');
		compress(reinterpret_cast<Bytef*>(compressed.data()), &compressed_size,
			 reinterpret_cast<const Bytef*>(plain.data()), static_cast<uLong>(plain.size()));
		compressed.resize(compressed_size);
		return compressed;
	};
	auto pdf_with_stream = [](const std::string& dictionary, const std::string& payload) {
		return "%PDF-1.4\n4 0 obj << " + dictionary + " /Length " + std::to_string(payload.size()) +
		       " >> stream\n" + payload + "\nendstream endobj\n%%EOF";
	};

	// 6.1 Scanned PDF: an image stream must contribute nothing, even if it contains BT..ET text
	{
		std::string image_payload = "BT (Declassified fake words inside jpeg bytes) Tj ET ";
		for (int i = 0; i < 4096; ++i) {
			image_payload += static_cast<char>((i * 131 + 7) % 256);
		}
		write_file(temp_dir / "scanned.pdf",
			   pdf_with_stream("/Type /XObject /Subtype /Image /Filter /DCTDecode", image_payload));
		std::string scanned_text = DecompressDocument::decompress_pdf(temp_dir / "scanned.pdf");
		test_check(scanned_text.empty(), "decompress_pdf returns nothing for DCTDecode image streams", pass_count, fail_count);

		write_file(temp_dir / "binary_only.pdf",
			   pdf_with_stream("/Filter /JBIG2Decode", image_payload));
		test_check(DecompressDocument::decompress_pdf(temp_dir / "binary_only.pdf").empty(), "decompress_pdf returns nothing for JBIG2Decode streams", pass_count, fail_count);
	}

	// 6.2 FlateDecode content stream: inflated, kerned TJ glyph runs are joined into words
	{
		std::string content = "BT\n/F1 12 Tf\n[(Hel)-10(lo) -400 (Wor)5(ld)] TJ\n(it\\222s plain text) Tj\nET\n";
		write_file(temp_dir / "flate.pdf",
			   pdf_with_stream("/Filter /FlateDecode", deflate_bytes(content)));
		std::string flate_text = DecompressDocument::decompress_pdf(temp_dir / "flate.pdf");
		test_check(flate_text.find("Hello World") != std::string::npos, "decompress_pdf joins kerned TJ glyph runs into words", pass_count, fail_count);
		test_check(flate_text.find("it's plain text") != std::string::npos, "decompress_pdf decodes octal escapes to ASCII punctuation", pass_count, fail_count);
	}

	// 6.3 Mis-encoded subset-font stream (shifted glyph codes) is dropped whole
	{
		std::string shifted_letters;
		for (int i = 0; i < 40; ++i) {
			shifted_letters += "jkqxzvwbpmjkqxzvwbpm ";
		}
		std::string content = "BT\n(" + shifted_letters + ") Tj\nET\n";
		write_file(temp_dir / "shifted.pdf",
			   pdf_with_stream("/Filter /FlateDecode", deflate_bytes(content)));
		test_check(DecompressDocument::decompress_pdf(temp_dir / "shifted.pdf").empty(), "decompress_pdf drops streams with non-linguistic letter statistics", pass_count, fail_count);
	}

	// 6.4 Binary renamed to .txt never reaches the text tokenizer
	{
		std::string noise("\x00\x01\x02\x03", 4);
		write_file(temp_dir / "fake.txt", noise);
		test_check(DecompressDocument::decompress_text(temp_dir / "fake.txt").empty(), "decompress_text rejects binary content with a .txt extension", pass_count, fail_count);
		test_check(DecompressDocument::extract_text(temp_dir / "fake.txt").empty(), "extract_text rejects binary content with a .txt extension", pass_count, fail_count);
		test_check(Identifier::identify(temp_dir / "fake.txt").format == FileFormat::UNKNOWN_BINARY, "Identifier labels binary .txt as UNKNOWN_BINARY", pass_count, fail_count);

		write_file(temp_dir / "fake.csv", noise);
		test_check(Identifier::identify(temp_dir / "fake.csv").modality == ModalityCategory::UNKNOWN, "Identifier never labels binary .csv as a text document", pass_count, fail_count);
	}

	// 6.5 Identifier false positives
	{
		write_file(temp_dir / "bmw.txt", "BMW car review: the engine is smooth and quiet.");
		test_check(Identifier::identify(temp_dir / "bmw.txt").format == FileFormat::TXT, "Text starting with 'BM' is not mistaken for BMP", pass_count, fail_count);

		write_file(temp_dir / "junk_before.pdf", "\n\n%PDF-1.4\n1 0 obj << >> endobj\n");
		test_check(Identifier::identify(temp_dir / "junk_before.pdf").format == FileFormat::PDF, "PDF header preceded by whitespace is still identified", pass_count, fail_count);

		std::string fake_mpeg("\xFF\xE0\x00\x00 random bytes", 20);
		write_file(temp_dir / "not_audio.bin", fake_mpeg);
		test_check(Identifier::identify(temp_dir / "not_audio.bin").format != FileFormat::MP3, "Reserved MPEG layer bits are not identified as MP3", pass_count, fail_count);
	}

	// 6.6 Hostile image headers
	{
		// BMP claiming 30000 x 30000 pixels in a 54-byte file must not allocate gigabytes
		std::string bomb(54, '\0');
		bomb[0] = 'B';
		bomb[1] = 'M';
		bomb[10] = 54;
		bomb[14] = 40;
		const int32_t huge = 30000;
		std::memcpy(&bomb[18], &huge, 4);
		std::memcpy(&bomb[22], &huge, 4);
		bomb[26] = 1;
		bomb[28] = 24;
		write_file(temp_dir / "bomb.bmp", bomb);
		test_check(!DecompressImage::decompress_bmp(temp_dir / "bomb.bmp").is_valid, "decompress_bmp rejects pixel arrays larger than the file", pass_count, fail_count);

		// RLE-compressed BMP would be misread as raw pixels
		std::string encoded_bitmap = bomb;
		const int32_t small = 4;
		std::memcpy(&encoded_bitmap[18], &small, 4);
		std::memcpy(&encoded_bitmap[22], &small, 4);
		encoded_bitmap[30] = 1;
		encoded_bitmap.append(64, '\x55');
		write_file(temp_dir / "rle.bmp", encoded_bitmap);
		test_check(!DecompressImage::decompress_bmp(temp_dir / "rle.bmp").is_valid, "decompress_bmp rejects compressed (RLE) pixel data", pass_count, fail_count);

		write_file(temp_dir / "no_sof.jpg", std::string("\xFF\xD8\xFF\xE0\x00\x10JFIF\x00 no frame marker here", 32));
		test_check(!DecompressImage::decompress_jpeg(temp_dir / "no_sof.jpg").is_valid, "decompress_jpeg rejects JPEG without a frame header", pass_count, fail_count);

		write_file(temp_dir / "bad_ihdr.png", std::string("\x89PNG\r\n\x1A\n\x00\x00\x00\x0DXXXX", 16) + std::string(20, '\0'));
		test_check(!DecompressImage::decompress_png(temp_dir / "bad_ihdr.png").is_valid, "decompress_png rejects PNG without IHDR", pass_count, fail_count);
	}

	// 6.7 WAV chunk parsing: odd-sized chunks are padded, bogus formats rejected
	{
		auto little_endian_16 = [](uint16_t value) { return std::string(reinterpret_cast<char*>(&value), 2); };
		auto little_endian_32 = [](uint32_t value) { return std::string(reinterpret_cast<char*>(&value), 4); };
		std::string pcm(16, '\x42');
		std::string format_body = little_endian_16(1) + little_endian_16(2) + little_endian_32(44100) + little_endian_32(176400) + little_endian_16(4) + little_endian_16(16);
		std::string wav = "RIFF" + little_endian_32(4 + 8 + 4 + 8 + 16 + 8 + 16) + "WAVE" + "LIST" + little_endian_32(3) +
				  "abc" + std::string(1, '\0') + "fmt " + little_endian_32(16) + format_body + "data" +
				  little_endian_32(16) + pcm;
		write_file(temp_dir / "odd_chunk.wav", wav);
		DecodedAudio odd = DecompressAudio::decompress_wav(temp_dir / "odd_chunk.wav");
		test_check(odd.is_valid && odd.pcm_bytes == std::vector<uint8_t>(pcm.begin(), pcm.end()), "decompress_wav honours RIFF even-byte chunk padding", pass_count, fail_count);

		std::string invalid_format = little_endian_16(1) + little_endian_16(0) + little_endian_32(44100) + little_endian_32(176400) + little_endian_16(4) + little_endian_16(16);
		std::string bad_wav = "RIFF" + little_endian_32(36 + 16) + "WAVE" + "fmt " + little_endian_32(16) + invalid_format +
				      "data" + little_endian_32(16) + pcm;
		write_file(temp_dir / "zero_channels.wav", bad_wav);
		test_check(!DecompressAudio::decompress_wav(temp_dir / "zero_channels.wav").is_valid, "decompress_wav rejects zero-channel format chunks", pass_count, fail_count);
	}

	// 6.8 MP4 track header: width/height come from the right tkhd offsets
	{
		auto big_endian_32 = [](uint32_t value) {
			std::string bytes(4, '\0');
			bytes[0] = static_cast<char>(value >> 24);
			bytes[1] = static_cast<char>(value >> 16);
			bytes[2] = static_cast<char>(value >> 8);
			bytes[3] = static_cast<char>(value);
			return bytes;
		};
		std::string track_header(92, '\0');
		track_header.replace(0, 4, big_endian_32(92));
		track_header.replace(4, 4, "tkhd");
		track_header.replace(84, 4, big_endian_32(1280u << 16));
		track_header.replace(88, 4, big_endian_32(720u << 16));
		std::string track_box = big_endian_32(8 + 92) + "trak" + track_header;
		std::string movie_box = big_endian_32(8 + static_cast<uint32_t>(track_box.size())) + "moov" + track_box;
		std::string file_type = big_endian_32(16) + "ftyp" + "isom" + big_endian_32(0);
		std::string media_data = big_endian_32(8 + 32) + "mdat" + std::string(32, '\x11');
		write_file(temp_dir / "dims.mp4", file_type + movie_box + media_data);
		DecodedVideo video = DecompressVideo::decompress_mp4(temp_dir / "dims.mp4");
		test_check(video.is_valid && video.width == 1280 && video.height == 720, "decompress_mp4 reads width/height from the tkhd 16.16 fields", pass_count, fail_count);
	}

	// Clean up temp directory
	std::error_code ec;
	std::filesystem::remove_all(temp_dir, ec);

	std::cout << "=== Multimodal Decompressor Test Suite: " << pass_count << " passed, " << fail_count << " failed ===\n";
	return (fail_count == 0);
}
