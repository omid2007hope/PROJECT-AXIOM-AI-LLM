# username-trace.tape
# Charmbracelet VHS — https://github.com/charmbracelet/vhs
# Demo: username trace across platforms in the OpenOSINT REPL
# Run: vhs media/tapes/username-trace.tape  (from repo root)

Output media/output/username-trace.gif
Output media/output/username-trace.mp4

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

# ── Scene 1: launch the REPL ───────────────────────────────────────────────
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
Sleep 1500ms

# ── Scene 2: user asks to trace a username ────────────────────────────────
Type "openosint ❯ find all accounts for h4x0r_dev"
Enter
Sleep 700ms

Type "  Thinking..."
Enter
Sleep 1500ms

# ── Scene 3: search_username runs across 300+ platforms ───────────────────
Type "  → search_username(username='h4x0r_dev')"
Enter
Sleep 400ms
Type "  Scanning 300+ platforms via Sherlock..."
Enter
Sleep 2500ms

# ── Scene 4: results populate one by one ──────────────────────────────────
Type "    GitHub        https://github.com/h4x0r_dev"
Enter
Sleep 350ms
Type "    Twitter/X     https://x.com/h4x0r_dev"
Enter
Sleep 350ms
Type "    Reddit        https://reddit.com/user/h4x0r_dev"
Enter
Sleep 350ms
Type "    HackerNews    https://news.ycombinator.com/user?id=h4x0r_dev"
Enter
Sleep 350ms
Type "    Dev.to        https://dev.to/h4x0r_dev"
Enter
Sleep 350ms
Type "    GitLab        https://gitlab.com/h4x0r_dev"
Enter
Sleep 350ms
Type "    Keybase       https://keybase.io/h4x0r_dev"
Enter
Sleep 350ms
Type "    Mastodon      https://infosec.exchange/@h4x0r_dev"
Enter
Sleep 1600ms

# ── Scene 5: summary ──────────────────────────────────────────────────────
Enter
Type "  Username h4x0r_dev traced to 8 platforms."
Enter
Enter
Type "  Consistent handle across GitHub, Dev.to, and Mastodon suggests a"
Enter
Type "  developer with a security / infosec background. GitHub profile is"
Enter
Type "  public — repositories and contribution graph are accessible."
Enter
Enter
Type "  ✓ Report saved → reports/2026-05-19_21-15-02_report.md"
Enter
Sleep 3000ms
