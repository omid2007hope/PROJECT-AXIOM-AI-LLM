---
name: axiom-server
description: Manage, run, configure, and troubleshoot Project Axiom's dual HTTP server (OpenAI /v1 and Ollama /api endpoints) implemented in core/src/server.cpp, including IDE agent tool-calling and ComfyUI integration.
---

# Project Axiom Dual Server Guide

Project Axiom exposes a lightweight Win32 socket HTTP server on `http://127.0.0.1:8080` bridging local stacked GQA Transformer inference to external developer environments.

## Architecture & Endpoints

| Endpoint | Protocol | Target Client | Handler Function |
| :--- | :--- | :--- | :--- |
| `POST /v1/chat/completions` | OpenAI API (SSE + JSON) | Cline, Antigravity IDE, Cursor | `build_qwen_chatml_prompt`, `execute_server_loop` |
| `GET /v1/models` | OpenAI API (JSON) | Cline / VS Code Model Discovery | `execute_server_loop` |
| `GET /api/tags` | Ollama API (JSON) | ComfyUI Ollama Nodes | `execute_server_loop` |
| `POST /api/generate` | Ollama API (JSON) | ComfyUI Prompt Expansion | `build_ollama_prompt`, `execute_server_loop` |

---

## Tool-Calling Protocols

### 1. Antigravity IDE & Cline Native Tool Calling
Antigravity IDE's agent runtime exposes the tool registry:
`[read_files, search_codebase, fetch_web_content, editor, ask_question, run_commands]`

When modifying workspace code, the server calls `editor`:
```json
{
  "path": "core/src/main.cpp",
  "old_text": "<exact single-occurrence snippet from disk>",
  "new_text": "<replacement C++ code>"
}
```
*Rules for `editor`:*
- `path` must stay within the workspace CWD (e.g. `core/src/main.cpp`).
- `old_text` must match exactly **once** in the target file.
- On Turn 2, the IDE replies with `role: "tool"` and content `Edited <path>...`. The server immediately concludes with `finish_reason: "stop"` to close the turn.

### 2. Legacy XML Fallback
When the client does not send an OpenAI `"tools"` schema in the request payload, the server emits standard XML blocks in `delta.content`:
```xml
<replace_in_file>
<path>core/src/main.cpp</path>
<diff>
------- SEARCH
<exact lines to replace>
=======
<replacement lines>
+++++++ REPLACE
</diff>
</replace_in_file>
```

### 3. Fast Context Compaction
When context fills toward 32.0k tokens, Cline transmits a compaction request requesting `### Goal`, `### State`, `### Highlights`, `### Next`, `### Files`.
- The server intercepts compaction prompts before tokenizer prefill.
- Returns a structured markdown summary in `<100ms` without running GPU inference.

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
   - Select option `[6] Launch OpenAI Server`
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

### Verifying Tool Calling
Run the automated validation script:
```powershell
python "C:\Users\Omid\.gemini\antigravity\brain\b8529fc5-a013-452c-89a9-1dc86c324104\scratch\test_antigravity_editor.py"
```

