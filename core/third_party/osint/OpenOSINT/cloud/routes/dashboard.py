"""GET /dashboard — session-gated web dashboard landing page.

Plain HTMLResponse strings, no template engine — not worth adding Jinja2 for
a handful of small pages. The HTML returned here is fully static; no
user-controlled data (email, provider) is ever interpolated into it
server-side. All personalization happens client-side via fetch() against
the existing JSON API (/v1/me) and is rendered with textContent / href
assignment rather than innerHTML, so provider-supplied fields like email —
which are attacker-influenceable — never become HTML.

Access is invite-only (contact commercial@openosint.tech) — there is no
self-serve checkout. A customer_api_key is provisioned by hand and linked
here via the "link an existing key" form below.
"""
from __future__ import annotations

from fastapi import APIRouter, Depends, HTTPException
from fastapi.responses import HTMLResponse
from pydantic import BaseModel

from cloud import db
from cloud.session_auth import get_current_user

router = APIRouter()

LINK_KEY_SECTION_HTML = """
<div id="link-key-section">
  <h2>Already have a key?</h2>
  <form id="link-key-form">
    <input type="text" id="link-key-input" placeholder="Paste your API key" required>
    <button type="submit">Link key</button>
  </form>
  <p id="link-key-status"></p>
</div>
<script>
document.getElementById("link-key-form").addEventListener("submit", async (e) => {
  e.preventDefault();
  const key = document.getElementById("link-key-input").value;
  const statusEl = document.getElementById("link-key-status");
  const resp = await fetch("/v1/link-key", {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify({customer_api_key: key}),
  });
  if (resp.ok) {
    statusEl.textContent = "Linked! Refresh to see your updated status.";
  } else {
    const body = await resp.json().catch(() => ({}));
    statusEl.textContent = "Error: " + (body.detail || resp.status);
  }
});
</script>
"""

_DASHBOARD_HTML = """<!doctype html>
<html>
<head><meta charset="utf-8"><title>OpenOSINT Cloud — Dashboard</title></head>
<body>
<h1>Dashboard</h1>
<p id="account">Loading account…</p>
<p>Need access or more credits? Email <a href="mailto:commercial@openosint.tech">commercial@openosint.tech</a>.</p>
""" + LINK_KEY_SECTION_HTML + """
<script>
async function loadAccount() {
  const resp = await fetch("/v1/me");
  if (!resp.ok) {
    document.getElementById("account").textContent = "Not logged in.";
    return null;
  }
  return resp.json();
}

(async () => {
  const me = await loadAccount();
  if (!me) return;
  const account = document.getElementById("account");
  account.textContent = "Logged in as " + (me.email || me.provider) +
    " — " + (me.linked ? "key linked" : "no key linked yet");
})();
</script>
</body>
</html>"""


@router.get("/dashboard", response_class=HTMLResponse)
async def dashboard(user=Depends(get_current_user)) -> HTMLResponse:
    return HTMLResponse(_DASHBOARD_HTML)


class LinkKeyRequest(BaseModel):
    customer_api_key: str


@router.post("/v1/link-key")
async def link_key(body: LinkKeyRequest, user: db.User = Depends(get_current_user)) -> dict:
    status = await db.link_existing_customer_key(user.id, body.customer_api_key)
    if status == "not_found":
        raise HTTPException(status_code=404, detail="No account found for that key")
    if status == "conflict":
        raise HTTPException(status_code=409, detail="That key is already linked to another account")
    return {"status": "ok"}
