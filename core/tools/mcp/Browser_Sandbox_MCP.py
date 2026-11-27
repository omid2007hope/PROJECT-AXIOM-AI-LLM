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
Browser Sandbox MCP — Tier-2 Isolated Playwright Chromium Server for Local LLMs.

Designed for Ollama + Cherry Studio (and any stdio MCP host):
- Launches an isolated, ephemeral browser context (zero personal cookies or logins).
- Dispatches all events inside Chromium without touching the Windows cursor or focus.
- Returns token-capped, numbered element lists (`[1]`, `[2]`, ...) and cleaned text.
- Handles `<select>` dropdowns, rich-text `[contenteditable]` composers, and `target="_blank"` popups.
- Stops and reports cleanly if gated by CAPTCHA or anti-bot verification challenges.
"""

import asyncio
import logging
import os
import re
import sys
from typing import Optional
from urllib.parse import urlparse

try:  # mcp 2.x renamed FastMCP -> MCPServer
    from mcp.server.mcpserver import MCPServer as FastMCP
except ImportError:  # mcp 1.x
    from mcp.server.fastmcp import FastMCP

from playwright.async_api import Browser, BrowserContext, Locator, Page, Playwright, async_playwright

if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

logging.basicConfig(
    level=os.getenv("SANDBOX_LOG_LEVEL", "WARNING"),
    format="%(asctime)s [%(levelname)s] %(message)s",
    stream=sys.stderr,
)
logger = logging.getLogger("browser_sandbox")

SANDBOX_HEADLESS = os.getenv("SANDBOX_HEADLESS", "false").strip().lower() == "true"
SANDBOX_CHANNEL = os.getenv("SANDBOX_CHANNEL", "").strip()
NAVIGATION_TIMEOUT = max(3000, int(os.getenv("NAV_TIMEOUT_MS", "25000")))
ACTION_TIMEOUT = max(2000, min(NAVIGATION_TIMEOUT, 10000))

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


class SandboxSession:
    """Manages a single isolated, ephemeral Playwright browser session with deterministic cleanup."""

    def __init__(self) -> None:
        self.playwright: Optional[Playwright] = None
        self.browser: Optional[Browser] = None
        self.context: Optional[BrowserContext] = None
        self.active_page: Optional[Page] = None
        self.lock = asyncio.Lock()

    async def _launch_browser(self, driver: Playwright) -> Browser:
        channels = [SANDBOX_CHANNEL] if SANDBOX_CHANNEL else ["", "msedge", "chrome"]
        last_error: Optional[Exception] = None
        for channel in channels:
            try:
                if channel:
                    return await driver.chromium.launch(headless=SANDBOX_HEADLESS, channel=channel)
                return await driver.chromium.launch(headless=SANDBOX_HEADLESS)
            except Exception as error:
                last_error = error
        raise RuntimeError(f"Unable to launch Chromium/Edge sandbox browser: {last_error}")

    async def ensure_page(self) -> Page:
        if (
            self.active_page is not None
            and not self.active_page.is_closed()
            and self.browser is not None
            and self.browser.is_connected()
        ):
            return self.active_page

        if self.playwright is None:
            self.playwright = await async_playwright().start()
        if self.browser is None or not self.browser.is_connected():
            self.browser = await self._launch_browser(self.playwright)
            self.context = None
        if self.context is None:
            self.context = await self.browser.new_context(viewport={"width": 1280, "height": 800})
        open_pages = [page for page in self.context.pages if not page.is_closed()]
        self.active_page = open_pages[-1] if open_pages else await self.context.new_page()
        self.active_page.set_default_timeout(NAVIGATION_TIMEOUT)
        return self.active_page

    async def adopt_popup(self, previous_pages: set[Page]) -> bool:
        if self.context is None:
            return False
        new_pages = [page for page in self.context.pages if page not in previous_pages and not page.is_closed()]
        if not new_pages:
            return False
        newest = new_pages[-1]
        newest.set_default_timeout(NAVIGATION_TIMEOUT)
        try:
            await newest.wait_for_load_state("domcontentloaded", timeout=5000)
        except Exception:
            pass
        self.active_page = newest
        return True

    async def close(self) -> None:
        try:
            if self.context is not None:
                try:
                    await self.context.close()
                except Exception as error:
                    logger.debug("Context close ignored: %s", error)
            if self.browser is not None:
                try:
                    await self.browser.close()
                except Exception as error:
                    logger.debug("Browser close ignored: %s", error)
            if self.playwright is not None:
                try:
                    await self.playwright.stop()
                except Exception as error:
                    logger.debug("Playwright stop ignored: %s", error)
        finally:
            self.active_page = None
            self.context = None
            self.browser = None
            self.playwright = None


mcp = FastMCP(
    "browser-sandbox",
    instructions=(
        "Tier-2 isolated Playwright sandbox browser. Call open_url first, get_elements to list "
        "interactive [id] controls before click_element or type_text, read_text for clean page text, "
        "and close_session when finished. Never attempt to solve or bypass CAPTCHAs or anti-bot gates."
    ),
)
session = SandboxSession()


@mcp.tool()
async def open_url(url: str) -> str:
    """Open an http/https URL in the isolated sandbox browser and return title and final URL."""
    async with session.lock:
        try:
            target_url = normalize_url(url)
            page = await session.ensure_page()
            await page.goto(target_url, wait_until="domcontentloaded", timeout=NAVIGATION_TIMEOUT)
            await page.wait_for_timeout(800)
            title = await page.title()
            sample = await page.evaluate(EXTRACT_SCRIPT)
            if detect_gate(title, sample):
                return (
                    f"Blocked by verification/CAPTCHA gate on {page.url} ({title}). "
                    "Stopped cleanly without attempting bypass."
                )
            return f"Opened: {title}\nURL: {page.url}"
        except Exception as error:
            return f"Error opening URL: {error}"


@mcp.tool()
async def get_elements(max_elements: int = 40) -> str:
    """Return a compact numbered list of visible interactive elements on the current sandbox page."""
    async with session.lock:
        try:
            limit = max(1, min(int(max_elements), 80))
            page = await session.ensure_page()
            title = await page.title()
            sample = await page.evaluate(EXTRACT_SCRIPT)
            if detect_gate(title, sample):
                return f"Blocked by verification/CAPTCHA gate on {page.url}. Do not click or bypass."
            items = await page.evaluate(ELEMENT_SCRIPT, limit)
            if not items:
                return f"URL: {page.url}\nNo visible interactive elements found."
            return f"URL: {page.url}\n" + "\n".join(items)
        except Exception as error:
            return f"Error listing elements: {error}"


@mcp.tool()
async def click_element(element_id: int) -> str:
    """Click a numbered element from get_elements() by its integer ID (follows popup tabs automatically)."""
    async with session.lock:
        try:
            page = await session.ensure_page()
            previous_pages = set(session.context.pages) if session.context else {page}
            locator = page.locator(f'[data-axiom-id="{int(element_id)}"]').first
            await locator.click(timeout=ACTION_TIMEOUT)
            await page.wait_for_timeout(900)
            switched = await session.adopt_popup(previous_pages)
            active = await session.ensure_page()
            try:
                await active.wait_for_load_state("domcontentloaded", timeout=4000)
            except Exception:
                pass
            note = " (switched to new popup tab)" if switched else ""
            return f"Clicked [{element_id}]{note}. Current page: {await active.title()} ({active.url})"
        except Exception as error:
            return f"Error clicking element [{element_id}]: {error}"


@mcp.tool()
async def type_text(element_id: int, text: str, press_enter: bool = False) -> str:
    """Type into an input/textarea/editable element or choose a <select> option by ID from get_elements()."""
    async with session.lock:
        try:
            page = await session.ensure_page()
            previous_pages = set(session.context.pages) if session.context else {page}
            locator = page.locator(f'[data-axiom-id="{int(element_id)}"]').first
            action_label = await perform_typing(locator, text)
            if press_enter:
                await locator.press("Enter")
                await page.wait_for_timeout(1100)
                await session.adopt_popup(previous_pages)
            active = await session.ensure_page()
            return f"{action_label} [{element_id}]. Current URL: {active.url}"
        except Exception as error:
            return f"Error typing into element [{element_id}]: {error}"


@mcp.tool()
async def scroll_page(direction: str = "down", pixels: int = 700) -> str:
    """Scroll the sandbox page 'up' or 'down' by a pixel delta."""
    async with session.lock:
        try:
            page = await session.ensure_page()
            step = max(50, min(abs(int(pixels)), 5000))
            delta = -step if direction.strip().lower() == "up" else step
            await page.mouse.wheel(0, delta)
            await page.wait_for_timeout(400)
            return f"Scrolled {'up' if delta < 0 else 'down'} by {step}px."
        except Exception as error:
            return f"Error scrolling: {error}"


@mcp.tool()
async def read_text(max_chars: int = 4000) -> str:
    """Extract cleaned visible text from the current sandbox page, truncated to max_chars."""
    async with session.lock:
        try:
            character_limit = max(200, min(int(max_chars), 16000))
            page = await session.ensure_page()
            title = await page.title()
            raw_text = await page.evaluate(EXTRACT_SCRIPT)
            if detect_gate(title, raw_text):
                return (
                    f"Blocked by verification/CAPTCHA gate on {page.url} ({title}). "
                    "Stopped cleanly without attempting bypass."
                )
            cleaned = re.sub(r"\n{3,}", "\n\n", raw_text or "").strip()
            if len(cleaned) > character_limit:
                cleaned = cleaned[:character_limit] + "\n...[truncated]"
            return f"Title: {title}\nURL: {page.url}\n\n{cleaned}"
        except Exception as error:
            return f"Error reading text: {error}"


@mcp.tool()
async def close_session() -> str:
    """Close the sandbox browser deterministically and discard all ephemeral session state."""
    async with session.lock:
        await session.close()
        return "Sandbox browser session closed."


if __name__ == "__main__":
    mcp.run()
