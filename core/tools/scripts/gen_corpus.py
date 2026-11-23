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
Project Axiom — Unified Layout-Aware Corpus & Vision Extractor (Tier 1 + Tier 2)

Ingests multimodal documents (PDF, DOCX, TXT, MD, and raster images) and outputs
STRICTLY normalized .md and .png files:
  - <output_dir>/text/<stem>.md       (Context-preserving Markdown: headings, prose,
                                       tables, code, math, interleaved ![caption](...) links)
  - <output_dir>/images/*.png         (Filtered raster + vector figures, normalized PNG)
  - <output_dir>/symbols/*_symbols.md (Optional secondary view via --symbols-view)

Or in --flat mode:
  - <output_dir>/<stem>.md
  - <output_dir>/images/*.png

Only .md and .png output formats are ever written.
"""

import argparse
from collections import Counter, defaultdict
from dataclasses import dataclass, field
import hashlib
import io
import math
import os
from pathlib import Path
import posixpath
import re
import statistics
import sys
from typing import Dict, List, Optional, Sequence, Set, Tuple
import unicodedata
import xml.etree.ElementTree as ET
import zipfile

try:
    from PIL import Image, ImageStat

    HAVE_PIL = True
except ImportError:
    HAVE_PIL = False

try:
    import pymupdf as pdf_engine

    HAVE_PYMUPDF = True
except ImportError:
    try:
        import fitz as pdf_engine

        HAVE_PYMUPDF = True
    except ImportError:
        HAVE_PYMUPDF = False

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

INVISIBLE_PATTERN = re.compile(r"[\u00ad\u200b-\u200f\u202a-\u202e\u2060\ufeff\ufffd]")
CID_PATTERN = re.compile(r"\(cid:\d+\)")
PAGE_NUMBER_PATTERN = re.compile(
    r"^\s*(?:page\s*)?\d+(?:\s*(?:/|of)\s*\d+)?\s*$", re.IGNORECASE
)
ROMAN_PAGE_PATTERN = re.compile(r"^\s*[ivxlcdm]{1,6}\s*$", re.IGNORECASE)
TOC_LEADER_PATTERN = re.compile(r"(?:\.{4,}|(?:\.\s){4,})\s*\d+\s*$")
WATERMARK_PATTERN = re.compile(
    r"^\s*(?:confidential|draft|do not distribute|internal use only|"
    r"all rights reserved\.?|downloaded from\b.*|copyright\s*(?:©|\(c\))?\s*\d{4}.*|"
    r"accept all cookies.*|we use cookies.*|cookie policy.*|privacy policy.*)\s*$",
    re.IGNORECASE,
)
REPEATED_RUN_PATTERN = re.compile(r"([A-Za-z._=~*#])\1{7,}")
LIST_ITEM_PATTERN = re.compile(r"^\s*(?:[-*•●▪◦]|\d+[.)]|\([a-zA-Z0-9]{1,3}\))\s+")
CAPTION_PATTERN = re.compile(
    r"^(?:Figure|Fig\.?|Diagram|Chart|Scheme|Plate|Table|Listing)\s*\d+[\.:a-zA-Z0-9\-]*",
    re.IGNORECASE,
)
REFERENCE_HEADING_PATTERN = re.compile(
    r"^#{1,3}\s*(?:references|bibliography|works cited)\s*$", re.IGNORECASE
)
CITATION_LINE_PATTERN = re.compile(
    r"(?:^\s*\[\d+\]|\bet\s+al\.|\b(?:19|20)\d{2}[a-z]?\b|\bdoi:|\barXiv:)",
    re.IGNORECASE,
)
URL_TOKEN_PATTERN = re.compile(r"^(?:https?://|www\.|ftp://|[a-zA-Z0-9_./\\:-]+/[a-zA-Z0-9_./\\:-]+)")

MONOSPACE_HINTS = (
    "courier",
    "consolas",
    "mono",
    "inconsolata",
    "menlo",
    "fira",
    "sourcecode",
    "fixed",
    "cmtt",
    "lmtt",
    "sfmono",
    "jetbrains",
    "hack",
)

KEEP_HYPHEN_PREFIXES = {
    "anti",
    "auto",
    "bi",
    "co",
    "cross",
    "e",
    "ex",
    "full",
    "half",
    "high",
    "in",
    "inter",
    "intra",
    "low",
    "meta",
    "micro",
    "mid",
    "mini",
    "multi",
    "near",
    "non",
    "off",
    "on",
    "out",
    "over",
    "post",
    "pre",
    "pro",
    "pseudo",
    "quasi",
    "re",
    "real",
    "self",
    "semi",
    "short",
    "single",
    "sub",
    "super",
    "time",
    "two",
    "ultra",
    "under",
    "up",
    "well",
    "zero",
}

MATH_OPERATORS = set(
    "=<>±×÷∑∏∫∂∇√∞≈≠≤≥∈∉⊂⊃∪∩∀∃→←↔⇒⇔αβγδεζηθικλμνξπρστυφχψωΔΘΛΞΠΣΦΨΩ^_²³⁴⁵⁶⁷⁸⁹⁰⁻¹⁺"
)
SYMBOL_CHARS = set("=+-*/^_{}[]<>|~#$@%&`\\") | MATH_OPERATORS
TEXT_EXTENSIONS = {".txt", ".text", ".md", ".markdown"}
IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".webp", ".bmp", ".tif", ".tiff", ".gif"}

MIN_IMAGE_SIZE = 100
MIN_IMAGE_AREA = 25000
MAX_ASPECT_RATIO = 8.0
MIN_IMAGE_ENTROPY = 1.5
MIN_IMAGE_DEVIATION = 12.0
MAX_IMAGE_EDGE = 1536
DOCUMENT_LOGO_PAGES = 3
CORPUS_LOGO_DOCUMENTS = 5
VECTOR_DPI = 200
MINHASH_SIZE = 64
MINHASH_BANDS = 16
MINHASH_ROWS = 4
MINHASH_PRIME = (1 << 61) - 1
NEAR_DUPLICATE_THRESHOLD = 0.80

MINHASH_COEFFICIENTS: List[Tuple[int, int]] = [
    ((i * 2654435761 + 1013904223) % (MINHASH_PRIME - 1) + 1, (i * 1597334677 + 3812015801) % MINHASH_PRIME)
    for i in range(1, MINHASH_SIZE + 1)
]


@dataclass
class PageItem:
    """Single positioned content unit on a document page."""

    bbox: Tuple[float, float, float, float]
    kind: str  # "heading", "prose", "code", "math", "table", "image"
    content: str
    raw_lines: List[str] = field(default_factory=list)


@dataclass
class ExtractedDocument:
    """Structured extraction result ready for filtering and Markdown emission."""

    source_type: str
    page_count: int
    blocks: List[Tuple[str, str]]  # (kind, markdown_text)
    image_count: int
    scanned_pages: int = 0
    corrupt_characters: int = 0
    raw_characters: int = 0
    saved_images: List[Path] = field(default_factory=list)
    image_dhashes: List[int] = field(default_factory=list)
    image_digests: Set[str] = field(default_factory=set)


@dataclass
class CorpusState:
    """Cross-document deduplication and repeated-asset tracking state."""

    exact_hashes: Set[str] = field(default_factory=set)
    lsh_buckets: Dict[Tuple[int, Tuple[int, ...]], List[int]] = field(
        default_factory=lambda: defaultdict(list)
    )
    minhash_signatures: List[Tuple[int, ...]] = field(default_factory=list)
    paragraph_counts: Counter = field(default_factory=Counter)
    image_doc_counts: Counter = field(default_factory=Counter)
    corpus_dhashes: Set[int] = field(default_factory=set)


def count_corrupt_characters(raw_text: str) -> Tuple[int, int]:
    """Counts unresolved CID tags, private-use codepoints, and U+FFFD chars."""
    if not raw_text:
        return 0, 0
    cid_chars = sum(len(match.group(0)) for match in CID_PATTERN.finditer(raw_text))
    bad_codepoints = sum(
        1 for ch in raw_text if ch == "\ufffd" or (0xE000 <= ord(ch) <= 0xF8FF)
    )
    return cid_chars + bad_codepoints, len(raw_text)


def normalize_unicode(text: str) -> str:
    """Applies NFC normalization, expands ligatures, and strips invisible noise."""
    if not text:
        return ""
    text = unicodedata.normalize("NFC", text)
    for ligature, replacement in LIGATURE_MAP.items():
        if ligature in text:
            text = text.replace(ligature, replacement)
    text = CID_PATTERN.sub("", text)
    text = INVISIBLE_PATTERN.sub("", text)
    cleaned: List[str] = []
    for ch in text:
        code = ord(ch)
        if code in (9, 10) or (32 <= code < 127) or (code >= 160 and not (0xE000 <= code <= 0xF8FF)):
            cleaned.append(ch)
        elif code == 13:
            cleaned.append("\n")
        else:
            cleaned.append(" ")
    return "".join(cleaned)


def normalize_template(text: str) -> str:
    """Normalizes digits to '#' for running header/footer template detection."""
    compact = " ".join(normalize_unicode(text).lower().split())
    return re.sub(r"\d+", "#", compact)


def dehyphenate_join(previous: str, following: str) -> str:
    """Joins two wrapped lines while repairing broken soft hyphens."""
    left = previous.rstrip()
    right = following.lstrip()
    if not left:
        return right
    if not right:
        return left

    if left.endswith("-") and len(left) >= 2 and left[-2].isalpha():
        last_token = left.split()[-1]
        if last_token.count("-") >= 2 and (right[0].islower() or right[0].isupper()):
            return left + right
        match = re.search(r"([A-Za-z]+)-$", left)
        prefix = match.group(1).lower() if match else ""
        if right[0].islower() and prefix not in KEEP_HYPHEN_PREFIXES and len(prefix) >= 2:
            return left[:-1] + right
        if right[0].islower() or right[0].isupper():
            return left + right
    return f"{left} {right}"


def is_monospace_span(font_name: str, flags: int) -> bool:
    """Detects monospace spans from PyMuPDF flags or font name."""
    if flags & 8:
        return True
    lower = font_name.lower()
    return any(hint in lower for hint in MONOSPACE_HINTS)


def is_math_display(text: str) -> bool:
    """Detects standalone display equations to wrap in $$...$$."""
    stripped = text.strip()
    if not stripped or len(stripped) > 220 or stripped.startswith(("```", "|", "#", "![", "$$")):
        return False
    if URL_TOKEN_PATTERN.match(stripped):
        return False
    math_hits = sum(1 for ch in stripped if ch in MATH_OPERATORS)
    if math_hits < 2:
        return False
    words = [w for w in stripped.split() if any(ch.isalpha() for ch in w)]
    long_prose_words = sum(1 for w in words if len(re.sub(r"[^a-zA-Z]", "", w)) >= 5)
    if long_prose_words >= 5:
        return False
    alpha_count = sum(1 for ch in stripped if ch.isalpha())
    return (math_hits / max(alpha_count, 1)) >= 0.18


def rectangle_overlap_ratio(
    inner: Tuple[float, float, float, float], outer: Tuple[float, float, float, float]
) -> float:
    """Returns fraction of inner rectangle area overlapping outer rectangle."""
    ix0 = max(inner[0], outer[0])
    iy0 = max(inner[1], outer[1])
    ix1 = min(inner[2], outer[2])
    iy1 = min(inner[3], outer[3])
    if ix1 <= ix0 or iy1 <= iy0:
        return 0.0
    inner_area = max((inner[2] - inner[0]) * (inner[3] - inner[1]), 1e-6)
    return ((ix1 - ix0) * (iy1 - iy0)) / inner_area


def sort_reading_order(items: List[PageItem], page_width: float) -> List[PageItem]:
    """Sorts page items using horizontal-band and vertical-gutter XY-cut."""
    if len(items) <= 1:
        return items

    sorted_y = sorted(items, key=lambda item: (item.bbox[1], item.bbox[0]))
    wide_threshold = 0.62 * page_width
    result: List[PageItem] = []
    band: List[PageItem] = []

    def flush_band(current_band: List[PageItem]) -> None:
        if len(current_band) <= 1:
            result.extend(current_band)
            return
        result.extend(sort_column_band(current_band, page_width))

    for item in sorted_y:
        width = item.bbox[2] - item.bbox[0]
        if width >= wide_threshold:
            flush_band(band)
            band = []
            result.append(item)
        else:
            band.append(item)

    flush_band(band)
    return result


def sort_column_band(items: List[PageItem], page_width: float) -> List[PageItem]:
    """Recursively splits multi-column bands along clear vertical gutters."""
    if len(items) <= 1:
        return items

    min_x = min(item.bbox[0] for item in items)
    max_x = max(item.bbox[2] for item in items)
    span = max_x - min_x
    if span < 0.35 * page_width:
        return sorted(items, key=lambda item: (item.bbox[1], item.bbox[0]))

    intervals = sorted((item.bbox[0], item.bbox[2]) for item in items)
    merged: List[List[float]] = []
    for x0, x1 in intervals:
        if not merged or x0 > merged[-1][1] + 2.0:
            merged.append([x0, x1])
        else:
            merged[-1][1] = max(merged[-1][1], x1)

    best_gap = 0.0
    split_x: Optional[float] = None
    for idx in range(len(merged) - 1):
        gap = merged[idx + 1][0] - merged[idx][1]
        mid = 0.5 * (merged[idx][1] + merged[idx + 1][0])
        if gap > best_gap and gap >= max(6.0, 0.015 * page_width):
            if min_x + 0.15 * span <= mid <= max_x - 0.15 * span:
                best_gap = gap
                split_x = mid

    if split_x is None:
        return sorted(items, key=lambda item: (item.bbox[1], item.bbox[0]))

    left = [item for item in items if 0.5 * (item.bbox[0] + item.bbox[2]) < split_x]
    right = [item for item in items if 0.5 * (item.bbox[0] + item.bbox[2]) >= split_x]
    if not left or not right:
        return sorted(items, key=lambda item: (item.bbox[1], item.bbox[0]))

    # Only treat as side-by-side columns when left and right overlap vertically
    left_top = min(item.bbox[1] for item in left)
    left_bottom = max(item.bbox[3] for item in left)
    right_top = min(item.bbox[1] for item in right)
    right_bottom = max(item.bbox[3] for item in right)
    if min(left_bottom, right_bottom) <= max(left_top, right_top):
        return sorted(items, key=lambda item: (item.bbox[1], item.bbox[0]))

    return sort_column_band(left, page_width) + sort_column_band(right, page_width)


def is_valid_table(
    rows: Sequence[Sequence[Optional[str]]],
    bbox: Tuple[float, float, float, float],
    page_height: float,
) -> bool:
    """Guards against find_tables() misclassifying 2-column prose as a table."""
    non_empty_rows = [
        [cell.strip() for cell in row if cell and cell.strip()] for row in rows
    ]
    non_empty_rows = [row for row in non_empty_rows if row]
    if len(non_empty_rows) < 2:
        return False
    if (bbox[3] - bbox[1]) > 0.85 * page_height and len(non_empty_rows) <= 2:
        return False
    all_cells = [cell for row in non_empty_rows for cell in row]
    if not all_cells:
        return False
    average_length = sum(len(cell) for cell in all_cells) / float(len(all_cells))
    return average_length <= 250.0


def format_markdown_table(rows: Sequence[Sequence[Optional[str]]]) -> str:
    """Serializes extracted table rows into a clean GFM pipe table."""
    cleaned_rows: List[List[str]] = []
    for row in rows:
        cells = [
            " ".join(normalize_unicode(cell or "").split()).replace("|", "\\|")
            for cell in row
        ]
        if any(cells):
            cleaned_rows.append(cells)

    if len(cleaned_rows) < 2:
        return ""
    col_count = max(len(row) for row in cleaned_rows)
    if col_count < 2:
        return ""

    padded = [row + [""] * (col_count - len(row)) for row in cleaned_rows]
    # Drop completely empty spacer columns produced by PDF rule intersections
    active_cols = [col for col in range(col_count) if any(row[col] for row in padded)]
    if len(active_cols) < 2:
        return ""
    compact_rows = [[row[col] for col in active_cols] for row in padded]

    header = "| " + " | ".join(compact_rows[0]) + " |"
    divider = "| " + " | ".join("---" for _ in active_cols) + " |"
    body = ["| " + " | ".join(row) + " |" for row in compact_rows[1:]]
    return "\n".join([header, divider] + body)


def compute_shannon_entropy(gray_image: "Image.Image") -> float:
    """Computes grayscale Shannon entropy from a Pillow L-mode image."""
    hist = gray_image.histogram()
    total = sum(hist)
    if total == 0:
        return 0.0
    entropy = 0.0
    for count in hist:
        if count > 0:
            prob = count / total
            entropy -= prob * math.log2(prob)
    return entropy


def compute_difference_hash(gray_image: "Image.Image") -> int:
    """Computes a 64-bit perceptual difference hash (dHash) via Pillow."""
    resample = getattr(getattr(Image, "Resampling", Image), "BILINEAR", 2)
    small = gray_image.resize((9, 8), resample)
    pixels = small.tobytes()
    bits = 0
    for row in range(8):
        offset = row * 9
        for col in range(8):
            bits = (bits << 1) | (1 if pixels[offset + col] > pixels[offset + col + 1] else 0)
    return bits


def hamming_distance(left_hash: int, right_hash: int) -> int:
    """Returns bit-level Hamming distance between two 64-bit hashes."""
    return (left_hash ^ right_hash).bit_count()


def save_filtered_png(
    raw_bytes: bytes,
    target_path: Path,
    document_dhashes: List[int],
    corpus_state: CorpusState,
    image_digests: Set[str],
    enable_dedup: bool = True,
    minimum_size: int = MIN_IMAGE_SIZE,
) -> str:
    """Filters low-signal/duplicate images and writes PNG. Returns saved|rejected|decode_error."""
    png_path = target_path.with_suffix(".png")

    if HAVE_PIL:
        try:
            with Image.open(io.BytesIO(raw_bytes)) as opened:
                width, height = opened.size
                if width < minimum_size or height < minimum_size:
                    return "rejected"
                if width * height < MIN_IMAGE_AREA:
                    return "rejected"
                aspect = max(width / max(height, 1), height / max(width, 1))
                if aspect > MAX_ASPECT_RATIO:
                    return "rejected"

                # Composite transparent RGBA/LA onto white so dark ink diagrams stay visible
                if opened.mode in ("RGBA", "LA") or (
                    opened.mode == "P" and "transparency" in opened.info
                ):
                    rgba = opened.convert("RGBA")
                    background = Image.new("RGB", rgba.size, (255, 255, 255))
                    background.paste(rgba, mask=rgba.getchannel("A"))
                    rgb_image = background
                else:
                    rgb_image = opened.convert("RGB")
        except Exception:
            return "decode_error"

        try:
            gray = rgb_image.convert("L")
            if compute_shannon_entropy(gray) < MIN_IMAGE_ENTROPY:
                return "rejected"
            if ImageStat.Stat(gray).stddev[0] < MIN_IMAGE_DEVIATION:
                return "rejected"

            pixel_digest = hashlib.sha256(rgb_image.tobytes()).hexdigest()
            if pixel_digest in image_digests:
                return "rejected"
            if corpus_state.image_doc_counts[pixel_digest] >= CORPUS_LOGO_DOCUMENTS:
                return "rejected"

            dhash = compute_difference_hash(gray)
            if any(hamming_distance(dhash, prev) <= 4 for prev in document_dhashes):
                return "rejected"
            if enable_dedup and any(
                hamming_distance(dhash, prev) <= 2 for prev in corpus_state.corpus_dhashes
            ):
                return "rejected"

            longest = max(rgb_image.size)
            if longest > MAX_IMAGE_EDGE:
                scale = MAX_IMAGE_EDGE / float(longest)
                new_size = (
                    max(1, int(round(rgb_image.width * scale))),
                    max(1, int(round(rgb_image.height * scale))),
                )
                lanczos = getattr(getattr(Image, "Resampling", Image), "LANCZOS", 1)
                rgb_image = rgb_image.resize(new_size, lanczos)

            rgb_image.save(png_path, format="PNG", optimize=True)
            document_dhashes.append(dhash)
            image_digests.add(pixel_digest)
            return "saved"
        except Exception:
            return "decode_error"

    if HAVE_PYMUPDF:
        try:
            pixmap = pdf_engine.Pixmap(raw_bytes)
            if (
                pixmap.width < minimum_size
                or pixmap.height < minimum_size
                or (pixmap.width * pixmap.height < MIN_IMAGE_AREA)
            ):
                return "rejected"
            if max(pixmap.width / max(pixmap.height, 1), pixmap.height / max(pixmap.width, 1)) > MAX_ASPECT_RATIO:
                return "rejected"
            if pixmap.n - pixmap.alpha >= 4:
                pixmap = pdf_engine.Pixmap(pdf_engine.csRGB, pixmap)
            pixmap.save(str(png_path))
            return "saved"
        except Exception:
            return "decode_error"

    return "decode_error"


def find_nearest_caption(
    fig_bbox: Tuple[float, float, float, float],
    text_candidates: Sequence[Tuple[Tuple[float, float, float, float], str]],
) -> str:
    """Finds the closest Figure/Diagram/Table caption near an image bbox."""
    best_dist = 85.0
    best_caption = ""
    fx0, fy0, fx1, fy1 = fig_bbox

    for tb, text in text_candidates:
        first_line = text.splitlines()[0].strip() if text else ""
        if not first_line or not CAPTION_PATTERN.match(first_line):
            continue
        horiz_overlap = min(fx1, tb[2]) - max(fx0, tb[0])
        if horiz_overlap < -40.0:
            continue
        if tb[1] >= fy1 - 10.0:
            dist = abs(tb[1] - fy1)
        elif tb[3] <= fy0 + 10.0:
            dist = abs(fy0 - tb[3])
        else:
            continue
        if dist < best_dist:
            best_dist = dist
            caption = " ".join(text.split())[:180]
            best_caption = caption.replace("[", "(").replace("]", ")")

    return best_caption


def cluster_vector_drawings(
    page: "pdf_engine.Page",
    excluded_bboxes: Sequence[Tuple[float, float, float, float]],
    text_bboxes: Sequence[Tuple[float, float, float, float]],
) -> List[Tuple[float, float, float, float]]:
    """Clusters vector paths on a page into candidate diagram/chart bounding boxes."""
    page_rect = page.rect
    page_width = max(float(page_rect.width), 1.0)
    page_height = max(float(page_rect.height), 1.0)
    page_area = page_width * page_height

    raw_rects: List[Tuple[float, float, float, float]] = []
    try:
        drawings = page.get_drawings()
    except Exception:
        return []

    for drawing in drawings:
        rect = drawing.get("rect")
        if rect is None:
            continue
        x0, y0, x1, y1 = float(rect.x0), float(rect.y0), float(rect.x1), float(rect.y1)
        width, height = x1 - x0, y1 - y0
        if width <= 0.5 and height <= 0.5:
            continue
        # Skip page backgrounds and header/footer rules
        if (width * height) > 0.82 * page_area:
            continue
        if y1 < 0.08 * page_height or y0 > 0.92 * page_height:
            continue
        candidate = (x0, y0, x1, y1)
        if any(rectangle_overlap_ratio(candidate, excluded) >= 0.5 for excluded in excluded_bboxes):
            continue
        raw_rects.append(candidate)

    if len(raw_rects) < 6:
        return []

    # O(N) spatial-grid union-find (half-margin 6pt guarantees touching cells within 12pt)
    half_margin = 6.0
    grid_step = 16.0
    parent = list(range(len(raw_rects)))

    def find_root(idx: int) -> int:
        while parent[idx] != idx:
            parent[idx] = parent[parent[idx]]
            idx = parent[idx]
        return idx

    def union_nodes(left_idx: int, right_idx: int) -> None:
        left_root, right_root = find_root(left_idx), find_root(right_idx)
        if left_root != right_root:
            parent[right_root] = left_root

    cell_owner: Dict[Tuple[int, int], int] = {}
    for idx, (x0, y0, x1, y1) in enumerate(raw_rects):
        gx0 = int(math.floor((x0 - half_margin) / grid_step))
        gx1 = int(math.floor((x1 + half_margin) / grid_step))
        gy0 = int(math.floor((y0 - half_margin) / grid_step))
        gy1 = int(math.floor((y1 + half_margin) / grid_step))
        for gx in range(gx0, gx1 + 1):
            for gy in range(gy0, gy1 + 1):
                cell = (gx, gy)
                owner = cell_owner.get(cell)
                if owner is None:
                    cell_owner[cell] = idx
                else:
                    union_nodes(owner, idx)

    groups: Dict[int, List[Tuple[float, float, float, float]]] = defaultdict(list)
    for idx, rect in enumerate(raw_rects):
        groups[find_root(idx)].append(rect)

    clusters: List[Tuple[float, float, float, float]] = []
    for rects in groups.values():
        if len(rects) < 6:
            continue
        cx0 = max(0.0, min(r[0] for r in rects) - 4.0)
        cy0 = max(0.0, min(r[1] for r in rects) - 4.0)
        cx1 = min(page_width, max(r[2] for r in rects) + 4.0)
        cy1 = min(page_height, max(r[3] for r in rects) + 4.0)
        cluster_width, cluster_height = cx1 - cx0, cy1 - cy0
        area = cluster_width * cluster_height
        if cluster_width < 80.0 or cluster_height < 60.0 or area > 0.80 * page_area:
            continue
        if max(cluster_width / max(cluster_height, 1.0), cluster_height / max(cluster_width, 1.0)) > 6.0:
            continue
        cluster_rect = (cx0, cy0, cx1, cy1)
        if any(rectangle_overlap_ratio(cluster_rect, excluded) >= 0.45 for excluded in excluded_bboxes):
            continue
        # Reject boxed prose callouts where text covers > 55% of the rectangle
        text_area = 0.0
        for text_box in text_bboxes:
            overlap = rectangle_overlap_ratio(text_box, cluster_rect)
            if overlap > 0.2:
                text_area += (text_box[2] - text_box[0]) * (text_box[3] - text_box[1]) * overlap
        if (text_area / max(area, 1.0)) > 0.55:
            continue
        clusters.append(cluster_rect)

    return clusters


def clean_prose_line(line: str) -> str:
    """Strips boilerplate, TOC leaders, and corrupted tokenless strings."""
    text = " ".join(normalize_unicode(line).split())
    if not text:
        return ""
    if PAGE_NUMBER_PATTERN.match(text):
        return ""
    if TOC_LEADER_PATTERN.search(text):
        return ""
    if WATERMARK_PATTERN.match(text):
        return ""
    if REPEATED_RUN_PATTERN.search(text):
        return ""
    tokens = text.split()
    kept_tokens = [
        tok for tok in tokens if len(tok) <= 45 or URL_TOKEN_PATTERN.match(tok)
    ]
    if not kept_tokens:
        return ""
    return " ".join(kept_tokens)


def extract_pdf_pymupdf(
    pdf_path: Path,
    stem: str,
    images_dir: Path,
    md_dir: Path,
    extract_images: bool,
    capture_vectors: bool,
    enable_dedup: bool,
    corpus_state: CorpusState,
) -> ExtractedDocument:
    """Layout-aware PDF extraction using PyMuPDF dict blocks, tables, and vector figures."""
    try:
        doc = pdf_engine.open(str(pdf_path))
    except Exception as exc:
        print(f"  [!] Failed to open {pdf_path.name}: {exc}", file=sys.stderr)
        return ExtractedDocument("pdf", 0, [], 0)

    total_pages = len(doc)
    dict_flags = getattr(pdf_engine, "TEXTFLAGS_DICT", 199) & ~getattr(
        pdf_engine, "TEXT_PRESERVE_IMAGES", 4
    )
    font_size_weights: Counter = Counter()
    chrome_counts: Counter = Counter()
    xref_page_map: Dict[int, Set[int]] = defaultdict(set)
    total_corrupt = 0
    total_raw = 0
    scanned_pages = 0

    try:
        # Pass 1: Stream pages to gather body font size, running headers/footers, and logo xrefs
        for page_idx in range(total_pages):
            try:
                page = doc[page_idx]
                page_h = max(float(page.rect.height), 1.0)
                pdict = page.get_text("dict", flags=dict_flags)

                page_seen_chrome: Set[Tuple[str, str]] = set()
                for block in pdict.get("blocks", []):
                    if block.get("type") != 0:
                        continue
                    bx0, by0, bx1, by1 = block["bbox"]
                    block_texts: List[str] = []
                    for line in block.get("lines", []):
                        for span in line.get("spans", []):
                            stext = span.get("text", "")
                            if not stext:
                                continue
                            corrupt, raw_len = count_corrupt_characters(stext)
                            total_corrupt += corrupt
                            total_raw += raw_len
                            block_texts.append(stext)
                            stripped_len = len(stext.strip())
                            if stripped_len > 0:
                                rounded_size = round(float(span.get("size", 10.0)) * 2.0) / 2.0
                                font_size_weights[rounded_size] += stripped_len
                    joined_block = " ".join(block_texts).strip()
                    if joined_block and total_pages >= 3:
                        if by1 <= 0.10 * page_h or by0 <= 0.08 * page_h:
                            page_seen_chrome.add(("top", normalize_template(joined_block)))
                        elif by0 >= 0.90 * page_h or by1 >= 0.92 * page_h:
                            page_seen_chrome.add(("bottom", normalize_template(joined_block)))

                for key in page_seen_chrome:
                    if key[1]:
                        chrome_counts[key] += 1

                if extract_images:
                    for img_info in page.get_images(full=True):
                        xref_page_map[img_info[0]].add(page_idx)
            except Exception:
                continue

        # Compute character-weighted median body font size
        body_size = 10.0
        if font_size_weights:
            weighted_total = sum(font_size_weights.values())
            running = 0
            for sz, weight in sorted(font_size_weights.items()):
                running += weight
                if running >= weighted_total / 2:
                    body_size = sz
                    break

        chrome_threshold = max(2, math.ceil(0.40 * total_pages)) if total_pages >= 3 else total_pages + 1
        repeated_chrome = {
            key for key, count in chrome_counts.items() if count >= chrome_threshold
        }
        repeated_xrefs = {
            xref for xref, pset in xref_page_map.items() if len(pset) >= DOCUMENT_LOGO_PAGES
        }

        all_blocks: List[Tuple[str, str]] = []
        saved_images: List[Path] = []
        image_count = 0
        doc_dhashes: List[int] = []
        doc_image_hashes: Set[str] = set()
        seen_xrefs: Set[int] = set()

        # Pass 2: Structured page reconstruction with tables, columns, figures, and captions
        for page_idx in range(total_pages):
            try:
                page = doc[page_idx]
                page_w = max(float(page.rect.width), 1.0)
                page_h = max(float(page.rect.height), 1.0)
                pdict = page.get_text("dict", flags=dict_flags)
                page_items: List[PageItem] = []
                table_bboxes: List[Tuple[float, float, float, float]] = []
                image_bboxes: List[Tuple[float, float, float, float]] = []
                code_bboxes: List[Tuple[float, float, float, float]] = []
                text_caption_pool: List[Tuple[Tuple[float, float, float, float], str]] = []
                prose_bboxes: List[Tuple[float, float, float, float]] = []

                # 1. Extract Markdown tables first so overlapping text blocks are not duplicated
                try:
                    found_tables = page.find_tables()
                    for table in getattr(found_tables, "tables", []):
                        tb = tuple(float(v) for v in table.bbox)
                        extracted_rows = table.extract()
                        if not is_valid_table(extracted_rows, tb, page_h):
                            continue
                        md_table = format_markdown_table(extracted_rows)
                        if md_table:
                            table_bboxes.append(tb)
                            page_items.append(PageItem(bbox=tb, kind="table", content=md_table))
                except Exception:
                    pass

                # 2. Extract and classify text blocks
                page_text_chars = 0
                for block in pdict.get("blocks", []):
                    if block.get("type") != 0:
                        continue
                    bbox = tuple(float(v) for v in block["bbox"])
                    if any(rectangle_overlap_ratio(bbox, tb) >= 0.50 for tb in table_bboxes):
                        continue

                    # Check running headers/footers & edge Roman page numbers
                    in_top = bbox[3] <= 0.10 * page_h or bbox[1] <= 0.08 * page_h
                    in_bottom = bbox[1] >= 0.90 * page_h or bbox[3] >= 0.92 * page_h
                    raw_block_text = " ".join(
                        span.get("text", "")
                        for line in block.get("lines", [])
                        for span in line.get("spans", [])
                    ).strip()
                    if not raw_block_text:
                        continue
                    tmpl = normalize_template(raw_block_text)
                    if in_top and ("top", tmpl) in repeated_chrome:
                        continue
                    if in_bottom and ("bottom", tmpl) in repeated_chrome:
                        continue
                    if (in_top or in_bottom) and (
                        PAGE_NUMBER_PATTERN.match(raw_block_text)
                        or ROMAN_PAGE_PATTERN.match(raw_block_text)
                    ):
                        continue

                    # Check page-level block corruption (> 15% bad chars in block -> skip)
                    blk_corrupt, blk_raw = count_corrupt_characters(raw_block_text)
                    if blk_raw > 0 and (blk_corrupt / blk_raw) > 0.15:
                        continue

                    line_records: List[Tuple[Tuple[float, float, float, float], str, float, bool, bool]] = []
                    for line in block.get("lines", []):
                        lbbox = tuple(float(v) for v in line["bbox"])
                        spans = line.get("spans", [])
                        raw_line = "".join(s.get("text", "") for s in spans)
                        norm_line = normalize_unicode(raw_line)
                        if not norm_line.strip():
                            continue

                        total_chars = 0
                        mono_chars = 0
                        bold_chars = 0
                        size_acc = 0.0
                        for span in spans:
                            st = span.get("text", "").strip()
                            if not st:
                                continue
                            cnt = len(st)
                            total_chars += cnt
                            sz = float(span.get("size", body_size))
                            fl = int(span.get("flags", 0))
                            fn = str(span.get("font", ""))
                            size_acc += sz * cnt
                            if is_monospace_span(fn, fl):
                                mono_chars += cnt
                            if (fl & 16) or ("bold" in fn.lower()):
                                bold_chars += cnt

                        avg_size = (size_acc / total_chars) if total_chars > 0 else body_size
                        is_mono = (total_chars > 0) and (mono_chars / total_chars >= 0.70)
                        is_bold = (total_chars > 0) and (bold_chars / total_chars >= 0.75)
                        line_records.append((lbbox, norm_line, avg_size, is_mono, is_bold))

                    if not line_records:
                        continue

                    clean_joined = "\n".join(rec[1].strip() for rec in line_records)
                    page_text_chars += len(clean_joined)

                    # Classify block: code, heading, display math, or paragraph prose
                    mono_line_ratio = sum(1 for rec in line_records if rec[3]) / len(line_records)
                    if mono_line_ratio >= 0.75:
                        code_lines = [rec[1].rstrip() for rec in line_records if rec[1].strip()]
                        if code_lines:
                            code_bboxes.append(bbox)
                            page_items.append(
                                PageItem(
                                    bbox=bbox,
                                    kind="code",
                                    content="```\n" + "\n".join(code_lines) + "\n```",
                                    raw_lines=code_lines,
                                )
                            )
                        continue

                    dominant_size = statistics.median(rec[2] for rec in line_records)
                    all_bold = all(rec[4] for rec in line_records)
                    heading_text = " ".join(clean_prose_line(rec[1]) for rec in line_records).strip()
                    is_heading = (
                        bool(heading_text)
                        and len(line_records) <= 2
                        and len(heading_text) <= 140
                        and not CAPTION_PATTERN.match(heading_text)
                        and (
                            dominant_size >= body_size * 1.15
                            or (
                                all_bold
                                and len(heading_text) <= 80
                                and not heading_text.endswith((".", ",", ";", ":"))
                            )
                        )
                    )
                    if is_heading:
                        level = "##" if dominant_size >= body_size * 1.35 else "###"
                        page_items.append(
                            PageItem(bbox=bbox, kind="heading", content=f"{level} {heading_text}")
                        )
                        continue

                    # Reconstruct paragraphs with tight per-paragraph bboxes (splits side-by-side captions)
                    prose_bboxes.append(bbox)
                    paragraphs: List[Tuple[str, str, Tuple[float, float, float, float]]] = []
                    current_para = ""
                    para_bbox: Optional[Tuple[float, float, float, float]] = None
                    prev_bbox: Optional[Tuple[float, float, float, float]] = None
                    prev_height = max(body_size, 10.0)

                    for lbbox, norm_line, avg_size, _, _ in line_records:
                        cleaned = clean_prose_line(norm_line)
                        if not cleaned:
                            continue

                        if is_math_display(cleaned) and len(line_records) <= 3:
                            if current_para and para_bbox is not None:
                                paragraphs.append(("prose", current_para, para_bbox))
                                current_para = ""
                                para_bbox = None
                            paragraphs.append(("math", f"$$\n{cleaned}\n$$", lbbox))
                            prev_bbox = lbbox
                            continue

                        line_h = max(lbbox[3] - lbbox[1], avg_size, 8.0)
                        starts_list = bool(LIST_ITEM_PATTERN.match(cleaned))
                        if prev_bbox is not None and current_para and para_bbox is not None:
                            v_gap = lbbox[1] - prev_bbox[3]
                            indent_shift = lbbox[0] - prev_bbox[0]
                            same_row_split = abs(lbbox[1] - prev_bbox[1]) < 0.5 * prev_height and (
                                lbbox[0] - prev_bbox[2] > 18.0
                            )
                            is_para_break = (
                                starts_list
                                or same_row_split
                                or v_gap > 1.3 * prev_height
                                or (
                                    indent_shift > 10.0
                                    and current_para.endswith((".", "!", "?", ":", '"', ")"))
                                )
                            )
                            if is_para_break:
                                paragraphs.append(("prose", current_para, para_bbox))
                                current_para = cleaned
                                para_bbox = lbbox
                            else:
                                current_para = dehyphenate_join(current_para, cleaned)
                                para_bbox = (
                                    min(para_bbox[0], lbbox[0]),
                                    min(para_bbox[1], lbbox[1]),
                                    max(para_bbox[2], lbbox[2]),
                                    max(para_bbox[3], lbbox[3]),
                                )
                        else:
                            current_para = cleaned
                            para_bbox = lbbox
                        prev_bbox = lbbox
                        prev_height = line_h

                    if current_para and para_bbox is not None:
                        paragraphs.append(("prose", current_para, para_bbox))

                    for kind, para_text, tight_bbox in paragraphs:
                        if kind == "prose":
                            text_caption_pool.append((tight_bbox, para_text))
                        page_items.append(PageItem(bbox=tight_bbox, kind=kind, content=para_text))

                # 3. Extract & filter raster images with interleaved Markdown captions
                has_large_raster = False
                if extract_images:
                    fig_seq = 0
                    for img_info in page.get_images(full=True):
                        xref = img_info[0]
                        smask = img_info[1]
                        if xref in repeated_xrefs or xref in seen_xrefs:
                            continue
                        rects = page.get_image_rects(xref)
                        if rects:
                            rect = rects[0]
                            ibbox = (float(rect.x0), float(rect.y0), float(rect.x1), float(rect.y1))
                            if ((ibbox[2] - ibbox[0]) * (ibbox[3] - ibbox[1])) >= 0.60 * page_w * page_h:
                                has_large_raster = True
                        else:
                            ibbox = (0.0, page_h * 0.5, page_w, page_h * 0.5)

                        seen_xrefs.add(xref)
                        fig_seq += 1
                        png_dest = (images_dir / f"{stem}_p{page_idx + 1}_fig{fig_seq}").with_suffix(".png")
                        save_status = "decode_error"

                        # Prefer Pixmap composition when PDF specifies an alpha SMask
                        if smask <= 0:
                            try:
                                base_img = doc.extract_image(xref)
                                raw_bytes = base_img.get("image")
                                if raw_bytes:
                                    save_status = save_filtered_png(
                                        raw_bytes,
                                        png_dest,
                                        doc_dhashes,
                                        corpus_state,
                                        doc_image_hashes,
                                        enable_dedup=enable_dedup,
                                    )
                            except Exception:
                                save_status = "decode_error"

                        if save_status == "decode_error":
                            try:
                                pix = pdf_engine.Pixmap(doc, xref)
                                if pix.n - pix.alpha >= 4:
                                    pix = pdf_engine.Pixmap(pdf_engine.csRGB, pix)
                                if smask > 0:
                                    try:
                                        mask_pix = pdf_engine.Pixmap(doc, smask)
                                        if pix.alpha == 0 and mask_pix.width == pix.width and mask_pix.height == pix.height:
                                            pix = pdf_engine.Pixmap(pix, mask_pix)
                                        mask_pix = None
                                    except Exception:
                                        pass
                                png_bytes = pix.tobytes("png")
                                pix = None
                                save_status = save_filtered_png(
                                    png_bytes,
                                    png_dest,
                                    doc_dhashes,
                                    corpus_state,
                                    doc_image_hashes,
                                    enable_dedup=enable_dedup,
                                )
                            except Exception:
                                save_status = "rejected"

                        if save_status == "saved":
                            image_count += 1
                            saved_images.append(png_dest)
                            image_bboxes.append(ibbox)
                            caption = find_nearest_caption(ibbox, text_caption_pool)
                            alt_text = caption or f"Figure {fig_seq} from {stem} (page {page_idx + 1})"
                            rel_link = os.path.relpath(png_dest, md_dir).replace("\\", "/")
                            page_items.append(
                                PageItem(bbox=ibbox, kind="image", content=f"![{alt_text}]({rel_link})")
                            )

                    # 4. Tier 2: Capture vector diagrams/charts via page.get_drawings()
                    if capture_vectors:
                        excluded = table_bboxes + image_bboxes + code_bboxes
                        vec_clusters = cluster_vector_drawings(page, excluded, prose_bboxes)
                        for vec_idx, vbbox in enumerate(vec_clusters, start=1):
                            png_dest = (
                                images_dir / f"{stem}_p{page_idx + 1}_vec{vec_idx}"
                            ).with_suffix(".png")
                            try:
                                clip_rect = pdf_engine.Rect(*vbbox)
                                pix = page.get_pixmap(clip=clip_rect, dpi=VECTOR_DPI)
                                png_bytes = pix.tobytes("png")
                                pix = None
                                if (
                                    save_filtered_png(
                                        png_bytes,
                                        png_dest,
                                        doc_dhashes,
                                        corpus_state,
                                        doc_image_hashes,
                                        enable_dedup=enable_dedup,
                                    )
                                    == "saved"
                                ):
                                    image_count += 1
                                    saved_images.append(png_dest)
                                    caption = find_nearest_caption(vbbox, text_caption_pool)
                                    alt_text = caption or f"Diagram {vec_idx} from {stem} (page {page_idx + 1})"
                                    rel_link = os.path.relpath(png_dest, md_dir).replace("\\", "/")
                                    page_items.append(
                                        PageItem(
                                            bbox=vbbox,
                                            kind="image",
                                            content=f"![{alt_text}]({rel_link})",
                                        )
                                    )
                            except Exception:
                                continue
                else:
                    if page_text_chars < 50:
                        for img_info in page.get_images(full=False):
                            rects = page.get_image_rects(img_info[0])
                            if rects:
                                rect = rects[0]
                                if ((rect.width * rect.height) >= 0.60 * page_w * page_h):
                                    has_large_raster = True
                                    break

                if page_text_chars < 50 and has_large_raster:
                    scanned_pages += 1

                ordered_items = sort_reading_order(page_items, page_w)
                for item in ordered_items:
                    # Merge mid-sentence breaks across column/page boundaries
                    if (
                        item.kind == "prose"
                        and all_blocks
                        and all_blocks[-1][0] == "prose"
                        and not LIST_ITEM_PATTERN.match(item.content)
                        and not all_blocks[-1][1].endswith((".", "!", "?", ":", '"', ")", "]"))
                        and item.content
                        and item.content[0].islower()
                    ):
                        prev_kind, prev_text = all_blocks[-1]
                        all_blocks[-1] = (prev_kind, dehyphenate_join(prev_text, item.content))
                    else:
                        all_blocks.append((item.kind, item.content))
            except Exception:
                continue
    finally:
        doc.close()

    return ExtractedDocument(
        source_type="pdf",
        page_count=total_pages,
        blocks=all_blocks,
        image_count=image_count,
        scanned_pages=scanned_pages,
        corrupt_characters=total_corrupt,
        raw_characters=total_raw,
        saved_images=saved_images,
        image_dhashes=doc_dhashes,
        image_digests=doc_image_hashes,
    )


def extract_pdf_pypdf(
    pdf_path: Path,
    stem: str,
    images_dir: Path,
    md_dir: Path,
    extract_images: bool,
    enable_dedup: bool,
    corpus_state: CorpusState,
) -> ExtractedDocument:
    """Fallback PDF extraction using pypdf with paragraph merging and image filtering."""
    try:
        reader = pypdf.PdfReader(str(pdf_path))
    except Exception as exc:
        print(f"  [!] Failed to read PDF {pdf_path.name}: {exc}", file=sys.stderr)
        return ExtractedDocument("pdf", 0, [], 0)

    blocks: List[Tuple[str, str]] = []
    saved_images: List[Path] = []
    image_count = 0
    doc_dhashes: List[int] = []
    doc_image_hashes: Set[str] = set()
    total_corrupt = 0
    total_raw = 0

    for page_idx, page in enumerate(reader.pages):
        try:
            page_text = page.extract_text() or ""
            corrupt_count, raw_count = count_corrupt_characters(page_text)
            total_corrupt += corrupt_count
            total_raw += raw_count
            blocks.extend(reconstruct_plain_blocks(page_text))

            if extract_images and hasattr(page, "images"):
                for img_idx, img_file in enumerate(page.images, start=1):
                    png_dest = (images_dir / f"{stem}_p{page_idx + 1}_fig{img_idx}").with_suffix(".png")
                    if (
                        save_filtered_png(
                            img_file.data,
                            png_dest,
                            doc_dhashes,
                            corpus_state,
                            doc_image_hashes,
                            enable_dedup=enable_dedup,
                        )
                        == "saved"
                    ):
                        image_count += 1
                        saved_images.append(png_dest)
                        rel_link = os.path.relpath(png_dest, md_dir).replace("\\", "/")
                        blocks.append(("image", f"![Figure {img_idx} from {stem} (page {page_idx + 1})]({rel_link})"))
        except Exception:
            continue

    return ExtractedDocument(
        source_type="pdf",
        page_count=len(reader.pages),
        blocks=blocks,
        image_count=image_count,
        corrupt_characters=total_corrupt,
        raw_characters=total_raw,
        saved_images=saved_images,
        image_dhashes=doc_dhashes,
        image_digests=doc_image_hashes,
    )


def extract_docx_document(
    docx_path: Path,
    stem: str,
    images_dir: Path,
    md_dir: Path,
    extract_images: bool,
    enable_dedup: bool,
    corpus_state: CorpusState,
) -> ExtractedDocument:
    """Structure-aware DOCX extractor preserving headings, lists, tables, and inline images."""
    blocks: List[Tuple[str, str]] = []
    saved_images: List[Path] = []
    image_count = 0
    doc_dhashes: List[int] = []
    doc_image_hashes: Set[str] = set()
    total_corrupt = 0
    total_raw = 0

    w_ns = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    a_ns = "http://schemas.openxmlformats.org/drawingml/2006/main"
    r_ns = "http://schemas.openxmlformats.org/officeDocument/2006/relationships"
    ns = {"w": w_ns, "a": a_ns, "r": r_ns}

    try:
        with zipfile.ZipFile(docx_path, "r") as zf:
            rel_targets: Dict[str, str] = {}
            if "word/_rels/document.xml.rels" in zf.namelist():
                rels_root = ET.fromstring(zf.read("word/_rels/document.xml.rels"))
                for rel in rels_root:
                    rid = rel.attrib.get("Id")
                    target = rel.attrib.get("Target", "")
                    if rid and target:
                        if target.startswith("/"):
                            normalized = target.lstrip("/")
                        else:
                            normalized = posixpath.normpath(f"word/{target}")
                        rel_targets[rid] = normalized

            saved_media: Dict[str, Path] = {}
            if "word/document.xml" in zf.namelist():
                root = ET.fromstring(zf.read("word/document.xml"))
                body = root.find("w:body", ns)
                if body is not None:
                    code_buffer: List[str] = []

                    def flush_code() -> None:
                        if code_buffer:
                            blocks.append(("code", "```\n" + "\n".join(code_buffer) + "\n```"))
                            code_buffer.clear()

                    for child in body:
                        tag = child.tag.split("}")[-1]
                        if tag == "tbl":
                            flush_code()
                            table_rows: List[List[str]] = []
                            for tr in child.findall("w:tr", ns):
                                row_cells: List[str] = []
                                for tc in tr.findall("w:tc", ns):
                                    cell_parts = [t.text for t in tc.findall(".//w:t", ns) if t.text]
                                    row_cells.append("".join(cell_parts))
                                table_rows.append(row_cells)
                            md_table = format_markdown_table(table_rows)
                            if md_table:
                                blocks.append(("table", md_table))
                            continue

                        if tag != "p":
                            continue

                        # Interleave embedded images anchored in this paragraph
                        if extract_images:
                            for blip in child.findall(".//a:blip", ns):
                                rid = blip.attrib.get(f"{{{r_ns}}}embed")
                                media_path = rel_targets.get(rid or "")
                                if media_path and media_path in zf.namelist():
                                    if media_path not in saved_media:
                                        png_dest = (
                                            images_dir / f"{stem}_fig{len(saved_media) + 1}"
                                        ).with_suffix(".png")
                                        if (
                                            save_filtered_png(
                                                zf.read(media_path),
                                                png_dest,
                                                doc_dhashes,
                                                corpus_state,
                                                doc_image_hashes,
                                                enable_dedup=enable_dedup,
                                            )
                                            == "saved"
                                        ):
                                            saved_media[media_path] = png_dest
                                            saved_images.append(png_dest)
                                            image_count += 1
                                    if media_path in saved_media:
                                        flush_code()
                                        rel_link = os.path.relpath(
                                            saved_media[media_path], md_dir
                                        ).replace("\\", "/")
                                        blocks.append(
                                            ("image", f"![Figure from {stem}]({rel_link})")
                                        )

                        raw_parts = [t.text for t in child.findall(".//w:t", ns) if t.text]
                        raw_text = "".join(raw_parts)
                        corrupt_count, raw_count = count_corrupt_characters(raw_text)
                        total_corrupt += corrupt_count
                        total_raw += raw_count
                        if not raw_text.strip():
                            flush_code()
                            continue

                        style_node = child.find("./w:pPr/w:pStyle", ns)
                        style_val = (
                            style_node.attrib.get(f"{{{w_ns}}}val", "")
                            if style_node is not None
                            else ""
                        )
                        is_list = child.find("./w:pPr/w:numPr", ns) is not None

                        # Check for monospace runs (code paragraphs)
                        named_fonts = [
                            (
                                rf.attrib.get(f"{{{w_ns}}}ascii")
                                or rf.attrib.get(f"{{{w_ns}}}hAnsi")
                                or ""
                            )
                            for rf in child.findall(".//w:rFonts", ns)
                        ]
                        named_fonts = [font for font in named_fonts if font]
                        is_code_para = bool(named_fonts) and all(
                            any(h in font.lower() for h in MONOSPACE_HINTS) for font in named_fonts
                        )
                        if is_code_para or "code" in style_val.lower():
                            code_buffer.append(normalize_unicode(raw_text).rstrip())
                            continue

                        flush_code()
                        cleaned = clean_prose_line(raw_text)
                        if not cleaned:
                            continue

                        heading_match = re.match(r"Heading(\d)", style_val, re.IGNORECASE)
                        if style_val.lower() == "title":
                            blocks.append(("heading", f"# {cleaned}"))
                        elif heading_match:
                            level = min(6, max(1, int(heading_match.group(1))))
                            blocks.append(("heading", f"{'#' * level} {cleaned}"))
                        elif is_list:
                            blocks.append(("prose", f"- {cleaned}"))
                        elif is_math_display(cleaned):
                            blocks.append(("math", f"$$\n{cleaned}\n$$"))
                        else:
                            blocks.append(("prose", cleaned))

                    flush_code()
    except Exception as exc:
        print(f"  [!] Failed to read DOCX {docx_path.name}: {exc}", file=sys.stderr)
        return ExtractedDocument("docx", 0, [], 0)

    return ExtractedDocument(
        source_type="docx",
        page_count=1,
        blocks=blocks,
        image_count=image_count,
        corrupt_characters=total_corrupt,
        raw_characters=total_raw,
        saved_images=saved_images,
        image_dhashes=doc_dhashes,
        image_digests=doc_image_hashes,
    )


def reconstruct_plain_blocks(raw_text: str) -> List[Tuple[str, str]]:
    """Reconstructs structured Markdown blocks from plain TXT/MD input."""
    normalized = normalize_unicode(raw_text)
    blocks: List[Tuple[str, str]] = []
    in_fence = False
    fence_lines: List[str] = []
    para_lines: List[str] = []

    def flush_para() -> None:
        if not para_lines:
            return
        merged = para_lines[0]
        for nxt in para_lines[1:]:
            merged = dehyphenate_join(merged, nxt)
        para_lines.clear()
        if is_math_display(merged):
            blocks.append(("math", f"$$\n{merged}\n$$"))
        else:
            blocks.append(("prose", merged))

    for raw_line in normalized.splitlines():
        stripped = raw_line.strip()
        if stripped.startswith("```"):
            flush_para()
            if in_fence:
                fence_lines.append(stripped)
                blocks.append(("code", "\n".join(fence_lines)))
                fence_lines = []
                in_fence = False
            else:
                in_fence = True
                fence_lines = [stripped]
            continue

        if in_fence:
            fence_lines.append(raw_line.rstrip())
            continue

        if not stripped:
            flush_para()
            continue

        if stripped.startswith("#"):
            flush_para()
            blocks.append(("heading", stripped))
            continue

        if stripped.startswith("|") and stripped.endswith("|"):
            flush_para()
            if blocks and blocks[-1][0] == "table":
                blocks[-1] = ("table", blocks[-1][1] + "\n" + stripped)
            else:
                blocks.append(("table", stripped))
            continue

        cleaned = clean_prose_line(raw_line)
        if not cleaned:
            continue

        if LIST_ITEM_PATTERN.match(cleaned):
            flush_para()
            blocks.append(("prose", cleaned))
            continue

        para_lines.append(cleaned)

    flush_para()
    if fence_lines:
        fence_lines.append("```")
        blocks.append(("code", "\n".join(fence_lines)))

    return blocks


def strip_references(blocks: List[Tuple[str, str]]) -> List[Tuple[str, str]]:
    """Strips trailing References/Bibliography wall in the final 35% of a document."""
    total = len(blocks)
    if total < 6:
        return blocks

    cutoff_start = int(total * 0.65)
    for idx in range(cutoff_start, total):
        kind, text = blocks[idx]
        if kind == "heading" and REFERENCE_HEADING_PATTERN.match(text.strip()):
            trailing = [b[1] for b in blocks[idx + 1 : idx + 15] if b[0] == "prose"]
            if not trailing:
                return blocks[:idx]
            citation_hits = sum(1 for item in trailing if CITATION_LINE_PATTERN.search(item))
            if (citation_hits / len(trailing)) >= 0.40:
                return blocks[:idx]
    return blocks


def deduplicate_paragraphs(
    blocks: List[Tuple[str, str]], corpus_state: CorpusState, enable_dedup: bool
) -> Tuple[List[Tuple[str, str]], Set[str]]:
    """Removes repeated boilerplate paragraphs without mutating corpus_state."""
    if not enable_dedup:
        return blocks, set()

    seen_in_doc: Set[str] = set()
    kept: List[Tuple[str, str]] = []
    doc_hashes: Set[str] = set()

    for kind, text in blocks:
        if kind != "prose" or len(text) < 40:
            kept.append((kind, text))
            continue
        norm = " ".join(text.lower().split())
        digest = hashlib.sha256(norm.encode("utf-8")).hexdigest()
        if digest in seen_in_doc:
            continue
        if corpus_state.paragraph_counts[digest] >= 3:
            continue
        seen_in_doc.add(digest)
        doc_hashes.add(digest)
        kept.append((kind, text))

    return kept, doc_hashes


def evaluate_quality(doc: ExtractedDocument, blocks: Sequence[Tuple[str, str]]) -> Tuple[float, str]:
    """Computes FineWeb/Gopher-style quality score in [0.0, 1.0] and rejection reason."""
    prose_texts = [text for kind, text in blocks if kind in ("prose", "heading")]
    all_texts = [text for kind, text in blocks if kind != "image"]
    if not all_texts:
        return 0.0, "empty"

    score = 1.0
    reasons: List[str] = []

    if doc.raw_characters > 0:
        corrupt_ratio = doc.corrupt_characters / float(doc.raw_characters)
        if corrupt_ratio > 0.01:
            score -= min(0.65, corrupt_ratio * 15.0)
            reasons.append(f"corrupt_encoding={corrupt_ratio:.2%}")

    prose_joined = "\n".join(prose_texts)
    words = prose_joined.split()
    if words:
        avg_word_len = sum(len(w) for w in words) / float(len(words))
        if avg_word_len < 2.8 or avg_word_len > 14.0:
            score -= 0.35
            reasons.append(f"avg_word_len={avg_word_len:.1f}")

        alpha_ratio = sum(1 for w in words if any(ch.isalpha() for ch in w)) / float(len(words))
        if alpha_ratio < 0.60:
            score -= 0.40
            reasons.append(f"alpha_ratio={alpha_ratio:.2f}")

    # Gopher line repetition gate
    lines = [
        " ".join(ln.lower().split())
        for text in all_texts
        for ln in text.splitlines()
        if len(ln.strip()) >= 15
    ]
    if len(lines) >= 10:
        unique_lines = len(set(lines))
        dup_line_ratio = 1.0 - (unique_lines / float(len(lines)))
        if dup_line_ratio > 0.30:
            score -= 0.45
            reasons.append(f"dup_lines={dup_line_ratio:.2%}")

    # Top-10 character 10-gram repetition check (catches stuck OCR loops)
    compact = re.sub(r"\s+", " ", prose_joined.lower()).strip()
    if len(compact) >= 500:
        sample = compact[:20000]
        ngrams = Counter(sample[i : i + 10] for i in range(0, len(sample) - 9, 2))
        total_ngrams = sum(ngrams.values())
        if total_ngrams > 0:
            top_share = sum(cnt for _, cnt in ngrams.most_common(10)) / float(total_ngrams)
            if top_share > 0.30:
                score -= 0.35
                reasons.append(f"ngram_loop={top_share:.2%}")

    clamped = max(0.0, min(1.0, round(score, 3)))
    return clamped, ", ".join(reasons)


def build_minhash(text: str) -> Optional[Tuple[int, ...]]:
    """Computes a 64-permutation MinHash signature over word 5-shingles."""
    words = re.findall(r"[a-z0-9]+", text.lower())
    if len(words) < 10:
        return None
    shingle_hashes: Set[int] = set()
    limit = min(len(words) - 4, 8000)
    for idx in range(limit):
        shingle = " ".join(words[idx : idx + 5]).encode("utf-8")
        hval = int.from_bytes(hashlib.blake2b(shingle, digest_size=8).digest(), "little") % MINHASH_PRIME
        shingle_hashes.add(hval)

    if not shingle_hashes:
        return None

    signature: List[int] = []
    for coeff_a, coeff_b in MINHASH_COEFFICIENTS:
        min_val = min((coeff_a * h + coeff_b) % MINHASH_PRIME for h in shingle_hashes)
        signature.append(min_val)
    return tuple(signature)


def check_duplicate_document(
    text: str, corpus_state: CorpusState
) -> Tuple[bool, str, str, Optional[Tuple[int, ...]]]:
    """Checks exact SHA-256 and MinHash LSH near-duplicate similarity without mutating state."""
    norm = " ".join(text.lower().split())
    exact_hash = hashlib.sha256(norm.encode("utf-8")).hexdigest()
    if exact_hash in corpus_state.exact_hashes:
        return True, "exact_duplicate", exact_hash, None

    signature = build_minhash(norm)
    if signature is not None:
        candidates: Set[int] = set()
        for band in range(MINHASH_BANDS):
            start = band * MINHASH_ROWS
            key = (band, signature[start : start + MINHASH_ROWS])
            candidates.update(corpus_state.lsh_buckets.get(key, []))

        for candidate_index in candidates:
            other = corpus_state.minhash_signatures[candidate_index]
            matches = sum(1 for a, b in zip(signature, other) if a == b)
            if (matches / float(MINHASH_SIZE)) >= NEAR_DUPLICATE_THRESHOLD:
                return True, f"near_duplicate(jaccard={matches / MINHASH_SIZE:.2f})", exact_hash, None

    return False, "", exact_hash, signature


def commit_corpus_state(
    corpus_state: CorpusState,
    exact_hash: Optional[str],
    signature: Optional[Tuple[int, ...]],
    paragraph_hashes: Set[str],
    image_dhashes: Sequence[int],
    image_digests: Set[str],
) -> None:
    """Atomically commits accepted document signatures and image hashes to corpus_state."""
    if exact_hash:
        corpus_state.exact_hashes.add(exact_hash)
    if signature is not None:
        sig_idx = len(corpus_state.minhash_signatures)
        corpus_state.minhash_signatures.append(signature)
        for band in range(MINHASH_BANDS):
            start = band * MINHASH_ROWS
            key = (band, signature[start : start + MINHASH_ROWS])
            corpus_state.lsh_buckets[key].append(sig_idx)
    for digest in paragraph_hashes:
        corpus_state.paragraph_counts[digest] += 1
    for dhash in image_dhashes:
        corpus_state.corpus_dhashes.add(dhash)
    for image_digest in image_digests:
        corpus_state.image_doc_counts[image_digest] += 1


def cleanup_saved_images(saved_images: Sequence[Path]) -> None:
    """Deletes PNG files written for a rejected or deduplicated document."""
    for image_path in saved_images:
        try:
            image_path.unlink(missing_ok=True)
        except OSError:
            pass


def build_frontmatter(
    doc_id: str,
    source_type: str,
    pages: int,
    words: int,
    images: int,
    quality_score: float,
) -> str:
    """Formats standardized YAML frontmatter header for Markdown corpus files."""
    safe_id = doc_id.replace('"', "'")
    return (
        "---\n"
        f'doc_id: "{safe_id}"\n'
        f'source_type: "{source_type}"\n'
        f"pages: {pages}\n"
        f"words: {words}\n"
        f"images: {images}\n"
        f"quality_score: {quality_score:.2f}\n"
        "---\n\n"
    )


def write_markdown_outputs(
    stem: str,
    doc: ExtractedDocument,
    blocks: Sequence[Tuple[str, str]],
    quality_score: float,
    output_base: Path,
    text_dir: Path,
    symbols_dir: Path,
    flat: bool,
    frontmatter: bool,
    symbols_view: bool,
) -> Tuple[int, int]:
    """Writes context-preserving primary .md and optional secondary symbols .md."""
    if not blocks:
        return 0, 0

    body = "\n\n".join(content for _, content in blocks if content.strip()).strip()
    if not body:
        return 0, 0

    word_count = sum(
        len(content.split()) for kind, content in blocks if kind != "image"
    )
    prefix = (
        build_frontmatter(
            doc_id=stem,
            source_type=doc.source_type,
            pages=doc.page_count,
            words=word_count,
            images=doc.image_count,
            quality_score=quality_score,
        )
        if frontmatter
        else ""
    )

    md_path = ((output_base / stem) if flat else (text_dir / stem)).with_suffix(".md")
    md_path.write_text(prefix + body + "\n", encoding="utf-8")

    symbol_words = 0
    if symbols_view:
        symbol_entries: List[str] = []
        for idx, (kind, content) in enumerate(blocks):
            if kind not in ("math", "code", "table"):
                continue
            context_line = ""
            if idx > 0 and blocks[idx - 1][0] in ("prose", "heading"):
                prev = blocks[idx - 1][1].splitlines()[-1].strip()
                context_line = f"> {prev[:200]}\n\n"
            symbol_entries.append(f"{context_line}{content}")
            symbol_words += len(content.split())

        if symbol_entries:
            symbols_dir.mkdir(parents=True, exist_ok=True)
            sym_path = (symbols_dir / f"{stem}_symbols").with_suffix(".md")
            sym_content = f"# {stem} — Equations, Tables & Code\n\n" + "\n\n".join(symbol_entries) + "\n"
            sym_path.write_text(sym_content, encoding="utf-8")

    return word_count, symbol_words


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
    capture_vectors: bool,
    flat: bool,
    frontmatter: bool,
    symbols_view: bool,
    keep_references: bool,
    min_quality: float,
    enable_dedup: bool,
    corpus_state: CorpusState,
) -> Tuple[str, int, int, int]:
    """Processes one file and returns (status, words, symbol_words, image_count)."""
    ext = source_path.suffix.lower()
    md_dir = output_base if flat else text_dir

    if ext in IMAGE_EXTENSIONS:
        if not extract_images:
            return "skipped", 0, 0, 0
        print(f"[..] Processing IMG:  {source_path.name}")
        try:
            raw_bytes = source_path.read_bytes()
            png_dest = (images_dir / stem).with_suffix(".png")
            doc_dhashes: List[int] = []
            doc_hashes: Set[str] = set()
            if (
                save_filtered_png(
                    raw_bytes,
                    png_dest,
                    doc_dhashes,
                    corpus_state,
                    doc_hashes,
                    enable_dedup=enable_dedup,
                )
                == "saved"
            ):
                commit_corpus_state(corpus_state, None, None, set(), doc_dhashes, doc_hashes)
                print(f"  [OK] Converted -> {png_dest.name} (.png)")
                return "ok", 0, 0, 1
            print(f"  [SKIP] Low-signal or duplicate image: {source_path.name}")
        except Exception as exc:
            print(f"  [!] Failed to convert image {source_path.name}: {exc}", file=sys.stderr)
        return "skipped", 0, 0, 0

    if ext == ".pdf":
        print(f"[..] Processing PDF:  {source_path.name}")
        if HAVE_PYMUPDF:
            doc = extract_pdf_pymupdf(
                source_path,
                stem,
                images_dir,
                md_dir,
                extract_images,
                capture_vectors,
                enable_dedup,
                corpus_state,
            )
        elif HAVE_PYPDF:
            doc = extract_pdf_pypdf(
                source_path,
                stem,
                images_dir,
                md_dir,
                extract_images,
                enable_dedup,
                corpus_state,
            )
        else:
            print(f"  [WARN] No PDF engine (pymupdf/pypdf) installed for {source_path.name}", file=sys.stderr)
            return "skipped", 0, 0, 0
    elif ext == ".docx":
        print(f"[..] Processing DOCX: {source_path.name}")
        doc = extract_docx_document(
            source_path,
            stem,
            images_dir,
            md_dir,
            extract_images,
            enable_dedup,
            corpus_state,
        )
    elif ext in TEXT_EXTENSIONS:
        print(f"[..] Processing TEXT: {source_path.name}")
        try:
            raw_text = source_path.read_text(encoding="utf-8", errors="replace")
            corrupt, raw_len = count_corrupt_characters(raw_text)
            doc = ExtractedDocument(
                source_type=ext.lstrip("."),
                page_count=1,
                blocks=reconstruct_plain_blocks(raw_text),
                image_count=0,
                corrupt_characters=corrupt,
                raw_characters=raw_len,
            )
        except Exception as exc:
            print(f"  [!] Failed to read {source_path.name}: {exc}", file=sys.stderr)
            return "skipped", 0, 0, 0
    else:
        return "skipped", 0, 0, 0

    blocks = doc.blocks
    if not keep_references:
        blocks = strip_references(blocks)

    if doc.scanned_pages > 0:
        print(f"  [WARN] Detected {doc.scanned_pages} scanned/image-only page(s) needing OCR")

    quality_score, reason = evaluate_quality(doc, blocks)
    if quality_score < min_quality:
        cleanup_saved_images(doc.saved_images)
        print(f"  [REJECT] Low quality ({quality_score:.2f} < {min_quality:.2f}): {reason or 'noisy'}")
        return "rejected", 0, 0, 0

    exact_hash: Optional[str] = None
    signature: Optional[Tuple[int, ...]] = None
    if enable_dedup:
        dedup_text = "\n".join(text for kind, text in blocks if kind != "image")
        is_dup, dup_reason, exact_hash, signature = check_duplicate_document(dedup_text, corpus_state)
        if is_dup:
            cleanup_saved_images(doc.saved_images)
            print(f"  [DEDUP] Skipped {source_path.name}: {dup_reason}")
            return "dedup", 0, 0, 0

    blocks, paragraph_hashes = deduplicate_paragraphs(blocks, corpus_state, enable_dedup=enable_dedup)
    if not any(kind != "image" and text.strip() for kind, text in blocks):
        cleanup_saved_images(doc.saved_images)
        return "skipped", 0, 0, 0

    words_total, words_symbols = write_markdown_outputs(
        stem=stem,
        doc=doc,
        blocks=blocks,
        quality_score=quality_score,
        output_base=output_base,
        text_dir=text_dir,
        symbols_dir=symbols_dir,
        flat=flat,
        frontmatter=frontmatter,
        symbols_view=symbols_view,
    )

    if words_total == 0 and doc.image_count == 0:
        cleanup_saved_images(doc.saved_images)
        return "skipped", 0, 0, 0

    commit_corpus_state(
        corpus_state=corpus_state,
        exact_hash=exact_hash,
        signature=signature,
        paragraph_hashes=paragraph_hashes,
        image_dhashes=doc.image_dhashes,
        image_digests=doc.image_digests,
    )

    print(
        f"  [OK] Quality {quality_score:.2f} | {words_total:,} words (.md) | "
        f"{doc.image_count} figures (.png)"
        + (f" | {words_symbols:,} symbol words" if symbols_view else "")
    )
    return "ok", words_total, words_symbols, doc.image_count


def resolve_smart_path(raw_path: str) -> Path:
    """Resolves paths with fallback support for core/ workspace layouts."""
    path_obj = Path(raw_path)
    if not path_obj.exists():
        if raw_path.startswith("core/") and Path(raw_path[5:]).exists():
            path_obj = Path(raw_path[5:])
        elif (Path("core") / raw_path).exists():
            path_obj = Path("core") / raw_path
    return path_obj.resolve()


def discover_files(input_path: Path, excluded_dirs: Set[Path], recursive: bool) -> List[Path]:
    """Discovers supported files while skipping output and archive directories."""
    supported = {".pdf", ".docx"} | TEXT_EXTENSIONS | IMAGE_EXTENSIONS
    discovered: List[Path] = []

    if not recursive:
        for item in sorted(input_path.iterdir()):
            if item.is_file() and item.suffix.lower() in supported:
                discovered.append(item)
        return discovered

    for root, dirs, files in os.walk(input_path):
        root_path = Path(root).resolve()
        dirs[:] = sorted(
            d
            for d in dirs
            if not d.startswith(".")
            and d != "archive"
            and (root_path / d).resolve() not in excluded_dirs
        )
        for fname in sorted(files):
            fpath = root_path / fname
            if fpath.suffix.lower() in supported:
                discovered.append(fpath)

    return discovered


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Axiom Unified Layout-Aware Multimodal Corpus Extractor (.md & .png only)"
    )
    parser.add_argument("pos_input_dir", nargs="?", default=None, help="Optional positional input directory")
    parser.add_argument("pos_output_dir", nargs="?", default=None, help="Optional positional output directory")
    parser.add_argument("--input-dir", type=str, default=None, help="Input directory (default: asset_sample)")
    parser.add_argument("--output-dir", type=str, default=None, help="Output directory (default: asset_sample)")
    parser.add_argument(
        "--extract-images",
        dest="extract_images",
        action="store_true",
        default=True,
        help="Extract & convert filtered images as PNG (default: enabled)",
    )
    parser.add_argument(
        "--no-images",
        dest="extract_images",
        action="store_false",
        help="Disable image and vector figure extraction",
    )
    parser.add_argument(
        "--no-vector-figures",
        dest="capture_vectors",
        action="store_false",
        default=True,
        help="Disable page.get_drawings() vector diagram rasterization",
    )
    parser.add_argument(
        "--flat",
        action="store_true",
        default=False,
        help="Write <stem>.md directly in output root instead of <output_dir>/text/",
    )
    parser.add_argument(
        "--symbols-view",
        action="store_true",
        default=False,
        help="Also emit secondary symbols/<stem>_symbols.md view with surrounding context",
    )
    parser.add_argument(
        "--frontmatter",
        action="store_true",
        default=False,
        help="Prepend YAML metadata frontmatter to each generated .md file",
    )
    parser.add_argument(
        "--min-quality",
        type=float,
        default=0.45,
        help="Reject documents scoring below this quality threshold [0.0-1.0] (default: 0.45)",
    )
    parser.add_argument(
        "--keep-references",
        action="store_true",
        default=False,
        help="Keep trailing References/Bibliography sections instead of stripping them",
    )
    parser.add_argument(
        "--no-dedup",
        dest="enable_dedup",
        action="store_false",
        default=True,
        help="Disable exact/MinHash document and boilerplate paragraph deduplication",
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
    if args.symbols_view:
        symbols_dir.mkdir(parents=True, exist_ok=True)
    if args.extract_images:
        images_dir.mkdir(parents=True, exist_ok=True)

    excluded_dirs: Set[Path] = {text_dir.resolve(), symbols_dir.resolve(), images_dir.resolve()}
    if output_base != input_path:
        excluded_dirs.add(output_base.resolve())

    files = discover_files(input_path, excluded_dirs, recursive=not args.no_recursive)
    if not files:
        print(f"[!] No supported files found in {input_path}")
        sys.exit(0)

    pdf_engine = (
        "PyMuPDF (layout dict + tables + vectors)"
        if HAVE_PYMUPDF
        else ("pypdf (fallback)" if HAVE_PYPDF else "None")
    )
    img_engine = (
        "Pillow + PyMuPDF (entropy/dHash/vector)"
        if (HAVE_PIL and HAVE_PYMUPDF)
        else ("Pillow" if HAVE_PIL else ("PyMuPDF" if HAVE_PYMUPDF else "None"))
    )

    print(f"[+] Found {len(files)} file(s) in {input_path}")
    print(f"  -> PDF Engine:          {pdf_engine}")
    print(f"  -> PNG Vision Engine:   {img_engine} (images={args.extract_images}, vectors={args.capture_vectors})")
    print(f"  -> Quality Gate:        min_quality={args.min_quality:.2f} | dedup={args.enable_dedup} | frontmatter={args.frontmatter}")
    print(f"  -> Output Format:       Strictly .md and .png (context-preserving)")
    print(f"  -> Markdown dest:       {output_base if args.flat else text_dir}")
    if args.symbols_view:
        print(f"  -> Symbols view dest:   {symbols_dir}")
    if args.extract_images:
        print(f"  -> Images destination:  {images_dir}")
    print("-" * 60)

    processed_count = 0
    rejected_count = 0
    dedup_count = 0
    total_words = 0
    total_symbols = 0
    total_images = 0
    used_stems: Set[str] = set()
    corpus_state = CorpusState()

    for fpath in files:
        stem = unique_stem(fpath, input_path, used_stems)
        status, words, s_words, imgs = process_file(
            source_path=fpath,
            stem=stem,
            output_base=output_base,
            text_dir=text_dir,
            symbols_dir=symbols_dir,
            images_dir=images_dir,
            extract_images=args.extract_images,
            capture_vectors=args.capture_vectors,
            flat=args.flat,
            frontmatter=args.frontmatter,
            symbols_view=args.symbols_view,
            keep_references=args.keep_references,
            min_quality=args.min_quality,
            enable_dedup=args.enable_dedup,
            corpus_state=corpus_state,
        )
        if status == "ok":
            processed_count += 1
            total_words += words
            total_symbols += s_words
            total_images += imgs
        elif status == "rejected":
            rejected_count += 1
        elif status == "dedup":
            dedup_count += 1

    print("=" * 60)
    print("[OK] Ingestion Pipeline Complete (.md & .png only):")
    print(f"     Files Written:             {processed_count:,}")
    print(f"     Files Rejected (Quality):  {rejected_count:,}")
    print(f"     Files Skipped (Dedup):     {dedup_count:,}")
    print(f"     Total Corpus Words (.md):  {total_words:,}")
    if args.symbols_view:
        print(f"     Secondary Symbol Words:    {total_symbols:,}")
    print(f"     Total High-Signal PNGs:    {total_images:,}")


if __name__ == "__main__":
    main()
