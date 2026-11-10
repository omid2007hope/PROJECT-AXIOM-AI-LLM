#!/usr/bin/env python3
"""
Project Axiom — Document & Media Separator
Splits raw mixed-media PDFs into:
  - asset_text/    (Clean prose for language training)
  - asset_symbols/ (Math, tables, datasheets, code)
  - asset_images/  (Extracted raster diagrams & figures)
"""

import sys
import os
import argparse
from pathlib import Path
from pypdf import PdfReader


def is_symbol_heavy(text: str) -> bool:
    """Classifies whether a text block is math/code/table or standard prose."""
    if not text:
        return False
    symbol_chars = set("=+\\-*/^_{}[]<>|~#$@%&`\\")
    symbols = sum(1 for c in text if c in symbol_chars)
    alphanumeric = sum(1 for c in text if c.isalnum())
    if alphanumeric == 0:
        return True
    return (symbols / max(alphanumeric, 1)) > 0.15


def separate_pdf(pdf_path: Path, text_dir: Path, symbols_dir: Path, images_dir: Path):
    print(f"[..] Processing: {pdf_path.name}")
    try:
        reader = PdfReader(str(pdf_path))
    except Exception as e:
        print(f"  [!] Failed to read PDF {pdf_path.name}: {e}")
        return 0, 0, 0

    prose_lines = []
    symbol_lines = []
    image_count = 0

    stem = pdf_path.stem

    for page_idx, page in enumerate(reader.pages):
        try:
            # 1. Extract text content
            page_text = page.extract_text() or ""
            for line in page_text.splitlines():
                clean_line = " ".join(line.split())
                if not clean_line:
                    continue
                if is_symbol_heavy(clean_line):
                    symbol_lines.append(clean_line)
                else:
                    prose_lines.append(clean_line)

            # 2. Extract embedded images
            if hasattr(page, "images"):
                for img_idx, img in enumerate(page.images):
                    try:
                        img_filename = f"{stem}_p{page_idx + 1}_img{img_idx + 1}.png"
                        img_out_path = images_dir / img_filename
                        with open(img_out_path, "wb") as f_img:
                            f_img.write(img.data)
                        image_count += 1
                    except Exception:
                        pass
        except Exception:
            continue

    # Write prose text file
    words_text = 0
    if prose_lines:
        prose_file = text_dir / f"{stem}_text.txt"
        with open(prose_file, "w", encoding="utf-8") as f:
            f.write("\n".join(prose_lines) + "\n")
        words_text = sum(len(line.split()) for line in prose_lines)

    # Write symbols file
    words_symbols = 0
    if symbol_lines:
        symbol_file = symbols_dir / f"{stem}_symbols.txt"
        with open(symbol_file, "w", encoding="utf-8") as f:
            f.write("\n".join(symbol_lines) + "\n")
        words_symbols = sum(len(line.split()) for line in symbol_lines)

    print(f"  [OK] Extracted: {words_text} prose words | {words_symbols} symbol words | {image_count} images")
    return words_text, words_symbols, image_count


def main():
    parser = argparse.ArgumentParser(description="Axiom Multimodal PDF Separator")
    parser.add_argument("--input-dir", type=str, default="core/asset_sample",
                        help="Directory containing source PDF files")
    parser.add_argument("--output-dir", type=str, default="core/asset_sample",
                        help="Base directory for separated asset folders")
    args = parser.parse_args()

    input_path = Path(args.input_dir).resolve()
    if not input_path.exists():
        # Fallback to local asset_sample if running from PROJECT-AXIOM root
        input_path = Path("asset_sample").resolve()
        if not input_path.exists():
            print(f"[!] Error: Input directory '{args.input_dir}' not found.")
            sys.exit(1)

    output_base = Path(args.output_dir).resolve()
    text_dir = output_base / "text"
    symbols_dir = output_base / "symbols"
    images_dir = output_base / "images"

    for d in [text_dir, symbols_dir, images_dir]:
        d.mkdir(parents=True, exist_ok=True)

    pdf_files = list(input_path.glob("*.pdf"))
    if not pdf_files:
        print(f"[!] No PDF files found in {input_path}")
        sys.exit(0)

    print(f"[+] Found {len(pdf_files)} PDF file(s) in {input_path}")
    print(f"  -> Text destination:    {text_dir}")
    print(f"  -> Symbols destination: {symbols_dir}")
    print(f"  -> Images destination:  {images_dir}")
    print("-" * 60)

    total_prose = 0
    total_symbols = 0
    total_images = 0

    for pdf in pdf_files:
        t_words, s_words, imgs = separate_pdf(pdf, text_dir, symbols_dir, images_dir)
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

