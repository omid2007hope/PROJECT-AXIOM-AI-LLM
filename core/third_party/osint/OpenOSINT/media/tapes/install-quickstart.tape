# install-quickstart.tape
# Charmbracelet VHS — https://github.com/charmbracelet/vhs
# Demo: 30-second install and first-run — top-of-funnel hook
# Run: vhs media/tapes/install-quickstart.tape  (from repo root)

Output media/output/install-quickstart.gif
Output media/output/install-quickstart.mp4

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
Type "export PS1='$ '"
Enter
Type "exec 2>/dev/null"
Enter
Type "clear"
Enter
Show

# ── Scene 1: install openosint ─────────────────────────────────────────────
Type "pip install openosint"
Enter
Sleep 600ms

Type "Collecting openosint"
Enter
Type "  Downloading openosint-2.11.0-py3-none-any.whl (51 kB)"
Enter
Type "     ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 51.2/51.2 kB 1.4 MB/s"
Enter
Type "Collecting anthropic>=0.30 (from openosint)"
Enter
Type "Collecting rich>=13.0 (from openosint)"
Enter
Type "Collecting prompt-toolkit>=3.0 (from openosint)"
Enter
Type "Installing collected packages: openosint"
Enter
Type "Successfully installed openosint-2.11.0"
Enter
Sleep 1800ms

# ── Scene 2: set the API key ───────────────────────────────────────────────
Type "export ANTHROPIC_API_KEY=sk-ant-api03-xxxxxxxxxxxxxxxxxxxxxxxx-xxxxxxxx"
Enter
Sleep 1200ms

# ── Scene 3: launch the REPL ───────────────────────────────────────────────
Hide
Type "export PS1=''"
Enter
Show

Type "openosint"
Enter
Sleep 1200ms

Type "  ╭────────────────────────────────────────────────────────────────────╮"
Enter
Type "  │  OpenOSINT v2.11.0  ·  Provider: Anthropic (claude-sonnet-4-6)   │"
Enter
Type "  ╰────────────────────────────────────────────────────────────────────╯"
Enter
Type "  Type a target or question. 'help' for commands. 'exit' to quit."
Enter
Enter
Sleep 800ms

Type "  ⭐ If OpenOSINT is useful, star it → github.com/OpenOSINT/OpenOSINT"
Enter
Enter
Sleep 1000ms

Type "openosint ❯ "
Sleep 2500ms
