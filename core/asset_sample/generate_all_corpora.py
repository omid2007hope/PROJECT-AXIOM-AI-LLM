#!/usr/bin/env python3
"""
Project Axiom — Master Corpus Generator
File: core/asset_sample/generate_all_corpora.py

Coordinates generation across all four core taxonomy pillars:
1. Foundation: Language & Lexicon (generate_foundation_corpus.py)
2. Web Development: HTML, CSS, JavaScript, Frameworks (generate_web_corpus.py)
3. Systems & Software Engineering: C, C++, Kernel, CUDA, Rust, Enterprise (generate_systems_corpus.py)
4. Quantitative & Scientific Domains: Math, Physics, Data Science (generate_quantitative_corpus.py)
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def run_corpus_generator(script_path: Path, repeat_multiplier: int) -> int:
    """Invokes an individual corpus generator script as a subprocess."""
    execution_command = [
        sys.executable,
        str(script_path),
        "--multiplier",
        str(repeat_multiplier),
    ]

    execution_result = subprocess.run(
        execution_command,
        capture_output=True,
        text=True,
        check=True
    )
    print(execution_result.stdout.strip())
    return execution_result.returncode


def concatenate_training_corpora(corpora_paths: list[Path], unified_output: Path) -> int:
    """Concatenates all individual corpus files into a unified master training dataset."""
    total_written_bytes = 0
    unified_output.parent.mkdir(parents=True, exist_ok=True)

    with open(unified_output, "w", encoding="utf-8") as target_file:
        for corpus_path in corpora_paths:
            if corpus_path.exists():
                with open(corpus_path, "r", encoding="utf-8") as source_file:
                    content_chunk = source_file.read()
                    target_file.write(content_chunk)
                    if not content_chunk.endswith("\n"):
                        target_file.write("\n")
                    total_written_bytes += len(content_chunk.encode("utf-8"))

    return total_written_bytes


def main() -> None:
    """Coordinates generation of all domain corpora across the Axiom roadmap."""
    argument_parser = argparse.ArgumentParser(
        description="Project Axiom Master Taxonomy Corpus Generator"
    )
    argument_parser.add_argument(
        "--multiplier", "-m",
        type=int,
        default=2,
        help="Repeat multiplier to apply across all domain corpus generators."
    )
    argument_parser.add_argument(
        "--unify", "-u",
        action="store_true",
        default=True,
        help="Concatenate all generated corpora into axiom_master_corpus.txt."
    )
    parsed_arguments = argument_parser.parse_args()

    base_directory = Path(__file__).resolve().parent

    generator_scripts = [
        ("Foundation", base_directory / "generate_foundation_corpus.py", base_directory / "foundation_training_corpus.txt"),
        ("Web Development", base_directory / "generate_web_corpus.py", base_directory / "web_training_corpus.txt"),
        ("Systems & Software", base_directory / "generate_systems_corpus.py", base_directory / "systems_training_corpus.txt"),
        ("Quantitative & Science", base_directory / "generate_quantitative_corpus.py", base_directory / "quantitative_training_corpus.txt"),
    ]

    print("======================================================================")
    print(" Project Axiom — Full Taxonomy Corpus Generation Pipeline")
    print("======================================================================")

    generated_corpus_files: list[Path] = []
    for domain_name, script_path, output_corpus in generator_scripts:
        print(f"\n[Domain] Running {domain_name} generator ({script_path.name})...")
        run_corpus_generator(script_path, parsed_arguments.multiplier)
        generated_corpus_files.append(output_corpus)

    if parsed_arguments.unify:
        master_output_file = base_directory / "axiom_master_corpus.txt"
        total_master_bytes = concatenate_training_corpora(generated_corpus_files, master_output_file)
        print("\n----------------------------------------------------------------------")
        print(f"[Master Corpus] Created unified training dataset: {master_output_file}")
        print(f"[Master Corpus] Total size: {total_master_bytes / 1024:.1f} KB")

    print("\n[Axiom Ingestion] Ready for training:")
    print("  .\\core\\app.exe --train")
    print("  .\\core\\app.exe --bulk")
    print("======================================================================")


if __name__ == "__main__":
    main()

