"""GET /v1/usage — return the caller's remaining credits and plan."""
from __future__ import annotations

from fastapi import APIRouter, Depends
from pydantic import BaseModel

from cloud import db
from cloud.auth import get_customer

router = APIRouter()


class UsageResponse(BaseModel):
    plan: str
    credits: int


@router.get("/usage", response_model=UsageResponse)
async def usage(customer: db.Customer = Depends(get_customer)) -> UsageResponse:
    return UsageResponse(plan=customer.plan, credits=customer.credits)
