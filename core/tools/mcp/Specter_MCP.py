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
Specter MCP — Sovereign All-Source OSINT Intelligence Server (Project Axiom / Spector).

Fuses Spy.py (MCP surface), We_Know/orchestrator.py (recursive pipeline) and
We_Know/pure.md (tradecraft upgrade plan + Spector roadmap) into one server:

  Phase 1  Immutable Context Anchor + competing hypotheses (H1 target vs H2 namesake)
  Phase 2  Multi-modal harvest: search -> C++23 native / Python document extraction
           -> Trafilatura Markdown -> Playwright + VLM vision fallback (ComfyUI 4x upscale)
  Phase 3  Think_2 sync hub: ACH + NATO Admiralty ratings, Research_req pivots (hard cap: 2 passes)
  Phase 4  ICD-203 dossier + deterministic chain-of-custody appendix (SHA-256, UTC)
  Sensors  NASA FIRMS thermal anomalies + USGS seismic events + regional news correlation

Guardrails: public open sources only, robots.txt honoured, per-domain rate limit,
CAPTCHA / anti-bot / login gates are skipped (never bypassed), every action is audit-logged,
sensitive personal identifiers are excluded and redacted from dossiers.

All logs go to stderr: stdout is reserved for MCP stdio JSON-RPC framing.
"""

import asyncio
import base64
import csv
import ctypes
import hashlib
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
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional
from urllib.parse import parse_qsl, urlencode, urlparse, urlunparse
from urllib.robotparser import RobotFileParser

import requests
try:  # mcp 2.x renamed FastMCP -> MCPServer (same decorator / Context surface used here)
    from mcp.server.mcpserver import Context, MCPServer as FastMCP
except ImportError:  # mcp 1.x
    from mcp.server.fastmcp import Context, FastMCP

logging.basicConfig(
    level=os.getenv("SPECTER_LOG_LEVEL", "INFO"),
    format="%(asctime)s [%(levelname)s] %(message)s",
    stream=sys.stderr,
)
log = logging.getLogger("specter")

# --- Optional dependencies (server still boots without them) -------------------
try:
    import trafilatura
except ImportError:
    trafilatura = None

warnings.filterwarnings("ignore", category=RuntimeWarning, module="duckduckgo_search")
try:
    from ddgs import DDGS
except ImportError:
    try:
        from duckduckgo_search import DDGS
    except ImportError:
        DDGS = None

# ==============================================================================
# 0. CONFIGURATION (every value overridable via environment)
# ==============================================================================
SCRIPT_DIR = Path(__file__).resolve().parent
OLLAMA_BASE = os.getenv("SPECTER_OLLAMA_URL", "http://127.0.0.1:11434").rstrip("/")
OLLAMA_GENERATE_URL = f"{OLLAMA_BASE}/api/generate"
OLLAMA_RUNNING_URL = f"{OLLAMA_BASE}/api/ps"
OLLAMA_TAGS_URL = f"{OLLAMA_BASE}/api/tags"
COMFYUI_URL = os.getenv("SPECTER_COMFYUI_URL", "http://127.0.0.1:8188").rstrip("/")
UPSCALE_MODEL = os.getenv("SPECTER_UPSCALE_MODEL", "4x_NMKD-Siax_200k.pth")
ANALYST_MODEL = os.getenv("SPECTER_ANALYST_LLM", "eriktonon/qwen3.8-uncensored:9b")
OBSERVER_MODEL = os.getenv("SPECTER_OBSERVER_VLM", "huihui_ai/gemma-4-abliterated:12b-qat")
CONTEXT_TOKENS = int(os.getenv("SPECTER_NUM_CTX", "32768"))
FIRMS_MAP_KEY = os.getenv("SPECTER_FIRMS_KEY", "")
FIRMS_SOURCE = os.getenv("SPECTER_FIRMS_SOURCE", "VIIRS_SNPP_NRT")

SPECTER_HOME = Path(os.getenv("SPECTER_HOME", str(SCRIPT_DIR / "specter_data")))
CASES_DIR = SPECTER_HOME / "cases"
AXIOM_DLL = Path(os.getenv("SPECTER_AXIOM_DLL", str(SCRIPT_DIR.parents[2] / "build" / "axiom_core.dll")))

USER_AGENT = "Mozilla/5.0 (compatible; SpecterOSINT/3.0; public-source research; honours robots.txt)"
CHARS_PER_TOKEN = 3.0              # conservative budget for URL/Markdown-dense text
MAX_RESEARCH_PASSES = 2            # Spector hard loop guard (Research_req node)
PIN_MODELS = os.getenv("SPECTER_PIN_MODELS", "0") == "1"
DOMAIN_INTERVAL_SECONDS = float(os.getenv("SPECTER_DOMAIN_INTERVAL", "1.5"))
MAX_DOWNLOAD_BYTES = 50 * 1024 * 1024
MAX_STORED_CHARS = 100_000
MIN_ARTICLE_CHARS = 200
NATIVE_EXTENSIONS = {".pdf", ".docx", ".wav", ".ogg", ".mp3"}
TRACKING_PARAMETERS = {"utm_source", "utm_medium", "utm_campaign", "utm_term", "utm_content", "fbclid", "gclid", "mc_eid"}
CASE_ID_PATTERN = re.compile(r"^[a-f0-9]{12}$")
STOP_WORDS = {"the", "and", "for", "with", "from", "that", "this", "site", "streamer", "switzerland", "profile"}

CASES_DIR.mkdir(parents=True, exist_ok=True)

ANALYTIC_RULES = """Analytic & ethical rules (mandatory):
- Use ONLY the supplied public evidence. Never invent facts, URLs, dates or hashes.
- Cite evidence inline with its tag, e.g. [E:1a2b3c4d5e].
- Never merge namesake / collision evidence into the primary target profile.
- EXCLUDE private residential addresses, personal phone numbers, government ID numbers,
  financial account data, medical information, and any details about minors."""

# ==============================================================================
# 1. PURE HELPERS
# ==============================================================================
SENSITIVE_PATTERNS = [
    (re.compile(r"\b\d{3}-\d{2}-\d{4}\b"), "[REDACTED-ID]"),
    (re.compile(r"\b(?:\d{4}[ -]){3}\d{4}\b"), "[REDACTED-CARD]"),
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


def parse_json(raw_text: str) -> Dict[str, Any]:
    cleaned = strip_reasoning(raw_text)
    start, end = cleaned.find("{"), cleaned.rfind("}")
    if start == -1 or end <= start:
        return {}
    try:
        parsed = json.loads(cleaned[start : end + 1])
    except json.JSONDecodeError:
        return {}
    return parsed if isinstance(parsed, dict) else {}


def canonicalize_url(url: str) -> str:
    try:
        parsed = urlparse(url.strip())
        clean_query = [(key, value) for key, value in parse_qsl(parsed.query) if key.lower() not in TRACKING_PARAMETERS]
        return urlunparse((parsed.scheme, parsed.netloc.lower(), parsed.path, parsed.params, urlencode(clean_query), ""))
    except ValueError:
        return url.strip()


def validate_case_id(case_id: str) -> str:
    """Guards every filesystem path built from client input (no traversal)."""
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
    return [value.strip() for value in values if isinstance(value, str) and value.strip()][:limit]


def char_budget(fixed_chars: int, reserve_tokens: int) -> int:
    """Characters left for evidence after fixed prompt text and reserved output tokens."""
    return max(4000, int((CONTEXT_TOKENS - reserve_tokens) * CHARS_PER_TOKEN) - fixed_chars)


def anchor_terms(anchor: Optional[Dict[str, Any]], query: str = "") -> List[str]:
    """Extracts lowercase target identifiers for relevance filtering and claim grounding."""
    terms: List[str] = []
    if anchor:
        candidates = (
            [anchor.get("primary_name", "")]
            + list(anchor.get("known_aliases", []))
            + list(anchor.get("known_handles_or_domains", []))
        )
        for item in candidates:
            value = str(item or "").strip().lower()
            if len(value) >= 3:
                terms.append(value)
    if not terms and query:
        for token in re.findall(r"[a-zA-Z0-9_.-]{3,}", re.sub(r"\bsite:\S+", "", query)):
            lowered = token.lower()
            if lowered not in STOP_WORDS and lowered not in ("and", "not"):
                terms.append(lowered)
    return list(dict.fromkeys(terms))


def matches_terms(text: str, terms: List[str], url: str = "") -> bool:
    """Returns True when text or URL contains at least one target/query identifier."""
    if not terms:
        return True
    haystack = f"{url}\n{text}".lower()
    return any(term in haystack for term in terms)


# ==============================================================================
# 2. POLITENESS GATE (robots.txt + per-domain rate limiting)
# ==============================================================================
class PolitenessGate:
    def __init__(self) -> None:
        self._robots: Dict[str, Optional[RobotFileParser]] = {}
        self._last_hit: Dict[str, float] = {}
        self._lock = threading.Lock()

    @staticmethod
    def _load_robots(root: str) -> Optional[RobotFileParser]:
        """RFC 9309: 2xx parse, 4xx allow all, 5xx / unreachable disallow all."""
        parser = RobotFileParser()
        try:
            response = requests.get(f"{root}/robots.txt", timeout=8, headers={"User-Agent": USER_AGENT})
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
        domain = urlparse(url).netloc
        with self._lock:
            now = time.monotonic()
            slot = max(now, self._last_hit.get(domain, 0.0) + DOMAIN_INTERVAL_SECONDS)
            self._last_hit[domain] = slot
        if slot > now:
            time.sleep(slot - now)


# ==============================================================================
# 3. NATIVE C++23 BRIDGE (+ pure-Python fallback extractors)
# ==============================================================================
class AxiomNativeBridge:
    """ctypes bridge to Project Axiom extractors; dynamic buffer when the DLL exports a size probe."""

    def __init__(self, dll_path: Path) -> None:
        self.dll = None
        self.has_probe = False
        for search_dir in (dll_path.parent, dll_path.parent.parent / "core", Path(r"C:\msys64\ucrt64\bin")):
            if search_dir.exists():
                try:
                    os.add_dll_directory(str(search_dir.resolve()))
                except (OSError, AttributeError):
                    pass
        if not dll_path.exists():
            log.info("Native DLL not found at %s; using Python extractors.", dll_path)
            return
        try:
            self.dll = ctypes.CDLL(str(dll_path.resolve()))
            self.dll.axiom_extract_document.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
            self.dll.axiom_extract_document.restype = ctypes.c_int
            if hasattr(self.dll, "axiom_required_size"):
                self.dll.axiom_required_size.argtypes = [ctypes.c_char_p]
                self.dll.axiom_required_size.restype = ctypes.c_int
                self.has_probe = True
            log.info("Bound native C++23 engine: %s (size probe: %s)", dll_path.name, self.has_probe)
        except (OSError, AttributeError) as error:
            self.dll = None
            log.warning("Failed to bind DLL %s: %s", dll_path, error)

    def _extract_native(self, path: Path) -> str:
        if not self.dll:
            return ""
        encoded_path = str(path).encode("utf-8")
        capacity = 8 * 1024 * 1024
        if self.has_probe:
            required = self.dll.axiom_required_size(encoded_path)
            if required <= 0:
                return ""
            capacity = required + 1
        buffer = ctypes.create_string_buffer(capacity)
        bytes_read = self.dll.axiom_extract_document(encoded_path, buffer, capacity)
        return buffer.value.decode("utf-8", errors="replace") if bytes_read > 0 else ""

    @staticmethod
    def _extract_python(path: Path) -> str:
        extension = path.suffix.lower()
        try:
            if extension == ".pdf":
                from pypdf import PdfReader
                return "\n".join(page.extract_text() or "" for page in PdfReader(str(path)).pages)
            if extension == ".docx":
                with zipfile.ZipFile(path) as archive:
                    xml = archive.read("word/document.xml").decode("utf-8", errors="replace")
                return re.sub(r"<[^>]+>", "", xml.replace("</w:p>", "\n"))
        except ImportError:
            log.debug("pypdf not installed; cannot extract %s", path.name)
        except Exception as error:  # third-party parsers raise heterogeneous errors on malformed files
            log.debug("Python extraction failed for %s: %s", path.name, error)
        return ""

    def extract_document(self, path: Path) -> str:
        return (self._extract_native(path) or self._extract_python(path)).strip()


# ==============================================================================
# 4. VRAM RESIDENCY CONTROLLER (thread-safe, single model resident on 12 GB)
# ==============================================================================
class VRAMController:
    _active_model: str = ""
    _lock = threading.RLock()

    @staticmethod
    def resident_models() -> List[str]:
        try:
            response = requests.get(OLLAMA_RUNNING_URL, timeout=5)
            if response.ok:
                return [entry.get("name", "") for entry in response.json().get("models", []) if entry.get("name")]
        except (requests.RequestException, ValueError):
            pass
        return []

    @classmethod
    def pick_model(cls, preferred_model: str, require_vision: bool = False) -> str:
        """Reuses an already-resident Ollama model when not pinned, avoiding 12 GB unload/reload ping-pong."""
        if PIN_MODELS or require_vision:
            return preferred_model
        loaded = cls.resident_models()
        if preferred_model in loaded:
            return preferred_model
        return loaded[0] if loaded else preferred_model

    @classmethod
    def switch_to(cls, target_model: str) -> None:
        with cls._lock:
            evict_targets = {cls._active_model} if cls._active_model and cls._active_model != target_model else set()
            for loaded in cls.resident_models():
                if loaded != target_model:
                    evict_targets.add(loaded)
            for loaded in evict_targets:
                log.info("[VRAM Guard] Evicting '%s'", loaded)
                try:
                    requests.post(OLLAMA_GENERATE_URL, json={"model": loaded, "keep_alive": 0}, timeout=15)
                except requests.RequestException:
                    pass
            cls._active_model = target_model

    @classmethod
    def evict_models(cls) -> None:
        cls.switch_to("")

    @classmethod
    def query_llm(
        cls,
        model: str,
        prompt: str,
        json_mode: bool = False,
        images_base64: Optional[List[str]] = None,
        temperature: float = 0.2,
    ) -> str:
        selected_model = cls.pick_model(model, require_vision=bool(images_base64))
        payload: Dict[str, Any] = {
            "model": selected_model,
            "prompt": prompt,
            "stream": False,
            "keep_alive": "15m",
            "options": {"temperature": temperature, "num_ctx": CONTEXT_TOKENS},
        }
        if json_mode:
            payload["format"] = "json"
        if images_base64:
            payload["images"] = images_base64
        last_error: Optional[Exception] = None
        with cls._lock:
            cls.switch_to(selected_model)
            for attempt in range(2):
                try:
                    response = requests.post(OLLAMA_GENERATE_URL, json=payload, timeout=(5, 600))
                    response.raise_for_status()
                    return strip_reasoning(response.json().get("response", ""))
                except (requests.RequestException, ValueError) as error:
                    last_error = error
                    log.warning("[LLM] %s attempt %d failed: %s", selected_model, attempt + 1, error)
                    time.sleep(2)
        raise RuntimeError(f"Ollama query failed for '{selected_model}' at {OLLAMA_BASE}: {last_error}")


# ==============================================================================
# 5. COMFYUI FORENSIC UPSCALER (polls /history, downloads real output)
# ==============================================================================
class ComfyUIBridge:
    @staticmethod
    def upscale_image(input_path: Path, timeout_seconds: int = 60) -> Optional[Path]:
        if not input_path.exists():
            return None
        try:
            VRAMController.evict_models()
            with open(input_path, "rb") as image_file:
                upload = requests.post(f"{COMFYUI_URL}/upload/image", files={"image": image_file}, timeout=15)
            upload.raise_for_status()
            uploaded_name = upload.json().get("name", input_path.name)
            workflow = {
                "1": {"inputs": {"image": uploaded_name, "upload": "image"}, "class_type": "LoadImage"},
                "2": {"inputs": {"model_name": UPSCALE_MODEL}, "class_type": "UpscaleModelLoader"},
                "3": {"inputs": {"upscale_model": ["2", 0], "image": ["1", 0]}, "class_type": "ImageUpscaleWithModel"},
                "4": {"inputs": {"filename_prefix": "Specter_Forensic", "images": ["3", 0]}, "class_type": "SaveImage"},
            }
            queued = requests.post(f"{COMFYUI_URL}/prompt", json={"prompt": workflow}, timeout=20)
            queued.raise_for_status()
            prompt_id = queued.json().get("prompt_id")
            if not prompt_id:
                return None
            deadline = time.monotonic() + timeout_seconds
            output_info = None
            while time.monotonic() < deadline and not output_info:
                history = requests.get(f"{COMFYUI_URL}/history/{prompt_id}", timeout=10)
                if history.ok:
                    images = history.json().get(prompt_id, {}).get("outputs", {}).get("4", {}).get("images", [])
                    output_info = images[0] if images else None
                if not output_info:
                    time.sleep(0.75)
            if not output_info:
                log.warning("[ComfyUI] Upscale timed out (prompt_id=%s)", prompt_id)
                return None
            view = requests.get(
                f"{COMFYUI_URL}/view",
                params={
                    "filename": output_info["filename"],
                    "subfolder": output_info.get("subfolder", ""),
                    "type": output_info.get("type", "output"),
                },
                timeout=30,
            )
            view.raise_for_status()
            input_path.write_bytes(view.content)
            return input_path
        except (requests.RequestException, OSError, ValueError, KeyError) as error:
            log.info("[ComfyUI Bypass] %s", error)
            return None
        finally:
            try:
                requests.post(f"{COMFYUI_URL}/free", json={"unload_models": True, "free_memory": True}, timeout=10)
            except requests.RequestException:
                pass


# ==============================================================================
# 6. CASE STORE (SQLite knowledge graph + chain of custody + audit log)
# ==============================================================================
class CaseStore:
    def __init__(self, case_id: str) -> None:
        self.case_id = validate_case_id(case_id)
        self.directory = CASES_DIR / self.case_id
        self.evidence_dir = self.directory / "evidence"
        self.anchor_path = self.directory / "anchor.json"
        self.dossier_path = self.directory / "dossier.md"
        self.database_path = self.directory / "case.db"
        self.evidence_dir.mkdir(parents=True, exist_ok=True)
        self._init_schema()

    def _connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(self.database_path, timeout=30)
        connection.row_factory = sqlite3.Row
        return connection

    def _init_schema(self) -> None:
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
                CREATE TABLE IF NOT EXISTS analysis_meta (key TEXT PRIMARY KEY, value TEXT);
                CREATE TABLE IF NOT EXISTS audit (
                    audit_id INTEGER PRIMARY KEY AUTOINCREMENT, utc TEXT NOT NULL, action TEXT NOT NULL, detail TEXT);
                """
            )

    # --- evidence -------------------------------------------------------------
    def add_evidence(self, url: str, content: str, method: str, query: str = "", artifact_path: str = "") -> bool:
        cleaned = (content or "").strip()[:MAX_STORED_CHARS]
        if not cleaned:
            return False
        with closing(self._connect()) as connection, connection:
            cursor = connection.execute(
                "INSERT OR IGNORE INTO evidence VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                (sha256_text(cleaned), url, urlparse(url).netloc, method, query, utc_now(), artifact_path, cleaned),
            )
            return cursor.rowcount > 0

    def evidence_rows(self) -> List[Dict[str, Any]]:
        with closing(self._connect()) as connection:
            return [dict(row) for row in connection.execute("SELECT * FROM evidence ORDER BY collected_utc, rowid")]

    def search_evidence(self, keyword: str, limit: int) -> List[Dict[str, Any]]:
        pattern = f"%{keyword}%"
        with closing(self._connect()) as connection:
            rows = connection.execute(
                "SELECT content_hash, url, method, collected_utc, content FROM evidence "
                "WHERE content LIKE ? OR url LIKE ? LIMIT ?",
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

    # --- queries --------------------------------------------------------------
    def executed_queries(self) -> set:
        with closing(self._connect()) as connection:
            return {row["query"].lower() for row in connection.execute("SELECT query FROM queries")}

    def record_query(self, query: str, new_artifacts: int) -> None:
        with closing(self._connect()) as connection, connection:
            connection.execute("INSERT OR REPLACE INTO queries VALUES (?, ?, ?)", (query, utc_now(), new_artifacts))

    # --- analysis -------------------------------------------------------------
    def _ground_analysis(self, analysis: Dict[str, Any], anchor: Optional[Dict[str, Any]]) -> Dict[str, Any]:
        """Prevents Anchor-echo hallucinations by verifying claims against stored evidence text."""
        rows = self.evidence_rows()
        evidence_by_tag = {f"E:{row['content_hash'][:10]}": row for row in rows}
        terms = anchor_terms(anchor)
        matched_rows = sum(1 for row in rows if matches_terms(row["content"], terms, row["url"]))

        try:
            confidence = float(analysis.get("confidence", 0.0) or 0.0)
        except (TypeError, ValueError):
            confidence = 0.0

        verified_claims: List[Dict[str, Any]] = []
        rejected_claims = 0
        for claim in analysis.get("claims", []):
            if not isinstance(claim, dict):
                continue
            tag = str(claim.get("evidence_tag", "")).strip()
            row = evidence_by_tag.get(tag)
            if not row or not matches_terms(row["content"], terms, row["url"]):
                rejected_claims += 1
                continue
            verified_claims.append(claim)

        gaps = clean_strings(analysis.get("intelligence_gaps"), 12)
        if matched_rows == 0:
            confidence = min(confidence, 0.10)
            notice = "Zero collected evidence artifacts mention the target identifiers."
            if notice not in gaps:
                gaps.insert(0, notice)
        elif matched_rows == 1:
            confidence = min(confidence, 0.55)
        if rejected_claims:
            gaps.append(f"Dropped {rejected_claims} ungrounded claim(s) lacking target-matching evidence.")

        return {
            **analysis,
            "confidence": min(1.0, max(0.0, confidence)),
            "claims": verified_claims,
            "intelligence_gaps": gaps[:12],
        }

    def replace_analysis(self, analysis: Dict[str, Any], pass_number: int, anchor: Optional[Dict[str, Any]] = None) -> None:
        grounded = self._ground_analysis(analysis, anchor)
        hypotheses = [item for item in grounded.get("hypotheses", []) if isinstance(item, dict)]
        claims = [item for item in grounded.get("claims", []) if isinstance(item, dict)]
        entities = [item for item in grounded.get("entities", []) if isinstance(item, dict) and item.get("name")]
        confidence = float(grounded.get("confidence", 0.0) or 0.0)
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
                     str(claim.get("info_credibility", "6"))[:1], str(claim.get("hypothesis_alignment", "AMBIGUOUS"))),
                )
            for entity in entities:
                connection.execute(
                    "INSERT OR REPLACE INTO entities VALUES (?, ?, ?)",
                    (str(entity["name"]), str(entity.get("type", "unknown")), str(entity.get("relation", ""))),
                )
            meta = {
                "confidence": f"{min(1.0, max(0.0, confidence)):.2f}",
                "intelligence_gaps": json.dumps(clean_strings(grounded.get("intelligence_gaps"), 12)),
                "key_assumptions": json.dumps(clean_strings(grounded.get("key_assumptions"), 12)),
                "recommended_pivots": json.dumps(clean_strings(grounded.get("pivot_queries"), 3)),
                "passes_completed": str(pass_number),
                "analyzed_utc": utc_now(),
            }
            connection.executemany("INSERT OR REPLACE INTO analysis_meta VALUES (?, ?)", meta.items())

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
                "hypotheses": [dict(row) for row in connection.execute("SELECT * FROM hypotheses")],
                "claims": [dict(row) for row in connection.execute("SELECT * FROM claims")],
                "entities": [dict(row) for row in connection.execute("SELECT * FROM entities")],
            }

    # --- audit ----------------------------------------------------------------
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
            return {
                "evidence_total": sum(by_method.values()),
                "evidence_by_method": by_method,
                "unique_domains": connection.execute("SELECT COUNT(DISTINCT domain) FROM evidence").fetchone()[0],
                "queries_executed": connection.execute("SELECT COUNT(*) FROM queries").fetchone()[0],
                "claims": connection.execute("SELECT COUNT(*) FROM claims").fetchone()[0],
                "entities": connection.execute("SELECT COUNT(*) FROM entities").fetchone()[0],
            }


def load_anchor(case_id: str) -> Dict[str, Any]:
    """Reads an existing case anchor without creating directories for unknown ids."""
    anchor_path = CASES_DIR / validate_case_id(case_id) / "anchor.json"
    if not anchor_path.exists():
        raise FileNotFoundError(f"No case '{case_id}'. Run anchor_target first.")
    return json.loads(anchor_path.read_text(encoding="utf-8"))


def open_case(case_id: str) -> tuple:
    """Returns (anchor, store) for an existing case."""
    anchor = load_anchor(case_id)
    return anchor, CaseStore(case_id)


# ==============================================================================
# 7. SOURCE COLLECTOR (search -> native -> Markdown -> vision fallback)
# ==============================================================================
class SourceCollector:
    def __init__(self, gate: PolitenessGate, bridge: AxiomNativeBridge) -> None:
        self.gate = gate
        self.bridge = bridge

    @staticmethod
    def search(query: str, max_results: int) -> List[Dict[str, str]]:
        if DDGS is None:
            raise RuntimeError("Search backend missing: pip install ddgs (or duckduckgo_search).")
        hits: List[Dict[str, str]] = []
        seen = set()
        try:
            with DDGS() as search_client:
                for item in search_client.text(query, region="wt-wt", max_results=max_results) or []:
                    canonical = canonicalize_url(item.get("href", ""))
                    if canonical and canonical not in seen:
                        seen.add(canonical)
                        hits.append({
                            "url": canonical,
                            "snippet": f"{item.get('title', '')}\n{item.get('body', '')}".strip(),
                        })
        except Exception as error:  # search backends raise many custom exception types
            log.warning("[Search] '%s' failed: %s", query, error)
        return hits

    def _download_native(self, url: str, extension: str, evidence_dir: Path) -> tuple:
        local_file = evidence_dir / f"doc_{uuid.uuid4().hex[:10]}{extension}"
        with requests.get(url, timeout=30, stream=True, headers={"User-Agent": USER_AGENT}) as response:
            response.raise_for_status()
            written = 0
            with open(local_file, "wb") as handle:
                for chunk in response.iter_content(chunk_size=65536):
                    written += len(chunk)
                    if written > MAX_DOWNLOAD_BYTES:
                        raise ValueError(f"Artifact exceeds {MAX_DOWNLOAD_BYTES} bytes")
                    handle.write(chunk)
        return self.bridge.extract_document(local_file), str(local_file)

    def fetch_document(self, url: str, evidence_dir: Path) -> tuple:
        """Returns (url, text, method, artifact_path). method 'needs_vision' marks fallback candidates."""
        if not self.gate.allowed(url):
            return url, "", "robots_denied", ""
        self.gate.throttle(url)
        extension = Path(urlparse(url).path).suffix.lower()
        try:
            if extension in NATIVE_EXTENSIONS:
                text, artifact = self._download_native(url, extension, evidence_dir)
                return url, text, ("native_document" if text else "failed"), artifact
            response = requests.get(url, timeout=20, headers={"User-Agent": USER_AGENT})
            if response.status_code in (403, 429, 503):
                return url, "", "needs_vision", ""
            response.raise_for_status()
            if "pdf" in response.headers.get("Content-Type", "").lower():
                local_file = evidence_dir / f"doc_{uuid.uuid4().hex[:10]}.pdf"
                local_file.write_bytes(response.content[:MAX_DOWNLOAD_BYTES])
                text = self.bridge.extract_document(local_file)
                return url, text, ("native_document" if text else "failed"), str(local_file)
            if trafilatura is not None:
                markdown = trafilatura.extract(response.text, output_format="markdown", include_links=True, url=url)
                if markdown and len(markdown.strip()) >= MIN_ARTICLE_CHARS:
                    return url, markdown, "markdown", ""
            return url, "", "needs_vision", ""
        except (requests.RequestException, OSError, ValueError) as error:
            log.debug("[Fetch] %s: %s", url, error)
            return url, "", "failed", ""

    def vision_batch(self, urls: List[str], evidence_dir: Path) -> List[tuple]:
        """One headless browser for all fallback pages; CAPTCHA / login gates are skipped, never bypassed."""
        if not urls:
            return []
        try:
            from playwright.sync_api import sync_playwright
        except ImportError:
            log.info("[Vision] Playwright not installed; skipping %d pages.", len(urls))
            return []
        results: List[tuple] = []
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(headless=True)
            try:
                browser_context = browser.new_context(viewport={"width": 1280, "height": 900}, user_agent=USER_AGENT)
                for url in urls:
                    self.gate.throttle(url)
                    shot_path = evidence_dir / f"snap_{uuid.uuid4().hex[:10]}.png"
                    page = browser_context.new_page()
                    try:
                        page.goto(url, timeout=20000, wait_until="domcontentloaded")
                        page.wait_for_timeout(1800)
                        page.screenshot(path=str(shot_path))
                        if shot_path.stat().st_size < 64 * 1024:
                            ComfyUIBridge.upscale_image(shot_path)
                        image_data = base64.b64encode(shot_path.read_bytes()).decode("ascii")
                        decision = parse_json(VRAMController.query_llm(
                            OBSERVER_MODEL,
                            "Inspect this webpage screenshot. If a CAPTCHA, login wall or anti-bot gate is present, "
                            'return {"action": "blocked"}. If a cookie banner or modal covers the content, return '
                            '{"action": "dismiss", "x": int, "y": int} with the dismiss button centre. '
                            'Otherwise return {"action": "extract"}.',
                            json_mode=True,
                            images_base64=[image_data],
                        ))
                        if decision.get("action") == "blocked":
                            log.info("[Vision] Gate detected on %s; skipped.", url)
                            continue
                        if decision.get("action") == "dismiss":
                            page.mouse.click(int(decision.get("x", 640)), int(decision.get("y", 450)))
                            page.wait_for_timeout(1500)
                            page.screenshot(path=str(shot_path))
                            image_data = base64.b64encode(shot_path.read_bytes()).decode("ascii")
                        extracted = VRAMController.query_llm(
                            OBSERVER_MODEL,
                            "Transcribe all factual organisational, professional, biographical, handle, domain "
                            "and date details visible on this page. Do not speculate.",
                            images_base64=[image_data],
                        )
                        if extracted.strip():
                            results.append((url, extracted, "vision_vlm", str(shot_path)))
                    except Exception as error:  # Playwright / LLM failures must not abort the batch
                        log.warning("[Vision] %s: %s", url, error)
                    finally:
                        page.close()
                browser_context.close()
            finally:
                browser.close()
        return results

    def harvest(
        self,
        store: CaseStore,
        query: str,
        max_results: int = 8,
        use_vision: bool = True,
        anchor: Optional[Dict[str, Any]] = None,
    ) -> Dict[str, Any]:
        hits = self.search(query, max_results)
        terms = anchor_terms(anchor, query)
        report = {"query": query, "candidates": len(hits), "new_artifacts": 0, "duplicates": 0,
                  "rejected_off_topic": 0, "robots_denied": 0, "failed": 0, "vision_pages": 0}
        filtered_urls: List[str] = []
        for hit in hits:
            if hit["snippet"] and not matches_terms(hit["snippet"], terms, hit["url"]):
                report["rejected_off_topic"] += 1
            else:
                filtered_urls.append(hit["url"])
        vision_queue: List[str] = []

        def commit(url: str, text: str, method: str, artifact: str) -> None:
            if not matches_terms(text, terms, url):
                report["rejected_off_topic"] += 1
                return
            if store.add_evidence(url, text, method, query, artifact):
                report["new_artifacts"] += 1
            else:
                report["duplicates"] += 1

        if filtered_urls:
            with ThreadPoolExecutor(max_workers=4) as pool:
                for url, text, method, artifact in pool.map(
                    lambda item: self.fetch_document(item, store.evidence_dir), filtered_urls
                ):
                    if method == "needs_vision":
                        vision_queue.append(url)
                    elif method in ("robots_denied", "failed"):
                        report[method] += 1
                    elif text:
                        commit(url, text, method, artifact)
        if use_vision and vision_queue:
            for url, text, method, artifact in self.vision_batch(vision_queue[:4], store.evidence_dir):
                report["vision_pages"] += 1
                commit(url, text, method, artifact)
        store.record_query(query, report["new_artifacts"])
        store.audit("HARVEST", report)
        log.info("[Harvest] %s", report)
        return report


# ==============================================================================
# 8. INTELLIGENCE CORE (anchor -> ACH sync hub -> bounded pivots -> ICD-203 dossier)
# ==============================================================================
class IntelligenceCore:
    def __init__(self) -> None:
        self.bridge = AxiomNativeBridge(AXIOM_DLL)
        self.collector = SourceCollector(PolitenessGate(), self.bridge)

    # --- Phase 1: Immutable Context Anchor (Input node + Think_1 zero-shot hypotheses) ---
    def build_anchor(self, seed: str, purpose: str) -> Dict[str, Any]:
        seed, purpose = (seed or "").strip(), (purpose or "").strip()
        if not seed:
            raise ValueError("seed must not be empty.")
        if len(purpose) < 10:
            raise ValueError("purpose must state the lawful research objective (min 10 characters).")
        case_id = uuid.uuid4().hex[:12]
        prompt = f"""You are an all-source intelligence targeting officer.
Convert the seed into a strict JSON Context Anchor and formulate competing hypotheses.
Schema:
{{"primary_name": "string", "entity_kind": "person|organization|domain|event|other",
  "known_aliases": ["string"], "known_handles_or_domains": ["string"],
  "anchor_demographics": {{"birth_year": null, "origin_city": "string"}},
  "core_profession": "string",
  "initial_hypotheses": [
    {{"id": "H1", "label": "Primary Target", "description": "subject matching the exact seed qualifiers"}},
    {{"id": "H2", "label": "Namesake / Collision", "description": "distinct entity sharing name or alias"}}]}}
Only use information present in the seed. Seed: {seed}"""
        try:
            anchor = parse_json(VRAMController.query_llm(ANALYST_MODEL, prompt, json_mode=True))
        except RuntimeError as error:
            log.warning("[Anchor] LLM unavailable, deterministic anchor used: %s", error)
            anchor = {}
        if not isinstance(anchor.get("anchor_demographics"), dict):
            anchor["anchor_demographics"] = {}
        anchor["anchor_demographics"].setdefault("birth_year", None)
        anchor["anchor_demographics"].setdefault("origin_city", "")
        anchor["primary_name"] = str(anchor.get("primary_name") or seed.split(",")[0].strip() or "Unknown Target")
        anchor["entity_kind"] = str(anchor.get("entity_kind") or "other")
        anchor["known_aliases"] = clean_strings(anchor.get("known_aliases"), 10)
        anchor["known_handles_or_domains"] = clean_strings(anchor.get("known_handles_or_domains"), 10)
        anchor["core_profession"] = str(anchor.get("core_profession") or "")
        if not isinstance(anchor.get("initial_hypotheses"), list) or not anchor["initial_hypotheses"]:
            anchor["initial_hypotheses"] = [
                {"id": "H1", "label": "Primary Target", "description": f"Subject matching: {seed}"},
                {"id": "H2", "label": "Namesake / Collision", "description": "Distinct entity sharing identifiers"},
            ]
        anchor.update({"case_id": case_id, "seed": seed, "purpose": purpose,
                       "created_utc": utc_now(), "immutable": True})
        store = CaseStore(case_id)
        store.anchor_path.write_text(json.dumps(anchor, indent=2, ensure_ascii=False), encoding="utf-8")
        store.audit("ANCHOR_LOCKED", {"seed": seed, "purpose": purpose})
        log.info("[Phase 1] Anchor locked: %s (%s)", anchor["primary_name"], case_id)
        return anchor

    @staticmethod
    def wide_net_queries(anchor: Dict[str, Any]) -> List[str]:
        name = anchor["primary_name"]
        queries = [f'"{name}"']
        for alias in anchor.get("known_aliases", [])[:2]:
            queries.append(f'"{alias}"')
        for handle in anchor.get("known_handles_or_domains", [])[:2]:
            queries.append(f'"{handle}"')
        return list(dict.fromkeys(queries))

    @staticmethod
    def evidence_digest(rows: List[Dict[str, Any]], budget: int) -> str:
        if not rows:
            return "(no evidence collected yet)"
        per_source = max(1200, budget // len(rows))
        blocks, used = [], 0
        for row in rows:
            block = (f"[E:{row['content_hash'][:10]}] {row['url']} | method={row['method']} | "
                     f"utc={row['collected_utc']}\n{row['content'][:per_source]}")
            if used + len(block) > budget:
                break
            blocks.append(block)
            used += len(block)
        return "\n\n---\n\n".join(blocks)

    # --- Phase 3: Think_2 sync hub (ACH) --------------------------------------
    def ach_pass(self, anchor: Dict[str, Any], store: CaseStore) -> Dict[str, Any]:
        template = f"""Perform Analysis of Competing Hypotheses (ACH) with diagnosticity scoring.
{ANALYTIC_RULES}

Immutable Context Anchor (re-anchored lifeline; never drift from it):
{json.dumps(anchor, ensure_ascii=False)}

Already executed queries (do not repeat): {json.dumps(sorted(store.executed_queries()))}

Tasks:
1. CRITICAL: Do NOT treat the Context Anchor as evidence. Extract claims ONLY if explicitly corroborated in the Evidence text below. If 0 evidence documents mention the target, return `claims: []`, `confidence <= 0.10`, and state what is missing in `intelligence_gaps`.
2. confidence (0.0-1.0) that the evidence describes the primary target rather than a namesake.
3. Up to 3 clean, high-diagnosticity pivot queries (avoid `site:A OR site:B` boolean chains; prefer simple identifier + platform queries) that would confirm or refute a hypothesis.
4. Rate every claim: source_reliability A (official/primary) .. F (cannot judge);
   info_credibility 1 (independently confirmed) .. 6 (cannot judge);
   hypothesis_alignment PRIMARY | NAMESAKE_COLLISION | AMBIGUOUS.
5. Extract entities (organisations, collaborators, domains, publications) actually observed in Evidence.

Return strict JSON:
{{"confidence": 0.0, "pivot_queries": ["..."],
  "hypotheses": [{{"id": "H1", "label": "...", "description": "...", "status": "LEADING|ACTIVE|DISCOUNTED", "diagnostic_score": 0.0}}],
  "claims": [{{"category": "temporal|relational|digital_footprint|professional", "statement": "...",
              "evidence_tag": "E:xxxxxxxxxx", "source_url": "...", "source_reliability": "B",
              "info_credibility": "2", "hypothesis_alignment": "PRIMARY"}}],
  "entities": [{{"name": "...", "type": "organization|person|domain|publication|handle", "relation": "..."}}],
  "key_assumptions": ["..."], "intelligence_gaps": ["..."]}}

Evidence:
"""
        digest = self.evidence_digest(store.evidence_rows(), char_budget(len(template), reserve_tokens=6000))
        return parse_json(VRAMController.query_llm(ANALYST_MODEL, template + digest, json_mode=True))

    def run_analysis(
        self,
        anchor: Dict[str, Any],
        store: CaseStore,
        max_passes: int = 1,
        notify: Optional[Callable[[str], None]] = None,
    ) -> Dict[str, Any]:
        """Think_2 -> Research_req loop, hard-capped at MAX_RESEARCH_PASSES."""
        passes = max(1, min(int(max_passes), MAX_RESEARCH_PASSES))
        analysis: Dict[str, Any] = {}
        for pass_number in range(1, passes + 1):
            analysis = self.ach_pass(anchor, store) or analysis
            store.replace_analysis(analysis, pass_number, anchor=anchor)
            state = store.analysis_state()
            store.audit("ANALYSIS_PASS", {"pass": pass_number, "confidence": state["confidence"], "claims": len(state["claims"])})
            if pass_number == passes:
                break
            executed = store.executed_queries()
            pivots = [query for query in clean_strings(analysis.get("pivot_queries"), 3) if query.lower() not in executed]
            if not pivots:
                break
            for query in pivots:
                if notify:
                    notify(f"Research_req pass {pass_number}: pivot '{query}'")
                self.collector.harvest(store, query, max_results=5, anchor=anchor)
        return store.analysis_state()

    # --- Phase 4: ICD-203 synthesis + deterministic appendices ----------------
    @staticmethod
    def custody_appendix(store: CaseStore, state: Dict[str, Any]) -> str:
        lines = ["\n\n---\n\n## Appendix A — Evidence Register (Chain of Custody)\n",
                 "| Tag | SHA-256 | Method | Collected (UTC) | Source |", "|---|---|---|---|---|"]
        for row in store.evidence_rows():
            lines.append(f"| E:{row['content_hash'][:10]} | `{row['content_hash']}` | {row['method']} | "
                         f"{row['collected_utc']} | {row['url']} |")
        lines += ["\n## Appendix B — ACH Matrix\n", "| ID | Hypothesis | Status | Diagnostic score |", "|---|---|---|---|"]
        for hypothesis in state["hypotheses"]:
            lines.append(f"| {hypothesis['hypothesis_id']} | {hypothesis['label']} — {hypothesis['description']} | "
                         f"{hypothesis['status']} | {hypothesis['diagnostic_score']:.2f} |")
        lines += ["\n## Appendix C — Rated Claims (NATO Admiralty)\n",
                  "| Rating | Alignment | Claim | Evidence |", "|---|---|---|---|"]
        for claim in state["claims"]:
            statement = claim["statement"].replace("|", "/")
            lines.append(f"| {claim['source_reliability']}{claim['info_credibility']} | {claim['hypothesis_alignment']} | "
                         f"{statement} | {claim['evidence_tag'] or claim['source_url']} |")
        return "\n".join(lines) + "\n"

    def synthesize(self, anchor: Dict[str, Any], store: CaseStore) -> Path:
        state = store.analysis_state()
        template = f"""You are the Phase 4 all-source synthesis engine. Write an executive-grade Markdown
intelligence dossier adhering to ICD-203 analytic standards.
{ANALYTIC_RULES}
- Flag single-source or low-credibility (ratings D-F or 4-6) items as [UNVERIFIED / LOW CONFIDENCE].
- Use estimative language (almost certainly, likely, roughly even chance, unlikely).

Mandatory sections:
1. BLUF & Analytic Confidence (High / Moderate / Low) with key judgments
2. ACH & Disambiguation (how the target was separated from namesakes)
3. Chronological Timeline with [E:tag] citations and [Reliability/Credibility] codes, e.g. [B2]
4. Entity & Network Relationship Matrix
5. Verified Public Digital Footprint (organisational domains, public handles, publications)
6. Key Assumptions & Intelligence Gaps

Immutable Context Anchor:
{json.dumps(anchor, indent=2, ensure_ascii=False)}

ACH state (hypotheses, rated claims, entities, gaps):
{json.dumps(state, indent=2, ensure_ascii=False)}

Evidence corpus:
"""
        digest = self.evidence_digest(store.evidence_rows(), char_budget(len(template), reserve_tokens=8000))
        dossier = VRAMController.query_llm(OBSERVER_MODEL, template + digest, temperature=0.25)
        VRAMController.evict_models()
        header = f"<!-- Specter case {store.case_id} | generated {utc_now()} | purpose: {anchor.get('purpose', '')} -->\n\n"
        store.dossier_path.write_text(redact_sensitive(header + dossier + self.custody_appendix(store, state)),
                                      encoding="utf-8")
        store.audit("DOSSIER_WRITTEN", str(store.dossier_path))
        log.info("[Phase 4] Dossier written: %s", store.dossier_path)
        return store.dossier_path


# ==============================================================================
# 9. GEO SENSOR FUSION (Spector sensory layer: FIRMS thermal + USGS seismic + news)
# ==============================================================================
class GeoSensor:
    EARTH_RADIUS_KM = 6371.0

    @classmethod
    def distance_km(cls, latitude_a: float, longitude_a: float, latitude_b: float, longitude_b: float) -> float:
        phi_a, phi_b = math.radians(latitude_a), math.radians(latitude_b)
        delta_phi = math.radians(latitude_b - latitude_a)
        delta_lambda = math.radians(longitude_b - longitude_a)
        haversine = math.sin(delta_phi / 2) ** 2 + math.cos(phi_a) * math.cos(phi_b) * math.sin(delta_lambda / 2) ** 2
        return 2 * cls.EARTH_RADIUS_KM * math.asin(math.sqrt(haversine))

    @staticmethod
    def bounding_box(latitude: float, longitude: float, radius_km: float) -> tuple:
        latitude_delta = radius_km / 111.32
        longitude_delta = radius_km / (111.32 * max(0.01, math.cos(math.radians(latitude))))
        return (max(-180.0, longitude - longitude_delta), max(-90.0, latitude - latitude_delta),
                min(180.0, longitude + longitude_delta), min(90.0, latitude + latitude_delta))

    @classmethod
    def thermal_hotspots(cls, latitude: float, longitude: float, radius_km: float, days: int) -> Dict[str, Any]:
        if not FIRMS_MAP_KEY:
            return {"status": "disabled", "reason": "Set SPECTER_FIRMS_KEY (free NASA Earthdata FIRMS MAP_KEY)."}
        west, south, east, north = cls.bounding_box(latitude, longitude, radius_km)
        url = (f"https://firms.modaps.eosdis.nasa.gov/api/area/csv/{FIRMS_MAP_KEY}/{FIRMS_SOURCE}/"
               f"{west:.4f},{south:.4f},{east:.4f},{north:.4f}/{max(1, min(days, 5))}")
        response = requests.get(url, timeout=30, headers={"User-Agent": USER_AGENT})
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
                hotspots.append({"latitude": point_latitude, "longitude": point_longitude,
                                 "distance_km": round(distance, 2), "frp_mw": radiative_power,
                                 "acquired": f"{row.get('acq_date', '')} {row.get('acq_time', '')}",
                                 "confidence": row.get("confidence"), "daynight": row.get("daynight")})
        hotspots.sort(key=lambda spot: spot["frp_mw"], reverse=True)
        return {"status": "ok", "source": FIRMS_SOURCE, "count": len(hotspots), "hotspots": hotspots[:50]}

    @staticmethod
    def seismic_events(latitude: float, longitude: float, radius_km: float, days: int) -> Dict[str, Any]:
        start = (datetime.now(timezone.utc) - timedelta(days=days)).strftime("%Y-%m-%dT%H:%M:%S")
        response = requests.get(
            "https://earthquake.usgs.gov/fdsnws/event/1/query",
            params={"format": "geojson", "latitude": latitude, "longitude": longitude,
                    "maxradiuskm": radius_km, "starttime": start, "orderby": "magnitude", "limit": 50},
            timeout=30, headers={"User-Agent": USER_AGENT},
        )
        response.raise_for_status()
        events = []
        for feature in response.json().get("features", []):
            properties = feature.get("properties", {})
            coordinates = feature.get("geometry", {}).get("coordinates", [])
            events.append({"magnitude": properties.get("mag"), "place": properties.get("place"),
                           "time_utc": datetime.fromtimestamp((properties.get("time") or 0) / 1000, timezone.utc).isoformat(),
                           "depth_km": coordinates[2] if len(coordinates) > 2 else None, "url": properties.get("url")})
        return {"status": "ok", "count": len(events), "events": events}

    @staticmethod
    def regional_news(place_name: str, max_results: int = 10) -> Dict[str, Any]:
        if not place_name.strip() or DDGS is None:
            return {"status": "skipped"}
        query = f"{place_name} fire OR explosion OR earthquake OR outage OR strike"
        try:
            with DDGS() as search_client:
                items = search_client.news(query, max_results=max_results) or []
        except Exception as error:  # search backends raise many custom exception types
            return {"status": "error", "reason": str(error)}
        return {"status": "ok", "query": query, "items": [
            {"title": item.get("title"), "date": item.get("date"), "source": item.get("source"), "url": item.get("url")}
            for item in items]}


# ==============================================================================
# 10. FASTMCP SERVER SURFACE
# ==============================================================================
mcp = FastMCP("Specter_Intel")
core = IntelligenceCore()


def to_json(payload: Any) -> str:
    return json.dumps(payload, indent=2, ensure_ascii=False, default=str)


@mcp.tool()
async def system_health() -> str:
    """Reports readiness of every subsystem: Ollama models, ComfyUI, native DLL, search, Playwright, FIRMS."""
    def probe() -> Dict[str, Any]:
        report: Dict[str, Any] = {"specter_home": str(SPECTER_HOME), "native_dll": bool(core.bridge.dll),
                                  "trafilatura": trafilatura is not None, "search_backend": DDGS is not None,
                                  "firms_key": bool(FIRMS_MAP_KEY)}
        try:
            installed = {model.get("name") for model in requests.get(OLLAMA_TAGS_URL, timeout=5).json().get("models", [])}
            report["ollama"] = {"reachable": True, "analyst_ready": ANALYST_MODEL in installed,
                                "observer_ready": OBSERVER_MODEL in installed}
        except (requests.RequestException, ValueError):
            report["ollama"] = {"reachable": False, "url": OLLAMA_BASE}
        try:
            report["comfyui"] = requests.get(f"{COMFYUI_URL}/system_stats", timeout=5).ok
        except requests.RequestException:
            report["comfyui"] = False
        try:
            import playwright  # noqa: F401
            report["playwright"] = True
        except ImportError:
            report["playwright"] = False
        return report
    return to_json(await asyncio.to_thread(probe))


@mcp.tool()
async def anchor_target(seed: str, purpose: str) -> str:
    """
    Phase 1: Lock an Immutable Context Anchor and open a case (SQLite chain-of-custody store).
    seed: identifying qualifiers, e.g. "Jane Doe, materials scientist, ETH Zurich".
    purpose: lawful research objective (recorded in the audit log), e.g. "vendor due diligence".
    """
    anchor = await asyncio.to_thread(core.build_anchor, seed, purpose)
    return to_json({"status": "ANCHOR_LOCKED", "case_id": anchor["case_id"], "primary_name": anchor["primary_name"],
                    "hypotheses": anchor["initial_hypotheses"], "wide_net_queries": core.wide_net_queries(anchor)})


@mcp.tool()
async def harvest_sources(case_id: str, query: str, max_results: int = 8, use_vision: bool = True) -> str:
    """Phase 2: Search public sources for `query` and ingest target-relevant, SHA-256 deduplicated evidence."""
    if not query.strip():
        raise ValueError("query must not be empty.")
    anchor, store = open_case(case_id)
    report = await asyncio.to_thread(
        core.collector.harvest, store, query.strip(), max(1, min(max_results, 25)), use_vision, anchor
    )
    return to_json({**report, "case_id": store.case_id, "total_artifacts": store.statistics()["evidence_total"]})


@mcp.tool()
async def ingest_url(case_id: str, url: str) -> str:
    """Phase 2 (targeted): Fetch a single public URL (robots.txt honoured) into the case evidence store."""
    _, store = open_case(case_id)
    canonical = canonicalize_url(url)

    def ingest() -> Dict[str, Any]:
        _, text, method, artifact = core.collector.fetch_document(canonical, store.evidence_dir)
        if method == "needs_vision":
            fallback = core.collector.vision_batch([canonical], store.evidence_dir)
            _, text, method, artifact = fallback[0] if fallback else (canonical, "", "failed", "")
        added = bool(text) and store.add_evidence(canonical, text, method, "manual_ingest", artifact)
        store.audit("INGEST_URL", {"url": canonical, "method": method, "added": added})
        return {"url": canonical, "method": method, "added": added, "chars": len(text or "")}

    return to_json(await asyncio.to_thread(ingest))


@mcp.tool()
async def run_analysis(case_id: str, max_passes: int = 1) -> str:
    """
    Phase 3: Grounded ACH + NATO Admiralty claim rating (defaults to 1 fast pass to stay under
    60s client tool timeouts; returns recommended_pivots for optional follow-up harvests).
    """
    anchor, store = open_case(case_id)
    state = await asyncio.to_thread(core.run_analysis, anchor, store, max_passes)
    summary = {key: state[key] for key in ("confidence", "passes_completed", "hypotheses",
                                           "intelligence_gaps", "key_assumptions", "recommended_pivots")}
    return to_json({**summary, "claims_rated": len(state["claims"]), "entities": len(state["entities"])})


@mcp.tool()
async def synthesize_dossier(case_id: str) -> str:
    """Phase 4: Write the ICD-203 dossier with chain-of-custody, ACH and rated-claims appendices."""
    anchor, store = open_case(case_id)
    path = await asyncio.to_thread(core.synthesize, anchor, store)
    return to_json({"status": "DOSSIER_READY", "case_id": store.case_id, "dossier_path": str(path),
                    "resource": f"specter://cases/{store.case_id}/dossier"})


@mcp.tool()
async def run_full_cycle(seed: str, purpose: str, context: Context, max_results: int = 8) -> str:
    """End-to-end: anchor -> wide-net harvest -> ACH with bounded pivots -> ICD-203 dossier (with live progress)."""
    loop = asyncio.get_running_loop()

    async def phase(step: int, message: str) -> None:
        await context.info(message)
        await context.report_progress(step, 4)

    def notify(message: str) -> None:
        asyncio.run_coroutine_threadsafe(context.info(message), loop)

    await phase(0, "Phase 1: locking Immutable Context Anchor...")
    anchor = await asyncio.to_thread(core.build_anchor, seed, purpose)
    store = CaseStore(anchor["case_id"])
    await phase(1, f"Phase 2: wide-net harvest for {anchor['primary_name']}...")
    for query in core.wide_net_queries(anchor):
        await asyncio.to_thread(core.collector.harvest, store, query, max(1, min(max_results, 25)), True, anchor)
    await phase(2, f"Phase 3: ACH over {store.statistics()['evidence_total']} artifacts (max {MAX_RESEARCH_PASSES} passes)...")
    state = await asyncio.to_thread(core.run_analysis, anchor, store, MAX_RESEARCH_PASSES, notify)
    await phase(3, "Phase 4: synthesizing ICD-203 dossier...")
    path = await asyncio.to_thread(core.synthesize, anchor, store)
    await phase(4, "Complete.")
    return to_json({"status": "COMPLETE", "case_id": store.case_id, "primary_name": anchor["primary_name"],
                    "analytic_confidence": state["confidence"], "statistics": store.statistics(),
                    "dossier_path": str(path)})


@mcp.tool()
async def case_status(case_id: str) -> str:
    """Summarise a case: anchor, evidence statistics, ACH confidence, gaps, dossier and recent audit trail."""
    anchor, store = open_case(case_id)
    state = store.analysis_state()
    return to_json({"case_id": store.case_id, "primary_name": anchor["primary_name"], "purpose": anchor.get("purpose"),
                    "created_utc": anchor.get("created_utc"), "statistics": store.statistics(),
                    "confidence": state["confidence"], "passes_completed": state["passes_completed"],
                    "intelligence_gaps": state["intelligence_gaps"], "dossier_ready": store.dossier_path.exists(),
                    "recent_audit": store.audit_log(10)})


@mcp.tool()
async def list_cases() -> str:
    """List every case on disk with target name, creation time and evidence count."""
    def collect() -> List[Dict[str, Any]]:
        cases = []
        for directory in sorted(CASES_DIR.iterdir()):
            anchor_file = directory / "anchor.json"
            if directory.is_dir() and CASE_ID_PATTERN.match(directory.name) and anchor_file.exists():
                anchor = json.loads(anchor_file.read_text(encoding="utf-8"))
                cases.append({"case_id": directory.name, "primary_name": anchor.get("primary_name"),
                              "created_utc": anchor.get("created_utc"),
                              "evidence_total": CaseStore(directory.name).statistics()["evidence_total"],
                              "dossier_ready": (directory / "dossier.md").exists()})
        return cases
    return to_json(await asyncio.to_thread(collect))


@mcp.tool()
async def search_evidence(case_id: str, keyword: str, limit: int = 10) -> str:
    """Keyword search across collected case evidence; returns tagged excerpts with source URLs."""
    if not keyword.strip():
        raise ValueError("keyword must not be empty.")
    _, store = open_case(case_id)
    return to_json(await asyncio.to_thread(store.search_evidence, keyword.strip(), max(1, min(limit, 50))))


@mcp.tool()
async def scan_geo_events(latitude: float, longitude: float, radius_km: float = 25.0,
                          days: int = 2, place_name: str = "") -> str:
    """
    Spector sensory fusion: NASA FIRMS thermal anomalies + USGS seismic events within radius_km
    of a coordinate over the last `days`, optionally correlated with regional news for place_name.
    """
    if not (-90 <= latitude <= 90 and -180 <= longitude <= 180):
        raise ValueError("Coordinates out of range.")
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


if __name__ == "__main__":
    mcp.run()
