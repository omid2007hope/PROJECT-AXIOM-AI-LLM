# web-demo.tape
# Charmbracelet VHS — https://github.com/charmbracelet/vhs
# Demo: OpenOSINT web server + API in action
# Run: vhs media/tapes/web-demo.tape  (from repo root)

Output media/output/web-demo.gif
Output media/output/web-demo.mp4

Set Shell "bash"
Set FontSize 14
Set FontFamily "Courier New"
Set Width 1200
Set Height 600
Set Theme "Dracula"
Set Padding 20
Set TypingSpeed 80ms
Set PlaybackSpeed 0.9

# ── Setup ──────────────────────────────────────────────────────────────────
Hide
Type "export PS1='$ '"
Enter
Type "exec 2>/dev/null"
Enter
Type "clear"
Enter
Show

# ── Scene 1: start the web server ─────────────────────────────────────────
Type "openosint web --no-browser --port 8080"
Enter
Sleep 1000ms

Type "[*] OpenOSINT 2.11.0 web server"
Enter
Type "[*] App  → http://localhost:8080/"
Enter
Type "[*] Docs → http://localhost:8080/docs/"
Enter
Type "[*] Press Ctrl+C to stop."
Enter
Sleep 2000ms

# ── Scene 2: health check ─────────────────────────────────────────────────
Type "$ curl -s http://localhost:8080/api/health | python3 -m json.tool"
Enter
Sleep 800ms

Type '{'
Enter
Type '    "status": "ok",'
Enter
Type '    "version": "2.11.0",'
Enter
Type '    "setup_complete": true,'
Enter
Type '    "ai_backend": "claude"'
Enter
Type '}'
Enter
Sleep 1500ms

# ── Scene 3: tool list ────────────────────────────────────────────────────
Type "$ curl -s http://localhost:8080/api/tools | python3 -m json.tool | head -20"
Enter
Sleep 800ms

Type '['
Enter
Type '    { "name": "search_email",    "category": "Identity", "available": true },'
Enter
Type '    { "name": "search_username", "category": "Identity", "available": true },'
Enter
Type '    { "name": "search_breach",   "category": "Identity", "available": true },'
Enter
Type '    { "name": "search_ip",       "category": "Network",  "available": true },'
Enter
Type '    { "name": "search_whois",    "category": "Network",  "available": true },'
Enter
Type '    { "name": "generate_dorks",  "category": "Recon",    "available": true },'
Enter
Type '    ...'
Enter
Type ']'
Enter
Sleep 1500ms

# ── Scene 4: backend status ────────────────────────────────────────────────
Type "$ curl -s http://localhost:8080/api/chat/test | python3 -m json.tool"
Enter
Sleep 800ms

Type '{'
Enter
Type '    "status": "ok",'
Enter
Type '    "backend": "claude",'
Enter
Type '    "ollama_reachable": null'
Enter
Type '}'
Enter
Sleep 1500ms

# ── Scene 5: AI chat request via SSE ──────────────────────────────────────
Type "$ curl -s -X POST http://localhost:8080/api/chat \\"
Enter
Type '  -H "Content-Type: application/json" \\'
Enter
Type '  -d '"'"'{"message":"what tools do you have?","history":[],"model":"claude"}'"'"
Enter
Sleep 800ms

Type "data: {\"type\":\"text\",\"content\":\"I have 13 OSINT tools available:\"}"
Enter
Sleep 400ms
Type "data: {\"type\":\"text\",\"content\":\" search_email, search_username, search_breach,\"}"
Enter
Sleep 400ms
Type "data: {\"type\":\"text\",\"content\":\" search_ip, search_whois, search_domain,\"}"
Enter
Sleep 400ms
Type "data: {\"type\":\"text\",\"content\":\" generate_dorks, search_paste, search_phone,\"}"
Enter
Sleep 400ms
Type "data: {\"type\":\"text\",\"content\":\" search_shodan, search_virustotal, search_censys,\"}"
Enter
Sleep 400ms
Type "data: {\"type\":\"text\",\"content\":\" and search_ip2location.\"}"
Enter
Sleep 400ms
Type "data: {\"type\":\"done\"}"
Enter
Sleep 3000ms
