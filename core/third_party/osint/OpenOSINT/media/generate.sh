#!/usr/bin/env bash
# generate.sh — render all OpenOSINT demo tapes into media/output/
# Usage: ./media/generate.sh

set -euo pipefail

# ── Resolve repo root regardless of where the script is called from ────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TAPES_DIR="$SCRIPT_DIR/tapes"
OUTPUT_DIR="$SCRIPT_DIR/output"

cd "$REPO_ROOT"

# ── Dependency check ───────────────────────────────────────────────────────
if ! command -v vhs &>/dev/null; then
  echo ""
  echo "  Error: 'vhs' is not installed."
  echo ""
  echo "  Install it with:"
  echo "    brew install vhs          # macOS / Linux (Homebrew)"
  echo "    go install github.com/charmbracelet/vhs@latest  # Go"
  echo ""
  exit 1
fi

# ── Ensure output directory exists ────────────────────────────────────────
mkdir -p "$OUTPUT_DIR"

# ── Render each tape ───────────────────────────────────────────────────────
generated=()
failed=()

for tape in "$TAPES_DIR"/*.tape; do
  name="$(basename "$tape" .tape)"
  echo "  Rendering  $name ..."
  if vhs "$tape"; then
    generated+=("$name")
  else
    echo "  FAILED     $name"
    failed+=("$name")
  fi
done

# ── Summary ────────────────────────────────────────────────────────────────
echo ""
echo "  ────────────────────────────────────────────────────────"
echo "  Done."
echo ""

if [[ ${#generated[@]} -gt 0 ]]; then
  echo "  Generated (${#generated[@]}):"
  for name in "${generated[@]}"; do
    echo "    ✓  $OUTPUT_DIR/${name}.gif"
    echo "       $OUTPUT_DIR/${name}.mp4"
  done
fi

if [[ ${#failed[@]} -gt 0 ]]; then
  echo ""
  echo "  Failed (${#failed[@]}):"
  for name in "${failed[@]}"; do
    echo "    ✗  $name"
  done
  echo ""
  exit 1
fi

echo ""
