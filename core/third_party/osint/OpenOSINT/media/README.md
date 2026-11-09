# media/

Demo assets for OpenOSINT social content — GIFs for GitHub embeds, MP4s for Threads.

## Prerequisites

Install [VHS](https://github.com/charmbracelet/vhs) by Charmbracelet:

```bash
brew install vhs          # macOS / Linux (Homebrew)
# or
go install github.com/charmbracelet/vhs@latest
```

## Generate all assets

Run from the repo root:

```bash
./media/generate.sh
```

This renders every `.tape` in `media/tapes/` and writes `.gif` + `.mp4` pairs
into `media/output/` (created automatically).

## Generate a single tape

```bash
vhs media/tapes/email-investigation.tape
vhs media/tapes/username-trace.tape
vhs media/tapes/ip-intel.tape
vhs media/tapes/mcp-showcase.tape
vhs media/tapes/install-quickstart.tape
```

## Tapes

| File | Scenario | Duration |
|------|----------|----------|
| `email-investigation.tape` | AI calls `search_email` → `search_breach` → `generate_dorks`, compiles report | ~20s |
| `username-trace.tape` | `search_username` scans 300+ platforms, results populate | ~15s |
| `ip-intel.tape` | `search_ip` + `search_whois` — structured IP intel with abuse flags | ~12s |
| `mcp-showcase.tape` | `claude mcp list` confirms registration, autonomous one-liner investigation | ~25s |
| `install-quickstart.tape` | `pip install` → set API key → launch REPL | ~10s |

## Record the web UI demo (Playwright)

Captures an automated browser demo of the web interface — no manual interaction needed.

```bash
pip install "openosint[web]"
playwright install chromium
python media/record_demo.py
```

**Requires** `ffmpeg` for MP4/GIF conversion (`brew install ffmpeg`).  
Without ffmpeg, the raw `.webm` and screenshots are still saved.

**Outputs:**

| File | Description |
|------|-------------|
| `media/output/web-demo.mp4` | Full demo video (H.264) |
| `media/output/web-demo.gif` | Animated GIF (10 fps, palette-optimised) |
| `media/screenshots/01-landing-dark.png` | Landing page — dark theme |
| `media/screenshots/02-chat-response.png` | Chat with AI response |
| `media/screenshots/03-landing-light.png` | Landing page — light theme |
| `media/screenshots/04-settings.png` | Settings modal |

## Output

`media/output/` is **git-ignored** — assets are generated locally, not committed.

Each tape produces two files:

```
media/output/email-investigation.gif
media/output/email-investigation.mp4
```

## Posting to Threads

**Use `.mp4` for Threads** — MP4 renders natively in the feed with autoplay and
performs significantly better than GIF (smaller file, better quality, no dithering).

**Use `.gif` for GitHub** — embed in README or docs:

```markdown
![OpenOSINT email investigation demo](media/output/email-investigation.gif)
```

The `mcp-showcase.tape` is the strongest differentiator for Threads: it shows
Claude Code calling OpenOSINT tools autonomously from a single natural-language
prompt, which is the core value proposition.
