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

/*
 * Project Axiom — Unified Layout-Aware Corpus & Vision Extractor (C11)
 *
 * Build (Windows MSVC x64 against PyMuPDF's mupdfcpp64):
 *   call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
 *   cl /nologo /W3 /O2 /std:c11 /utf-8 /I"%PYMUPDF_DIR%\mupdf-devel\include" gen_corpus.c ^
 *      /Fe:gen_corpus.exe /link "%PYMUPDF_DIR%\mupdf-devel\lib\mupdfcpp64.lib" ^
 *      normaliz.lib ole32.lib shell32.lib windowscodecs.lib
 *
 * Build (POSIX GCC/Clang against libmupdf):
 *   cc -std=c11 -O2 -Wall -Wextra gen_corpus.c -lmupdf -lmupdfthird -lm -o gen_corpus
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <shellapi.h>
#include <wincodec.h>
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mupdf/fitz.h"
#include "mupdf/pdf.h"

/* ========================================================================= */
/* Constants & Configuration                                                 */
/* ========================================================================= */

#define MIN_IMAGE_SIZE 100
#define MIN_IMAGE_AREA 25000
#define MAX_ASPECT_RATIO 8.0
#define MIN_IMAGE_ENTROPY 1.5
#define MIN_IMAGE_DEVIATION 12.0
#define MAX_IMAGE_EDGE 1536
#define DOCUMENT_LOGO_PAGES 3
#define CORPUS_LOGO_DOCUMENTS 5
#define VECTOR_DPI 200
#define MINHASH_SIZE 64
#define MINHASH_BANDS 16
#define MINHASH_ROWS 4
#define MINHASH_PRIME ((1ULL << 61) - 1ULL)
#define NEAR_DUPLICATE_THRESHOLD 0.80

static const fz_color_params DEFAULT_COLOR_PARAMS = {
	FZ_RI_RELATIVE_COLORIMETRIC, 1, 0, 0
};

typedef enum {
	BLOCK_HEADING = 0,
	BLOCK_PROSE = 1,
	BLOCK_CODE = 2,
	BLOCK_MATH = 3,
	BLOCK_TABLE = 4,
	BLOCK_IMAGE = 5
} BlockKind;

typedef enum {
	PNG_SAVED = 0,
	PNG_REJECTED = 1,
	PNG_DECODE_ERROR = 2
} SavePngStatus;

typedef struct {
	uint32_t codepoint;
	const char *replacement;
} LigatureEntry;

static const LigatureEntry LIGATURE_MAP[] = {
	{0xFB00, "ff"},  {0xFB01, "fi"},  {0xFB02, "fl"},  {0xFB03, "ffi"},
	{0xFB04, "ffl"}, {0xFB05, "ft"},  {0xFB06, "st"},  {0x2018, "'"},
	{0x2019, "'"},   {0x201A, "'"},   {0x201B, "'"},   {0x201C, "\""},
	{0x201D, "\""},  {0x201E, "\""},  {0x201F, "\""},  {0x2013, "-"},
	{0x2014, "-"},   {0x2015, "-"},   {0x00A0, " "},   {0x2026, "..."}
};

static const char *const MONOSPACE_HINTS[] = {
	"courier", "consolas", "mono",    "inconsolata", "menlo",  "fira",
	"sourcecode", "fixed", "cmtt",    "lmtt",        "sfmono", "jetbrains",
	"hack"
};

static const char *const KEEP_HYPHEN_PREFIXES[] = {
	"anti",  "auto",   "bi",     "co",     "cross", "e",     "ex",    "full",
	"half",  "high",   "in",     "inter",  "intra", "low",   "meta",  "micro",
	"mid",   "mini",   "multi",  "near",   "non",   "off",   "on",    "out",
	"over",  "post",   "pre",    "pro",    "pseudo","quasi", "re",    "real",
	"self",  "semi",   "short",  "single", "sub",   "super", "time",  "two",
	"ultra", "under",  "up",     "well",   "zero"
};

/* Sorted codepoints of MATH_OPERATORS from gen_corpus.py */
static const uint32_t MATH_OPERATORS[] = {
	0x003C, 0x003D, 0x003E, 0x005E, 0x005F, 0x00B1, 0x00B2, 0x00B3, 0x00B9, 0x00D7,
	0x00F7, 0x0394, 0x0398, 0x039B, 0x039E, 0x03A0, 0x03A3, 0x03A6, 0x03A8, 0x03A9,
	0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7, 0x03B8, 0x03B9, 0x03BA,
	0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03C0, 0x03C1, 0x03C3, 0x03C4, 0x03C5, 0x03C6,
	0x03C7, 0x03C8, 0x03C9, 0x2070, 0x2074, 0x2075, 0x2076, 0x2077, 0x2078, 0x2079,
	0x207A, 0x207B, 0x2190, 0x2192, 0x2194, 0x21D2, 0x21D4, 0x2200, 0x2202, 0x2203,
	0x2207, 0x2208, 0x2209, 0x220F, 0x2211, 0x221A, 0x221E, 0x2229, 0x222A, 0x222B,
	0x2248, 0x2260, 0x2264, 0x2265, 0x2282, 0x2283
};

static const char *const TEXT_EXTENSIONS[] = {".txt", ".text", ".md", ".markdown"};
static const char *const IMAGE_EXTENSIONS[] = {
	".png", ".jpg", ".jpeg", ".webp", ".bmp", ".tif", ".tiff", ".gif"
};

static uint64_t MINHASH_COEFF_A[MINHASH_SIZE];
static uint64_t MINHASH_COEFF_B[MINHASH_SIZE];

static void init_minhash_tables(void) {
	for (int index = 0; index < MINHASH_SIZE; ++index) {
		uint64_t multiplier = (uint64_t)(index + 1);
		MINHASH_COEFF_A[index] =
			((multiplier * 2654435761ULL + 1013904223ULL) % (MINHASH_PRIME - 1ULL)) + 1ULL;
		MINHASH_COEFF_B[index] =
			(multiplier * 1597334677ULL + 3812015801ULL) % MINHASH_PRIME;
	}
}

/* ========================================================================= */
/* Section 1: Safe Memory, String Buffer, UTF-8 & Unicode Helpers            */
/* ========================================================================= */

static void *allocate_memory(size_t size) {
	void *pointer = malloc(size > 0 ? size : 1);
	if (!pointer) {
		fprintf(stderr, "[!] Fatal: out of memory (%zu bytes)\n", size);
		exit(1);
	}
	return pointer;
}

static void *allocate_zeroed(size_t count, size_t element_size) {
	void *pointer = calloc(count > 0 ? count : 1, element_size > 0 ? element_size : 1);
	if (!pointer) {
		fprintf(stderr, "[!] Fatal: out of memory (%zu x %zu)\n", count, element_size);
		exit(1);
	}
	return pointer;
}

static void *reallocate_memory(void *pointer, size_t size) {
	void *resized = realloc(pointer, size > 0 ? size : 1);
	if (!resized) {
		fprintf(stderr, "[!] Fatal: out of memory on realloc (%zu bytes)\n", size);
		exit(1);
	}
	return resized;
}

static char *duplicate_range(const char *source, size_t length) {
	char *copy = (char *)allocate_memory(length + 1);
	if (length > 0 && source) {
		memcpy(copy, source, length);
	}
	copy[length] = '\0';
	return copy;
}

static char *duplicate_string(const char *source) {
	if (!source) {
		return duplicate_range("", 0);
	}
	return duplicate_range(source, strlen(source));
}

typedef struct {
	char *data;
	size_t length;
	size_t capacity;
} TextBuffer;

static void init_buffer(TextBuffer *buffer) {
	buffer->capacity = 64;
	buffer->length = 0;
	buffer->data = (char *)allocate_memory(buffer->capacity);
	buffer->data[0] = '\0';
}

static void free_buffer(TextBuffer *buffer) {
	free(buffer->data);
	buffer->data = NULL;
	buffer->length = 0;
	buffer->capacity = 0;
}

static void reserve_buffer(TextBuffer *buffer, size_t extra) {
	size_t needed = buffer->length + extra + 1;
	if (needed <= buffer->capacity) {
		return;
	}
	size_t next_capacity = buffer->capacity ? buffer->capacity : 64;
	while (next_capacity < needed) {
		next_capacity *= 2;
	}
	buffer->data = (char *)reallocate_memory(buffer->data, next_capacity);
	buffer->capacity = next_capacity;
}

static void append_bytes(TextBuffer *buffer, const char *bytes, size_t count) {
	if (!bytes || count == 0) {
		return;
	}
	reserve_buffer(buffer, count);
	memcpy(buffer->data + buffer->length, bytes, count);
	buffer->length += count;
	buffer->data[buffer->length] = '\0';
}

static void append_string(TextBuffer *buffer, const char *text) {
	if (text && *text) {
		append_bytes(buffer, text, strlen(text));
	}
}

static void append_char(TextBuffer *buffer, char character) {
	reserve_buffer(buffer, 1);
	buffer->data[buffer->length++] = character;
	buffer->data[buffer->length] = '\0';
}

static char *detach_buffer(TextBuffer *buffer) {
	char *result = buffer->data;
	buffer->data = NULL;
	buffer->length = 0;
	buffer->capacity = 0;
	return result ? result : duplicate_string("");
}

static int decode_utf8_point(const char *text, size_t byte_length, size_t offset,
                             uint32_t *out_codepoint) {
	const unsigned char *bytes = (const unsigned char *)text;
	if (offset >= byte_length) {
		*out_codepoint = 0;
		return 0;
	}
	unsigned char first = bytes[offset];
	if (first < 0x80) {
		*out_codepoint = first;
		return 1;
	}
	if ((first & 0xE0) == 0xC0 && offset + 1 < byte_length) {
		unsigned char second = bytes[offset + 1];
		if ((second & 0xC0) == 0x80) {
			uint32_t code = ((uint32_t)(first & 0x1F) << 6) | (uint32_t)(second & 0x3F);
			if (code >= 0x80) {
				*out_codepoint = code;
				return 2;
			}
		}
	} else if ((first & 0xF0) == 0xE0 && offset + 2 < byte_length) {
		unsigned char second = bytes[offset + 1];
		unsigned char third = bytes[offset + 2];
		if ((second & 0xC0) == 0x80 && (third & 0xC0) == 0x80) {
			uint32_t code = ((uint32_t)(first & 0x0F) << 12) |
					((uint32_t)(second & 0x3F) << 6) |
					(uint32_t)(third & 0x3F);
			if (code >= 0x800 && !(code >= 0xD800 && code <= 0xDFFF)) {
				*out_codepoint = code;
				return 3;
			}
		}
	} else if ((first & 0xF8) == 0xF0 && offset + 3 < byte_length) {
		unsigned char second = bytes[offset + 1];
		unsigned char third = bytes[offset + 2];
		unsigned char fourth = bytes[offset + 3];
		if ((second & 0xC0) == 0x80 && (third & 0xC0) == 0x80 && (fourth & 0xC0) == 0x80) {
			uint32_t code = ((uint32_t)(first & 0x07) << 18) |
					((uint32_t)(second & 0x3F) << 12) |
					((uint32_t)(third & 0x3F) << 6) |
					(uint32_t)(fourth & 0x3F);
			if (code >= 0x10000 && code <= 0x10FFFF) {
				*out_codepoint = code;
				return 4;
			}
		}
	}
	*out_codepoint = 0xFFFD;
	return 1;
}

static void append_utf8_point(TextBuffer *buffer, uint32_t codepoint) {
	char encoded[4];
	if (codepoint < 0x80) {
		encoded[0] = (char)codepoint;
		append_bytes(buffer, encoded, 1);
	} else if (codepoint < 0x800) {
		encoded[0] = (char)(0xC0 | (codepoint >> 6));
		encoded[1] = (char)(0x80 | (codepoint & 0x3F));
		append_bytes(buffer, encoded, 2);
	} else if (codepoint < 0x10000) {
		encoded[0] = (char)(0xE0 | (codepoint >> 12));
		encoded[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
		encoded[2] = (char)(0x80 | (codepoint & 0x3F));
		append_bytes(buffer, encoded, 3);
	} else if (codepoint <= 0x10FFFF) {
		encoded[0] = (char)(0xF0 | (codepoint >> 18));
		encoded[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
		encoded[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
		encoded[3] = (char)(0x80 | (codepoint & 0x3F));
		append_bytes(buffer, encoded, 4);
	} else {
		encoded[0] = (char)0xEF;
		encoded[1] = (char)0xBF;
		encoded[2] = (char)0xBD;
		append_bytes(buffer, encoded, 3);
	}
}

static size_t count_utf8_points(const char *text, size_t byte_length) {
	size_t count = 0;
	size_t offset = 0;
	while (offset < byte_length) {
		uint32_t codepoint = 0;
		int step = decode_utf8_point(text, byte_length, offset, &codepoint);
		offset += (size_t)(step > 0 ? step : 1);
		count++;
	}
	return count;
}

#ifdef _WIN32
static wchar_t *utf8_to_wide(const char *text) {
	if (!text) {
		return NULL;
	}
	int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	if (length <= 0) {
		return NULL;
	}
	wchar_t *wide = (wchar_t *)allocate_memory((size_t)length * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, length);
	return wide;
}

static char *wide_to_utf8(const wchar_t *wide) {
	if (!wide) {
		return NULL;
	}
	int length = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	if (length <= 0) {
		return NULL;
	}
	char *utf8 = (char *)allocate_memory((size_t)length);
	WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, length, NULL, NULL);
	return utf8;
}

static int utf16_from_codepoint(uint32_t codepoint, wchar_t pair[2]) {
	if (codepoint <= 0xFFFF) {
		pair[0] = (wchar_t)codepoint;
		return 1;
	}
	codepoint -= 0x10000;
	pair[0] = (wchar_t)(0xD800 + (codepoint >> 10));
	pair[1] = (wchar_t)(0xDC00 + (codepoint & 0x3FF));
	return 2;
}
#endif

static bool is_unicode_space(uint32_t codepoint) {
	if (codepoint <= 0x7F) {
		return codepoint == ' ' || codepoint == '\t' || codepoint == '\n' ||
		       codepoint == '\r' || codepoint == '\v' || codepoint == '\f';
	}
	if (codepoint == 0x0085 || codepoint == 0x00A0 || codepoint == 0x1680 ||
	    (codepoint >= 0x2000 && codepoint <= 0x200A) || codepoint == 0x2028 ||
	    codepoint == 0x2029 || codepoint == 0x202F || codepoint == 0x205F ||
	    codepoint == 0x3000) {
		return true;
	}
#ifdef _WIN32
	wchar_t pair[2];
	WORD ctype[2] = {0, 0};
	int count = utf16_from_codepoint(codepoint, pair);
	if (GetStringTypeW(CT_CTYPE1, pair, count, ctype)) {
		return (ctype[0] & (C1_SPACE | C1_BLANK)) != 0;
	}
#endif
	return false;
}

static bool is_unicode_alpha(uint32_t codepoint) {
	if (codepoint <= 0x7F) {
		return (codepoint >= 'A' && codepoint <= 'Z') ||
		       (codepoint >= 'a' && codepoint <= 'z');
	}
#ifdef _WIN32
	wchar_t pair[2];
	WORD ctype[2] = {0, 0};
	int count = utf16_from_codepoint(codepoint, pair);
	if (GetStringTypeW(CT_CTYPE1, pair, count, ctype)) {
		return (ctype[0] & C1_ALPHA) != 0;
	}
#endif
	return fz_isalpha((int)codepoint) != 0;
}

static bool is_unicode_lower(uint32_t codepoint) {
	if (codepoint <= 0x7F) {
		return codepoint >= 'a' && codepoint <= 'z';
	}
#ifdef _WIN32
	wchar_t pair[2];
	WORD ctype[2] = {0, 0};
	int count = utf16_from_codepoint(codepoint, pair);
	if (GetStringTypeW(CT_CTYPE1, pair, count, ctype)) {
		return (ctype[0] & C1_LOWER) != 0;
	}
#endif
	return is_unicode_alpha(codepoint) && (uint32_t)fz_tolower((int)codepoint) == codepoint;
}

static bool is_unicode_upper(uint32_t codepoint) {
	if (codepoint <= 0x7F) {
		return codepoint >= 'A' && codepoint <= 'Z';
	}
#ifdef _WIN32
	wchar_t pair[2];
	WORD ctype[2] = {0, 0};
	int count = utf16_from_codepoint(codepoint, pair);
	if (GetStringTypeW(CT_CTYPE1, pair, count, ctype)) {
		return (ctype[0] & C1_UPPER) != 0;
	}
#endif
	return is_unicode_alpha(codepoint) && (uint32_t)fz_toupper((int)codepoint) == codepoint;
}

static char *normalize_nfc_utf8(const char *text) {
	if (!text || !*text) {
		return duplicate_string("");
	}
#ifdef _WIN32
	bool ascii_only = true;
	for (const unsigned char *scan = (const unsigned char *)text; *scan; ++scan) {
		if (*scan >= 0x80) {
			ascii_only = false;
			break;
		}
	}
	if (ascii_only) {
		return duplicate_string(text);
	}
	wchar_t *wide = utf8_to_wide(text);
	if (!wide) {
		return duplicate_string(text);
	}
	int wide_len = (int)wcslen(wide);
	int estimate = NormalizeString(NormalizationC, wide, wide_len, NULL, 0);
	if (estimate <= 0) {
		free(wide);
		return duplicate_string(text);
	}
	wchar_t *normalized = (wchar_t *)allocate_memory((size_t)(estimate + 4) * sizeof(wchar_t));
	int actual = NormalizeString(NormalizationC, wide, wide_len, normalized, estimate + 3);
	free(wide);
	if (actual <= 0) {
		free(normalized);
		return duplicate_string(text);
	}
	normalized[actual] = L'\0';
	char *result = wide_to_utf8(normalized);
	free(normalized);
	return result ? result : duplicate_string(text);
#else
	return duplicate_string(text);
#endif
}

static char *to_lowercase_utf8(const char *text) {
	if (!text || !*text) {
		return duplicate_string("");
	}
#ifdef _WIN32
	bool ascii_only = true;
	for (const unsigned char *scan = (const unsigned char *)text; *scan; ++scan) {
		if (*scan >= 0x80) {
			ascii_only = false;
			break;
		}
	}
	if (ascii_only) {
		size_t length = strlen(text);
		char *lowered = (char *)allocate_memory(length + 1);
		for (size_t index = 0; index < length; ++index) {
			unsigned char ch = (unsigned char)text[index];
			lowered[index] = (ch >= 'A' && ch <= 'Z') ? (char)(ch + 32) : (char)ch;
		}
		lowered[length] = '\0';
		return lowered;
	}
	wchar_t *wide = utf8_to_wide(text);
	if (!wide) {
		return duplicate_string(text);
	}
	int wide_len = (int)wcslen(wide);
	int needed = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, wide, wide_len, NULL, 0,
				   NULL, NULL, 0);
	if (needed <= 0) {
		free(wide);
		return duplicate_string(text);
	}
	wchar_t *lowered_wide = (wchar_t *)allocate_memory((size_t)(needed + 2) * sizeof(wchar_t));
	int written = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, wide, wide_len,
				    lowered_wide, needed + 1, NULL, NULL, 0);
	free(wide);
	if (written <= 0) {
		free(lowered_wide);
		return duplicate_string(text);
	}
	lowered_wide[written] = L'\0';
	char *result = wide_to_utf8(lowered_wide);
	free(lowered_wide);
	return result ? result : duplicate_string(text);
#else
	TextBuffer out;
	init_buffer(&out);
	size_t length = strlen(text);
	size_t offset = 0;
	while (offset < length) {
		uint32_t code = 0;
		int step = decode_utf8_point(text, length, offset, &code);
		offset += (size_t)(step > 0 ? step : 1);
		append_utf8_point(&out, (uint32_t)fz_tolower((int)code));
	}
	return detach_buffer(&out);
#endif
}

static bool equals_ignore_case(const char *left, const char *right) {
	if (!left || !right) {
		return left == right;
	}
	return fz_strcasecmp_ascii(left, right) == 0;
}

static bool starts_with_str(const char *text, const char *prefix) {
	if (!text || !prefix) {
		return false;
	}
	size_t prefix_len = strlen(prefix);
	return strncmp(text, prefix, prefix_len) == 0;
}

static bool ends_with_str(const char *text, const char *suffix) {
	if (!text || !suffix) {
		return false;
	}
	size_t text_len = strlen(text);
	size_t suffix_len = strlen(suffix);
	if (suffix_len > text_len) {
		return false;
	}
	return memcmp(text + text_len - suffix_len, suffix, suffix_len) == 0;
}

static bool contains_ignore_case(const char *haystack, const char *needle) {
	if (!haystack || !needle) {
		return false;
	}
	size_t needle_len = strlen(needle);
	if (needle_len == 0) {
		return true;
	}
	size_t hay_len = strlen(haystack);
	if (needle_len > hay_len) {
		return false;
	}
	for (size_t index = 0; index <= hay_len - needle_len; ++index) {
		size_t pos = 0;
		while (pos < needle_len) {
			unsigned char left = (unsigned char)haystack[index + pos];
			unsigned char right = (unsigned char)needle[pos];
			if (left >= 'A' && left <= 'Z') {
				left += 32;
			}
			if (right >= 'A' && right <= 'Z') {
				right += 32;
			}
			if (left != right) {
				break;
			}
			pos++;
		}
		if (pos == needle_len) {
			return true;
		}
	}
	return false;
}

static char *collapse_whitespace_alloc(const char *text) {
	TextBuffer out;
	init_buffer(&out);
	if (!text) {
		return detach_buffer(&out);
	}
	size_t byte_len = strlen(text);
	size_t offset = 0;
	bool in_space = true;
	while (offset < byte_len) {
		uint32_t code = 0;
		int step = decode_utf8_point(text, byte_len, offset, &code);
		offset += (size_t)(step > 0 ? step : 1);
		if (is_unicode_space(code)) {
			if (!in_space) {
				append_char(&out, ' ');
				in_space = true;
			}
		} else {
			append_utf8_point(&out, code);
			in_space = false;
		}
	}
	if (out.length > 0 && out.data[out.length - 1] == ' ') {
		out.data[--out.length] = '\0';
	}
	return detach_buffer(&out);
}

static void trim_inplace(char *text) {
	if (!text) {
		return;
	}
	size_t byte_len = strlen(text);
	size_t start = 0;
	while (start < byte_len) {
		uint32_t code = 0;
		int step = decode_utf8_point(text, byte_len, start, &code);
		if (!is_unicode_space(code)) {
			break;
		}
		start += (size_t)(step > 0 ? step : 1);
	}
	size_t end = byte_len;
	while (end > start) {
		size_t probe = end - 1;
		while (probe > start && (((unsigned char)text[probe] & 0xC0) == 0x80)) {
			probe--;
		}
		uint32_t code = 0;
		decode_utf8_point(text, byte_len, probe, &code);
		if (!is_unicode_space(code)) {
			break;
		}
		end = probe;
	}
	size_t new_len = end - start;
	if (start > 0 && new_len > 0) {
		memmove(text, text + start, new_len);
	}
	text[new_len] = '\0';
}

static bool has_non_whitespace(const char *text) {
	if (!text) {
		return false;
	}
	size_t byte_len = strlen(text);
	size_t offset = 0;
	while (offset < byte_len) {
		uint32_t code = 0;
		int step = decode_utf8_point(text, byte_len, offset, &code);
		if (!is_unicode_space(code)) {
			return true;
		}
		offset += (size_t)(step > 0 ? step : 1);
	}
	return false;
}

static void format_thousands(int64_t value, char out[32]) {
	char raw[32];
	snprintf(raw, sizeof(raw), "%lld", (long long)value);
	size_t len = strlen(raw);
	size_t start = (raw[0] == '-') ? 1 : 0;
	size_t digits = len - start;
	size_t pos = 0;
	if (start > 0) {
		out[pos++] = '-';
	}
	for (size_t index = 0; index < digits; ++index) {
		if (index > 0 && (digits - index) % 3 == 0) {
			out[pos++] = ',';
		}
		out[pos++] = raw[start + index];
	}
	out[pos] = '\0';
}

/* ========================================================================= */
/* Section 2: Filesystem & Path Utilities                                    */
/* ========================================================================= */

static bool path_exists(const char *path) {
	if (!path || !*path) {
		return false;
	}
#ifdef _WIN32
	wchar_t *wide = utf8_to_wide(path);
	if (!wide) {
		return false;
	}
	DWORD attrs = GetFileAttributesW(wide);
	free(wide);
	return attrs != INVALID_FILE_ATTRIBUTES;
#else
	struct stat info;
	return stat(path, &info) == 0;
#endif
}

static bool path_is_directory(const char *path) {
	if (!path || !*path) {
		return false;
	}
#ifdef _WIN32
	wchar_t *wide = utf8_to_wide(path);
	if (!wide) {
		return false;
	}
	DWORD attrs = GetFileAttributesW(wide);
	free(wide);
	return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	struct stat info;
	return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
#endif
}

static bool make_directory_tree(const char *path) {
	if (!path || !*path) {
		return false;
	}
	char *copy = duplicate_string(path);
	size_t length = strlen(copy);
	for (size_t index = 0; index < length; ++index) {
		if (copy[index] == '/' || copy[index] == '\\') {
			if (index == 0) {
				continue;
			}
#ifdef _WIN32
			if (index == 2 && copy[1] == ':') {
				continue;
			}
			if (index <= 2 && copy[0] == '\\' && copy[1] == '\\') {
				continue;
			}
#endif
			char saved = copy[index];
			copy[index] = '\0';
			if (!path_exists(copy)) {
#ifdef _WIN32
				wchar_t *wide = utf8_to_wide(copy);
				if (wide) {
					CreateDirectoryW(wide, NULL);
					free(wide);
				}
#else
				mkdir(copy, 0755);
#endif
			}
			copy[index] = saved;
		}
	}
	if (!path_exists(copy)) {
#ifdef _WIN32
		wchar_t *wide = utf8_to_wide(copy);
		if (wide) {
			CreateDirectoryW(wide, NULL);
			free(wide);
		}
#else
		mkdir(copy, 0755);
#endif
	}
	bool ok = path_is_directory(copy);
	free(copy);
	return ok;
}

static char *resolve_absolute_path(const char *path) {
	if (!path || !*path) {
		return duplicate_string("");
	}
#ifdef _WIN32
	wchar_t *wide = utf8_to_wide(path);
	if (!wide) {
		return duplicate_string(path);
	}
	DWORD needed = GetFullPathNameW(wide, 0, NULL, NULL);
	wchar_t *full = (wchar_t *)allocate_memory((size_t)(needed + 4) * sizeof(wchar_t));
	DWORD written = GetFullPathNameW(wide, needed + 3, full, NULL);
	free(wide);
	if (written == 0) {
		free(full);
		return duplicate_string(path);
	}
	size_t flen = wcslen(full);
	while (flen > 3 && (full[flen - 1] == L'\\' || full[flen - 1] == L'/')) {
		full[--flen] = L'\0';
	}
	char *utf8 = wide_to_utf8(full);
	free(full);
	return utf8 ? utf8 : duplicate_string(path);
#else
	char resolved[PATH_MAX];
	if (realpath(path, resolved)) {
		return duplicate_string(resolved);
	}
	return duplicate_string(path);
#endif
}

static char *join_path(const char *base, const char *child) {
	TextBuffer out;
	init_buffer(&out);
	append_string(&out, base);
#ifdef _WIN32
	char sep = '\\';
#else
	char sep = '/';
#endif
	if (out.length > 0 && out.data[out.length - 1] != '/' && out.data[out.length - 1] != '\\') {
		append_char(&out, sep);
	}
	append_string(&out, child);
	return detach_buffer(&out);
}

static char *replace_extension(const char *path, const char *new_ext) {
	const char *slash = strrchr(path, '/');
	const char *backslash = strrchr(path, '\\');
	const char *sep = slash > backslash ? slash : backslash;
	const char *dot = strrchr(path, '.');
	size_t base_len = (dot && (!sep || dot > sep)) ? (size_t)(dot - path) : strlen(path);
	TextBuffer out;
	init_buffer(&out);
	append_bytes(&out, path, base_len);
	append_string(&out, new_ext);
	return detach_buffer(&out);
}

static const char *path_basename_ptr(const char *path) {
	const char *slash = strrchr(path, '/');
	const char *backslash = strrchr(path, '\\');
	const char *sep = slash > backslash ? slash : backslash;
	return sep ? sep + 1 : path;
}

static const char *path_extension_ptr(const char *path) {
	const char *base = path_basename_ptr(path);
	const char *dot = strrchr(base, '.');
	return dot ? dot : "";
}

static char *path_stem_alloc(const char *path) {
	const char *base = path_basename_ptr(path);
	const char *dot = strrchr(base, '.');
	if (!dot || dot == base) {
		return duplicate_string(base);
	}
	return duplicate_range(base, (size_t)(dot - base));
}

static char *normalize_path_key(const char *path) {
#ifdef _WIN32
	char *lower = to_lowercase_utf8(path);
	for (char *scan = lower; *scan; ++scan) {
		if (*scan == '\\') {
			*scan = '/';
		}
	}
	return lower;
#else
	return duplicate_string(path);
#endif
}

static bool paths_equal(const char *left, const char *right) {
	char *left_key = normalize_path_key(left);
	char *right_key = normalize_path_key(right);
	bool same = strcmp(left_key, right_key) == 0;
	free(left_key);
	free(right_key);
	return same;
}

static char *compute_relative_path(const char *target_path, const char *base_dir) {
	char *target_abs = resolve_absolute_path(target_path);
	char *base_abs = resolve_absolute_path(base_dir);
	char *target_norm = duplicate_string(target_abs);
	char *base_norm = duplicate_string(base_abs);
	for (char *p = target_norm; *p; ++p) {
		if (*p == '\\') {
			*p = '/';
		}
	}
	for (char *p = base_norm; *p; ++p) {
		if (*p == '\\') {
			*p = '/';
		}
	}
	free(target_abs);
	free(base_abs);

#ifdef _WIN32
	if (strlen(target_norm) >= 2 && strlen(base_norm) >= 2 && target_norm[1] == ':' &&
	    base_norm[1] == ':' && tolower((unsigned char)target_norm[0]) != tolower((unsigned char)base_norm[0])) {
		free(base_norm);
		return target_norm;
	}
#endif

	const char *t_parts[256];
	size_t t_lens[256];
	int t_count = 0;
	const char *b_parts[256];
	size_t b_lens[256];
	int b_count = 0;

	const char *scan = target_norm;
	while (*scan && t_count < 256) {
		while (*scan == '/') {
			scan++;
		}
		if (!*scan) {
			break;
		}
		const char *end = strchr(scan, '/');
		size_t len = end ? (size_t)(end - scan) : strlen(scan);
		t_parts[t_count] = scan;
		t_lens[t_count] = len;
		t_count++;
		scan += len;
	}

	scan = base_norm;
	while (*scan && b_count < 256) {
		while (*scan == '/') {
			scan++;
		}
		if (!*scan) {
			break;
		}
		const char *end = strchr(scan, '/');
		size_t len = end ? (size_t)(end - scan) : strlen(scan);
		b_parts[b_count] = scan;
		b_lens[b_count] = len;
		b_count++;
		scan += len;
	}

	int common = 0;
	while (common < t_count && common < b_count && t_lens[common] == b_lens[common]) {
#ifdef _WIN32
		if (_strnicmp(t_parts[common], b_parts[common], t_lens[common]) != 0) {
			break;
		}
#else
		if (memcmp(t_parts[common], b_parts[common], t_lens[common]) != 0) {
			break;
		}
#endif
		common++;
	}

	TextBuffer out;
	init_buffer(&out);
	for (int index = common; index < b_count; ++index) {
		if (out.length > 0) {
			append_char(&out, '/');
		}
		append_string(&out, "..");
	}
	for (int index = common; index < t_count; ++index) {
		if (out.length > 0) {
			append_char(&out, '/');
		}
		append_bytes(&out, t_parts[index], t_lens[index]);
	}
	if (out.length == 0) {
		append_char(&out, '.');
	}
	free(target_norm);
	free(base_norm);
	return detach_buffer(&out);
}

static void remove_file_utf8(const char *path) {
	if (!path || !*path) {
		return;
	}
#ifdef _WIN32
	wchar_t *wide = utf8_to_wide(path);
	if (wide) {
		DeleteFileW(wide);
		free(wide);
	}
#else
	unlink(path);
#endif
}

static FILE *open_file_utf8(const char *path, const char *mode) {
#ifdef _WIN32
	wchar_t *wide_path = utf8_to_wide(path);
	wchar_t *wide_mode = utf8_to_wide(mode);
	FILE *handle = NULL;
	if (wide_path && wide_mode) {
		handle = _wfopen(wide_path, wide_mode);
	}
	free(wide_path);
	free(wide_mode);
	return handle;
#else
	return fopen(path, mode);
#endif
}

static bool read_file_bytes(const char *path, uint8_t **out_data, size_t *out_size) {
	*out_data = NULL;
	*out_size = 0;
	FILE *file = open_file_utf8(path, "rb");
	if (!file) {
		return false;
	}
	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return false;
	}
	long size = ftell(file);
	if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return false;
	}
	uint8_t *buffer = (uint8_t *)allocate_memory((size_t)size + 1);
	size_t read_len = size > 0 ? fread(buffer, 1, (size_t)size, file) : 0;
	fclose(file);
	if (read_len != (size_t)size) {
		free(buffer);
		return false;
	}
	buffer[read_len] = 0;
	*out_data = buffer;
	*out_size = read_len;
	return true;
}

static bool write_file_utf8(const char *path, const char *data, size_t length) {
	FILE *file = open_file_utf8(path, "wb");
	if (!file) {
		return false;
	}
	size_t written = length > 0 ? fwrite(data, 1, length, file) : 0;
	fclose(file);
	return written == length;
}

static char *resolve_smart_path(const char *raw_path) {
	if (path_exists(raw_path)) {
		return resolve_absolute_path(raw_path);
	}
	if (starts_with_str(raw_path, "core/") || starts_with_str(raw_path, "core\\")) {
		if (path_exists(raw_path + 5)) {
			return resolve_absolute_path(raw_path + 5);
		}
	} else {
		char *with_core = join_path("core", raw_path);
		if (path_exists(with_core)) {
			char *resolved = resolve_absolute_path(with_core);
			free(with_core);
			return resolved;
		}
		free(with_core);
	}
	return resolve_absolute_path(raw_path);
}

/* ========================================================================= */
/* Section 3: Core Containers & Deduplication State                          */
/* ========================================================================= */

typedef struct {
	char **items;
	size_t count;
	size_t capacity;
} TextList;

static void init_text_list(TextList *list) {
	list->items = NULL;
	list->count = 0;
	list->capacity = 0;
}

static void push_text_owned(TextList *list, char *owned_text) {
	if (list->count == list->capacity) {
		list->capacity = list->capacity ? list->capacity * 2 : 16;
		list->items = (char **)reallocate_memory(list->items, list->capacity * sizeof(char *));
	}
	list->items[list->count++] = owned_text;
}

static void push_text_copy(TextList *list, const char *text) {
	push_text_owned(list, duplicate_string(text));
}

static void free_text_list(TextList *list) {
	for (size_t index = 0; index < list->count; ++index) {
		free(list->items[index]);
	}
	free(list->items);
	list->items = NULL;
	list->count = 0;
	list->capacity = 0;
}

static uint64_t hash_string_fnv(const char *text, size_t length) {
	uint64_t hash = 14695981039346656037ULL;
	for (size_t index = 0; index < length; ++index) {
		hash ^= (unsigned char)text[index];
		hash *= 1099511628211ULL;
	}
	return hash;
}

typedef struct {
	char *key;
	size_t key_len;
	int value;
	uint8_t occupied;
} StringMapSlot;

typedef struct {
	StringMapSlot *slots;
	size_t capacity;
	size_t count;
} TextCounter;

static void init_text_counter(TextCounter *map) {
	map->capacity = 64;
	map->count = 0;
	map->slots = (StringMapSlot *)allocate_zeroed(map->capacity, sizeof(StringMapSlot));
}

static void free_text_counter(TextCounter *map) {
	if (map->slots) {
		for (size_t index = 0; index < map->capacity; ++index) {
			if (map->slots[index].occupied) {
				free(map->slots[index].key);
			}
		}
		free(map->slots);
	}
	map->slots = NULL;
	map->capacity = 0;
	map->count = 0;
}

static void rehash_text_counter(TextCounter *map, size_t next_capacity) {
	StringMapSlot *old_slots = map->slots;
	size_t old_capacity = map->capacity;
	map->slots = (StringMapSlot *)allocate_zeroed(next_capacity, sizeof(StringMapSlot));
	map->capacity = next_capacity;
	size_t mask = next_capacity - 1;
	for (size_t index = 0; index < old_capacity; ++index) {
		if (!old_slots[index].occupied) {
			continue;
		}
		size_t pos = (size_t)hash_string_fnv(old_slots[index].key, old_slots[index].key_len) & mask;
		while (map->slots[pos].occupied) {
			pos = (pos + 1) & mask;
		}
		map->slots[pos] = old_slots[index];
	}
	free(old_slots);
}

static int get_text_count(const TextCounter *map, const char *key) {
	if (!map->slots || !key) {
		return 0;
	}
	size_t key_len = strlen(key);
	size_t mask = map->capacity - 1;
	size_t pos = (size_t)hash_string_fnv(key, key_len) & mask;
	while (map->slots[pos].occupied) {
		if (map->slots[pos].key_len == key_len &&
		    memcmp(map->slots[pos].key, key, key_len) == 0) {
			return map->slots[pos].value;
		}
		pos = (pos + 1) & mask;
	}
	return 0;
}

static bool add_text_count(TextCounter *map, const char *key, int delta) {
	if ((map->count + 1) * 10 >= map->capacity * 7) {
		rehash_text_counter(map, map->capacity ? map->capacity * 2 : 64);
	}
	size_t key_len = strlen(key);
	size_t mask = map->capacity - 1;
	size_t pos = (size_t)hash_string_fnv(key, key_len) & mask;
	while (map->slots[pos].occupied) {
		if (map->slots[pos].key_len == key_len &&
		    memcmp(map->slots[pos].key, key, key_len) == 0) {
			map->slots[pos].value += delta;
			return false;
		}
		pos = (pos + 1) & mask;
	}
	map->slots[pos].key = duplicate_range(key, key_len);
	map->slots[pos].key_len = key_len;
	map->slots[pos].value = delta;
	map->slots[pos].occupied = 1;
	map->count++;
	return true;
}

static bool has_text_key(const TextCounter *map, const char *key) {
	return get_text_count(map, key) > 0;
}

typedef struct {
	uint64_t *keys;
	uint8_t *occupied;
	size_t capacity;
	size_t count;
} Hash64Set;

static void init_hash64_set(Hash64Set *set) {
	set->capacity = 64;
	set->count = 0;
	set->keys = (uint64_t *)allocate_zeroed(set->capacity, sizeof(uint64_t));
	set->occupied = (uint8_t *)allocate_zeroed(set->capacity, sizeof(uint8_t));
}

static void free_hash64_set(Hash64Set *set) {
	free(set->keys);
	free(set->occupied);
	set->keys = NULL;
	set->occupied = NULL;
	set->capacity = 0;
	set->count = 0;
}

static uint64_t mix_hash64(uint64_t value) {
	value ^= value >> 33;
	value *= 0xff51afd7ed558ccdULL;
	value ^= value >> 33;
	value *= 0xc4ceb9fe1a85ec53ULL;
	value ^= value >> 33;
	return value;
}

static void rehash_hash64_set(Hash64Set *set, size_t next_capacity) {
	uint64_t *old_keys = set->keys;
	uint8_t *old_occ = set->occupied;
	size_t old_cap = set->capacity;
	set->keys = (uint64_t *)allocate_zeroed(next_capacity, sizeof(uint64_t));
	set->occupied = (uint8_t *)allocate_zeroed(next_capacity, sizeof(uint8_t));
	set->capacity = next_capacity;
	size_t mask = next_capacity - 1;
	for (size_t index = 0; index < old_cap; ++index) {
		if (!old_occ[index]) {
			continue;
		}
		size_t pos = (size_t)mix_hash64(old_keys[index]) & mask;
		while (set->occupied[pos]) {
			pos = (pos + 1) & mask;
		}
		set->keys[pos] = old_keys[index];
		set->occupied[pos] = 1;
	}
	free(old_keys);
	free(old_occ);
}

static bool insert_hash64(Hash64Set *set, uint64_t key) {
	if ((set->count + 1) * 10 >= set->capacity * 7) {
		rehash_hash64_set(set, set->capacity ? set->capacity * 2 : 64);
	}
	size_t mask = set->capacity - 1;
	size_t pos = (size_t)mix_hash64(key) & mask;
	while (set->occupied[pos]) {
		if (set->keys[pos] == key) {
			return false;
		}
		pos = (pos + 1) & mask;
	}
	set->keys[pos] = key;
	set->occupied[pos] = 1;
	set->count++;
	return true;
}

typedef struct {
	int *items;
	size_t count;
	size_t capacity;
} IntList;

static void push_int(IntList *list, int value) {
	if (list->count == list->capacity) {
		list->capacity = list->capacity ? list->capacity * 2 : 8;
		list->items = (int *)reallocate_memory(list->items, list->capacity * sizeof(int));
	}
	list->items[list->count++] = value;
}

static void free_int_list(IntList *list) {
	free(list->items);
	list->items = NULL;
	list->count = 0;
	list->capacity = 0;
}

typedef struct {
	int band;
	uint64_t rows[MINHASH_ROWS];
	IntList doc_indices;
	uint8_t occupied;
} LshSlot;

typedef struct {
	LshSlot *slots;
	size_t capacity;
	size_t count;
} LshBucketMap;

static void init_lsh_map(LshBucketMap *map) {
	map->capacity = 256;
	map->count = 0;
	map->slots = (LshSlot *)allocate_zeroed(map->capacity, sizeof(LshSlot));
}

static void free_lsh_map(LshBucketMap *map) {
	if (map->slots) {
		for (size_t index = 0; index < map->capacity; ++index) {
			if (map->slots[index].occupied) {
				free_int_list(&map->slots[index].doc_indices);
			}
		}
		free(map->slots);
	}
	map->slots = NULL;
	map->capacity = 0;
	map->count = 0;
}

static uint64_t hash_lsh_key(int band, const uint64_t rows[MINHASH_ROWS]) {
	uint64_t hash = mix_hash64((uint64_t)(uint32_t)band + 0x9e3779b97f4a7c15ULL);
	for (int index = 0; index < MINHASH_ROWS; ++index) {
		hash ^= mix_hash64(rows[index] + 0x9e3779b97f4a7c15ULL * (uint64_t)(index + 1));
		hash = mix_hash64(hash);
	}
	return hash;
}

static bool equal_lsh_key(const LshSlot *slot, int band, const uint64_t rows[MINHASH_ROWS]) {
	if (slot->band != band) {
		return false;
	}
	for (int index = 0; index < MINHASH_ROWS; ++index) {
		if (slot->rows[index] != rows[index]) {
			return false;
		}
	}
	return true;
}

static void rehash_lsh_map(LshBucketMap *map, size_t next_capacity) {
	LshSlot *old_slots = map->slots;
	size_t old_cap = map->capacity;
	map->slots = (LshSlot *)allocate_zeroed(next_capacity, sizeof(LshSlot));
	map->capacity = next_capacity;
	size_t mask = next_capacity - 1;
	for (size_t index = 0; index < old_cap; ++index) {
		if (!old_slots[index].occupied) {
			continue;
		}
		size_t pos = (size_t)hash_lsh_key(old_slots[index].band, old_slots[index].rows) & mask;
		while (map->slots[pos].occupied) {
			pos = (pos + 1) & mask;
		}
		map->slots[pos] = old_slots[index];
	}
	free(old_slots);
}

static const IntList *find_lsh_bucket(const LshBucketMap *map, int band,
                                      const uint64_t rows[MINHASH_ROWS]) {
	if (!map->slots) {
		return NULL;
	}
	size_t mask = map->capacity - 1;
	size_t pos = (size_t)hash_lsh_key(band, rows) & mask;
	while (map->slots[pos].occupied) {
		if (equal_lsh_key(&map->slots[pos], band, rows)) {
			return &map->slots[pos].doc_indices;
		}
		pos = (pos + 1) & mask;
	}
	return NULL;
}

static void insert_lsh_bucket(LshBucketMap *map, int band, const uint64_t rows[MINHASH_ROWS],
                              int doc_index) {
	if ((map->count + 1) * 10 >= map->capacity * 7) {
		rehash_lsh_map(map, map->capacity ? map->capacity * 2 : 256);
	}
	size_t mask = map->capacity - 1;
	size_t pos = (size_t)hash_lsh_key(band, rows) & mask;
	while (map->slots[pos].occupied) {
		if (equal_lsh_key(&map->slots[pos], band, rows)) {
			push_int(&map->slots[pos].doc_indices, doc_index);
			return;
		}
		pos = (pos + 1) & mask;
	}
	map->slots[pos].band = band;
	memcpy(map->slots[pos].rows, rows, sizeof(uint64_t) * MINHASH_ROWS);
	map->slots[pos].doc_indices.items = NULL;
	map->slots[pos].doc_indices.count = 0;
	map->slots[pos].doc_indices.capacity = 0;
	push_int(&map->slots[pos].doc_indices, doc_index);
	map->slots[pos].occupied = 1;
	map->count++;
}

typedef struct {
	double x0, y0, x1, y1;
} BoxRect;

typedef struct {
	BoxRect bbox;
	BlockKind kind;
	char *content;
} PageItem;

typedef struct {
	PageItem *items;
	size_t count;
	size_t capacity;
} PageItemList;

static void push_page_item(PageItemList *list, BoxRect bbox, BlockKind kind, char *owned_content) {
	if (list->count == list->capacity) {
		list->capacity = list->capacity ? list->capacity * 2 : 32;
		list->items =
			(PageItem *)reallocate_memory(list->items, list->capacity * sizeof(PageItem));
	}
	list->items[list->count].bbox = bbox;
	list->items[list->count].kind = kind;
	list->items[list->count].content = owned_content;
	list->count++;
}

static void free_page_items(PageItemList *list) {
	for (size_t index = 0; index < list->count; ++index) {
		free(list->items[index].content);
	}
	free(list->items);
	list->items = NULL;
	list->count = 0;
	list->capacity = 0;
}

typedef struct {
	BlockKind kind;
	char *text;
} ContentBlock;

typedef struct {
	ContentBlock *items;
	size_t count;
	size_t capacity;
} BlockList;

static void init_block_list(BlockList *list) {
	list->items = NULL;
	list->count = 0;
	list->capacity = 0;
}

static void push_block(BlockList *list, BlockKind kind, char *owned_text) {
	if (list->count == list->capacity) {
		list->capacity = list->capacity ? list->capacity * 2 : 32;
		list->items = (ContentBlock *)reallocate_memory(list->items,
								list->capacity * sizeof(ContentBlock));
	}
	list->items[list->count].kind = kind;
	list->items[list->count].text = owned_text;
	list->count++;
}

static void free_block_list(BlockList *list) {
	for (size_t index = 0; index < list->count; ++index) {
		free(list->items[index].text);
	}
	free(list->items);
	list->items = NULL;
	list->count = 0;
	list->capacity = 0;
}

typedef struct {
	uint64_t *items;
	size_t count;
	size_t capacity;
} Hash64List;

static void push_hash64(Hash64List *list, uint64_t value) {
	if (list->count == list->capacity) {
		list->capacity = list->capacity ? list->capacity * 2 : 16;
		list->items =
			(uint64_t *)reallocate_memory(list->items, list->capacity * sizeof(uint64_t));
	}
	list->items[list->count++] = value;
}

static void free_hash64_list(Hash64List *list) {
	free(list->items);
	list->items = NULL;
	list->count = 0;
	list->capacity = 0;
}

typedef struct {
	char source_type[32];
	int page_count;
	BlockList blocks;
	int image_count;
	int scanned_pages;
	size_t corrupt_characters;
	size_t raw_characters;
	TextList saved_images;
	Hash64List image_dhashes;
	TextCounter image_digests;
} ExtractedDocument;

static void init_extracted_doc(ExtractedDocument *doc, const char *source_type) {
	memset(doc, 0, sizeof(*doc));
	fz_strlcpy(doc->source_type, source_type ? source_type : "", sizeof(doc->source_type));
	init_block_list(&doc->blocks);
	init_text_list(&doc->saved_images);
	init_text_counter(&doc->image_digests);
}

static void free_extracted_doc(ExtractedDocument *doc) {
	free_block_list(&doc->blocks);
	free_text_list(&doc->saved_images);
	free_hash64_list(&doc->image_dhashes);
	free_text_counter(&doc->image_digests);
}

typedef struct {
	uint64_t values[MINHASH_SIZE];
} MinHashSignature;

typedef struct {
	TextCounter exact_hashes;
	LshBucketMap lsh_buckets;
	MinHashSignature *signatures;
	size_t signature_count;
	size_t signature_capacity;
	TextCounter paragraph_counts;
	TextCounter image_doc_counts;
	Hash64Set corpus_dhashes;
} CorpusState;

static void init_corpus_state(CorpusState *state) {
	init_text_counter(&state->exact_hashes);
	init_lsh_map(&state->lsh_buckets);
	state->signatures = NULL;
	state->signature_count = 0;
	state->signature_capacity = 0;
	init_text_counter(&state->paragraph_counts);
	init_text_counter(&state->image_doc_counts);
	init_hash64_set(&state->corpus_dhashes);
}

static void free_corpus_state(CorpusState *state) {
	free_text_counter(&state->exact_hashes);
	free_lsh_map(&state->lsh_buckets);
	free(state->signatures);
	state->signatures = NULL;
	state->signature_count = 0;
	state->signature_capacity = 0;
	free_text_counter(&state->paragraph_counts);
	free_text_counter(&state->image_doc_counts);
	free_hash64_set(&state->corpus_dhashes);
}

/* ========================================================================= */
/* Section 4: Hashing (SHA-256, RFC 7693 BLAKE2b-64, Mersenne 61 Mul)        */
/* ========================================================================= */

static void compute_sha256_hex(const uint8_t *data, size_t size, char hex_out[65]) {
	static const char hex_digits[] = "0123456789abcdef";
	fz_sha256 state;
	unsigned char digest[32];
	fz_sha256_init(&state);
	if (data && size > 0) {
		fz_sha256_update(&state, data, size);
	}
	fz_sha256_final(&state, digest);
	for (int index = 0; index < 32; ++index) {
		hex_out[index * 2] = hex_digits[(digest[index] >> 4) & 0x0F];
		hex_out[index * 2 + 1] = hex_digits[digest[index] & 0x0F];
	}
	hex_out[64] = '\0';
}

static uint64_t rotate_right64(uint64_t value, int bits) {
	return (value >> bits) | (value << (64 - bits));
}

static uint64_t load_le64(const uint8_t *src) {
	return ((uint64_t)src[0]) | ((uint64_t)src[1] << 8) | ((uint64_t)src[2] << 16) |
	       ((uint64_t)src[3] << 24) | ((uint64_t)src[4] << 32) | ((uint64_t)src[5] << 40) |
	       ((uint64_t)src[6] << 48) | ((uint64_t)src[7] << 56);
}

static void blake2b_compress(uint64_t state[8], const uint8_t block[128], uint64_t t0,
                             uint64_t t1, bool is_last) {
	static const uint64_t iv[8] = {
		0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL,
		0xa54ff53a5f1d36f1ULL, 0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
		0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
	};
	static const uint8_t sigma[12][16] = {
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
		{14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
		{11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
		{7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
		{9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
		{2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
		{12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
		{13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
		{6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
		{10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
		{14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3}
	};

	uint64_t m[16];
	for (int index = 0; index < 16; ++index) {
		m[index] = load_le64(block + index * 8);
	}
	uint64_t v[16];
	for (int index = 0; index < 8; ++index) {
		v[index] = state[index];
		v[index + 8] = iv[index];
	}
	v[12] ^= t0;
	v[13] ^= t1;
	if (is_last) {
		v[14] = ~v[14];
	}

#define B2B_G(a, b, c, d, x, y)            \
	do {                               \
		v[a] = v[a] + v[b] + (x);  \
		v[d] = rotate_right64(v[d] ^ v[a], 32); \
		v[c] = v[c] + v[d];        \
		v[b] = rotate_right64(v[b] ^ v[c], 24); \
		v[a] = v[a] + v[b] + (y);  \
		v[d] = rotate_right64(v[d] ^ v[a], 16); \
		v[c] = v[c] + v[d];        \
		v[b] = rotate_right64(v[b] ^ v[c], 63); \
	} while (0)

	for (int round = 0; round < 12; ++round) {
		const uint8_t *s = sigma[round];
		B2B_G(0, 4, 8, 12, m[s[0]], m[s[1]]);
		B2B_G(1, 5, 9, 13, m[s[2]], m[s[3]]);
		B2B_G(2, 6, 10, 14, m[s[4]], m[s[5]]);
		B2B_G(3, 7, 11, 15, m[s[6]], m[s[7]]);
		B2B_G(0, 5, 10, 15, m[s[8]], m[s[9]]);
		B2B_G(1, 6, 11, 12, m[s[10]], m[s[11]]);
		B2B_G(2, 7, 8, 13, m[s[12]], m[s[13]]);
		B2B_G(3, 4, 9, 14, m[s[14]], m[s[15]]);
	}
#undef B2B_G

	for (int index = 0; index < 8; ++index) {
		state[index] ^= v[index] ^ v[index + 8];
	}
}

static uint64_t compute_blake2b_64(const uint8_t *data, size_t size) {
	uint64_t state[8] = {
		0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL,
		0xa54ff53a5f1d36f1ULL, 0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
		0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
	};
	/* Parameter block: digest_size=8, fanout=1, depth=1 -> 0x01010008 */
	state[0] ^= 0x01010008ULL;

	uint8_t block[128];
	uint64_t t0 = 0;
	uint64_t t1 = 0;
	size_t offset = 0;
	while (size - offset > 128) {
		uint64_t prev = t0;
		t0 += 128;
		if (t0 < prev) {
			t1++;
		}
		blake2b_compress(state, data + offset, t0, t1, false);
		offset += 128;
	}
	size_t rem = size - offset;
	memset(block, 0, sizeof(block));
	if (rem > 0) {
		memcpy(block, data + offset, rem);
	}
	uint64_t prev = t0;
	t0 += (uint64_t)rem;
	if (t0 < prev) {
		t1++;
	}
	blake2b_compress(state, block, t0, t1, true);
	return state[0];
}

static uint64_t reduce_mersenne61(uint64_t value) {
	uint64_t folded = (value & MINHASH_PRIME) + (value >> 61);
	if (folded >= MINHASH_PRIME) {
		folded -= MINHASH_PRIME;
	}
	return folded;
}

static uint64_t multiply_mod_mersenne(uint64_t left, uint64_t right) {
	/* Both inputs < 2^61; split into 31-bit halves to stay within 64-bit unsigned. */
	uint64_t mask31 = (1ULL << 31) - 1ULL;
	uint64_t l_lo = left & mask31;
	uint64_t l_hi = left >> 31;
	uint64_t r_lo = right & mask31;
	uint64_t r_hi = right >> 31;

	uint64_t mid = l_lo * r_hi + l_hi * r_lo;
	uint64_t hi = l_hi * r_hi;
	uint64_t lo = l_lo * r_lo;

	/* Contribution of hi * 2^62 mod (2^61 - 1) == 2 * hi */
	uint64_t hi_mod = reduce_mersenne61(hi * 2ULL);

	/* Contribution of mid * 2^31 mod (2^61 - 1) */
	uint64_t mid_lo = (mid & ((1ULL << 30) - 1ULL)) << 31;
	uint64_t mid_hi = mid >> 30;
	uint64_t mid_mod = reduce_mersenne61(mid_lo + reduce_mersenne61(mid_hi));

	uint64_t sum = reduce_mersenne61(lo);
	sum = reduce_mersenne61(sum + mid_mod);
	sum = reduce_mersenne61(sum + hi_mod);
	return sum;
}

/* ========================================================================= */
/* Section 5: Text Normalization & Deterministic Pattern Matchers            */
/* ========================================================================= */

static bool is_math_operator_cp(uint32_t codepoint) {
	size_t low = 0;
	size_t high = sizeof(MATH_OPERATORS) / sizeof(MATH_OPERATORS[0]);
	while (low < high) {
		size_t mid = low + (high - low) / 2;
		if (MATH_OPERATORS[mid] == codepoint) {
			return true;
		}
		if (MATH_OPERATORS[mid] < codepoint) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}
	return false;
}

static size_t match_cid_tag_at(const char *text, size_t offset, size_t byte_length) {
	if (offset + 6 > byte_length || memcmp(text + offset, "(cid:", 5) != 0) {
		return 0;
	}
	size_t scan = offset + 5;
	if (scan >= byte_length || !isdigit((unsigned char)text[scan])) {
		return 0;
	}
	while (scan < byte_length && isdigit((unsigned char)text[scan])) {
		scan++;
	}
	if (scan < byte_length && text[scan] == ')') {
		return (scan + 1) - offset;
	}
	return 0;
}

static void count_corrupt_characters(const char *raw_text, size_t *out_corrupt,
                                     size_t *out_raw_len) {
	*out_corrupt = 0;
	*out_raw_len = 0;
	if (!raw_text || !*raw_text) {
		return;
	}
	size_t byte_len = strlen(raw_text);
	size_t cid_chars = 0;
	size_t bad_points = 0;
	size_t codepoint_count = 0;
	size_t offset = 0;

	while (offset < byte_len) {
		size_t cid_len = match_cid_tag_at(raw_text, offset, byte_len);
		if (cid_len > 0) {
			cid_chars += cid_len;
			codepoint_count += cid_len;
			offset += cid_len;
			continue;
		}
		uint32_t code = 0;
		int step = decode_utf8_point(raw_text, byte_len, offset, &code);
		offset += (size_t)(step > 0 ? step : 1);
		codepoint_count++;
		if (code == 0xFFFD || (code >= 0xE000 && code <= 0xF8FF)) {
			bad_points++;
		}
	}
	*out_corrupt = cid_chars + bad_points;
	*out_raw_len = codepoint_count;
}

static bool is_invisible_cp(uint32_t codepoint) {
	return codepoint == 0x00AD || (codepoint >= 0x200B && codepoint <= 0x200F) ||
	       (codepoint >= 0x202A && codepoint <= 0x202E) || codepoint == 0x2060 ||
	       codepoint == 0xFEFF || codepoint == 0xFFFD;
}

static const char *lookup_ligature(uint32_t codepoint) {
	size_t count = sizeof(LIGATURE_MAP) / sizeof(LIGATURE_MAP[0]);
	for (size_t index = 0; index < count; ++index) {
		if (LIGATURE_MAP[index].codepoint == codepoint) {
			return LIGATURE_MAP[index].replacement;
		}
	}
	return NULL;
}

static char *normalize_unicode(const char *text) {
	if (!text || !*text) {
		return duplicate_string("");
	}
	char *nfc = normalize_nfc_utf8(text);
	size_t byte_len = strlen(nfc);
	TextBuffer out;
	init_buffer(&out);
	size_t offset = 0;

	while (offset < byte_len) {
		size_t cid_len = match_cid_tag_at(nfc, offset, byte_len);
		if (cid_len > 0) {
			offset += cid_len;
			continue;
		}
		uint32_t code = 0;
		int step = decode_utf8_point(nfc, byte_len, offset, &code);
		offset += (size_t)(step > 0 ? step : 1);

		const char *replacement = lookup_ligature(code);
		if (replacement) {
			append_string(&out, replacement);
			continue;
		}
		if (is_invisible_cp(code)) {
			continue;
		}
		if (code == 9 || code == 10 || (code >= 32 && code < 127) ||
		    (code >= 160 && !(code >= 0xE000 && code <= 0xF8FF))) {
			append_utf8_point(&out, code);
		} else if (code == 13) {
			if (offset >= byte_len || nfc[offset] != '\n') {
				append_char(&out, '\n');
			}
		} else {
			append_char(&out, ' ');
		}
	}
	free(nfc);
	return detach_buffer(&out);
}

static char *normalize_template(const char *text) {
	char *norm = normalize_unicode(text);
	char *lower = to_lowercase_utf8(norm);
	free(norm);
	char *compact = collapse_whitespace_alloc(lower);
	free(lower);

	TextBuffer out;
	init_buffer(&out);
	size_t length = strlen(compact);
	size_t index = 0;
	while (index < length) {
		if (compact[index] >= '0' && compact[index] <= '9') {
			append_char(&out, '#');
			while (index < length && compact[index] >= '0' && compact[index] <= '9') {
				index++;
			}
		} else {
			append_char(&out, compact[index]);
			index++;
		}
	}
	free(compact);
	return detach_buffer(&out);
}

static bool is_keep_hyphen_prefix(const char *prefix) {
	size_t count = sizeof(KEEP_HYPHEN_PREFIXES) / sizeof(KEEP_HYPHEN_PREFIXES[0]);
	for (size_t index = 0; index < count; ++index) {
		if (strcmp(KEEP_HYPHEN_PREFIXES[index], prefix) == 0) {
			return true;
		}
	}
	return false;
}

static char *dehyphenate_join(const char *previous, const char *following) {
	char *left = duplicate_string(previous ? previous : "");
	char *right = duplicate_string(following ? following : "");
	trim_inplace(left);
	trim_inplace(right);

	/* Match Python rstrip() on left and lstrip() on right: preserve leading on left, trailing on right */
	free(left);
	free(right);
	left = duplicate_string(previous ? previous : "");
	{
		size_t llen = strlen(left);
		while (llen > 0) {
			size_t probe = llen - 1;
			while (probe > 0 && (((unsigned char)left[probe] & 0xC0) == 0x80)) {
				probe--;
			}
			uint32_t cp = 0;
			decode_utf8_point(left, llen, probe, &cp);
			if (!is_unicode_space(cp)) {
				break;
			}
			llen = probe;
		}
		left[llen] = '\0';
	}
	{
		const char *scan = following ? following : "";
		size_t flen = strlen(scan);
		size_t off = 0;
		while (off < flen) {
			uint32_t cp = 0;
			int step = decode_utf8_point(scan, flen, off, &cp);
			if (!is_unicode_space(cp)) {
				break;
			}
			off += (size_t)(step > 0 ? step : 1);
		}
		right = duplicate_string(scan + off);
	}

	if (!*left) {
		free(left);
		return right;
	}
	if (!*right) {
		free(right);
		return left;
	}

	size_t left_len = strlen(left);
	uint32_t right_first = 0;
	decode_utf8_point(right, strlen(right), 0, &right_first);

	if (left_len >= 2 && left[left_len - 1] == '-') {
		size_t prev_pos = left_len - 2;
		while (prev_pos > 0 && (((unsigned char)left[prev_pos] & 0xC0) == 0x80)) {
			prev_pos--;
		}
		uint32_t prev_cp = 0;
		decode_utf8_point(left, left_len - 1, prev_pos, &prev_cp);
		if (is_unicode_alpha(prev_cp)) {
			/* Find last whitespace-delimited token */
			size_t tok_start = left_len;
			size_t scan = 0;
			size_t last_ws_end = 0;
			while (scan < left_len) {
				uint32_t cp = 0;
				int step = decode_utf8_point(left, left_len, scan, &cp);
				if (is_unicode_space(cp)) {
					last_ws_end = scan + (size_t)step;
				}
				scan += (size_t)(step > 0 ? step : 1);
			}
			tok_start = last_ws_end;
			int hyphen_count = 0;
			for (size_t pos = tok_start; pos < left_len; ++pos) {
				if (left[pos] == '-') {
					hyphen_count++;
				}
			}
			bool right_case = is_unicode_lower(right_first) || is_unicode_upper(right_first);
			if (hyphen_count >= 2 && right_case) {
				TextBuffer out;
				init_buffer(&out);
				append_string(&out, left);
				append_string(&out, right);
				free(left);
				free(right);
				return detach_buffer(&out);
			}
			size_t alpha_end = left_len - 1;
			size_t alpha_start = alpha_end;
			while (alpha_start > 0 && isalpha((unsigned char)left[alpha_start - 1])) {
				alpha_start--;
			}
			size_t prefix_len = alpha_end - alpha_start;
			char prefix_buf[128];
			bool in_keep = false;
			if (prefix_len > 0 && prefix_len < sizeof(prefix_buf)) {
				for (size_t k = 0; k < prefix_len; ++k) {
					prefix_buf[k] = (char)tolower((unsigned char)left[alpha_start + k]);
				}
				prefix_buf[prefix_len] = '\0';
				in_keep = is_keep_hyphen_prefix(prefix_buf);
			}
			if (is_unicode_lower(right_first) && !in_keep && prefix_len >= 2) {
				TextBuffer out;
				init_buffer(&out);
				append_bytes(&out, left, left_len - 1);
				append_string(&out, right);
				free(left);
				free(right);
				return detach_buffer(&out);
			}
			if (right_case) {
				TextBuffer out;
				init_buffer(&out);
				append_string(&out, left);
				append_string(&out, right);
				free(left);
				free(right);
				return detach_buffer(&out);
			}
		}
	}

	TextBuffer out;
	init_buffer(&out);
	append_string(&out, left);
	append_char(&out, ' ');
	append_string(&out, right);
	free(left);
	free(right);
	return detach_buffer(&out);
}

static bool match_page_number(const char *text) {
	if (!text) {
		return false;
	}
	const char *p = text;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	if (fz_strncasecmp(p, "page", 4) == 0) {
		p += 4;
		while (*p == ' ' || *p == '\t') {
			p++;
		}
	}
	if (!isdigit((unsigned char)*p)) {
		return false;
	}
	while (isdigit((unsigned char)*p)) {
		p++;
	}
	const char *after_num = p;
	while (*p == ' ' || *p == '\t') {
		p++;
	}
	if (*p == '/' || fz_strncasecmp(p, "of", 2) == 0) {
		if (*p == '/') {
			p++;
		} else {
			p += 2;
		}
		while (*p == ' ' || *p == '\t') {
			p++;
		}
		if (!isdigit((unsigned char)*p)) {
			return false;
		}
		while (isdigit((unsigned char)*p)) {
			p++;
		}
	} else {
		p = after_num;
	}
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	return *p == '\0';
}

static bool match_roman_page(const char *text) {
	if (!text) {
		return false;
	}
	const char *p = text;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	int count = 0;
	while (*p && strchr("ivxlcdmIVXLCDM", *p) != NULL) {
		count++;
		p++;
	}
	if (count < 1 || count > 6) {
		return false;
	}
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	return *p == '\0';
}

static bool search_toc_leader(const char *text) {
	if (!text) {
		return false;
	}
	size_t len = strlen(text);
	size_t end = len;
	while (end > 0 && isspace((unsigned char)text[end - 1])) {
		end--;
	}
	if (end == 0 || !isdigit((unsigned char)text[end - 1])) {
		return false;
	}
	while (end > 0 && isdigit((unsigned char)text[end - 1])) {
		end--;
	}
	while (end > 0 && isspace((unsigned char)text[end - 1])) {
		end--;
	}
	/* Check for 4+ dots "...." */
	if (end >= 4 && memcmp(text + end - 4, "....", 4) == 0) {
		return true;
	}
	/* Check for 4+ spaced dots ". . . . " (trailing space already trimmed, so check ending with ".") */
	size_t pos = end;
	int spaced_dots = 0;
	if (pos >= 7 && text[pos - 1] == '.') {
		/* Match (?:\.\s){4,}\s*\d+\s*$ -> i.e. ". " repeated 4 times before optional whitespace + digits */
		/* Wait: if whitespace after the 4th dot was consumed by \s*, end points at '.' or after '. ' */
		/* Check backwards for ". <ws>" or "." pairs */
		size_t probe = end;
		while (probe > 0) {
			if (text[probe - 1] == '.' && probe + 1 <= len && isspace((unsigned char)text[probe])) {
				spaced_dots++;
				probe--;
				if (probe > 0 && isspace((unsigned char)text[probe - 1])) {
					probe--;
				} else if (spaced_dots < 4) {
					break;
				}
			} else {
				break;
			}
		}
		if (spaced_dots >= 4) {
			return true;
		}
	}
	return false;
}

static bool match_watermark(const char *text) {
	if (!text) {
		return false;
	}
	const char *p = text;
	while (*p && isspace((unsigned char)*p)) {
		p++;
	}
	size_t len = strlen(p);
	while (len > 0 && isspace((unsigned char)p[len - 1])) {
		len--;
	}
	if (len == 0) {
		return false;
	}
	static const char *const exact_phrases[] = {
		"confidential", "draft", "do not distribute", "internal use only"
	};
	for (size_t i = 0; i < 4; ++i) {
		size_t plen = strlen(exact_phrases[i]);
		if (len == plen && fz_strncasecmp(p, exact_phrases[i], plen) == 0) {
			return true;
		}
	}
	if ((len == 19 || (len == 20 && p[19] == '.')) &&
	    fz_strncasecmp(p, "all rights reserved", 19) == 0) {
		return true;
	}
	if (len >= 15 && fz_strncasecmp(p, "downloaded from", 15) == 0 &&
	    (len == 15 || !isalnum((unsigned char)p[15]))) {
		return true;
	}
	static const char *const prefix_phrases[] = {
		"accept all cookies", "we use cookies", "cookie policy", "privacy policy"
	};
	for (size_t i = 0; i < 4; ++i) {
		size_t plen = strlen(prefix_phrases[i]);
		if (len >= plen && fz_strncasecmp(p, prefix_phrases[i], plen) == 0) {
			return true;
		}
	}
	if (len >= 13 && fz_strncasecmp(p, "copyright", 9) == 0) {
		size_t pos = 9;
		while (pos < len && isspace((unsigned char)p[pos])) {
			pos++;
		}
		if (pos + 2 <= len && (unsigned char)p[pos] == 0xC2 && (unsigned char)p[pos + 1] == 0xA9) {
			pos += 2;
		} else if (pos + 3 <= len && fz_strncasecmp(p + pos, "(c)", 3) == 0) {
			pos += 3;
		}
		while (pos < len && isspace((unsigned char)p[pos])) {
			pos++;
		}
		if (pos + 4 <= len && isdigit((unsigned char)p[pos]) &&
		    isdigit((unsigned char)p[pos + 1]) && isdigit((unsigned char)p[pos + 2]) &&
		    isdigit((unsigned char)p[pos + 3])) {
			return true;
		}
	}
	return false;
}

static bool search_repeated_run(const char *text) {
	if (!text) {
		return false;
	}
	for (const char *p = text; *p; ++p) {
		char ch = *p;
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '.' ||
		    ch == '_' || ch == '=' || ch == '~' || ch == '*' || ch == '#') {
			int run = 1;
			while (p[run] == ch) {
				run++;
				if (run >= 8) {
					return true;
				}
			}
		}
	}
	return false;
}

static bool match_list_item(const char *text) {
	if (!text) {
		return false;
	}
	size_t len = strlen(text);
	size_t off = 0;
	while (off < len) {
		uint32_t cp = 0;
		int step = decode_utf8_point(text, len, off, &cp);
		if (!is_unicode_space(cp)) {
			break;
		}
		off += (size_t)(step > 0 ? step : 1);
	}
	if (off >= len) {
		return false;
	}
	uint32_t first = 0;
	int step = decode_utf8_point(text, len, off, &first);
	if (first == '-' || first == '*' || first == 0x2022 || first == 0x25CF ||
	    first == 0x25AA || first == 0x25E6) {
		off += (size_t)step;
	} else if (first >= '0' && first <= '9') {
		while (off < len && text[off] >= '0' && text[off] <= '9') {
			off++;
		}
		if (off >= len || (text[off] != '.' && text[off] != ')')) {
			return false;
		}
		off++;
	} else if (first == '(') {
		off++;
		int inner = 0;
		while (off < len && isalnum((unsigned char)text[off])) {
			inner++;
			off++;
		}
		if (inner < 1 || inner > 3 || off >= len || text[off] != ')') {
			return false;
		}
		off++;
	} else {
		return false;
	}
	if (off >= len) {
		return false;
	}
	uint32_t after = 0;
	decode_utf8_point(text, len, off, &after);
	return is_unicode_space(after);
}

static bool match_caption(const char *text) {
	if (!text) {
		return false;
	}
	static const char *const prefixes[] = {
		"Figure", "Fig", "Diagram", "Chart", "Scheme", "Plate", "Table", "Listing"
	};
	const char *after = NULL;
	for (size_t i = 0; i < 8; ++i) {
		size_t plen = strlen(prefixes[i]);
		if (fz_strncasecmp(text, prefixes[i], plen) == 0) {
			after = text + plen;
			if (i == 1 && *after == '.') {
				after++;
			}
			break;
		}
	}
	if (!after) {
		return false;
	}
	while (*after && isspace((unsigned char)*after)) {
		after++;
	}
	return isdigit((unsigned char)*after) != 0;
}

static bool match_reference_heading(const char *text) {
	if (!text) {
		return false;
	}
	int hashes = 0;
	while (text[hashes] == '#') {
		hashes++;
	}
	if (hashes < 1 || hashes > 3) {
		return false;
	}
	const char *p = text + hashes;
	while (*p && isspace((unsigned char)*p)) {
		p++;
	}
	size_t len = strlen(p);
	while (len > 0 && isspace((unsigned char)p[len - 1])) {
		len--;
	}
	return (len == 10 && fz_strncasecmp(p, "references", 10) == 0) ||
	       (len == 12 && fz_strncasecmp(p, "bibliography", 12) == 0) ||
	       (len == 11 && fz_strncasecmp(p, "works cited", 11) == 0);
}

static bool is_word_boundary_char(char ch) {
	return isalnum((unsigned char)ch) || ch == '_';
}

static bool search_citation_line(const char *text) {
	if (!text) {
		return false;
	}
	const char *p = text;
	while (*p && isspace((unsigned char)*p)) {
		p++;
	}
	if (*p == '[' && isdigit((unsigned char)p[1])) {
		const char *q = p + 1;
		while (isdigit((unsigned char)*q)) {
			q++;
		}
		if (*q == ']') {
			return true;
		}
	}
	size_t len = strlen(text);
	for (size_t i = 0; i < len; ++i) {
		bool left_boundary = (i == 0 || !is_word_boundary_char(text[i - 1]));
		if (!left_boundary) {
			continue;
		}
		if (i + 4 <= len && fz_strncasecmp(text + i, "doi:", 4) == 0) {
			return true;
		}
		if (i + 6 <= len && fz_strncasecmp(text + i, "arXiv:", 6) == 0) {
			return true;
		}
		if (i + 5 <= len && fz_strncasecmp(text + i, "et", 2) == 0 &&
		    isspace((unsigned char)text[i + 2])) {
			size_t q = i + 2;
			while (q < len && isspace((unsigned char)text[q])) {
				q++;
			}
			if (q + 3 <= len && fz_strncasecmp(text + q, "al.", 3) == 0) {
				return true;
			}
		}
		if (i + 4 <= len &&
		    ((text[i] == '1' && text[i + 1] == '9') ||
		     (text[i] == '2' && text[i + 1] == '0')) &&
		    isdigit((unsigned char)text[i + 2]) && isdigit((unsigned char)text[i + 3])) {
			size_t end = i + 4;
			if (end < len && isalpha((unsigned char)text[end])) {
				end++;
			}
			if (end == len || !is_word_boundary_char(text[end])) {
				return true;
			}
		}
	}
	return false;
}

static bool is_url_token_char(char ch) {
	return isalnum((unsigned char)ch) || strchr("_./\\:-", ch) != NULL;
}

static bool match_url_token(const char *text) {
	if (!text || !*text) {
		return false;
	}
	if (starts_with_str(text, "http://") || starts_with_str(text, "https://") ||
	    starts_with_str(text, "www.") || starts_with_str(text, "ftp://")) {
		return true;
	}
	size_t i = 0;
	while (text[i] && is_url_token_char(text[i]) && text[i] != '/') {
		i++;
	}
	if (i > 0 && text[i] == '/' && text[i + 1] && is_url_token_char(text[i + 1])) {
		return true;
	}
	return false;
}

static bool is_monospace_span(const char *font_name, int flags) {
	if (flags & 8) {
		return true;
	}
	if (!font_name || !*font_name) {
		return false;
	}
	size_t count = sizeof(MONOSPACE_HINTS) / sizeof(MONOSPACE_HINTS[0]);
	for (size_t index = 0; index < count; ++index) {
		if (contains_ignore_case(font_name, MONOSPACE_HINTS[index])) {
			return true;
		}
	}
	return false;
}

static bool is_math_display(const char *text) {
	if (!text) {
		return false;
	}
	char *stripped = duplicate_string(text);
	trim_inplace(stripped);
	size_t byte_len = strlen(stripped);
	size_t cp_len = count_utf8_points(stripped, byte_len);
	if (cp_len == 0 || cp_len > 220 || starts_with_str(stripped, "```") ||
	    starts_with_str(stripped, "|") || starts_with_str(stripped, "#") ||
	    starts_with_str(stripped, "![") || starts_with_str(stripped, "$$") ||
	    match_url_token(stripped)) {
		free(stripped);
		return false;
	}
	int math_hits = 0;
	int alpha_count = 0;
	size_t offset = 0;
	while (offset < byte_len) {
		uint32_t code = 0;
		int step = decode_utf8_point(stripped, byte_len, offset, &code);
		offset += (size_t)(step > 0 ? step : 1);
		if (is_math_operator_cp(code)) {
			math_hits++;
		}
		if (is_unicode_alpha(code)) {
			alpha_count++;
		}
	}
	if (math_hits < 2) {
		free(stripped);
		return false;
	}

	/* Count words containing any alpha that have >= 5 ASCII letters */
	int long_prose_words = 0;
	offset = 0;
	while (offset < byte_len) {
		while (offset < byte_len) {
			uint32_t cp = 0;
			int step = decode_utf8_point(stripped, byte_len, offset, &cp);
			if (!is_unicode_space(cp)) {
				break;
			}
			offset += (size_t)(step > 0 ? step : 1);
		}
		if (offset >= byte_len) {
			break;
		}
		bool has_alpha = false;
		int ascii_alpha = 0;
		while (offset < byte_len) {
			uint32_t cp = 0;
			int step = decode_utf8_point(stripped, byte_len, offset, &cp);
			if (is_unicode_space(cp)) {
				break;
			}
			if (is_unicode_alpha(cp)) {
				has_alpha = true;
			}
			if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z')) {
				ascii_alpha++;
			}
			offset += (size_t)(step > 0 ? step : 1);
		}
		if (has_alpha && ascii_alpha >= 5) {
			long_prose_words++;
		}
	}
	free(stripped);
	if (long_prose_words >= 5) {
		return false;
	}
	return ((double)math_hits / (double)(alpha_count > 1 ? alpha_count : 1)) >= 0.18;
}

static char *clean_prose_line(const char *line) {
	char *norm = normalize_unicode(line);
	char *text = collapse_whitespace_alloc(norm);
	free(norm);
	if (!*text || match_page_number(text) || search_toc_leader(text) ||
	    match_watermark(text) || search_repeated_run(text)) {
		free(text);
		return duplicate_string("");
	}

	TextBuffer out;
	init_buffer(&out);
	size_t byte_len = strlen(text);
	size_t offset = 0;
	while (offset < byte_len) {
		while (offset < byte_len && text[offset] == ' ') {
			offset++;
		}
		if (offset >= byte_len) {
			break;
		}
		size_t tok_start = offset;
		while (offset < byte_len && text[offset] != ' ') {
			offset++;
		}
		size_t tok_len = offset - tok_start;
		char *token = duplicate_range(text + tok_start, tok_len);
		size_t cp_len = count_utf8_points(token, tok_len);
		if (cp_len <= 45 || match_url_token(token)) {
			if (out.length > 0) {
				append_char(&out, ' ');
			}
			append_bytes(&out, token, tok_len);
		}
		free(token);
	}
	free(text);
	return detach_buffer(&out);
}

/* ========================================================================= */
/* Section 6: Layout Geometry & Markdown Tables                              */
/* ========================================================================= */

static double rectangle_overlap_ratio(BoxRect inner, BoxRect outer) {
	double ix0 = fmax(inner.x0, outer.x0);
	double iy0 = fmax(inner.y0, outer.y0);
	double ix1 = fmin(inner.x1, outer.x1);
	double iy1 = fmin(inner.y1, outer.y1);
	if (ix1 <= ix0 || iy1 <= iy0) {
		return 0.0;
	}
	double inner_area = fmax((inner.x1 - inner.x0) * (inner.y1 - inner.y0), 1e-6);
	return ((ix1 - ix0) * (iy1 - iy0)) / inner_area;
}

static int compare_page_item_y(const void *left, const void *right) {
	const PageItem *a = (const PageItem *)left;
	const PageItem *b = (const PageItem *)right;
	if (a->bbox.y0 < b->bbox.y0) {
		return -1;
	}
	if (a->bbox.y0 > b->bbox.y0) {
		return 1;
	}
	if (a->bbox.x0 < b->bbox.x0) {
		return -1;
	}
	if (a->bbox.x0 > b->bbox.x0) {
		return 1;
	}
	return 0;
}

typedef struct {
	double x0, x1;
} XInterval;

static int compare_interval_x(const void *left, const void *right) {
	const XInterval *a = (const XInterval *)left;
	const XInterval *b = (const XInterval *)right;
	if (a->x0 < b->x0) {
		return -1;
	}
	if (a->x0 > b->x0) {
		return 1;
	}
	if (a->x1 < b->x1) {
		return -1;
	}
	if (a->x1 > b->x1) {
		return 1;
	}
	return 0;
}

static void sort_column_band(PageItem *items, size_t count, double page_width) {
	if (count <= 1) {
		return;
	}
	double min_x = items[0].bbox.x0;
	double max_x = items[0].bbox.x1;
	for (size_t i = 1; i < count; ++i) {
		if (items[i].bbox.x0 < min_x) {
			min_x = items[i].bbox.x0;
		}
		if (items[i].bbox.x1 > max_x) {
			max_x = items[i].bbox.x1;
		}
	}
	double span = max_x - min_x;
	if (span < 0.35 * page_width) {
		qsort(items, count, sizeof(PageItem), compare_page_item_y);
		return;
	}

	XInterval *intervals = (XInterval *)allocate_memory(count * sizeof(XInterval));
	for (size_t i = 0; i < count; ++i) {
		intervals[i].x0 = items[i].bbox.x0;
		intervals[i].x1 = items[i].bbox.x1;
	}
	qsort(intervals, count, sizeof(XInterval), compare_interval_x);

	XInterval *merged = (XInterval *)allocate_memory(count * sizeof(XInterval));
	size_t merged_count = 0;
	for (size_t i = 0; i < count; ++i) {
		if (merged_count == 0 || intervals[i].x0 > merged[merged_count - 1].x1 + 2.0) {
			merged[merged_count++] = intervals[i];
		} else if (intervals[i].x1 > merged[merged_count - 1].x1) {
			merged[merged_count - 1].x1 = intervals[i].x1;
		}
	}
	free(intervals);

	double best_gap = 0.0;
	bool has_split = false;
	double split_x = 0.0;
	double min_gutter = fmax(6.0, 0.015 * page_width);
	for (size_t i = 0; i + 1 < merged_count; ++i) {
		double gap = merged[i + 1].x0 - merged[i].x1;
		double mid = 0.5 * (merged[i].x1 + merged[i + 1].x0);
		if (gap > best_gap && gap >= min_gutter && mid >= min_x + 0.15 * span &&
		    mid <= max_x - 0.15 * span) {
			best_gap = gap;
			split_x = mid;
			has_split = true;
		}
	}
	free(merged);

	if (!has_split) {
		qsort(items, count, sizeof(PageItem), compare_page_item_y);
		return;
	}

	PageItem *left = (PageItem *)allocate_memory(count * sizeof(PageItem));
	PageItem *right = (PageItem *)allocate_memory(count * sizeof(PageItem));
	size_t left_count = 0;
	size_t right_count = 0;
	double left_top = 1e30, left_bottom = -1e30;
	double right_top = 1e30, right_bottom = -1e30;

	for (size_t i = 0; i < count; ++i) {
		double cx = 0.5 * (items[i].bbox.x0 + items[i].bbox.x1);
		if (cx < split_x) {
			left[left_count++] = items[i];
			if (items[i].bbox.y0 < left_top) {
				left_top = items[i].bbox.y0;
			}
			if (items[i].bbox.y1 > left_bottom) {
				left_bottom = items[i].bbox.y1;
			}
		} else {
			right[right_count++] = items[i];
			if (items[i].bbox.y0 < right_top) {
				right_top = items[i].bbox.y0;
			}
			if (items[i].bbox.y1 > right_bottom) {
				right_bottom = items[i].bbox.y1;
			}
		}
	}

	if (left_count == 0 || right_count == 0 ||
	    fmin(left_bottom, right_bottom) <= fmax(left_top, right_top)) {
		free(left);
		free(right);
		qsort(items, count, sizeof(PageItem), compare_page_item_y);
		return;
	}

	sort_column_band(left, left_count, page_width);
	sort_column_band(right, right_count, page_width);
	memcpy(items, left, left_count * sizeof(PageItem));
	memcpy(items + left_count, right, right_count * sizeof(PageItem));
	free(left);
	free(right);
}

static void sort_reading_order(PageItemList *list, double page_width) {
	if (list->count <= 1) {
		return;
	}
	qsort(list->items, list->count, sizeof(PageItem), compare_page_item_y);
	double wide_threshold = 0.62 * page_width;
	size_t band_start = 0;
	for (size_t i = 0; i < list->count; ++i) {
		double width = list->items[i].bbox.x1 - list->items[i].bbox.x0;
		if (width >= wide_threshold) {
			if (i > band_start) {
				sort_column_band(list->items + band_start, i - band_start, page_width);
			}
			band_start = i + 1;
		}
	}
	if (list->count > band_start) {
		sort_column_band(list->items + band_start, list->count - band_start, page_width);
	}
}

typedef struct {
	TextList *rows;
	size_t row_count;
	size_t row_capacity;
} TableRows;

static void init_table_rows(TableRows *table) {
	table->rows = NULL;
	table->row_count = 0;
	table->row_capacity = 0;
}

static void push_table_row(TableRows *table, TextList row) {
	if (table->row_count == table->row_capacity) {
		table->row_capacity = table->row_capacity ? table->row_capacity * 2 : 8;
		table->rows =
			(TextList *)reallocate_memory(table->rows, table->row_capacity * sizeof(TextList));
	}
	table->rows[table->row_count++] = row;
}

static void free_table_rows(TableRows *table) {
	for (size_t i = 0; i < table->row_count; ++i) {
		free_text_list(&table->rows[i]);
	}
	free(table->rows);
	table->rows = NULL;
	table->row_count = 0;
	table->row_capacity = 0;
}

static bool is_valid_table(const TableRows *table, BoxRect bbox, double page_height) {
	size_t non_empty_rows = 0;
	size_t cell_count = 0;
	size_t cell_len_sum = 0;

	for (size_t r = 0; r < table->row_count; ++r) {
		bool row_has_text = false;
		for (size_t c = 0; c < table->rows[r].count; ++c) {
			const char *cell = table->rows[r].items[c];
			if (!cell) {
				continue;
			}
			char *trimmed = duplicate_string(cell);
			trim_inplace(trimmed);
			if (*trimmed) {
				row_has_text = true;
				cell_count++;
				cell_len_sum += count_utf8_points(trimmed, strlen(trimmed));
			}
			free(trimmed);
		}
		if (row_has_text) {
			non_empty_rows++;
		}
	}
	if (non_empty_rows < 2) {
		return false;
	}
	if ((bbox.y1 - bbox.y0) > 0.85 * page_height && non_empty_rows <= 2) {
		return false;
	}
	if (cell_count == 0) {
		return false;
	}
	return ((double)cell_len_sum / (double)cell_count) <= 250.0;
}

static char *escape_table_cell(const char *raw_cell) {
	char *norm = normalize_unicode(raw_cell ? raw_cell : "");
	char *compact = collapse_whitespace_alloc(norm);
	free(norm);
	TextBuffer out;
	init_buffer(&out);
	for (const char *p = compact; *p; ++p) {
		if (*p == '|') {
			append_string(&out, "\\|");
		} else {
			append_char(&out, *p);
		}
	}
	free(compact);
	return detach_buffer(&out);
}

static char *format_markdown_table(const TableRows *table) {
	TableRows cleaned;
	init_table_rows(&cleaned);
	size_t col_count = 0;

	for (size_t r = 0; r < table->row_count; ++r) {
		TextList row;
		init_text_list(&row);
		bool any_non_empty = false;
		for (size_t c = 0; c < table->rows[r].count; ++c) {
			char *cell = escape_table_cell(table->rows[r].items[c]);
			if (*cell) {
				any_non_empty = true;
			}
			push_text_owned(&row, cell);
		}
		if (any_non_empty) {
			if (row.count > col_count) {
				col_count = row.count;
			}
			push_table_row(&cleaned, row);
		} else {
			free_text_list(&row);
		}
	}

	if (cleaned.row_count < 2 || col_count < 2) {
		free_table_rows(&cleaned);
		return duplicate_string("");
	}

	bool *active = (bool *)allocate_zeroed(col_count, sizeof(bool));
	size_t active_count = 0;
	for (size_t c = 0; c < col_count; ++c) {
		for (size_t r = 0; r < cleaned.row_count; ++r) {
			if (c < cleaned.rows[r].count && cleaned.rows[r].items[c][0] != '\0') {
				active[c] = true;
				active_count++;
				break;
			}
		}
	}
	if (active_count < 2) {
		free(active);
		free_table_rows(&cleaned);
		return duplicate_string("");
	}

	TextBuffer out;
	init_buffer(&out);
	/* Header row */
	append_string(&out, "| ");
	bool first_col = true;
	for (size_t c = 0; c < col_count; ++c) {
		if (!active[c]) {
			continue;
		}
		if (!first_col) {
			append_string(&out, " | ");
		}
		append_string(&out, c < cleaned.rows[0].count ? cleaned.rows[0].items[c] : "");
		first_col = false;
	}
	append_string(&out, " |\n| ");
	first_col = true;
	for (size_t c = 0; c < col_count; ++c) {
		if (!active[c]) {
			continue;
		}
		if (!first_col) {
			append_string(&out, " | ");
		}
		append_string(&out, "---");
		first_col = false;
	}
	append_string(&out, " |");

	for (size_t r = 1; r < cleaned.row_count; ++r) {
		append_string(&out, "\n| ");
		first_col = true;
		for (size_t c = 0; c < col_count; ++c) {
			if (!active[c]) {
				continue;
			}
			if (!first_col) {
				append_string(&out, " | ");
			}
			append_string(&out, c < cleaned.rows[r].count ? cleaned.rows[r].items[c] : "");
			first_col = false;
		}
		append_string(&out, " |");
	}

	free(active);
	free_table_rows(&cleaned);
	return detach_buffer(&out);
}

typedef struct {
	BoxRect bbox;
	char *text;
} CaptionCandidate;

static char *find_nearest_caption(BoxRect fig_bbox, const CaptionCandidate *candidates,
                                  size_t count) {
	double best_dist = 85.0;
	char *best_caption = NULL;

	for (size_t index = 0; index < count; ++index) {
		const char *text = candidates[index].text;
		if (!text || !*text) {
			continue;
		}
		const char *newline = strchr(text, '\n');
		size_t first_len = newline ? (size_t)(newline - text) : strlen(text);
		char *first_line = duplicate_range(text, first_len);
		trim_inplace(first_line);
		bool is_cap = *first_line && match_caption(first_line);
		free(first_line);
		if (!is_cap) {
			continue;
		}
		BoxRect tb = candidates[index].bbox;
		double horiz_overlap = fmin(fig_bbox.x1, tb.x1) - fmax(fig_bbox.x0, tb.x0);
		if (horiz_overlap < -40.0) {
			continue;
		}
		double dist = 0.0;
		if (tb.y0 >= fig_bbox.y1 - 10.0) {
			dist = fabs(tb.y0 - fig_bbox.y1);
		} else if (tb.y1 <= fig_bbox.y0 + 10.0) {
			dist = fabs(fig_bbox.y0 - tb.y1);
		} else {
			continue;
		}
		if (dist < best_dist) {
			best_dist = dist;
			char *compact = collapse_whitespace_alloc(text);
			TextBuffer truncated;
			init_buffer(&truncated);
			size_t len = strlen(compact);
			size_t off = 0;
			int cp_count = 0;
			while (off < len && cp_count < 180) {
				uint32_t cp = 0;
				int step = decode_utf8_point(compact, len, off, &cp);
				off += (size_t)(step > 0 ? step : 1);
				if (cp == '[') {
					cp = '(';
				} else if (cp == ']') {
					cp = ')';
				}
				append_utf8_point(&truncated, cp);
				cp_count++;
			}
			free(compact);
			free(best_caption);
			best_caption = detach_buffer(&truncated);
		}
	}
	return best_caption ? best_caption : duplicate_string("");
}

/* ========================================================================= */
/* Section 7: Vision Pipeline & PNG Filtering                                */
/* ========================================================================= */

static double compute_shannon_entropy(const uint8_t *gray, size_t pixel_count) {
	if (pixel_count == 0) {
		return 0.0;
	}
	size_t hist[256] = {0};
	for (size_t i = 0; i < pixel_count; ++i) {
		hist[gray[i]]++;
	}
	double entropy = 0.0;
	double total = (double)pixel_count;
	for (int bin = 0; bin < 256; ++bin) {
		if (hist[bin] > 0) {
			double prob = (double)hist[bin] / total;
			entropy -= prob * log2(prob);
		}
	}
	return entropy;
}

static double compute_stddev(const uint8_t *gray, size_t pixel_count) {
	if (pixel_count == 0) {
		return 0.0;
	}
	double sum = 0.0;
	double sum_sq = 0.0;
	for (size_t i = 0; i < pixel_count; ++i) {
		double v = (double)gray[i];
		sum += v;
		sum_sq += v * v;
	}
	double mean = sum / (double)pixel_count;
	double var = (sum_sq / (double)pixel_count) - mean * mean;
	return var > 0.0 ? sqrt(var) : 0.0;
}

static double triangle_kernel(double x) {
	x = fabs(x);
	return x < 1.0 ? (1.0 - x) : 0.0;
}

static uint64_t compute_difference_hash(const uint8_t *gray, int width, int height) {
	double scale_x = (double)width / 9.0;
	double scale_y = (double)height / 8.0;
	double filter_scale_x = scale_x > 1.0 ? scale_x : 1.0;
	double filter_scale_y = scale_y > 1.0 ? scale_y : 1.0;

	double *temp = (double *)allocate_memory((size_t)height * 9 * sizeof(double));
	for (int dx = 0; dx < 9; ++dx) {
		double center = ((double)dx + 0.5) * scale_x - 0.5;
		int xmin = (int)floor(center - filter_scale_x);
		int xmax = (int)ceil(center + filter_scale_x);
		if (xmin < 0) {
			xmin = 0;
		}
		if (xmax >= width) {
			xmax = width - 1;
		}
		int klen = xmax - xmin + 1;
		double *weights = (double *)allocate_memory((size_t)klen * sizeof(double));
		double wsum = 0.0;
		for (int sx = xmin; sx <= xmax; ++sx) {
			double w = triangle_kernel(((double)sx - center) / filter_scale_x);
			weights[sx - xmin] = w;
			wsum += w;
		}
		if (wsum != 0.0) {
			for (int k = 0; k < klen; ++k) {
				weights[k] /= wsum;
			}
		}
		for (int y = 0; y < height; ++y) {
			const uint8_t *row_ptr = gray + (size_t)y * (size_t)width;
			double acc = 0.0;
			for (int sx = xmin; sx <= xmax; ++sx) {
				acc += (double)row_ptr[sx] * weights[sx - xmin];
			}
			temp[(size_t)y * 9 + (size_t)dx] = acc;
		}
		free(weights);
	}

	uint8_t sampled[8 * 9];
	for (int dy = 0; dy < 8; ++dy) {
		double center = ((double)dy + 0.5) * scale_y - 0.5;
		int ymin = (int)floor(center - filter_scale_y);
		int ymax = (int)ceil(center + filter_scale_y);
		if (ymin < 0) {
			ymin = 0;
		}
		if (ymax >= height) {
			ymax = height - 1;
		}
		int klen = ymax - ymin + 1;
		double *weights = (double *)allocate_memory((size_t)klen * sizeof(double));
		double wsum = 0.0;
		for (int sy = ymin; sy <= ymax; ++sy) {
			double w = triangle_kernel(((double)sy - center) / filter_scale_y);
			weights[sy - ymin] = w;
			wsum += w;
		}
		if (wsum != 0.0) {
			for (int k = 0; k < klen; ++k) {
				weights[k] /= wsum;
			}
		}
		for (int x = 0; x < 9; ++x) {
			double acc = 0.0;
			for (int sy = ymin; sy <= ymax; ++sy) {
				acc += temp[(size_t)sy * 9 + (size_t)x] * weights[sy - ymin];
			}
			int iv = (int)llround(acc);
			sampled[dy * 9 + x] = (uint8_t)(iv < 0 ? 0 : (iv > 255 ? 255 : iv));
		}
		free(weights);
	}
	free(temp);

	uint64_t bits = 0;
	for (int row = 0; row < 8; ++row) {
		int off = row * 9;
		for (int col = 0; col < 8; ++col) {
			bits = (bits << 1) |
			       (sampled[off + col] > sampled[off + col + 1] ? 1ULL : 0ULL);
		}
	}
	return bits;
}

static int hamming_distance64(uint64_t left, uint64_t right) {
	uint64_t x = left ^ right;
	int count = 0;
	while (x) {
		x &= x - 1ULL;
		count++;
	}
	return count;
}

static double lanczos3_kernel(double x) {
	const double pi = 3.14159265358979323846;
	x = fabs(x);
	if (x < 1e-8) {
		return 1.0;
	}
	if (x >= 3.0) {
		return 0.0;
	}
	double px = pi * x;
	return (sin(px) / px) * (sin(px / 3.0) / (px / 3.0));
}

static uint8_t *resize_rgb_lanczos(const uint8_t *src, int src_w, int src_h, int dst_w, int dst_h) {
	double scale_x = (double)src_w / (double)dst_w;
	double scale_y = (double)src_h / (double)dst_h;
	double filter_scale_x = scale_x > 1.0 ? scale_x : 1.0;
	double filter_scale_y = scale_y > 1.0 ? scale_y : 1.0;
	double support_x = 3.0 * filter_scale_x;
	double support_y = 3.0 * filter_scale_y;

	double *temp = (double *)allocate_memory((size_t)src_h * (size_t)dst_w * 3 * sizeof(double));
	for (int dx = 0; dx < dst_w; ++dx) {
		double center = ((double)dx + 0.5) * scale_x - 0.5;
		int xmin = (int)floor(center - support_x);
		int xmax = (int)ceil(center + support_x);
		if (xmin < 0) {
			xmin = 0;
		}
		if (xmax >= src_w) {
			xmax = src_w - 1;
		}
		int klen = xmax - xmin + 1;
		double *weights = (double *)allocate_memory((size_t)klen * sizeof(double));
		double wsum = 0.0;
		for (int sx = xmin; sx <= xmax; ++sx) {
			double w = lanczos3_kernel(((double)sx - center) / filter_scale_x);
			weights[sx - xmin] = w;
			wsum += w;
		}
		if (wsum != 0.0) {
			for (int k = 0; k < klen; ++k) {
				weights[k] /= wsum;
			}
		}
		for (int y = 0; y < src_h; ++y) {
			double acc0 = 0.0, acc1 = 0.0, acc2 = 0.0;
			const uint8_t *row_ptr = src + ((size_t)y * (size_t)src_w) * 3;
			for (int sx = xmin; sx <= xmax; ++sx) {
				double w = weights[sx - xmin];
				const uint8_t *px = row_ptr + (size_t)sx * 3;
				acc0 += px[0] * w;
				acc1 += px[1] * w;
				acc2 += px[2] * w;
			}
			double *dst_px = temp + ((size_t)y * (size_t)dst_w + (size_t)dx) * 3;
			dst_px[0] = acc0;
			dst_px[1] = acc1;
			dst_px[2] = acc2;
		}
		free(weights);
	}

	uint8_t *out = (uint8_t *)allocate_memory((size_t)dst_w * (size_t)dst_h * 3);
	for (int dy = 0; dy < dst_h; ++dy) {
		double center = ((double)dy + 0.5) * scale_y - 0.5;
		int ymin = (int)floor(center - support_y);
		int ymax = (int)ceil(center + support_y);
		if (ymin < 0) {
			ymin = 0;
		}
		if (ymax >= src_h) {
			ymax = src_h - 1;
		}
		int klen = ymax - ymin + 1;
		double *weights = (double *)allocate_memory((size_t)klen * sizeof(double));
		double wsum = 0.0;
		for (int sy = ymin; sy <= ymax; ++sy) {
			double w = lanczos3_kernel(((double)sy - center) / filter_scale_y);
			weights[sy - ymin] = w;
			wsum += w;
		}
		if (wsum != 0.0) {
			for (int k = 0; k < klen; ++k) {
				weights[k] /= wsum;
			}
		}
		for (int x = 0; x < dst_w; ++x) {
			double acc0 = 0.0, acc1 = 0.0, acc2 = 0.0;
			for (int sy = ymin; sy <= ymax; ++sy) {
				double w = weights[sy - ymin];
				const double *px = temp + ((size_t)sy * (size_t)dst_w + (size_t)x) * 3;
				acc0 += px[0] * w;
				acc1 += px[1] * w;
				acc2 += px[2] * w;
			}
			uint8_t *dst_px = out + ((size_t)dy * (size_t)dst_w + (size_t)x) * 3;
			int r = (int)llround(acc0);
			int g = (int)llround(acc1);
			int b = (int)llround(acc2);
			dst_px[0] = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
			dst_px[1] = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
			dst_px[2] = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
		}
		free(weights);
	}
	free(temp);
	return out;
}

static SavePngStatus filter_and_save_rgb(fz_context *context, const uint8_t *rgb_pixels, int width,
                                         int height, const char *target_path,
                                         Hash64List *document_dhashes,
                                         const CorpusState *corpus_state,
                                         TextCounter *image_digests, bool enable_dedup,
                                         int minimum_size) {
	if (!rgb_pixels || width < minimum_size || height < minimum_size) {
		return PNG_REJECTED;
	}
	if ((int64_t)width * (int64_t)height < MIN_IMAGE_AREA) {
		return PNG_REJECTED;
	}
	double aspect = fmax((double)width / (double)(height > 1 ? height : 1),
			     (double)height / (double)(width > 1 ? width : 1));
	if (aspect > MAX_ASPECT_RATIO) {
		return PNG_REJECTED;
	}

	size_t pixel_count = (size_t)width * (size_t)height;
	uint8_t *gray = (uint8_t *)allocate_memory(pixel_count);
	for (size_t i = 0; i < pixel_count; ++i) {
		uint32_t r = rgb_pixels[i * 3];
		uint32_t g = rgb_pixels[i * 3 + 1];
		uint32_t b = rgb_pixels[i * 3 + 2];
		gray[i] = (uint8_t)((r * 19595U + g * 38470U + b * 7471U + 32768U) >> 16);
	}

	if (compute_shannon_entropy(gray, pixel_count) < MIN_IMAGE_ENTROPY ||
	    compute_stddev(gray, pixel_count) < MIN_IMAGE_DEVIATION) {
		free(gray);
		return PNG_REJECTED;
	}

	char pixel_digest[65];
	compute_sha256_hex(rgb_pixels, pixel_count * 3, pixel_digest);
	if (has_text_key(image_digests, pixel_digest) ||
	    get_text_count(&corpus_state->image_doc_counts, pixel_digest) >= CORPUS_LOGO_DOCUMENTS) {
		free(gray);
		return PNG_REJECTED;
	}

	uint64_t dhash = compute_difference_hash(gray, width, height);
	free(gray);

	for (size_t i = 0; i < document_dhashes->count; ++i) {
		if (hamming_distance64(dhash, document_dhashes->items[i]) <= 4) {
			return PNG_REJECTED;
		}
	}
	if (enable_dedup) {
		for (size_t i = 0; i < corpus_state->corpus_dhashes.capacity; ++i) {
			if (corpus_state->corpus_dhashes.occupied[i] &&
			    hamming_distance64(dhash, corpus_state->corpus_dhashes.keys[i]) <= 2) {
				return PNG_REJECTED;
			}
		}
	}

	int final_w = width;
	int final_h = height;
	uint8_t *resized = NULL;
	const uint8_t *save_buf = rgb_pixels;
	int longest = width > height ? width : height;
	if (longest > MAX_IMAGE_EDGE) {
		double scale = (double)MAX_IMAGE_EDGE / (double)longest;
		final_w = (int)llround((double)width * scale);
		final_h = (int)llround((double)height * scale);
		if (final_w < 1) {
			final_w = 1;
		}
		if (final_h < 1) {
			final_h = 1;
		}
		resized = resize_rgb_lanczos(rgb_pixels, width, height, final_w, final_h);
		save_buf = resized;
	}

	char *png_path = replace_extension(target_path, ".png");
	fz_pixmap *out_pix = NULL;
	bool saved_ok = false;
	fz_var(out_pix);
	fz_var(saved_ok);
	fz_try(context) {
		out_pix = fz_new_pixmap_with_data(context, fz_device_rgb(context), final_w, final_h,
						  NULL, 0, final_w * 3, (unsigned char *)save_buf);
		fz_save_pixmap_as_png(context, out_pix, png_path);
		saved_ok = true;
	}
	fz_always(context) {
		if (out_pix) {
			fz_drop_pixmap(context, out_pix);
		}
	}
	fz_catch(context) {
		saved_ok = false;
	}
	free(png_path);
	free(resized);

	if (!saved_ok) {
		return PNG_DECODE_ERROR;
	}
	push_hash64(document_dhashes, dhash);
	add_text_count(image_digests, pixel_digest, 1);
	return PNG_SAVED;
}

static uint8_t *pixmap_to_rgb8(fz_context *context, fz_pixmap *pixmap, int *out_w, int *out_h) {
	*out_w = 0;
	*out_h = 0;
	if (!pixmap) {
		return NULL;
	}
	fz_pixmap *rgb_pix = NULL;
	int w = 0, h = 0;
	uint8_t *rgb = NULL;
	fz_var(rgb_pix);
	fz_var(rgb);
	fz_try(context) {
		fz_colorspace *cs = fz_pixmap_colorspace(context, pixmap);
		int keep_alpha = fz_pixmap_alpha(context, pixmap);
		if (cs == fz_device_rgb(context) && fz_pixmap_spots(context, pixmap) == 0) {
			rgb_pix = fz_keep_pixmap(context, pixmap);
		} else {
			rgb_pix = fz_convert_pixmap(context, pixmap, fz_device_rgb(context), NULL,
						    NULL, DEFAULT_COLOR_PARAMS, keep_alpha);
		}
		w = fz_pixmap_width(context, rgb_pix);
		h = fz_pixmap_height(context, rgb_pix);
		int n = fz_pixmap_components(context, rgb_pix);
		int alpha = fz_pixmap_alpha(context, rgb_pix);
		int stride = fz_pixmap_stride(context, rgb_pix);
		const unsigned char *samples = fz_pixmap_samples(context, rgb_pix);

		if (w > 0 && h > 0 && samples && (n == 3 || (n == 4 && alpha == 1))) {
			rgb = (uint8_t *)allocate_memory((size_t)w * (size_t)h * 3);
			for (int y = 0; y < h; ++y) {
				const unsigned char *src_row = samples + (size_t)y * (size_t)stride;
				uint8_t *dst_row = rgb + (size_t)y * (size_t)w * 3;
				if (alpha == 0) {
					memcpy(dst_row, src_row, (size_t)w * 3);
				} else {
					for (int x = 0; x < w; ++x) {
						/* MuPDF alpha pixmaps are premultiplied; composite on white. */
						int pr = src_row[x * 4];
						int pg = src_row[x * 4 + 1];
						int pb = src_row[x * 4 + 2];
						int a = src_row[x * 4 + 3];
						int inv = 255 - a;
						int r = pr + inv;
						int g = pg + inv;
						int b = pb + inv;
						dst_row[x * 3] = (uint8_t)(r > 255 ? 255 : r);
						dst_row[x * 3 + 1] = (uint8_t)(g > 255 ? 255 : g);
						dst_row[x * 3 + 2] = (uint8_t)(b > 255 ? 255 : b);
					}
				}
			}
		}
	}
	fz_always(context) {
		if (rgb_pix) {
			fz_drop_pixmap(context, rgb_pix);
		}
	}
	fz_catch(context) {
		free(rgb);
		rgb = NULL;
	}
	if (rgb) {
		*out_w = w;
		*out_h = h;
	}
	return rgb;
}

#ifdef _WIN32
static uint8_t *decode_with_wic(const uint8_t *raw_bytes, size_t raw_size, int *out_w, int *out_h) {
	*out_w = 0;
	*out_h = 0;
	if (!raw_bytes || raw_size == 0) {
		return NULL;
	}
	HRESULT hr_init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	IWICImagingFactory *factory = NULL;
	IWICStream *stream = NULL;
	IWICBitmapDecoder *decoder = NULL;
	IWICBitmapFrameDecode *frame = NULL;
	IWICFormatConverter *converter = NULL;
	uint8_t *rgb = NULL;

	HRESULT hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
				      &IID_IWICImagingFactory, (void **)&factory);
	if (SUCCEEDED(hr) && factory) {
		hr = IWICImagingFactory_CreateStream(factory, &stream);
	}
	if (SUCCEEDED(hr) && stream) {
		hr = IWICStream_InitializeFromMemory(stream, (WICInProcPointer)raw_bytes,
						     (DWORD)raw_size);
	}
	if (SUCCEEDED(hr) && stream) {
		hr = IWICImagingFactory_CreateDecoderFromStream(
			factory, (IStream *)stream, NULL, WICDecodeMetadataCacheOnLoad, &decoder);
	}
	if (SUCCEEDED(hr) && decoder) {
		hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
	}
	UINT w = 0, h = 0;
	if (SUCCEEDED(hr) && frame) {
		hr = IWICBitmapFrameDecode_GetSize(frame, &w, &h);
	}
	if (SUCCEEDED(hr) && w > 0 && h > 0 && w <= 32768 && h <= 32768) {
		hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
		if (SUCCEEDED(hr) && converter) {
			hr = IWICFormatConverter_Initialize(
				converter, (IWICBitmapSource *)frame,
				&GUID_WICPixelFormat32bppPRGBA, WICBitmapDitherTypeNone, NULL, 0.0,
				WICBitmapPaletteTypeCustom);
		}
		if (SUCCEEDED(hr)) {
			size_t rgba_stride = (size_t)w * 4;
			uint8_t *rgba = (uint8_t *)allocate_memory(rgba_stride * (size_t)h);
			hr = IWICFormatConverter_CopyPixels(converter, NULL, (UINT)rgba_stride,
							    (UINT)(rgba_stride * (size_t)h), rgba);
			if (SUCCEEDED(hr)) {
				rgb = (uint8_t *)allocate_memory((size_t)w * (size_t)h * 3);
				size_t pixels = (size_t)w * (size_t)h;
				for (size_t i = 0; i < pixels; ++i) {
					int pr = rgba[i * 4];
					int pg = rgba[i * 4 + 1];
					int pb = rgba[i * 4 + 2];
					int a = rgba[i * 4 + 3];
					int inv = 255 - a;
					int r = pr + inv;
					int g = pg + inv;
					int b = pb + inv;
					rgb[i * 3] = (uint8_t)(r > 255 ? 255 : r);
					rgb[i * 3 + 1] = (uint8_t)(g > 255 ? 255 : g);
					rgb[i * 3 + 2] = (uint8_t)(b > 255 ? 255 : b);
				}
				*out_w = (int)w;
				*out_h = (int)h;
			}
			free(rgba);
		}
	}

	if (converter) {
		IWICFormatConverter_Release(converter);
	}
	if (frame) {
		IWICBitmapFrameDecode_Release(frame);
	}
	if (decoder) {
		IWICBitmapDecoder_Release(decoder);
	}
	if (stream) {
		IWICStream_Release(stream);
	}
	if (factory) {
		IWICImagingFactory_Release(factory);
	}
	if (SUCCEEDED(hr_init)) {
		CoUninitialize();
	}
	return rgb;
}
#endif

static SavePngStatus save_filtered_pixmap(fz_context *context, fz_pixmap *pixmap,
                                          const char *target_path, Hash64List *document_dhashes,
                                          const CorpusState *corpus_state,
                                          TextCounter *image_digests, bool enable_dedup,
                                          int minimum_size) {
	int w = 0, h = 0;
	uint8_t *rgb = pixmap_to_rgb8(context, pixmap, &w, &h);
	if (!rgb) {
		return PNG_DECODE_ERROR;
	}
	SavePngStatus status = filter_and_save_rgb(context, rgb, w, h, target_path, document_dhashes,
						   corpus_state, image_digests, enable_dedup,
						   minimum_size);
	free(rgb);
	return status;
}

static SavePngStatus save_filtered_png(fz_context *context, const uint8_t *raw_bytes,
                                       size_t raw_size, const char *target_path,
                                       Hash64List *document_dhashes,
                                       const CorpusState *corpus_state,
                                       TextCounter *image_digests, bool enable_dedup,
                                       int minimum_size) {
	if (!raw_bytes || raw_size == 0) {
		return PNG_DECODE_ERROR;
	}
	fz_buffer *buf = NULL;
	fz_image *img = NULL;
	fz_pixmap *pix = NULL;
	uint8_t *rgb = NULL;
	int w = 0, h = 0;

	fz_var(buf);
	fz_var(img);
	fz_var(pix);
	fz_var(rgb);
	fz_try(context) {
		buf = fz_new_buffer_from_shared_data(context, raw_bytes, raw_size);
		img = fz_new_image_from_buffer(context, buf);
		pix = fz_get_pixmap_from_image(context, img, NULL, NULL, NULL, NULL);
		rgb = pixmap_to_rgb8(context, pix, &w, &h);
	}
	fz_always(context) {
		if (pix) {
			fz_drop_pixmap(context, pix);
		}
		if (img) {
			fz_drop_image(context, img);
		}
		if (buf) {
			fz_drop_buffer(context, buf);
		}
	}
	fz_catch(context) {
		free(rgb);
		rgb = NULL;
	}

#ifdef _WIN32
	if (!rgb) {
		rgb = decode_with_wic(raw_bytes, raw_size, &w, &h);
	}
#endif
	if (!rgb) {
		return PNG_DECODE_ERROR;
	}
	SavePngStatus status = filter_and_save_rgb(context, rgb, w, h, target_path, document_dhashes,
						   corpus_state, image_digests, enable_dedup,
						   minimum_size);
	free(rgb);
	return status;
}

/* ========================================================================= */
/* Section 8: PDF Extraction (Layout Dict, Tables, Images, Vectors)          */
/* ========================================================================= */

typedef struct {
	int xref;
	int smask;
} PdfImageRef;

typedef struct {
	unsigned char digest[16];
	BoxRect bbox;
} PlacedImageInfo;

typedef struct {
	double coord;  /* y for horizontal, x for vertical */
	double start;  /* x0 for horizontal, y0 for vertical */
	double end;    /* x1 for horizontal, y1 for vertical */
	bool vertical;
} TableEdge;

typedef struct {
	fz_device super;
	fz_context *context;
	BoxRect *drawing_rects;
	size_t drawing_count;
	size_t drawing_capacity;
	BoxRect *path_rects;
	size_t path_count;
	size_t path_capacity;
	TableEdge *edges;
	size_t edge_count;
	size_t edge_capacity;
	PlacedImageInfo *placed_images;
	size_t image_count;
	size_t image_capacity;
} PageCollectDevice;

static void push_box_rect(BoxRect **arr, size_t *count, size_t *cap, BoxRect rect) {
	if (*count == *cap) {
		*cap = *cap ? (*cap * 2) : 64;
		*arr = (BoxRect *)reallocate_memory(*arr, (*cap) * sizeof(BoxRect));
	}
	(*arr)[(*count)++] = rect;
}

static void push_table_edge(PageCollectDevice *dev, bool vertical, double coord, double start,
                            double end) {
	if (end < start) {
		double tmp = start;
		start = end;
		end = tmp;
	}
	if (end - start <= 0.0) {
		return;
	}
	if (dev->edge_count == dev->edge_capacity) {
		dev->edge_capacity = dev->edge_capacity ? dev->edge_capacity * 2 : 128;
		dev->edges = (TableEdge *)reallocate_memory(dev->edges,
							    dev->edge_capacity * sizeof(TableEdge));
	}
	dev->edges[dev->edge_count].coord = coord;
	dev->edges[dev->edge_count].start = start;
	dev->edges[dev->edge_count].end = end;
	dev->edges[dev->edge_count].vertical = vertical;
	dev->edge_count++;
}

typedef struct {
	PageCollectDevice *dev;
	fz_matrix ctm;
	fz_point current;
	fz_point subpath_start;
	bool has_current;
	bool has_subpath;
} PathWalkState;

static void walk_add_segment(PathWalkState *state, fz_point p1, fz_point p2) {
	double dx = fabs(p1.x - p2.x);
	double dy = fabs(p1.y - p2.y);
	if (dx <= 3.0 && dy > 0.0 && dy >= dx) {
		push_table_edge(state->dev, true, 0.5 * (p1.x + p2.x), fmin(p1.y, p2.y),
				fmax(p1.y, p2.y));
	} else if (dy <= 3.0 && dx > 0.0 && dx >= dy) {
		push_table_edge(state->dev, false, 0.5 * (p1.y + p2.y), fmin(p1.x, p2.x),
				fmax(p1.x, p2.x));
	}
}

static void walk_moveto(fz_context *ctx, void *arg, float x, float y) {
	(void)ctx;
	PathWalkState *state = (PathWalkState *)arg;
	fz_point tp = fz_transform_point_xy(x, y, state->ctm);
	state->current = tp;
	state->subpath_start = tp;
	state->has_current = true;
	state->has_subpath = true;
}

static void walk_lineto(fz_context *ctx, void *arg, float x, float y) {
	(void)ctx;
	PathWalkState *state = (PathWalkState *)arg;
	fz_point tp = fz_transform_point_xy(x, y, state->ctm);
	if (state->has_current) {
		walk_add_segment(state, state->current, tp);
	}
	state->current = tp;
	state->has_current = true;
}

static void walk_curveto(fz_context *ctx, void *arg, float x1, float y1, float x2, float y2,
                         float x3, float y3) {
	(void)ctx;
	(void)x1;
	(void)y1;
	(void)x2;
	(void)y2;
	PathWalkState *state = (PathWalkState *)arg;
	state->current = fz_transform_point_xy(x3, y3, state->ctm);
	state->has_current = true;
}

static void walk_quadto(fz_context *ctx, void *arg, float x1, float y1, float x2, float y2) {
	(void)ctx;
	(void)x1;
	(void)y1;
	PathWalkState *state = (PathWalkState *)arg;
	state->current = fz_transform_point_xy(x2, y2, state->ctm);
	state->has_current = true;
}

static void walk_curvetov(fz_context *ctx, void *arg, float x2, float y2, float x3, float y3) {
	(void)ctx;
	(void)x2;
	(void)y2;
	PathWalkState *state = (PathWalkState *)arg;
	state->current = fz_transform_point_xy(x3, y3, state->ctm);
	state->has_current = true;
}

static void walk_curvetoy(fz_context *ctx, void *arg, float x1, float y1, float x3, float y3) {
	(void)ctx;
	(void)x1;
	(void)y1;
	PathWalkState *state = (PathWalkState *)arg;
	state->current = fz_transform_point_xy(x3, y3, state->ctm);
	state->has_current = true;
}

static void walk_closepath(fz_context *ctx, void *arg) {
	(void)ctx;
	PathWalkState *state = (PathWalkState *)arg;
	if (state->has_current && state->has_subpath) {
		walk_add_segment(state, state->current, state->subpath_start);
		state->current = state->subpath_start;
	}
}

static void walk_rectto(fz_context *ctx, void *arg, float x0, float y0, float x1, float y1) {
	(void)ctx;
	PathWalkState *state = (PathWalkState *)arg;
	fz_point p00 = fz_transform_point_xy(x0, y0, state->ctm);
	fz_point p10 = fz_transform_point_xy(x1, y0, state->ctm);
	fz_point p11 = fz_transform_point_xy(x1, y1, state->ctm);
	fz_point p01 = fz_transform_point_xy(x0, y1, state->ctm);
	double rx0 = fmin(fmin(p00.x, p10.x), fmin(p11.x, p01.x));
	double rx1 = fmax(fmax(p00.x, p10.x), fmax(p11.x, p01.x));
	double ry0 = fmin(fmin(p00.y, p10.y), fmin(p11.y, p01.y));
	double ry1 = fmax(fmax(p00.y, p10.y), fmax(p11.y, p01.y));
	double w = rx1 - rx0;
	double h = ry1 - ry0;
	if (w <= 3.0 && w < h) {
		push_table_edge(state->dev, true, 0.5 * (rx0 + rx1), ry0, ry1);
	} else if (h <= 3.0 && h < w) {
		push_table_edge(state->dev, false, 0.5 * (ry0 + ry1), rx0, rx1);
	} else {
		push_table_edge(state->dev, true, rx0, ry0, ry1);
		push_table_edge(state->dev, true, rx1, ry0, ry1);
		push_table_edge(state->dev, false, ry0, rx0, rx1);
		push_table_edge(state->dev, false, ry1, rx0, rx1);
	}
	state->current = p00;
	state->subpath_start = p00;
	state->has_current = true;
	state->has_subpath = true;
}

static const fz_path_walker TABLE_PATH_WALKER = {
	walk_moveto,   walk_lineto,   walk_curveto,  walk_closepath,
	walk_quadto,   walk_curvetov, walk_curvetoy, walk_rectto
};

static void collect_record_path(fz_context *ctx, PageCollectDevice *dev, const fz_path *path,
                                const fz_stroke_state *stroke, fz_matrix ctm) {
	fz_rect bound = fz_bound_path(ctx, path, stroke, ctm);
	if (!fz_is_empty_rect(bound) && !fz_is_infinite_rect(bound)) {
		BoxRect rect = {bound.x0, bound.y0, bound.x1, bound.y1};
		push_box_rect(&dev->drawing_rects, &dev->drawing_count, &dev->drawing_capacity,
			      rect);
		push_box_rect(&dev->path_rects, &dev->path_count, &dev->path_capacity, rect);
	} else if (fz_is_valid_rect(bound) && !fz_is_infinite_rect(bound)) {
		/* Zero-width or zero-height axis-aligned stroke line still counts as a drawing rect */
		BoxRect rect = {bound.x0, bound.y0, bound.x1, bound.y1};
		if (rect.x1 > rect.x0 || rect.y1 > rect.y0) {
			push_box_rect(&dev->drawing_rects, &dev->drawing_count,
				      &dev->drawing_capacity, rect);
			push_box_rect(&dev->path_rects, &dev->path_count, &dev->path_capacity,
				      rect);
		}
	}
	PathWalkState state;
	memset(&state, 0, sizeof(state));
	state.dev = dev;
	state.ctm = ctm;
	fz_walk_path(ctx, path, &TABLE_PATH_WALKER, &state);
}

static void collect_fill_path(fz_context *ctx, fz_device *dev, const fz_path *path, int even_odd,
                              fz_matrix ctm, fz_colorspace *cs, const float *color, float alpha,
                              fz_color_params color_params) {
	(void)even_odd;
	(void)cs;
	(void)color;
	(void)alpha;
	(void)color_params;
	collect_record_path(ctx, (PageCollectDevice *)dev, path, NULL, ctm);
}

static void collect_stroke_path(fz_context *ctx, fz_device *dev, const fz_path *path,
                                const fz_stroke_state *stroke, fz_matrix ctm, fz_colorspace *cs,
                                const float *color, float alpha, fz_color_params color_params) {
	(void)cs;
	(void)color;
	(void)alpha;
	(void)color_params;
	collect_record_path(ctx, (PageCollectDevice *)dev, path, stroke, ctm);
}

static void collect_record_image(fz_context *ctx, PageCollectDevice *dev, fz_image *image,
                                 fz_matrix ctm) {
	fz_rect bound = fz_transform_rect(fz_make_rect(0.0f, 0.0f, 1.0f, 1.0f), ctm);
	if (fz_is_empty_rect(bound) || fz_is_infinite_rect(bound)) {
		return;
	}
	if (dev->image_count == dev->image_capacity) {
		dev->image_capacity = dev->image_capacity ? dev->image_capacity * 2 : 16;
		dev->placed_images = (PlacedImageInfo *)reallocate_memory(
			dev->placed_images, dev->image_capacity * sizeof(PlacedImageInfo));
	}
	fz_image_digest(ctx, image, dev->placed_images[dev->image_count].digest);
	dev->placed_images[dev->image_count].bbox.x0 = bound.x0;
	dev->placed_images[dev->image_count].bbox.y0 = bound.y0;
	dev->placed_images[dev->image_count].bbox.x1 = bound.x1;
	dev->placed_images[dev->image_count].bbox.y1 = bound.y1;
	dev->image_count++;
}

static void collect_fill_image(fz_context *ctx, fz_device *dev, fz_image *image, fz_matrix ctm,
                               float alpha, fz_color_params color_params) {
	(void)alpha;
	(void)color_params;
	collect_record_image(ctx, (PageCollectDevice *)dev, image, ctm);
}

static void collect_fill_image_mask(fz_context *ctx, fz_device *dev, fz_image *image, fz_matrix ctm,
                                    fz_colorspace *cs, const float *color, float alpha,
                                    fz_color_params color_params) {
	(void)cs;
	(void)color;
	(void)alpha;
	(void)color_params;
	collect_record_image(ctx, (PageCollectDevice *)dev, image, ctm);
}

static void collect_drop_device(fz_context *ctx, fz_device *dev) {
	(void)ctx;
	PageCollectDevice *collect = (PageCollectDevice *)dev;
	free(collect->drawing_rects);
	free(collect->path_rects);
	free(collect->edges);
	free(collect->placed_images);
	collect->drawing_rects = NULL;
	collect->path_rects = NULL;
	collect->edges = NULL;
	collect->placed_images = NULL;
}

static PageCollectDevice *new_collect_device(fz_context *context) {
	PageCollectDevice *dev =
		(PageCollectDevice *)fz_new_device_of_size(context, sizeof(PageCollectDevice));
	dev->context = context;
	dev->super.fill_path = collect_fill_path;
	dev->super.stroke_path = collect_stroke_path;
	dev->super.fill_image = collect_fill_image;
	dev->super.fill_image_mask = collect_fill_image_mask;
	dev->super.drop_device = collect_drop_device;
	return dev;
}

/* Vector drawing clustering (O(N) spatial-grid union-find matching gen_corpus.py) */
typedef struct {
	int gx, gy;
	int owner;
	uint8_t occupied;
} GridCellSlot;

static int find_root_node(int *parent, int index) {
	while (parent[index] != index) {
		parent[index] = parent[parent[index]];
		index = parent[index];
	}
	return index;
}

static void union_grid_nodes(int *parent, int left, int right) {
	int lr = find_root_node(parent, left);
	int rr = find_root_node(parent, right);
	if (lr != rr) {
		parent[rr] = lr;
	}
}

static size_t cluster_vector_drawings(const BoxRect *drawings, size_t drawing_count,
                                      double page_width, double page_height,
                                      const BoxRect *excluded, size_t excluded_count,
                                      const BoxRect *text_boxes, size_t text_count,
                                      BoxRect **out_clusters) {
	*out_clusters = NULL;
	double page_area = page_width * page_height;
	BoxRect *raw = (BoxRect *)allocate_memory((drawing_count + 1) * sizeof(BoxRect));
	size_t raw_count = 0;

	for (size_t i = 0; i < drawing_count; ++i) {
		BoxRect r = drawings[i];
		double w = r.x1 - r.x0;
		double h = r.y1 - r.y0;
		if (w <= 0.5 && h <= 0.5) {
			continue;
		}
		if ((w * h) > 0.82 * page_area) {
			continue;
		}
		if (r.y1 < 0.08 * page_height || r.y0 > 0.92 * page_height) {
			continue;
		}
		bool is_excluded = false;
		for (size_t j = 0; j < excluded_count; ++j) {
			if (rectangle_overlap_ratio(r, excluded[j]) >= 0.5) {
				is_excluded = true;
				break;
			}
		}
		if (!is_excluded) {
			raw[raw_count++] = r;
		}
	}

	if (raw_count < 6) {
		free(raw);
		return 0;
	}

	int *parent = (int *)allocate_memory(raw_count * sizeof(int));
	for (size_t i = 0; i < raw_count; ++i) {
		parent[i] = (int)i;
	}

	size_t grid_cap = 4096;
	while (grid_cap < raw_count * 16) {
		grid_cap *= 2;
	}
	GridCellSlot *grid = (GridCellSlot *)allocate_zeroed(grid_cap, sizeof(GridCellSlot));
	size_t mask = grid_cap - 1;
	size_t grid_used = 0;

	const double half_margin = 6.0;
	const double grid_step = 16.0;
	for (size_t i = 0; i < raw_count; ++i) {
		int gx0 = (int)floor((raw[i].x0 - half_margin) / grid_step);
		int gx1 = (int)floor((raw[i].x1 + half_margin) / grid_step);
		int gy0 = (int)floor((raw[i].y0 - half_margin) / grid_step);
		int gy1 = (int)floor((raw[i].y1 + half_margin) / grid_step);
		for (int gx = gx0; gx <= gx1; ++gx) {
			for (int gy = gy0; gy <= gy1; ++gy) {
				if ((grid_used + 1) * 10 >= grid_cap * 7) {
					size_t next_cap = grid_cap * 2;
					GridCellSlot *next_grid = (GridCellSlot *)allocate_zeroed(
						next_cap, sizeof(GridCellSlot));
					size_t next_mask = next_cap - 1;
					for (size_t k = 0; k < grid_cap; ++k) {
						if (!grid[k].occupied) {
							continue;
						}
						uint64_t h = mix_hash64(
							((uint64_t)(uint32_t)grid[k].gx << 32) |
							(uint32_t)grid[k].gy);
						size_t pos = (size_t)h & next_mask;
						while (next_grid[pos].occupied) {
							pos = (pos + 1) & next_mask;
						}
						next_grid[pos] = grid[k];
					}
					free(grid);
					grid = next_grid;
					grid_cap = next_cap;
					mask = next_mask;
				}
				uint64_t h = mix_hash64(((uint64_t)(uint32_t)gx << 32) |
							(uint32_t)gy);
				size_t pos = (size_t)h & mask;
				while (grid[pos].occupied) {
					if (grid[pos].gx == gx && grid[pos].gy == gy) {
						union_grid_nodes(parent, grid[pos].owner, (int)i);
						goto next_cell;
					}
					pos = (pos + 1) & mask;
				}
				grid[pos].gx = gx;
				grid[pos].gy = gy;
				grid[pos].owner = (int)i;
				grid[pos].occupied = 1;
				grid_used++;
			next_cell:;
			}
		}
	}
	free(grid);

	typedef struct {
		int count;
		double min_x0, min_y0, max_x1, max_y1;
	} ClusterAccum;

	ClusterAccum *accum = (ClusterAccum *)allocate_zeroed(raw_count, sizeof(ClusterAccum));
	for (size_t i = 0; i < raw_count; ++i) {
		int root = find_root_node(parent, (int)i);
		if (accum[root].count == 0) {
			accum[root].min_x0 = raw[i].x0;
			accum[root].min_y0 = raw[i].y0;
			accum[root].max_x1 = raw[i].x1;
			accum[root].max_y1 = raw[i].y1;
		} else {
			if (raw[i].x0 < accum[root].min_x0) {
				accum[root].min_x0 = raw[i].x0;
			}
			if (raw[i].y0 < accum[root].min_y0) {
				accum[root].min_y0 = raw[i].y0;
			}
			if (raw[i].x1 > accum[root].max_x1) {
				accum[root].max_x1 = raw[i].x1;
			}
			if (raw[i].y1 > accum[root].max_y1) {
				accum[root].max_y1 = raw[i].y1;
			}
		}
		accum[root].count++;
	}

	BoxRect *clusters = (BoxRect *)allocate_memory(raw_count * sizeof(BoxRect));
	size_t cluster_count = 0;
	for (size_t i = 0; i < raw_count; ++i) {
		if (accum[i].count < 6) {
			continue;
		}
		double cx0 = fmax(0.0, accum[i].min_x0 - 4.0);
		double cy0 = fmax(0.0, accum[i].min_y0 - 4.0);
		double cx1 = fmin(page_width, accum[i].max_x1 + 4.0);
		double cy1 = fmin(page_height, accum[i].max_y1 + 4.0);
		double cw = cx1 - cx0;
		double ch = cy1 - cy0;
		double area = cw * ch;
		if (cw < 80.0 || ch < 60.0 || area > 0.80 * page_area) {
			continue;
		}
		if (fmax(cw / fmax(ch, 1.0), ch / fmax(cw, 1.0)) > 6.0) {
			continue;
		}
		BoxRect crect = {cx0, cy0, cx1, cy1};
		bool hit_excluded = false;
		for (size_t j = 0; j < excluded_count; ++j) {
			if (rectangle_overlap_ratio(crect, excluded[j]) >= 0.45) {
				hit_excluded = true;
				break;
			}
		}
		if (hit_excluded) {
			continue;
		}
		double text_area = 0.0;
		for (size_t j = 0; j < text_count; ++j) {
			double overlap = rectangle_overlap_ratio(text_boxes[j], crect);
			if (overlap > 0.2) {
				text_area += (text_boxes[j].x1 - text_boxes[j].x0) *
					     (text_boxes[j].y1 - text_boxes[j].y0) * overlap;
			}
		}
		if ((text_area / fmax(area, 1.0)) > 0.55) {
			continue;
		}
		clusters[cluster_count++] = crect;
	}

	free(accum);
	free(parent);
	free(raw);
	*out_clusters = clusters;
	return cluster_count;
}

/* Line-based Table Extraction (matching PyMuPDF/pdfplumber table.py) */
typedef struct {
	double x, y;
} Point2D;

typedef struct {
	BoxRect rect;
	char *utf8;
} StextCharBox;

static int compare_edge_coord_start(const void *left, const void *right) {
	const TableEdge *a = (const TableEdge *)left;
	const TableEdge *b = (const TableEdge *)right;
	if (a->coord < b->coord) {
		return -1;
	}
	if (a->coord > b->coord) {
		return 1;
	}
	if (a->start < b->start) {
		return -1;
	}
	if (a->start > b->start) {
		return 1;
	}
	return 0;
}

static int compare_double_asc(const void *left, const void *right) {
	double a = *(const double *)left;
	double b = *(const double *)right;
	return (a > b) - (a < b);
}

static int compare_cell_top_left(const void *left, const void *right) {
	const BoxRect *a = (const BoxRect *)left;
	const BoxRect *b = (const BoxRect *)right;
	if (a->y0 < b->y0) {
		return -1;
	}
	if (a->y0 > b->y0) {
		return 1;
	}
	if (a->x0 < b->x0) {
		return -1;
	}
	if (a->x0 > b->x0) {
		return 1;
	}
	return 0;
}

static void snap_and_merge_edges(TableEdge *edges, size_t count, bool vertical, double snap_tol,
                                 double join_tol, double min_len, TableEdge **out_edges,
                                 size_t *out_count) {
	*out_edges = NULL;
	*out_count = 0;
	TableEdge *sub = (TableEdge *)allocate_memory((count + 1) * sizeof(TableEdge));
	size_t sub_count = 0;
	for (size_t i = 0; i < count; ++i) {
		if (edges[i].vertical == vertical) {
			sub[sub_count++] = edges[i];
		}
	}
	if (sub_count == 0) {
		free(sub);
		return;
	}
	qsort(sub, sub_count, sizeof(TableEdge), compare_edge_coord_start);

	/* Snap close coordinates to cluster mean */
	size_t cluster_start = 0;
	while (cluster_start < sub_count) {
		size_t cluster_end = cluster_start + 1;
		double last = sub[cluster_start].coord;
		double sum = last;
		while (cluster_end < sub_count && sub[cluster_end].coord <= last + snap_tol) {
			last = sub[cluster_end].coord;
			sum += last;
			cluster_end++;
		}
		double avg = sum / (double)(cluster_end - cluster_start);
		for (size_t k = cluster_start; k < cluster_end; ++k) {
			sub[k].coord = avg;
		}
		cluster_start = cluster_end;
	}

	qsort(sub, sub_count, sizeof(TableEdge), compare_edge_coord_start);

	TableEdge *merged = (TableEdge *)allocate_memory(sub_count * sizeof(TableEdge));
	size_t merged_count = 0;
	for (size_t i = 0; i < sub_count; ++i) {
		if (merged_count == 0 || fabs(sub[i].coord - merged[merged_count - 1].coord) > 1e-6 ||
		    sub[i].start > merged[merged_count - 1].end + join_tol) {
			merged[merged_count++] = sub[i];
		} else if (sub[i].end > merged[merged_count - 1].end) {
			merged[merged_count - 1].end = sub[i].end;
		}
	}
	free(sub);

	size_t final_count = 0;
	for (size_t i = 0; i < merged_count; ++i) {
		if ((merged[i].end - merged[i].start) >= min_len) {
			merged[final_count++] = merged[i];
		}
	}
	*out_edges = merged;
	*out_count = final_count;
}

static bool rect_contains_any_nonspace(BoxRect box, const StextCharBox *chars, size_t char_count) {
	for (size_t i = 0; i < char_count; ++i) {
		const char *s = chars[i].utf8;
		if (!has_non_whitespace(s)) {
			continue;
		}
		BoxRect c = chars[i].rect;
		if (c.x0 < box.x1 && c.x1 > box.x0 && c.y0 < box.y1 && c.y1 > box.y0) {
			return true;
		}
	}
	return false;
}

static bool paths_are_neighbors(BoxRect r1, BoxRect r2, double tol) {
	bool x_near = ((r1.x0 >= r2.x0 - tol && r1.x0 <= r2.x1 + tol) ||
		       (r1.x1 >= r2.x0 - tol && r1.x1 <= r2.x1 + tol) ||
		       (r2.x0 >= r1.x0 - tol && r2.x0 <= r1.x1 + tol) ||
		       (r2.x1 >= r1.x0 - tol && r2.x1 <= r1.x1 + tol));
	bool y_near = ((r1.y0 >= r2.y0 - tol && r1.y0 <= r2.y1 + tol) ||
		       (r1.y1 >= r2.y0 - tol && r1.y1 <= r2.y1 + tol) ||
		       (r2.y0 >= r1.y0 - tol && r2.y0 <= r1.y1 + tol) ||
		       (r2.y1 >= r1.y0 - tol && r2.y1 <= r1.y1 + tol));
	return x_near && y_near;
}

static char *extract_cell_text_from_stext(fz_stext_page *stext_page, BoxRect cell) {
	TextBuffer out;
	init_buffer(&out);

	for (fz_stext_block *blk = stext_page->first_block; blk; blk = blk->next) {
		if (blk->type != FZ_STEXT_BLOCK_TEXT) {
			continue;
		}
		if (blk->bbox.x0 > cell.x1 || blk->bbox.x1 < cell.x0 || blk->bbox.y0 > cell.y1 ||
		    blk->bbox.y1 < cell.y0) {
			continue;
		}
		for (fz_stext_line *line = blk->u.t.first_line; line; line = line->next) {
			if (line->bbox.x0 > cell.x1 || line->bbox.x1 < cell.x0 ||
			    line->bbox.y0 > cell.y1 || line->bbox.y1 < cell.y0) {
				continue;
			}
			TextBuffer line_buf;
			init_buffer(&line_buf);
			for (fz_stext_char *ch = line->first_char; ch; ch = ch->next) {
				fz_rect cr = fz_rect_from_quad(ch->quad);
				double cx = 0.5 * (cr.x0 + cr.x1);
				double cy = 0.5 * (cr.y0 + cr.y1);
				if (cx >= cell.x0 && cx < cell.x1 && cy >= cell.y0 && cy < cell.y1) {
					if (ch->flags & FZ_STEXT_UNICODE_IS_CID) {
						char cid[32];
						snprintf(cid, sizeof(cid), "(cid:%d)", ch->c);
						append_string(&line_buf, cid);
					} else if (ch->c >= 0) {
						append_utf8_point(&line_buf, (uint32_t)ch->c);
					}
				}
			}
			char *line_str = detach_buffer(&line_buf);
			if (*line_str) {
				if (out.length > 0) {
					append_char(&out, ' ');
				}
				append_string(&out, line_str);
			}
			free(line_str);
		}
	}
	char *result = detach_buffer(&out);
	trim_inplace(result);
	return result;
}

static void extract_page_tables(PageCollectDevice *collect, fz_stext_page *stext_page,
                                double page_height, PageItemList *page_items,
                                BoxRect **out_table_boxes, size_t *out_table_count) {
	*out_table_boxes = NULL;
	*out_table_count = 0;

	/* Collect all stext char boxes once for fast text-in-rect probes */
	size_t char_cap = 256;
	size_t char_count = 0;
	StextCharBox *char_boxes = (StextCharBox *)allocate_memory(char_cap * sizeof(StextCharBox));
	for (fz_stext_block *blk = stext_page->first_block; blk; blk = blk->next) {
		if (blk->type != FZ_STEXT_BLOCK_TEXT) {
			continue;
		}
		for (fz_stext_line *line = blk->u.t.first_line; line; line = line->next) {
			for (fz_stext_char *ch = line->first_char; ch; ch = ch->next) {
				if (char_count == char_cap) {
					char_cap *= 2;
					char_boxes = (StextCharBox *)reallocate_memory(
						char_boxes, char_cap * sizeof(StextCharBox));
				}
				fz_rect cr = fz_rect_from_quad(ch->quad);
				char_boxes[char_count].rect.x0 = cr.x0;
				char_boxes[char_count].rect.y0 = cr.y0;
				char_boxes[char_count].rect.x1 = cr.x1;
				char_boxes[char_count].rect.y1 = cr.y1;
				TextBuffer cb;
				init_buffer(&cb);
				if (ch->c >= 0 && !(ch->flags & FZ_STEXT_UNICODE_IS_CID)) {
					append_utf8_point(&cb, (uint32_t)ch->c);
				} else {
					append_char(&cb, ' ');
				}
				char_boxes[char_count].utf8 = detach_buffer(&cb);
				char_count++;
			}
		}
	}

	/* Add enveloping border edges from connected path clusters containing text */
	size_t edge_total = collect->edge_count;
	TableEdge *all_edges = (TableEdge *)allocate_memory((edge_total + collect->path_count * 4 + 1) *
							    sizeof(TableEdge));
	if (edge_total > 0) {
		memcpy(all_edges, collect->edges, edge_total * sizeof(TableEdge));
	}

	if (collect->path_count > 0) {
		BoxRect *prects =
			(BoxRect *)allocate_memory(collect->path_count * sizeof(BoxRect));
		memcpy(prects, collect->path_rects, collect->path_count * sizeof(BoxRect));
		size_t pcount = collect->path_count;
		while (pcount > 0) {
			BoxRect cur = prects[0];
			bool changed = true;
			while (changed) {
				changed = false;
				for (size_t i = pcount; i > 1; --i) {
					size_t idx = i - 1;
					if (paths_are_neighbors(cur, prects[idx], 3.0)) {
						if (prects[idx].x0 < cur.x0) {
							cur.x0 = prects[idx].x0;
						}
						if (prects[idx].y0 < cur.y0) {
							cur.y0 = prects[idx].y0;
						}
						if (prects[idx].x1 > cur.x1) {
							cur.x1 = prects[idx].x1;
						}
						if (prects[idx].y1 > cur.y1) {
							cur.y1 = prects[idx].y1;
						}
						prects[idx] = prects[--pcount];
						changed = true;
					}
				}
			}
			prects[0] = prects[--pcount];
			if ((cur.x1 - cur.x0) > 3.0 && (cur.y1 - cur.y0) > 3.0 &&
			    rect_contains_any_nonspace(cur, char_boxes, char_count)) {
				all_edges[edge_total++] =
					(TableEdge){cur.x0, cur.y0, cur.y1, true};
				all_edges[edge_total++] =
					(TableEdge){cur.x1, cur.y0, cur.y1, true};
				all_edges[edge_total++] =
					(TableEdge){cur.y0, cur.x0, cur.x1, false};
				all_edges[edge_total++] =
					(TableEdge){cur.y1, cur.x0, cur.x1, false};
			}
		}
		free(prects);
	}

	TableEdge *v_edges = NULL;
	TableEdge *h_edges = NULL;
	size_t v_count = 0;
	size_t h_count = 0;
	snap_and_merge_edges(all_edges, edge_total, true, 3.0, 3.0, 3.0, &v_edges, &v_count);
	snap_and_merge_edges(all_edges, edge_total, false, 3.0, 3.0, 3.0, &h_edges, &h_count);
	free(all_edges);

	if (v_count < 2 || h_count < 2) {
		goto cleanup_tables;
	}

	/* Compute intersections (v_idx, h_idx) on grid */
	typedef struct {
		double x, y;
		size_t v_idx, h_idx;
	} Intersection;

	size_t inter_cap = 256;
	size_t inter_count = 0;
	Intersection *inters = (Intersection *)allocate_memory(inter_cap * sizeof(Intersection));

	for (size_t vi = 0; vi < v_count; ++vi) {
		for (size_t hi = 0; hi < h_count; ++hi) {
			if (v_edges[vi].start <= h_edges[hi].coord + 3.0 &&
			    v_edges[vi].end >= h_edges[hi].coord - 3.0 &&
			    v_edges[vi].coord >= h_edges[hi].start - 3.0 &&
			    v_edges[vi].coord <= h_edges[hi].end + 3.0) {
				if (inter_count == inter_cap) {
					inter_cap *= 2;
					inters = (Intersection *)reallocate_memory(
						inters, inter_cap * sizeof(Intersection));
				}
				inters[inter_count].x = v_edges[vi].coord;
				inters[inter_count].y = h_edges[hi].coord;
				inters[inter_count].v_idx = vi;
				inters[inter_count].h_idx = hi;
				inter_count++;
			}
		}
	}

	/* Find smallest cells from intersections */
	size_t cell_cap = 128;
	size_t cell_count = 0;
	BoxRect *cells = (BoxRect *)allocate_memory(cell_cap * sizeof(BoxRect));

	for (size_t i = 0; i < inter_count; ++i) {
		double px = inters[i].x;
		double py = inters[i].y;
		size_t p_vi = inters[i].v_idx;
		size_t p_hi = inters[i].h_idx;

		/* Find nearest intersection directly below on same v_edge and right on same h_edge */
		double best_below_y = 1e30;
		double best_right_x = 1e30;
		size_t below_idx = (size_t)-1;
		size_t right_idx = (size_t)-1;

		for (size_t j = 0; j < inter_count; ++j) {
			if (j == i) {
				continue;
			}
			if (inters[j].v_idx == p_vi && inters[j].y > py + 1e-6 &&
			    inters[j].y < best_below_y) {
				best_below_y = inters[j].y;
				below_idx = j;
			}
			if (inters[j].h_idx == p_hi && inters[j].x > px + 1e-6 &&
			    inters[j].x < best_right_x) {
				best_right_x = inters[j].x;
				right_idx = j;
			}
		}
		if (below_idx == (size_t)-1 || right_idx == (size_t)-1) {
			continue;
		}
		/* Verify closing bottom-right corner shares h_edge with below and v_edge with right */
		bool closed = false;
		for (size_t j = 0; j < inter_count; ++j) {
			if (inters[j].v_idx == inters[right_idx].v_idx &&
			    inters[j].h_idx == inters[below_idx].h_idx) {
				closed = true;
				break;
			}
		}
		if (closed) {
			if (cell_count == cell_cap) {
				cell_cap *= 2;
				cells = (BoxRect *)reallocate_memory(cells,
								     cell_cap * sizeof(BoxRect));
			}
			cells[cell_count++] =
				(BoxRect){px, py, inters[right_idx].x, inters[below_idx].y};
		}
	}
	free(inters);

	if (cell_count == 0) {
		free(cells);
		goto cleanup_tables;
	}

	/* Group cells into connected tables via shared corners (Union-Find) */
	int *cell_parent = (int *)allocate_memory(cell_count * sizeof(int));
	for (size_t i = 0; i < cell_count; ++i) {
		cell_parent[i] = (int)i;
	}
	for (size_t i = 0; i < cell_count; ++i) {
		for (size_t j = i + 1; j < cell_count; ++j) {
			double ax[2] = {cells[i].x0, cells[i].x1};
			double ay[2] = {cells[i].y0, cells[i].y1};
			double bx[2] = {cells[j].x0, cells[j].x1};
			double by[2] = {cells[j].y0, cells[j].y1};
			bool share_corner = false;
			for (int ix = 0; ix < 2 && !share_corner; ++ix) {
				for (int iy = 0; iy < 2 && !share_corner; ++iy) {
					for (int jx = 0; jx < 2 && !share_corner; ++jx) {
						for (int jy = 0; jy < 2; ++jy) {
							if (fabs(ax[ix] - bx[jx]) <= 1e-4 &&
							    fabs(ay[iy] - by[jy]) <= 1e-4) {
								share_corner = true;
								break;
							}
						}
					}
				}
			}
			if (share_corner) {
				union_grid_nodes(cell_parent, (int)i, (int)j);
			}
		}
	}

	bool *visited_root = (bool *)allocate_zeroed(cell_count, sizeof(bool));
	BoxRect *table_boxes = (BoxRect *)allocate_memory(cell_count * sizeof(BoxRect));
	size_t table_box_count = 0;

	for (size_t i = 0; i < cell_count; ++i) {
		int root = find_root_node(cell_parent, (int)i);
		if (visited_root[root]) {
			continue;
		}
		visited_root[root] = true;

		size_t grp_count = 0;
		BoxRect *grp = (BoxRect *)allocate_memory(cell_count * sizeof(BoxRect));
		BoxRect tbbox = {1e30, 1e30, -1e30, -1e30};
		for (size_t j = 0; j < cell_count; ++j) {
			if (find_root_node(cell_parent, (int)j) == root) {
				grp[grp_count++] = cells[j];
				if (cells[j].x0 < tbbox.x0) {
					tbbox.x0 = cells[j].x0;
				}
				if (cells[j].y0 < tbbox.y0) {
					tbbox.y0 = cells[j].y0;
				}
				if (cells[j].x1 > tbbox.x1) {
					tbbox.x1 = cells[j].x1;
				}
				if (cells[j].y1 > tbbox.y1) {
					tbbox.y1 = cells[j].y1;
				}
			}
		}

		/* Require >= 2 distinct x0 columns and text inside */
		double *xs = (double *)allocate_memory(grp_count * sizeof(double));
		double *ys = (double *)allocate_memory(grp_count * sizeof(double));
		for (size_t j = 0; j < grp_count; ++j) {
			xs[j] = grp[j].x0;
			ys[j] = grp[j].y0;
		}
		qsort(xs, grp_count, sizeof(double), compare_double_asc);
		qsort(ys, grp_count, sizeof(double), compare_double_asc);
		size_t uxs = 0;
		for (size_t j = 0; j < grp_count; ++j) {
			if (uxs == 0 || fabs(xs[j] - xs[uxs - 1]) > 1e-4) {
				xs[uxs++] = xs[j];
			}
		}
		size_t uys = 0;
		for (size_t j = 0; j < grp_count; ++j) {
			if (uys == 0 || fabs(ys[j] - ys[uys - 1]) > 1e-4) {
				ys[uys++] = ys[j];
			}
		}

		if (uxs >= 2 && uys >= 2 && rect_contains_any_nonspace(tbbox, char_boxes, char_count)) {
			qsort(grp, grp_count, sizeof(BoxRect), compare_cell_top_left);
			TableRows rows;
			init_table_rows(&rows);
			for (size_t r = 0; r < uys; ++r) {
				TextList row;
				init_text_list(&row);
				for (size_t c = 0; c < uxs; ++c) {
					const BoxRect *found_cell = NULL;
					for (size_t j = 0; j < grp_count; ++j) {
						if (fabs(grp[j].y0 - ys[r]) <= 1e-4 &&
						    fabs(grp[j].x0 - xs[c]) <= 1e-4) {
							found_cell = &grp[j];
							break;
						}
					}
					if (found_cell) {
						push_text_owned(&row,
								extract_cell_text_from_stext(
									stext_page, *found_cell));
					} else {
						push_text_copy(&row, "");
					}
				}
				push_table_row(&rows, row);
			}
			if (is_valid_table(&rows, tbbox, page_height)) {
				char *md_table = format_markdown_table(&rows);
				if (*md_table) {
					table_boxes[table_box_count++] = tbbox;
					push_page_item(page_items, tbbox, BLOCK_TABLE, md_table);
				} else {
					free(md_table);
				}
			}
			free_table_rows(&rows);
		}
		free(xs);
		free(ys);
		free(grp);
	}

	free(visited_root);
	free(cell_parent);
	free(cells);
	*out_table_boxes = table_boxes;
	*out_table_count = table_box_count;

cleanup_tables:
	free(v_edges);
	free(h_edges);
	for (size_t i = 0; i < char_count; ++i) {
		free(char_boxes[i].utf8);
	}
	free(char_boxes);
}

/* Recursive PDF page image resource enumeration matching PyMuPDF get_images(full=True) */
typedef struct {
	int *seen_forms;
	size_t seen_count;
	size_t seen_cap;
	PdfImageRef *refs;
	size_t ref_count;
	size_t ref_cap;
} XObjectCollector;

static void collect_xobjects_recursive(fz_context *context, pdf_obj *resources,
                                       XObjectCollector *collector) {
	if (!resources || !pdf_is_dict(context, resources)) {
		return;
	}
	pdf_obj *xobj_dict = pdf_dict_get(context, resources, PDF_NAME(XObject));
	if (!xobj_dict || !pdf_is_dict(context, xobj_dict)) {
		return;
	}
	int len = pdf_dict_len(context, xobj_dict);
	for (int i = 0; i < len; ++i) {
		pdf_obj *val = pdf_dict_get_val(context, xobj_dict, i);
		if (!val || !pdf_is_dict(context, val)) {
			continue;
		}
		pdf_obj *subtype = pdf_dict_get(context, val, PDF_NAME(Subtype));
		const char *sname = subtype ? pdf_to_name(context, subtype) : "";
		if (strcmp(sname, "Image") == 0) {
			int xref = pdf_to_num(context, val);
			if (xref <= 0) {
				continue;
			}
			int smask = 0;
			pdf_obj *smask_obj = pdf_dict_get(context, val, PDF_NAME(SMask));
			if (smask_obj) {
				smask = pdf_to_num(context, smask_obj);
			}
			bool duplicate = false;
			for (size_t j = 0; j < collector->ref_count; ++j) {
				if (collector->refs[j].xref == xref) {
					duplicate = true;
					break;
				}
			}
			if (!duplicate) {
				if (collector->ref_count == collector->ref_cap) {
					collector->ref_cap =
						collector->ref_cap ? collector->ref_cap * 2 : 16;
					collector->refs = (PdfImageRef *)reallocate_memory(
						collector->refs,
						collector->ref_cap * sizeof(PdfImageRef));
				}
				collector->refs[collector->ref_count].xref = xref;
				collector->refs[collector->ref_count].smask = smask;
				collector->ref_count++;
			}
		} else if (strcmp(sname, "Form") == 0) {
			int form_xref = pdf_to_num(context, val);
			bool seen = false;
			if (form_xref > 0) {
				for (size_t j = 0; j < collector->seen_count; ++j) {
					if (collector->seen_forms[j] == form_xref) {
						seen = true;
						break;
					}
				}
			}
			if (!seen) {
				if (form_xref > 0) {
					if (collector->seen_count == collector->seen_cap) {
						collector->seen_cap = collector->seen_cap
									      ? collector->seen_cap * 2
									      : 16;
						collector->seen_forms = (int *)reallocate_memory(
							collector->seen_forms,
							collector->seen_cap * sizeof(int));
					}
					collector->seen_forms[collector->seen_count++] = form_xref;
				}
				pdf_obj *sub_res =
					pdf_dict_get(context, val, PDF_NAME(Resources));
				if (sub_res) {
					collect_xobjects_recursive(context, sub_res, collector);
				}
			}
		}
	}
}

static size_t collect_page_image_refs(fz_context *context, pdf_page *page, PdfImageRef **out_refs) {
	XObjectCollector collector;
	memset(&collector, 0, sizeof(collector));
	fz_try(context) {
		pdf_obj *res = pdf_page_resources(context, page);
		if (res) {
			collect_xobjects_recursive(context, res, &collector);
		}
	}
	fz_catch(context) {
		/* Best-effort enumeration */
	}
	free(collector.seen_forms);
	*out_refs = collector.refs;
	return collector.ref_count;
}

typedef struct {
	BoxRect bbox;
	char *norm_line;
	double avg_size;
	bool is_mono;
	bool is_bold;
} LineRecord;

static void analyze_stext_line(fz_context *context, fz_stext_line *line, double body_size,
                               size_t *total_corrupt, size_t *total_raw,
                               LineRecord *out_record, bool *out_nonempty) {
	*out_nonempty = false;
	TextBuffer raw_line;
	init_buffer(&raw_line);

	size_t total_chars = 0;
	size_t mono_chars = 0;
	size_t bold_chars = 0;
	double size_acc = 0.0;

	/* Group consecutive chars into spans matching PyMuPDF span boundaries */
	fz_stext_char *ch = line->first_char;
	while (ch) {
		fz_font *span_font = ch->font;
		float span_size = ch->size;
		int span_bold_flag = (ch->flags & FZ_STEXT_BOLD) != 0;
		const char *font_name = span_font ? fz_font_name(context, span_font) : "";
		int font_flags = 0;
		if (span_font && fz_font_is_monospaced(context, span_font)) {
			font_flags |= 8;
		}
		if (span_bold_flag || (span_font && fz_font_is_bold(context, span_font))) {
			font_flags |= 16;
		}
		bool span_mono = is_monospace_span(font_name, font_flags);
		bool span_bold = (font_flags & 16) || contains_ignore_case(font_name, "bold");

		TextBuffer span_buf;
		init_buffer(&span_buf);
		while (ch && ch->font == span_font && fabsf(ch->size - span_size) < 0.01f &&
		       ((ch->flags & FZ_STEXT_BOLD) != 0) == span_bold_flag) {
			if (ch->flags & FZ_STEXT_UNICODE_IS_CID) {
				char cid[32];
				snprintf(cid, sizeof(cid), "(cid:%d)", ch->c);
				append_string(&span_buf, cid);
			} else if (ch->c >= 0) {
				append_utf8_point(&span_buf, (uint32_t)ch->c);
			}
			ch = ch->next;
		}

		if (span_buf.length > 0) {
			append_bytes(&raw_line, span_buf.data, span_buf.length);
			if (total_corrupt && total_raw) {
				size_t sc = 0, sr = 0;
				count_corrupt_characters(span_buf.data, &sc, &sr);
				*total_corrupt += sc;
				*total_raw += sr;
			}
			char *trimmed = duplicate_range(span_buf.data, span_buf.length);
			trim_inplace(trimmed);
			size_t cp_len = count_utf8_points(trimmed, strlen(trimmed));
			free(trimmed);
			if (cp_len > 0) {
				total_chars += cp_len;
				size_acc += (double)span_size * (double)cp_len;
				if (span_mono) {
					mono_chars += cp_len;
				}
				if (span_bold) {
					bold_chars += cp_len;
				}
			}
		}
		free_buffer(&span_buf);
	}

	char *raw_str = detach_buffer(&raw_line);
	char *norm = normalize_unicode(raw_str);
	free(raw_str);

	if (!has_non_whitespace(norm)) {
		free(norm);
		return;
	}

	out_record->bbox = (BoxRect){line->bbox.x0, line->bbox.y0, line->bbox.x1, line->bbox.y1};
	out_record->norm_line = norm;
	out_record->avg_size = total_chars > 0 ? (size_acc / (double)total_chars) : body_size;
	out_record->is_mono = (total_chars > 0) && (((double)mono_chars / (double)total_chars) >= 0.70);
	out_record->is_bold = (total_chars > 0) && (((double)bold_chars / (double)total_chars) >= 0.75);
	*out_nonempty = true;
}

typedef struct {
	int half_pt_size;
	size_t weight;
} FontSizeWeight;

static int compare_font_weight(const void *left, const void *right) {
	const FontSizeWeight *a = (const FontSizeWeight *)left;
	const FontSizeWeight *b = (const FontSizeWeight *)right;
	return (a->half_pt_size > b->half_pt_size) - (a->half_pt_size < b->half_pt_size);
}

static int compare_int_asc(const void *left, const void *right) {
	int a = *(const int *)left;
	int b = *(const int *)right;
	return (a > b) - (a < b);
}

static void extract_pdf_document(fz_context *context, const char *pdf_path, const char *stem,
                                 const char *images_dir, const char *md_dir,
                                 bool extract_images, bool capture_vectors, bool enable_dedup,
                                 CorpusState *corpus_state, ExtractedDocument *out_doc) {
	init_extracted_doc(out_doc, "pdf");

	fz_document *fz_doc = NULL;
	pdf_document *pdf_doc = NULL;
	fz_var(fz_doc);
	fz_var(pdf_doc);

	fz_try(context) {
		fz_doc = fz_open_document(context, pdf_path);
		if (fz_needs_password(context, fz_doc)) {
			fz_authenticate_password(context, fz_doc, "");
		}
		pdf_doc = pdf_document_from_fz_document(context, fz_doc);
	}
	fz_catch(context) {
		fprintf(stderr, "  [!] Failed to open %s: %s\n", path_basename_ptr(pdf_path),
			fz_caught_message(context));
		if (fz_doc) {
			fz_drop_document(context, fz_doc);
		}
		return;
	}

	int total_pages = 0;
	fz_try(context) {
		total_pages = fz_count_pages(context, fz_doc);
	}
	fz_catch(context) {
		fz_drop_document(context, fz_doc);
		return;
	}
	out_doc->page_count = total_pages;

	fz_stext_options stext_opts;
	fz_init_stext_options(context, &stext_opts);
	stext_opts.flags = FZ_STEXT_PRESERVE_LIGATURES | FZ_STEXT_PRESERVE_WHITESPACE |
			   FZ_STEXT_CLIP | FZ_STEXT_USE_CID_FOR_UNKNOWN_UNICODE;

	FontSizeWeight *size_weights = NULL;
	size_t size_weight_count = 0;
	size_t size_weight_cap = 0;

	TextCounter chrome_counts;
	init_text_counter(&chrome_counts);

	typedef struct {
		int xref;
		int count;
		int last_page;
	} XrefPageStat;

	XrefPageStat *xref_stats = NULL;
	size_t xref_stat_count = 0;
	size_t xref_stat_cap = 0;

	size_t total_corrupt = 0;
	size_t total_raw = 0;

	/* Pass 1: Stream pages for body font size, running headers/footers, and logo xrefs */
	for (int page_idx = 0; page_idx < total_pages; ++page_idx) {
		fz_page *page = NULL;
		fz_stext_page *stext = NULL;
		TextCounter page_seen_chrome;
		init_text_counter(&page_seen_chrome);
		fz_var(page);
		fz_var(stext);

		fz_try(context) {
			page = fz_load_page(context, fz_doc, page_idx);
			fz_rect prect = fz_bound_page(context, page);
			double page_h = fmax((double)(prect.y1 - prect.y0), 1.0);
			stext = fz_new_stext_page_from_page(context, page, &stext_opts);

			for (fz_stext_block *blk = stext->first_block; blk; blk = blk->next) {
				if (blk->type != FZ_STEXT_BLOCK_TEXT) {
					continue;
				}
				TextBuffer blk_text;
				init_buffer(&blk_text);
				for (fz_stext_line *line = blk->u.t.first_line; line;
				     line = line->next) {
					fz_stext_char *ch = line->first_char;
					while (ch) {
						fz_font *sfont = ch->font;
						float ssize = ch->size;
						TextBuffer span_buf;
						init_buffer(&span_buf);
						while (ch && ch->font == sfont &&
						       fabsf(ch->size - ssize) < 0.01f) {
							if (ch->flags & FZ_STEXT_UNICODE_IS_CID) {
								char cid[32];
								snprintf(cid, sizeof(cid),
									 "(cid:%d)", ch->c);
								append_string(&span_buf, cid);
							} else if (ch->c >= 0) {
								append_utf8_point(&span_buf,
										  (uint32_t)ch->c);
							}
							ch = ch->next;
						}
						if (span_buf.length > 0) {
							size_t sc = 0, sr = 0;
							count_corrupt_characters(span_buf.data, &sc,
										 &sr);
							total_corrupt += sc;
							total_raw += sr;
							if (blk_text.length > 0) {
								append_char(&blk_text, ' ');
							}
							append_bytes(&blk_text, span_buf.data,
								     span_buf.length);

							char *trimmed = duplicate_range(
								span_buf.data, span_buf.length);
							trim_inplace(trimmed);
							size_t slen = count_utf8_points(
								trimmed, strlen(trimmed));
							free(trimmed);
							if (slen > 0) {
								int half_pt = (int)rint(
									(double)ssize * 2.0);
								bool found = false;
								for (size_t k = 0;
								     k < size_weight_count; ++k) {
									if (size_weights[k]
										    .half_pt_size ==
									    half_pt) {
										size_weights[k]
											.weight +=
											slen;
										found = true;
										break;
									}
								}
								if (!found) {
									if (size_weight_count ==
									    size_weight_cap) {
										size_weight_cap =
											size_weight_cap
												? size_weight_cap *
													  2
												: 32;
										size_weights = (FontSizeWeight *)
											reallocate_memory(
												size_weights,
												size_weight_cap *
													sizeof(FontSizeWeight));
									}
									size_weights
										[size_weight_count]
											.half_pt_size =
											half_pt;
									size_weights
										[size_weight_count]
											.weight =
											slen;
									size_weight_count++;
								}
							}
						}
						free_buffer(&span_buf);
					}
				}
				char *joined = detach_buffer(&blk_text);
				trim_inplace(joined);
				if (*joined && total_pages >= 3) {
					double by0 = blk->bbox.y0;
					double by1 = blk->bbox.y1;
					const char *pos_tag = NULL;
					if (by1 <= 0.10 * page_h || by0 <= 0.08 * page_h) {
						pos_tag = "top|";
					} else if (by0 >= 0.90 * page_h || by1 >= 0.92 * page_h) {
						pos_tag = "bottom|";
					}
					if (pos_tag) {
						char *tmpl = normalize_template(joined);
						if (*tmpl) {
							TextBuffer key;
							init_buffer(&key);
							append_string(&key, pos_tag);
							append_string(&key, tmpl);
							char *key_str = detach_buffer(&key);
							if (!has_text_key(&page_seen_chrome,
									  key_str)) {
								add_text_count(&page_seen_chrome,
									       key_str, 1);
							}
							free(key_str);
						}
						free(tmpl);
					}
				}
				free(joined);
			}

			if (extract_images && pdf_doc) {
				pdf_page *ppage = pdf_page_from_fz_page(context, page);
				PdfImageRef *refs = NULL;
				size_t ref_count = collect_page_image_refs(context, ppage, &refs);
				for (size_t i = 0; i < ref_count; ++i) {
					int xref = refs[i].xref;
					bool found = false;
					for (size_t j = 0; j < xref_stat_count; ++j) {
						if (xref_stats[j].xref == xref) {
							if (xref_stats[j].last_page != page_idx) {
								xref_stats[j].count++;
								xref_stats[j].last_page = page_idx;
							}
							found = true;
							break;
						}
					}
					if (!found) {
						if (xref_stat_count == xref_stat_cap) {
							xref_stat_cap = xref_stat_cap
										? xref_stat_cap * 2
										: 32;
							xref_stats = (XrefPageStat *)
								reallocate_memory(
									xref_stats,
									xref_stat_cap *
										sizeof(XrefPageStat));
						}
						xref_stats[xref_stat_count].xref = xref;
						xref_stats[xref_stat_count].count = 1;
						xref_stats[xref_stat_count].last_page = page_idx;
						xref_stat_count++;
					}
				}
				free(refs);
			}
		}
		fz_always(context) {
			if (stext) {
				fz_drop_stext_page(context, stext);
			}
			if (page) {
				fz_drop_page(context, page);
			}
		}
		fz_catch(context) {
			free_text_counter(&page_seen_chrome);
			continue;
		}

		for (size_t i = 0; i < page_seen_chrome.capacity; ++i) {
			if (page_seen_chrome.slots[i].occupied) {
				add_text_count(&chrome_counts, page_seen_chrome.slots[i].key, 1);
			}
		}
		free_text_counter(&page_seen_chrome);
	}

	double body_size = 10.0;
	if (size_weight_count > 0) {
		qsort(size_weights, size_weight_count, sizeof(FontSizeWeight), compare_font_weight);
		size_t weighted_total = 0;
		for (size_t i = 0; i < size_weight_count; ++i) {
			weighted_total += size_weights[i].weight;
		}
		size_t running = 0;
		for (size_t i = 0; i < size_weight_count; ++i) {
			running += size_weights[i].weight;
			if ((double)running >= (double)weighted_total / 2.0) {
				body_size = (double)size_weights[i].half_pt_size / 2.0;
				break;
			}
		}
	}
	free(size_weights);

	int chrome_threshold = (total_pages >= 3)
				       ? ((int)ceil(0.40 * (double)total_pages) > 2
						  ? (int)ceil(0.40 * (double)total_pages)
						  : 2)
				       : (total_pages + 1);

	int *seen_xrefs = NULL;
	size_t seen_xref_count = 0;
	size_t seen_xref_cap = 0;

	/* Pass 2: Structured page reconstruction with tables, columns, figures, and captions */
	for (int page_idx = 0; page_idx < total_pages; ++page_idx) {
		fz_page *page = NULL;
		fz_stext_page *stext = NULL;
		PageCollectDevice *collect = NULL;
		PageItemList page_items = {NULL, 0, 0};
		BoxRect *table_bboxes = NULL;
		size_t table_count = 0;
		BoxRect *image_bboxes = NULL;
		size_t image_bbox_count = 0;
		size_t image_bbox_cap = 0;
		BoxRect *code_bboxes = NULL;
		size_t code_bbox_count = 0;
		size_t code_bbox_cap = 0;
		BoxRect *prose_bboxes = NULL;
		size_t prose_bbox_count = 0;
		size_t prose_bbox_cap = 0;
		CaptionCandidate *caption_pool = NULL;
		size_t caption_count = 0;
		size_t caption_cap = 0;

		fz_var(page);
		fz_var(stext);
		fz_var(collect);

		fz_try(context) {
			page = fz_load_page(context, fz_doc, page_idx);
			fz_rect prect = fz_bound_page(context, page);
			double page_w = fmax((double)(prect.x1 - prect.x0), 1.0);
			double page_h = fmax((double)(prect.y1 - prect.y0), 1.0);

			stext = fz_new_stext_page_from_page(context, page, &stext_opts);
			collect = new_collect_device(context);
			fz_run_page(context, page, &collect->super,
				    fz_make_matrix(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f), NULL);
			fz_close_device(context, &collect->super);

			/* 1. Extract Markdown tables first so overlapping text blocks are not duplicated */
			extract_page_tables(collect, stext, page_h, &page_items, &table_bboxes,
					    &table_count);

			/* 2. Extract and classify text blocks */
			size_t page_text_chars = 0;
			for (fz_stext_block *blk = stext->first_block; blk; blk = blk->next) {
				if (blk->type != FZ_STEXT_BLOCK_TEXT) {
					continue;
				}
				BoxRect bbox = {blk->bbox.x0, blk->bbox.y0, blk->bbox.x1,
						blk->bbox.y1};
				bool overlaps_table = false;
				for (size_t t = 0; t < table_count; ++t) {
					if (rectangle_overlap_ratio(bbox, table_bboxes[t]) >= 0.50) {
						overlaps_table = true;
						break;
					}
				}
				if (overlaps_table) {
					continue;
				}

				bool in_top = (bbox.y1 <= 0.10 * page_h || bbox.y0 <= 0.08 * page_h);
				bool in_bottom =
					(bbox.y0 >= 0.90 * page_h || bbox.y1 >= 0.92 * page_h);
				TextBuffer raw_blk_buf;
				init_buffer(&raw_blk_buf);
				for (fz_stext_line *line = blk->u.t.first_line; line;
				     line = line->next) {
					for (fz_stext_char *ch = line->first_char; ch;
					     ch = ch->next) {
						if (ch->flags & FZ_STEXT_UNICODE_IS_CID) {
							char cid[32];
							snprintf(cid, sizeof(cid), "(cid:%d)",
								 ch->c);
							append_string(&raw_blk_buf, cid);
						} else if (ch->c >= 0) {
							append_utf8_point(&raw_blk_buf,
									  (uint32_t)ch->c);
						}
					}
					if (line->next) {
						append_char(&raw_blk_buf, ' ');
					}
				}
				char *raw_blk_text = detach_buffer(&raw_blk_buf);
				trim_inplace(raw_blk_text);
				if (!*raw_blk_text) {
					free(raw_blk_text);
					continue;
				}

				char *tmpl = normalize_template(raw_blk_text);
				bool skip_chrome = false;
				if (*tmpl) {
					if (in_top) {
						TextBuffer k;
						init_buffer(&k);
						append_string(&k, "top|");
						append_string(&k, tmpl);
						if (get_text_count(&chrome_counts, k.data) >=
						    chrome_threshold) {
							skip_chrome = true;
						}
						free_buffer(&k);
					}
					if (!skip_chrome && in_bottom) {
						TextBuffer k;
						init_buffer(&k);
						append_string(&k, "bottom|");
						append_string(&k, tmpl);
						if (get_text_count(&chrome_counts, k.data) >=
						    chrome_threshold) {
							skip_chrome = true;
						}
						free_buffer(&k);
					}
				}
				free(tmpl);

				if (skip_chrome ||
				    ((in_top || in_bottom) &&
				     (match_page_number(raw_blk_text) ||
				      match_roman_page(raw_blk_text)))) {
					free(raw_blk_text);
					continue;
				}

				size_t blk_corrupt = 0, blk_raw = 0;
				count_corrupt_characters(raw_blk_text, &blk_corrupt, &blk_raw);
				free(raw_blk_text);
				if (blk_raw > 0 && ((double)blk_corrupt / (double)blk_raw) > 0.15) {
					continue;
				}

				LineRecord *lines = NULL;
				size_t line_count = 0;
				size_t line_cap = 0;
				for (fz_stext_line *line = blk->u.t.first_line; line;
				     line = line->next) {
					LineRecord rec;
					bool nonempty = false;
					analyze_stext_line(context, line, body_size, NULL, NULL,
							   &rec, &nonempty);
					if (nonempty) {
						if (line_count == line_cap) {
							line_cap = line_cap ? line_cap * 2 : 8;
							lines = (LineRecord *)reallocate_memory(
								lines,
								line_cap * sizeof(LineRecord));
						}
						lines[line_count++] = rec;
					}
				}
				if (line_count == 0) {
					free(lines);
					continue;
				}

				size_t clean_joined_len = 0;
				size_t mono_lines = 0;
				bool all_bold = true;
				double *sizes =
					(double *)allocate_memory(line_count * sizeof(double));
				for (size_t l = 0; l < line_count; ++l) {
					char *tr = duplicate_string(lines[l].norm_line);
					trim_inplace(tr);
					clean_joined_len += count_utf8_points(tr, strlen(tr));
					if (l > 0) {
						clean_joined_len += 1;
					}
					free(tr);
					if (lines[l].is_mono) {
						mono_lines++;
					}
					if (!lines[l].is_bold) {
						all_bold = false;
					}
					sizes[l] = lines[l].avg_size;
				}
				page_text_chars += clean_joined_len;

				/* Classify code block */
				if (((double)mono_lines / (double)line_count) >= 0.75) {
					TextBuffer code_buf;
					init_buffer(&code_buf);
					append_string(&code_buf, "```\n");
					size_t kept_code = 0;
					for (size_t l = 0; l < line_count; ++l) {
						if (!has_non_whitespace(lines[l].norm_line)) {
							continue;
						}
						char *rl = duplicate_string(lines[l].norm_line);
						size_t rlen = strlen(rl);
						while (rlen > 0 &&
						       (rl[rlen - 1] == ' ' ||
							rl[rlen - 1] == '\t' ||
							rl[rlen - 1] == '\r' ||
							rl[rlen - 1] == '\n')) {
							rl[--rlen] = '\0';
						}
						if (kept_code > 0) {
							append_char(&code_buf, '\n');
						}
						append_string(&code_buf, rl);
						free(rl);
						kept_code++;
					}
					append_string(&code_buf, "\n```");
					if (kept_code > 0) {
						push_box_rect(&code_bboxes, &code_bbox_count,
							      &code_bbox_cap, bbox);
						push_page_item(&page_items, bbox, BLOCK_CODE,
							       detach_buffer(&code_buf));
					} else {
						free_buffer(&code_buf);
					}
					free(sizes);
					for (size_t l = 0; l < line_count; ++l) {
						free(lines[l].norm_line);
					}
					free(lines);
					continue;
				}

				/* Median line size */
				qsort(sizes, line_count, sizeof(double), compare_double_asc);
				double dominant_size = (line_count % 2 == 1)
							       ? sizes[line_count / 2]
							       : 0.5 * (sizes[line_count / 2 - 1] +
									sizes[line_count / 2]);
				free(sizes);

				TextBuffer head_buf;
				init_buffer(&head_buf);
				for (size_t l = 0; l < line_count; ++l) {
					char *cline = clean_prose_line(lines[l].norm_line);
					if (l > 0) {
						append_char(&head_buf, ' ');
					}
					append_string(&head_buf, cline);
					free(cline);
				}
				char *heading_text = detach_buffer(&head_buf);
				trim_inplace(heading_text);
				size_t heading_cp_len =
					count_utf8_points(heading_text, strlen(heading_text));
				bool ends_punct = false;
				if (heading_text[0] != '\0') {
					char last = heading_text[strlen(heading_text) - 1];
					ends_punct = (last == '.' || last == ',' || last == ';' ||
						      last == ':');
				}
				bool is_heading =
					(heading_cp_len > 0) && (line_count <= 2) &&
					(heading_cp_len <= 140) && !match_caption(heading_text) &&
					(dominant_size >= body_size * 1.15 ||
					 (all_bold && heading_cp_len <= 80 && !ends_punct));
				if (is_heading) {
					TextBuffer h_out;
					init_buffer(&h_out);
					append_string(&h_out, (dominant_size >= body_size * 1.35)
								      ? "## "
								      : "### ");
					append_string(&h_out, heading_text);
					push_page_item(&page_items, bbox, BLOCK_HEADING,
						       detach_buffer(&h_out));
					free(heading_text);
					for (size_t l = 0; l < line_count; ++l) {
						free(lines[l].norm_line);
					}
					free(lines);
					continue;
				}
				free(heading_text);

				/* Reconstruct paragraphs with tight per-paragraph bboxes */
				push_box_rect(&prose_bboxes, &prose_bbox_count, &prose_bbox_cap,
					      bbox);

				typedef struct {
					BlockKind kind;
					char *text;
					BoxRect box;
				} TightPara;

				TightPara *paras = NULL;
				size_t para_count = 0;
				size_t para_cap = 0;
				char *current_para = NULL;
				BoxRect para_bbox = {0, 0, 0, 0};
				bool has_para_bbox = false;
				BoxRect prev_bbox = {0, 0, 0, 0};
				bool has_prev_bbox = false;
				double prev_height = fmax(body_size, 10.0);

				for (size_t l = 0; l < line_count; ++l) {
					char *cleaned = clean_prose_line(lines[l].norm_line);
					if (!*cleaned) {
						free(cleaned);
						continue;
					}
					BoxRect lbbox = lines[l].bbox;
					if (is_math_display(cleaned) && line_count <= 3) {
						if (current_para && has_para_bbox) {
							if (para_count == para_cap) {
								para_cap = para_cap ? para_cap * 2
										    : 8;
								paras = (TightPara *)reallocate_memory(
									paras,
									para_cap *
										sizeof(TightPara));
							}
							paras[para_count++] = (TightPara){
								BLOCK_PROSE, current_para,
								para_bbox};
							current_para = NULL;
							has_para_bbox = false;
						}
						TextBuffer mb;
						init_buffer(&mb);
						append_string(&mb, "$$\n");
						append_string(&mb, cleaned);
						append_string(&mb, "\n$$");
						free(cleaned);
						if (para_count == para_cap) {
							para_cap = para_cap ? para_cap * 2 : 8;
							paras = (TightPara *)reallocate_memory(
								paras,
								para_cap * sizeof(TightPara));
						}
						paras[para_count++] = (TightPara){
							BLOCK_MATH, detach_buffer(&mb), lbbox};
						prev_bbox = lbbox;
						has_prev_bbox = true;
						continue;
					}

					double line_h = fmax(fmax(lbbox.y1 - lbbox.y0,
								  lines[l].avg_size),
							     8.0);
					bool starts_list = match_list_item(cleaned);
					if (has_prev_bbox && current_para && has_para_bbox) {
						double v_gap = lbbox.y0 - prev_bbox.y1;
						double indent_shift = lbbox.x0 - prev_bbox.x0;
						bool same_row_split =
							(fabs(lbbox.y0 - prev_bbox.y0) <
							 0.5 * prev_height) &&
							((lbbox.x0 - prev_bbox.x1) > 18.0);
						char last_ch =
							current_para[strlen(current_para) - 1];
						bool ends_sent =
							(last_ch == '.' || last_ch == '!' ||
							 last_ch == '?' || last_ch == ':' ||
							 last_ch == '"' || last_ch == ')');
						bool is_para_break = starts_list || same_row_split ||
								     (v_gap > 1.3 * prev_height) ||
								     (indent_shift > 10.0 &&
								      ends_sent);
						if (is_para_break) {
							if (para_count == para_cap) {
								para_cap = para_cap ? para_cap * 2
										    : 8;
								paras = (TightPara *)reallocate_memory(
									paras,
									para_cap *
										sizeof(TightPara));
							}
							paras[para_count++] = (TightPara){
								BLOCK_PROSE, current_para,
								para_bbox};
							current_para = cleaned;
							para_bbox = lbbox;
							has_para_bbox = true;
						} else {
							char *joined = dehyphenate_join(
								current_para, cleaned);
							free(current_para);
							free(cleaned);
							current_para = joined;
							para_bbox.x0 = fmin(para_bbox.x0, lbbox.x0);
							para_bbox.y0 = fmin(para_bbox.y0, lbbox.y0);
							para_bbox.x1 = fmax(para_bbox.x1, lbbox.x1);
							para_bbox.y1 = fmax(para_bbox.y1, lbbox.y1);
						}
					} else {
						free(current_para);
						current_para = cleaned;
						para_bbox = lbbox;
						has_para_bbox = true;
					}
					prev_bbox = lbbox;
					has_prev_bbox = true;
					prev_height = line_h;
				}
				if (current_para && has_para_bbox) {
					if (para_count == para_cap) {
						para_cap = para_cap ? para_cap * 2 : 8;
						paras = (TightPara *)reallocate_memory(
							paras, para_cap * sizeof(TightPara));
					}
					paras[para_count++] = (TightPara){BLOCK_PROSE, current_para,
									  para_bbox};
					current_para = NULL;
				}
				free(current_para);

				for (size_t p = 0; p < para_count; ++p) {
					if (paras[p].kind == BLOCK_PROSE) {
						if (caption_count == caption_cap) {
							caption_cap =
								caption_cap ? caption_cap * 2 : 16;
							caption_pool =
								(CaptionCandidate *)reallocate_memory(
									caption_pool,
									caption_cap *
										sizeof(CaptionCandidate));
						}
						caption_pool[caption_count].bbox = paras[p].box;
						caption_pool[caption_count].text =
							duplicate_string(paras[p].text);
						caption_count++;
					}
					push_page_item(&page_items, paras[p].box, paras[p].kind,
						       paras[p].text);
				}
				free(paras);
				for (size_t l = 0; l < line_count; ++l) {
					free(lines[l].norm_line);
				}
				free(lines);
			}

			/* 3. Extract & filter raster images with interleaved Markdown captions */
			bool has_large_raster = false;
			if (extract_images && pdf_doc) {
				pdf_page *ppage = pdf_page_from_fz_page(context, page);
				PdfImageRef *refs = NULL;
				size_t ref_count = collect_page_image_refs(context, ppage, &refs);
				int fig_seq = 0;

				for (size_t i = 0; i < ref_count; ++i) {
					int xref = refs[i].xref;
					int smask = refs[i].smask;

					bool is_repeated = false;
					for (size_t j = 0; j < xref_stat_count; ++j) {
						if (xref_stats[j].xref == xref &&
						    xref_stats[j].count >= DOCUMENT_LOGO_PAGES) {
							is_repeated = true;
							break;
						}
					}
					if (is_repeated) {
						continue;
					}
					bool already_seen = false;
					for (size_t j = 0; j < seen_xref_count; ++j) {
						if (seen_xrefs[j] == xref) {
							already_seen = true;
							break;
						}
					}
					if (already_seen) {
						continue;
					}

					if (seen_xref_count == seen_xref_cap) {
						seen_xref_cap =
							seen_xref_cap ? seen_xref_cap * 2 : 32;
						seen_xrefs = (int *)reallocate_memory(
							seen_xrefs, seen_xref_cap * sizeof(int));
					}
					seen_xrefs[seen_xref_count++] = xref;
					fig_seq++;

					fz_image *fz_img = NULL;
					fz_image *mask_img = NULL;
					fz_pixmap *pix = NULL;
					fz_pixmap *mask_pix = NULL;
					unsigned char img_digest[16] = {0};
					bool got_digest = false;
					SavePngStatus save_status = PNG_DECODE_ERROR;

					fz_var(fz_img);
					fz_var(mask_img);
					fz_var(pix);
					fz_var(mask_pix);
					fz_var(got_digest);
					fz_var(save_status);

					char fig_name[512];
					snprintf(fig_name, sizeof(fig_name), "%s_p%d_fig%d.png",
						 stem, page_idx + 1, fig_seq);
					char *png_dest = join_path(images_dir, fig_name);

					fz_try(context) {
						pdf_obj *img_obj =
							pdf_new_indirect(context, pdf_doc, xref, 0);
						fz_img = pdf_load_image(context, pdf_doc, img_obj);
						pdf_drop_obj(context, img_obj);
						fz_image_digest(context, fz_img, img_digest);
						got_digest = true;

						pix = fz_get_pixmap_from_image(context, fz_img,
									       NULL, NULL, NULL,
									       NULL);
						if (fz_pixmap_components(context, pix) -
							    fz_pixmap_alpha(context, pix) >=
						    4) {
							fz_pixmap *conv = fz_convert_pixmap(
								context, pix, fz_device_rgb(context),
								NULL, NULL, DEFAULT_COLOR_PARAMS,
								fz_pixmap_alpha(context, pix));
							fz_drop_pixmap(context, pix);
							pix = conv;
						}
						if (smask > 0 && fz_pixmap_alpha(context, pix) == 0) {
							fz_try(context) {
								pdf_obj *mobj = pdf_new_indirect(
									context, pdf_doc, smask, 0);
								mask_img = pdf_load_image(
									context, pdf_doc, mobj);
								pdf_drop_obj(context, mobj);
								mask_pix =
									fz_get_pixmap_from_image(
										context, mask_img,
										NULL, NULL, NULL,
										NULL);
								if (fz_pixmap_components(
									    context, mask_pix) !=
									    1 ||
								    fz_pixmap_alpha(context,
										    mask_pix) !=
									    0) {
									fz_pixmap *gmask =
										fz_convert_pixmap(
											context,
											mask_pix,
											fz_device_gray(
												context),
											NULL, NULL,
											DEFAULT_COLOR_PARAMS,
											0);
									fz_drop_pixmap(context,
										       mask_pix);
									mask_pix = gmask;
								}
								if (fz_pixmap_width(context,
										    mask_pix) ==
									    fz_pixmap_width(context,
											    pix) &&
								    fz_pixmap_height(context,
										     mask_pix) ==
									    fz_pixmap_height(
										    context, pix)) {
									fz_pixmap *masked =
										fz_new_pixmap_from_color_and_mask(
											context,
											pix,
											mask_pix);
									fz_drop_pixmap(context,
										       pix);
									pix = masked;
								}
							}
							fz_catch(context) {
								/* Ignore broken SMask */
							}
						}
						save_status = save_filtered_pixmap(
							context, pix, png_dest,
							&out_doc->image_dhashes, corpus_state,
							&out_doc->image_digests, enable_dedup,
							MIN_IMAGE_SIZE);
					}
					fz_always(context) {
						if (mask_pix) {
							fz_drop_pixmap(context, mask_pix);
						}
						if (mask_img) {
							fz_drop_image(context, mask_img);
						}
						if (pix) {
							fz_drop_pixmap(context, pix);
						}
						if (fz_img) {
							fz_drop_image(context, fz_img);
						}
					}
					fz_catch(context) {
						save_status = PNG_REJECTED;
					}

					BoxRect ibbox = {0.0, page_h * 0.5, page_w, page_h * 0.5};
					if (got_digest) {
						for (size_t k = 0; k < collect->image_count; ++k) {
							if (memcmp(img_digest,
								   collect->placed_images[k].digest,
								   16) == 0) {
								ibbox = collect->placed_images[k]
										.bbox;
								double area =
									(ibbox.x1 - ibbox.x0) *
									(ibbox.y1 - ibbox.y0);
								if (area >= 0.60 * page_w * page_h) {
									has_large_raster = true;
								}
								break;
							}
						}
					}

					if (save_status == PNG_SAVED) {
						out_doc->image_count++;
						push_text_owned(&out_doc->saved_images,
								duplicate_string(png_dest));
						push_box_rect(&image_bboxes, &image_bbox_count,
							      &image_bbox_cap, ibbox);
						char *caption = find_nearest_caption(
							ibbox, caption_pool, caption_count);
						char *rel_link =
							compute_relative_path(png_dest, md_dir);
						TextBuffer md_img;
						init_buffer(&md_img);
						append_string(&md_img, "![");
						if (*caption) {
							append_string(&md_img, caption);
						} else {
							char fallback_alt[512];
							snprintf(fallback_alt, sizeof(fallback_alt),
								 "Figure %d from %s (page %d)",
								 fig_seq, stem, page_idx + 1);
							append_string(&md_img, fallback_alt);
						}
						append_string(&md_img, "](");
						append_string(&md_img, rel_link);
						append_char(&md_img, ')');
						push_page_item(&page_items, ibbox, BLOCK_IMAGE,
							       detach_buffer(&md_img));
						free(caption);
						free(rel_link);
					}
					free(png_dest);
				}
				free(refs);

				/* 4. Tier 2: Capture vector diagrams/charts via path collector */
				if (capture_vectors) {
					size_t excluded_total = table_count + image_bbox_count +
								code_bbox_count;
					BoxRect *excluded = (BoxRect *)allocate_memory(
						(excluded_total + 1) * sizeof(BoxRect));
					size_t ex_pos = 0;
					for (size_t k = 0; k < table_count; ++k) {
						excluded[ex_pos++] = table_bboxes[k];
					}
					for (size_t k = 0; k < image_bbox_count; ++k) {
						excluded[ex_pos++] = image_bboxes[k];
					}
					for (size_t k = 0; k < code_bbox_count; ++k) {
						excluded[ex_pos++] = code_bboxes[k];
					}

					BoxRect *vec_clusters = NULL;
					size_t vec_count = cluster_vector_drawings(
						collect->drawing_rects, collect->drawing_count,
						page_w, page_h, excluded, excluded_total,
						prose_bboxes, prose_bbox_count, &vec_clusters);
					free(excluded);

					for (size_t v = 0; v < vec_count; ++v) {
						BoxRect vbbox = vec_clusters[v];
						char vec_name[512];
						snprintf(vec_name, sizeof(vec_name),
							 "%s_p%d_vec%zu.png", stem, page_idx + 1,
							 v + 1);
						char *png_dest = join_path(images_dir, vec_name);
						fz_pixmap *vpix = NULL;
						fz_device *draw_dev = NULL;
						SavePngStatus vstatus = PNG_REJECTED;
						fz_var(vpix);
						fz_var(draw_dev);
						fz_var(vstatus);

						fz_try(context) {
							float scale = (float)VECTOR_DPI / 72.0f;
							fz_matrix ctm = fz_scale(scale, scale);
							fz_rect clip_rect = fz_make_rect(
								(float)vbbox.x0, (float)vbbox.y0,
								(float)vbbox.x1, (float)vbbox.y1);
							fz_rect scaled =
								fz_transform_rect(clip_rect, ctm);
							fz_irect clip_ib = fz_round_rect(scaled);
							if (!fz_is_empty_irect(clip_ib)) {
								vpix = fz_new_pixmap_with_bbox(
									context,
									fz_device_rgb(context),
									clip_ib, NULL, 0);
								fz_clear_pixmap_with_value(
									context, vpix, 255);
								draw_dev =
									fz_new_draw_device_with_bbox(
										context, ctm, vpix,
										&clip_ib);
								fz_run_page(
									context, page, draw_dev,
									fz_make_matrix(1.0f, 0.0f,
										       0.0f, 1.0f,
										       0.0f, 0.0f),
									NULL);
								fz_close_device(context, draw_dev);
								vstatus = save_filtered_pixmap(
									context, vpix, png_dest,
									&out_doc->image_dhashes,
									corpus_state,
									&out_doc->image_digests,
									enable_dedup,
									MIN_IMAGE_SIZE);
							}
						}
						fz_always(context) {
							if (draw_dev) {
								fz_drop_device(context, draw_dev);
							}
							if (vpix) {
								fz_drop_pixmap(context, vpix);
							}
						}
						fz_catch(context) {
							vstatus = PNG_REJECTED;
						}

						if (vstatus == PNG_SAVED) {
							out_doc->image_count++;
							push_text_owned(&out_doc->saved_images,
									duplicate_string(png_dest));
							char *caption = find_nearest_caption(
								vbbox, caption_pool, caption_count);
							char *rel_link = compute_relative_path(
								png_dest, md_dir);
							TextBuffer md_vec;
							init_buffer(&md_vec);
							append_string(&md_vec, "![");
							if (*caption) {
								append_string(&md_vec, caption);
							} else {
								char fallback_alt[512];
								snprintf(fallback_alt,
									 sizeof(fallback_alt),
									 "Diagram %zu from %s "
									 "(page %d)",
									 v + 1, stem, page_idx + 1);
								append_string(&md_vec,
									      fallback_alt);
							}
							append_string(&md_vec, "](");
							append_string(&md_vec, rel_link);
							append_char(&md_vec, ')');
							push_page_item(&page_items, vbbox,
								       BLOCK_IMAGE,
								       detach_buffer(&md_vec));
							free(caption);
							free(rel_link);
						}
						free(png_dest);
					}
					free(vec_clusters);
				}
			} else if (page_text_chars < 50) {
				double page_area = page_w * page_h;
				for (size_t k = 0; k < collect->image_count; ++k) {
					BoxRect ib = collect->placed_images[k].bbox;
					if (((ib.x1 - ib.x0) * (ib.y1 - ib.y0)) >=
					    0.60 * page_area) {
						has_large_raster = true;
						break;
					}
				}
			}

			if (page_text_chars < 50 && has_large_raster) {
				out_doc->scanned_pages++;
			}

			sort_reading_order(&page_items, page_w);
			for (size_t i = 0; i < page_items.count; ++i) {
				PageItem *item = &page_items.items[i];
				bool merge_with_prev = false;
				if (item->kind == BLOCK_PROSE && out_doc->blocks.count > 0 &&
				    out_doc->blocks.items[out_doc->blocks.count - 1].kind ==
					    BLOCK_PROSE &&
				    !match_list_item(item->content) && item->content[0] != '\0') {
					const char *prev_text =
						out_doc->blocks.items[out_doc->blocks.count - 1]
							.text;
					size_t plen = strlen(prev_text);
					char plast = plen > 0 ? prev_text[plen - 1] : '\0';
					bool ends_stop = (plast == '.' || plast == '!' ||
							  plast == '?' || plast == ':' ||
							  plast == '"' || plast == ')' ||
							  plast == ']');
					uint32_t first_cp = 0;
					decode_utf8_point(item->content, strlen(item->content), 0,
							  &first_cp);
					if (!ends_stop && is_unicode_lower(first_cp)) {
						merge_with_prev = true;
					}
				}
				if (merge_with_prev) {
					ContentBlock *prev_blk =
						&out_doc->blocks.items[out_doc->blocks.count - 1];
					char *merged =
						dehyphenate_join(prev_blk->text, item->content);
					free(prev_blk->text);
					prev_blk->text = merged;
				} else {
					push_block(&out_doc->blocks, item->kind,
						   duplicate_string(item->content));
				}
			}
		}
		fz_always(context) {
			if (collect) {
				fz_drop_device(context, &collect->super);
			}
			if (stext) {
				fz_drop_stext_page(context, stext);
			}
			if (page) {
				fz_drop_page(context, page);
			}
		}
		fz_catch(context) {
			/* Skip broken page cleanly */
		}

		free_page_items(&page_items);
		free(table_bboxes);
		free(image_bboxes);
		free(code_bboxes);
		free(prose_bboxes);
		for (size_t c = 0; c < caption_count; ++c) {
			free(caption_pool[c].text);
		}
		free(caption_pool);
	}

	out_doc->corrupt_characters = total_corrupt;
	out_doc->raw_characters = total_raw;

	free(seen_xrefs);
	free(xref_stats);
	free_text_counter(&chrome_counts);
	fz_drop_document(context, fz_doc);
}

/* ========================================================================= */
/* Section 9: DOCX, Plain Text, Quality Gate, MinHash LSH, Output & CLI      */
/* ========================================================================= */

static char *normalize_docx_target(const char *raw_target) {
	if (!raw_target || !*raw_target) {
		return duplicate_string("");
	}
	if (raw_target[0] == '/') {
		const char *p = raw_target;
		while (*p == '/') {
			p++;
		}
		return duplicate_string(p);
	}
	TextBuffer combined;
	init_buffer(&combined);
	append_string(&combined, "word/");
	append_string(&combined, raw_target);

	const char *parts[64];
	size_t lens[64];
	int count = 0;
	const char *scan = combined.data;
	while (*scan) {
		while (*scan == '/') {
			scan++;
		}
		if (!*scan) {
			break;
		}
		const char *end = strchr(scan, '/');
		size_t seg_len = end ? (size_t)(end - scan) : strlen(scan);
		if (seg_len == 1 && scan[0] == '.') {
			/* skip */
		} else if (seg_len == 2 && scan[0] == '.' && scan[1] == '.') {
			if (count > 0) {
				count--;
			}
		} else if (count < 64) {
			parts[count] = scan;
			lens[count] = seg_len;
			count++;
		}
		scan += seg_len;
	}
	TextBuffer out;
	init_buffer(&out);
	for (int i = 0; i < count; ++i) {
		if (i > 0) {
			append_char(&out, '/');
		}
		append_bytes(&out, parts[i], lens[i]);
	}
	free_buffer(&combined);
	return detach_buffer(&out);
}

static void collect_xml_wt_text(fz_xml *node, TextBuffer *out) {
	if (!node) {
		return;
	}
	for (fz_xml *child = fz_xml_down(node); child; child = fz_xml_next(child)) {
		const char *tag = fz_xml_tag(child);
		if (tag && strcmp(tag, "t") == 0) {
			for (fz_xml *tchild = fz_xml_down(child); tchild;
			     tchild = fz_xml_next(tchild)) {
				const char *txt = fz_xml_text(tchild);
				if (txt) {
					append_string(out, txt);
				}
			}
		} else if (tag) {
			collect_xml_wt_text(child, out);
		}
	}
}

static void collect_xml_blips_and_fonts(fz_xml *node, TextList *rids, TextList *fonts) {
	if (!node) {
		return;
	}
	for (fz_xml *child = fz_xml_down(node); child; child = fz_xml_next(child)) {
		const char *tag = fz_xml_tag(child);
		if (!tag) {
			continue;
		}
		if (rids && strcmp(tag, "blip") == 0) {
			const char *rid = fz_xml_att_alt(child, "r:embed", "embed");
			if (rid && *rid) {
				push_text_copy(rids, rid);
			}
		} else if (fonts && strcmp(tag, "rFonts") == 0) {
			const char *fname = fz_xml_att_alt(child, "w:ascii", "ascii");
			if (!fname || !*fname) {
				fname = fz_xml_att_alt(child, "w:hAnsi", "hAnsi");
			}
			if (fname && *fname) {
				push_text_copy(fonts, fname);
			}
		}
		collect_xml_blips_and_fonts(child, rids, fonts);
	}
}

static void flush_docx_code_buffer(TextList *code_buffer, BlockList *blocks) {
	if (code_buffer->count == 0) {
		return;
	}
	TextBuffer out;
	init_buffer(&out);
	append_string(&out, "```\n");
	for (size_t i = 0; i < code_buffer->count; ++i) {
		if (i > 0) {
			append_char(&out, '\n');
		}
		append_string(&out, code_buffer->items[i]);
	}
	append_string(&out, "\n```");
	push_block(blocks, BLOCK_CODE, detach_buffer(&out));
	free_text_list(code_buffer);
	init_text_list(code_buffer);
}

static void extract_docx_document(fz_context *context, const char *docx_path, const char *stem,
                                  const char *images_dir, const char *md_dir,
                                  bool extract_images, bool enable_dedup,
                                  CorpusState *corpus_state, ExtractedDocument *out_doc) {
	init_extracted_doc(out_doc, "docx");

	typedef struct {
		char *rid;
		char *target;
	} RelEntry;

	typedef struct {
		char *media_path;
		char *png_dest;
	} SavedMediaEntry;

	fz_archive *arch = NULL;
	fz_buffer *rels_buf = NULL;
	fz_xml_doc *rels_xml = NULL;
	fz_buffer *doc_buf = NULL;
	fz_xml_doc *doc_xml = NULL;

	RelEntry *rels = NULL;
	size_t rel_count = 0;
	size_t rel_cap = 0;
	SavedMediaEntry *saved_media = NULL;
	size_t saved_media_count = 0;
	size_t saved_media_cap = 0;

	size_t total_corrupt = 0;
	size_t total_raw = 0;

	fz_var(arch);
	fz_var(rels_buf);
	fz_var(rels_xml);
	fz_var(doc_buf);
	fz_var(doc_xml);

	fz_try(context) {
		arch = fz_open_archive(context, docx_path);
		if (fz_has_archive_entry(context, arch, "word/_rels/document.xml.rels")) {
			rels_buf = fz_read_archive_entry(context, arch,
							 "word/_rels/document.xml.rels");
			rels_xml = fz_parse_xml(context, rels_buf, 1);
			fz_xml *rroot = fz_xml_root(rels_xml);
			for (fz_xml *node = rroot ? fz_xml_down(rroot) : NULL; node;
			     node = fz_xml_next(node)) {
				const char *rid = fz_xml_att(node, "Id");
				const char *target = fz_xml_att(node, "Target");
				if (rid && *rid && target && *target) {
					if (rel_count == rel_cap) {
						rel_cap = rel_cap ? rel_cap * 2 : 16;
						rels = (RelEntry *)reallocate_memory(
							rels, rel_cap * sizeof(RelEntry));
					}
					rels[rel_count].rid = duplicate_string(rid);
					rels[rel_count].target = normalize_docx_target(target);
					rel_count++;
				}
			}
		}

		if (fz_has_archive_entry(context, arch, "word/document.xml")) {
			doc_buf = fz_read_archive_entry(context, arch, "word/document.xml");
			doc_xml = fz_parse_xml(context, doc_buf, 1);
			fz_xml *root = fz_xml_root(doc_xml);
			fz_xml *body = root ? fz_xml_find_down(root, "body") : NULL;
			if (body) {
				TextList code_buffer;
				init_text_list(&code_buffer);

				for (fz_xml *child = fz_xml_down(body); child;
				     child = fz_xml_next(child)) {
					const char *tag = fz_xml_tag(child);
					if (!tag) {
						continue;
					}
					if (strcmp(tag, "tbl") == 0) {
						flush_docx_code_buffer(&code_buffer,
								       &out_doc->blocks);
						TableRows table;
						init_table_rows(&table);
						for (fz_xml *tr = fz_xml_down(child); tr;
						     tr = fz_xml_next(tr)) {
							const char *tr_tag = fz_xml_tag(tr);
							if (!tr_tag || strcmp(tr_tag, "tr") != 0) {
								continue;
							}
							TextList row_cells;
							init_text_list(&row_cells);
							for (fz_xml *tc = fz_xml_down(tr); tc;
							     tc = fz_xml_next(tc)) {
								const char *tc_tag = fz_xml_tag(tc);
								if (!tc_tag ||
								    strcmp(tc_tag, "tc") != 0) {
									continue;
								}
								TextBuffer cell_text;
								init_buffer(&cell_text);
								collect_xml_wt_text(tc, &cell_text);
								push_text_owned(
									&row_cells,
									detach_buffer(&cell_text));
							}
							push_table_row(&table, row_cells);
						}
						char *md_table = format_markdown_table(&table);
						if (*md_table) {
							push_block(&out_doc->blocks, BLOCK_TABLE,
								   md_table);
						} else {
							free(md_table);
						}
						free_table_rows(&table);
						continue;
					}

					if (strcmp(tag, "p") != 0) {
						continue;
					}

					TextList blips;
					TextList fonts;
					init_text_list(&blips);
					init_text_list(&fonts);
					collect_xml_blips_and_fonts(
						child, extract_images ? &blips : NULL, &fonts);

					if (extract_images) {
						for (size_t b = 0; b < blips.count; ++b) {
							const char *rid = blips.items[b];
							const char *media_path = NULL;
							for (size_t r = 0; r < rel_count; ++r) {
								if (strcmp(rels[r].rid, rid) == 0) {
									media_path = rels[r].target;
									break;
								}
							}
							if (!media_path ||
							    !fz_has_archive_entry(context, arch,
										  media_path)) {
								continue;
							}
							const char *existing_png = NULL;
							for (size_t sm = 0; sm < saved_media_count;
							     ++sm) {
								if (strcmp(saved_media[sm]
										   .media_path,
									   media_path) == 0) {
									existing_png =
										saved_media[sm]
											.png_dest;
									break;
								}
							}
							if (!existing_png) {
								char fig_name[512];
								snprintf(fig_name, sizeof(fig_name),
									 "%s_fig%zu.png", stem,
									 saved_media_count + 1);
								char *png_dest = join_path(
									images_dir, fig_name);
								fz_buffer *mbuf =
									fz_read_archive_entry(
										context, arch,
										media_path);
								unsigned char *mdata = NULL;
								size_t mlen = fz_buffer_storage(
									context, mbuf, &mdata);
								SavePngStatus st = save_filtered_png(
									context, mdata, mlen,
									png_dest,
									&out_doc->image_dhashes,
									corpus_state,
									&out_doc->image_digests,
									enable_dedup,
									MIN_IMAGE_SIZE);
								fz_drop_buffer(context, mbuf);
								if (st == PNG_SAVED) {
									if (saved_media_count ==
									    saved_media_cap) {
										saved_media_cap =
											saved_media_cap
												? saved_media_cap *
													  2
												: 16;
										saved_media = (SavedMediaEntry *)
											reallocate_memory(
												saved_media,
												saved_media_cap *
													sizeof(SavedMediaEntry));
									}
									saved_media
										[saved_media_count]
											.media_path =
											duplicate_string(
												media_path);
									saved_media
										[saved_media_count]
											.png_dest =
											duplicate_string(
												png_dest);
									existing_png =
										saved_media
											[saved_media_count]
												.png_dest;
									saved_media_count++;
									push_text_owned(
										&out_doc->saved_images,
										duplicate_string(
											png_dest));
									out_doc->image_count++;
								}
								free(png_dest);
							}
							if (existing_png) {
								flush_docx_code_buffer(
									&code_buffer,
									&out_doc->blocks);
								char *rel_link =
									compute_relative_path(
										existing_png,
										md_dir);
								TextBuffer img_md;
								init_buffer(&img_md);
								append_string(&img_md,
									      "![Figure from ");
								append_string(&img_md, stem);
								append_string(&img_md, "](");
								append_string(&img_md, rel_link);
								append_char(&img_md, ')');
								push_block(&out_doc->blocks,
									   BLOCK_IMAGE,
									   detach_buffer(&img_md));
								free(rel_link);
							}
						}
					}
					free_text_list(&blips);

					TextBuffer raw_p_buf;
					init_buffer(&raw_p_buf);
					collect_xml_wt_text(child, &raw_p_buf);
					char *raw_text = detach_buffer(&raw_p_buf);
					size_t rc = 0, rr = 0;
					count_corrupt_characters(raw_text, &rc, &rr);
					total_corrupt += rc;
					total_raw += rr;

					if (!has_non_whitespace(raw_text)) {
						flush_docx_code_buffer(&code_buffer,
								       &out_doc->blocks);
						free(raw_text);
						free_text_list(&fonts);
						continue;
					}

					const char *style_val = "";
					bool is_list = false;
					fz_xml *ppr = fz_xml_find_down(child, "pPr");
					if (ppr) {
						fz_xml *pstyle = fz_xml_find_down(ppr, "pStyle");
						if (pstyle) {
							const char *v = fz_xml_att_alt(
								pstyle, "w:val", "val");
							if (v) {
								style_val = v;
							}
						}
						if (fz_xml_find_down(ppr, "numPr") != NULL) {
							is_list = true;
						}
					}

					bool is_code_para = fonts.count > 0;
					for (size_t f = 0; f < fonts.count; ++f) {
						if (!is_monospace_span(fonts.items[f], 0)) {
							is_code_para = false;
							break;
						}
					}
					free_text_list(&fonts);

					if (is_code_para ||
					    contains_ignore_case(style_val, "code")) {
						char *norm = normalize_unicode(raw_text);
						size_t nlen = strlen(norm);
						while (nlen > 0 &&
						       (norm[nlen - 1] == ' ' ||
							norm[nlen - 1] == '\t' ||
							norm[nlen - 1] == '\r' ||
							norm[nlen - 1] == '\n')) {
							norm[--nlen] = '\0';
						}
						push_text_owned(&code_buffer, norm);
						free(raw_text);
						continue;
					}

					flush_docx_code_buffer(&code_buffer, &out_doc->blocks);
					char *cleaned = clean_prose_line(raw_text);
					free(raw_text);
					if (!*cleaned) {
						free(cleaned);
						continue;
					}

					if (equals_ignore_case(style_val, "title")) {
						TextBuffer h;
						init_buffer(&h);
						append_string(&h, "# ");
						append_string(&h, cleaned);
						push_block(&out_doc->blocks, BLOCK_HEADING,
							   detach_buffer(&h));
						free(cleaned);
					} else if (fz_strncasecmp(style_val, "Heading", 7) == 0 &&
						   isdigit((unsigned char)style_val[7])) {
						int level = style_val[7] - '0';
						if (level < 1) {
							level = 1;
						}
						if (level > 6) {
							level = 6;
						}
						TextBuffer h;
						init_buffer(&h);
						for (int k = 0; k < level; ++k) {
							append_char(&h, '#');
						}
						append_char(&h, ' ');
						append_string(&h, cleaned);
						push_block(&out_doc->blocks, BLOCK_HEADING,
							   detach_buffer(&h));
						free(cleaned);
					} else if (is_list) {
						TextBuffer li;
						init_buffer(&li);
						append_string(&li, "- ");
						append_string(&li, cleaned);
						push_block(&out_doc->blocks, BLOCK_PROSE,
							   detach_buffer(&li));
						free(cleaned);
					} else if (is_math_display(cleaned)) {
						TextBuffer mb;
						init_buffer(&mb);
						append_string(&mb, "$$\n");
						append_string(&mb, cleaned);
						append_string(&mb, "\n$$");
						push_block(&out_doc->blocks, BLOCK_MATH,
							   detach_buffer(&mb));
						free(cleaned);
					} else {
						push_block(&out_doc->blocks, BLOCK_PROSE, cleaned);
					}
				}
				flush_docx_code_buffer(&code_buffer, &out_doc->blocks);
				free_text_list(&code_buffer);
			}
		}
		out_doc->page_count = 1;
		out_doc->corrupt_characters = total_corrupt;
		out_doc->raw_characters = total_raw;
	}
	fz_always(context) {
		if (doc_xml) {
			fz_drop_xml(context, doc_xml);
		}
		if (doc_buf) {
			fz_drop_buffer(context, doc_buf);
		}
		if (rels_xml) {
			fz_drop_xml(context, rels_xml);
		}
		if (rels_buf) {
			fz_drop_buffer(context, rels_buf);
		}
		if (arch) {
			fz_drop_archive(context, arch);
		}
	}
	fz_catch(context) {
		fprintf(stderr, "  [!] Failed to read DOCX %s: %s\n", path_basename_ptr(docx_path),
			fz_caught_message(context));
		free_extracted_doc(out_doc);
		init_extracted_doc(out_doc, "docx");
	}

	for (size_t i = 0; i < rel_count; ++i) {
		free(rels[i].rid);
		free(rels[i].target);
	}
	free(rels);
	for (size_t i = 0; i < saved_media_count; ++i) {
		free(saved_media[i].media_path);
		free(saved_media[i].png_dest);
	}
	free(saved_media);
}

static void flush_plain_paragraph(TextList *para_lines, BlockList *blocks) {
	if (para_lines->count == 0) {
		return;
	}
	char *merged = duplicate_string(para_lines->items[0]);
	for (size_t i = 1; i < para_lines->count; ++i) {
		char *joined = dehyphenate_join(merged, para_lines->items[i]);
		free(merged);
		merged = joined;
	}
	free_text_list(para_lines);
	init_text_list(para_lines);

	if (is_math_display(merged)) {
		TextBuffer mb;
		init_buffer(&mb);
		append_string(&mb, "$$\n");
		append_string(&mb, merged);
		append_string(&mb, "\n$$");
		free(merged);
		push_block(blocks, BLOCK_MATH, detach_buffer(&mb));
	} else {
		push_block(blocks, BLOCK_PROSE, merged);
	}
}

static void reconstruct_plain_blocks(const char *raw_text, BlockList *blocks) {
	char *normalized = normalize_unicode(raw_text ? raw_text : "");
	bool in_fence = false;
	TextList fence_lines;
	TextList para_lines;
	init_text_list(&fence_lines);
	init_text_list(&para_lines);

	const char *cursor = normalized;
	while (*cursor) {
		const char *eol = strchr(cursor, '\n');
		size_t line_len = eol ? (size_t)(eol - cursor) : strlen(cursor);
		char *raw_line = duplicate_range(cursor, line_len);
		cursor = eol ? (eol + 1) : (cursor + line_len);

		char *stripped = duplicate_string(raw_line);
		trim_inplace(stripped);

		if (starts_with_str(stripped, "```")) {
			flush_plain_paragraph(&para_lines, blocks);
			if (in_fence) {
				push_text_owned(&fence_lines, stripped);
				TextBuffer fb;
				init_buffer(&fb);
				for (size_t i = 0; i < fence_lines.count; ++i) {
					if (i > 0) {
						append_char(&fb, '\n');
					}
					append_string(&fb, fence_lines.items[i]);
				}
				push_block(blocks, BLOCK_CODE, detach_buffer(&fb));
				free_text_list(&fence_lines);
				init_text_list(&fence_lines);
				in_fence = false;
			} else {
				in_fence = true;
				free_text_list(&fence_lines);
				init_text_list(&fence_lines);
				push_text_owned(&fence_lines, stripped);
			}
			free(raw_line);
			continue;
		}

		if (in_fence) {
			free(stripped);
			size_t rlen = strlen(raw_line);
			while (rlen > 0 &&
			       (raw_line[rlen - 1] == ' ' || raw_line[rlen - 1] == '\t' ||
				raw_line[rlen - 1] == '\r' || raw_line[rlen - 1] == '\n')) {
				raw_line[--rlen] = '\0';
			}
			push_text_owned(&fence_lines, raw_line);
			continue;
		}

		if (!*stripped) {
			flush_plain_paragraph(&para_lines, blocks);
			free(stripped);
			free(raw_line);
			continue;
		}

		if (stripped[0] == '#') {
			flush_plain_paragraph(&para_lines, blocks);
			push_block(blocks, BLOCK_HEADING, stripped);
			free(raw_line);
			continue;
		}

		if (stripped[0] == '|' && stripped[strlen(stripped) - 1] == '|') {
			flush_plain_paragraph(&para_lines, blocks);
			if (blocks->count > 0 &&
			    blocks->items[blocks->count - 1].kind == BLOCK_TABLE) {
				TextBuffer tb;
				init_buffer(&tb);
				append_string(&tb, blocks->items[blocks->count - 1].text);
				append_char(&tb, '\n');
				append_string(&tb, stripped);
				free(blocks->items[blocks->count - 1].text);
				blocks->items[blocks->count - 1].text = detach_buffer(&tb);
				free(stripped);
			} else {
				push_block(blocks, BLOCK_TABLE, stripped);
			}
			free(raw_line);
			continue;
		}
		free(stripped);

		char *cleaned = clean_prose_line(raw_line);
		free(raw_line);
		if (!*cleaned) {
			free(cleaned);
			continue;
		}

		if (match_list_item(cleaned)) {
			flush_plain_paragraph(&para_lines, blocks);
			push_block(blocks, BLOCK_PROSE, cleaned);
			continue;
		}

		push_text_owned(&para_lines, cleaned);
	}

	flush_plain_paragraph(&para_lines, blocks);
	if (fence_lines.count > 0) {
		push_text_copy(&fence_lines, "```");
		TextBuffer fb;
		init_buffer(&fb);
		for (size_t i = 0; i < fence_lines.count; ++i) {
			if (i > 0) {
				append_char(&fb, '\n');
			}
			append_string(&fb, fence_lines.items[i]);
		}
		push_block(blocks, BLOCK_CODE, detach_buffer(&fb));
	}
	free_text_list(&fence_lines);
	free_text_list(&para_lines);
	free(normalized);
}

static size_t strip_references_count(const BlockList *blocks) {
	size_t total = blocks->count;
	if (total < 6) {
		return total;
	}
	size_t cutoff_start = (size_t)((double)total * 0.65);
	for (size_t idx = cutoff_start; idx < total; ++idx) {
		if (blocks->items[idx].kind == BLOCK_HEADING) {
			char *trimmed = duplicate_string(blocks->items[idx].text);
			trim_inplace(trimmed);
			bool is_ref = match_reference_heading(trimmed);
			free(trimmed);
			if (is_ref) {
				size_t end = idx + 15 < total ? idx + 15 : total;
				size_t prose_count = 0;
				size_t cit_hits = 0;
				for (size_t k = idx + 1; k < end; ++k) {
					if (blocks->items[k].kind == BLOCK_PROSE) {
						prose_count++;
						if (search_citation_line(blocks->items[k].text)) {
							cit_hits++;
						}
					}
				}
				if (prose_count == 0 ||
				    ((double)cit_hits / (double)prose_count) >= 0.40) {
					return idx;
				}
			}
		}
	}
	return total;
}

static void deduplicate_paragraphs(const BlockList *input, size_t active_count,
                                   const CorpusState *corpus_state, bool enable_dedup,
                                   BlockList *kept, TextCounter *doc_hashes) {
	init_block_list(kept);
	init_text_counter(doc_hashes);
	if (!enable_dedup) {
		for (size_t i = 0; i < active_count; ++i) {
			push_block(kept, input->items[i].kind,
				   duplicate_string(input->items[i].text));
		}
		return;
	}

	TextCounter seen_in_doc;
	init_text_counter(&seen_in_doc);

	for (size_t i = 0; i < active_count; ++i) {
		BlockKind kind = input->items[i].kind;
		const char *text = input->items[i].text;
		if (kind != BLOCK_PROSE || count_utf8_points(text, strlen(text)) < 40) {
			push_block(kept, kind, duplicate_string(text));
			continue;
		}
		char *lower = to_lowercase_utf8(text);
		char *norm = collapse_whitespace_alloc(lower);
		free(lower);
		char digest[65];
		compute_sha256_hex((const uint8_t *)norm, strlen(norm), digest);
		free(norm);

		if (has_text_key(&seen_in_doc, digest) ||
		    get_text_count(&corpus_state->paragraph_counts, digest) >= 3) {
			continue;
		}
		add_text_count(&seen_in_doc, digest, 1);
		add_text_count(doc_hashes, digest, 1);
		push_block(kept, kind, duplicate_string(text));
	}
	free_text_counter(&seen_in_doc);
}

typedef struct {
	char *ngram;
	int count;
} NgramCountItem;

static int compare_ngram_desc(const void *left, const void *right) {
	const NgramCountItem *a = (const NgramCountItem *)left;
	const NgramCountItem *b = (const NgramCountItem *)right;
	return (b->count > a->count) - (b->count < a->count);
}

static double evaluate_quality(const ExtractedDocument *doc, const BlockList *blocks,
                               size_t active_count, char **out_reason) {
	TextBuffer reasons;
	init_buffer(&reasons);
	bool any_non_image = false;
	for (size_t i = 0; i < active_count; ++i) {
		if (blocks->items[i].kind != BLOCK_IMAGE) {
			any_non_image = true;
			break;
		}
	}
	if (!any_non_image) {
		free_buffer(&reasons);
		*out_reason = duplicate_string("empty");
		return 0.0;
	}

	double score = 1.0;
	if (doc->raw_characters > 0) {
		double corrupt_ratio =
			(double)doc->corrupt_characters / (double)doc->raw_characters;
		if (corrupt_ratio > 0.01) {
			score -= fmin(0.65, corrupt_ratio * 15.0);
			char msg[96];
			snprintf(msg, sizeof(msg), "corrupt_encoding=%.2f%%",
				 corrupt_ratio * 100.0);
			append_string(&reasons, msg);
		}
	}

	TextBuffer prose_joined_buf;
	init_buffer(&prose_joined_buf);
	for (size_t i = 0; i < active_count; ++i) {
		if (blocks->items[i].kind == BLOCK_PROSE ||
		    blocks->items[i].kind == BLOCK_HEADING) {
			if (prose_joined_buf.length > 0) {
				append_char(&prose_joined_buf, '\n');
			}
			append_string(&prose_joined_buf, blocks->items[i].text);
		}
	}
	char *prose_joined = detach_buffer(&prose_joined_buf);

	/* Word stats matching prose_joined.split() */
	size_t word_count = 0;
	size_t word_len_sum = 0;
	size_t alpha_words = 0;
	size_t pj_len = strlen(prose_joined);
	size_t off = 0;
	while (off < pj_len) {
		while (off < pj_len) {
			uint32_t cp = 0;
			int step = decode_utf8_point(prose_joined, pj_len, off, &cp);
			if (!is_unicode_space(cp)) {
				break;
			}
			off += (size_t)(step > 0 ? step : 1);
		}
		if (off >= pj_len) {
			break;
		}
		size_t wlen = 0;
		bool has_alpha = false;
		while (off < pj_len) {
			uint32_t cp = 0;
			int step = decode_utf8_point(prose_joined, pj_len, off, &cp);
			if (is_unicode_space(cp)) {
				break;
			}
			wlen++;
			if (is_unicode_alpha(cp)) {
				has_alpha = true;
			}
			off += (size_t)(step > 0 ? step : 1);
		}
		word_count++;
		word_len_sum += wlen;
		if (has_alpha) {
			alpha_words++;
		}
	}

	if (word_count > 0) {
		double avg_word_len = (double)word_len_sum / (double)word_count;
		if (avg_word_len < 2.8 || avg_word_len > 14.0) {
			score -= 0.35;
			char msg[96];
			snprintf(msg, sizeof(msg), "avg_word_len=%.1f", avg_word_len);
			if (reasons.length > 0) {
				append_string(&reasons, ", ");
			}
			append_string(&reasons, msg);
		}
		double alpha_ratio = (double)alpha_words / (double)word_count;
		if (alpha_ratio < 0.60) {
			score -= 0.40;
			char msg[96];
			snprintf(msg, sizeof(msg), "alpha_ratio=%.2f", alpha_ratio);
			if (reasons.length > 0) {
				append_string(&reasons, ", ");
			}
			append_string(&reasons, msg);
		}
	}

	/* Gopher line repetition gate */
	TextCounter unique_lines;
	init_text_counter(&unique_lines);
	size_t total_lines = 0;
	for (size_t i = 0; i < active_count; ++i) {
		if (blocks->items[i].kind == BLOCK_IMAGE) {
			continue;
		}
		const char *scan = blocks->items[i].text;
		while (*scan) {
			const char *eol = strchr(scan, '\n');
			size_t llen = eol ? (size_t)(eol - scan) : strlen(scan);
			char *ln = duplicate_range(scan, llen);
			trim_inplace(ln);
			if (count_utf8_points(ln, strlen(ln)) >= 15) {
				char *lower = to_lowercase_utf8(ln);
				char *norm_ln = collapse_whitespace_alloc(lower);
				free(lower);
				add_text_count(&unique_lines, norm_ln, 1);
				total_lines++;
				free(norm_ln);
			}
			free(ln);
			scan = eol ? (eol + 1) : (scan + llen);
		}
	}
	if (total_lines >= 10) {
		double dup_line_ratio = 1.0 - ((double)unique_lines.count / (double)total_lines);
		if (dup_line_ratio > 0.30) {
			score -= 0.45;
			char msg[96];
			snprintf(msg, sizeof(msg), "dup_lines=%.2f%%", dup_line_ratio * 100.0);
			if (reasons.length > 0) {
				append_string(&reasons, ", ");
			}
			append_string(&reasons, msg);
		}
	}
	free_text_counter(&unique_lines);

	/* Top-10 character 10-gram repetition check */
	char *lower_pj = to_lowercase_utf8(prose_joined);
	free(prose_joined);
	char *compact = collapse_whitespace_alloc(lower_pj);
	free(lower_pj);

	/* Collect up to 20,000 codepoints as byte offsets */
	size_t compact_bytes = strlen(compact);
	size_t *cp_offsets = (size_t *)allocate_memory((compact_bytes + 2) * sizeof(size_t));
	size_t cp_count = 0;
	off = 0;
	while (off < compact_bytes && cp_count < 20000) {
		cp_offsets[cp_count++] = off;
		uint32_t cp = 0;
		int step = decode_utf8_point(compact, compact_bytes, off, &cp);
		off += (size_t)(step > 0 ? step : 1);
	}
	cp_offsets[cp_count] = off;

	if (cp_count >= 500) {
		TextCounter ngram_counts;
		init_text_counter(&ngram_counts);
		size_t total_ngrams = 0;
		for (size_t i = 0; i + 10 <= cp_count; i += 2) {
			char *gram = duplicate_range(compact + cp_offsets[i],
						     cp_offsets[i + 10] - cp_offsets[i]);
			add_text_count(&ngram_counts, gram, 1);
			free(gram);
			total_ngrams++;
		}
		if (total_ngrams > 0 && ngram_counts.count > 0) {
			NgramCountItem *items = (NgramCountItem *)allocate_memory(
				ngram_counts.count * sizeof(NgramCountItem));
			size_t icount = 0;
			for (size_t k = 0; k < ngram_counts.capacity; ++k) {
				if (ngram_counts.slots[k].occupied) {
					items[icount].ngram = ngram_counts.slots[k].key;
					items[icount].count = ngram_counts.slots[k].value;
					icount++;
				}
			}
			qsort(items, icount, sizeof(NgramCountItem), compare_ngram_desc);
			size_t top_sum = 0;
			size_t top_limit = icount < 10 ? icount : 10;
			for (size_t k = 0; k < top_limit; ++k) {
				top_sum += (size_t)items[k].count;
			}
			free(items);
			double top_share = (double)top_sum / (double)total_ngrams;
			if (top_share > 0.30) {
				score -= 0.35;
				char msg[96];
				snprintf(msg, sizeof(msg), "ngram_loop=%.2f%%", top_share * 100.0);
				if (reasons.length > 0) {
					append_string(&reasons, ", ");
				}
				append_string(&reasons, msg);
			}
		}
		free_text_counter(&ngram_counts);
	}
	free(cp_offsets);
	free(compact);

	double rounded = rint(score * 1000.0) / 1000.0;
	if (rounded < 0.0) {
		rounded = 0.0;
	}
	if (rounded > 1.0) {
		rounded = 1.0;
	}
	*out_reason = detach_buffer(&reasons);
	return rounded;
}

static bool build_minhash(const char *text, MinHashSignature *out_sig) {
	char *lower = to_lowercase_utf8(text ? text : "");
	TextList words;
	init_text_list(&words);

	const char *p = lower;
	while (*p) {
		while (*p && !((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9'))) {
			p++;
		}
		if (!*p) {
			break;
		}
		const char *start = p;
		while (*p && ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9'))) {
			p++;
		}
		push_text_owned(&words, duplicate_range(start, (size_t)(p - start)));
	}
	free(lower);

	if (words.count < 10) {
		free_text_list(&words);
		return false;
	}

	size_t limit = words.count - 4;
	if (limit > 8000) {
		limit = 8000;
	}

	Hash64Set shingle_hashes;
	init_hash64_set(&shingle_hashes);
	for (size_t i = 0; i < limit; ++i) {
		TextBuffer sb;
		init_buffer(&sb);
		for (int w = 0; w < 5; ++w) {
			if (w > 0) {
				append_char(&sb, ' ');
			}
			append_string(&sb, words.items[i + (size_t)w]);
		}
		uint64_t hval =
			compute_blake2b_64((const uint8_t *)sb.data, sb.length) % MINHASH_PRIME;
		insert_hash64(&shingle_hashes, hval);
		free_buffer(&sb);
	}
	free_text_list(&words);

	if (shingle_hashes.count == 0) {
		free_hash64_set(&shingle_hashes);
		return false;
	}

	for (int k = 0; k < MINHASH_SIZE; ++k) {
		uint64_t a = MINHASH_COEFF_A[k];
		uint64_t b = MINHASH_COEFF_B[k];
		uint64_t min_val = UINT64_MAX;
		for (size_t s = 0; s < shingle_hashes.capacity; ++s) {
			if (!shingle_hashes.occupied[s]) {
				continue;
			}
			uint64_t prod = multiply_mod_mersenne(a, shingle_hashes.keys[s]);
			uint64_t val = reduce_mersenne61(prod + b);
			if (val < min_val) {
				min_val = val;
			}
		}
		out_sig->values[k] = min_val;
	}
	free_hash64_set(&shingle_hashes);
	return true;
}

static bool check_duplicate_document(const char *text, const CorpusState *corpus_state,
                                     char **out_reason, char out_exact_hash[65],
                                     MinHashSignature *out_sig, bool *out_has_sig) {
	*out_reason = NULL;
	*out_has_sig = false;

	char *lower = to_lowercase_utf8(text ? text : "");
	char *norm = collapse_whitespace_alloc(lower);
	free(lower);
	compute_sha256_hex((const uint8_t *)norm, strlen(norm), out_exact_hash);

	if (has_text_key(&corpus_state->exact_hashes, out_exact_hash)) {
		free(norm);
		*out_reason = duplicate_string("exact_duplicate");
		return true;
	}

	MinHashSignature sig;
	bool has_sig = build_minhash(norm, &sig);
	free(norm);

	if (has_sig) {
		IntList candidates = {NULL, 0, 0};
		for (int band = 0; band < MINHASH_BANDS; ++band) {
			const IntList *bucket = find_lsh_bucket(
				&corpus_state->lsh_buckets, band, &sig.values[band * MINHASH_ROWS]);
			if (bucket) {
				for (size_t i = 0; i < bucket->count; ++i) {
					int cidx = bucket->items[i];
					bool dup = false;
					for (size_t j = 0; j < candidates.count; ++j) {
						if (candidates.items[j] == cidx) {
							dup = true;
							break;
						}
					}
					if (!dup) {
						push_int(&candidates, cidx);
					}
				}
			}
		}

		for (size_t i = 0; i < candidates.count; ++i) {
			const MinHashSignature *other =
				&corpus_state->signatures[candidates.items[i]];
			int matches = 0;
			for (int k = 0; k < MINHASH_SIZE; ++k) {
				if (sig.values[k] == other->values[k]) {
					matches++;
				}
			}
			double jaccard = (double)matches / (double)MINHASH_SIZE;
			if (jaccard >= NEAR_DUPLICATE_THRESHOLD) {
				char msg[64];
				snprintf(msg, sizeof(msg), "near_duplicate(jaccard=%.2f)", jaccard);
				*out_reason = duplicate_string(msg);
				free_int_list(&candidates);
				return true;
			}
		}
		free_int_list(&candidates);
		*out_sig = sig;
		*out_has_sig = true;
	}
	return false;
}

static void commit_corpus_state(CorpusState *corpus_state, const char *exact_hash,
                                const MinHashSignature *sig, bool has_sig,
                                const TextCounter *paragraph_hashes,
                                const Hash64List *image_dhashes, const TextCounter *image_digests) {
	if (exact_hash && *exact_hash) {
		add_text_count(&corpus_state->exact_hashes, exact_hash, 1);
	}
	if (has_sig && sig) {
		if (corpus_state->signature_count == corpus_state->signature_capacity) {
			corpus_state->signature_capacity =
				corpus_state->signature_capacity ? corpus_state->signature_capacity * 2
								 : 32;
			corpus_state->signatures = (MinHashSignature *)reallocate_memory(
				corpus_state->signatures,
				corpus_state->signature_capacity * sizeof(MinHashSignature));
		}
		int sig_idx = (int)corpus_state->signature_count;
		corpus_state->signatures[corpus_state->signature_count++] = *sig;
		for (int band = 0; band < MINHASH_BANDS; ++band) {
			insert_lsh_bucket(&corpus_state->lsh_buckets, band,
					  &sig->values[band * MINHASH_ROWS], sig_idx);
		}
	}
	if (paragraph_hashes && paragraph_hashes->slots) {
		for (size_t i = 0; i < paragraph_hashes->capacity; ++i) {
			if (paragraph_hashes->slots[i].occupied) {
				add_text_count(&corpus_state->paragraph_counts,
					       paragraph_hashes->slots[i].key, 1);
			}
		}
	}
	if (image_dhashes) {
		for (size_t i = 0; i < image_dhashes->count; ++i) {
			insert_hash64(&corpus_state->corpus_dhashes, image_dhashes->items[i]);
		}
	}
	if (image_digests && image_digests->slots) {
		for (size_t i = 0; i < image_digests->capacity; ++i) {
			if (image_digests->slots[i].occupied) {
				add_text_count(&corpus_state->image_doc_counts,
					       image_digests->slots[i].key, 1);
			}
		}
	}
}

static void cleanup_saved_images(const TextList *saved_images) {
	for (size_t i = 0; i < saved_images->count; ++i) {
		remove_file_utf8(saved_images->items[i]);
	}
}

static size_t count_split_words(const char *text) {
	if (!text) {
		return 0;
	}
	size_t count = 0;
	size_t len = strlen(text);
	size_t off = 0;
	while (off < len) {
		while (off < len) {
			uint32_t cp = 0;
			int step = decode_utf8_point(text, len, off, &cp);
			if (!is_unicode_space(cp)) {
				break;
			}
			off += (size_t)(step > 0 ? step : 1);
		}
		if (off >= len) {
			break;
		}
		count++;
		while (off < len) {
			uint32_t cp = 0;
			int step = decode_utf8_point(text, len, off, &cp);
			if (is_unicode_space(cp)) {
				break;
			}
			off += (size_t)(step > 0 ? step : 1);
		}
	}
	return count;
}

static char *build_frontmatter(const char *doc_id, const char *source_type, int pages, size_t words,
                               int images, double quality_score) {
	TextBuffer safe_id;
	init_buffer(&safe_id);
	for (const char *p = doc_id; *p; ++p) {
		append_char(&safe_id, (*p == '"') ? '\'' : *p);
	}
	TextBuffer out;
	init_buffer(&out);
	char header_buf[256];
	append_string(&out, "---\ndoc_id: \"");
	append_string(&out, safe_id.data);
	append_string(&out, "\"\nsource_type: \"");
	append_string(&out, source_type);
	snprintf(header_buf, sizeof(header_buf),
		 "\"\npages: %d\nwords: %zu\nimages: %d\nquality_score: %.2f\n---\n\n", pages,
		 words, images, quality_score);
	append_string(&out, header_buf);
	free_buffer(&safe_id);
	return detach_buffer(&out);
}

static void write_markdown_outputs(const char *stem, const ExtractedDocument *doc,
                                   const BlockList *blocks, double quality_score,
                                   const char *output_base, const char *text_dir,
                                   const char *symbols_dir, bool flat, bool frontmatter,
                                   bool symbols_view, size_t *out_word_count,
                                   size_t *out_symbol_words) {
	*out_word_count = 0;
	*out_symbol_words = 0;
	if (blocks->count == 0) {
		return;
	}

	TextBuffer body_buf;
	init_buffer(&body_buf);
	for (size_t i = 0; i < blocks->count; ++i) {
		if (!has_non_whitespace(blocks->items[i].text)) {
			continue;
		}
		if (body_buf.length > 0) {
			append_string(&body_buf, "\n\n");
		}
		append_string(&body_buf, blocks->items[i].text);
	}
	char *body = detach_buffer(&body_buf);
	trim_inplace(body);
	if (!*body) {
		free(body);
		return;
	}

	size_t word_count = 0;
	for (size_t i = 0; i < blocks->count; ++i) {
		if (blocks->items[i].kind != BLOCK_IMAGE) {
			word_count += count_split_words(blocks->items[i].text);
		}
	}

	TextBuffer final_md;
	init_buffer(&final_md);
	if (frontmatter) {
		char *fm = build_frontmatter(stem, doc->source_type, doc->page_count, word_count,
					     doc->image_count, quality_score);
		append_string(&final_md, fm);
		free(fm);
	}
	append_string(&final_md, body);
	append_char(&final_md, '\n');
	free(body);

	char md_name[512];
	snprintf(md_name, sizeof(md_name), "%s.md", stem);
	char *md_path = join_path(flat ? output_base : text_dir, md_name);
	write_file_utf8(md_path, final_md.data, final_md.length);
	free(md_path);
	free_buffer(&final_md);

	size_t symbol_words = 0;
	if (symbols_view) {
		TextList entries;
		init_text_list(&entries);
		for (size_t i = 0; i < blocks->count; ++i) {
			BlockKind k = blocks->items[i].kind;
			if (k != BLOCK_MATH && k != BLOCK_CODE && k != BLOCK_TABLE) {
				continue;
			}
			TextBuffer entry;
			init_buffer(&entry);
			if (i > 0 && (blocks->items[i - 1].kind == BLOCK_PROSE ||
				      blocks->items[i - 1].kind == BLOCK_HEADING)) {
				const char *prev_text = blocks->items[i - 1].text;
				const char *last_nl = strrchr(prev_text, '\n');
				const char *last_line = last_nl ? (last_nl + 1) : prev_text;
				char *tr = duplicate_string(last_line);
				trim_inplace(tr);
				append_string(&entry, "> ");
				size_t tlen = strlen(tr);
				size_t off = 0;
				int cp_count = 0;
				while (off < tlen && cp_count < 200) {
					uint32_t cp = 0;
					int step = decode_utf8_point(tr, tlen, off, &cp);
					off += (size_t)(step > 0 ? step : 1);
					append_utf8_point(&entry, cp);
					cp_count++;
				}
				free(tr);
				append_string(&entry, "\n\n");
			}
			append_string(&entry, blocks->items[i].text);
			push_text_owned(&entries, detach_buffer(&entry));
			symbol_words += count_split_words(blocks->items[i].text);
		}
		if (entries.count > 0) {
			make_directory_tree(symbols_dir);
			TextBuffer sym_buf;
			init_buffer(&sym_buf);
			append_string(&sym_buf, "# ");
			append_string(&sym_buf, stem);
			append_string(&sym_buf, " — Equations, Tables & Code\n\n");
			for (size_t i = 0; i < entries.count; ++i) {
				if (i > 0) {
					append_string(&sym_buf, "\n\n");
				}
				append_string(&sym_buf, entries.items[i]);
			}
			append_char(&sym_buf, '\n');
			char sym_name[512];
			snprintf(sym_name, sizeof(sym_name), "%s_symbols.md", stem);
			char *sym_path = join_path(symbols_dir, sym_name);
			write_file_utf8(sym_path, sym_buf.data, sym_buf.length);
			free(sym_path);
			free_buffer(&sym_buf);
		}
		free_text_list(&entries);
	}

	*out_word_count = word_count;
	*out_symbol_words = symbol_words;
}

static bool has_extension_in(const char *ext, const char *const *list, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		if (equals_ignore_case(ext, list[i])) {
			return true;
		}
	}
	return false;
}

static bool is_supported_file(const char *path) {
	const char *ext = path_extension_ptr(path);
	if (equals_ignore_case(ext, ".pdf") || equals_ignore_case(ext, ".docx")) {
		return true;
	}
	if (has_extension_in(ext, TEXT_EXTENSIONS,
			     sizeof(TEXT_EXTENSIONS) / sizeof(TEXT_EXTENSIONS[0]))) {
		return true;
	}
	return has_extension_in(ext, IMAGE_EXTENSIONS,
				sizeof(IMAGE_EXTENSIONS) / sizeof(IMAGE_EXTENSIONS[0]));
}

static int compare_strings_lex(const void *left, const void *right) {
	const char *a = *(const char *const *)left;
	const char *b = *(const char *const *)right;
	return strcmp(a, b);
}

static void walk_directory_recursive(const char *dir_path, const TextList *excluded_dirs,
                                     TextList *discovered) {
	TextList subdirs;
	TextList files;
	init_text_list(&subdirs);
	init_text_list(&files);

#ifdef _WIN32
	char *pattern = join_path(dir_path, "*");
	wchar_t *wide_pattern = utf8_to_wide(pattern);
	free(pattern);
	if (wide_pattern) {
		WIN32_FIND_DATAW fdata;
		HANDLE handle = FindFirstFileW(wide_pattern, &fdata);
		free(wide_pattern);
		if (handle != INVALID_HANDLE_VALUE) {
			do {
				char *name = wide_to_utf8(fdata.cFileName);
				if (!name) {
					continue;
				}
				if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
					free(name);
					continue;
				}
				char *full = join_path(dir_path, name);
				if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (name[0] != '.' && strcmp(name, "archive") != 0) {
						char *resolved = resolve_absolute_path(full);
						bool is_excl = false;
						for (size_t i = 0; i < excluded_dirs->count; ++i) {
							if (paths_equal(resolved,
									excluded_dirs->items[i])) {
								is_excl = true;
								break;
							}
						}
						free(resolved);
						if (!is_excl) {
							push_text_owned(&subdirs, name);
							free(full);
							continue;
						}
					}
					free(name);
					free(full);
				} else {
					if (is_supported_file(full)) {
						push_text_owned(&files, full);
					} else {
						free(full);
					}
					free(name);
				}
			} while (FindNextFileW(handle, &fdata));
			FindClose(handle);
		}
	}
#else
	DIR *dir = opendir(dir_path);
	if (dir) {
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) {
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
				continue;
			}
			char *full = join_path(dir_path, entry->d_name);
			if (path_is_directory(full)) {
				if (entry->d_name[0] != '.' &&
				    strcmp(entry->d_name, "archive") != 0) {
					char *resolved = resolve_absolute_path(full);
					bool is_excl = false;
					for (size_t i = 0; i < excluded_dirs->count; ++i) {
						if (paths_equal(resolved,
								excluded_dirs->items[i])) {
							is_excl = true;
							break;
						}
					}
					free(resolved);
					if (!is_excl) {
						push_text_copy(&subdirs, entry->d_name);
					}
				}
				free(full);
			} else {
				if (is_supported_file(full)) {
					push_text_owned(&files, full);
				} else {
					free(full);
				}
			}
		}
		closedir(dir);
	}
#endif

	if (files.count > 1) {
		qsort(files.items, files.count, sizeof(char *), compare_strings_lex);
	}
	for (size_t i = 0; i < files.count; ++i) {
		push_text_owned(discovered, files.items[i]);
		files.items[i] = NULL;
	}
	free_text_list(&files);

	if (subdirs.count > 1) {
		qsort(subdirs.items, subdirs.count, sizeof(char *), compare_strings_lex);
	}
	for (size_t i = 0; i < subdirs.count; ++i) {
		char *sub_full = join_path(dir_path, subdirs.items[i]);
		walk_directory_recursive(sub_full, excluded_dirs, discovered);
		free(sub_full);
	}
	free_text_list(&subdirs);
}

static void discover_files(const char *input_path, const TextList *excluded_dirs, bool recursive,
                           TextList *discovered) {
	init_text_list(discovered);
	if (!recursive) {
#ifdef _WIN32
		char *pattern = join_path(input_path, "*");
		wchar_t *wide_pattern = utf8_to_wide(pattern);
		free(pattern);
		if (wide_pattern) {
			WIN32_FIND_DATAW fdata;
			HANDLE handle = FindFirstFileW(wide_pattern, &fdata);
			free(wide_pattern);
			if (handle != INVALID_HANDLE_VALUE) {
				do {
					if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						continue;
					}
					char *name = wide_to_utf8(fdata.cFileName);
					if (!name) {
						continue;
					}
					char *full = join_path(input_path, name);
					free(name);
					if (is_supported_file(full)) {
						push_text_owned(discovered, full);
					} else {
						free(full);
					}
				} while (FindNextFileW(handle, &fdata));
				FindClose(handle);
			}
		}
#else
		DIR *dir = opendir(input_path);
		if (dir) {
			struct dirent *entry;
			while ((entry = readdir(dir)) != NULL) {
				char *full = join_path(input_path, entry->d_name);
				if (!path_is_directory(full) && is_supported_file(full)) {
					push_text_owned(discovered, full);
				} else {
					free(full);
				}
			}
			closedir(dir);
		}
#endif
		if (discovered->count > 1) {
			qsort(discovered->items, discovered->count, sizeof(char *),
			      compare_strings_lex);
		}
		return;
	}
	walk_directory_recursive(input_path, excluded_dirs, discovered);
}

static char *unique_stem(const char *source_path, const char *input_root, TextCounter *used_stems) {
	char *stem = path_stem_alloc(source_path);
	if (!has_text_key(used_stems, stem)) {
		add_text_count(used_stems, stem, 1);
		return stem;
	}

	char *rel = compute_relative_path(source_path, input_root);
	char *rel_no_ext = replace_extension(rel, "");
	free(rel);
	for (char *p = rel_no_ext; *p; ++p) {
		if (*p == '/' || *p == '\\') {
			*p = '_';
		}
	}
	char *candidate = rel_no_ext;
	int counter = 1;
	while (has_text_key(used_stems, candidate)) {
		free(candidate);
		char buf[1024];
		snprintf(buf, sizeof(buf), "%s_%d", stem, counter++);
		candidate = duplicate_string(buf);
	}
	add_text_count(used_stems, candidate, 1);
	free(stem);
	return candidate;
}

typedef enum {
	STATUS_OK = 0,
	STATUS_SKIPPED = 1,
	STATUS_REJECTED = 2,
	STATUS_DEDUP = 3
} ProcessStatus;

static ProcessStatus process_file(fz_context *context, const char *source_path, const char *stem,
                                  const char *output_base, const char *text_dir,
                                  const char *symbols_dir, const char *images_dir,
                                  bool extract_images, bool capture_vectors, bool flat,
                                  bool frontmatter, bool symbols_view, bool keep_references,
                                  double min_quality, bool enable_dedup, CorpusState *corpus_state,
                                  size_t *out_words, size_t *out_symbol_words, int *out_images) {
	*out_words = 0;
	*out_symbol_words = 0;
	*out_images = 0;

	const char *ext = path_extension_ptr(source_path);
	const char *md_dir = flat ? output_base : text_dir;
	const char *fname = path_basename_ptr(source_path);

	if (has_extension_in(ext, IMAGE_EXTENSIONS,
			     sizeof(IMAGE_EXTENSIONS) / sizeof(IMAGE_EXTENSIONS[0]))) {
		if (!extract_images) {
			return STATUS_SKIPPED;
		}
		printf("[..] Processing IMG:  %s\n", fname);
		uint8_t *raw_bytes = NULL;
		size_t raw_size = 0;
		if (!read_file_bytes(source_path, &raw_bytes, &raw_size)) {
			fprintf(stderr, "  [!] Failed to convert image %s: read error\n", fname);
			return STATUS_SKIPPED;
		}
		char img_name[512];
		snprintf(img_name, sizeof(img_name), "%s.png", stem);
		char *png_dest = join_path(images_dir, img_name);
		Hash64List doc_dhashes = {NULL, 0, 0};
		TextCounter doc_hashes;
		init_text_counter(&doc_hashes);
		SavePngStatus st = save_filtered_png(context, raw_bytes, raw_size, png_dest,
						     &doc_dhashes, corpus_state, &doc_hashes,
						     enable_dedup, MIN_IMAGE_SIZE);
		free(raw_bytes);
		if (st == PNG_SAVED) {
			commit_corpus_state(corpus_state, NULL, NULL, false, NULL, &doc_dhashes,
					    &doc_hashes);
			printf("  [OK] Converted -> %s (.png)\n", path_basename_ptr(png_dest));
			free(png_dest);
			free_hash64_list(&doc_dhashes);
			free_text_counter(&doc_hashes);
			*out_images = 1;
			return STATUS_OK;
		}
		printf("  [SKIP] Low-signal or duplicate image: %s\n", fname);
		free(png_dest);
		free_hash64_list(&doc_dhashes);
		free_text_counter(&doc_hashes);
		return STATUS_SKIPPED;
	}

	ExtractedDocument doc;
	if (equals_ignore_case(ext, ".pdf")) {
		printf("[..] Processing PDF:  %s\n", fname);
		extract_pdf_document(context, source_path, stem, images_dir, md_dir, extract_images,
				     capture_vectors, enable_dedup, corpus_state, &doc);
	} else if (equals_ignore_case(ext, ".docx")) {
		printf("[..] Processing DOCX: %s\n", fname);
		extract_docx_document(context, source_path, stem, images_dir, md_dir,
				      extract_images, enable_dedup, corpus_state, &doc);
	} else if (has_extension_in(ext, TEXT_EXTENSIONS,
				    sizeof(TEXT_EXTENSIONS) / sizeof(TEXT_EXTENSIONS[0]))) {
		printf("[..] Processing TEXT: %s\n", fname);
		uint8_t *raw_bytes = NULL;
		size_t raw_size = 0;
		if (!read_file_bytes(source_path, &raw_bytes, &raw_size)) {
			fprintf(stderr, "  [!] Failed to read %s\n", fname);
			return STATUS_SKIPPED;
		}
		const char *src_ext = (ext[0] == '.') ? (ext + 1) : ext;
		char *lower_ext = to_lowercase_utf8(src_ext);
		init_extracted_doc(&doc, lower_ext);
		free(lower_ext);
		doc.page_count = 1;
		count_corrupt_characters((const char *)raw_bytes, &doc.corrupt_characters,
					 &doc.raw_characters);
		reconstruct_plain_blocks((const char *)raw_bytes, &doc.blocks);
		free(raw_bytes);
	} else {
		return STATUS_SKIPPED;
	}

	size_t active_count =
		keep_references ? doc.blocks.count : strip_references_count(&doc.blocks);

	if (doc.scanned_pages > 0) {
		printf("  [WARN] Detected %d scanned/image-only page(s) needing OCR\n",
		       doc.scanned_pages);
	}

	char *reason = NULL;
	double quality_score = evaluate_quality(&doc, &doc.blocks, active_count, &reason);
	if (quality_score < min_quality) {
		cleanup_saved_images(&doc.saved_images);
		printf("  [REJECT] Low quality (%.2f < %.2f): %s\n", quality_score, min_quality,
		       (reason && *reason) ? reason : "noisy");
		free(reason);
		free_extracted_doc(&doc);
		return STATUS_REJECTED;
	}
	free(reason);

	char exact_hash[65] = {0};
	MinHashSignature signature;
	bool has_sig = false;
	if (enable_dedup) {
		TextBuffer dedup_buf;
		init_buffer(&dedup_buf);
		for (size_t i = 0; i < active_count; ++i) {
			if (doc.blocks.items[i].kind != BLOCK_IMAGE) {
				if (dedup_buf.length > 0) {
					append_char(&dedup_buf, '\n');
				}
				append_string(&dedup_buf, doc.blocks.items[i].text);
			}
		}
		char *dup_reason = NULL;
		bool is_dup = check_duplicate_document(dedup_buf.data, corpus_state, &dup_reason,
						       exact_hash, &signature, &has_sig);
		free_buffer(&dedup_buf);
		if (is_dup) {
			cleanup_saved_images(&doc.saved_images);
			printf("  [DEDUP] Skipped %s: %s\n", fname,
			       dup_reason ? dup_reason : "duplicate");
			free(dup_reason);
			free_extracted_doc(&doc);
			return STATUS_DEDUP;
		}
		free(dup_reason);
	}

	BlockList kept_blocks;
	TextCounter para_hashes;
	deduplicate_paragraphs(&doc.blocks, active_count, corpus_state, enable_dedup, &kept_blocks,
			       &para_hashes);

	bool has_non_img_text = false;
	for (size_t i = 0; i < kept_blocks.count; ++i) {
		if (kept_blocks.items[i].kind != BLOCK_IMAGE &&
		    has_non_whitespace(kept_blocks.items[i].text)) {
			has_non_img_text = true;
			break;
		}
	}
	if (!has_non_img_text) {
		cleanup_saved_images(&doc.saved_images);
		free_block_list(&kept_blocks);
		free_text_counter(&para_hashes);
		free_extracted_doc(&doc);
		return STATUS_SKIPPED;
	}

	size_t words_total = 0;
	size_t words_symbols = 0;
	write_markdown_outputs(stem, &doc, &kept_blocks, quality_score, output_base, text_dir,
			       symbols_dir, flat, frontmatter, symbols_view, &words_total,
			       &words_symbols);

	if (words_total == 0 && doc.image_count == 0) {
		cleanup_saved_images(&doc.saved_images);
		free_block_list(&kept_blocks);
		free_text_counter(&para_hashes);
		free_extracted_doc(&doc);
		return STATUS_SKIPPED;
	}

	commit_corpus_state(corpus_state, enable_dedup ? exact_hash : NULL, &signature, has_sig,
			    &para_hashes, &doc.image_dhashes, &doc.image_digests);

	char w_buf[32], s_buf[32];
	format_thousands((int64_t)words_total, w_buf);
	format_thousands((int64_t)words_symbols, s_buf);
	if (symbols_view) {
		printf("  [OK] Quality %.2f | %s words (.md) | %d figures (.png) | %s symbol "
		       "words\n",
		       quality_score, w_buf, doc.image_count, s_buf);
	} else {
		printf("  [OK] Quality %.2f | %s words (.md) | %d figures (.png)\n", quality_score,
		       w_buf, doc.image_count);
	}

	*out_words = words_total;
	*out_symbol_words = words_symbols;
	*out_images = doc.image_count;

	free_block_list(&kept_blocks);
	free_text_counter(&para_hashes);
	free_extracted_doc(&doc);
	return STATUS_OK;
}

static void silent_mupdf_cb(void *user, const char *message) {
	(void)user;
	(void)message;
}

static void print_usage(const char *prog) {
	printf("usage: %s [options] [pos_input_dir] [pos_output_dir]\n\n"
	       "Axiom Unified Layout-Aware Multimodal Corpus Extractor (.md & .png only)\n\n"
	       "options:\n"
	       "  -h, --help            show this help message and exit\n"
	       "  --input-dir DIR       Input directory (default: asset_sample)\n"
	       "  --output-dir DIR      Output directory (default: asset_sample)\n"
	       "  --extract-images      Extract & convert filtered images as PNG (default: enabled)\n"
	       "  --no-images           Disable image and vector figure extraction\n"
	       "  --no-vector-figures   Disable vector diagram rasterization\n"
	       "  --flat                Write <stem>.md directly in output root instead of "
	       "<output_dir>/text/\n"
	       "  --symbols-view        Also emit secondary symbols/<stem>_symbols.md view with "
	       "surrounding context\n"
	       "  --frontmatter         Prepend YAML metadata frontmatter to each generated .md "
	       "file\n"
	       "  --min-quality FLOAT   Reject documents scoring below this quality threshold "
	       "[0.0-1.0] (default: 0.45)\n"
	       "  --keep-references     Keep trailing References/Bibliography sections instead of "
	       "stripping them\n"
	       "  --no-dedup            Disable exact/MinHash document and boilerplate paragraph "
	       "deduplication\n"
	       "  --no-recursive        Only scan top-level input directory without recursing into "
	       "subfolders\n",
	       prog);
}

int main(int argc, char **argv) {
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	int wide_argc = 0;
	LPWSTR *wide_argv = CommandLineToArgvW(GetCommandLineW(), &wide_argc);
	char **utf8_argv = NULL;
	if (wide_argv && wide_argc > 0) {
		utf8_argv = (char **)allocate_memory((size_t)wide_argc * sizeof(char *));
		for (int i = 0; i < wide_argc; ++i) {
			utf8_argv[i] = wide_to_utf8(wide_argv[i]);
		}
		LocalFree(wide_argv);
		argc = wide_argc;
		argv = utf8_argv;
	}
#endif

	init_minhash_tables();

	const char *pos_input_dir = NULL;
	const char *pos_output_dir = NULL;
	const char *flag_input_dir = NULL;
	const char *flag_output_dir = NULL;
	bool extract_images = true;
	bool capture_vectors = true;
	bool flat = false;
	bool symbols_view = false;
	bool frontmatter = false;
	double min_quality = 0.45;
	bool keep_references = false;
	bool enable_dedup = true;
	bool no_recursive = false;

	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
			print_usage(argv[0]);
			return 0;
		} else if (strcmp(arg, "--input-dir") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "error: argument --input-dir: expected one argument\n");
				return 2;
			}
			flag_input_dir = argv[++i];
		} else if (strcmp(arg, "--output-dir") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "error: argument --output-dir: expected one argument\n");
				return 2;
			}
			flag_output_dir = argv[++i];
		} else if (strcmp(arg, "--extract-images") == 0) {
			extract_images = true;
		} else if (strcmp(arg, "--no-images") == 0) {
			extract_images = false;
		} else if (strcmp(arg, "--no-vector-figures") == 0) {
			capture_vectors = false;
		} else if (strcmp(arg, "--flat") == 0) {
			flat = true;
		} else if (strcmp(arg, "--symbols-view") == 0) {
			symbols_view = true;
		} else if (strcmp(arg, "--frontmatter") == 0) {
			frontmatter = true;
		} else if (strcmp(arg, "--min-quality") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "error: argument --min-quality: expected one argument\n");
				return 2;
			}
			char *end_ptr = NULL;
			min_quality = strtod(argv[++i], &end_ptr);
			if (!end_ptr || *end_ptr != '\0') {
				fprintf(stderr, "error: argument --min-quality: invalid float value\n");
				return 2;
			}
		} else if (strcmp(arg, "--keep-references") == 0) {
			keep_references = true;
		} else if (strcmp(arg, "--no-dedup") == 0) {
			enable_dedup = false;
		} else if (strcmp(arg, "--no-recursive") == 0) {
			no_recursive = true;
		} else if (arg[0] == '-' && arg[1] != '\0') {
			fprintf(stderr, "error: unrecognized argument: %s\n", arg);
			return 2;
		} else if (!pos_input_dir) {
			pos_input_dir = arg;
		} else if (!pos_output_dir) {
			pos_output_dir = arg;
		} else {
			fprintf(stderr, "error: unrecognized argument: %s\n", arg);
			return 2;
		}
	}

	const char *in_arg =
		flag_input_dir ? flag_input_dir : (pos_input_dir ? pos_input_dir : "asset_sample");
	const char *out_arg = flag_output_dir
				      ? flag_output_dir
				      : (pos_output_dir ? pos_output_dir : "asset_sample");

	char *input_path = resolve_smart_path(in_arg);
	if (!path_exists(input_path)) {
		free(input_path);
		char *fallback = resolve_absolute_path("asset_sample");
		if (path_exists(fallback)) {
			input_path = fallback;
		} else {
			free(fallback);
			fprintf(stderr, "[!] Error: Input directory '%s' not found.\n", in_arg);
			return 1;
		}
	}

	char *output_base = resolve_smart_path(out_arg);
	char *text_dir = join_path(output_base, "text");
	char *symbols_dir = join_path(output_base, "symbols");
	char *images_dir = join_path(output_base, "images");

	make_directory_tree(output_base);
	if (!flat) {
		make_directory_tree(text_dir);
	}
	if (symbols_view) {
		make_directory_tree(symbols_dir);
	}
	if (extract_images) {
		make_directory_tree(images_dir);
	}

	TextList excluded_dirs;
	init_text_list(&excluded_dirs);
	push_text_owned(&excluded_dirs, resolve_absolute_path(text_dir));
	push_text_owned(&excluded_dirs, resolve_absolute_path(symbols_dir));
	push_text_owned(&excluded_dirs, resolve_absolute_path(images_dir));
	if (!paths_equal(output_base, input_path)) {
		push_text_owned(&excluded_dirs, resolve_absolute_path(output_base));
	}

	TextList files;
	discover_files(input_path, &excluded_dirs, !no_recursive, &files);
	if (files.count == 0) {
		printf("[!] No supported files found in %s\n", input_path);
		free_text_list(&files);
		free_text_list(&excluded_dirs);
		free(input_path);
		free(output_base);
		free(text_dir);
		free(symbols_dir);
		free(images_dir);
		return 0;
	}

	fz_context *context = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
	if (!context) {
		fprintf(stderr, "[!] Failed to initialize MuPDF context.\n");
		return 1;
	}
	fz_set_warning_callback(context, silent_mupdf_cb, NULL);
	fz_set_error_callback(context, silent_mupdf_cb, NULL);
	fz_register_document_handlers(context);

	printf("[+] Found %zu file(s) in %s\n", files.count, input_path);
	printf("  -> PDF Engine:          MuPDF C11 (layout dict + tables + vectors)\n");
	printf("  -> PNG Vision Engine:   MuPDF + Native Vision (entropy/dHash/vector) "
	       "(images=%s, vectors=%s)\n",
	       extract_images ? "True" : "False", capture_vectors ? "True" : "False");
	printf("  -> Quality Gate:        min_quality=%.2f | dedup=%s | frontmatter=%s\n",
	       min_quality, enable_dedup ? "True" : "False", frontmatter ? "True" : "False");
	printf("  -> Output Format:       Strictly .md and .png (context-preserving)\n");
	printf("  -> Markdown dest:       %s\n", flat ? output_base : text_dir);
	if (symbols_view) {
		printf("  -> Symbols view dest:   %s\n", symbols_dir);
	}
	if (extract_images) {
		printf("  -> Images destination:  %s\n", images_dir);
	}
	printf("------------------------------------------------------------\n");

	size_t processed_count = 0;
	size_t rejected_count = 0;
	size_t dedup_count = 0;
	size_t total_words = 0;
	size_t total_symbols = 0;
	size_t total_images = 0;

	TextCounter used_stems;
	init_text_counter(&used_stems);
	CorpusState corpus_state;
	init_corpus_state(&corpus_state);

	for (size_t i = 0; i < files.count; ++i) {
		char *stem = unique_stem(files.items[i], input_path, &used_stems);
		size_t words = 0;
		size_t s_words = 0;
		int imgs = 0;
		ProcessStatus st = process_file(
			context, files.items[i], stem, output_base, text_dir, symbols_dir,
			images_dir, extract_images, capture_vectors, flat, frontmatter,
			symbols_view, keep_references, min_quality, enable_dedup, &corpus_state,
			&words, &s_words, &imgs);
		if (st == STATUS_OK) {
			processed_count++;
			total_words += words;
			total_symbols += s_words;
			total_images += (size_t)imgs;
		} else if (st == STATUS_REJECTED) {
			rejected_count++;
		} else if (st == STATUS_DEDUP) {
			dedup_count++;
		}
		free(stem);
	}

	char c1[32], c2[32], c3[32], c4[32], c5[32], c6[32];
	format_thousands((int64_t)processed_count, c1);
	format_thousands((int64_t)rejected_count, c2);
	format_thousands((int64_t)dedup_count, c3);
	format_thousands((int64_t)total_words, c4);
	format_thousands((int64_t)total_symbols, c5);
	format_thousands((int64_t)total_images, c6);

	printf("============================================================\n");
	printf("[OK] Ingestion Pipeline Complete (.md & .png only):\n");
	printf("     Files Written:             %s\n", c1);
	printf("     Files Rejected (Quality):  %s\n", c2);
	printf("     Files Skipped (Dedup):     %s\n", c3);
	printf("     Total Corpus Words (.md):  %s\n", c4);
	if (symbols_view) {
		printf("     Secondary Symbol Words:    %s\n", c5);
	}
	printf("     Total High-Signal PNGs:    %s\n", c6);

	free_corpus_state(&corpus_state);
	free_text_counter(&used_stems);
	free_text_list(&files);
	free_text_list(&excluded_dirs);
	free(input_path);
	free(output_base);
	free(text_dir);
	free(symbols_dir);
	free(images_dir);
	fz_drop_context(context);

#ifdef _WIN32
	if (utf8_argv) {
		for (int i = 0; i < wide_argc; ++i) {
			free(utf8_argv[i]);
		}
		free(utf8_argv);
	}
#endif
	return 0;
}
