"""FastAPI dependency that reads the signed-cookie session and returns the User.

Separate from cloud/auth.py (X-API-Key → Customer) on purpose: this is the
web-dashboard login identity, not the tool-call credential. Session holds
only `user_id` — never an api_key.
"""
from __future__ import annotations

from fastapi import HTTPException, Request

from cloud import db


async def get_current_user(request: Request) -> db.User:
    user_id = request.session.get("user_id")
    if user_id is None:
        raise HTTPException(status_code=401, detail="Not logged in")
    user = await db.get_user(user_id)
    if user is None:
        raise HTTPException(status_code=401, detail="Session user no longer exists")
    return user
