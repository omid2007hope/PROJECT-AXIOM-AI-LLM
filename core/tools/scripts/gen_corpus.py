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
Project Axiom — Unified High-Speed Corpus & Media Extractor
Ingests multimodal documents (PDF, DOCX, TXT, MD, and raster images) and
outputs STRICTLY normalized .md and .png files:
  - <output_dir>/text/    (*_text.md: Clean normalized prose in Markdown)
  - <output_dir>/symbols/ (*_symbols.md: Math, tables, code in Markdown)
  - <output_dir>/images/  (*.png: Diagrams, figures & converted media as PNG)

Or in --flat mode:
  - <output_dir>/*.md
  - <output_dir>/images/*.png

Only .md and .png output formats are ever written.
"""

import argparse
import io
import os
import re
import sys
import unicodedata
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path
from typing import List, Set, Tuple

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

try:
    import pypdf
    HAVE_PYPDF = True
except ImportError:
    HAVE_PYPDF = False

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

TEXT_EXTENSIONS = {".txt", ".text", ".md", ".markdown"}
IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".webp", ".bmp", ".tif", ".tiff", ".gif"}
SYMBOL_CHARS = set("=+-*/^_{}[]<>|~#$@%&`\\")
MIN_IMAGE_DIM = 100


def normalize_unicode(text: str) -> str:
    """Expands ligatures, normalizes typography, and strips control characters."""
    text = unicodedata.normalize("NFKD", text)
    for ligature, replacement in LIGATURE_MAP.items():
        text = text.replace(ligature, replacement)

    cleaned_chars = [
        ch if (ord(ch) in (9, 10, 13) or (32 <= ord(ch) < 127) or ord(ch) >= 160) else " "
        for ch in text
    ]
    return "".join(cleaned_chars)


def clean_line_text(line: str) -> str:
    """Normalizes a single line to clean UTF-8 ASCII-compatible tokens."""
    return " ".join(normalize_unicode(line).split())


def clean_document_text(text: str) -> str:
    """Normalizes multi-line document text while preserving paragraph breaks."""
    cleaned = normalize_unicode(text)
    cleaned = re.sub(r"[ \t]+", " ", cleaned)
    cleaned = re.sub(r"\n{3,}", "\n\n", cleaned)
    return cleaned.strip()


def is_symbol_heavy(text: str) -> bool:
    """Returns True if a line is predominantly math, code, or structural symbols."""
    if not text:
        return False
    symbols = sum(1 for c in text if c in SYMBOL_CHARS)
    alphanumeric = sum(1 for c in text if c.isalnum())
    if alphanumeric == 0:
        return True
    return (symbols / max(alphanumeric, 1)) > 0.15


def save_image_as_png(raw_bytes: bytes, dest_path: Path, min_dim: int = MIN_IMAGE_DIM) -> bool:
    """Strictly converts raw image bytes to an optimized PNG file."""
    png_path = dest_path.with_suffix(".png")

    if HAVE_PIL:
        try:
            with Image.open(io.BytesIO(raw_bytes)) as img:
                w, h = img.size
                if w < min_dim or h < min_dim:
                    return False
                if img.mode not in ("RGB", "RGBA"):
                    img = img.convert("RGBA" if "A" in img.mode else "RGB")
                img.save(png_path, format="PNG", optimize=True)
                return True
        except Exception:
            return False

    # Fallback via PyMuPDF if PIL is unavailable
    if HAVE_FITZ:
        try:
            pix = fitz.Pixmap(raw_bytes)
            if pix.width < min_dim or pix.height < min_dim:
                return False
            if pix.n - pix.alpha >= 4:
                pix = fitz.Pixmap(fitz.csRGB, pix)
            pix.save(str(png_path))
            return True
        except Exception:
            return False

    return False


def write_markdown_outputs(
    stem: str,
    raw_text: str,
    output_base: Path,
    text_dir: Path,
    symbols_dir: Path,
    flat: bool = False,
) -> Tuple[int, int]:
    """Splits and writes strictly .md outputs, returning (prose_words, symbol_words)."""
    if flat:
        cleaned = clean_document_text(raw_text)
        if not cleaned:
            return 0, 0
        md_path = (output_base / stem).with_suffix(".md")
        md_path.write_text(cleaned + "\n", encoding="utf-8")
        return len(cleaned.split()), 0

    prose_lines: List[str] = []
    symbol_lines: List[str] = []

    for raw_line in raw_text.splitlines():
        line = clean_line_text(raw_line)
        if not line:
            continue
        if is_symbol_heavy(line):
            symbol_lines.append(line)
        else:
            prose_lines.append(line)

    words_text = 0
    if prose_lines:
        md_text_path = (text_dir / f"{stem}_text").with_suffix(".md")
        content = f"# {stem} — Prose Corpus\n\n" + "\n\n".join(prose_lines) + "\n"
        md_text_path.write_text(content, encoding="utf-8")
        words_text = sum(len(line.split()) for line in prose_lines)

    words_symbols = 0
    if symbol_lines:
        md_symbols_path = (symbols_dir / f"{stem}_symbols").with_suffix(".md")
        content = (
            f"# {stem} — Mathematical & Structural Symbols\n\n"
            "```axiom\n" + "\n".join(symbol_lines) + "\n```\n"
        )
        md_symbols_path.write_text(content, encoding="utf-8")
        words_symbols = sum(len(line.split()) for line in symbol_lines)

    return words_text, words_symbols


def extract_pdf_fitz(
    pdf_path: Path,
    stem: str,
    images_dir: Path,
    extract_images: bool,
) -> Tuple[str, int]:
    """Extracts PDF text and optional PNG images using PyMuPDF."""
    pages_text: List[str] = []
    image_count = 0
    seen_xrefs: Set[int] = set()

    try:
        doc = fitz.open(str(pdf_path))
    except Exception as e:
        print(f"  [!] Failed to open {pdf_path.name}: {e}", file=sys.stderr)
        return "", 0

    try:
        for page_idx in range(len(doc)):
            try:
                page = doc[page_idx]
                text = page.get_text("text") or ""
                if text:
                    pages_text.append(text)

                if extract_images:
                    for img_info in page.get_images(full=True):
                        xref = img_info[0]
                        if xref in seen_xrefs:
                            continue
                        seen_xrefs.add(xref)
                        png_dest = (images_dir / f"{stem}_xref{xref}").with_suffix(".png")
                        try:
                            pix = fitz.Pixmap(doc, xref)
                            if pix.width < MIN_IMAGE_DIM or pix.height < MIN_IMAGE_DIM:
                                pix = None
                                continue
                            # Convert CMYK / high-channel colorspaces to RGB for PNG
                            if pix.n - pix.alpha >= 4:
                                pix = fitz.Pixmap(fitz.csRGB, pix)
                            pix.save(str(png_dest))
                            pix = None
                            image_count += 1
                        except Exception:
                            try:
                                base_img = doc.extract_image(xref)
                                b_bytes = base_img.get("image")
                                if b_bytes and save_image_as_png(b_bytes, png_dest):
                                    image_count += 1
                            except Exception:
                                pass
            except Exception:
                continue
    finally:
        doc.close()

    return "\n\n".join(pages_text), image_count


def extract_pdf_pypdf(
    pdf_path: Path,
    stem: str,
    images_dir: Path,
    extract_images: bool,
) -> Tuple[str, int]:
    """Extracts PDF text and optional PNG images using pypdf."""
    pages_text: List[str] = []
    image_count = 0

    try:
        reader = pypdf.PdfReader(str(pdf_path))
    except Exception as e:
        print(f"  [!] Failed to read PDF {pdf_path.name}: {e}", file=sys.stderr)
        return "", 0

    for page_idx, page in enumerate(reader.pages):
        try:
            page_text = page.extract_text() or ""
            if page_text:
                pages_text.append(page_text)

            if extract_images and hasattr(page, "images"):
                for img_idx, img_file in enumerate(page.images):
                    try:
                        png_dest = (images_dir / f"{stem}_p{page_idx}_img{img_idx}").with_suffix(".png")
                        if save_image_as_png(img_file.data, png_dest):
                            image_count += 1
                    except Exception:
                        pass
        except Exception:
            continue

    return "\n\n".join(pages_text), image_count


def extract_docx(
    docx_path: Path,
    stem: str,
    images_dir: Path,
    extract_images: bool,
) -> Tuple[str, int]:
    """Extracts DOCX text and embedded media (as PNG) using stdlib zipfile + XML."""
    paragraphs: List[str] = []
    image_count = 0

    try:
        with zipfile.ZipFile(docx_path, "r") as zf:
            if "word/document.xml" in zf.namelist():
                xml_bytes = zf.read("word/document.xml")
                root = ET.fromstring(xml_bytes)
                ns = {"w": "http://schemas.openxmlformats.org/wordprocessingml/2006/main"}
                for p in root.findall(".//w:p", ns):
                    texts = [t.text for t in p.findall(".//w:t", ns) if t.text]
                    if texts:
                        paragraphs.append("".join(texts))

            if extract_images:
                media_files = [n for n in zf.namelist() if n.startswith("word/media/") and not n.endswith("/")]
                for idx, media_name in enumerate(media_files):
                    try:
                        raw_bytes = zf.read(media_name)
                        png_dest = (images_dir / f"{stem}_media{idx}").with_suffix(".png")
                        if save_image_as_png(raw_bytes, png_dest):
                            image_count += 1
                    except Exception:
                        pass
    except Exception as e:
        print(f"  [!] Failed to read DOCX {docx_path.name}: {e}", file=sys.stderr)
        return "", 0

    return "\n".join(paragraphs), image_count


def unique_stem(source_path: Path, input_root: Path, used_stems: Set[str]) -> str:
    """Generates a collision-free output stem."""
    stem = source_path.stem
    if stem not in used_stems:
        used_stems.add(stem)
        return stem

    rel = source_path.relative_to(input_root).with_suffix("")
    candidate = "_".join(rel.parts)
    counter = 1
    while candidate in used_stems:
        candidate = f"{stem}_{counter}"
        counter += 1
    used_stems.add(candidate)
    return candidate


def process_file(
    source_path: Path,
    stem: str,
    output_base: Path,
    text_dir: Path,
    symbols_dir: Path,
    images_dir: Path,
    extract_images: bool,
    flat: bool,
) -> Tuple[bool, int, int, int]:
    """Processes a single supported document or image file."""
    ext = source_path.suffix.lower()
    raw_text = ""
    image_count = 0

    if ext == ".pdf":
        print(f"[..] Processing PDF:  {source_path.name}")
        if HAVE_FITZ:
            raw_text, image_count = extract_pdf_fitz(source_path, stem, images_dir, extract_images)
        elif HAVE_PYPDF:
            raw_text, image_count = extract_pdf_pypdf(source_path, stem, images_dir, extract_images)
        else:
            print(f"  [WARN] No PDF engine (pymupdf/pypdf) installed for {source_path.name}", file=sys.stderr)
            return False, 0, 0, 0

    elif ext == ".docx":
        print(f"[..] Processing DOCX: {source_path.name}")
        raw_text, image_count = extract_docx(source_path, stem, images_dir, extract_images)

    elif ext in TEXT_EXTENSIONS:
        print(f"[..] Processing TEXT: {source_path.name}")
        try:
            raw_text = source_path.read_text(encoding="utf-8", errors="replace")
        except Exception as e:
            print(f"  [!] Failed to read {source_path.name}: {e}", file=sys.stderr)
            return False, 0, 0, 0

    elif ext in IMAGE_EXTENSIONS:
        if not extract_images:
            return False, 0, 0, 0
        print(f"[..] Processing IMG:  {source_path.name}")
        try:
            raw_bytes = source_path.read_bytes()
            png_dest = (images_dir / stem).with_suffix(".png")
            if save_image_as_png(raw_bytes, png_dest):
                print(f"  [OK] Converted -> {png_dest.name} (.png)")
                return True, 0, 0, 1
        except Exception as e:
            print(f"  [!] Failed to convert image {source_path.name}: {e}", file=sys.stderr)
        return False, 0, 0, 0

    else:
        return False, 0, 0, 0

    words_text, words_symbols = write_markdown_outputs(
        stem=stem,
        raw_text=raw_text,
        output_base=output_base,
        text_dir=text_dir,
        symbols_dir=symbols_dir,
        flat=flat,
    )

    if words_text == 0 and words_symbols == 0 and image_count == 0:
        return False, 0, 0, 0

    print(
        f"  [OK] Extracted: {words_text:,} prose words (.md) | "
        f"{words_symbols:,} symbols (.md) | {image_count} images (.png)"
    )
    return True, words_text, words_symbols, image_count


def resolve_smart_path(raw_path: str) -> Path:
    """Resolves paths with fallback support for core/ workspace layouts."""
    p = Path(raw_path)
    if not p.exists():
        if raw_path.startswith("core/") and Path(raw_path[5:]).exists():
            p = Path(raw_path[5:])
        elif (Path("core") / raw_path).exists():
            p = Path("core") / raw_path
    return p.resolve()


def discover_files(input_path: Path, excluded_dirs: Set[Path], recursive: bool) -> List[Path]:
    """Discovers supported files while skipping output directories to prevent loops."""
    supported = {".pdf", ".docx"} | TEXT_EXTENSIONS | IMAGE_EXTENSIONS
    discovered: List[Path] = []

    if not recursive:
        for item in sorted(input_path.iterdir()):
            if item.is_file() and item.suffix.lower() in supported:
                discovered.append(item)
        return discovered

    for root, dirs, files in os.walk(input_path):
        root_path = Path(root).resolve()
        # Prune output directories and hidden directories
        dirs[:] = sorted(
            d for d in dirs
            if not d.startswith(".") and (root_path / d).resolve() not in excluded_dirs
        )
        for fname in sorted(files):
            fpath = root_path / fname
            if fpath.suffix.lower() in supported:
                discovered.append(fpath)

    return discovered


def main():
    parser = argparse.ArgumentParser(
        description="Axiom Unified Multimodal Corpus Extractor (strictly .md & .png outputs)"
    )
    # Supports both positional (<input_dir> <output_dir>) and flag-based invocation
    parser.add_argument("pos_input_dir", nargs="?", default=None, help="Optional positional input directory")
    parser.add_argument("pos_output_dir", nargs="?", default=None, help="Optional positional output directory")
    parser.add_argument("--input-dir", type=str, default=None, help="Input directory (default: asset_sample)")
    parser.add_argument("--output-dir", type=str, default=None, help="Output directory (default: asset_sample)")
    parser.add_argument(
        "--extract-images",
        dest="extract_images",
        action="store_true",
        default=True,
        help="Extract & convert images strictly as PNG (default: enabled)",
    )
    parser.add_argument(
        "--no-images",
        dest="extract_images",
        action="store_false",
        help="Disable image extraction and conversion",
    )
    parser.add_argument(
        "--flat",
        action="store_true",
        default=False,
        help="Write single <stem>.md per document in output root instead of splitting text/ and symbols/",
    )
    parser.add_argument(
        "--no-recursive",
        action="store_true",
        default=False,
        help="Only scan top-level input directory without recursing into subfolders",
    )
    args = parser.parse_args()

    in_arg = args.input_dir or args.pos_input_dir or "asset_sample"
    out_arg = args.output_dir or args.pos_output_dir or "asset_sample"

    input_path = resolve_smart_path(in_arg)
    if not input_path.exists():
        fallback = Path("asset_sample").resolve()
        if fallback.exists():
            input_path = fallback
        else:
            print(f"[!] Error: Input directory '{in_arg}' not found.", file=sys.stderr)
            sys.exit(1)

    output_base = resolve_smart_path(out_arg)
    text_dir = output_base / "text"
    symbols_dir = output_base / "symbols"
    images_dir = output_base / "images"

    output_base.mkdir(parents=True, exist_ok=True)
    if not args.flat:
        text_dir.mkdir(parents=True, exist_ok=True)
        symbols_dir.mkdir(parents=True, exist_ok=True)
    if args.extract_images:
        images_dir.mkdir(parents=True, exist_ok=True)

    # Prevent walking into generated output directories
    excluded_dirs: Set[Path] = {text_dir.resolve(), symbols_dir.resolve(), images_dir.resolve()}
    if output_base != input_path:
        excluded_dirs.add(output_base.resolve())

    files = discover_files(input_path, excluded_dirs, recursive=not args.no_recursive)
    if not files:
        print(f"[!] No supported files found in {input_path}")
        sys.exit(0)

    if HAVE_FITZ:
        pdf_engine = "PyMuPDF (fitz)"
    elif HAVE_PYPDF:
        pdf_engine = "pypdf"
    else:
        pdf_engine = "None (PDF extraction unavailable)"

    img_engine = "Pillow + PyMuPDF" if (HAVE_PIL and HAVE_FITZ) else ("Pillow" if HAVE_PIL else ("PyMuPDF" if HAVE_FITZ else "None"))

    print(f"[+] Found {len(files)} file(s) in {input_path}")
    print(f"  -> PDF Engine:          {pdf_engine}")
    print(f"  -> PNG Image Engine:    {img_engine} (extract_images={args.extract_images})")
    print(f"  -> Output Format:       Strictly .md and .png")
    if args.flat:
        print(f"  -> Markdown dest:       {output_base} (flat mode)")
    else:
        print(f"  -> Text destination:    {text_dir}")
        print(f"  -> Symbols destination: {symbols_dir}")
    if args.extract_images:
        print(f"  -> Images destination:  {images_dir}")
    print("-" * 60)

    processed_count = 0
    total_prose = 0
    total_symbols = 0
    total_images = 0
    used_stems: Set[str] = set()

    for fpath in files:
        stem = unique_stem(fpath, input_path, used_stems)
        ok, t_words, s_words, imgs = process_file(
            source_path=fpath,
            stem=stem,
            output_base=output_base,
            text_dir=text_dir,
            symbols_dir=symbols_dir,
            images_dir=images_dir,
            extract_images=args.extract_images,
            flat=args.flat,
        )
        if ok:
            processed_count += 1
            total_prose += t_words
            total_symbols += s_words
            total_images += imgs

    print("=" * 60)
    print(f"[OK] Ingestion Pipeline Complete (.md & .png only):")
    print(f"     Files Processed:           {processed_count:,}")
    print(f"     Total Prose Words (.md):   {total_prose:,}")
    print(f"     Total Symbol Words (.md):  {total_symbols:,}")
    print(f"     Total Extracted PNG:       {total_images:,}")


if __name__ == "__main__":
    main()