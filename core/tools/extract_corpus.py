#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Axiom-Proprietary
"""
Copyright (c) 2026 Omid Teimory. All Rights Reserved.

PROPRIETARY SOURCE-AVAILABLE SOFTWARE — NOT OPEN SOURCE.
Permission to view, clone, or compile this file is granted strictly on the
condition precedent of full compliance with the root ./LICENSE file.
Copying snippets, modifying, creating derivative works, redistributing,
commercial/SaaS use, and AI/ML training (reserved under EU Directive
2019/790 Art. 4(3) and § 42h(6) Austrian UrhG) are strictly prohibited.
Removal or alteration of this notice violates 17 U.S.C. § 1202 and § 90c UrhG.

Project Axiom - Clean Corpus Text Extractor (Phase 4: Fixes G1, G2)
Extracts documents (PDF, DOCX, TXT, MD) to strictly normalized, verified UTF-8 text files.
Expands ligatures, strips control characters, and normalizes typography to prevent <UNK> flood.
"""

import sys
import os
import re
import unicodedata
from pathlib import Path

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

def clean_text(text: str) -> str:
    """Normalizes Unicode text to clean UTF-8 ASCII-compatible tokens."""
    # Expand composite ligatures
    text = unicodedata.normalize("NFKD", text)
    for ligature, replacement in LIGATURE_MAP.items():
        text = text.replace(ligature, replacement)
    
    # Strip non-printable control characters except standard whitespace
    cleaned_chars = []
    for char in text:
        cp = ord(char)
        if cp in (9, 10, 13) or (32 <= cp < 127) or (cp >= 160):
            cleaned_chars.append(char)
        else:
            cleaned_chars.append(" ")
            
    # Collapse irregular whitespace runs
    cleaned = "".join(cleaned_chars)
    cleaned = re.sub(r"[ \t]+", " ", cleaned)
    cleaned = re.sub(r"\n{3,}", "\n\n", cleaned)
    return cleaned.strip()

def extract_pdf(pdf_path: Path) -> str:
    """Extracts text from PDF via pypdf or PyPDF2 if installed, or fallback."""
    try:
        import pypdf
        reader = pypdf.PdfReader(str(pdf_path))
        pages_text = []
        for page in reader.pages:
            t = page.extract_text()
            if t:
                pages_text.append(t)
        return clean_text("\n\n".join(pages_text))
    except ImportError:
        pass

    try:
        import fitz  # PyMuPDF
        doc = fitz.open(str(pdf_path))
        pages_text = [page.get_text() for page in doc]
        return clean_text("\n\n".join(pages_text))
    except ImportError:
        pass

    sys.stderr.write(f"[WARN] No Python PDF library (pypdf/pymupdf) found for {pdf_path.name}\n")
    return ""

def process_file(source_path: Path, output_dir: Path) -> bool:
    ext = source_path.suffix.lower()
    text = ""
    
    if ext == ".pdf":
        text = extract_pdf(source_path)
    elif ext in (".txt", ".text", ".md", ".markdown"):
        try:
            raw = source_path.read_text(encoding="utf-8", errors="replace")
            text = clean_text(raw)
        except Exception as e:
            sys.stderr.write(f"[ERR] Failed to read {source_path}: {e}\n")
            return False
    else:
        return False

    if not text:
        return False

    out_file = output_dir / f"{source_path.stem}.md"
    out_file.write_text(text, encoding="utf-8")
    print(f"[OK] Extracted {source_path.name} -> {out_file.name} ({len(text)} chars)")
    return True

def main():
    if len(sys.argv) < 3:
        print("Usage: extract_corpus.py <input_dir> <output_dir>")
        sys.exit(1)
        
    in_dir = Path(sys.argv[1])
    out_dir = Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)
    
    count = 0
    for root, _, files in os.walk(in_dir):
        for f in files:
            p = Path(root) / f
            if process_file(p, out_dir):
                count += 1
                
    print(f"\n[+] Successfully extracted and normalized {count} corpus files.")

if __name__ == "__main__":
    main()

