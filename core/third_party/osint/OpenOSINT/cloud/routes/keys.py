"""POST /v1/keys, GET /v1/keys, DELETE /v1/keys/{provider}."""
from __future__ import annotations

from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel

from cloud import db, keys
from cloud.auth import get_customer
from cloud.key_sources import (
    TOOL_KEY_CONFIG,
    InvalidSecretFormatError,
    KeySource,
    validate_secret_format,
)

router = APIRouter()

# Accepted provider strings derived from TOOL_KEY_CONFIG — one source of truth.
# Platform-sourced keys are operator-managed, not tenant-manageable.
_CUSTOMER_PROVIDERS: frozenset[str] = frozenset(
    cfg.provider
    for cfg in TOOL_KEY_CONFIG.values()
    if cfg.provider is not None
    and cfg.source in (KeySource.tenant, KeySource.tenant_optional)
)


class StoreKeyRequest(BaseModel):
    provider: str
    secret: str


class KeyEntry(BaseModel):
    provider: str
    masked: str


@router.post("/keys", status_code=204)
async def store_key_route(
    body: StoreKeyRequest,
    customer: db.Customer = Depends(get_customer),
) -> None:
    if body.provider not in _CUSTOMER_PROVIDERS:
        raise HTTPException(
            status_code=400,
            detail=(
                f"Unknown provider '{body.provider}'. "
                f"Accepted values: {sorted(_CUSTOMER_PROVIDERS)}"
            ),
        )
    if not body.secret.strip():
        raise HTTPException(status_code=400, detail="secret must not be empty")
    try:
        validate_secret_format(body.provider, body.secret.strip())
    except InvalidSecretFormatError as exc:
        raise HTTPException(status_code=422, detail=str(exc))
    # TODO v1.1: lightweight validation call for ipinfo (GET /json with the token)
    await keys.store_key(customer.api_key, body.provider, body.secret.strip())


@router.get("/keys", response_model=list[KeyEntry])
async def list_keys_route(
    customer: db.Customer = Depends(get_customer),
) -> list[KeyEntry]:
    stored = await keys.list_keys(customer.api_key)
    return [KeyEntry(**entry) for entry in stored]


@router.delete("/keys/{provider}", status_code=204)
async def delete_key_route(
    provider: str,
    customer: db.Customer = Depends(get_customer),
) -> None:
    deleted = await keys.delete_key(customer.api_key, provider)
    if not deleted:
        raise HTTPException(
            status_code=404,
            detail=f"No key stored for provider '{provider}'",
        )
