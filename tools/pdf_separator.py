#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Axiom-Proprietary
#
# Copyright (c) 2026 Omid Teimory. All Rights Reserved.
#
# PROPRIETARY SOURCE-AVAILABLE SOFTWARE — NOT OPEN SOURCE.
# Permission to view, clone, or compile this file is granted strictly on the
# condition precedent of full compliance with the root ./LICENSE file.
# Copying snippets, modifying, creating derivative works, redistributing,
# commercial/SaaS use, and AI/ML training (reserved under EU Directive
# 2019/790 Art. 4(3) and § 42h(6) Austrian UrhG) are strictly prohibited.
# Removal or alteration of this notice violates 17 U.S.C. § 1202 and § 90c UrhG.
#

"""
Project Axiom — High-Speed Document & Media Separator
Extracts structured multimodal documents into STRICTLY .md and .png formats:
  - asset_sample/text/    (*_text.md: Clean normalized prose in Markdown)
  - asset_sample/symbols/ (*_symbols.md: Math, tables, code in Markdown)
  - asset_sample/images/  (*.png: High-resolution diagrams & figures converted to PNG)

Only .md and .png output formats are allowed.
"""

import sys
import os
import io
import re
import unicodedata
import argparse
from pathlib import Path

# Optional PIL for strict PNG encoding/conversion
try:
    from PIL import Image
    HAVE_PIL = True
except ImportError:
    HAVE_PIL = False

# High-speed PyMuPDF first, fallback to pypdf
try:
    import pymupdf as fitz
    HAVE_FITZ = True
except ImportError:
    try:
        import fitz
        HAVE_FITZ = True
    except ImportError:
        HAVE_FITZ = False

LIGATURE_MAP = {
    "\ufb00": "ff",
    "\ufb01": "fi",
    "\ufb02": "fl",
    "\ufb03": "ffi",
    "\ufb04": "ffl",
    "\ufb05": "ft",
    "\ufb06": "st",
    "\u2018": "'",
    "\u2019": "'",
    "\u201a": "'",
    "\u201b": "'",
    "\u201c": '"',
    "\u201d": '"',
    "\u201e": '"',
    "\u201f": '"',
    "\u2013": "-",
    "\u2014": "-",
    "\u2015": "-",
    "\u00a0": " ",
    "\u2026": "...",
}


def clean_line_text(line: str) -> str:
    """Normalizes Unicode text to clean ASCII/UTF-8 compatible strings."""
    line = unicodedata.normalize("NFKD", line)
    for lig, rep in LIGATURE_MAP.items():
        line = line.replace(lig, rep)
    # Filter non-printable control characters except tab and newline
    clean_chars = [c for c in line if ord(c) in (9, 10, 13) or (32 <= ord(c) < 127) or (ord(c) >= 160)]
    cleaned = "".join(clean_chars)
    return " ".join(cleaned.split())


def is_symbol_heavy(text: str) -> bool:
    if not text:
        return False
    symbol_chars = set("=+\\-*/^_{}[]<>|~#$@%&`\\")
    symbols = sum(1 for c in text if c in symbol_chars)
    alphanumeric = sum(1 for c in text if c.isalnum())
    if alphanumeric == 0:
        return True
    return (symbols / max(alphanumeric, 1)) > 0.15


def save_image_as_png(raw_bytes: bytes, dest_path: Path) -> bool:
    """Converts any extracted image format to strictly PNG."""
    if HAVE_PIL:
        try:
            with Image.open(io.BytesIO(raw_bytes)) as img:
                w, h = img.size
                if w < 100 or h < 100:
                    return False
                if img.mode not in ("RGB", "RGBA"):
                    img = img.convert("RGBA" if "A" in img.mode else "RGB")
                img.save(dest_path.with_suffix(".png"), format="PNG", optimize=True)
                return True
        except Exception:
            return False
    return False


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
            for raw_line in text.splitlines():
                line = clean_line_text(raw_line)
                if not line:
                    continue
                if is_symbol_heavy(line):
                    symbol_lines.append(line)
                else:
                    prose_lines.append(line)

            # Strict PNG image extraction
            if extract_images:
                for img_info in page.get_images(full=True):
                    xref = img_info[0]
                    if xref in seen_xrefs:
                        continue
                    seen_xrefs.add(xref)
                    try:
                        pix = fitz.Pixmap(doc, xref)
                        if pix.width < 100 or pix.height < 100:
                            pix = None
                            continue
                        # Ensure valid RGB/RGBA for PNG
                        if pix.n >= 5:
                            pix_rgb = fitz.Pixmap(fitz.csRGB, pix)
                            pix = None
                            pix = pix_rgb

                        png_dest = images_dir / f"{stem}_xref{xref}.png"
                        pix.save(str(png_dest))
                        pix = None
                        image_count += 1
                    except Exception:
                        # Fallback via raw bytes + PIL
                        try:
                            base_img = doc.extract_image(xref)
                            b_bytes = base_img.get("image")
                            if b_bytes:
                                png_dest = images_dir / f"{stem}_xref{xref}.png"
                                if save_image_as_png(b_bytes, png_dest):
                                    image_count += 1
                        except Exception:
                            pass
        except Exception:
            continue

    doc.close()

    words_text = 0
    if prose_lines:
        md_text_path = text_dir / f"{stem}_text.md"
        with open(md_text_path, "w", encoding="utf-8") as f:
            f.write(f"# {stem} — Prose Corpus\n\n")
            f.write("\n\n".join(prose_lines) + "\n")
        words_text = sum(len(line.split()) for line in prose_lines)

    words_symbols = 0
    if symbol_lines:
        md_symbols_path = symbols_dir / f"{stem}_symbols.md"
        with open(md_symbols_path, "w", encoding="utf-8") as f:
            f.write(f"# {stem} — Mathematical & Structural Symbols\n\n")
            f.write("```axiom\n" + "\n".join(symbol_lines) + "\n```\n")
        words_symbols = sum(len(line.split()) for line in symbol_lines)

    print(f"  [OK] Extracted: {words_text:,} prose words (.md) | {words_symbols:,} symbols (.md) | {image_count} images (.png)")
    return words_text, words_symbols, image_count


def separate_with_pypdf(pdf_path: Path, text_dir: Path, symbols_dir: Path, images_dir: Path, extract_images: bool = False):
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
            for raw_line in page_text.splitlines():
                line = clean_line_text(raw_line)
                if not line:
                    continue
                if is_symbol_heavy(line):
                    symbol_lines.append(line)
                else:
                    prose_lines.append(line)

            if extract_images and hasattr(page, "images"):
                for img_idx, img_file in enumerate(page.images):
                    try:
                        png_dest = images_dir / f"{stem}_p{page_idx}_img{img_idx}.png"
                        if save_image_as_png(img_file.data, png_dest):
                            image_count += 1
                    except Exception:
                        pass
        except Exception:
            continue

    words_text = 0
    if prose_lines:
        md_text_path = text_dir / f"{stem}_text.md"
        with open(md_text_path, "w", encoding="utf-8") as f:
            f.write(f"# {stem} — Prose Corpus\n\n")
            f.write("\n\n".join(prose_lines) + "\n")
        words_text = sum(len(line.split()) for line in prose_lines)

    words_symbols = 0
    if symbol_lines:
        md_symbols_path = symbols_dir / f"{stem}_symbols.md"
        with open(md_symbols_path, "w", encoding="utf-8") as f:
            f.write(f"# {stem} — Mathematical & Structural Symbols\n\n")
            f.write("```axiom\n" + "\n".join(symbol_lines) + "\n```\n")
        words_symbols = sum(len(line.split()) for line in symbol_lines)

    print(f"  [OK] Extracted: {words_text:,} prose words (.md) | {words_symbols:,} symbols (.md) | {image_count} images (.png)")
    return words_text, words_symbols, image_count


def separate_pdf(pdf_path: Path, text_dir: Path, symbols_dir: Path, images_dir: Path, extract_images: bool = False):
    print(f"[..] Processing: {pdf_path.name}")
    if HAVE_FITZ:
        return separate_with_fitz(pdf_path, text_dir, symbols_dir, images_dir, extract_images)
    else:
        return separate_with_pypdf(pdf_path, text_dir, symbols_dir, images_dir, extract_images)


def main():
    parser = argparse.ArgumentParser(description="Axiom Multimodal High-Speed PDF Separator (.md & .png strictly)")
    parser.add_argument("--input-dir", type=str, default="asset_sample")
    parser.add_argument("--output-dir", type=str, default="asset_sample")
    parser.add_argument("--extract-images", action="store_true", default=False,
                        help="Extract embedded diagrams & figures strictly as PNG (default: False)")
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

    engine_name = "PyMuPDF (Strict .md & .png Engine)" if HAVE_FITZ else "pypdf (.md & .png Engine)"
    print(f"[+] Found {len(pdf_files)} PDF file(s) in {input_path}")
    print(f"  -> Extraction Engine:   {engine_name}")
    print(f"  -> Output Format:       Strictly .md (Markdown text & symbols) and .png (Images)")
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
    print(f"[OK] Ingestion Pipeline Ready (.md & .png only):")
    print(f"     Total Prose Words (.md):   {total_prose:,}")
    print(f"     Total Symbol Words (.md):  {total_symbols:,}")
    print(f"     Total Extracted PNG:       {total_images:,}")


if __name__ == "__main__":
    main()
