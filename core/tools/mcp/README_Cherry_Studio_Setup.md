# 3-Tier MCP Web Research Stack — Cherry Studio + Ollama Guide

Complete setup and daily-use reference for running **Specter** (HTTP/API), **Browser Sandbox** (isolated Playwright), and **Browser Live** (side-by-side CDP Chrome/Edge) in Cherry Studio with local Ollama models.

---

## 1. Architecture & Files

All files live in `core\tools\mcp\`:

| Tier | Server / File | Active Tools | Role |
|---|---|---|---|
| **Tier 1** | `Specter_MCP.py` (`specter`) | **5** (`research`) / **17** (`full`) | Fast stateless web search (`web_search`), HTTP/PDF extraction (`fetch_page`), one-shot OSINT dossiers (`run_full_cycle`), geo-events (`scan_geo_events`). |
| **Tier 2** | `Browser_Sandbox_MCP.py` (`browser-sandbox`) | **7** | Clean isolated Playwright Chromium/Edge session for JS-rendered SPAs, clicking, typing, `<select>` dropdowns, and popup tabs. No personal cookies. |
| **Tier 3** | `Browser_Live_MCP.py` (`browser-live`) | **9** | Attaches over loopback CDP (`127.0.0.1:9222`) to a docked visible Chrome/Edge window (`AXIOM_LiveBrowser`). **Never touches your Windows mouse or keyboard focus.** |

Supporting files:
- `cherry_studio_mcp.json` — Ready-to-import Cherry Studio `mcpServers` config (pinned to `We_Know\.venv\Scripts\python.exe`).
- `Cherry_Studio_System_Prompt.md` — Routing prompt to paste into your Cherry Studio assistant.
- `launch_live_browser.bat` — Optional manual Windows launcher for the docked side-by-side live browser.

---

## 2. One-Time Cherry Studio Configuration Checklist

### A. Import MCP Servers
1. Open **Settings → MCP Servers → Add → Import from JSON**.
2. Paste the contents of `cherry_studio_mcp.json`.
3. Set toggles:
   - **`specter`:** **ON** (uses `SPECTER_TOOL_PROFILE=research` -> 5 tools).
   - **`browser-sandbox`:** **ON** (7 tools).
   - **`browser-live`:** **ON** when using the side-by-side browser (or leave **OFF** by default so 8B–12B models stay at 12 active tools instead of 21).
   - **`axiom_spy`:** **OFF** (duplicate full-profile `Specter_MCP.py`; only turn ON if you turn `specter` OFF and want all 17 granular OSINT tools).

### B. Disable Conflicting Built-In Features
- **Settings → Skills:**
  - `cherry-computer` → **OFF** (prevents OS mouse/keyboard takeover).
  - `cherry-browser` → **OFF** (prevents competing with `browser-sandbox` / `browser-live`).
- **Settings → Computer Use:**
  - `Allow agents to control desktop applications` → **OFF** (`browser-live` uses CDP and does not need OS Computer Use).
- **Settings → Browser:**
  - `Allow AI browser control` → **OFF**
  - `Automatically open the browser panel` → **OFF**
- **Settings → Web Search:**
  - `Search service provider` → **`DuckDuckGo`** (free, keyless).
  - `URL fetch provider` → **`fetch`** (built-in local HTTP fetch; stops Jina API errors).
  - `Prefer model-native web tools` → **OFF**
- **Chat Input Bar:**
  - Keep the built-in **Web Search** globe icon **OFF** so Ollama calls your MCP tools directly.

### C. Bind MCPs & System Prompt to Your Assistant
1. Open your **Assistant / Agent settings → System Prompt** and paste `Cherry_Studio_System_Prompt.md`.
2. Open the assistant's **MCP** tab and enable `specter`, `browser-sandbox`, and (when needed) `browser-live`.

---

## 3. Ollama Context & Model Tuning

Ollama's default context (`4096`) silently truncates your system prompt and tool schemas after a single `get_elements` + `read_text` call. Raise it to **at least `16384`**:

### Option A — Global Windows Environment Variable (PowerShell)
```powershell
setx OLLAMA_CONTEXT_LENGTH 16384
```
Restart Ollama after running `setx`.

### Option B — Custom Modelfile
```text
FROM qwen3:8b
PARAMETER num_ctx 16384
```

### Recommended Tool-Calling Tips
- Keep active enabled tools around **≤ 12** when using 8B–12B models (`specter` [5] + `browser-sandbox` [7] = 12).
- Strong 2026 local tool-calling options: `qwen3:8b`, `qwen3.5:9b`, `glm-4:9b`, `gemma4:12b`, or larger (`llama3.3`, `mistral-small3.2`, larger `qwen3`) if VRAM allows.

---

## 4. Daily Usage

### Tier 1 & Tier 2 (Automatic)
Just ask normally in Cherry Studio:
- *"Search the web for CUDA Blackwell shared memory limits."* → Calls `specter` (`web_search` + `fetch_page`).
- *"Compile an OSINT dossier on `<target>`."* → Calls `specter` (`run_full_cycle`).
- *"Open `<SPA URL>`, click the pricing tab, and summarize it."* → Escalates to `browser-sandbox` (`open_url` → `get_elements` → `click_element` → `read_text` → `close_session`).

### Tier 3 — Side-by-Side Live Browser (`browser-live`)
- **Auto-launch (default):** Because `LIVE_AUTO_LAUNCH=true` is set in `cherry_studio_mcp.json`, calling any `browser-live` tool automatically opens a docked Chrome/Edge window on the right half of your screen using profile `%LOCALAPPDATA%\AXIOM_LiveBrowser` on `127.0.0.1:9222`.
- **Manual launch:** Double-click `launch_live_browser.bat` anytime to open the docked browser first (e.g., to sign into a site manually before asking the AI to research in it).
- **Verify CDP is alive:** Open `http://127.0.0.1:9222/json/version` in any browser; it should return JSON containing `webSocketDebuggerUrl`.
- **Adjust window docking:** Edit `LIVE_WINDOW_POSITION` / `LIVE_WINDOW_SIZE` in `cherry_studio_mcp.json` (and `--window-position` / `--window-size` in `launch_live_browser.bat`) if your screen is not `1920x1080` at 100% scaling.
- **Keep working side-by-side:** Because `browser-live` sends events over CDP instead of Windows `SendInput`, you can keep using your mouse and keyboard on the other half of the screen.
- **Disconnecting:** Calling `disconnect_session` detaches the MCP client **without closing your docked Chrome/Edge window**.

---

## 5. Switching Specter Modes (`research` vs `full`)

In `cherry_studio_mcp.json` under `mcpServers.specter.env`:
- `"SPECTER_TOOL_PROFILE": "research"` *(default for Cherry Studio)* → Exposes **5** tools (`web_search`, `fetch_page`, `run_full_cycle`, `case_status`, `scan_geo_events`).
- `"SPECTER_TOOL_PROFILE": "full"` → Exposes all **17** OSINT graph/evidence tools (`anchor_target`, `harvest_sources`, `traverse_links`, `run_analysis`, `synthesize_dossier`, etc.).

---

## 6. Troubleshooting

| Symptom | Cause & Fix |
|---|---|
| `"Web lookup: Jina could not complete a request"` | Switch **Settings → Web Search → URL fetch provider** from `Jina` to `fetch` and disable built-in Web Search in the chat bar. |
| AI grabs physical mouse / steals window focus | Turn **OFF** `cherry-computer` skill and **Settings → Computer Use → Allow agents to control desktop applications**. |
| Ollama picks wrong tool or ignores instructions | Turn **OFF** `axiom_spy` and `cherry-browser`; verify `OLLAMA_CONTEXT_LENGTH=16384`; toggle `browser-live` off unless needed. |
| `browser-live` cannot connect to `127.0.0.1:9222` | Run `launch_live_browser.bat`, check `http://127.0.0.1:9222/json/version`, and ensure no other app owns port `9222`. |
| Site shows login or CAPTCHA / verification gate | Both browser MCPs stop cleanly and will never bypass anti-bot gates. Complete any needed login/verification manually in the docked `browser-live` window, then continue. |
