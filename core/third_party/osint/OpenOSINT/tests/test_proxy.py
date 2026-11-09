# tests/test_proxy.py
"""
Tests for openosint.proxy: precedence, redaction, and per-mechanism helpers.
"""

from __future__ import annotations

import pytest

from openosint import proxy


@pytest.fixture(autouse=True)
def _reset_proxy_state(monkeypatch):
    """Ensure each test starts with no CLI flag and no env var set."""
    proxy.set_cli_proxy_url(None)
    monkeypatch.delenv(proxy._ENV_VAR, raising=False)
    yield
    proxy.set_cli_proxy_url(None)


class TestPrecedence:
    def test_returns_none_when_unset(self):
        assert proxy.get_proxy_url() is None

    def test_env_var_used_when_no_cli_flag(self, monkeypatch):
        monkeypatch.setenv(proxy._ENV_VAR, "http://envhost:8080")
        assert proxy.get_proxy_url() == "http://envhost:8080"

    def test_cli_flag_overrides_env_var(self, monkeypatch):
        monkeypatch.setenv(proxy._ENV_VAR, "http://envhost:8080")
        proxy.set_cli_proxy_url("http://clihost:9090")
        assert proxy.get_proxy_url() == "http://clihost:9090"


class TestSchemeValidation:
    def test_rejects_unsupported_scheme(self):
        proxy.set_cli_proxy_url("ftp://host:21")
        with pytest.raises(proxy.ProxyConfigError, match="Unsupported proxy scheme"):
            proxy.get_proxy_url()

    def test_accepts_http(self):
        proxy.set_cli_proxy_url("http://host:8080")
        assert proxy.get_proxy_url() == "http://host:8080"

    def test_accepts_https(self):
        proxy.set_cli_proxy_url("https://host:8443")
        assert proxy.get_proxy_url() == "https://host:8443"


class TestRequestsProxies:
    def test_none_when_unset(self):
        assert proxy.get_requests_proxies() is None

    def test_dict_when_set(self):
        proxy.set_cli_proxy_url("http://host:8080")
        assert proxy.get_requests_proxies() == {
            "http": "http://host:8080",
            "https": "http://host:8080",
        }


class TestAiohttpProxy:
    def test_none_when_unset(self):
        assert proxy.get_aiohttp_proxy() is None

    def test_returns_url_for_http(self):
        proxy.set_cli_proxy_url("http://host:8080")
        assert proxy.get_aiohttp_proxy() == "http://host:8080"

    def test_none_for_socks_scheme(self, monkeypatch):
        import sys
        import types

        monkeypatch.setitem(sys.modules, "socks", types.ModuleType("socks"))
        proxy.set_cli_proxy_url("socks5://host:1080")
        assert proxy.get_aiohttp_proxy() is None


class TestSherlockArgs:
    def test_empty_when_unset(self):
        assert proxy.get_sherlock_proxy_args() == []

    def test_flag_when_set(self):
        proxy.set_cli_proxy_url("http://host:8080")
        assert proxy.get_sherlock_proxy_args() == ["--proxy", "http://host:8080"]


class TestSubprocessEnv:
    def test_none_when_unset(self):
        assert proxy.get_subprocess_env() is None

    def test_sets_proxy_vars_when_configured(self):
        proxy.set_cli_proxy_url("http://host:8080")
        env = proxy.get_subprocess_env()
        assert env["HTTP_PROXY"] == "http://host:8080"
        assert env["HTTPS_PROXY"] == "http://host:8080"
        assert env["http_proxy"] == "http://host:8080"
        assert env["https_proxy"] == "http://host:8080"

    def test_preserves_existing_environment(self, monkeypatch):
        monkeypatch.setenv("SOME_UNRELATED_VAR", "keep-me")
        proxy.set_cli_proxy_url("http://host:8080")
        env = proxy.get_subprocess_env()
        assert env["SOME_UNRELATED_VAR"] == "keep-me"


class TestRedaction:
    def test_none_url_returns_none_literal(self):
        assert proxy.redact_proxy_url(None) == "none"

    def test_masks_username_and_password(self):
        redacted = proxy.redact_proxy_url("http://myuser:mypassword@proxy.example.com:8080")
        assert "myuser" not in redacted
        assert "mypassword" not in redacted
        assert redacted == "http://***:***@proxy.example.com:8080"

    def test_no_credentials_returns_url_unchanged(self):
        assert proxy.redact_proxy_url("http://proxy.example.com:8080") == (
            "http://proxy.example.com:8080"
        )


class TestSocksExtraMissing:
    def test_clear_error_when_pysocks_missing(self, monkeypatch):
        import builtins

        real_import = builtins.__import__

        def _fake_import(name, *args, **kwargs):
            if name == "socks":
                raise ImportError("no module named socks")
            return real_import(name, *args, **kwargs)

        monkeypatch.setattr(builtins, "__import__", _fake_import)
        proxy.set_cli_proxy_url("socks5://host:1080")
        with pytest.raises(proxy.ProxyConfigError, match="pip install openosint\\[socks\\]"):
            proxy.get_proxy_url()
