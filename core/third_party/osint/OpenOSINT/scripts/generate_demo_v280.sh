#!/usr/bin/env bash
# scripts/generate_demo_v280.sh
# OpenOSINT v2.8.0 — demo recording script for asciinema.
#
# All text sourced directly from openosint/ source files:
#   - Banner:       repl._print_banner (repl.py:76-95)
#   - Star prompt:  repl._print_banner (repl.py:93-95)
#   - History hint: repl.run (repl.py:343-346)
#   - Thinking:     repl._run_investigation (repl.py:268)
#   - Tool calls:   repl._print_tool_call (repl.py:148-150)
#   - Report panel: repl._print_result (repl.py:153-159)
#   - Save paths:   repl._run_investigation (repl.py:297,309)
#   - Goodbye:      repl.run (repl.py:355)
#   - Tool arg format: agent._print_tool_call → f"{k}={v!r}"
#   - Report headers: agent.SYSTEM_PROMPT sections
#
# Usage:
#   asciinema rec assets/demo_v280_raw.cast \
#     --command "bash scripts/generate_demo_v280.sh" \
#     --title "OpenOSINT v2.8.0" \
#     --cols 110 --rows 38 --overwrite

# ── ANSI palette (Tokyo Night) ───────────────────────────────────────────────
R=$'\033[0m'
B=$'\033[1m'
DIM=$'\033[2m'
GRN=$'\033[38;2;0;255;136m'    # #00ff88  accent green
WHT=$'\033[38;2;241;245;249m'  # #f1f5f9  primary text
YLW=$'\033[38;2;224;175;104m'  # #e0af68  warning / yellow
SLT=$'\033[38;2;148;163;184m'  # #94a3b8  secondary / dim text
BRD=$'\033[38;2;30;41;59m'     # #1e293b  border colour

# ── Helpers ──────────────────────────────────────────────────────────────────
type_text() {
  local txt="$1" spd="${2:-0.055}"
  local i
  for (( i=0; i<${#txt}; i++ )); do
    printf '%s' "${txt:$i:1}"
    sleep "$spd"
  done
}

spinner() {
  # spinner <message> <reps>   (reps × 10 frames × 0.08 s each)
  local msg="$1" reps="${2:-4}"
  local frames=("⠋" "⠙" "⠹" "⠸" "⠼" "⠴" "⠦" "⠧" "⠇" "⠏")
  local i frame
  for (( i=0; i<reps; i++ )); do
    for frame in "${frames[@]}"; do
      printf "\r  ${DIM}%s${R} ${SLT}%s${R}   " "$frame" "$msg"
      sleep 0.08
    done
  done
  printf "\r%-80s\r" ""
}

# ── Panel helpers — 110-col terminal ─────────────────────────────────────────
# _print_result uses Panel(Markdown(content), border_style="#00ff88", padding=(1,2))
# 110-col terminal → panel width = 110 → ╭ + 108 dashes + ╮
# Content area = 110 − 2 (│chars) − 2 (left pad) − 2 (right pad) = 104 chars

_DASHES=$(printf '─%.0s' {1..108})

panel_top()    { printf "${GRN}╭%s╮${R}\n" "$_DASHES"; }
panel_bottom() { printf "${GRN}╰%s╯${R}\n" "$_DASHES"; }
panel_blank()  { printf "${GRN}│${R}%108s${GRN}│${R}\n" ""; }

panel_row() {
  # Strip ANSI codes to measure visible length, pad to 104 visible chars.
  local content="$1"
  local visible
  visible=$(printf '%s' "$content" | sed $'s/\033\[[0-9;]*m//g')
  local vlen=${#visible}
  local pad=$(( 104 - vlen ))
  (( pad < 0 )) && pad=0
  printf "${GRN}│${R}  %s%*s  ${GRN}│${R}\n" "$content" "$pad" ""
}

# ── 1. Banner — repl._print_banner ───────────────────────────────────────────
# Panel.fit content: "OpenOSINT v2.8.0  ·  Provider: Anthropic (claude-sonnet-4-20250514)"
# Visible width = 67, padding=(0,2) → 71 inside → 73 chars total (71 dashes)
clear
sleep 0.8

printf "${BRD}╭───────────────────────────────────────────────────────────────────────╮${R}\n"
printf "${BRD}│${R}  ${B}${GRN}OpenOSINT${R} ${DIM}v2.8.0${R}  ${DIM}·${R}  ${DIM}Provider: Anthropic (claude-sonnet-4-20250514)${R}  ${BRD}│${R}\n"
printf "${BRD}╰───────────────────────────────────────────────────────────────────────╯${R}\n"
printf "\n"

# repl.py:89-90 — welcome line
printf "  Type a target or question. ${DIM}'help'${R} for commands. ${DIM}'exit'${R} to quit.\n\n"

# repl.py:93-95 — star prompt (printed to stderr in real code)
printf "${YLW}⭐${R} ${DIM}If OpenOSINT is useful, star it → https://github.com/OpenOSINT/OpenOSINT${R}\n"
printf "\n"

# repl.py:343-346 — session history hint (shown when count_sessions() > 0)
printf "  ${DIM}💾 3 sessions saved — type 'history' to browse${R}\n\n"

sleep 1.5

# ── 2. Investigation — email ──────────────────────────────────────────────────
# Prompt: HTML("<prompt>openosint</prompt> <prompt-text>❯</prompt-text> ")
printf "  ${B}${GRN}openosint${R} ${WHT}❯${R} "
type_text "investigate target@example.com" 0.06
printf "\n\n"
sleep 0.4

# repl.py:268 — "  Thinking..."
printf "  ${DIM}Thinking...${R}\n\n"
sleep 0.6

# Tool calls — _print_tool_call: f"  [dim]→[/] [#00ff88]{name}[/][dim]({arg_str})[/]"
# arg_str = ", ".join(f"{k}={v!r}" for k, v in args.items())

printf "  ${DIM}→${R} ${GRN}generate_dorks${R}${DIM}(target='target@example.com')${R}\n"
spinner "Generating dork URLs..." 2
printf "  ${GRN}✓${R} ${WHT}12${R} ${SLT}targeted Google dork URLs generated${R}\n\n"
sleep 0.4

printf "  ${DIM}→${R} ${GRN}search_email${R}${DIM}(email='target@example.com')${R}\n"
spinner "Scanning platforms via holehe..." 5
printf "  ${GRN}✓${R} ${WHT}4${R} ${SLT}platforms: Spotify · WordPress · Gravatar · Office365${R}\n\n"
sleep 0.4

printf "  ${DIM}→${R} ${GRN}search_breach${R}${DIM}(email='target@example.com')${R}\n"
spinner "Checking HaveIBeenPwned..." 4
printf "  ${YLW}⚠${R}  ${WHT}2${R} ${SLT}data breaches: LinkedIn (2016) · Adobe (2013)${R}\n\n"
sleep 0.4

printf "  ${DIM}→${R} ${GRN}search_paste${R}${DIM}(query='target@example.com')${R}\n"
spinner "Searching paste dumps..." 3
printf "  ${DIM}✗ No paste results found${R}\n\n"
sleep 0.8

# Report — _print_result: Panel(Markdown(content), border_style="#00ff88", padding=(1,2))
# Report sections from agent.SYSTEM_PROMPT:
#   ## Summary / ## Online Presence / ## Data Breaches (if any) / ## Conclusion & Recommendations
printf "\n"
panel_top
panel_blank
panel_row "${B}${WHT}## Summary${R}"
panel_row ""
panel_row "${WHT}Single identity confirmed — high confidence.${R}"
panel_blank
panel_row "${B}${WHT}## Online Presence${R}"
panel_row ""
panel_row "${WHT}Spotify · WordPress · Gravatar · Office365${R}"
panel_blank
panel_row "${B}${WHT}## Data Breaches (if any)${R}"
panel_row ""
panel_row "${YLW}⚠  LinkedIn${R} ${SLT}(2016-05-17) — leaked: Email addresses, Passwords, Names${R}"
panel_row "${YLW}⚠  Adobe   ${R} ${SLT}(2013-10-04) — leaked: Email addresses, Passwords, Usernames${R}"
panel_blank
panel_row "${B}${WHT}## Conclusion & Recommendations${R}"
panel_row ""
panel_row "${WHT}Moderate digital footprint. Credential rotation strongly advised.${R}"
panel_row "${WHT}Monitor HaveIBeenPwned for future breach notifications.${R}"
panel_blank
panel_bottom
printf "\n"

sleep 0.4

# repl.py:297 — "  [dim]✓ Report saved → {path}[/]"
# repl.py:309 — "  [dim]✓ PDF saved     → {pdf_path}[/]"
# Path format: reports/{YYYY-MM-DD_HH-MM-SS}_report.{md,pdf}
printf "  ${DIM}✓ Report saved → reports/2026-05-18_14-32-11_report.md${R}\n"
printf "  ${DIM}✓ PDF saved     → reports/2026-05-18_14-32-11_report.pdf${R}\n\n"
sleep 2.5

# ── 3. Investigation — username ───────────────────────────────────────────────
printf "  ${B}${GRN}openosint${R} ${WHT}❯${R} "
type_text "find all accounts for johndoe99" 0.06
printf "\n\n"
sleep 0.4

printf "  ${DIM}Thinking...${R}\n\n"
sleep 0.5

printf "  ${DIM}→${R} ${GRN}search_username${R}${DIM}(username='johndoe99')${R}\n"
spinner "Searching 300+ platforms via sherlock..." 7
printf "  ${GRN}✓${R} ${WHT}8${R} ${SLT}platforms found${R}\n\n"
sleep 0.4

printf "  ${DIM}→${R} ${GRN}search_paste${R}${DIM}(query='johndoe99')${R}\n"
spinner "Searching paste dumps..." 3
printf "  ${DIM}✗ No paste results${R}\n\n"
sleep 0.8

printf "\n"
panel_top
panel_blank
panel_row "${B}${WHT}## Summary${R}"
panel_row ""
panel_row "${WHT}Username 'johndoe99' — active developer profile, 8 platforms confirmed.${R}"
panel_blank
panel_row "${B}${WHT}## Online Presence${R}"
panel_row ""
panel_row "${WHT}GitHub · Twitter · Reddit · HackerNews · GitLab · Replit · Dev.to · Keybase${R}"
panel_blank
panel_row "${B}${WHT}## Conclusion & Recommendations${R}"
panel_row ""
panel_row "${WHT}Consistent username across technical platforms. Likely a software developer.${R}"
panel_row "${WHT}Cross-reference GitHub contributions for further identity signals.${R}"
panel_blank
panel_bottom
printf "\n"

sleep 0.4
printf "  ${DIM}✓ Report saved → reports/2026-05-18_14-36-44_report.md${R}\n\n"
sleep 1.8

# ── 4. history command ────────────────────────────────────────────────────────
# session_history.display_history_table — Rich Table, box.SIMPLE_HEAD
# Columns: #(dim,right) | Date(#f1f5f9) | Duration(dim,right) | Targets(#94a3b8) | Tools Used(#94a3b8) | Report(dim)
# Timestamp displayed as: session["timestamp"][:16]  → "2026-05-18T14:36"
# Duration from _fmt_duration: "1m44" for 104s, "2m11" for 131s
# Targets from _fmt_targets: single target → shown as-is
# Tools from _fmt_tools: ", ".join(tools)

printf "  ${B}${GRN}openosint${R} ${WHT}❯${R} "
type_text "history" 0.07
printf "\n"
sleep 0.5

printf "\n"
printf "  ${B}${GRN} #   Date              Duration  Targets                    Tools Used                       Report${R}\n"
printf "  ${BRD} ───  ────────────────  ────────  ─────────────────────────  ───────────────────────────────  ──────${R}\n"
printf "   ${DIM}2${R}   ${WHT}2026-05-18T14:36${R}     ${DIM}1m44${R}  ${SLT}johndoe99${R}                  ${SLT}search_username, search_paste${R}        ${DIM}—${R}\n"
printf "   ${DIM}1${R}   ${WHT}2026-05-18T14:32${R}     ${DIM}2m11${R}  ${SLT}target@example.com${R}         ${SLT}generate_dorks, search_email, +2${R}     ${DIM}yes${R}\n"
printf "\n"
sleep 2.0

# ── 5. exit ───────────────────────────────────────────────────────────────────
printf "  ${B}${GRN}openosint${R} ${WHT}❯${R} "
type_text "exit" 0.08
printf "\n"
sleep 0.3

# repl.py:355 — "\n[dim]Goodbye.[/]\n"
printf "\n${DIM}Goodbye.${R}\n\n"
sleep 1.2
