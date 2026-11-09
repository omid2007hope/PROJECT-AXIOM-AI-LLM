#!/bin/bash

clear
sleep 0.5

echo ""
echo "  ╭──────────────────────────────────────────────────────╮"
echo "  │  OpenOSINT  v2.2.0        provider: Claude Sonnet    │"
echo "  ╰──────────────────────────────────────────────────────╯"
echo ""
echo "  Type a target or question. 'help' for commands. 'exit' to quit."
echo ""
sleep 1

echo -n "  openosint ❯ "
sleep 0.8
echo "investigate target@example.com"
echo ""
sleep 0.6

echo "  Thinking..."
echo ""
sleep 0.8

echo "  → generate_dorks(target='target@example.com')"
sleep 1.5
echo "  ✓ Generated 12 dork URLs"
echo ""
sleep 0.4

echo "  → search_email(email='target@example.com')"
sleep 2.5
echo "  ✓ Found: Spotify, WordPress, Gravatar, Office365"
echo ""
sleep 0.4

echo "  → search_breach(email='target@example.com')"
sleep 1.8
echo "  ✓ Found in 2 breaches: LinkedIn (2016), Adobe (2013)"
echo ""
sleep 0.4

echo "  → search_paste(query='target@example.com')"
sleep 1.5
echo "  ✗ No results found"
echo ""
sleep 0.8

echo "  ╭─────────────────────── Report ───────────────────────╮"
sleep 0.2
echo "  │                                                       │"
sleep 0.2
echo "  │  ## Summary                                           │"
sleep 0.2
echo "  │  Single target identified — high confidence.          │"
sleep 0.2
echo "  │                                                       │"
sleep 0.2
echo "  │  ## Online Presence                                   │"
sleep 0.2
echo "  │  Spotify · WordPress · Gravatar · Office365           │"
sleep 0.2
echo "  │                                                       │"
sleep 0.2
echo "  │  ## Data Breaches                                     │"
sleep 0.2
echo "  │  LinkedIn (2016) · Adobe (2013)                       │"
sleep 0.2
echo "  │                                                       │"
sleep 0.2
echo "  │  ## Conclusion                                        │"
sleep 0.2
echo "  │  Moderate footprint. Credential rotation advised.     │"
sleep 0.2
echo "  │                                                       │"
echo "  ╰───────────────────────────────────────────────────────╯"
echo ""
sleep 0.5
echo "  ✓ Report saved → reports/2026-05-11_14-32-11_report.md"
echo ""
sleep 1.2

echo -n "  openosint ❯ "
sleep 0.8
echo "find accounts for johndoe99"
echo ""
sleep 0.6

echo "  Thinking..."
echo ""
sleep 0.6

echo "  → search_username(username='johndoe99')"
sleep 2.8
echo "  ✓ Found 8 accounts"
echo ""
sleep 0.8

echo "  ╭─────────────────────── Report ───────────────────────╮"
sleep 0.2
echo "  │                                                       │"
sleep 0.2
echo "  │  ## Online Presence                                   │"
sleep 0.2
echo "  │  GitHub · Twitter · Reddit · HackerNews              │"
sleep 0.2
echo "  │  GitLab · Replit · Dev.to · Keybase                  │"
sleep 0.2
echo "  │                                                       │"
sleep 0.2
echo "  │  ## Conclusion                                        │"
sleep 0.2
echo "  │  Active developer profile. Consistent username        │"
sleep 0.2
echo "  │  across technical platforms.                          │"
sleep 0.2
echo "  │                                                       │"
echo "  ╰───────────────────────────────────────────────────────╯"
echo ""
sleep 0.5
echo "  ✓ Report saved → reports/2026-05-11_14-35-02_report.md"
echo ""
sleep 2