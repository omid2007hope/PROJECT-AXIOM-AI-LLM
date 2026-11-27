"""
// SPDX-License-Identifier: LicenseRef-Axiom-Proprietary
/*
 * Copyright (c) 2026 Omid Teimory. All Rights Reserved.
 *
 * PROPRIETARY SOURCE-AVAILABLE SOFTWARE — NOT OPEN SOURCE.
 * Permission to view, clone, or compile this file is granted strictly on the
 * condition precedent of full compliance with the root ./LICENSE file.
 * Copying snippets, modifying, creating derivative works, redistributing,
 * commercial/SaaS use, and AI/ML training (reserved under EU Directive
 * 2019/790 Art. 4(3) and § 42h(6) Austrian UrhG) are strictly prohibited.
 * Removal or alteration of this notice violates 17 U.S.C. § 1202 and § 90c UrhG.
 */
"""

"""
Browser Live MCP — Tier-3 Side-by-Side Visible Browser Server over CDP.

Attaches over loopback Chrome DevTools Protocol (`CDP_URL`, default `http://127.0.0.1:9222`)
to a dedicated visible Chrome or Microsoft Edge window (`AXIOM_LiveBrowser`):
- Zero interference with Windows' global physical mouse cursor or keyboard focus.
- Preserves dedicated research logins/cookies across sessions via `--user-data-dir`.
- Uses `connect_over_cdp(..., is_local=True, no_defaults=True)` so existing profile settings stay intact.
- Auto-launches the docked browser window if port 9222 is closed (when `LIVE_AUTO_LAUNCH=true`).
- Drops the local Playwright driver cleanly on `disconnect_session()` without closing the user's browser.
- Never attempts to solve or bypass CAPTCHAs/anti-bot challenges; yields to the user if gated.
"""

import asyncio
import json
import logging
import os
import re
import subprocess
import sys
import time
import urllib.request
from pathlib import Path
from typing import Optional
from urllib.parse import urlparse

try:  # mcp 2.x renamed FastMCP -> MCPServer
    from mcp.server.mcpserver import MCPServer as FastMCP
except ImportError:  # mcp 1.x
    from mcp.server.fastmcp import FastMCP

from playwright.async_api import Browser, Locator, Page, Playwright, async_playwright

if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

logging.basicConfig(
    level=os.getenv("LIVE_LOG_LEVEL", "WARNING"),
    format="%(asctime)s [%(levelname)s] %(message)s",
    stream=sys.stderr,
)
logger = logging.getLogger("browser_live")

CDP_URL = os.getenv("CDP_URL", "http://127.0.0.1:9222").strip().rstrip("/")
ALLOW_REMOTE_CDP = os.getenv("ALLOW_REMOTE_CDP", "false").strip().lower() == "true"
LIVE_AUTO_LAUNCH = os.getenv("LIVE_AUTO_LAUNCH", "true").strip().lower() == "true"
NAVIGATION_TIMEOUT = max(3000, int(os.getenv("NAV_TIMEOUT_MS", "25000")))
ACTION_TIMEOUT = max(2000, min(NAVIGATION_TIMEOUT, 10000))
WINDOW_POSITION = os.getenv("LIVE_WINDOW_POSITION", "960,0").strip()
WINDOW_SIZE = os.getenv("LIVE_WINDOW_SIZE", "960,1040").strip()

LOOPBACK_HOSTS = {"127.0.0.1", "localhost", "::1"}
IGNORED_SCHEMES = ("devtools://", "chrome-extension://", "extension://")

GATE_MARKERS = (
    "please verify you are a human",
    "verify you are human",
    "checking your browser before accessing",
    "attention required! | cloudflare",
    "just a moment...",
    "enable javascript and cookies to continue",
    "cf-turnstile",
    "g-recaptcha",
    "hcaptcha",
)

ELEMENT_SCRIPT = """
(maxElements) => {
  document.querySelectorAll('[data-axiom-id]').forEach((node) => node.removeAttribute('data-axiom-id'));
  const selector = [
    'a[href]',
    'button',
    'input:not([type="hidden"])',
    'textarea',
    'select',
    'summary',
    '[role="button"]',
    '[role="link"]',
    '[role="textbox"]',
    '[role="searchbox"]',
    '[role="combobox"]',
    '[role="tab"]',
    '[role="menuitem"]',
    '[contenteditable="true"]',
    '[contenteditable=""]'
  ].join(', ');

  const candidates = Array.from(document.querySelectorAll(selector));
  const results = [];
  let index = 1;

  for (const element of candidates) {
    if (results.length >= maxElements) break;
    if (element.disabled || element.getAttribute('aria-disabled') === 'true') continue;

    const computedStyle = window.getComputedStyle(element);
    if (
      computedStyle.display === 'none' ||
      computedStyle.visibility === 'hidden' ||
      parseFloat(computedStyle.opacity || '1') === 0
    ) {
      continue;
    }

    const boundingBox = element.getBoundingClientRect();
    if (boundingBox.width < 4 || boundingBox.height < 4) continue;

    const tag = element.tagName.toLowerCase();
    const role = (element.getAttribute('role') || '').toLowerCase();
    const inputType = element.getAttribute('type') ? `(${element.getAttribute('type')})` : '';
    const isEditable = (
      tag === 'input' ||
      tag === 'textarea' ||
      tag === 'select' ||
      element.isContentEditable ||
      role === 'textbox' ||
      role === 'searchbox' ||
      role === 'combobox'
    );

    let rawLabel = '';
    if (tag === 'select') {
      const options = Array.from(element.options || [])
        .map((option) => (option.text || option.value || '').trim())
        .filter(Boolean)
        .slice(0, 5);
      const baseName = element.getAttribute('aria-label') || element.getAttribute('name') || 'dropdown';
      rawLabel = `${baseName} [options: ${options.join(' | ')}]`;
    } else {
      rawLabel = (
        element.innerText ||
        element.getAttribute('aria-label') ||
        element.getAttribute('placeholder') ||
        element.getAttribute('title') ||
        element.getAttribute('name') ||
        element.value ||
        ''
      );
    }

    const cleanLabel = rawLabel.replace(/\\s+/g, ' ').trim().slice(0, 80);
    if (!cleanLabel && !isEditable) continue;

    element.setAttribute('data-axiom-id', String(index));
    const kind = element.isContentEditable && tag !== 'input' && tag !== 'textarea'
      ? `${tag}(editable)`
      : `${tag}${inputType}`;
    results.push(`[${index}] ${kind}: ${cleanLabel || '(empty input)'}`);
    index += 1;
  }
  return results;
}
"""

EXTRACT_SCRIPT = """
() => {
  const clone = document.body ? document.body.cloneNode(true) : null;
  if (!clone) return '';
  clone.querySelectorAll('script, style, noscript, svg, iframe, template').forEach((node) => node.remove());
  return clone.innerText || '';
}
"""


def validate_endpoint(endpoint_url: str) -> str:
    parsed = urlparse(endpoint_url)
    host = (parsed.hostname or "").lower()
    if parsed.scheme not in ("http", "https", "ws", "wss") or not host:
        raise ValueError(f"Invalid CDP_URL: {endpoint_url}")
    if not ALLOW_REMOTE_CDP and host not in LOOPBACK_HOSTS:
        raise ValueError(f"Refusing non-loopback CDP host '{host}' (set ALLOW_REMOTE_CDP=true to override).")
    return endpoint_url


def normalize_url(raw_url: str) -> str:
    candidate = (raw_url or "").strip()
    if not candidate:
        raise ValueError("URL must not be empty.")
    if not candidate.startswith(("http://", "https://")):
        if "://" in candidate or candidate.lower().startswith(("javascript:", "data:", "file:", "about:", "chrome:", "edge:")):
            raise ValueError("Only http:// and https:// URLs are permitted.")
        candidate = f"https://{candidate}"
    parsed = urlparse(candidate)
    if parsed.scheme not in ("http", "https") or not parsed.netloc:
        raise ValueError(f"Invalid URL: {raw_url}")
    return candidate


def detect_gate(title: str, sample_text: str) -> bool:
    haystack = f"{title}\n{sample_text[:2000]}".lower()
    return any(marker in haystack for marker in GATE_MARKERS)


def probe_endpoint(endpoint_url: str, timeout_seconds: float = 1.5) -> bool:
    parsed = urlparse(endpoint_url)
    if parsed.scheme in ("ws", "wss"):
        return True
    version_url = f"{endpoint_url.rstrip('/')}/json/version"
    try:
        with urllib.request.urlopen(version_url, timeout=timeout_seconds) as response:
            payload = json.loads(response.read().decode("utf-8", errors="replace"))
            return bool(payload.get("webSocketDebuggerUrl"))
    except Exception:
        return False


def find_browser_binary() -> Optional[Path]:
    override = os.getenv("LIVE_BROWSER_EXE", "").strip()
    candidates = [override] if override else [
        os.path.expandvars(r"%ProgramFiles%\Google\Chrome\Application\chrome.exe"),
        os.path.expandvars(r"%LOCALAPPDATA%\Google\Chrome\Application\chrome.exe"),
        os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft\Edge\Application\msedge.exe"),
        os.path.expandvars(r"%ProgramFiles%\Microsoft\Edge\Application\msedge.exe"),
    ]
    for item in candidates:
        if item and Path(item).is_file():
            return Path(item)
    return None


def spawn_docked_browser(endpoint_url: str) -> bool:
    binary = find_browser_binary()
    if binary is None:
        return False
    parsed = urlparse(endpoint_url)
    port = parsed.port or 9222
    profile_directory = Path(os.getenv("LIVE_PROFILE_DIR", os.path.expandvars(r"%LOCALAPPDATA%\AXIOM_LiveBrowser")))
    profile_directory.mkdir(parents=True, exist_ok=True)
    command = [
        str(binary),
        f"--remote-debugging-port={port}",
        "--remote-debugging-address=127.0.0.1",
        f"--user-data-dir={profile_directory}",
        "--no-first-run",
        "--no-default-browser-check",
        f"--window-position={WINDOW_POSITION}",
        f"--window-size={WINDOW_SIZE}",
    ]
    creation_flags = 0
    if sys.platform == "win32":
        creation_flags = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
    subprocess.Popen(
        command,
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        close_fds=True,
        creationflags=creation_flags,
    )
    deadline = time.monotonic() + 6.0
    while time.monotonic() < deadline:
        if probe_endpoint(endpoint_url, timeout_seconds=0.8):
            return True
        time.sleep(0.25)
    return False


async def perform_typing(locator: Locator, text: str) -> str:
    metadata = await locator.evaluate(
        """(element) => ({
            tag: element.tagName.toLowerCase(),
            editable: Boolean(element.isContentEditable)
        })"""
    )
    tag = metadata.get("tag", "")
    if tag == "select":
        try:
            await locator.select_option(label=text, timeout=ACTION_TIMEOUT)
        except Exception:
            await locator.select_option(value=text, timeout=ACTION_TIMEOUT)
        return "Selected option"

    await locator.click(timeout=ACTION_TIMEOUT)
    if metadata.get("editable") and tag not in ("input", "textarea"):
        try:
            await locator.fill(text, timeout=ACTION_TIMEOUT)
        except Exception:
            await locator.press("Control+A")
            await locator.press("Backspace")
            await locator.press_sequentially(text, delay=12)
        return "Typed into editable"

    await locator.fill(text, timeout=ACTION_TIMEOUT)
    return "Typed"


class LiveBrowserSession:
    """Manages CDP attachment to an external side-by-side Chrome/Edge browser without stealing OS focus."""

    def __init__(self) -> None:
        self.playwright: Optional[Playwright] = None
        self.browser: Optional[Browser] = None
        self.tab_index: int = 0
        self.lock = asyncio.Lock()

    async def connect_browser(self) -> Browser:
        endpoint = validate_endpoint(CDP_URL)
        endpoint_alive = await asyncio.to_thread(probe_endpoint, endpoint)
        if not endpoint_alive and LIVE_AUTO_LAUNCH:
            endpoint_alive = await asyncio.to_thread(spawn_docked_browser, endpoint)
        if not endpoint_alive:
            await self.disconnect()
            raise RuntimeError(
                f"Live browser is not reachable at {endpoint}. "
                "Run launch_live_browser.bat first (or set LIVE_AUTO_LAUNCH=true)."
            )

        if self.browser is not None and self.browser.is_connected():
            return self.browser

        await self.disconnect()
        self.playwright = await async_playwright().start()
        self.browser = await self.playwright.chromium.connect_over_cdp(
            endpoint,
            is_local=True,
            no_defaults=True,
            timeout=10000,
        )
        return self.browser

    async def collect_pages(self) -> list[Page]:
        browser = await self.connect_browser()
        pages: list[Page] = []
        for context in browser.contexts:
            for page in context.pages:
                if page.is_closed():
                    continue
                if page.url.startswith(IGNORED_SCHEMES):
                    continue
                pages.append(page)
        if not pages:
            context = browser.contexts[0] if browser.contexts else await browser.new_context()
            pages.append(await context.new_page())
        return pages

    async def active_page(self) -> Page:
        pages = await self.collect_pages()
        if self.tab_index < 0 or self.tab_index >= len(pages):
            self.tab_index = len(pages) - 1
        page = pages[self.tab_index]
        page.set_default_timeout(NAVIGATION_TIMEOUT)
        return page

    async def adopt_popup(self, previous_pages: set[Page]) -> bool:
        pages = await self.collect_pages()
        for index, candidate in enumerate(pages):
            if candidate not in previous_pages:
                self.tab_index = index
                candidate.set_default_timeout(NAVIGATION_TIMEOUT)
                try:
                    await candidate.wait_for_load_state("domcontentloaded", timeout=5000)
                except Exception:
                    pass
                return True
        return False

    async def disconnect(self) -> None:
        """Stops only the local Playwright client driver so the external Chrome/Edge window stays open."""
        try:
            if self.playwright is not None:
                try:
                    await self.playwright.stop()
                except Exception as error:
                    logger.debug("Playwright driver stop ignored: %s", error)
        finally:
            self.browser = None
            self.playwright = None


mcp = FastMCP(
    "browser-live",
    instructions=(
        "Tier-3 side-by-side visible Chrome/Edge controller over CDP (never touches your physical mouse "
        "or keyboard). Call list_tabs first, get_elements before click_element or type_text, and read_text "
        "for page text. Never bypass CAPTCHAs or anti-bot gates; ask the user to handle logins/verification manually."
    ),
)
session = LiveBrowserSession()


@mcp.tool()
async def list_tabs() -> str:
    """List open tabs in the live side-by-side browser (marks active tab with '*')."""
    async with session.lock:
        try:
            pages = await session.collect_pages()
            if session.tab_index >= len(pages):
                session.tab_index = len(pages) - 1
            lines = []
            for index, page in enumerate(pages):
                marker = "*" if index == session.tab_index else " "
                lines.append(f"{marker}[{index}] {await page.title()} — {page.url}")
            return "\n".join(lines)
        except Exception as error:
            return f"Cannot connect to live browser at {CDP_URL}. Details: {error}"


@mcp.tool()
async def switch_tab(tab_index: int) -> str:
    """Select which tab index the agent controls in the live side-by-side browser."""
    async with session.lock:
        try:
            pages = await session.collect_pages()
            target = int(tab_index)
            if target < 0 or target >= len(pages):
                return f"Invalid tab_index {target}. Open tabs: 0 to {len(pages) - 1}."
            session.tab_index = target
            chosen = pages[target]
            return f"Switched to tab [{target}]: {await chosen.title()} ({chosen.url})"
        except Exception as error:
            return f"Error switching tab: {error}"


@mcp.tool()
async def open_url(url: str, new_tab: bool = False) -> str:
    """Navigate the live side-by-side browser to an http/https URL without touching your mouse or keyboard."""
    async with session.lock:
        try:
            target_url = normalize_url(url)
            if new_tab:
                browser = await session.connect_browser()
                context = browser.contexts[0] if browser.contexts else await browser.new_context()
                page = await context.new_page()
                page.set_default_timeout(NAVIGATION_TIMEOUT)
                pages = await session.collect_pages()
                session.tab_index = pages.index(page) if page in pages else len(pages) - 1
            else:
                page = await session.active_page()
            await page.goto(target_url, wait_until="domcontentloaded", timeout=NAVIGATION_TIMEOUT)
            await page.wait_for_timeout(800)
            title = await page.title()
            sample = await page.evaluate(EXTRACT_SCRIPT)
            if detect_gate(title, sample):
                return (
                    f"Verification/CAPTCHA gate detected on {page.url} ({title}). "
                    "Stopped cleanly—please complete any required verification manually in the docked browser window."
                )
            return f"Opened: {title}\nURL: {page.url}"
        except Exception as error:
            return f"Error opening URL in live browser: {error}"


@mcp.tool()
async def get_elements(max_elements: int = 40) -> str:
    """Return a compact numbered list of visible interactive elements in the active live tab."""
    async with session.lock:
        try:
            limit = max(1, min(int(max_elements), 80))
            page = await session.active_page()
            title = await page.title()
            sample = await page.evaluate(EXTRACT_SCRIPT)
            if detect_gate(title, sample):
                return (
                    f"Verification/CAPTCHA gate active on {page.url}. "
                    "Please complete it manually in the side-by-side browser before continuing."
                )
            items = await page.evaluate(ELEMENT_SCRIPT, limit)
            if not items:
                return f"URL: {page.url}\nNo visible interactive elements found."
            return f"URL: {page.url}\n" + "\n".join(items)
        except Exception as error:
            return f"Error listing elements: {error}"


@mcp.tool()
async def click_element(element_id: int) -> str:
    """Dispatch a click directly to element_id in the live browser without moving your physical mouse."""
    async with session.lock:
        try:
            pages_before = set(await session.collect_pages())
            page = await session.active_page()
            locator = page.locator(f'[data-axiom-id="{int(element_id)}"]').first
            await locator.click(timeout=ACTION_TIMEOUT)
            await page.wait_for_timeout(900)
            switched = await session.adopt_popup(pages_before)
            active = await session.active_page()
            try:
                await active.wait_for_load_state("domcontentloaded", timeout=4000)
            except Exception:
                pass
            note = f" (followed new tab [{session.tab_index}])" if switched else ""
            return f"Clicked [{element_id}]{note}. Page: {await active.title()} ({active.url})"
        except Exception as error:
            return f"Error clicking [{element_id}]: {error}"


@mcp.tool()
async def type_text(element_id: int, text: str, press_enter: bool = False) -> str:
    """Type into element_id (or select a dropdown option) in the live browser without stealing keyboard focus."""
    async with session.lock:
        try:
            pages_before = set(await session.collect_pages())
            page = await session.active_page()
            locator = page.locator(f'[data-axiom-id="{int(element_id)}"]').first
            action_label = await perform_typing(locator, text)
            if press_enter:
                await locator.press("Enter")
                await page.wait_for_timeout(1100)
                await session.adopt_popup(pages_before)
            active = await session.active_page()
            return f"{action_label} [{element_id}]. URL: {active.url}"
        except Exception as error:
            return f"Error typing into [{element_id}]: {error}"


@mcp.tool()
async def scroll_page(direction: str = "down", pixels: int = 700) -> str:
    """Scroll the active live browser tab 'up' or 'down' without affecting your physical mouse."""
    async with session.lock:
        try:
            page = await session.active_page()
            step = max(50, min(abs(int(pixels)), 5000))
            delta = -step if direction.strip().lower() == "up" else step
            await page.mouse.wheel(0, delta)
            await page.wait_for_timeout(400)
            return f"Scrolled {'up' if delta < 0 else 'down'} by {step}px."
        except Exception as error:
            return f"Error scrolling: {error}"


@mcp.tool()
async def read_text(max_chars: int = 4000) -> str:
    """Extract cleaned visible text from the active live tab, truncated to max_chars."""
    async with session.lock:
        try:
            character_limit = max(200, min(int(max_chars), 16000))
            page = await session.active_page()
            title = await page.title()
            raw_text = await page.evaluate(EXTRACT_SCRIPT)
            if detect_gate(title, raw_text):
                return (
                    f"Verification/CAPTCHA gate detected on {page.url} ({title}). "
                    "Please complete verification manually in the docked browser window."
                )
            cleaned = re.sub(r"\n{3,}", "\n\n", raw_text or "").strip()
            if len(cleaned) > character_limit:
                cleaned = cleaned[:character_limit] + "\n...[truncated]"
            return f"Title: {title}\nURL: {page.url}\n\n{cleaned}"
        except Exception as error:
            return f"Error reading text: {error}"


@mcp.tool()
async def disconnect_session() -> str:
    """Detach MCP from the live browser while keeping your side-by-side Chrome/Edge window open."""
    async with session.lock:
        await session.disconnect()
        return "Disconnected from live browser (window left open)."


if __name__ == "__main__":
    mcp.run()
