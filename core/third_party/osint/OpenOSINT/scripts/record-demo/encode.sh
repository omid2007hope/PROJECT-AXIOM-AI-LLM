#!/usr/bin/env bash
# encode.sh — ffmpeg + gifski encoding pipeline for OpenOSINT demo recording.
#
# Reads:   scripts/record-demo/out/raw.webm
# Writes:  docs/assets/demo-web-graph.mp4
#          docs/assets/demo-web-graph.gif   (< 10 MB; auto-reduces fps/width if over)
#          docs/assets/demo-web-graph-poster.png  (MP4 fallback; record.mjs also writes it)
#
# Amendment 5: frames are extracted at FULL captured resolution (1440x860).
# gifski is the ONLY downscaler — no ffmpeg scale filter on the frame extraction pass.
# This preserves sharp text through the entire pipeline.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

OUT_DIR="$SCRIPT_DIR/out"
FRAMES_DIR="$OUT_DIR/frames"
ASSETS_DIR="$ROOT/docs/assets"

WEBM="$OUT_DIR/raw.webm"
MP4="$ASSETS_DIR/demo-web-graph.mp4"
POSTER="$ASSETS_DIR/demo-web-graph-poster.png"
GIF="$ASSETS_DIR/demo-web-graph.gif"

MAX_GIF_BYTES=$((10 * 1024 * 1024))   # 10 MB — GitHub inline GIF limit

# ---------------------------------------------------------------------------
# Dependency checks
# ---------------------------------------------------------------------------
for tool in ffmpeg gifski; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "ERROR: $tool not found."
    echo "  ffmpeg:  brew install ffmpeg"
    echo "  gifski:  brew install gifski"
    exit 1
  }
done

[ -f "$WEBM" ] || {
  echo "ERROR: $WEBM not found — run record.mjs first (make demo)."
  exit 1
}

mkdir -p "$ASSETS_DIR" "$FRAMES_DIR"

# ---------------------------------------------------------------------------
# Source dimensions (for reporting only)
# ---------------------------------------------------------------------------
SRC_DIMS=$(ffprobe -v error -select_streams v:0 \
  -show_entries stream=width,height -of csv=p=0 "$WEBM" 2>/dev/null || echo "unknown")
echo "[*] Source: $WEBM  (${SRC_DIMS}px)"

# ---------------------------------------------------------------------------
# MP4 — H.264, high quality, web-optimised faststart
# ---------------------------------------------------------------------------
echo "[*] Encoding MP4..."
ffmpeg -y -i "$WEBM" \
  -c:v libx264 -preset slow -crf 18 \
  -pix_fmt yuv420p \
  -movflags +faststart \
  "$MP4" 2>&1 | grep -E "^(video|frame|Output|error)" || true
MP4_SIZE=$(stat -f%z "$MP4" 2>/dev/null || stat -c%s "$MP4")
echo "[+] MP4:  $MP4  ($(( MP4_SIZE / 1024 )) KB)"

# ---------------------------------------------------------------------------
# Poster frame — from MP4 at 5s; fallback to 1s for short recordings
# ---------------------------------------------------------------------------
DURATION=$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$MP4" 2>/dev/null || echo 0)
POSTER_SS=5
if awk "BEGIN { exit !($DURATION < 6) }" 2>/dev/null; then
  POSTER_SS=1
fi
echo "[*] Extracting poster at ${POSTER_SS}s..."
ffmpeg -y -ss "$POSTER_SS" -i "$MP4" -vframes 1 "$POSTER" 2>/dev/null || true
if [ -f "$POSTER" ]; then
  POSTER_SIZE=$(stat -f%z "$POSTER" 2>/dev/null || stat -c%s "$POSTER")
  echo "[+] Poster: $POSTER  ($(( POSTER_SIZE / 1024 )) KB)"
else
  echo "[!] Poster extraction failed — check record.mjs screenshot output"
fi

# ---------------------------------------------------------------------------
# Frame extraction at FULL resolution — NO scale filter.
# gifski is the only downscaler; a double resize would blur text.
# ---------------------------------------------------------------------------
echo "[*] Extracting frames at native ${SRC_DIMS}px (fps=15, no scale)..."
rm -f "$FRAMES_DIR"/frame_*.png
ffmpeg -y -i "$WEBM" -vf "fps=15" "$FRAMES_DIR/frame_%04d.png" 2>&1 | tail -1
FRAME_COUNT=$(find "$FRAMES_DIR" -name 'frame_*.png' | wc -l | tr -d ' ')
echo "[+] Frames: $FRAME_COUNT PNGs at native resolution"

# ---------------------------------------------------------------------------
# gifski encode with automatic size-control fallback.
#
# Fallback order (each attempt prints its size before deciding to proceed):
#   1. 15fps / 1440px / q90  — primary
#   2. 12fps / 1440px / q80  — reduce fps
#   3. 12fps / 1200px / q75  — reduce fps + width
#   4. 10fps / 1000px / q65  — last resort
#   5. abort with clear error and final size report
# ---------------------------------------------------------------------------
encode_gif() {
  local fps="$1" width="$2" quality="$3"
  echo "[*] gifski: fps=${fps}  width=${width}px  quality=${quality}..."
  gifski --fps "$fps" --width "$width" --quality "$quality" \
    -o "$GIF" "$FRAMES_DIR"/frame_*.png
  local gif_bytes
  gif_bytes=$(stat -f%z "$GIF" 2>/dev/null || stat -c%s "$GIF")
  local gif_mb
  gif_mb=$(awk "BEGIN {printf \"%.2f\", $gif_bytes/1048576}")
  echo "[i] GIF size: ${gif_mb} MB (${gif_bytes} bytes)"
  [ "$gif_bytes" -le "$MAX_GIF_BYTES" ]
}

FINAL_LABEL="15fps / 1440px / q90"
if   encode_gif 15 1440 90; then :
elif encode_gif 12 1440 80; then FINAL_LABEL="12fps / 1440px / q80"; echo "[!] Fallback 1 applied"
elif encode_gif 12 1200 75; then FINAL_LABEL="12fps / 1200px / q75"; echo "[!] Fallback 2 applied"
elif encode_gif 10 1000 65; then FINAL_LABEL="10fps / 1000px / q65"; echo "[!] Fallback 3 applied"
else
  GIF_BYTES=$(stat -f%z "$GIF" 2>/dev/null || stat -c%s "$GIF")
  GIF_MB=$(awk "BEGIN {printf \"%.2f\", $GIF_BYTES/1048576}")
  echo ""
  echo "ERROR: GIF is ${GIF_MB} MB after all fallbacks — exceeds 10 MB GitHub limit."
  echo "  Options: trim the recording length, reduce NODE_THRESHOLD_FINAL in record.mjs,"
  echo "  or lower the hold time at the end of the recording."
  exit 1
fi

# ---------------------------------------------------------------------------
# Final report
# ---------------------------------------------------------------------------
MP4_SIZE=$(stat -f%z "$MP4"    2>/dev/null || stat -c%s "$MP4")
GIF_SIZE=$(stat -f%z "$GIF"    2>/dev/null || stat -c%s "$GIF")
PST_SIZE=$([ -f "$POSTER" ] && (stat -f%z "$POSTER" 2>/dev/null || stat -c%s "$POSTER") || echo 0)

echo ""
echo "======================================"
echo "  Demo artifacts -> docs/assets/"
echo "--------------------------------------"
printf "  MP4    %6s KB\n" "$(( MP4_SIZE / 1024 ))"
printf "  GIF    %6s KB   [%s]\n" "$(( GIF_SIZE / 1024 ))" "$FINAL_LABEL"
printf "  Poster %6s KB\n" "$(( PST_SIZE / 1024 ))"
echo "======================================"
echo "  git add docs/assets/demo-web-graph.* && git commit to publish."
echo "  Raw frames in $FRAMES_DIR are gitignored."
