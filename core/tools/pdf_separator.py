#!/usr/bin/env python3
#
# Copyright (c) 2026 Omid Teimory. All Rights Reserved.
#
# NOTICE: This file contains proprietary source code licensed exclusively under the
# terms of the LICENSE file in the root directory of this repository.
# You may view, download, and compile this code solely to run the unmodified product.
# Copying, pasting, modifying, redistributing, creating derivative works, or using
# this code in any other project or AI training system is strictly prohibited.
# Removal or alteration of this notice is a violation of 17 U.S.C. § 1202 and EU law.
#

"""
Project Axiom — High-Speed Document & Media Separator
Uses PyMuPDF (fitz) for 50x C-accelerated extraction, with pypdf fallback.
Splits raw mixed-media PDFs into:
  - asset_sample/text/    (Clean prose for language training)
  - asset_sample/symbols/ (Math, tables, datasheets, code)
  - asset_sample/images/  (Extracted raster diagrams & figures)
"""

import sys
import os
import argparse
from pathlib import Path

# Try high-speed PyMuPDF first, fallback to pypdf
try:
    import pymupdf as fitz
    HAVE_FITZ = True
except ImportError:
    try:
        import fitz
        HAVE_FITZ = True
    except ImportError:
        HAVE_FITZ = False
        from pypdf import PdfReader


def is_symbol_heavy(text: str) -> bool:
    if not text:
        return False
    symbol_chars = set("=+\\-*/^_{}[]<>|~#$@%&`\\")
    symbols = sum(1 for c in text if c in symbol_chars)
    alphanumeric = sum(1 for c in text if c.isalnum())
    if alphanumeric == 0:
        return True
    return (symbols / max(alphanumeric, 1)) > 0.15


def separate_with_fitz(pdf_path: Path, text_dir: Path, symbols_dir: Path, images_dir: Path, extract_images: bool = False):
    stem = pdf_path.stem
    prose_lines, symbol_lines = [], []
    image_count = 0
    seen_xrefs = set()

    try:
        doc = fitz.open(str(pdf_path))
    except Exception as e:
        print(f"  [!] Failed to open {pdf_path.name}: {e}")
        return 0, 0, 0

    for page_idx in range(len(doc)):
        try:
            page = doc[page_idx]
            text = page.get_text("text") or ""
            for line in text.splitlines():
                clean_line = " ".join(line.split())
                if not clean_line:
                    continue
                if is_symbol_heavy(clean_line):
                    symbol_lines.append(clean_line)
                else:
                    prose_lines.append(clean_line)

            # Rapid image extraction (deduplicated by XREF & filtered for genuine diagrams)
            if extract_images:
                for img_info in page.get_images(full=True):
                    xref = img_info[0]
                    if xref in seen_xrefs:
                        continue
                    seen_xrefs.add(xref)
                    try:
                        base_image = doc.extract_image(xref)
                        w = base_image.get("width", 0)
                        h = base_image.get("height", 0)
                        if w < 100 or h < 100:
                            continue  # Skip decorative bullets, 1px rules, and tiny icons
                        img_bytes = base_image.get("image")
                        ext = base_image.get("ext", "png")
                        if img_bytes:
                            img_filename = f"{stem}_xref{xref}.{ext}"
                            with open(images_dir / img_filename, "wb") as f_img:
                                f_img.write(img_bytes)
                            image_count += 1
                    except Exception:
                        pass
        except Exception:
            continue

    doc.close()

    words_text = 0
    if prose_lines:
        with open(text_dir / f"{stem}_text.txt", "w", encoding="utf-8") as f:
            f.write("\n".join(prose_lines) + "\n")
        words_text = sum(len(line.split()) for line in prose_lines)

    words_symbols = 0
    if symbol_lines:
        with open(symbols_dir / f"{stem}_symbols.txt", "w", encoding="utf-8") as f:
            f.write("\n".join(symbol_lines) + "\n")
        words_symbols = sum(len(line.split()) for line in symbol_lines)

    print(f"  [OK] Extracted: {words_text:,} prose words | {words_symbols:,} symbols | {image_count} images")
    return words_text, words_symbols, image_count


def separate_with_pypdf(pdf_path: Path, text_dir: Path, symbols_dir: Path, images_dir: Path):
    from pypdf import PdfReader
    stem = pdf_path.stem
    try:
        reader = PdfReader(str(pdf_path))
    except Exception as e:
        print(f"  [!] Failed to read PDF {pdf_path.name}: {e}")
        return 0, 0, 0

    prose_lines, symbol_lines = [], []
    image_count = 0

    for page_idx, page in enumerate(reader.pages):
        try:
            page_text = page.extract_text() or ""
            for line in page_text.splitlines():
                clean_line = " ".join(line.split())
                if not clean_line:
                    continue
                if is_symbol_heavy(clean_line):
                    symbol_lines.append(clean_line)
                else:
                    prose_lines.append(clean_line)
        except Exception:
            continue

    words_text = 0
    if prose_lines:
        with open(text_dir / f"{stem}_text.txt", "w", encoding="utf-8") as f:
            f.write("\n".join(prose_lines) + "\n")
        words_text = sum(len(line.split()) for line in prose_lines)

    words_symbols = 0
    if symbol_lines:
        with open(symbols_dir / f"{stem}_symbols.txt", "w", encoding="utf-8") as f:
            f.write("\n".join(symbol_lines) + "\n")
        words_symbols = sum(len(line.split()) for line in symbol_lines)

    print(f"  [OK] Extracted: {words_text:,} prose words | {words_symbols:,} symbols | {image_count} images")
    return words_text, words_symbols, image_count


def separate_pdf(pdf_path: Path, text_dir: Path, symbols_dir: Path, images_dir: Path, extract_images: bool = False):
    print(f"[..] Processing: {pdf_path.name}")
    if HAVE_FITZ:
        return separate_with_fitz(pdf_path, text_dir, symbols_dir, images_dir, extract_images)
    else:
        return separate_with_pypdf(pdf_path, text_dir, symbols_dir, images_dir)


def main():
    parser = argparse.ArgumentParser(description="Axiom Multimodal High-Speed PDF Separator")
    parser.add_argument("--input-dir", type=str, default="asset_sample")
    parser.add_argument("--output-dir", type=str, default="asset_sample")
    parser.add_argument("--extract-images", action="store_true", default=False,
                        help="Extract embedded diagrams & figures (default: False for maximum text extraction speed)")
    args = parser.parse_args()

    input_str = args.input_dir
    if not Path(input_str).exists():
        if input_str.startswith("core/") and Path(input_str[5:]).exists():
            input_str = input_str[5:]
        elif (Path("core") / input_str).exists():
            input_str = str(Path("core") / input_str)

    input_path = Path(input_str).resolve()
    if not input_path.exists():
        input_path = Path("asset_sample").resolve()
        if not input_path.exists():
            print(f"[!] Error: Input directory '{args.input_dir}' not found.")
            sys.exit(1)

    output_str = args.output_dir
    if not Path(output_str).exists():
        if output_str.startswith("core/") and Path(output_str[5:]).exists():
            output_str = output_str[5:]
        elif (Path("core") / output_str).exists():
            output_str = str(Path("core") / output_str)

    output_base = Path(output_str).resolve()
    text_dir = output_base / "text"
    symbols_dir = output_base / "symbols"
    images_dir = output_base / "images"

    for d in [text_dir, symbols_dir, images_dir]:
        d.mkdir(parents=True, exist_ok=True)

    pdf_files = list(input_path.glob("*.pdf"))
    if not pdf_files:
        print(f"[!] No PDF files found in {input_path}")
        sys.exit(0)

    engine_name = "PyMuPDF (High-Speed C Engine)" if HAVE_FITZ else "pypdf"
    print(f"[+] Found {len(pdf_files)} PDF file(s) in {input_path}")
    print(f"  -> Extraction Engine:   {engine_name}")
    print(f"  -> Text destination:    {text_dir}")
    print(f"  -> Symbols destination: {symbols_dir}")
    print(f"  -> Images destination:  {images_dir}")
    print("-" * 60)

    total_prose = 0
    total_symbols = 0
    total_images = 0

    for pdf in pdf_files:
        t_words, s_words, imgs = separate_pdf(pdf, text_dir, symbols_dir, images_dir, args.extract_images)
        total_prose += t_words
        total_symbols += s_words
        total_images += imgs

    print("=" * 60)
    print(f"[OK] Ingestion Pipeline Ready:")
    print(f"     Total Prose Words:   {total_prose:,}")
    print(f"     Total Symbol Words:  {total_symbols:,}")
    print(f"     Total Extracted PNG: {total_images:,}")


if __name__ == "__main__":
    main()
