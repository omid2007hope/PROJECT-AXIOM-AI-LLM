# ip-intel.tape
# Charmbracelet VHS — https://github.com/charmbracelet/vhs
# Demo: IP intelligence lookup in the OpenOSINT REPL
# Run: vhs media/tapes/ip-intel.tape  (from repo root)

Output media/output/ip-intel.gif
Output media/output/ip-intel.mp4

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

# ── Scene 2: user asks about an IP ────────────────────────────────────────
Type "openosint ❯ what do you know about 185.220.101.47"
Enter
Sleep 700ms

Type "  Thinking..."
Enter
Sleep 1500ms

# ── Scene 3: search_ip called ─────────────────────────────────────────────
Type "  → search_ip(ip='185.220.101.47')"
Enter
Sleep 1800ms

Type "    IP:          185.220.101.47"
Enter
Sleep 200ms
Type "    Hostname:    tor-exit-relay.privex.io"
Enter
Sleep 200ms
Type "    Country:     DE  (Germany)"
Enter
Sleep 200ms
Type "    City:        Frankfurt am Main"
Enter
Sleep 200ms
Type "    Org:         Privex Inc."
Enter
Sleep 200ms
Type "    ASN:         AS201814 (MEVSPACE sp. z o.o.)"
Enter
Sleep 1200ms

# ── Scene 4: search_whois called ──────────────────────────────────────────
Type "  → search_whois(target='185.220.101.47')"
Enter
Sleep 1800ms

Type "    Registrar:   RIPE Network Coordination Centre"
Enter
Sleep 200ms
Type "    Registered:  2019-03-14"
Enter
Sleep 200ms
Type "    NetRange:    185.220.101.0 / 24"
Enter
Sleep 200ms
Type "    Abuse email: abuse@privex.io"
Enter
Sleep 1400ms

# ── Scene 5: structured verdict ───────────────────────────────────────────
Enter
Type "  IP Intelligence Summary — 185.220.101.47"
Enter
Enter
Type "  Classification: Tor Exit Node (confirmed)"
Enter
Type "  Abuse flags:    Tor relay  ·  Known anonymizer  ·  Datacenter IP"
Enter
Type "  Risk level:     High — traffic from this IP cannot be attributed"
Enter
Type "  Recommendation: Block or flag at network perimeter"
Enter
Sleep 3200ms
