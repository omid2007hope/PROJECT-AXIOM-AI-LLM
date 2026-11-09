#!/usr/bin/env python3
"""
Record an automated demo video and GIF of the OpenOSINT web UI using Playwright.

Conversation state is injected directly into Alpine.js reactive data in
animated stages — no HTTP/SSE calls needed during recording.

Usage:
    pip install "openosint[web]"
    playwright install chromium
    python media/record_demo.py

Outputs:
    media/output/web-demo.mp4          H.264, faststart
    media/output/web-demo.gif          1280px, 12fps, palette-optimised
    media/output/web-demo-small.gif    800px,   8fps, docs-embed size
    media/screenshots/01-landing.png
    media/screenshots/02-input.png
    media/screenshots/03-investigation.png
    media/screenshots/04-light-theme.png
"""

from __future__ import annotations

import shutil
import subprocess
import sys
import time
from pathlib import Path

PORT = 8181
BASE_URL = f"http://localhost:{PORT}"
REPO_ROOT = Path(__file__).parent.parent
OUT_DIR = REPO_ROOT / "media" / "output"
SS_DIR = REPO_ROOT / "media" / "screenshots"

OUT_DIR.mkdir(parents=True, exist_ok=True)
SS_DIR.mkdir(parents=True, exist_ok=True)

# ---------------------------------------------------------------------------
# Alpine.js conversation stages — injected directly into the reactive store
# ---------------------------------------------------------------------------

_EMAIL = "demo@example.com"

# Tool output strings (identical to what the real demo endpoint sends)
_EMAIL_TOOL_OUTPUT = (
    "[+] Spotify       https://open.spotify.com/user/demo\n"
    "[+] GitHub        https://github.com/demo\n"
    "[+] Gravatar      https://gravatar.com/demo\n"
    "[+] WordPress     https://wordpress.com/demo\n"
    "[*] Holehe scan complete — 4 accounts found"
)
_BREACH_TOOL_OUTPUT = (
    "[!] LinkedIn (2016-05-17) — Passwords, Email addresses\n"
    "[!] Adobe (2013-10-04) — Passwords, Email addresses, Usernames\n"
    "[*] 2 breach(es) found via HaveIBeenPwned"
)
_SUMMARY = (
    "## Summary\n\n"
    "Target **demo@example.com** has accounts on **4 platforms** and appears "
    "in **2 known data breaches** (LinkedIn 2016, Adobe 2013). "
    "Credential rotation strongly advised."
)

# Each call to _set_state(page, JS_EXPR) injects one animation frame
def _set_state(page, js: str) -> None:
    page.evaluate(f"""
        () => {{
            const data = Alpine.$data(
                document.querySelector('[x-data="openosint"]')
            );
            if (!data) return;
            {js}
        }}
    """)


def log(msg: str) -> None:
    print(f"[record_demo] {msg}", flush=True)


def start_server() -> subprocess.Popen:
    log(f"Starting OpenOSINT web server on port {PORT}...")
    proc = subprocess.Popen(
        ["openosint", "web", "--no-browser", "--port", str(PORT)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    time.sleep(3)
    if proc.poll() is not None:
        sys.exit(
            "[record_demo] ERROR: web server exited immediately — "
            "is 'openosint' installed with all web dependencies?"
        )
    log("Server started.")
    return proc


def convert_video(webm_path: Path) -> None:
    if not shutil.which("ffmpeg"):
        log("ffmpeg not found — skipping MP4/GIF conversion.")
        log("Install ffmpeg: brew install ffmpeg")
        log(f"Raw video saved at: {webm_path}")
        return

    mp4_path = OUT_DIR / "web-demo.mp4"
    gif_path = OUT_DIR / "web-demo.gif"
    gif_small = OUT_DIR / "web-demo-small.gif"

    log("Converting to MP4 (H.264, faststart)...")
    r = subprocess.run(
        [
            "ffmpeg", "-y", "-i", str(webm_path),
            "-vcodec", "libx264", "-crf", "23", "-preset", "medium",
            "-movflags", "+faststart", str(mp4_path),
        ],
        capture_output=True,
    )
    if r.returncode == 0:
        log(f"MP4 saved: {mp4_path} ({mp4_path.stat().st_size / 1_048_576:.1f} MB)")
    else:
        log(f"MP4 conversion failed:\n{r.stderr.decode()[-400:]}")

    log("Converting to GIF (1280px, 12fps)...")
    r = subprocess.run(
        [
            "ffmpeg", "-y", "-i", str(webm_path),
            "-vf",
            (
                "fps=12,scale=1280:-1:flags=lanczos,"
                "split[s0][s1];[s0]palettegen=max_colors=128[p];[s1][p]paletteuse=dither=bayer"
            ),
            "-loop", "0", str(gif_path),
        ],
        capture_output=True,
    )
    if r.returncode == 0:
        log(f"GIF saved: {gif_path} ({gif_path.stat().st_size / 1_048_576:.1f} MB)")
    else:
        log(f"GIF conversion failed:\n{r.stderr.decode()[-400:]}")

    log("Converting to small GIF (800px, 8fps)...")
    r = subprocess.run(
        [
            "ffmpeg", "-y", "-i", str(webm_path),
            "-vf",
            (
                "fps=8,scale=800:-1:flags=lanczos,"
                "split[s0][s1];[s0]palettegen=max_colors=64[p];[s1][p]paletteuse=dither=bayer"
            ),
            "-loop", "0", str(gif_small),
        ],
        capture_output=True,
    )
    if r.returncode == 0:
        log(f"Small GIF saved: {gif_small} ({gif_small.stat().st_size / 1_048_576:.1f} MB)")
    else:
        log(f"Small GIF conversion failed:\n{r.stderr.decode()[-400:]}")

    webm_path.unlink(missing_ok=True)
    log("Source .webm deleted.")


def _js_str(s: str) -> str:
    """Escape a Python string for safe embedding in a JS single-quoted string."""
    return s.replace("\\", "\\\\").replace("'", "\\'").replace("\n", "\\n")


def _inject_investigation(page) -> None:
    """Animate the email investigation in 4 stages via direct Alpine.js state injection."""
    email = _EMAIL
    email_js = email.replace("@", "\\u0040")   # safe in JS strings
    intro_content = _js_str(f"Investigating **{email}**...\n\n")
    email_out_js = _js_str(_EMAIL_TOOL_OUTPUT)
    breach_out_js = _js_str(_BREACH_TOOL_OUTPUT)
    summary_js = _js_str(_SUMMARY)

    # Stage 1 — user message sent, assistant thinking
    log("  Stage 1/4: thinking...")
    page.evaluate("""
        () => {
            const data = Alpine.$data(document.querySelector('[x-data="openosint"]'));
            if (!data) return;
            const now = new Date().toLocaleTimeString([], {hour:'2-digit', minute:'2-digit'});
            data.aiBackend = 'claude';
            data.messages = [
                { role: 'user', content: 'Investigate demo@example.com', time: now },
                { role: 'assistant', parts: [] }
            ];
            data.isStreaming = true;
            data.input = '';
        }
    """)
    time.sleep(1.5)

    # Stage 2 — intro text + search_email running
    log("  Stage 2/4: search_email running...")
    page.evaluate(f"""
        () => {{
            const data = Alpine.$data(document.querySelector('[x-data="openosint"]'));
            if (!data) return;
            const ai = data.messages[1];
            ai.parts = [
                {{ type: 'text', content: '{intro_content}', streaming: false }},
                {{ type: 'tool', tool: 'search_email', input: '{email_js}',
                   status: 'running', output: '', elapsed: null, collapsed: false }}
            ];
        }}
    """)
    time.sleep(1.5)

    # Stage 3 — search_email done, search_breach running
    log("  Stage 3/4: search_breach running...")
    page.evaluate(f"""
        () => {{
            const data = Alpine.$data(document.querySelector('[x-data="openosint"]'));
            if (!data) return;
            const ai = data.messages[1];
            ai.parts = [
                {{ type: 'text', content: '{intro_content}', streaming: false }},
                {{ type: 'tool', tool: 'search_email', input: '{email_js}',
                   status: 'done', output: '{email_out_js}', elapsed: 1.4, collapsed: false }},
                {{ type: 'tool', tool: 'search_breach', input: '{email_js}',
                   status: 'running', output: '', elapsed: null, collapsed: false }}
            ];
        }}
    """)
    time.sleep(1.2)

    # Stage 4 — all done, summary rendered
    log("  Stage 4/4: response complete...")
    page.evaluate(f"""
        () => {{
            const data = Alpine.$data(document.querySelector('[x-data="openosint"]'));
            if (!data) return;
            const ai = data.messages[1];
            ai.parts = [
                {{ type: 'text', content: '{intro_content}', streaming: false }},
                {{ type: 'tool', tool: 'search_email', input: '{email_js}',
                   status: 'done', output: '{email_out_js}', elapsed: 1.4, collapsed: false }},
                {{ type: 'tool', tool: 'search_breach', input: '{email_js}',
                   status: 'done', output: '{breach_out_js}', elapsed: 1.1, collapsed: false }},
                {{ type: 'text', content: '{summary_js}', streaming: false }}
            ];
            data.isStreaming = false;
        }}
    """)
    time.sleep(2)


def record() -> None:
    try:
        from playwright.sync_api import sync_playwright
    except ImportError:
        sys.exit(
            "[record_demo] ERROR: playwright not installed.\n"
            "  pip install playwright && playwright install chromium"
        )

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        context = browser.new_context(
            viewport={"width": 1280, "height": 720},
            record_video_dir=str(OUT_DIR),
            record_video_size={"width": 1280, "height": 720},
        )
        page = context.new_page()

        # ------------------------------------------------------------------
        # Scene 1 (4s) — landing page, empty state, suggestion chips visible
        # ------------------------------------------------------------------
        log("Scene 1: landing page...")
        page.goto(BASE_URL)
        page.wait_for_load_state("networkidle")
        time.sleep(4)
        page.screenshot(path=str(SS_DIR / "01-landing.png"))
        log("Screenshot: 01-landing.png")

        # ------------------------------------------------------------------
        # Scene 2 (3s) — fill input to show "about to investigate" state
        # ------------------------------------------------------------------
        log("Scene 2: filling input...")
        _set_state(page, f"data.input = 'Investigate {_EMAIL}';")
        time.sleep(2)
        page.screenshot(path=str(SS_DIR / "02-input.png"))
        log("Screenshot: 02-input.png")
        time.sleep(1)

        # ------------------------------------------------------------------
        # Scene 3 (~6s) — animated investigation in 4 stages
        # ------------------------------------------------------------------
        log("Scene 3: running animated investigation...")
        _inject_investigation(page)
        page.screenshot(path=str(SS_DIR / "03-investigation.png"))
        log("Screenshot: 03-investigation.png")

        # ------------------------------------------------------------------
        # Scene 4 (3s) — scroll to top then back to show full conversation
        # ------------------------------------------------------------------
        log("Scene 4: scrolling conversation...")
        page.evaluate("document.getElementById('chat-area').scrollTo(0, 0)")
        time.sleep(1)
        page.evaluate(
            "document.getElementById('chat-area').scrollTo(0, "
            "document.getElementById('chat-area').scrollHeight)"
        )
        time.sleep(2)

        # ------------------------------------------------------------------
        # Scene 5 (3s) — toggle to light theme
        # ------------------------------------------------------------------
        log("Scene 5: switching to light theme...")
        _click_theme_toggle(page)
        time.sleep(2)
        page.screenshot(path=str(SS_DIR / "04-light-theme.png"))
        log("Screenshot: 04-light-theme.png")

        # ------------------------------------------------------------------
        # Scene 6 (3s) — toggle back to dark
        # ------------------------------------------------------------------
        log("Scene 6: switching back to dark theme...")
        _click_theme_toggle(page)
        time.sleep(2)

        context.close()
        browser.close()

        webm_files = sorted(OUT_DIR.glob("*.webm"), key=lambda f: f.stat().st_mtime)
        if not webm_files:
            log("WARNING: no .webm file found — conversion skipped.")
            return

        webm = webm_files[-1]
        log(f"Raw video: {webm} ({webm.stat().st_size / 1_048_576:.1f} MB)")
        convert_video(webm)


def _click_theme_toggle(page) -> None:
    for selector in [
        "button[aria-label='Toggle theme']",
        "button[title='Switch to dark theme']",
        "button[title='Switch to light theme']",
    ]:
        try:
            page.locator(selector).first.click(timeout=3000)
            return
        except Exception:
            continue
    log("  (theme toggle: all selectors failed)")


def main() -> None:
    server = start_server()
    try:
        record()
    finally:
        log("Stopping web server...")
        server.terminate()
        server.wait(timeout=5)
        log("Done.")


if __name__ == "__main__":
    main()
