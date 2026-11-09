"""
GET /auth/login/{provider}, GET /auth/callback/{provider}, GET /auth/logout,
GET /v1/me — browser-facing OAuth login (GitHub / Google).

Purely a web-dashboard identity layer on top of the existing api_key model.
X-API-Key auth (cloud/auth.py) and MCP bearer auth (cloud/routes/mcp_gateway.py)
are untouched by this file. No checkout/linking logic here — see the
customer-key rendezvous added in a later commit.
"""
from __future__ import annotations

from fastapi import APIRouter, Depends, HTTPException, Request
from fastapi.responses import RedirectResponse
from pydantic import BaseModel

from cloud import db
from cloud.oauth import oauth
from cloud.session_auth import get_current_user

router = APIRouter()

_PROVIDERS = ("github", "google")


class MeResponse(BaseModel):
    id: int
    provider: str
    email: str | None
    linked: bool


def _require_known_provider(provider: str) -> None:
    if provider not in _PROVIDERS:
        raise HTTPException(status_code=404, detail=f"Unknown provider '{provider}'")


@router.get("/auth/login/{provider}")
async def login(provider: str, request: Request):
    _require_known_provider(provider)
    client = oauth.create_client(provider)
    redirect_uri = request.url_for("auth_callback", provider=provider)
    return await client.authorize_redirect(request, redirect_uri)


@router.get("/auth/callback/{provider}", name="auth_callback")
async def auth_callback(provider: str, request: Request) -> RedirectResponse:
    _require_known_provider(provider)
    client = oauth.create_client(provider)
    token = await client.authorize_access_token(request)

    if provider == "google":
        info = token.get("userinfo") or {}
        provider_user_id = info.get("sub", "")
        email = info.get("email")
    else:  # github — not OIDC, fetch the profile explicitly
        resp = await client.get("user", token=token)
        info = resp.json()
        provider_user_id = str(info.get("id") or "")
        email = info.get("email")

    if not provider_user_id:
        raise HTTPException(status_code=502, detail=f"'{provider}' did not return a user id")

    user = await db.get_or_create_user(provider, provider_user_id, email)
    request.session["user_id"] = user.id
    return RedirectResponse(url="/dashboard")


@router.get("/auth/logout")
async def logout(request: Request) -> RedirectResponse:
    request.session.clear()
    return RedirectResponse(url="/")


@router.get("/v1/me", response_model=MeResponse)
async def me(user: db.User = Depends(get_current_user)) -> MeResponse:
    return MeResponse(
        id=user.id,
        provider=user.provider,
        email=user.email,
        linked=user.customer_api_key is not None,
    )
