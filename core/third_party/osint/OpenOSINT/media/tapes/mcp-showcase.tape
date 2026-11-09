# mcp-showcase.tape
# Charmbracelet VHS — https://github.com/charmbracelet/vhs
# Demo: OpenOSINT registered as a Claude Code MCP server — the key differentiator
# Run: vhs media/tapes/mcp-showcase.tape  (from repo root)

Output media/output/mcp-showcase.gif
Output media/output/mcp-showcase.mp4

Set Shell "bash"
Set FontSize 14
Set FontFamily "Courier New"
Set Width 1200
Set Height 600
Set Theme "Dracula"
Set Padding 20
Set TypingSpeed 80ms
Set PlaybackSpeed 0.9
WindowBar Colorful

# ── Setup: blank slate, silent errors ──────────────────────────────────────
Hide
Type "export PS1=''"
Enter
Type "exec 2>/dev/null"
Enter
Type "clear"
Enter
Show

# ── Scene 1: confirm openosint is registered as an MCP server ─────────────
Type "claude mcp list"
Enter
Sleep 1800ms

Type "  Name            Scope   Server"
Enter
Type "  ─────────────────────────────────────────────────────────────────"
Enter
Type "  openosint       user    uvx openosint-server"
Enter
Sleep 2000ms

# ── Scene 2: single-prompt autonomous investigation ───────────────────────
Enter
Type "claude 'investigate dev.researcher47@proton.me and compile a report'"
Enter
Sleep 1500ms

Type "  I'll investigate dev.researcher47@proton.me using the OpenOSINT"
Enter
Type "  tools available via MCP. Running the full investigation now."
Enter
Sleep 1200ms

# ── Scene 3: Claude calls openosint tools one by one ─────────────────────
Type "  openosint:search_email(email='dev.researcher47@proton.me')"
Enter
Sleep 1800ms
Type "    → 5 accounts found: GitHub, Twitter, Dev.to, Keybase, GitLab"
Enter
Sleep 800ms

Type "  openosint:search_breach(email='dev.researcher47@proton.me')"
Enter
Sleep 1800ms
Type "    → Collection 1 (2019) — email + plaintext password exposed"
Enter
Sleep 800ms

Type "  openosint:search_username(username='devresearcher47')"
Enter
Sleep 2000ms
Type "    → 8 platforms: GitHub · HackerNews · Reddit · Dev.to + 4 others"
Enter
Sleep 800ms

Type "  openosint:generate_dorks(target='dev.researcher47@proton.me')"
Enter
Sleep 1400ms
Type "    → 8 Google / Bing dork URLs generated"
Enter
Sleep 1400ms

# ── Scene 4: Claude compiles the report autonomously ─────────────────────
Enter
Type "  Compiling full OSINT report from all tool results..."
Enter
Sleep 1800ms

Type "  ## OSINT Report — dev.researcher47@proton.me"
Enter
Enter
Type "  Accounts (5): GitHub · Twitter · Dev.to · Keybase · GitLab"
Enter
Type "  Username:     devresearcher47 — active on 8 platforms"
Enter
Type "  Breaches:     Collection 1 (2019) — plaintext credentials exposed"
Enter
Type "  Dorks:        8 search URLs ready for extended research"
Enter
Enter
Type "  Action required: password rotation on all linked accounts."
Enter
Sleep 3500ms
