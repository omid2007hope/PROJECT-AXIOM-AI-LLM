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
Specter MCP — Standalone, LLM-Agnostic All-Source OSINT & Sensory Fusion Server.

Works with ANY MCP host LLM (AnythingLLM, Claude, OpenAI, Gemini, Ollama, LM Studio,
Cursor, Cline, Antigravity) with ZERO hard dependency on Project Axiom, Ollama,
ComfyUI, or Playwright:

  Phase 1  Immutable Context Anchor + competing hypotheses (H1 target vs H2 namesake)
  Phase 2  Multi-source harvest: web search + keyless GitHub/Wikipedia enrichment
           + relevance gate + Trafilatura / stdlib HTML & document extractors
  Phase 3  Grounded ACH + NATO Admiralty ratings (A1-F6) + clean bounded pivots (<= 2)
  Phase 4  ICD-203 Markdown dossier + SHA-256 chain-of-custody & redaction appendices
  Sensors  NASA FIRMS thermal anomalies + USGS seismic events + regional news fusion

Design invariants:
- Host-LLM-first: deterministic core runs in seconds with ZERO internal VRAM contention.
- Optional pluggable LLM backend (`SPECTER_LLM_PROVIDER=openai|ollama`) if desired.
- Hard wall-clock deadline (`max_seconds`, default 42s) prevents 60s MCP client timeouts.
- Ethical guardrails: public open sources only, RFC 9309 robots.txt honoured, per-domain
  rate limiting, anti-bot/login gates never bypassed, PII redacted, full audit trail.
- All logs route strictly to stderr so MCP stdio JSON-RPC framing is never corrupted.
"""

import asyncio
import csv
import ctypes
import hashlib
import html
import io
import json
import logging
import math
import os
import re
import sqlite3
import sys
import threading
import time
import uuid
import warnings
import zipfile
from concurrent.futures import ThreadPoolExecutor
from contextlib import closing
from datetime import datetime, timedelta, timezone
from html.parser import HTMLParser
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Tuple
from urllib.parse import parse_qsl, urlencode, urljoin, urlparse, urlunparse
from urllib.robotparser import RobotFileParser

import requests

try:  # mcp 2.x renamed FastMCP -> MCPServer
    from mcp.server.mcpserver import Context, MCPServer as FastMCP
except ImportError:  # mcp 1.x
    from mcp.server.fastmcp import Context, FastMCP

logging.basicConfig(
    level=os.getenv("SPECTER_LOG_LEVEL", "INFO"),
    format="%(asctime)s [%(levelname)s] %(message)s",
    stream=sys.stderr,
)
logger = logging.getLogger("specter")

# --- Optional dependencies (server boots and runs cleanly without them) -------
try:
    import trafilatura
except ImportError:
    trafilatura = None

warnings.filterwarnings("ignore", message=".*duckduckgo_search.*", category=RuntimeWarning)
try:
    from ddgs import DDGS
except ImportError:
    try:
        from duckduckgo_search import DDGS
    except ImportError:
        DDGS = None


# ==============================================================================
# 0. CONFIGURATION (100% standalone; every option overridable via environment)
# ==============================================================================
SCRIPT_DIRECTORY = Path(__file__).resolve().parent
SPECTER_HOME = Path(os.getenv("SPECTER_HOME", str(SCRIPT_DIRECTORY / "specter_data")))
CASES_DIRECTORY = SPECTER_HOME / "cases"

# Optional external LLM backend (default "none": host MCP LLM handles reasoning)
LLM_PROVIDER = os.getenv("SPECTER_LLM_PROVIDER", "none").strip().lower()  # none | openai | ollama
LLM_URL = os.getenv("SPECTER_LLM_URL", "").strip().rstrip("/")
LLM_MODEL = os.getenv("SPECTER_LLM_MODEL", "").strip()
LLM_API_KEY = os.getenv("SPECTER_LLM_API_KEY", "").strip()
LLM_CONTEXT_TOKENS = int(os.getenv("SPECTER_NUM_CTX", "32768"))

# Optional external plugins (never required)
NATIVE_DLL_PATH = Path(os.getenv("SPECTER_NATIVE_DLL", "")) if os.getenv("SPECTER_NATIVE_DLL") else None
FIRMS_MAP_KEY = os.getenv("SPECTER_FIRMS_KEY", "").strip()
FIRMS_SOURCE = os.getenv("SPECTER_FIRMS_SOURCE", "VIIRS_SNPP_NRT").strip()

USER_AGENT = "Mozilla/5.0 (compatible; SpecterOSINT/4.0; public-source research; honours robots.txt)"
MAX_RESEARCH_PASSES = 2
MAX_TRAVERSAL_DEPTH = 2
MAX_TRAVERSAL_PAGES = 16
DEFAULT_CYCLE_SECONDS = float(os.getenv("SPECTER_CYCLE_SECONDS", "42"))
DOMAIN_INTERVAL_SECONDS = float(os.getenv("SPECTER_DOMAIN_INTERVAL", "1.0"))
MAX_DOWNLOAD_BYTES = 25 * 1024 * 1024
MAX_STORED_CHARACTERS = 100_000
MIN_ARTICLE_CHARACTERS = 140
DOCUMENT_EXTENSIONS = {".pdf", ".docx", ".txt", ".md", ".json", ".csv", ".xml"}
ASSET_EXTENSIONS = {
    ".png", ".jpg", ".jpeg", ".gif", ".webp", ".svg", ".ico", ".avif", ".bmp", ".tiff",
    ".css", ".js", ".mjs", ".map", ".woff", ".woff2", ".ttf", ".eot",
    ".mp3", ".wav", ".ogg", ".mp4", ".webm", ".avi", ".mov", ".mkv",
    ".zip", ".gz", ".tar", ".tgz", ".bz2", ".xz", ".7z", ".rar",
    ".exe", ".msi", ".dmg", ".pkg", ".deb", ".rpm", ".apk", ".bin", ".iso", ".dll", ".so",
}
HIGH_VALUE_PATHS = (
    "/about", "/project", "/resume", "/cv", "/work", "/research",
    "/publication", "/portfolio", "/blog", "/contact", "/bio", "/profile",
)
SKIP_LINK_SEGMENTS = (
    "privacy", "terms", "cookie", "login", "signin", "signup", "register",
    "share?", "intent/tweet", "sharer.php", "wp-content", "cdn-cgi", "{search_term_string}", "%7b", "/www.",
)
SKIP_LINK_DOMAINS = (
    "fonts.googleapis.com", "fonts.gstatic.com", "w3.org", "schema.org",
    "creativecommons.org", "cloudflare.com", "google.com", "gstatic.com",
    "apple.com", "microsoft.com", "adobe.com", "gravatar.com", "avatars.githubusercontent.com",
    "consent.youtube.com", "consent.google.com",
)
CONSENT_MARKERS = (
    "before you continue to youtube", "bevor sie zu youtube weitergehen",
    "before you continue to google", "bevor sie zu google weitergehen",
    "enable javascript and cookies to continue", "please verify you are a human",
    "checking your browser before accessing", "attention required! | cloudflare",
)
BOILERPLATE_PHRASES = (
    "what these projects show", "problem, approach, outcome", "each project is selected to show",
    "the fastest way to start a collaboration", "as more work is added to the portfolio",
    "that keeps the portfolio useful", "have a question or want to work together",
    "fill out the form below", "start here", "get in touch",
)
CORROBORATION_DOMAINS = (
    "github.com", "gitlab.com", "credly.com", "orcid.org", "wakatime.com",
    "committers.top", "npmjs.com", "pypi.org", "arxiv.org", "scholar.google.com",
    "huggingface.co", "kaggle.com", "stackoverflow.com", "dev.to", "substack.com",
    "medium.com", "linkedin.com", "x.com", "twitter.com", "youtube.com", "twitch.tv",
)
TRACKING_PARAMETERS = {
    "utm_source", "utm_medium", "utm_campaign", "utm_term", "utm_content",
    "fbclid", "gclid", "mc_eid", "msockid", "ref_src", "ref_url",
}
CASE_ID_PATTERN = re.compile(r"^[a-f0-9]{12}$")

PROSE_STOP_WORDS = {
    "a", "an", "and", "are", "as", "at", "be", "by", "confirm", "databases", "described",
    "e.g", "entity", "find", "for", "from", "history", "in", "is", "legal", "matching",
    "of", "official", "on", "or", "ownership", "profile", "query", "related", "search",
    "site", "that", "the", "this", "to", "under", "verify", "with",
}

AUTHORITATIVE_SUFFIXES = (".gov", ".gv.at", ".gov.uk", ".europa.eu", ".edu", ".ac.at", ".ac.uk", ".org")
REPUTABLE_DOMAINS = (
    "github.com", "gitlab.com", "wikipedia.org", "arxiv.org", "orcid.org",
    "scholar.google.com", "linkedin.com", "ieee.org", "acm.org", "npmjs.com",
    "pypi.org", "reuters.com", "apnews.com", "bbc.com", "firmenbuch.at", "justiz.gv.at",
)
SOCIAL_PLATFORMS = {
    "github.com": "github",
    "gitlab.com": "gitlab",
    "linkedin.com": "linkedin",
    "x.com": "x_twitter",
    "twitter.com": "x_twitter",
    "youtube.com": "youtube",
    "twitch.tv": "twitch",
    "instagram.com": "instagram",
    "orcid.org": "orcid",
    "medium.com": "medium",
    "substack.com": "substack",
}

CASES_DIRECTORY.mkdir(parents=True, exist_ok=True)


# ==============================================================================
# 1. PURE HELPERS (deterministic seed/query/HTML parsing, grounding, redaction)
# ==============================================================================
SENSITIVE_PATTERNS = [
    (re.compile(r"\b\d{3}-\d{2}-\d{4}\b"), "[REDACTED-ID]"),
    (re.compile(r"(?<![Oo][Rr][Cc][Ii][Dd]\.org/)(?<![Oo][Rr][Cc][Ii][Dd]: )(?<![Oo][Rr][Cc][Ii][Dd] )\b(?:\d{4}[ -]){3}\d{4}\b"), "[REDACTED-CARD]"),
    (re.compile(r"\+\d{1,3}(?:[\s.-]?\(?\d{2,4}\)?){2,4}\d"), "[REDACTED-PHONE]"),
    (re.compile(r"(?<!\w)\(?\d{3}\)?[\s.-]\d{3}[\s.-]\d{4}(?!\w)"), "[REDACTED-PHONE]"),
]


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def strip_reasoning(text: str) -> str:
    """Removes <think>...</think> traces emitted by reasoning models."""
    return re.sub(r"<think>.*?</think>", "", text or "", flags=re.S).strip()


def canonicalize_url(url: str) -> str:
    try:
        parsed = urlparse(url.strip())
        clean_query = [
            (key, value)
            for key, value in parse_qsl(parsed.query)
            if key.lower() not in TRACKING_PARAMETERS
        ]
        return urlunparse((parsed.scheme, parsed.netloc.lower(), parsed.path, parsed.params, urlencode(clean_query), ""))
    except ValueError:
        return url.strip()


def validate_case_id(case_id: str) -> str:
    """Prevents path traversal on every case-scoped path."""
    normalized = (case_id or "").strip().lower()
    if not CASE_ID_PATTERN.match(normalized):
        raise ValueError(f"Invalid case_id '{case_id}' (expected 12 hex characters).")
    return normalized


def redact_sensitive(text: str) -> str:
    for pattern, replacement in SENSITIVE_PATTERNS:
        text = pattern.sub(replacement, text)
    return text


def clean_strings(values: Any, limit: int) -> List[str]:
    if not isinstance(values, list):
        return []
    cleaned = [value.strip() for value in values if isinstance(value, str) and value.strip()]
    return list(dict.fromkeys(cleaned))[:limit]


def coerce_string_list(value: Any) -> List[str]:
    """Converts a string or list of strings into a normalized string list."""
    if isinstance(value, list):
        return [str(item).strip() for item in value if isinstance(item, str) and item.strip()]
    if isinstance(value, str) and value.strip():
        return [value.strip()]
    return []


def parse_seed(
    seed: str,
    aliases: Optional[List[str]] = None,
    handles: Optional[List[str]] = None,
    location: str = "",
    profession: str = "",
    hints: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    """Deterministic, zero-LLM seed parser (<1ms) preserving exact ground-truth qualifiers & hints."""
    raw = (seed or "").strip()
    hint_map = hints if isinstance(hints, dict) else {}
    extracted_aliases = list(aliases or []) + coerce_string_list(hint_map.get("aliases"))
    extracted_handles = list(handles or []) + coerce_string_list(hint_map.get("handles"))
    extracted_domains = coerce_string_list(hint_map.get("domains"))
    extracted_emails = coerce_string_list(hint_map.get("emails"))

    # Extract parenthetical aliases: "Name (formerly Alias)" / "(aka Alias)"
    for match in re.findall(r"\((?:formerly|aka|also known as|né|nee|alias)\s+([^)]+)\)", raw, flags=re.I):
        for piece in re.split(r"[,;/]", match):
            if piece.strip():
                extracted_aliases.append(piece.strip())

    # Extract explicit public emails, @handles, and domains from seed
    for email in re.findall(r"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b", raw):
        extracted_emails.append(email.lower())
    for handle in re.findall(r"(?<![\w.])@[A-Za-z0-9_.-]{2,}", raw):
        extracted_handles.append(handle)
    for domain in re.findall(r"\b(?:[a-zA-Z0-9-]+\.)+(?:com|org|net|io|dev|ai|app|at|ch|de|uk|edu|gov|me|co|eu)\b", raw):
        lowered = domain.lower()
        if not any(lowered in mail for mail in extracted_emails):
            extracted_domains.append(lowered)

    # Strip parenthetical notes to isolate primary_name and qualifiers
    stripped = re.sub(r"\([^)]*\)", "", raw).strip()
    parts = [part.strip() for part in stripped.split(",") if part.strip()]
    primary_name = parts[0] if parts else raw
    remainder = ", ".join(parts[1:]) if len(parts) > 1 else ""

    hint_location = hint_map.get("location")
    origin_city = location.strip() or (hint_location.strip() if isinstance(hint_location, str) else "")
    if not origin_city:
        loc_match = re.search(r"\b(?:from|in|based in)\s+([A-Z][A-Za-z\s-]{2,30})", raw)
        if loc_match:
            origin_city = loc_match.group(1).strip(" .,")

    birth_year: Optional[int] = None
    year_match = re.search(r"\b(?:born\s+(?:in\s+)?)?((?:19|20)\d{2})\b", raw)
    if year_match:
        candidate = int(year_match.group(1))
        if 1900 <= candidate <= datetime.now(timezone.utc).year:
            birth_year = candidate

    hint_profession = hint_map.get("profession")
    core_profession = (
        profession.strip()
        or (hint_profession.strip() if isinstance(hint_profession, str) else "")
        or remainder
    )
    entity_kind = "domain" if "." in primary_name and " " not in primary_name else "person"
    if re.search(r"\b(?:inc|llc|gmbh|ag|corp|ltd|foundation|university|institute)\b", raw, flags=re.I):
        entity_kind = "organization"

    clean_handles = clean_strings(extracted_handles, 10)
    clean_domains = clean_strings(extracted_domains, 10)
    clean_emails = clean_strings(extracted_emails, 10)

    return {
        "primary_name": primary_name or "Unknown Target",
        "entity_kind": entity_kind,
        "known_aliases": clean_strings(extracted_aliases, 10),
        "known_handles_or_domains": clean_strings(clean_handles + clean_domains, 15),
        "hints": {
            "emails": clean_emails,
            "phones": [],
            "handles": clean_handles,
            "domains": clean_domains,
        },
        "anchor_demographics": {"birth_year": birth_year, "origin_city": origin_city},
        "core_profession": core_profession[:160],
        "initial_hypotheses": [
            {"id": "H1", "label": "Primary Target", "description": f"Subject matching seed qualifiers: {raw}"},
            {"id": "H2", "label": "Namesake / Collision", "description": "Distinct entity sharing name, alias, or handle"},
        ],
    }


def sanitize_query(raw_query: str) -> str:
    """Collapses LLM-generated prose or comma-stuffed queries into compact, search-engine-safe terms."""
    cleaned = re.sub(r"\s+", " ", (raw_query or "").strip())
    # Strip trailing/inline list commas outside quotes so search engines don't match literal punctuation
    normalized = re.sub(r"(?<!\d)[,;]+(?=\s|$)", "", cleaned)
    words = normalized.split()
    if len(words) <= 6:
        return normalized
    quoted = re.findall(r'["\']([^"\']{2,50})["\']', normalized)
    tokens = [
        token.strip(".,;:()[]{}")
        for token in words
        if token.strip(".,;:()[]{}").lower() not in PROSE_STOP_WORDS and len(token.strip(".,;:()[]{}")) >= 3
    ]
    compact: List[str] = [f'"{phrase}"' for phrase in quoted[:2]]
    for token in tokens:
        if len(compact) >= 6:
            break
        if all(token.lower() not in existing.lower() for existing in compact):
            compact.append(token)
    return " ".join(compact) or " ".join(words[:6])


def anchor_terms(anchor: Optional[Dict[str, Any]], query: str = "") -> List[str]:
    """Extracts lowercase target identifiers (including compact no-space handle forms) for relevance filtering."""
    terms: List[str] = []
    if anchor:
        candidates = (
            [anchor.get("primary_name", "")]
            + list(anchor.get("known_aliases", []))
            + list(anchor.get("known_handles_or_domains", []))
        )
        for item in candidates:
            value = str(item or "").strip().lower().lstrip("@")
            if len(value) >= 3:
                terms.append(value)
                compact = re.sub(r"[\s_-]+", "", value)
                if len(compact) >= 6 and compact != value:
                    terms.append(compact)
    if not terms and query:
        stripped = re.sub(r"\bsite:\S+", "", query)
        for token in re.findall(r"[A-Za-z0-9_.-]{3,}", stripped):
            lowered = token.lower()
            if lowered not in PROSE_STOP_WORDS:
                terms.append(lowered)
    return list(dict.fromkeys(terms))


def matches_terms(text: str, terms: List[str], url: str = "") -> bool:
    if not terms:
        return True
    haystack = f"{url}\n{text}".lower()
    return any(term in haystack for term in terms)


class HtmlTextExtractor(HTMLParser):
    """Zero-dependency stdlib HTML text, metadata, outbound-link, and temporal-signal extractor."""

    SKIP_TAGS = {"script", "style", "noscript", "svg", "iframe", "template"}

    def __init__(self, base_url: str = "") -> None:
        super().__init__(convert_charrefs=True)
        self.base_url = base_url
        self.title = ""
        self.descriptions: List[str] = []
        self.json_ld: List[str] = []
        self.links: List[Dict[str, Any]] = []
        self.dates: List[str] = []
        self._chunks: List[str] = []
        self._skip_depth = 0
        self._in_title = False
        self._in_json_ld = False
        self._active_href = ""
        self._active_rel = ""
        self._active_label: List[str] = []

    def handle_starttag(self, tag: str, attrs: List[Tuple[str, Optional[str]]]) -> None:
        lower = tag.lower()
        attr_map = {key.lower(): (val or "") for key, val in attrs}
        if lower == "script" and "ld+json" in attr_map.get("type", "").lower():
            self._in_json_ld = True
            return
        if lower in self.SKIP_TAGS:
            self._skip_depth += 1
            return
        if lower == "title":
            self._in_title = True
        elif lower == "a" and self._skip_depth == 0:
            href = attr_map.get("href", "").strip()
            if href and not href.startswith(("#", "javascript:", "tel:", "mailto:")):
                if href.lower().startswith("www."):
                    href = f"https://{href}"
                self._active_href = urljoin(self.base_url, href) if self.base_url else href
                self._active_rel = attr_map.get("rel", "").lower()
                self._active_label = []
        elif lower == "time":
            dt_val = attr_map.get("datetime", "").strip()
            if dt_val:
                self.dates.append(dt_val)
        elif lower == "meta":
            name = (attr_map.get("name") or attr_map.get("property") or "").lower()
            content = attr_map.get("content", "").strip()
            if content and name in ("description", "og:title", "og:description", "twitter:description", "author"):
                self.descriptions.append(f"{name}: {content}")
            elif content and name in (
                "article:published_time", "article:modified_time", "og:updated_time", "date", "last-modified"
            ):
                self.dates.append(content)
        elif lower in ("p", "div", "br", "li", "h1", "h2", "h3", "h4", "tr", "section", "article"):
            self._chunks.append("\n")

    def handle_endtag(self, tag: str) -> None:
        lower = tag.lower()
        if lower == "script" and self._in_json_ld:
            self._in_json_ld = False
        elif lower in self.SKIP_TAGS and self._skip_depth > 0:
            self._skip_depth -= 1
        elif lower == "title":
            self._in_title = False
        elif lower == "a" and self._active_href:
            label = re.sub(r"\s+", " ", "".join(self._active_label)).strip()[:100]
            self.links.append({
                "url": canonicalize_url(self._active_href),
                "label": label,
                "rel_me": "me" in self._active_rel.split(),
            })
            self._active_href = ""
            self._active_rel = ""
            self._active_label = []

    def handle_data(self, data: str) -> None:
        if self._in_json_ld:
            self.json_ld.append(data)
        elif self._in_title:
            self.title += data
        elif self._skip_depth == 0 and data.strip():
            if self._active_href:
                self._active_label.append(data)
            self._chunks.append(data.strip() + " ")

    def rendered_text(self) -> str:
        header_lines: List[str] = []
        if self.title.strip():
            header_lines.append(f"# {self.title.strip()}")
        for meta in list(dict.fromkeys(self.descriptions))[:5]:
            header_lines.append(meta)
        body = re.sub(r"\n{3,}", "\n\n", "".join(self._chunks)).strip()
        return "\n".join(header_lines + (["", body] if body else [])).strip()


def extract_linked_urls(blob: str) -> List[Dict[str, Any]]:
    """Extracts profile/canonical URLs from a JSON-LD script block while ignoring templates and images."""
    found: List[Dict[str, Any]] = []

    def walk(node: Any, key_name: str = "") -> None:
        if isinstance(node, dict):
            if str(node.get("@type", "")).lower() in ("searchaction", "entrypoint", "imageobject"):
                return
            for k, v in node.items():
                if k.lower() in ("image", "logo", "thumbnailurl", "urltemplate"):
                    continue
                walk(v, k.lower())
        elif isinstance(node, list):
            for item in node:
                walk(item, key_name)
        elif isinstance(node, str) and node.startswith(("http://", "https://")) and "{" not in node:
            if key_name in ("sameas", "url", "mainentityofpage"):
                found.append({"url": canonicalize_url(node), "label": "json-ld", "rel_me": key_name == "sameas"})

    try:
        walk(json.loads(blob))
        return found
    except (json.JSONDecodeError, TypeError, ValueError):
        for candidate in re.findall(r'https?://[^\s"<>]+', blob):
            if "{" not in candidate and "%7b" not in candidate.lower():
                found.append({"url": canonicalize_url(candidate), "label": "json-ld", "rel_me": False})
        return found


def extract_html_bundle(raw_html: str, url: str) -> Dict[str, Any]:
    """Extracts clean text, outbound links, and temporal metadata from HTML."""
    extracted = ""
    if trafilatura is not None:
        try:
            extracted = trafilatura.extract(raw_html, output_format="markdown", include_links=True, url=url) or ""
        except Exception as error:
            logger.debug("[HTML] Trafilatura fallback on %s: %s", url, error)
    parser = HtmlTextExtractor(base_url=url)
    try:
        parser.feed(raw_html)
    except Exception:
        pass
    stdlib_text = parser.rendered_text()
    if len(extracted.strip()) >= MIN_ARTICLE_CHARACTERS:
        title_prefix = (
            f"# {parser.title.strip()}\n\n"
            if parser.title.strip() and parser.title.strip() not in extracted[:200]
            else ""
        )
        text = (title_prefix + extracted).strip()
    else:
        text = stdlib_text if len(stdlib_text) >= 60 else extracted.strip()

    # Extract JSON-LD dates and sameAs profile links
    dates = list(parser.dates)
    links = list(parser.links)
    for blob in parser.json_ld:
        for date_match in re.findall(r'"date(?:Published|Modified|Created)"\s*:\s*"([^"]{8,32})"', blob):
            dates.append(date_match)
        links.extend(extract_linked_urls(blob))

    # Deduplicate links while preserving order
    unique_links: List[Dict[str, Any]] = []
    seen_urls = set()
    for link in links:
        href = link["url"]
        if href.startswith(("http://", "https://")) and href not in seen_urls:
            seen_urls.add(href)
            unique_links.append(link)

    return {
        "text": text,
        "links": unique_links[:80],
        "dates": list(dict.fromkeys(dates))[:12],
    }


def is_gate_page(text: str, url: str = "") -> bool:
    """Detects cookie-consent walls and anti-bot interstitials so they are never treated as evidence."""
    host = urlparse(url).netloc.lower() if url else ""
    if host.startswith(("consent.youtube.", "consent.google.")):
        return True
    sample = (text or "")[:2000].lower()
    return any(marker in sample for marker in CONSENT_MARKERS)


def rank_link(url: str, label: str, source_url: str, anchor: Optional[Dict[str, Any]], rel_me: bool = False) -> int:
    """Scores discovered outbound links for the bounded `Links -> Open -> spy -> Data` loop (0 = skip)."""
    if "{" in url or "%7b" in url.lower():
        return 0
    parsed = urlparse(url)
    if parsed.scheme not in ("http", "https") or not parsed.netloc:
        return 0
    host = parsed.netloc.lower()
    if host.startswith("www."):
        host = host[4:]
    path = parsed.path.lower()
    if Path(path).suffix in ASSET_EXTENSIONS:
        return 0
    if any(host == skip or host.endswith(f".{skip}") for skip in SKIP_LINK_DOMAINS):
        return 0
    if any(bad in f"{path}?{parsed.query.lower()}" for bad in SKIP_LINK_SEGMENTS):
        return 0

    source_host = urlparse(source_url).netloc.lower().removeprefix("www.")
    terms = anchor_terms(anchor)
    score = 0

    # Priority 1: Independent corroboration platforms (ORCID, Credly, GitHub, X, Wikipedia, etc.)
    if any(host == platform or host.endswith(f".{platform}") for platform in CORROBORATION_DOMAINS):
        if len(path.strip("/")) >= 1:
            score = max(score, 96 if rel_me else 92)

    # Priority 2: High-value routes on same verified domain (/about, /resume before nested /projects/*)
    if host == source_host and source_host:
        segments = [seg for seg in path.strip("/").split("/") if seg]
        if any(path.startswith(prefix) or prefix in path for prefix in HIGH_VALUE_PATHS):
            score = max(score, 93 if len(segments) <= 1 else 84)
        elif path not in ("", "/") and len(segments) <= 2:
            score = max(score, 68)

    # Priority 3: Deployed apps, personal domains, or anchor-term matches
    if matches_terms(label, terms, url):
        score = max(score, 85)
    elif rel_me:
        score = max(score, 82)
    elif host != source_host and any(host.endswith(tld) for tld in (".app", ".dev", ".io", ".ai")):
        score = max(score, 76)
    elif label and any(word in label.lower() for word in ("project", "resume", "cv", "github", "orcid", "credly", "demo", "live")):
        score = max(score, 78)

    return score


def classify_reliability(url: str, method: str, anchor: Optional[Dict[str, Any]] = None) -> Tuple[str, str]:
    """Deterministic NATO Admiralty source-reliability (A-F) and credibility baseline (1-6)."""
    if method == "search_snippet":
        return ("E", "5")
    if method in ("github_api", "github_repo_api", "orcid_api", "wikipedia_api"):
        return ("A", "2")
    domain = urlparse(url).netloc.lower()
    if domain.startswith("www."):
        domain = domain[4:]
    if any(domain.endswith(suffix) for suffix in AUTHORITATIVE_SUFFIXES):
        return ("A", "2")
    if anchor:
        compact = re.sub(r"[\s_-]+", "", str(anchor.get("primary_name", "")).lower())
        if len(compact) >= 6 and compact in domain:
            return ("B", "2")
    if any(domain == known or domain.endswith(f".{known}") for known in REPUTABLE_DOMAINS):
        return ("B", "2")
    if method in ("markdown", "native_document", "html_text"):
        return ("C", "3")
    return ("D", "4")


# ==============================================================================
# 2. POLITENESS GATE (RFC 9309 robots.txt + per-domain rate limit)
# ==============================================================================
class PolitenessGate:
    def __init__(self) -> None:
        self._robots: Dict[str, Optional[RobotFileParser]] = {}
        self._last_hit: Dict[str, float] = {}
        self._lock = threading.Lock()

    @staticmethod
    def _load_robots(root: str) -> Optional[RobotFileParser]:
        parser = RobotFileParser()
        try:
            response = requests.get(f"{root}/robots.txt", timeout=4, headers={"User-Agent": USER_AGENT})
        except requests.RequestException:
            parser.disallow_all = True
            return parser
        if response.status_code < 300:
            parser.parse(response.text.splitlines())
            return parser
        if response.status_code < 500:
            return None
        parser.disallow_all = True
        return parser

    def allowed(self, url: str) -> bool:
        parsed = urlparse(url)
        if parsed.scheme not in ("http", "https") or not parsed.netloc:
            return False
        root = f"{parsed.scheme}://{parsed.netloc}"
        with self._lock:
            cached = root in self._robots
            parser = self._robots.get(root)
        if not cached:
            parser = self._load_robots(root)
            with self._lock:
                self._robots[root] = parser
        return parser.can_fetch(USER_AGENT, url) if parser else True

    def throttle(self, url: str) -> None:
        domain = urlparse(url).netloc.lower()
        with self._lock:
            now = time.monotonic()
            slot = max(now, self._last_hit.get(domain, 0.0) + DOMAIN_INTERVAL_SECONDS)
            self._last_hit[domain] = slot
        if slot > now:
            time.sleep(slot - now)


# ==============================================================================
# 3. DOCUMENT EXTRACTOR (pure Python + optional C DLL plugin)
# ==============================================================================
class DocumentExtractor:
    """Extracts text from .txt/.md/.json/.csv/.docx/.pdf with optional native DLL acceleration."""

    def __init__(self, dll_path: Optional[Path] = None) -> None:
        self.dll = None
        self.has_probe = False
        if not dll_path or not dll_path.exists():
            return
        try:
            os.add_dll_directory(str(dll_path.parent.resolve()))
        except (OSError, AttributeError):
            pass
        try:
            self.dll = ctypes.CDLL(str(dll_path.resolve()))
            self.dll.axiom_extract_document.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
            self.dll.axiom_extract_document.restype = ctypes.c_int
            if hasattr(self.dll, "axiom_required_size"):
                self.dll.axiom_required_size.argtypes = [ctypes.c_char_p]
                self.dll.axiom_required_size.restype = ctypes.c_int
                self.has_probe = True
            logger.info("Loaded optional native extractor DLL: %s", dll_path.name)
        except (OSError, AttributeError) as error:
            self.dll = None
            logger.warning("Optional native DLL skipped (%s): %s", dll_path, error)

    def _extract_native(self, path: Path) -> str:
        if not self.dll:
            return ""
        encoded = str(path).encode("utf-8")
        capacity = 8 * 1024 * 1024
        if self.has_probe:
            required = self.dll.axiom_required_size(encoded)
            if required <= 0:
                return ""
            capacity = required + 1
        buffer = ctypes.create_string_buffer(capacity)
        bytes_read = self.dll.axiom_extract_document(encoded, buffer, capacity)
        return buffer.value.decode("utf-8", errors="replace") if bytes_read > 0 else ""

    @staticmethod
    def _extract_python(path: Path) -> str:
        extension = path.suffix.lower()
        try:
            if extension in (".txt", ".md", ".json", ".csv", ".xml"):
                return path.read_text(encoding="utf-8", errors="replace")
            if extension == ".docx":
                with zipfile.ZipFile(path) as archive:
                    xml_text = archive.read("word/document.xml").decode("utf-8", errors="replace")
                return html.unescape(re.sub(r"<[^>]+>", "", xml_text.replace("</w:p>", "\n")))
            if extension == ".pdf":
                from pypdf import PdfReader
                return "\n".join(page.extract_text() or "" for page in PdfReader(str(path)).pages)
        except ImportError:
            logger.debug("pypdf not installed; skipping PDF extraction for %s", path.name)
        except Exception as error:
            logger.debug("Document extraction failed for %s: %s", path.name, error)
        return ""

    def extract(self, path: Path) -> str:
        return (self._extract_native(path) or self._extract_python(path)).strip()


# ==============================================================================
# 4. OPTIONAL UNIVERSAL LLM CLIENT (disabled by default -> zero VRAM conflict)
# ==============================================================================
class LanguageModelClient:
    """
    Provider-agnostic optional LLM client.
    - SPECTER_LLM_PROVIDER=none   (default): disabled; host MCP LLM drives reasoning.
    - SPECTER_LLM_PROVIDER=openai : any OpenAI-compatible /v1/chat/completions API.
    - SPECTER_LLM_PROVIDER=ollama : local Ollama /api/generate (auto-reuses resident model).
    """

    def __init__(self) -> None:
        self.provider = LLM_PROVIDER
        self.base_url = LLM_URL or ("http://127.0.0.1:11434" if self.provider == "ollama" else "https://api.openai.com/v1")
        self.model = LLM_MODEL
        self.api_key = LLM_API_KEY

    @property
    def enabled(self) -> bool:
        return self.provider in ("openai", "ollama")

    def _resolve_ollama_model(self) -> str:
        if self.model:
            return self.model
        try:
            response = requests.get(f"{self.base_url}/api/ps", timeout=3)
            if response.ok:
                running = [entry.get("name", "") for entry in response.json().get("models", []) if entry.get("name")]
                if running:
                    return running[0]
            tags = requests.get(f"{self.base_url}/api/tags", timeout=3)
            if tags.ok:
                models = [entry.get("name", "") for entry in tags.json().get("models", []) if entry.get("name")]
                if models:
                    return models[0]
        except (requests.RequestException, ValueError):
            pass
        return ""

    def complete(self, prompt: str, json_mode: bool = False, timeout_seconds: int = 35) -> str:
        if not self.enabled:
            return ""
        if self.provider == "openai":
            if not self.model:
                raise RuntimeError("SPECTER_LLM_MODEL must be set when SPECTER_LLM_PROVIDER=openai.")
            headers = {"Content-Type": "application/json"}
            if self.api_key:
                headers["Authorization"] = f"Bearer {self.api_key}"
            payload: Dict[str, Any] = {
                "model": self.model,
                "messages": [{"role": "user", "content": prompt}],
                "temperature": 0.2,
            }
            if json_mode:
                payload["response_format"] = {"type": "json_object"}
            response = requests.post(
                f"{self.base_url}/chat/completions", json=payload, headers=headers, timeout=(5, timeout_seconds)
            )
            response.raise_for_status()
            choices = response.json().get("choices", [])
            message_text = choices[0].get("message", {}).get("content") or "" if choices else ""
            return strip_reasoning(message_text)

        model = self._resolve_ollama_model()
        if not model:
            raise RuntimeError(f"No Ollama model available at {self.base_url}.")
        payload = {
            "model": model,
            "prompt": prompt,
            "stream": False,
            "keep_alive": "15m",
            "options": {"temperature": 0.2, "num_ctx": LLM_CONTEXT_TOKENS},
        }
        if json_mode:
            payload["format"] = "json"
        response = requests.post(f"{self.base_url}/api/generate", json=payload, timeout=(5, timeout_seconds))
        response.raise_for_status()
        return strip_reasoning(response.json().get("response", ""))


# ==============================================================================
# 5. CASE STORE (SQLite knowledge graph + chain of custody + claim grounding)
# ==============================================================================
class CaseStore:
    def __init__(self, case_id: str) -> None:
        self.case_id = validate_case_id(case_id)
        self.directory = CASES_DIRECTORY / self.case_id
        self.evidence_directory = self.directory / "evidence"
        self.anchor_path = self.directory / "anchor.json"
        self.dossier_path = self.directory / "dossier.md"
        self.database_path = self.directory / "case.db"
        self.evidence_directory.mkdir(parents=True, exist_ok=True)
        self._initialize_schema()

    def _connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(self.database_path, timeout=30)
        connection.row_factory = sqlite3.Row
        return connection

    def _initialize_schema(self) -> None:
        with closing(self._connect()) as connection, connection:
            connection.execute("PRAGMA journal_mode=WAL")
            connection.executescript(
                """
                CREATE TABLE IF NOT EXISTS evidence (
                    content_hash TEXT PRIMARY KEY, url TEXT NOT NULL, domain TEXT, method TEXT NOT NULL,
                    query TEXT, collected_utc TEXT NOT NULL, artifact_path TEXT, content TEXT NOT NULL);
                CREATE TABLE IF NOT EXISTS queries (
                    query TEXT PRIMARY KEY, executed_utc TEXT NOT NULL, new_artifacts INTEGER DEFAULT 0);
                CREATE TABLE IF NOT EXISTS hypotheses (
                    hypothesis_id TEXT PRIMARY KEY, label TEXT, description TEXT, status TEXT, diagnostic_score REAL);
                CREATE TABLE IF NOT EXISTS claims (
                    claim_id INTEGER PRIMARY KEY AUTOINCREMENT, category TEXT, statement TEXT, evidence_tag TEXT,
                    source_url TEXT, source_reliability TEXT, info_credibility TEXT, hypothesis_alignment TEXT);
                CREATE TABLE IF NOT EXISTS entities (
                    name TEXT NOT NULL, entity_type TEXT NOT NULL, relation TEXT, PRIMARY KEY (name, entity_type));
                CREATE TABLE IF NOT EXISTS connections (
                    source_node TEXT NOT NULL, target_node TEXT NOT NULL, relation TEXT NOT NULL,
                    direction TEXT NOT NULL, confidence REAL DEFAULT 0.5, verified_at TEXT NOT NULL,
                    sources TEXT NOT NULL, PRIMARY KEY (source_node, target_node, relation));
                CREATE TABLE IF NOT EXISTS links (
                    url TEXT PRIMARY KEY, source_url TEXT NOT NULL, label TEXT, depth INTEGER NOT NULL,
                    priority INTEGER NOT NULL, status TEXT NOT NULL, discovered_utc TEXT NOT NULL);
                CREATE TABLE IF NOT EXISTS analysis_meta (key TEXT PRIMARY KEY, value TEXT);
                CREATE TABLE IF NOT EXISTS audit (
                    audit_id INTEGER PRIMARY KEY AUTOINCREMENT, utc TEXT NOT NULL, action TEXT NOT NULL, detail TEXT);
                """
            )

    def has_url(self, url: str) -> bool:
        clean_url = canonicalize_url(url)
        with closing(self._connect()) as connection:
            row = connection.execute("SELECT 1 FROM evidence WHERE url = ? LIMIT 1", (clean_url,)).fetchone()
            return row is not None

    def evidence_tag_for(self, url: str) -> str:
        clean_url = canonicalize_url(url)
        with closing(self._connect()) as connection:
            row = connection.execute("SELECT content_hash FROM evidence WHERE url = ? LIMIT 1", (clean_url,)).fetchone()
            return f"E:{row['content_hash'][:10]}" if row else ""

    def queue_links(
        self,
        discovered: List[Dict[str, Any]],
        source_url: str,
        depth: int,
        anchor: Optional[Dict[str, Any]],
    ) -> int:
        """Scores and queues unseen outbound links for bounded `Links -> Open -> spy -> Data` traversal."""
        if depth > MAX_TRAVERSAL_DEPTH or not discovered:
            return 0
        added = 0
        now = utc_now()
        with closing(self._connect()) as connection, connection:
            for item in discovered:
                url = canonicalize_url(str(item.get("url", "")))
                if not url or url == source_url:
                    continue
                label = str(item.get("label", ""))[:100]
                rel_me = bool(item.get("rel_me", False))
                priority = rank_link(url, label, source_url, anchor, rel_me=rel_me)
                if priority <= 0:
                    continue
                cursor = connection.execute(
                    "INSERT OR IGNORE INTO links VALUES (?, ?, ?, ?, ?, 'queued', ?)",
                    (url, source_url, label, depth, priority, now),
                )
                if cursor.rowcount > 0:
                    added += 1
        return added

    def next_queued_links(self, limit: int, max_depth: int) -> List[Dict[str, Any]]:
        with closing(self._connect()) as connection:
            rows = connection.execute(
                "SELECT url, source_url, label, depth, priority FROM links "
                "WHERE status = 'queued' AND depth <= ? ORDER BY priority DESC, depth ASC, rowid ASC LIMIT ?",
                (max_depth, limit),
            ).fetchall()
            return [dict(row) for row in rows]

    def mark_link(self, url: str, status: str) -> None:
        with closing(self._connect()) as connection, connection:
            connection.execute("UPDATE links SET status = ? WHERE url = ?", (status, url))

    def add_connection(
        self,
        source_node: str,
        target_node: str,
        relation: str = "links_to",
        direction: str = "out",
        confidence: float = 0.65,
        evidence_tag: str = "",
    ) -> str:
        """
        Records a directional edge and automatically promotes A -> B + B -> A into a single
        `two_directional` (bidirectional) edge without duplicate conflicting records.
        """
        left = (source_node or "").strip()[:180]
        right = (target_node or "").strip()[:180]
        rel = (relation or "associated_with").strip()[:60]
        if not left or not right or left.lower() == right.lower():
            return "ignored"
        new_sources = [evidence_tag] if evidence_tag else []
        score = round(min(0.98, max(0.05, float(confidence))), 2)
        now = utc_now()

        with closing(self._connect()) as connection, connection:
            # Check if an existing bidirectional edge already covers this pair
            existing_bi = connection.execute(
                "SELECT source_node, target_node, relation, confidence, sources FROM connections "
                "WHERE direction = 'bidirectional' AND ("
                "(lower(source_node) = lower(?) AND lower(target_node) = lower(?)) OR "
                "(lower(source_node) = lower(?) AND lower(target_node) = lower(?))) LIMIT 1",
                (left, right, right, left),
            ).fetchone()
            if existing_bi:
                merged_sources = clean_strings(json.loads(existing_bi["sources"]) + new_sources, 15)
                merged_score = round(max(float(existing_bi["confidence"]), score), 2)
                connection.execute(
                    "UPDATE connections SET confidence = ?, verified_at = ?, sources = ? "
                    "WHERE source_node = ? AND target_node = ? AND relation = ?",
                    (merged_score, now, json.dumps(merged_sources),
                     existing_bi["source_node"], existing_bi["target_node"], existing_bi["relation"]),
                )
                return "two_directional"

            # Check if reverse one-directional edge (B -> A) exists -> PROMOTE to bidirectional!
            reverse = connection.execute(
                "SELECT source_node, target_node, relation, confidence, sources FROM connections "
                "WHERE direction = 'out' AND lower(source_node) = lower(?) AND lower(target_node) = lower(?) LIMIT 1",
                (right, left),
            ).fetchone()
            if reverse or direction == "bidirectional":
                prev_sources = json.loads(reverse["sources"]) if reverse else []
                prev_score = float(reverse["confidence"]) if reverse else score
                if reverse:
                    connection.execute(
                        "DELETE FROM connections WHERE source_node = ? AND target_node = ? AND relation = ?",
                        (reverse["source_node"], reverse["target_node"], reverse["relation"]),
                    )
                merged_sources = clean_strings(prev_sources + new_sources, 15)
                promoted_score = round(min(0.95, max(prev_score, score) + (0.15 if reverse else 0.0)), 2)
                canonical_pair = sorted([left, right], key=str.lower)
                mutual_rel = rel if rel.startswith("mutual_") else f"mutual_{rel}"
                connection.execute(
                    "INSERT OR REPLACE INTO connections VALUES (?, ?, ?, 'bidirectional', ?, ?, ?)",
                    (canonical_pair[0], canonical_pair[1], mutual_rel, promoted_score, now, json.dumps(merged_sources)),
                )
                return "promoted_two_directional"

            # Upsert one-directional edge (A -> B)
            existing = connection.execute(
                "SELECT confidence, sources FROM connections "
                "WHERE source_node = ? AND target_node = ? AND relation = ?",
                (left, right, rel),
            ).fetchone()
            merged_sources = clean_strings((json.loads(existing["sources"]) if existing else []) + new_sources, 15)
            merged_score = round(max(float(existing["confidence"]) if existing else 0.0, score), 2)
            connection.execute(
                "INSERT OR REPLACE INTO connections VALUES (?, ?, ?, 'out', ?, ?, ?)",
                (left, right, rel, merged_score, now, json.dumps(merged_sources)),
            )
            return "one_directional"

    def connection_graph(self, anchor: Dict[str, Any]) -> Dict[str, Any]:
        """Exports Page B's exact directional connection schema (one_directional vs. two_directional)."""
        one_way: List[Dict[str, Any]] = []
        two_way: List[Dict[str, Any]] = []
        with closing(self._connect()) as connection:
            rows = connection.execute(
                "SELECT source_node, target_node, relation, direction, confidence, verified_at, sources "
                "FROM connections ORDER BY confidence DESC, source_node ASC"
            ).fetchall()
        for row in rows:
            sources = json.loads(row["sources"])
            if row["direction"] == "bidirectional":
                two_way.append({
                    "pair": [row["source_node"], row["target_node"]],
                    "relation": row["relation"],
                    "direction": "bidirectional",
                    "confidence": row["confidence"],
                    "verified_at": row["verified_at"],
                    "sources": sources,
                })
            else:
                one_way.append({
                    "from": row["source_node"],
                    "to": row["target_node"],
                    "relation": row["relation"],
                    "direction": "out",
                    "confidence": row["confidence"],
                    "verified_at": row["verified_at"],
                    "sources": sources,
                })
        hints = anchor.get("hints") or {
            "emails": [],
            "phones": [],
            "handles": anchor.get("known_aliases", []),
            "domains": anchor.get("known_handles_or_domains", []),
        }
        return {
            "target": {
                "id": self.case_id,
                "label": anchor.get("primary_name", ""),
                "hints": hints,
            },
            "connections": {
                "one_directional": one_way,
                "two_directional": two_way,
            },
        }

    def add_evidence(self, url: str, content: str, method: str, query: str = "", artifact_path: str = "") -> bool:
        cleaned = (content or "").strip()[:MAX_STORED_CHARACTERS]
        if not cleaned:
            return False
        clean_url = canonicalize_url(url)
        with closing(self._connect()) as connection, connection:
            cursor = connection.execute(
                "INSERT OR IGNORE INTO evidence VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                (sha256_text(cleaned), clean_url, urlparse(clean_url).netloc.lower(), method, query, utc_now(), artifact_path, cleaned),
            )
            inserted = cursor.rowcount > 0
        if not inserted and artifact_path:
            candidate = Path(artifact_path)
            if candidate.parent == self.evidence_directory and candidate.exists():
                candidate.unlink(missing_ok=True)
        return inserted

    def evidence_rows(self) -> List[Dict[str, Any]]:
        with closing(self._connect()) as connection:
            return [dict(row) for row in connection.execute("SELECT * FROM evidence ORDER BY collected_utc, rowid")]

    def search_evidence(self, keyword: str, limit: int) -> List[Dict[str, Any]]:
        escaped = keyword.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_")
        pattern = f"%{escaped}%"
        with closing(self._connect()) as connection:
            rows = connection.execute(
                "SELECT content_hash, url, method, collected_utc, content FROM evidence "
                "WHERE content LIKE ? ESCAPE '\\' OR url LIKE ? ESCAPE '\\' LIMIT ?",
                (pattern, pattern, limit),
            ).fetchall()
        results = []
        for row in rows:
            content = row["content"]
            position = content.lower().find(keyword.lower())
            start = max(0, position - 200) if position >= 0 else 0
            results.append({
                "evidence_tag": f"E:{row['content_hash'][:10]}",
                "url": row["url"],
                "method": row["method"],
                "collected_utc": row["collected_utc"],
                "excerpt": content[start : start + 500],
            })
        return results

    def evidence_excerpts(self, anchor: Optional[Dict[str, Any]], limit: int = 8, excerpt_chars: int = 800) -> List[Dict[str, Any]]:
        """Returns compact, relevance-ranked primary evidence excerpts for ANY host LLM."""
        rows = self.evidence_rows()
        terms = anchor_terms(anchor)
        ranked = sorted(
            rows,
            key=lambda row: (
                0 if row["method"] == "search_snippet" else 1,
                sum(1 for term in terms if term in f"{row['url']}\n{row['content']}".lower()),
                len(row["content"]),
            ),
            reverse=True,
        )
        excerpts = []
        for row in ranked[:limit]:
            reliability, credibility = classify_reliability(row["url"], row["method"], anchor)
            excerpts.append({
                "evidence_tag": f"E:{row['content_hash'][:10]}",
                "rating": f"{reliability}{credibility}",
                "method": row["method"],
                "url": row["url"],
                "collected_utc": row["collected_utc"],
                "excerpt": redact_sensitive(row["content"][:excerpt_chars]),
            })
        return excerpts

    def executed_queries(self) -> set:
        with closing(self._connect()) as connection:
            return {row["query"].lower() for row in connection.execute("SELECT query FROM queries")}

    def record_query(self, query: str, new_artifacts: int) -> None:
        with closing(self._connect()) as connection, connection:
            connection.execute("INSERT OR REPLACE INTO queries VALUES (?, ?, ?)", (query, utc_now(), new_artifacts))

    def ground_analysis(self, analysis: Dict[str, Any], anchor: Optional[Dict[str, Any]]) -> Dict[str, Any]:
        """Server-side anti-hallucination guard: every claim must map to target-matching evidence."""
        rows = self.evidence_rows()
        evidence_by_tag = {f"E:{row['content_hash'][:10]}": row for row in rows}
        terms = anchor_terms(anchor)
        matched_rows = [
            row for row in rows
            if matches_terms(row["content"], terms, row["url"]) or str(row.get("query", "")).startswith(("link_crawl:", "github:"))
        ]
        primary_domains = {row["domain"] for row in matched_rows if row["method"] != "search_snippet"}

        try:
            confidence = float(analysis.get("confidence", 0.0) or 0.0)
        except (TypeError, ValueError):
            confidence = 0.0

        verified_claims: List[Dict[str, Any]] = []
        dropped = 0
        for claim in analysis.get("claims", []):
            if not isinstance(claim, dict) or not str(claim.get("statement", "")).strip():
                continue
            tag = str(claim.get("evidence_tag", "")).strip()
            row = evidence_by_tag.get(tag)
            is_verified_source = bool(
                row and (
                    matches_terms(row["content"], terms, row["url"])
                    or str(row.get("query", "")).startswith(("link_crawl:", "github:"))
                )
            )
            if not is_verified_source:
                dropped += 1
                continue
            verified_claims.append({
                **claim,
                "source_url": str(claim.get("source_url") or row["url"]),
            })

        gaps = clean_strings(analysis.get("intelligence_gaps"), 12)
        if not matched_rows:
            confidence = min(confidence, 0.10)
            notice = "Zero collected evidence artifacts mention the target identifiers."
            if notice not in gaps:
                gaps.insert(0, notice)
        elif not primary_domains:
            confidence = min(confidence, 0.30)
        elif len(primary_domains) == 1:
            confidence = min(confidence, 0.58)
        elif len(primary_domains) == 2:
            has_third_party_auth = any(
                classify_reliability(row["url"], row["method"], anchor)[0] == "A"
                and row["method"] != "github_api"
                for row in matched_rows
                if row["method"] != "search_snippet"
            )
            if not has_third_party_auth:
                confidence = min(confidence, 0.74)
        if dropped:
            gaps.append(f"Dropped {dropped} ungrounded claim(s) lacking matching evidence citation.")

        return {
            **analysis,
            "confidence": round(min(0.95, max(0.0, confidence)), 2),
            "claims": verified_claims,
            "intelligence_gaps": gaps[:12],
        }

    def link_summary(self) -> Dict[str, Any]:
        with closing(self._connect()) as connection:
            queued_rows = connection.execute(
                "SELECT url, priority FROM links WHERE status = 'queued' ORDER BY priority DESC, depth ASC, rowid ASC"
            ).fetchall()
            blocked_rows = connection.execute(
                "SELECT url, status FROM links WHERE status IN ('access_blocked', 'robots_denied')"
            ).fetchall()
        return {
            "queued_count": len(queued_rows),
            "top_queued": [row["url"] for row in queued_rows[:4]],
            "blocked": [{"url": row["url"], "status": row["status"]} for row in blocked_rows[:10]],
        }

    def replace_analysis(self, analysis: Dict[str, Any], pass_number: int, anchor: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        prior = self.analysis_state()
        prior_stats = self.statistics()
        prior_rows = self.evidence_rows()
        prior_primary = sorted({r["domain"] for r in prior_rows if r["domain"] and r["method"] != "search_snippet"})

        grounded = self.ground_analysis(analysis, anchor)
        hypotheses = [item for item in grounded.get("hypotheses", []) if isinstance(item, dict)]
        claims = [item for item in grounded.get("claims", []) if isinstance(item, dict)]
        entities = [item for item in grounded.get("entities", []) if isinstance(item, dict) and item.get("name")]
        confidence = float(grounded.get("confidence", 0.0) or 0.0)

        current_primary = sorted({r["domain"] for r in prior_rows if r["domain"] and r["method"] != "search_snippet"})
        new_domains = [d for d in current_primary if d not in prior_primary]
        delta_vs_previous: Dict[str, Any] = {}
        if prior.get("analyzed_utc"):
            prev_artifacts = int(prior.get("last_evidence_total", prior_stats["evidence_total"]))
            prev_mutual = int(prior.get("last_mutual_edges", prior_stats["connections_mutual"]))
            prev_domains = prior.get("last_primary_domains", prior_primary)
            prev_gaps = set(prior.get("intelligence_gaps", []))
            curr_gaps = clean_strings(grounded.get("intelligence_gaps"), 12)
            delta_vs_previous = {
                "previous_confidence": prior["confidence"],
                "current_confidence": round(confidence, 2),
                "confidence_delta": round(confidence - prior["confidence"], 2),
                "new_artifacts_delta": prior_stats["evidence_total"] - prev_artifacts,
                "new_primary_domains": [d for d in current_primary if d not in prev_domains],
                "mutual_edges_delta": prior_stats["connections_mutual"] - prev_mutual,
                "resolved_gaps": sorted(prev_gaps - set(curr_gaps)),
                "remaining_gaps_count": len(curr_gaps),
            }

        with closing(self._connect()) as connection, connection:
            connection.execute("DELETE FROM hypotheses")
            connection.execute("DELETE FROM claims")
            connection.execute("DELETE FROM entities")
            for index, hypothesis in enumerate(hypotheses, 1):
                try:
                    score = float(hypothesis.get("diagnostic_score", 0.5))
                except (TypeError, ValueError):
                    score = 0.5
                connection.execute(
                    "INSERT OR REPLACE INTO hypotheses VALUES (?, ?, ?, ?, ?)",
                    (str(hypothesis.get("id", f"H{index}")), str(hypothesis.get("label", "")),
                     str(hypothesis.get("description", "")), str(hypothesis.get("status", "ACTIVE")), score),
                )
            for claim in claims:
                connection.execute(
                    "INSERT INTO claims (category, statement, evidence_tag, source_url, source_reliability, "
                    "info_credibility, hypothesis_alignment) VALUES (?, ?, ?, ?, ?, ?, ?)",
                    (str(claim.get("category", "general")), str(claim.get("statement", "")),
                     str(claim.get("evidence_tag", "")), str(claim.get("source_url", "")),
                     str(claim.get("source_reliability", "F"))[:1].upper(),
                     str(claim.get("info_credibility", "6"))[:1],
                     str(claim.get("hypothesis_alignment", "AMBIGUOUS"))),
                )
            for entity in entities:
                connection.execute(
                    "INSERT OR REPLACE INTO entities VALUES (?, ?, ?)",
                    (str(entity["name"])[:160], str(entity.get("type", "unknown"))[:40], str(entity.get("relation", ""))[:200]),
                )
            meta = {
                "confidence": f"{confidence:.2f}",
                "intelligence_gaps": json.dumps(clean_strings(grounded.get("intelligence_gaps"), 12)),
                "key_assumptions": json.dumps(clean_strings(grounded.get("key_assumptions"), 12)),
                "recommended_pivots": json.dumps(clean_strings(grounded.get("pivot_queries"), 5)),
                "passes_completed": str(max(pass_number, prior.get("passes_completed", 0))),
                "analyzed_utc": utc_now(),
                "last_evidence_total": str(prior_stats["evidence_total"]),
                "last_mutual_edges": str(prior_stats["connections_mutual"]),
                "last_primary_domains": json.dumps(current_primary),
                "delta_vs_previous": json.dumps(delta_vs_previous),
            }
            connection.executemany("INSERT OR REPLACE INTO analysis_meta VALUES (?, ?)", meta.items())
        return self.analysis_state()

    def analysis_state(self) -> Dict[str, Any]:
        with closing(self._connect()) as connection:
            meta = {row["key"]: row["value"] for row in connection.execute("SELECT * FROM analysis_meta")}
            return {
                "confidence": float(meta.get("confidence", 0.0)),
                "passes_completed": int(meta.get("passes_completed", 0)),
                "analyzed_utc": meta.get("analyzed_utc"),
                "intelligence_gaps": json.loads(meta.get("intelligence_gaps", "[]")),
                "key_assumptions": json.loads(meta.get("key_assumptions", "[]")),
                "recommended_pivots": json.loads(meta.get("recommended_pivots", "[]")),
                "last_evidence_total": int(meta.get("last_evidence_total", 0)),
                "last_mutual_edges": int(meta.get("last_mutual_edges", 0)),
                "last_primary_domains": json.loads(meta.get("last_primary_domains", "[]")),
                "delta_vs_previous": json.loads(meta.get("delta_vs_previous", "{}")),
                "hypotheses": [dict(row) for row in connection.execute("SELECT * FROM hypotheses")],
                "claims": [dict(row) for row in connection.execute("SELECT * FROM claims")],
                "entities": [dict(row) for row in connection.execute("SELECT * FROM entities")],
            }

    def audit(self, action: str, detail: Any = "") -> None:
        text = detail if isinstance(detail, str) else json.dumps(detail, ensure_ascii=False)
        with closing(self._connect()) as connection, connection:
            connection.execute("INSERT INTO audit (utc, action, detail) VALUES (?, ?, ?)", (utc_now(), action, text))

    def audit_log(self, limit: int = 200) -> List[Dict[str, Any]]:
        with closing(self._connect()) as connection:
            rows = connection.execute("SELECT utc, action, detail FROM audit ORDER BY audit_id DESC LIMIT ?", (limit,))
            return [dict(row) for row in rows]

    def statistics(self) -> Dict[str, Any]:
        with closing(self._connect()) as connection:
            by_method = {row[0]: row[1] for row in connection.execute("SELECT method, COUNT(*) FROM evidence GROUP BY method")}
            conn_counts = {row[0]: row[1] for row in connection.execute("SELECT direction, COUNT(*) FROM connections GROUP BY direction")}
            link_counts = {row[0]: row[1] for row in connection.execute("SELECT status, COUNT(*) FROM links GROUP BY status")}
            return {
                "evidence_total": sum(by_method.values()),
                "evidence_by_method": by_method,
                "unique_domains": connection.execute("SELECT COUNT(DISTINCT domain) FROM evidence").fetchone()[0],
                "queries_executed": connection.execute("SELECT COUNT(*) FROM queries").fetchone()[0],
                "claims": connection.execute("SELECT COUNT(*) FROM claims").fetchone()[0],
                "entities": connection.execute("SELECT COUNT(*) FROM entities").fetchone()[0],
                "connections_one_way": conn_counts.get("out", 0),
                "connections_mutual": conn_counts.get("bidirectional", 0),
                "links_visited": link_counts.get("visited", 0),
                "links_queued": link_counts.get("queued", 0),
            }


def load_anchor(case_id: str) -> Dict[str, Any]:
    anchor_path = CASES_DIRECTORY / validate_case_id(case_id) / "anchor.json"
    if not anchor_path.exists():
        raise FileNotFoundError(f"No case '{case_id}'. Run anchor_target first.")
    return json.loads(anchor_path.read_text(encoding="utf-8"))


def open_case(case_id: str) -> Tuple[Dict[str, Any], CaseStore]:
    anchor = load_anchor(case_id)
    return anchor, CaseStore(case_id)


# ==============================================================================
# 6. SOURCE COLLECTOR (web search + public GitHub/Wikipedia APIs + relevance gate)
# ==============================================================================
class SourceCollector:
    def __init__(self, gate: PolitenessGate, extractor: DocumentExtractor) -> None:
        self.gate = gate
        self.extractor = extractor

    @staticmethod
    def search_web(query: str, max_results: int) -> List[Dict[str, str]]:
        if DDGS is None:
            return []
        clean_query = sanitize_query(query)
        unquoted = clean_query.replace('"', "").strip()
        attempts = [unquoted] if unquoted else []
        if clean_query and clean_query != unquoted:
            attempts.append(clean_query)
        words = unquoted.split()
        if len(words) > 3:
            attempts.append(" ".join(words[:3]))

        for candidate in dict.fromkeys(attempts):
            hits: List[Dict[str, str]] = []
            seen = set()
            try:
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore", RuntimeWarning)
                    with DDGS() as search_client:
                        raw_items: List[Dict[str, Any]] = []
                        probes = [lambda: search_client.text(candidate, max_results=max_results)]
                        if hasattr(search_client, "_text_html"):
                            probes.append(lambda: search_client._text_html(candidate, None, None, max_results))
                        else:
                            probes.append(lambda: search_client.text(candidate, backend="html", max_results=max_results))
                        for probe in probes:
                            try:
                                raw_items = probe() or []
                            except Exception:
                                raw_items = []
                            if raw_items:
                                break
                        for item in raw_items:
                            canonical = canonicalize_url(item.get("href", ""))
                            if canonical and canonical not in seen:
                                seen.add(canonical)
                                title = str(item.get("title", "")).strip()
                                body = str(item.get("body", "")).strip()
                                hits.append({"url": canonical, "snippet": f"{title}\n{body}".strip()})
            except Exception as error:
                logger.warning("[Search] '%s' failed: %s", candidate, error)
            if hits:
                return hits
        return []

    def fetch_github_profile(
        self,
        login: str,
        store: CaseStore,
        anchor: Dict[str, Any],
        source_url: str = "",
        depth: int = 0,
    ) -> Dict[str, int]:
        """Fetches GitHub user profile, recent repos (with homepages/pushed_at), and recent public events."""
        counts = {"github_hits": 0, "github_blogs": 0, "links_queued": 0}
        headers = {"User-Agent": USER_AGENT, "Accept": "application/json"}
        terms = anchor_terms(anchor)
        try:
            profile = requests.get(f"https://api.github.com/users/{login}", headers=headers, timeout=5)
            if not profile.ok:
                return counts
            data = profile.json()
        except (requests.RequestException, ValueError):
            return counts

        repo_lines: List[str] = []
        discovered_links: List[Dict[str, Any]] = []
        latest_activity = str(data.get("updated_at") or "")
        try:
            repos_resp = requests.get(
                f"https://api.github.com/users/{login}/repos",
                params={"sort": "pushed", "per_page": 10},
                headers=headers,
                timeout=5,
            )
            if repos_resp.ok:
                for repo in repos_resp.json()[:10]:
                    pushed = str(repo.get("pushed_at") or "")
                    if pushed and pushed > latest_activity:
                        latest_activity = pushed
                    is_fork = bool(repo.get("fork"))
                    homepage = str(repo.get("homepage") or "").strip()
                    # Forked repos inherit upstream project homepages unrelated to the target
                    if homepage and not is_fork:
                        norm_hp = canonicalize_url(homepage if homepage.startswith(("http://", "https://")) else f"https://{homepage}")
                        discovered_links.append({"url": norm_hp, "label": f"project:{repo.get('name')}", "rel_me": True})
                    fork_note = "fork, " if is_fork else ""
                    repo_lines.append(
                        f"- {repo.get('name')} ({fork_note}{repo.get('language') or 'n/a'}, "
                        f"stars={repo.get('stargazers_count', 0)}, pushed={pushed[:10]}): "
                        f"{repo.get('description') or ''}" + (f" [app: {homepage}]" if homepage and not is_fork else "")
                    )
        except (requests.RequestException, ValueError):
            pass

        event_lines: List[str] = []
        try:
            events_resp = requests.get(
                f"https://api.github.com/users/{login}/events/public",
                params={"per_page": 6},
                headers=headers,
                timeout=4,
            )
            if events_resp.ok:
                for event in events_resp.json()[:5]:
                    ev_date = str(event.get("created_at") or "")
                    if ev_date and ev_date > latest_activity:
                        latest_activity = ev_date
                    ev_repo = event.get("repo", {}).get("name", "")
                    event_lines.append(f"- {ev_date[:10]} {event.get('type', '')} on {ev_repo}")
        except (requests.RequestException, ValueError):
            pass

        blog = str(data.get("blog") or "").strip()
        summary = (
            f"GitHub Profile: @{data.get('login')} ({data.get('name') or ''})\n"
            f"Bio: {data.get('bio') or ''}\nCompany: {data.get('company') or ''}\n"
            f"Location: {data.get('location') or ''}\nBlog: {blog}\n"
            f"Public Repos: {data.get('public_repos', 0)} | Followers: {data.get('followers', 0)}\n"
            f"Account Created: {data.get('created_at', '')} | Latest Verified Activity: {latest_activity}"
        )
        if repo_lines:
            summary += "\nPublic Repositories:\n" + "\n".join(repo_lines)
        if event_lines:
            summary += "\nRecent Public Activity:\n" + "\n".join(event_lines)

        profile_url = data.get("html_url", f"https://github.com/{login}")
        src_host = urlparse(source_url).netloc.lower().removeprefix("www.") if source_url else ""
        if not matches_terms(summary, terms, profile_url) and not (src_host and src_host in summary.lower()):
            return counts

        added = store.add_evidence(profile_url, summary, "github_api", f"github:{login}")
        if added:
            counts["github_hits"] += 1
        tag = f"E:{sha256_text(summary.strip()[:MAX_STORED_CHARACTERS])[:10]}"
        target_label = anchor["primary_name"]
        store.add_connection(target_label, profile_url, "owns_profile", "out", 0.85, tag)
        if blog:
            blog_url = canonicalize_url(blog if blog.startswith(("http://", "https://")) else f"https://{blog}")
            blog_host = urlparse(blog_url).netloc.lower().removeprefix("www.")
            # GitHub profile links back to blog_host -> record directed edge (promotes to mutual if blog linked to GitHub!)
            store.add_connection(profile_url, blog_host, "links_to", "out", 0.88, tag)
            discovered_links.insert(0, {"url": blog_url, "label": f"github_blog:{login}", "rel_me": True})
        if source_url and src_host:
            store.add_connection(src_host, profile_url, "links_to", "out", 0.85, tag)
        counts["links_queued"] += store.queue_links(discovered_links, profile_url, depth + 1, anchor)
        return counts

    def enrich_public_apis(self, store: CaseStore, anchor: Dict[str, Any]) -> Dict[str, int]:
        """Keyless, high-reliability public API enrichment (GitHub + repos + activity + Wikipedia)."""
        name = anchor.get("primary_name", "").strip()
        terms = anchor_terms(anchor)
        counts = {"github_hits": 0, "github_blogs": 0, "wikipedia_hits": 0, "links_queued": 0}
        if not name:
            return counts
        headers = {"User-Agent": USER_AGENT, "Accept": "application/json"}
        fetched_logins = set()

        # 1. GitHub public user search + explicit @handle lookup (free, unauthenticated)
        try:
            response = requests.get(
                "https://api.github.com/search/users",
                params={"q": name, "per_page": 3},
                headers=headers,
                timeout=6,
            )
            if response.ok:
                for user in response.json().get("items", [])[:2]:
                    login = user.get("login", "")
                    if login and login.lower() not in fetched_logins:
                        fetched_logins.add(login.lower())
                        gh = self.fetch_github_profile(login, store, anchor, depth=0)
                        counts["github_hits"] += gh["github_hits"]
                        counts["links_queued"] += gh["links_queued"]
        except (requests.RequestException, ValueError, KeyError) as error:
            logger.debug("[GitHub API] %s", error)

        for handle in anchor.get("hints", {}).get("handles", [])[:2]:
            login = str(handle).strip().lstrip("@")
            if login and "." not in login and "/" not in login and login.lower() not in fetched_logins:
                fetched_logins.add(login.lower())
                gh = self.fetch_github_profile(login, store, anchor, depth=0)
                counts["github_hits"] += gh["github_hits"]
                counts["links_queued"] += gh["links_queued"]

        # 2. Wikipedia public REST summary lookup (keyless)
        try:
            search_resp = requests.get(
                "https://en.wikipedia.org/w/api.php",
                params={"action": "opensearch", "search": name, "limit": 2, "format": "json"},
                headers=headers,
                timeout=5,
            )
            if search_resp.ok:
                payload = search_resp.json()
                titles = payload[1] if isinstance(payload, list) and len(payload) > 1 else []
                urls = payload[3] if isinstance(payload, list) and len(payload) > 3 else []
                for title, page_url in zip(titles, urls):
                    if not matches_terms(title, terms, page_url):
                        continue
                    slug = title.replace(" ", "_")
                    summary_resp = requests.get(
                        f"https://en.wikipedia.org/api/rest_v1/page/summary/{slug}",
                        headers=headers,
                        timeout=5,
                    )
                    if summary_resp.ok:
                        extract = summary_resp.json().get("extract", "").strip()
                        if extract and matches_terms(extract, terms, page_url):
                            if store.add_evidence(page_url, f"# {title}\n\n{extract}", "wikipedia_api", f"wiki:{name}"):
                                counts["wikipedia_hits"] += 1
        except (requests.RequestException, ValueError, KeyError) as error:
            logger.debug("[Wikipedia API] %s", error)

        # 3. Also seed known domains from anchor hints directly into the link queue (depth 0)
        hint_domains = anchor.get("hints", {}).get("domains", []) or [
            h for h in anchor.get("known_handles_or_domains", []) if "." in h and not h.startswith("@")
        ]
        seed_links = [
            {"url": d if d.startswith(("http://", "https://")) else f"https://{d}", "label": "anchor_domain", "rel_me": True}
            for d in hint_domains[:4]
        ]
        if seed_links:
            counts["links_queued"] += store.queue_links(seed_links, "", 0, anchor)

        if any(counts.values()):
            store.audit("API_ENRICHMENT", counts)
        return counts

    def _download_document(self, url: str, extension: str, evidence_directory: Path) -> Tuple[str, str]:
        local_file = evidence_directory / f"doc_{uuid.uuid4().hex[:10]}{extension}"
        with requests.get(url, timeout=15, stream=True, headers={"User-Agent": USER_AGENT}) as response:
            response.raise_for_status()
            declared_length = response.headers.get("Content-Length")
            if declared_length and declared_length.isdigit() and int(declared_length) > MAX_DOWNLOAD_BYTES:
                raise ValueError(f"Artifact exceeds {MAX_DOWNLOAD_BYTES} bytes")
            written = 0
            try:
                with open(local_file, "wb") as handle:
                    for chunk in response.iter_content(chunk_size=65536):
                        written += len(chunk)
                        if written > MAX_DOWNLOAD_BYTES:
                            raise ValueError(f"Artifact exceeds {MAX_DOWNLOAD_BYTES} bytes")
                        handle.write(chunk)
            except Exception:
                local_file.unlink(missing_ok=True)
                raise
        return self.extractor.extract(local_file), str(local_file)

    def fetch_document(self, url: str, evidence_directory: Path) -> Tuple[str, str, str, str, List[Dict[str, Any]]]:
        """Returns (url, text, method, artifact_path, outbound_links)."""
        if not self.gate.allowed(url):
            return url, "", "robots_denied", "", []
        self.gate.throttle(url)
        extension = Path(urlparse(url).path).suffix.lower()
        try:
            if extension in DOCUMENT_EXTENSIONS:
                text, artifact = self._download_document(url, extension, evidence_directory)
                return url, text, ("native_document" if text else "failed"), artifact, []
            response = requests.get(url, timeout=10, headers={"User-Agent": USER_AGENT})
            if response.status_code in (401, 403, 429, 503):
                return url, "", "access_blocked", "", []
            response.raise_for_status()
            content_type = response.headers.get("Content-Type", "").lower()
            if any(ct in content_type for ct in ("image/", "video/", "audio/", "font/", "application/octet-stream", "application/zip", "application/x-msdownload")):
                return url, "", "failed", "", []
            if "pdf" in content_type:
                local_file = evidence_directory / f"doc_{uuid.uuid4().hex[:10]}.pdf"
                local_file.write_bytes(response.content[:MAX_DOWNLOAD_BYTES])
                text = self.extractor.extract(local_file)
                return url, text, ("native_document" if text else "failed"), str(local_file), []
            if "text/plain" in content_type or "application/json" in content_type:
                return url, response.text[:MAX_STORED_CHARACTERS], "markdown", "", []
            bundle = extract_html_bundle(response.text, url)
            text = bundle["text"]
            if is_gate_page(text, response.url or url):
                return url, "", "access_blocked", "", []
            dates = list(bundle["dates"])
            last_mod = response.headers.get("Last-Modified", "").strip()
            if last_mod:
                dates.append(last_mod)
            if dates and text:
                text = f"Temporal Metadata: {', '.join(list(dict.fromkeys(dates))[:4])}\n\n{text}"
            if text and len(text.strip()) >= 60:
                method = "markdown" if trafilatura is not None else "html_text"
                return url, text, method, "", bundle["links"]
            return url, "", "failed", "", bundle["links"]
        except (requests.RequestException, OSError, ValueError) as error:
            logger.debug("[Fetch] %s: %s", url, error)
            return url, "", "failed", "", []

    def harvest(
        self,
        store: CaseStore,
        query: str,
        max_results: int = 8,
        strict_relevance: bool = True,
        anchor: Optional[Dict[str, Any]] = None,
    ) -> Dict[str, Any]:
        clean_query = sanitize_query(query)
        hits = self.search_web(clean_query, max_results)
        terms = anchor_terms(anchor, clean_query) if strict_relevance else []
        report = {
            "query": clean_query,
            "candidates": len(hits),
            "new_artifacts": 0,
            "snippets_saved": 0,
            "links_queued": 0,
            "duplicates": 0,
            "rejected_off_topic": 0,
            "robots_denied": 0,
            "access_blocked": 0,
            "failed": 0,
        }
        snippet_by_url: Dict[str, str] = {}
        fetch_urls: List[str] = []
        for hit in hits:
            url, snippet = hit["url"], hit["snippet"]
            if strict_relevance and snippet and not matches_terms(snippet, terms, url):
                report["rejected_off_topic"] += 1
                continue
            snippet_by_url[url] = snippet
            fetch_urls.append(url)

        def save_snippet_fallback(url: str) -> None:
            snippet = snippet_by_url.get(url, "")
            if snippet and matches_terms(snippet, terms, url):
                body = f"[Public search index snippet; origin page not fetched]\n{snippet}"
                if store.add_evidence(url, body, "search_snippet", clean_query):
                    report["snippets_saved"] += 1
                    report["new_artifacts"] += 1

        if fetch_urls:
            with ThreadPoolExecutor(max_workers=5) as pool:
                for url, text, method, artifact, links in pool.map(
                    lambda target_url: self.fetch_document(target_url, store.evidence_directory), fetch_urls
                ):
                    if method in ("robots_denied", "access_blocked", "failed"):
                        report[method] += 1
                        save_snippet_fallback(url)
                    elif text:
                        if strict_relevance and not matches_terms(text, terms, url):
                            report["rejected_off_topic"] += 1
                            continue
                        if store.add_evidence(url, text, method, clean_query, artifact):
                            report["new_artifacts"] += 1
                        else:
                            report["duplicates"] += 1
                        if links:
                            report["links_queued"] += store.queue_links(links, url, 1, anchor)

        store.record_query(clean_query, report["new_artifacts"])
        store.audit("HARVEST", report)
        logger.info("[Harvest] %s", report)
        return report

    def fetch_github_repo(
        self,
        owner: str,
        repo_name: str,
        store: CaseStore,
        anchor: Dict[str, Any],
        source_url: str = "",
        depth: int = 0,
    ) -> Dict[str, int]:
        """Fetches public GitHub repository metadata via REST API."""
        counts = {"hits": 0, "links_queued": 0}
        headers = {"User-Agent": USER_AGENT, "Accept": "application/json"}
        terms = anchor_terms(anchor)
        try:
            response = requests.get(f"https://api.github.com/repos/{owner}/{repo_name}", headers=headers, timeout=5)
            if not response.ok:
                return counts
            data = response.json()
        except (requests.RequestException, ValueError):
            return counts

        repo_url = data.get("html_url", f"https://github.com/{owner}/{repo_name}")
        is_fork = bool(data.get("fork"))
        homepage = str(data.get("homepage") or "").strip()
        topics = ", ".join(data.get("topics") or [])
        summary = (
            f"GitHub Repository: {data.get('full_name')}\n"
            f"Description: {data.get('description') or ''}\n"
            f"Language: {data.get('language') or 'n/a'} | Stars: {data.get('stargazers_count', 0)} | Fork: {is_fork}\n"
            f"Topics: {topics}\nHomepage: {homepage}\n"
            f"Created: {data.get('created_at', '')} | Pushed: {data.get('pushed_at', '')}"
        )
        src_host = urlparse(source_url).netloc.lower().removeprefix("www.") if source_url else ""
        if not matches_terms(summary, terms, repo_url) and not (src_host and src_host in summary.lower()):
            return counts
        if store.add_evidence(repo_url, summary, "github_repo_api", f"github_repo:{owner}/{repo_name}"):
            counts["hits"] += 1
        tag = f"E:{sha256_text(summary.strip()[:MAX_STORED_CHARACTERS])[:10]}"
        if src_host:
            store.add_connection(src_host, repo_url, "links_to", "out", 0.85, tag)
        if homepage and not is_fork:
            hp_url = canonicalize_url(homepage if homepage.startswith(("http://", "https://")) else f"https://{homepage}")
            hp_host = urlparse(hp_url).netloc.lower().removeprefix("www.")
            store.add_connection(repo_url, hp_host, "links_to", "out", 0.85, tag)
            counts["links_queued"] += store.queue_links([{"url": hp_url, "label": f"repo_app:{repo_name}", "rel_me": True}], repo_url, depth + 1, anchor)
        return counts

    def fetch_orcid_profile(
        self,
        orcid_id: str,
        store: CaseStore,
        anchor: Dict[str, Any],
        source_url: str = "",
        depth: int = 0,
    ) -> Dict[str, int]:
        """Fetches structured public ORCID record via official keyless REST API."""
        counts = {"hits": 0, "links_queued": 0}
        headers = {"User-Agent": USER_AGENT, "Accept": "application/json"}
        terms = anchor_terms(anchor)
        try:
            response = requests.get(f"https://pub.orcid.org/v3.0/{orcid_id}/record", headers=headers, timeout=6)
            if not response.ok:
                return counts
            data = response.json()
        except (requests.RequestException, ValueError):
            return counts

        person = data.get("person", {}) if isinstance(data, dict) else {}
        name_block = person.get("name") or {}
        credit_name = (name_block.get("credit-name") or {}).get("value") or ""
        given = (name_block.get("given-names") or {}).get("value") or ""
        family = (name_block.get("family-name") or {}).get("value") or ""
        full_name = credit_name or f"{given} {family}".strip()
        bio = (person.get("biography") or {}).get("content") or ""
        keywords = [kw.get("content", "") for kw in (person.get("keywords") or {}).get("keyword", []) if kw.get("content")]
        researcher_urls = (person.get("researcher-urls") or {}).get("researcher-url", [])
        discovered_links: List[Dict[str, Any]] = []
        url_lines: List[str] = []
        for entry in researcher_urls:
            raw_url = str((entry.get("url") or {}).get("value") or "").strip()
            label = str(entry.get("url-name") or "orcid_link").strip()
            if raw_url:
                norm_url = canonicalize_url(raw_url if raw_url.startswith(("http://", "https://")) else f"https://{raw_url}")
                url_lines.append(f"- {label}: {norm_url}")
                discovered_links.append({"url": norm_url, "label": label, "rel_me": True})

        history = data.get("history") or {}
        profile_url = f"https://orcid.org/{orcid_id}"
        summary = (
            f"ORCID Profile: {orcid_id} ({full_name})\n"
            f"Claimed: {history.get('claimed')} | Verified Email: {history.get('verified-email')}\n"
            f"Keywords: {', '.join(keywords)}\nBiography: {bio}"
        )
        if url_lines:
            summary += "\nResearcher URLs:\n" + "\n".join(url_lines)

        src_host = urlparse(source_url).netloc.lower().removeprefix("www.") if source_url else ""
        if not matches_terms(summary, terms, profile_url) and not (src_host and src_host in summary.lower()):
            return counts
        if store.add_evidence(profile_url, summary, "orcid_api", f"orcid:{orcid_id}"):
            counts["hits"] += 1
        tag = f"E:{sha256_text(summary.strip()[:MAX_STORED_CHARACTERS])[:10]}"
        store.add_connection(anchor["primary_name"], profile_url, "profile_orcid", "out", 0.90, tag)
        if src_host:
            store.add_connection(src_host, "orcid.org", "links_to", "out", 0.85, tag)
        for link in discovered_links:
            out_host = urlparse(link["url"]).netloc.lower().removeprefix("www.")
            if out_host and out_host != "orcid.org":
                store.add_connection("orcid.org", out_host, "links_to", "out", 0.88, tag)
        counts["links_queued"] += store.queue_links(discovered_links, profile_url, depth + 1, anchor)
        return counts

    def crawl_links(
        self,
        store: CaseStore,
        anchor: Dict[str, Any],
        max_depth: int = MAX_TRAVERSAL_DEPTH,
        max_pages: int = MAX_TRAVERSAL_PAGES,
        deadline: Optional[float] = None,
    ) -> Dict[str, Any]:
        """
        Bounded traversal across diverse queued domains: routes GitHub/ORCID links via public APIs,
        caps single-domain consumption so third-party platforms are reached, and promotes reciprocal links.
        """
        depth_cap = max(1, min(int(max_depth), MAX_TRAVERSAL_DEPTH))
        page_cap = max(1, min(int(max_pages), MAX_TRAVERSAL_PAGES))
        terms = anchor_terms(anchor)
        domain_hits: Dict[str, int] = {}
        report = {
            "pages_opened": 0,
            "new_artifacts": 0,
            "links_queued": 0,
            "connections_added": 0,
            "mutual_promoted": 0,
            "skipped_or_blocked": 0,
        }

        while report["pages_opened"] < page_cap:
            if deadline and time.monotonic() >= deadline - 4.0:
                break
            candidates = store.next_queued_links(limit=32, max_depth=depth_cap)
            if not candidates:
                break

            # Select up to 4 links while enforcing per-domain crawl diversity (max 5 per host)
            batch: List[Dict[str, Any]] = []
            batch_hosts = set()
            for entry in candidates:
                host = urlparse(entry["url"]).netloc.lower().removeprefix("www.")
                if domain_hits.get(host, 0) >= 5:
                    continue
                if host not in batch_hosts or len(candidates) <= 4:
                    batch.append(entry)
                    batch_hosts.add(host)
                if len(batch) >= min(4, page_cap - report["pages_opened"]):
                    break
            if not batch:
                batch = candidates[: min(4, page_cap - report["pages_opened"])]

            fetch_batch: List[Dict[str, Any]] = []
            for entry in batch:
                url = entry["url"]
                source_url = entry["source_url"]
                depth = entry["depth"]
                parsed = urlparse(url)
                host = parsed.netloc.lower().removeprefix("www.")
                segments = [seg for seg in parsed.path.strip("/").split("/") if seg]
                domain_hits[host] = domain_hits.get(host, 0) + 1

                # Route GitHub profile/repo and ORCID links via public keyless REST APIs
                if host == "github.com" and len(segments) == 1 and segments[0].lower() not in (
                    "topics", "explore", "features", "pricing", "about", "login", "signup", "orgs", "marketplace"
                ):
                    store.mark_link(url, "visited")
                    report["pages_opened"] += 1
                    gh = self.fetch_github_profile(segments[0], store, anchor, source_url=source_url, depth=depth)
                    report["new_artifacts"] += gh["github_hits"]
                    report["links_queued"] += gh["links_queued"]
                    continue
                if host == "github.com" and len(segments) == 2 and segments[0].lower() not in ("topics", "orgs", "marketplace", "settings"):
                    store.mark_link(url, "visited")
                    report["pages_opened"] += 1
                    repo_res = self.fetch_github_repo(segments[0], segments[1], store, anchor, source_url=source_url, depth=depth)
                    report["new_artifacts"] += repo_res["hits"]
                    report["links_queued"] += repo_res["links_queued"]
                    continue
                if host == "orcid.org" and len(segments) == 1 and re.match(r"^\d{4}-\d{4}-\d{4}-\d{3}[\dX]$", segments[0]):
                    store.mark_link(url, "visited")
                    report["pages_opened"] += 1
                    orcid_res = self.fetch_orcid_profile(segments[0], store, anchor, source_url=source_url, depth=depth)
                    report["new_artifacts"] += orcid_res["hits"]
                    report["links_queued"] += orcid_res["links_queued"]
                    continue

                if store.has_url(url):
                    store.mark_link(url, "visited")
                    continue
                fetch_batch.append(entry)

            if not fetch_batch:
                continue

            with ThreadPoolExecutor(max_workers=4) as pool:
                fetched = list(pool.map(
                    lambda entry: (entry, self.fetch_document(entry["url"], store.evidence_directory)),
                    fetch_batch,
                ))

            for entry, (url, text, method, artifact, links) in fetched:
                source_url = entry["source_url"]
                depth = entry["depth"]
                report["pages_opened"] += 1
                if method in ("robots_denied", "access_blocked", "failed") or not text:
                    store.mark_link(url, method or "failed")
                    report["skipped_or_blocked"] += 1
                    # Still record outbound platform reference if linked directly from a verified page
                    if source_url and any(urlparse(url).netloc.lower().endswith(dom) for dom in CORROBORATION_DOMAINS):
                        src_host = urlparse(source_url).netloc.lower().removeprefix("www.")
                        src_tag = store.evidence_tag_for(source_url)
                        outcome = store.add_connection(src_host, url, "links_to", "out", 0.72, src_tag)
                        report["connections_added"] += 1
                        if outcome == "promoted_two_directional":
                            report["mutual_promoted"] += 1
                    continue

                store.mark_link(url, "visited")
                # Accept pages matching target identifiers OR verified same-origin subpages (/about, /projects, /resume)
                src_host = urlparse(source_url).netloc.lower().removeprefix("www.") if source_url else ""
                cur_host = urlparse(url).netloc.lower().removeprefix("www.")
                is_same_origin = bool(src_host and cur_host == src_host and store.has_url(source_url))
                if not (matches_terms(text, terms, url) or is_same_origin):
                    continue

                if store.add_evidence(url, text, method, f"link_crawl:d{depth}", artifact):
                    report["new_artifacts"] += 1
                tag = f"E:{sha256_text(text.strip()[:MAX_STORED_CHARACTERS])[:10]}"

                # Record directional edge from source -> opened link (and check reciprocal links!)
                if src_host and cur_host and src_host != cur_host:
                    outcome = store.add_connection(src_host, cur_host, "links_to", "out", 0.78, tag)
                    report["connections_added"] += 1
                    if outcome == "promoted_two_directional":
                        report["mutual_promoted"] += 1

                # Inspect outbound links on this page for reciprocal backlinks & next-hop traversal
                for out_link in links:
                    out_url = out_link["url"]
                    out_host = urlparse(out_url).netloc.lower().removeprefix("www.")
                    if out_host and out_host != cur_host and any(
                        out_host == dom or out_host.endswith(f".{dom}") for dom in CORROBORATION_DOMAINS
                    ):
                        outcome = store.add_connection(cur_host, out_url, "links_to", "out", 0.82 if out_link.get("rel_me") else 0.75, tag)
                        report["connections_added"] += 1
                        if outcome == "promoted_two_directional":
                            report["mutual_promoted"] += 1
                    elif src_host and out_host == src_host and cur_host != src_host:
                        # Reciprocal backlink discovered -> promotes src_host <-> cur_host to two_directional!
                        outcome = store.add_connection(cur_host, src_host, "links_to", "out", 0.85, tag)
                        if outcome == "promoted_two_directional":
                            report["mutual_promoted"] += 1

                if depth < depth_cap and links:
                    report["links_queued"] += store.queue_links(links, url, depth + 1, anchor)

        store.audit("LINK_TRAVERSAL", report)
        logger.info("[Traverse] %s", report)
        return report


# ==============================================================================
# 7. INTELLIGENCE CORE (deterministic ACH + entities + ICD-203 dossier)
# ==============================================================================
class IntelligenceCore:
    def __init__(self) -> None:
        self.extractor = DocumentExtractor(NATIVE_DLL_PATH)
        self.collector = SourceCollector(PolitenessGate(), self.extractor)
        self.llm = LanguageModelClient()

    def build_anchor(
        self,
        seed: str,
        purpose: str,
        aliases: Optional[List[str]] = None,
        handles: Optional[List[str]] = None,
        location: str = "",
        profession: str = "",
        hints: Optional[Dict[str, Any]] = None,
    ) -> Dict[str, Any]:
        seed, purpose = (seed or "").strip(), (purpose or "").strip()
        if not seed:
            raise ValueError("seed must not be empty.")
        if len(purpose) < 10:
            raise ValueError("purpose must state the lawful research objective (min 10 characters).")
        case_id = uuid.uuid4().hex[:12]
        anchor = parse_seed(
            seed, aliases=aliases, handles=handles, location=location, profession=profession, hints=hints
        )
        anchor.update({"case_id": case_id, "seed": seed, "purpose": purpose, "created_utc": utc_now(), "immutable": True})
        store = CaseStore(case_id)
        store.anchor_path.write_text(json.dumps(anchor, indent=2, ensure_ascii=False), encoding="utf-8")
        store.audit("ANCHOR_LOCKED", {"seed": seed, "purpose": purpose})
        logger.info("[Phase 1] Anchor locked: %s (%s)", anchor["primary_name"], case_id)
        return anchor

    @staticmethod
    def wide_net_queries(anchor: Dict[str, Any]) -> List[str]:
        name = anchor["primary_name"]
        queries = [name]
        for alias in anchor.get("known_aliases", [])[:2]:
            queries.append(alias)
        for handle in anchor.get("known_handles_or_domains", [])[:2]:
            queries.append(handle)
        return list(dict.fromkeys(queries))

    @staticmethod
    def is_profile_url(parsed_url: Any, platform: str) -> bool:
        """Checks whether a social/platform URL looks like an account profile rather than an issue/video/post."""
        segments = [seg for seg in parsed_url.path.strip("/").split("/") if seg]
        if not segments:
            return False
        first = segments[0].lower()
        if platform in ("github", "gitlab"):
            return len(segments) == 1 and first not in (
                "topics", "explore", "features", "pricing", "about", "login", "signup", "orgs", "marketplace", "search",
            )
        if platform == "linkedin":
            return len(segments) >= 2 and first in ("in", "company", "pub")
        if platform == "youtube":
            return first.startswith("@") or (len(segments) >= 2 and first in ("c", "channel", "user"))
        if platform == "x_twitter":
            return len(segments) == 1 and first not in ("intent", "share", "search", "home", "explore", "i")
        if platform in ("instagram", "twitch", "medium", "orcid"):
            return len(segments) == 1
        if platform == "substack":
            host = parsed_url.netloc.lower()
            return first.startswith("@") or (host.endswith(".substack.com") and host != "substack.com")
        return True

    @staticmethod
    def extract_latest_activity(rows: List[Dict[str, Any]]) -> str:
        """Extracts the most recent verified ISO timestamp (YYYY-MM-DD...) proving active vs. stale presence."""
        iso_dates: List[str] = []
        today_iso = datetime.now(timezone.utc).strftime("%Y-%m-%d")
        for row in rows:
            for match in re.findall(r"\b(20\d{2}-(?:0[1-9]|1[0-2])-(?:0[1-9]|[12]\d|3[01])(?:T\d{2}:\d{2}:\d{2}Z?)?)\b", row["content"]):
                if match[:10] <= today_iso:
                    iso_dates.append(match)
        return max(iso_dates) if iso_dates else ""

    def deterministic_analysis(self, anchor: Dict[str, Any], store: CaseStore) -> Dict[str, Any]:
        """Fast (<50ms), 100% grounded ACH, entity, temporal activity, connection, and pivot extraction."""
        rows = store.evidence_rows()
        terms = anchor_terms(anchor)
        name = anchor["primary_name"]
        location = str(anchor.get("anchor_demographics", {}).get("origin_city") or "").strip().lower()
        profession = str(anchor.get("core_profession") or "").strip().lower()

        matched_rows = [
            row for row in rows
            if not is_gate_page(row["content"], row["url"])
            and (
                matches_terms(row["content"], terms, row["url"])
                or str(row.get("query", "")).startswith(("link_crawl:", "github:", "github_repo:", "orcid:"))
            )
        ]
        primary_rows = [row for row in matched_rows if row["method"] != "search_snippet"]
        snippet_rows = [row for row in matched_rows if row["method"] == "search_snippet"]
        unique_domains = sorted({row["domain"].removeprefix("www.") for row in primary_rows if row["domain"]})
        all_domains = sorted({row["domain"].removeprefix("www.") for row in matched_rows if row["domain"]})
        latest_activity = self.extract_latest_activity(matched_rows)

        compact_name = re.sub(r"[\s_-]+", "", name.lower())
        third_party_domains = [
            dom for dom in unique_domains
            if dom != "github.com" and (len(compact_name) < 6 or compact_name not in dom)
        ]

        claims: List[Dict[str, Any]] = []
        entities: Dict[Tuple[str, str], str] = {}
        seen_statements = set()
        qualifier_hits = 0
        verified_handles = set()

        for row in matched_rows:
            tag = f"E:{row['content_hash'][:10]}"
            url = row["url"]
            domain = row["domain"]
            text = row["content"]
            lower_text = text.lower()
            reliability, credibility = classify_reliability(url, row["method"], anchor)
            is_verified_crawl = str(row.get("query", "")).startswith(("link_crawl:", "github:", "github_repo:", "orcid:"))

            if domain:
                clean_domain = domain.removeprefix("www.")
                platform = next((label for host, label in SOCIAL_PLATFORMS.items() if clean_domain == host or clean_domain.endswith(f".{host}")), None)
                if platform:
                    entities[(url, platform)] = f"Public {platform} footprint ({tag})"
                    store.add_connection(name, url, f"profile_{platform}", "out", 0.80 if row["method"] != "search_snippet" else 0.55, tag)
                    if row["method"] != "search_snippet":
                        segs = [seg for seg in urlparse(url).path.strip("/").split("/") if seg]
                        if segs:
                            verified_handles.add(segs[-1].lstrip("@").lower())
                else:
                    entities[(clean_domain, "domain")] = f"Referenced source domain ({tag})"
                    if matches_terms(clean_domain, terms) or is_verified_crawl:
                        store.add_connection(name, clean_domain, "associated_domain", "out", 0.82, tag)

            if location and location in lower_text:
                qualifier_hits += 1
            if profession and any(word in lower_text for word in profession.split() if len(word) >= 4):
                qualifier_hits += 1

            # Extract grounded public social/repo URLs mentioned inside verified primary pages
            for found_url in re.findall(r"https?://[A-Za-z0-9._~:/?#\[\]@!$&'()*+,;=-]{8,140}", text):
                cleaned_url = found_url.rstrip(".,);]")
                parsed = urlparse(cleaned_url)
                host = parsed.netloc.lower().removeprefix("www.")
                platform = next((label for key, label in SOCIAL_PLATFORMS.items() if host == key or host.endswith(f".{key}")), None)
                if platform and self.is_profile_url(parsed, platform):
                    entities[(cleaned_url, platform)] = f"Linked profile in {tag}"
                    store.add_connection(name, cleaned_url, f"profile_{platform}", "out", 0.78, tag)
                    if row["method"] != "search_snippet":
                        segs = [seg for seg in parsed.path.strip("/").split("/") if seg]
                        if segs:
                            verified_handles.add(segs[-1].lstrip("@").lower())

            # Extract concrete, non-boilerplate grounded claims
            sentences = re.split(r"(?<=[.!?])\s+|\n+", text)
            added_from_row = 0
            for sentence in sentences:
                clean = re.sub(r"\s+", " ", sentence).strip(" -*#")
                if not (20 <= len(clean) <= 260):
                    continue
                lowered_clean = clean.lower()
                if any(phrase in lowered_clean for phrase in BOILERPLATE_PHRASES):
                    continue
                has_iso_date = bool(re.search(r"\b20\d{2}-(?:0[1-9]|1[0-2])-(?:0[1-9]|[12]\d|3[01])\b", clean))
                is_repo_or_project = (
                    row["method"] in ("github_api", "github_repo_api", "orcid_api")
                    and ("pushed=" in clean or "Latest Verified Activity" in clean or "PushEvent" in clean or "ORCID" in clean or "Language:" in clean)
                ) or (is_verified_crawl and any(k in lowered_clean for k in ("project", "built", "engine", "simulator", "platform", "app", "c++", "cuda", "research", "location", "innsbruck", "austria", "orcid")))
                if not (matches_terms(clean, terms) or is_repo_or_project or (is_verified_crawl and has_iso_date)):
                    continue
                normalized = lowered_clean
                if normalized in seen_statements:
                    continue
                seen_statements.add(normalized)
                has_year = has_iso_date or bool(re.search(r"\b(?:19|20)\d{2}\b", clean))
                category = "temporal" if has_year else ("project" if is_repo_or_project else ("digital_footprint" if row["method"] in ("github_api", "search_snippet") else "biographical"))
                claims.append({
                    "category": category,
                    "statement": redact_sensitive(clean),
                    "evidence_tag": tag,
                    "source_url": url,
                    "source_reliability": reliability,
                    "info_credibility": credibility,
                    "hypothesis_alignment": "PRIMARY",
                })
                added_from_row += 1
                if added_from_row >= 6:
                    break

        # Detect uncorroborated search-snippet handles (potential H2 collision candidates)
        collision_candidates: List[str] = []
        for srow in snippet_rows:
            segs = [seg for seg in urlparse(srow["url"]).path.strip("/").split("/") if seg]
            handle = segs[-1].lstrip("@").lower() if segs else ""
            if handle and verified_handles and handle not in verified_handles and compact_name not in handle:
                collision_candidates.append(f"{srow['url']} (@{handle})")

        # Rigorous confidence calibration: 2 first-party sources cap at MODERATE (<= 0.74);
        # 3+ independent primary domains (including third-party) are required for HIGH (>= 0.78).
        graph = store.connection_graph(anchor)
        mutual_count = len(graph["connections"]["two_directional"])
        has_authority = any(c["source_reliability"] == "A" for c in claims)
        if not matched_rows:
            confidence = 0.05
        elif not primary_rows:
            confidence = min(0.30, 0.12 + 0.06 * len(all_domains))
        elif len(unique_domains) == 1:
            confidence = min(0.58, round(0.44 + (0.06 if qualifier_hits else 0.0) + (0.06 if has_authority else 0.0), 2))
        elif len(unique_domains) == 2 and not third_party_domains:
            confidence = min(
                0.74,
                round(0.58 + (0.05 if qualifier_hits else 0.0) + (0.05 if has_authority else 0.0) + (0.06 if mutual_count > 0 else 0.0), 2),
            )
        else:
            base = 0.62 + 0.07 * min(len(unique_domains) - 1, 3)
            bonus = 0.04 if qualifier_hits > 0 else 0.0
            authority_bonus = 0.05 if has_authority else 0.0
            mutual_bonus = 0.05 if mutual_count > 0 else 0.0
            third_party_bonus = 0.05 if third_party_domains else 0.0
            collision_penalty = 0.04 if collision_candidates else 0.0
            confidence = min(0.94, max(0.40, round(base + bonus + authority_bonus + mutual_bonus + third_party_bonus - collision_penalty, 2)))

        h1_status = "LEADING" if confidence >= 0.50 else ("ACTIVE" if matched_rows else "UNVERIFIED")
        h2_score = round(max(0.08, 1.0 - confidence), 2)
        h2_status = "DISCOUNTED" if confidence >= 0.78 and not collision_candidates else "ACTIVE"

        # Clean, short, search-engine-safe pivot queries (never prose sentences!)
        executed = store.executed_queries()
        pivot_candidates = [
            f"{name} github",
            f"{name} orcid",
            f"{name} linkedin",
            f"{name} {location}" if location else f"{name} biography",
            f"{name} projects",
        ]
        for alias in anchor.get("known_aliases", [])[:2]:
            pivot_candidates.append(alias)
        pivots = [
            query.strip()
            for query in dict.fromkeys(pivot_candidates)
            if query.strip() and query.strip().lower() not in executed
        ][:3]

        links_info = store.link_summary()
        gaps: List[str] = []
        if not matched_rows:
            gaps.append("Zero collected evidence artifacts mention the target identifiers.")
        if len(unique_domains) < 2:
            gaps.append("Corroboration from additional independent primary domains is needed.")
        elif not third_party_domains:
            gaps.append("Verified primary evidence comes only from first-party site/GitHub; independent third-party domain corroboration is needed.")
        if not latest_activity:
            gaps.append("No recent ISO-dated activity timestamp confirmed; subject currency unverified.")
        if mutual_count == 0:
            gaps.append("No reciprocal two-directional cross-domain backlink verified yet.")
        if not any(etype in ("github", "linkedin", "x_twitter", "orcid") for _, etype in entities):
            gaps.append("No verified third-party social or code-repository profile confirmed yet.")
        if snippet_rows:
            sample_snippets = ", ".join(urlparse(r["url"]).netloc.removeprefix("www.") for r in snippet_rows[:4])
            gaps.append(f"{len(snippet_rows)} profile(s) exist only as E5 search snippets awaiting primary verification: {sample_snippets}.")
        if collision_candidates:
            gaps.append(f"Uncorroborated handle divergence (possible H2 collision): {', '.join(collision_candidates[:3])}.")
        if links_info["queued_count"] > 0:
            gaps.append(f"{links_info['queued_count']} discovered outbound link(s) remain unvisited (e.g. {', '.join(links_info['top_queued'][:2])}).")
        if links_info["blocked"]:
            blocked_hosts = ", ".join(sorted({urlparse(b["url"]).netloc.removeprefix("www.") for b in links_info["blocked"]})[:4])
            gaps.append(f"Automated fetch blocked or gated by robots/consent on: {blocked_hosts}.")

        assumptions = [
            "Open-source collection is restricted to publicly indexable pages honouring robots.txt; login/consent gates are never bypassed.",
            "Edges promote from one_directional to two_directional only when reciprocal/independent evidence confirms both directions.",
        ]

        return {
            "confidence": confidence,
            "latest_activity_utc": latest_activity,
            "pivot_queries": pivots,
            "hypotheses": [
                {
                    "id": "H1",
                    "label": "Primary Target",
                    "description": anchor["initial_hypotheses"][0]["description"],
                    "status": h1_status,
                    "diagnostic_score": confidence,
                },
                {
                    "id": "H2",
                    "label": "Namesake / Collision",
                    "description": anchor["initial_hypotheses"][1]["description"],
                    "status": h2_status,
                    "diagnostic_score": h2_score,
                },
            ],
            "claims": claims[:50],
            "entities": [{"name": entity_name, "type": entity_type, "relation": relation}
                         for (entity_name, entity_type), relation in list(entities.items())[:50]],
            "key_assumptions": assumptions,
            "intelligence_gaps": gaps,
        }

    def run_analysis(
        self,
        anchor: Dict[str, Any],
        store: CaseStore,
        max_passes: int = 1,
        deadline: Optional[float] = None,
        notify: Optional[Callable[[str], None]] = None,
    ) -> Dict[str, Any]:
        """Bounded Research_req + grounded ACH loop (crawls unvisited links & pivots on pass > 1)."""
        passes = max(1, min(int(max_passes), MAX_RESEARCH_PASSES))
        for pass_number in range(1, passes + 1):
            analysis = self.deterministic_analysis(anchor, store)
            state = store.replace_analysis(analysis, pass_number, anchor=anchor)
            store.audit("ANALYSIS_PASS", {
                "pass": pass_number, "confidence": state["confidence"],
                "claims": len(state["claims"]), "entities": len(state["entities"]),
                "delta": state.get("delta_vs_previous", {}),
            })
            if pass_number == passes:
                break
            if deadline and time.monotonic() >= deadline - 6.0:
                logger.info("[Analysis] Deadline budget reached before pivot pass; finishing cleanly.")
                break
            # Expand queued corroboration links first when follow-up verification runs
            if store.link_summary()["queued_count"] > 0:
                if notify:
                    notify(f"Research_req pass {pass_number}: verifying queued links")
                self.collector.crawl_links(store, anchor, MAX_TRAVERSAL_DEPTH, 6, deadline)
            pivots = state.get("recommended_pivots", [])[:2]
            for query in pivots:
                if deadline and time.monotonic() >= deadline - 4.0:
                    break
                if notify:
                    notify(f"Research_req pass {pass_number}: pivot '{query}'")
                self.collector.harvest(store, query, max_results=5, strict_relevance=True, anchor=anchor)
        return store.analysis_state()

    @staticmethod
    def confidence_label(score: float) -> str:
        if score >= 0.78:
            return "HIGH"
        if score >= 0.45:
            return "MODERATE"
        return "LOW"

    def synthesize(self, anchor: Dict[str, Any], store: CaseStore, analyst_narrative: str = "") -> Path:
        """Compiles an ICD-203 Markdown dossier deterministically (+ optional LLM/host narrative)."""
        state = store.analysis_state()
        rows = store.evidence_rows()
        stats = store.statistics()
        graph = store.connection_graph(anchor)
        latest_activity = self.extract_latest_activity(rows)
        confidence = state["confidence"]
        band = self.confidence_label(confidence)
        delta = state.get("delta_vs_previous") or {}

        narrative = analyst_narrative.strip()
        if not narrative and self.llm.enabled and rows:
            try:
                excerpts = store.evidence_excerpts(anchor, limit=6, excerpt_chars=600)
                prompt = (
                    "Write a concise ICD-203 Executive BLUF (3-5 bullet judgments with [E:tag] citations) "
                    "using ONLY these grounded facts. Do not invent URLs, dates, or private PII.\n"
                    f"Anchor: {json.dumps(anchor, ensure_ascii=False)}\n"
                    f"Evidence: {json.dumps(excerpts, ensure_ascii=False)}"
                )
                narrative = self.llm.complete(prompt, timeout_seconds=25)
            except Exception as error:
                logger.warning("[Synthesize] Optional LLM narrative skipped: %s", error)

        lines = [
            f"<!-- Specter case {store.case_id} | generated {utc_now()} | purpose: {anchor.get('purpose', '')} -->",
            "",
            f"# Intelligence Dossier: {anchor['primary_name']}",
            "",
            "## 1. BLUF (Bottom Line Up Front) & Analytic Confidence",
            f"- **Case ID:** `{store.case_id}`",
            f"- **Analytic Confidence:** **{band}** (`{confidence:.2f}`)",
            f"- **Latest Verified Activity (Temporal Grounding):** `{latest_activity or 'Unverified'}`",
            f"- **Target Identity:** {anchor['primary_name']} (`{anchor.get('entity_kind', 'unknown')}`)",
            f"- **Aliases / Handles:** {', '.join(anchor.get('known_aliases', []) + anchor.get('known_handles_or_domains', [])) or 'None specified'}",
            f"- **Evidence Base:** {stats['evidence_total']} unique SHA-256 artifacts across {stats['unique_domains']} domain(s); "
            f"{len(state['claims'])} grounded claims; {stats['connections_mutual']} mutual / {stats['connections_one_way']} one-way connections.",
        ]
        if delta:
            lines.append(
                f"- **Verification Delta (Pass {state['passes_completed']} vs. Previous):** "
                f"Confidence `{delta.get('previous_confidence', 0.0):.2f}` -> `{delta.get('current_confidence', confidence):.2f}` "
                f"(`{delta.get('confidence_delta', 0.0):+.2f}`); "
                f"New Artifacts `+{delta.get('new_artifacts_delta', 0)}`; "
                f"New Mutual Edges `+{delta.get('mutual_edges_delta', 0)}`; "
                f"New Primary Domains: `{', '.join(delta.get('new_primary_domains') or []) or 'None'}`."
            )
        if narrative:
            lines += ["", "### Analyst Executive Assessment", narrative]

        lines += [
            "",
            "## 2. Analysis of Competing Hypotheses (ACH)",
            "| ID | Hypothesis | Status | Diagnostic Score |",
            "|---|---|---|---|",
        ]
        for hyp in state["hypotheses"]:
            desc = str(hyp["description"]).replace("|", "/")
            lines.append(f"| {hyp['hypothesis_id']} | **{hyp['label']}** — {desc} | {hyp['status']} | {hyp['diagnostic_score']:.2f} |")

        timeline_claims = [c for c in state["claims"] if c["category"] == "temporal"]
        lines += ["", "## 3. Grounded Chronological Timeline & Temporal Proof"]
        if timeline_claims:
            for claim in timeline_claims:
                code = f"{claim['source_reliability']}{claim['info_credibility']}"
                flag = " `[UNVERIFIED / LOW CONFIDENCE]`" if claim["source_reliability"] in ("D", "E", "F") else ""
                lines.append(f"- `[{code}]` [{claim['evidence_tag']}] {claim['statement']}{flag}")
        else:
            lines.append("- *No independently dated milestones verified in current evidence.*")

        lines += [
            "",
            "## 4. Directional Connection Graph & Digital Footprint",
            "### Two-Directional Connections (Mutual / Reciprocally Verified `A <-> B`)",
            "| Pair | Relation | Confidence | Sources |",
            "|---|---|---|---|",
        ]
        if graph["connections"]["two_directional"]:
            for edge in graph["connections"]["two_directional"]:
                pair_str = f"{edge['pair'][0]} <-> {edge['pair'][1]}".replace("|", "/")
                lines.append(f"| {pair_str} | {edge['relation']} | {edge['confidence']:.2f} | {', '.join(edge['sources']) or '—'} |")
        else:
            lines.append("| *None verified yet* | — | — | — |")

        lines += [
            "",
            "### One-Directional Connections (`A -> B`)",
            "| From | To | Relation | Confidence | Sources |",
            "|---|---|---|---|---|",
        ]
        if graph["connections"]["one_directional"]:
            for edge in graph["connections"]["one_directional"][:30]:
                lines.append(
                    f"| {str(edge['from']).replace('|', '/')} | {str(edge['to']).replace('|', '/')} | "
                    f"{edge['relation']} | {edge['confidence']:.2f} | {', '.join(edge['sources']) or '—'} |"
                )
        else:
            lines.append("| *None recorded* | — | — | — | — |")

        lines += [
            "",
            "### Verified Entities & Profiles",
            "| Entity / Handle / URL | Type | Relation |",
            "|---|---|---|",
        ]
        if state["entities"]:
            for entity in state["entities"]:
                lines.append(f"| {str(entity['name']).replace('|', '/')} | {entity['entity_type']} | {str(entity['relation']).replace('|', '/')} |")
        else:
            lines.append("| *None verified* | — | — |")

        lines += ["", "## 5. Key Assumptions & Intelligence Gaps", "### Key Assumptions"]
        for item in state["key_assumptions"] or ["None recorded."]:
            lines.append(f"- {item}")
        lines += ["", "### Intelligence Gaps"]
        for item in state["intelligence_gaps"] or ["None recorded."]:
            lines.append(f"- {item}")

        lines += [
            "",
            "---",
            "",
            "## Appendix A — Evidence Register (Chain of Custody)",
            "| Tag | SHA-256 | Method | Collected (UTC) | Source |",
            "|---|---|---|---|---|",
        ]
        for row in rows:
            lines.append(
                f"| E:{row['content_hash'][:10]} | `{row['content_hash']}` | {row['method']} | "
                f"{row['collected_utc']} | {row['url']} |"
            )

        lines += [
            "",
            "## Appendix B — Grounded Rated Claims (NATO Admiralty)",
            "| Rating | Category | Claim | Evidence |",
            "|---|---|---|---|",
        ]
        for claim in state["claims"]:
            statement = str(claim["statement"]).replace("|", "/")
            lines.append(
                f"| {claim['source_reliability']}{claim['info_credibility']} | {claim['category']} | "
                f"{statement} | {claim['evidence_tag'] or claim['source_url']} |"
            )

        dossier_text = redact_sensitive("\n".join(lines) + "\n")
        store.dossier_path.write_text(dossier_text, encoding="utf-8")
        store.audit("DOSSIER_WRITTEN", str(store.dossier_path))
        logger.info("[Phase 4] Dossier written: %s", store.dossier_path)
        return store.dossier_path


# ==============================================================================
# 8. GEO SENSOR FUSION (NASA FIRMS thermal + USGS seismic + regional news)
# ==============================================================================
class GeoSensor:
    EARTH_RADIUS_KM = 6371.0

    @classmethod
    def distance_km(cls, latitude_a: float, longitude_a: float, latitude_b: float, longitude_b: float) -> float:
        phi_a, phi_b = math.radians(latitude_a), math.radians(latitude_b)
        delta_phi = math.radians(latitude_b - latitude_a)
        delta_lambda = math.radians(longitude_b - longitude_a)
        haversine = math.sin(delta_phi / 2) ** 2 + math.cos(phi_a) * math.cos(phi_b) * math.sin(delta_lambda / 2) ** 2
        return 2 * cls.EARTH_RADIUS_KM * math.asin(math.sqrt(min(1.0, max(0.0, haversine))))

    @staticmethod
    def bounding_box(latitude: float, longitude: float, radius_km: float) -> Tuple[float, float, float, float]:
        latitude_delta = radius_km / 111.32
        longitude_delta = radius_km / (111.32 * max(0.01, math.cos(math.radians(latitude))))
        return (
            max(-180.0, longitude - longitude_delta), max(-90.0, latitude - latitude_delta),
            min(180.0, longitude + longitude_delta), min(90.0, latitude + latitude_delta),
        )

    @classmethod
    def thermal_hotspots(cls, latitude: float, longitude: float, radius_km: float, days: int) -> Dict[str, Any]:
        if not FIRMS_MAP_KEY:
            return {"status": "disabled", "reason": "Set SPECTER_FIRMS_KEY (free NASA Earthdata FIRMS MAP_KEY)."}
        west, south, east, north = cls.bounding_box(latitude, longitude, radius_km)
        url = (
            f"https://firms.modaps.eosdis.nasa.gov/api/area/csv/{FIRMS_MAP_KEY}/{FIRMS_SOURCE}/"
            f"{west:.4f},{south:.4f},{east:.4f},{north:.4f}/{max(1, min(days, 5))}"
        )
        response = requests.get(url, timeout=20, headers={"User-Agent": USER_AGENT})
        response.raise_for_status()
        hotspots = []
        for row in csv.DictReader(io.StringIO(response.text)):
            try:
                point_latitude, point_longitude = float(row["latitude"]), float(row["longitude"])
                radiative_power = float(row.get("frp") or 0)
            except (KeyError, ValueError):
                continue
            distance = cls.distance_km(latitude, longitude, point_latitude, point_longitude)
            if distance <= radius_km:
                hotspots.append({
                    "latitude": point_latitude, "longitude": point_longitude,
                    "distance_km": round(distance, 2), "frp_mw": radiative_power,
                    "acquired": f"{row.get('acq_date', '')} {row.get('acq_time', '')}",
                    "confidence": row.get("confidence"), "daynight": row.get("daynight"),
                })
        hotspots.sort(key=lambda spot: spot["frp_mw"], reverse=True)
        return {"status": "ok", "source": FIRMS_SOURCE, "count": len(hotspots), "hotspots": hotspots[:50]}

    @staticmethod
    def seismic_events(latitude: float, longitude: float, radius_km: float, days: int) -> Dict[str, Any]:
        start = (datetime.now(timezone.utc) - timedelta(days=days)).strftime("%Y-%m-%dT%H:%M:%S")
        response = requests.get(
            "https://earthquake.usgs.gov/fdsnws/event/1/query",
            params={
                "format": "geojson", "latitude": latitude, "longitude": longitude,
                "maxradiuskm": radius_km, "starttime": start, "orderby": "magnitude", "limit": 50,
            },
            timeout=20, headers={"User-Agent": USER_AGENT},
        )
        response.raise_for_status()
        events = []
        for feature in response.json().get("features", []):
            properties = feature.get("properties", {})
            coordinates = feature.get("geometry", {}).get("coordinates", [])
            events.append({
                "magnitude": properties.get("mag"),
                "place": properties.get("place"),
                "time_utc": datetime.fromtimestamp((properties.get("time") or 0) / 1000, timezone.utc).isoformat(),
                "depth_km": coordinates[2] if len(coordinates) > 2 else None,
                "url": properties.get("url"),
            })
        return {"status": "ok", "count": len(events), "events": events}

    @staticmethod
    def regional_news(place_name: str, max_results: int = 10) -> Dict[str, Any]:
        if not place_name.strip() or DDGS is None:
            return {"status": "skipped"}
        query = f"{place_name} fire OR explosion OR earthquake OR outage OR strike"
        try:
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", RuntimeWarning)
                with DDGS() as search_client:
                    items = search_client.news(query, max_results=max_results) or []
        except Exception as error:
            return {"status": "error", "reason": str(error)}
        return {"status": "ok", "query": query, "items": [
            {"title": item.get("title"), "date": item.get("date"), "source": item.get("source"), "url": item.get("url")}
            for item in items
        ]}


# ==============================================================================
# 9. FASTMCP / MCPSERVER SURFACE (LLM-agnostic, fast, timeout-safe)
# ==============================================================================
mcp = FastMCP(
    "Specter_Intel",
    instructions=(
        "All-source OSINT & Geo-Sensory MCP server. Always call anchor_target first to get a 12-hex case_id "
        "(or run_full_cycle for a single-call workflow). Use short 2-6 keyword queries in harvest_sources "
        "(no prose or comma lists), traverse_links to crawl discovered outbound links, run_analysis for grounded ACH, "
        "and synthesize_dossier to emit the ICD-203 report."
    ),
)
core = IntelligenceCore()


def to_json(payload: Any) -> str:
    return json.dumps(payload, indent=2, ensure_ascii=False, default=str)


def list_case_summaries() -> List[Dict[str, Any]]:
    """Lists existing cases on disk for list_cases and error recovery."""
    cases = []
    for directory in sorted(CASES_DIRECTORY.iterdir()):
        anchor_file = directory / "anchor.json"
        if directory.is_dir() and CASE_ID_PATTERN.match(directory.name) and anchor_file.exists():
            anchor = json.loads(anchor_file.read_text(encoding="utf-8"))
            cases.append({
                "case_id": directory.name,
                "primary_name": anchor.get("primary_name"),
                "created_utc": anchor.get("created_utc"),
                "evidence_total": CaseStore(directory.name).statistics()["evidence_total"],
                "dossier_ready": (directory / "dossier.md").exists(),
            })
    return cases


def case_error_json(error: Exception, case_id: str = "") -> str:
    """Returns explicit, unmasked recovery JSON (mcp 2.x masks raised exceptions)."""
    return to_json({
        "status": "ERROR",
        "error": str(error),
        "bad_case_id": case_id or None,
        "existing_cases": list_case_summaries()[:10],
        "action_required": (
            "Call run_full_cycle(seed, purpose) to investigate a target and create a dossier in one call, "
            "or call anchor_target(seed, purpose) first to obtain a valid 12-hex case_id."
        ),
    })


@mcp.tool()
async def system_health() -> str:
    """Reports runtime readiness (zero hard dependencies; lists optional modules and LLM mode)."""
    def probe() -> Dict[str, Any]:
        try:
            import pypdf  # noqa: F401
            has_pypdf = True
        except ImportError:
            has_pypdf = False
        return {
            "status": "READY",
            "mode": "host_llm_agnostic" if not core.llm.enabled else f"external_llm:{core.llm.provider}",
            "specter_home": str(SPECTER_HOME),
            "web_search": DDGS is not None,
            "trafilatura": trafilatura is not None,
            "stdlib_html_fallback": True,
            "pypdf": has_pypdf,
            "optional_native_dll": bool(core.extractor.dll),
            "firms_key": bool(FIRMS_MAP_KEY),
            "external_llm": {"provider": core.llm.provider, "url": core.llm.base_url if core.llm.enabled else None,
                             "model": core.llm.model or None},
        }
    return to_json(await asyncio.to_thread(probe))


@mcp.tool()
async def anchor_target(
    seed: str,
    purpose: str = "Open-source public intelligence research",
    aliases: Optional[List[str]] = None,
    handles_or_domains: Optional[List[str]] = None,
    location: str = "",
    profession: str = "",
    hints: Optional[Dict[str, Any]] = None,
) -> str:
    """
    Create and start a new OSINT / spying / intelligence investigation case for a target
    person, company, or domain and return a valid 12-hex case_id.
    Required before calling harvest_sources, ingest_url, traverse_links, run_analysis, or synthesize_dossier.
    """
    try:
        anchor = await asyncio.to_thread(
            core.build_anchor, seed, purpose, aliases, handles_or_domains, location, profession, hints
        )
    except ValueError as error:
        return to_json({"status": "ERROR", "error": str(error)})
    return to_json({
        "status": "ANCHOR_LOCKED",
        "case_id": anchor["case_id"],
        "primary_name": anchor["primary_name"],
        "aliases": anchor["known_aliases"],
        "hints": anchor.get("hints", {}),
        "hypotheses": anchor["initial_hypotheses"],
        "wide_net_queries": core.wide_net_queries(anchor),
    })


@mcp.tool()
async def harvest_sources(
    case_id: str,
    query: str,
    max_results: int = 8,
    strict_relevance: bool = True,
    enrich_apis: bool = False,
) -> str:
    """
    Phase 2: Searches public web sources for an existing 12-hex case_id, honours robots.txt,
    queues outbound links for traverse_links, and ingests SHA-256 deduplicated evidence.
    """
    if not query.strip():
        return to_json({"status": "ERROR", "error": "query must not be empty (use 2-5 bare keywords)."})
    try:
        anchor, store = open_case(case_id)
    except (ValueError, FileNotFoundError) as error:
        return case_error_json(error, case_id)
    report = await asyncio.to_thread(
        core.collector.harvest, store, query.strip(), max(1, min(max_results, 20)), strict_relevance, anchor
    )
    if enrich_apis:
        report["api_enrichment"] = await asyncio.to_thread(core.collector.enrich_public_apis, store, anchor)
    return to_json({**report, "case_id": store.case_id, "total_artifacts": store.statistics()["evidence_total"]})


@mcp.tool()
async def ingest_url(case_id: str, url: str) -> str:
    """Phase 2 (targeted): Fetches a single public URL (robots.txt honoured) and queues its outbound links."""
    try:
        anchor, store = open_case(case_id)
    except (ValueError, FileNotFoundError) as error:
        return case_error_json(error, case_id)
    canonical = canonicalize_url(url)

    def ingest() -> Dict[str, Any]:
        _, text, method, artifact, links = core.collector.fetch_document(canonical, store.evidence_directory)
        added = bool(text) and store.add_evidence(canonical, text, method, "manual_ingest", artifact)
        queued = store.queue_links(links, canonical, 1, anchor) if links else 0
        store.audit("INGEST_URL", {"url": canonical, "method": method, "added": added, "links_queued": queued})
        return {"url": canonical, "method": method, "added": added, "characters": len(text or ""), "links_queued": queued}

    return to_json(await asyncio.to_thread(ingest))


@mcp.tool()
async def traverse_links(case_id: str, max_depth: int = MAX_TRAVERSAL_DEPTH, max_pages: int = MAX_TRAVERSAL_PAGES) -> str:
    """
    Phase 2B: Opens queued outbound links (/projects, /resume, GitHub, ORCID, Credly,
    deployed apps), extracts temporal proof, and promotes reciprocal links to two_directional.
    """
    try:
        anchor, store = open_case(case_id)
    except (ValueError, FileNotFoundError) as error:
        return case_error_json(error, case_id)
    deadline = time.monotonic() + DEFAULT_CYCLE_SECONDS
    report = await asyncio.to_thread(core.collector.crawl_links, store, anchor, max_depth, max_pages, deadline)
    return to_json({**report, "case_id": store.case_id, "statistics": store.statistics()})


@mcp.tool()
async def get_connection_graph(case_id: str) -> str:
    """Returns the directional JSON connection graph (`one_directional` vs. `two_directional`) for a 12-hex case_id."""
    try:
        anchor, store = open_case(case_id)
    except (ValueError, FileNotFoundError) as error:
        return case_error_json(error, case_id)
    return to_json(store.connection_graph(anchor))


@mcp.tool()
async def record_connection(
    case_id: str,
    source_node: str,
    target_node: str,
    relation: str = "knows",
    direction: str = "out",
    evidence_tag: str = "",
    confidence: float = 0.70,
) -> str:
    """
    Records a directed (`out`) or mutual (`bidirectional`) connection edge in the case graph.
    If the reverse edge (`target_node -> source_node`) already exists, automatically promotes
    the pair to `two_directional` (`bidirectional`).
    """
    try:
        anchor, store = open_case(case_id)
    except (ValueError, FileNotFoundError) as error:
        return case_error_json(error, case_id)
    outcome = store.add_connection(source_node, target_node, relation, direction, confidence, evidence_tag)
    store.audit("CONNECTION_RECORDED", {
        "from": source_node, "to": target_node, "relation": relation, "outcome": outcome,
    })
    return to_json({"status": outcome, "graph": store.connection_graph(anchor)})


@mcp.tool()
async def run_analysis(case_id: str, max_passes: int = 1) -> str:
    """
    Phase 3: Grounded ACH, NATO Admiralty claim rating, entity/timeline extraction,
    and pivot generation for an existing 12-hex case_id.
    """
    try:
        anchor, store = open_case(case_id)
    except (ValueError, FileNotFoundError) as error:
        return case_error_json(error, case_id)
    deadline = time.monotonic() + DEFAULT_CYCLE_SECONDS
    state = await asyncio.to_thread(core.run_analysis, anchor, store, max_passes, deadline)
    return to_json({
        "case_id": store.case_id,
        "confidence": state["confidence"],
        "confidence_band": core.confidence_label(state["confidence"]),
        "passes_completed": state["passes_completed"],
        "delta_vs_previous": state.get("delta_vs_previous", {}),
        "hypotheses": state["hypotheses"],
        "entities": state["entities"],
        "claims_rated": len(state["claims"]),
        "claims": state["claims"][:20],
        "intelligence_gaps": state["intelligence_gaps"],
        "recommended_pivots": state["recommended_pivots"],
        "evidence_excerpts": store.evidence_excerpts(anchor, limit=6),
    })


@mcp.tool()
async def record_findings(
    case_id: str,
    claims: Optional[List[Dict[str, Any]]] = None,
    entities: Optional[List[Dict[str, Any]]] = None,
    hypotheses: Optional[List[Dict[str, Any]]] = None,
    intelligence_gaps: Optional[List[str]] = None,
    key_assumptions: Optional[List[str]] = None,
    confidence: Optional[float] = None,
) -> str:
    """
    Commits structured ACH claims/entities into an existing 12-hex case_id with server-side
    verification against stored [E:<hash>] evidence tags.
    """
    try:
        anchor, store = open_case(case_id)
    except (ValueError, FileNotFoundError) as error:
        return case_error_json(error, case_id)
    current = store.analysis_state()
    merged = {
        "confidence": current["confidence"] if confidence is None else confidence,
        "hypotheses": hypotheses if hypotheses is not None else current["hypotheses"],
        "claims": claims if claims is not None else current["claims"],
        "entities": entities if entities is not None else current["entities"],
        "intelligence_gaps": intelligence_gaps if intelligence_gaps is not None else current["intelligence_gaps"],
        "key_assumptions": key_assumptions if key_assumptions is not None else current["key_assumptions"],
        "pivot_queries": current["recommended_pivots"],
    }
    state = await asyncio.to_thread(store.replace_analysis, merged, max(1, current["passes_completed"]), anchor)
    store.audit("HOST_FINDINGS_RECORDED", {"claims": len(state["claims"]), "confidence": state["confidence"]})
    return to_json({
        "status": "FINDINGS_VERIFIED",
        "case_id": store.case_id,
        "confidence": state["confidence"],
        "delta_vs_previous": state.get("delta_vs_previous", {}),
        "verified_claims": len(state["claims"]),
        "entities": len(state["entities"]),
        "intelligence_gaps": state["intelligence_gaps"],
    })


@mcp.tool()
async def synthesize_dossier(case_id: str, analyst_narrative: str = "") -> str:
    """
    Phase 4 (final step after anchor/harvest/analysis): Compiles an ICD-203 Markdown dossier
    with SHA-256 chain-of-custody appendices for an existing 12-hex case_id.
    """
    try:
        anchor, store = open_case(case_id)
    except (ValueError, FileNotFoundError) as error:
        return case_error_json(error, case_id)
    path = await asyncio.to_thread(core.synthesize, anchor, store, analyst_narrative)
    state = store.analysis_state()
    return to_json({
        "status": "DOSSIER_READY",
        "case_id": store.case_id,
        "confidence": state["confidence"],
        "confidence_band": core.confidence_label(state["confidence"]),
        "delta_vs_previous": state.get("delta_vs_previous", {}),
        "intelligence_gaps": state["intelligence_gaps"],
        "dossier_path": str(path),
        "resource": f"specter://cases/{store.case_id}/dossier",
    })


@mcp.tool()
async def run_full_cycle(
    seed: str,
    purpose: str = "Open-source public intelligence investigation",
    context: Context = None,
    max_results: int = 6,
    max_seconds: float = DEFAULT_CYCLE_SECONDS,
    aliases: Optional[List[str]] = None,
    location: str = "",
    profession: str = "",
) -> str:
    """
    Investigate, spy on, research, or compile a full intelligence dossier on a target person,
    organization, or domain in ONE call (<= 50s). Creates a 12-hex case_id, queries web search,
    GitHub, and Wikipedia, crawls discovered links, evaluates hypotheses (ACH), and builds the dossier.
    """
    started = time.monotonic()
    budget = max(12.0, min(float(max_seconds), 50.0))
    deadline = started + budget
    harvest_reserve = min(14.0, budget * 0.4)
    loop = asyncio.get_running_loop()

    async def report_phase(step: int, message: str) -> None:
        if context is None:
            return
        try:
            await context.info(message)
            await context.report_progress(step, 4)
        except Exception:
            pass

    def notify(message: str) -> None:
        if context is None:
            return
        try:
            asyncio.run_coroutine_threadsafe(context.info(message), loop)
        except Exception:
            pass

    try:
        await report_phase(0, "Phase 1: locking Immutable Context Anchor...")
        anchor = await asyncio.to_thread(core.build_anchor, seed, purpose, aliases, None, location, profession)
    except ValueError as error:
        return to_json({"status": "ERROR", "error": str(error)})
    store = CaseStore(anchor["case_id"])

    await report_phase(1, f"Phase 2A: wide-net harvest + public API enrichment for {anchor['primary_name']}...")
    capped_results = max(1, min(max_results, 12))
    await asyncio.to_thread(core.collector.enrich_public_apis, store, anchor)
    for index, query in enumerate(core.wide_net_queries(anchor)):
        if index > 0 and time.monotonic() >= deadline - harvest_reserve:
            break
        if time.monotonic() >= deadline - 6.0:
            break
        await asyncio.to_thread(core.collector.harvest, store, query, capped_results, True, anchor)

    await report_phase(2, "Phase 2B: Links -> Open -> spy -> Data bounded traversal (Repeate loop)...")
    traversal = await asyncio.to_thread(core.collector.crawl_links, store, anchor, MAX_TRAVERSAL_DEPTH, MAX_TRAVERSAL_PAGES, deadline)

    await report_phase(3, f"Phase 3: grounded ACH + directional graph ({store.statistics()['evidence_total']} artifacts)...")
    state = await asyncio.to_thread(core.run_analysis, anchor, store, MAX_RESEARCH_PASSES, deadline, notify)

    await report_phase(4, "Phase 4: compiling ICD-203 dossier & directional graph appendices...")
    dossier_path = await asyncio.to_thread(core.synthesize, anchor, store)
    graph = store.connection_graph(anchor)
    latest_activity = core.extract_latest_activity(store.evidence_rows())

    compact_graph = {
        "two_directional": graph["connections"]["two_directional"][:10],
        "one_directional": graph["connections"]["one_directional"][:12],
        "one_directional_total": len(graph["connections"]["one_directional"]),
    }

    return to_json({
        "status": "COMPLETE",
        "elapsed_seconds": round(time.monotonic() - started, 2),
        "case_id": store.case_id,
        "primary_name": anchor["primary_name"],
        "analytic_confidence": state["confidence"],
        "confidence_band": core.confidence_label(state["confidence"]),
        "delta_vs_previous": state.get("delta_vs_previous", {}),
        "latest_activity_utc": latest_activity or None,
        "traversal": traversal,
        "statistics": store.statistics(),
        "dossier_path": str(dossier_path),
        "resource": f"specter://cases/{store.case_id}/dossier",
        "graph_resource": f"specter://cases/{store.case_id}/graph",
        "connection_graph": compact_graph,
        "entities": state["entities"][:15],
        "top_claims": state["claims"][:12],
        "intelligence_gaps": state["intelligence_gaps"],
        "recommended_pivots": state["recommended_pivots"],
        "evidence_excerpts": store.evidence_excerpts(anchor, limit=4, excerpt_chars=450),
    })


@mcp.tool()
async def case_status(case_id: str) -> str:
    """Summarizes an existing 12-hex case_id: anchor, evidence stats, confidence, gaps, and audit log."""
    try:
        anchor, store = open_case(case_id)
    except (ValueError, FileNotFoundError) as error:
        return case_error_json(error, case_id)
    state = store.analysis_state()
    return to_json({
        "case_id": store.case_id,
        "primary_name": anchor["primary_name"],
        "purpose": anchor.get("purpose"),
        "created_utc": anchor.get("created_utc"),
        "statistics": store.statistics(),
        "confidence": state["confidence"],
        "confidence_band": core.confidence_label(state["confidence"]),
        "passes_completed": state["passes_completed"],
        "delta_vs_previous": state.get("delta_vs_previous", {}),
        "intelligence_gaps": state["intelligence_gaps"],
        "recommended_pivots": state["recommended_pivots"],
        "dossier_ready": store.dossier_path.exists(),
        "recent_audit": store.audit_log(10),
    })


@mcp.tool()
async def list_cases() -> str:
    """Lists all existing investigation cases on disk with their 12-hex case_id and target name."""
    return to_json(await asyncio.to_thread(list_case_summaries))


@mcp.tool()
async def search_evidence(case_id: str, keyword: str, limit: int = 10) -> str:
    """Keyword search across collected case evidence for a 12-hex case_id; returns tagged excerpts."""
    if not keyword.strip():
        return to_json({"status": "ERROR", "error": "keyword must not be empty."})
    try:
        _, store = open_case(case_id)
    except (ValueError, FileNotFoundError) as error:
        return case_error_json(error, case_id)
    return to_json(await asyncio.to_thread(store.search_evidence, keyword.strip(), max(1, min(limit, 50))))


@mcp.tool()
async def scan_geo_events(
    latitude: float,
    longitude: float,
    radius_km: float = 25.0,
    days: int = 2,
    place_name: str = "",
) -> str:
    """
    Geo-sensory fusion: NASA FIRMS thermal anomalies + USGS seismic events within radius_km
    over the last `days`, optionally correlated with regional news for `place_name`.
    """
    if not (-90 <= latitude <= 90 and -180 <= longitude <= 180):
        return to_json({"status": "ERROR", "error": "Coordinates out of range (-90..90, -180..180)."})
    radius_km, days = max(1.0, min(radius_km, 500.0)), max(1, min(days, 30))

    def scan() -> Dict[str, Any]:
        report: Dict[str, Any] = {"center": [latitude, longitude], "radius_km": radius_km, "days": days}
        probes = {
            "thermal": lambda: GeoSensor.thermal_hotspots(latitude, longitude, radius_km, days),
            "seismic": lambda: GeoSensor.seismic_events(latitude, longitude, radius_km, days),
            "news": lambda: GeoSensor.regional_news(place_name),
        }
        for key, probe in probes.items():
            try:
                report[key] = probe()
            except (requests.RequestException, ValueError) as error:
                report[key] = {"status": "error", "reason": str(error)}
        return report

    return to_json(await asyncio.to_thread(scan))


@mcp.resource("specter://cases/{case_id}/anchor")
def read_anchor(case_id: str) -> str:
    """Immutable JSON Context Anchor for a case."""
    return to_json(load_anchor(case_id))


@mcp.resource("specter://cases/{case_id}/dossier")
def read_dossier(case_id: str) -> str:
    """Generated Markdown intelligence dossier for a case."""
    _, store = open_case(case_id)
    if not store.dossier_path.exists():
        raise FileNotFoundError(f"No dossier yet for case '{store.case_id}'. Run synthesize_dossier.")
    return store.dossier_path.read_text(encoding="utf-8")


@mcp.resource("specter://cases/{case_id}/audit")
def read_audit(case_id: str) -> str:
    """Full audit trail (accountability log) for a case."""
    _, store = open_case(case_id)
    return to_json(store.audit_log())


@mcp.resource("specter://cases/{case_id}/graph")
def read_graph(case_id: str) -> str:
    """Page B directional JSON connection graph (`one_directional` vs. `two_directional`)."""
    anchor, store = open_case(case_id)
    return to_json(store.connection_graph(anchor))


if __name__ == "__main__":
    mcp.run()
