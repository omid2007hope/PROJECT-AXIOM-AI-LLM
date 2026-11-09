"""
OpenOSINT Cloud — Authlib OAuth client registry (GitHub + Google login).

Web-dashboard login only. X-API-Key auth (cloud/auth.py) and MCP bearer
auth (cloud/routes/mcp_gateway.py) never touch this module — an OAuth
identity is a separate login layer on top of the existing api_key model,
optionally linked to a customer via cloud/db.get_or_create_user.

No Facebook: Meta rejects OSINT apps in app review.
"""
from __future__ import annotations

from authlib.integrations.starlette_client import OAuth

from cloud.config import (
    GITHUB_CLIENT_ID,
    GITHUB_CLIENT_SECRET,
    GOOGLE_CLIENT_ID,
    GOOGLE_CLIENT_SECRET,
)

oauth = OAuth()

oauth.register(
    name="github",
    client_id=GITHUB_CLIENT_ID,
    client_secret=GITHUB_CLIENT_SECRET,
    access_token_url="https://github.com/login/oauth/access_token",
    authorize_url="https://github.com/login/oauth/authorize",
    api_base_url="https://api.github.com/",
    client_kwargs={"scope": "read:user"},
)

oauth.register(
    name="google",
    client_id=GOOGLE_CLIENT_ID,
    client_secret=GOOGLE_CLIENT_SECRET,
    server_metadata_url="https://accounts.google.com/.well-known/openid-configuration",
    client_kwargs={"scope": "openid email profile"},
)
