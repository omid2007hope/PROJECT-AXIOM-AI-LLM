# email-investigation.tape
# Charmbracelet VHS — https://github.com/charmbracelet/vhs
# Demo: AI-powered email investigation in the OpenOSINT REPL
# Run: vhs media/tapes/email-investigation.tape  (from repo root)

Output media/output/email-investigation.gif
Output media/output/email-investigation.mp4

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

# ── Scene 2: user submits investigation query ──────────────────────────────
Type "openosint ❯ investigate john.doe.dev@proton.me"
Enter
Sleep 700ms

Type "  Thinking..."
Enter
Sleep 1800ms

# ── Scene 3: AI calls search_email ────────────────────────────────────────
Type "  → search_email(email='john.doe.dev@proton.me')"
Enter
Sleep 2200ms

Type "    Spotify       account found  (spotify.com/user/jdoedev)"
Enter
Sleep 250ms
Type "    GitHub        account found  (github.com/john-doe-dev)"
Enter
Sleep 250ms
Type "    Gravatar      account found  (gravatar.com/jdoedev)"
Enter
Sleep 250ms
Type "    LinkedIn      account found  (linkedin.com/in/johndoedev)"
Enter
Sleep 250ms
Type "    WordPress     account found  (johndoedev.wordpress.com)"
Enter
Sleep 1400ms

# ── Scene 4: AI calls search_breach ───────────────────────────────────────
Type "  → search_breach(email='john.doe.dev@proton.me')"
Enter
Sleep 2000ms

Type "    LinkedIn 2016 — 164M records  (emails, hashed passwords)"
Enter
Sleep 300ms
Type "    Adobe 2013    — 153M records  (emails, hashed passwords, usernames)"
Enter
Sleep 1400ms

# ── Scene 5: AI calls generate_dorks ──────────────────────────────────────
Type "  → generate_dorks(target='john.doe.dev@proton.me')"
Enter
Sleep 1400ms

Type "    Generated 6 dork URLs for Google / Bing"
Enter
Sleep 1200ms

# ── Scene 6: final compiled report ────────────────────────────────────────
Enter
Type "  ## Email Investigation Report"
Enter
Enter
Type "  Target:   john.doe.dev@proton.me"
Enter
Type "  Accounts: Spotify · GitHub · Gravatar · LinkedIn · WordPress (5 found)"
Enter
Type "  Breaches: LinkedIn 2016 · Adobe 2013 (credential rotation advised)"
Enter
Type "  Dorks:    6 search URLs generated for deeper research"
Enter
Type "  Verdict:  Single identity confirmed across platforms — High confidence"
Enter
Enter
Type "  ✓ Report saved → reports/2026-05-19_21-04-37_report.md"
Enter
Sleep 3500ms
