---
name: axiom-server
description: Manage, run, configure, and troubleshoot Project Axiom's local OpenAI-compatible HTTP server implemented in core/src/server.cpp.
---

# Project Axiom HTTP Server Guide

Project Axiom exposes a lightweight Win32 socket HTTP server on `http://127.0.0.1:8080` bridging local stacked GQA Transformer inference to external HTTP clients.

## Architecture & Endpoints

| Endpoint | Protocol | Purpose | Handler Function |
| :--- | :--- | :--- | :--- |
| `POST /v1/chat/completions` | OpenAI API (SSE + JSON) | Local chat completion inference | `build_qwen_chatml_prompt`, `execute_server_loop` |
| `GET /v1/models` | OpenAI API (JSON) | Model discovery | `execute_server_loop` |

---

## Operations & Lifecycle

### Starting the Server
1. **Background Daemon (Recommended):**
   ```powershell
   cd "core"
   .\app.exe --serve
   ```
2. **From Interactive Console Menu:**
   - Launch `.\app.exe`
   - Select option `[6] Launch HTTP Server`
   - Select option `[1] Start Background Server` or `[2] Start Foreground Server`

### Stopping / Freeing Port 8080
If port 8080 reports `WSAEADDRINUSE`:
```powershell
Stop-Process -Name app -Force -ErrorAction SilentlyContinue
```

### Compiling Changes
```powershell
cd "core"
Stop-Process -Name app -Force -ErrorAction SilentlyContinue
mingw32-make -j8 app.exe
```

### Testing the Server
```powershell
curl http://127.0.0.1:8080/v1/chat/completions `
  -H "Content-Type: application/json" `
  -d '{"messages":[{"role":"user","content":"Explain GPU Tensor Cores in C++."}],"stream":false}'
```
