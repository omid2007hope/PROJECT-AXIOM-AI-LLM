"""
OpenOSINT Cloud — per-tool key-source configuration.

KeySource controls where /v1/enrich resolves the upstream API key:
  platform         — platform's own env var (sponsored / included perk)
  tenant           — tenant's stored BYOK key (required — 422 if absent)
  tenant_optional  — tenant's stored key when present, else call unauth
  none             — tool needs no credential

To move a tool between platform-provided and tenant-provided, change its
entry here.  No other files need to change.

Canonical provider strings (used by POST /v1/keys and 422 messages):
  "ipinfo"      — ipinfo.io token (search_ip)
  "abuseipdb"   — AbuseIPDB API key (search_abuseipdb)
  "virustotal"  — VirusTotal API key (search_virustotal)
  "censys"      — Censys credentials, stored as "api_id:api_secret" (search_censys)
"""
from __future__ import annotations

import os
from enum import Enum
from typing import NamedTuple

from cloud import keys
from cloud.config import SHODAN_CREDIT_COST


class KeySource(str, Enum):
    platform = "platform"
    tenant = "tenant"
    tenant_optional = "tenant_optional"
    none = "none"


class ToolKeyConfig(NamedTuple):
    env_var: str | None      # platform env-var name; None when source is tenant/none
    source: KeySource
    provider: str | None     # canonical tenant-facing provider string; None for platform/none tools
    credit_cost: int = 1     # credits charged per successful call (see cloud/config.py for tuning)


# Single source of truth for v1 tool credentials.
TOOL_KEY_CONFIG: dict[str, ToolKeyConfig] = {
    # Sponsored — key is platform-provided; tenants get this included.
    "search_ip2location": ToolKeyConfig("IP2LOCATION_API_KEY", KeySource.platform, provider=None),
    # Platform pool — Shodan ToS requires attribution on every response, added
    # in cloud/tools.py dispatch() via the ATTRIBUTION map.
    # Cost is a single tunable constant in cloud/config.SHODAN_CREDIT_COST.
    "search_shodan":      ToolKeyConfig("SHODAN_API_KEY",      KeySource.platform, provider=None, credit_cost=SHODAN_CREDIT_COST),
    # BYOK required — tenant must POST /v1/keys with the provider string below.
    # Never platform/tenant_optional: upstream ToS forbids a shared platform key.
    "search_ip":          ToolKeyConfig("IPINFO_TOKEN",        KeySource.tenant,   provider="ipinfo"),
    "search_abuseipdb":   ToolKeyConfig("ABUSEIPDB_API_KEY",   KeySource.tenant,   provider="abuseipdb"),
    "search_virustotal":  ToolKeyConfig("VIRUSTOTAL_API_KEY",  KeySource.tenant,   provider="virustotal"),
    "search_censys":      ToolKeyConfig(None,                  KeySource.tenant,   provider="censys"),
    # No credential required.
    "search_dns":         ToolKeyConfig(None,                  KeySource.none,     provider=None),
    "search_domain":      ToolKeyConfig(None,                  KeySource.none,     provider=None),
}


def get_credit_cost(tool: str) -> int:
    """Return the credit cost for `tool`. Unknown tools default to 1."""
    cfg = TOOL_KEY_CONFIG.get(tool)
    return cfg.credit_cost if cfg is not None else 1


def is_platform_pool_tool(tool: str) -> bool:
    """True if `tool` is backed by a shared platform-pool key (burst-limited)."""
    cfg = TOOL_KEY_CONFIG.get(tool)
    return cfg is not None and cfg.source == KeySource.platform


# Censys stores two credentials (API ID + secret) under one BYOK provider
# slot, concatenated in this exact format. Split on the FIRST colon only, so
# a secret that itself contains ':' stays intact as the second part.
CENSYS_SECRET_FORMAT = "<api_id>:<api_secret>"

# Per-provider placeholder shown in POST /v1/keys examples and 422 messages.
# Providers not listed use a plain opaque-key placeholder.
_DEFAULT_SECRET_HINT = "your_key"
PROVIDER_SECRET_HINT: dict[str, str] = {
    "censys": CENSYS_SECRET_FORMAT,
}


class InvalidSecretFormatError(ValueError):
    """Raised when a secret submitted to POST /v1/keys doesn't match its
    provider's required format (currently only Censys has a compound format)."""

    def __init__(self, provider: str, detail: str) -> None:
        self.provider = provider
        super().__init__(detail)


def validate_secret_format(provider: str, secret: str) -> None:
    """Validate provider-specific secret formats before storage.

    No-op for providers with a plain opaque-key format. Raises
    InvalidSecretFormatError on a malformed compound secret (e.g. Censys).
    """
    if provider == "censys":
        api_id, sep, api_secret = secret.partition(":")
        if not sep or not api_id or not api_secret:
            raise InvalidSecretFormatError(
                provider,
                f"Invalid 'censys' secret format. Expected \"{CENSYS_SECRET_FORMAT}\" "
                "with both parts non-empty, separated by a colon.",
            )


class MissingCredentialError(ValueError):
    """Raised when a tenant-required tool has no stored BYOK key.

    Subclasses ValueError so the MCP gateway's existing `except ValueError`
    handling keeps working unchanged.
    """

    def __init__(self, tool: str, provider: str) -> None:
        self.tool = tool
        self.provider = provider
        hint = PROVIDER_SECRET_HINT.get(provider, _DEFAULT_SECRET_HINT)
        super().__init__(
            f"Tool '{tool}' requires a connected '{provider}' key. "
            f"Connect it with: POST /v1/keys "
            f'{{"provider": "{provider}", "secret": "{hint}"}}'
        )


async def resolve_key(tool: str, customer_api_key: str) -> str | None:
    """Return the upstream API key for `tool`, per TOOL_KEY_CONFIG.

    Raises MissingCredentialError when the tool's source is `tenant` and no
    key is stored for this customer. Shared by /v1/enrich and the MCP gateway
    so both surfaces resolve credentials identically.
    """
    cfg = TOOL_KEY_CONFIG.get(tool)
    if cfg is None or cfg.source == KeySource.none:
        return None

    if cfg.source == KeySource.platform:
        return os.environ.get(cfg.env_var or "", "") or None

    # tenant or tenant_optional — look up from the customer's encrypted store
    stored = await keys.get_key(customer_api_key, cfg.provider)

    if cfg.source == KeySource.tenant and stored is None:
        raise MissingCredentialError(tool, cfg.provider)

    return stored  # None is valid for tenant_optional when key is absent
