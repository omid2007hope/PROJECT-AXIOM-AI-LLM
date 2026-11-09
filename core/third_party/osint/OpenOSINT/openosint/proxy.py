# openosint/proxy.py
"""
Global upstream-proxy configuration.

Precedence: --proxy CLI flag > OPENOSINT_PROXY_URL env var > unset (no proxy).

Excluded by design (see CLAUDE.md investigation notes):
  - search_dns: raw DNS resolution, not proxyable via HTTP/SOCKS.
  - generate_dorks: no network call.
  - the Anthropic client (agent.py): LLM API traffic, not target-facing OSINT
    traffic — proxying it adds latency/cost and exposes prompt content to a
    third party for no benefit.
  - scrape_url, search_dorks_live, search_footprint: all three call Bright
    Data's own API. Bright Data is already the unlocker/residential-network
    layer; a second proxy in front of that API call doesn't touch the actual
    target-fetch (Bright Data does that server-side) and is a redundant hop.
"""

from __future__ import annotations

import os
from urllib.parse import urlsplit, urlunsplit

_ENV_VAR = "OPENOSINT_PROXY_URL"
_SOCKS_INSTALL_HINT = "SOCKS proxy requires: pip install openosint[socks]"
_SUPPORTED_SCHEMES = ("http", "https", "socks5", "socks5h")

_cli_proxy_url: str | None = None


class ProxyConfigError(Exception):
    """Raised when the configured proxy URL has an unsupported scheme or a missing extra."""


def set_cli_proxy_url(url: str | None) -> None:
    """Register the --proxy CLI flag value. Call once at startup."""
    global _cli_proxy_url
    _cli_proxy_url = url or None


def _is_socks(url: str) -> bool:
    return urlsplit(url).scheme.lower().startswith("socks5")


def _validate(url: str) -> None:
    scheme = urlsplit(url).scheme.lower()
    if scheme not in _SUPPORTED_SCHEMES:
        raise ProxyConfigError(
            f"Unsupported proxy scheme '{scheme}://'. Use http://, https://, or socks5://."
        )
    if _is_socks(url):
        try:
            import socks  # noqa: F401  (pysocks — required by requests/urllib3 for SOCKS)
        except ImportError as exc:
            raise ProxyConfigError(_SOCKS_INSTALL_HINT) from exc


def get_proxy_url() -> str | None:
    """Return the effective proxy URL: CLI flag > env var > None."""
    url = _cli_proxy_url or os.environ.get(_ENV_VAR) or None
    if url:
        _validate(url)
    return url


def get_requests_proxies() -> dict[str, str] | None:
    """Return a requests-style {'http': url, 'https': url} dict, or None.

    Used for requests calls and for the shodan/censys SDKs, which both
    forward this dict to their internal requests.Session. SOCKS5 URLs work
    transparently here as long as pysocks is installed.
    """
    url = get_proxy_url()
    return {"http": url, "https": url} if url else None


def get_aiohttp_proxy() -> str | None:
    """Return the proxy URL for aiohttp's per-request proxy= kwarg (HTTP/HTTPS only).

    aiohttp cannot use a SOCKS proxy via the plain proxy= kwarg — that case
    is handled by get_aiohttp_connector() at session-creation time instead.
    """
    url = get_proxy_url()
    if not url or _is_socks(url):
        return None
    return url


def get_aiohttp_connector():
    """Return an aiohttp_socks ProxyConnector when a SOCKS5 proxy is configured, else None."""
    url = get_proxy_url()
    if not url or not _is_socks(url):
        return None
    try:
        from aiohttp_socks import ProxyConnector
    except ImportError as exc:
        raise ProxyConfigError(_SOCKS_INSTALL_HINT) from exc
    return ProxyConnector.from_url(url)


def get_subprocess_env() -> dict[str, str] | None:
    """Return an env dict with proxy vars set for subprocess inheritance, or None.

    None means "use the default" — run_subprocess() then omits env= entirely,
    which preserves today's behavior of inheriting the parent environment.
    """
    url = get_proxy_url()
    if not url:
        return None
    env = dict(os.environ)
    env["HTTP_PROXY"] = url
    env["HTTPS_PROXY"] = url
    env["http_proxy"] = url
    env["https_proxy"] = url
    return env


def get_sherlock_proxy_args() -> list[str]:
    """Return ['--proxy', url] for sherlock's native flag, or [] when no proxy is set."""
    url = get_proxy_url()
    return ["--proxy", url] if url else []


def redact_proxy_url(url: str | None) -> str:
    """Mask user:pass in a proxy URL for safe logging: http://***:***@host:port."""
    if not url:
        return "none"
    parts = urlsplit(url)
    if not parts.username and not parts.password:
        return url
    netloc = f"***:***@{parts.hostname or ''}"
    if parts.port:
        netloc += f":{parts.port}"
    return urlunsplit((parts.scheme, netloc, parts.path, parts.query, parts.fragment))


def log_proxy_status(logger) -> None:
    """Log one redacted line confirming proxy mode, if a proxy is configured."""
    url = get_proxy_url()
    if url:
        logger.info("Upstream proxy active: %s", redact_proxy_url(url))
