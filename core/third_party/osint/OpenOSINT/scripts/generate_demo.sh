#!/usr/bin/env bash
# scripts/generate_demo.sh
# OpenOSINT v2.8.0 — simulated OSINT investigation for demo recording.
# Run via: asciinema rec ... --command "bash scripts/generate_demo.sh"

# ── ANSI palette (Tokyo Night) ───────────────────────────────────────────────
R=$'\033[0m'
B=$'\033[1m'
DIM=$'\033[2m'
GRN=$'\033[38;2;0;255;136m'     # #00ff88  accent green
WHT=$'\033[38;2;241;245;249m'   # #f1f5f9  primary text
YLW=$'\033[38;2;224;175;104m'   # #e0af68  yellow / warning
SLT=$'\033[38;2;148;163;184m'   # #94a3b8  secondary / dim text
BRD=$'\033[38;2;30;41;59m'      # #1e293b  border colour

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
  printf "\r%-70s\r" ""
}

# 120-col full-width panel (Rich Panel, border_style green, padding=(1,2))
# Top / bottom border: ╭ + 118 × ─ + ╮ = 120 chars
# Content line:        │ + 2sp + <114 visible chars> + 2sp + │ = 120 chars
_DASHES=$(printf '─%.0s' {1..118})

panel_top()    { printf "${GRN}╭%s╮${R}\n" "$_DASHES"; }
panel_bottom() { printf "${GRN}╰%s╯${R}\n" "$_DASHES"; }
panel_blank()  { printf "${GRN}│${R}%118s${GRN}│${R}\n" ""; }

panel_row() {
  # Strip ANSI codes to measure visible length, then right-pad to 114 chars.
  local content="$1"
  local visible
  visible=$(printf '%s' "$content" | sed $'s/\033\[[0-9;]*m//g')
  local vlen=${#visible}
  local pad=$(( 114 - vlen ))
  (( pad < 0 )) && pad=0
  printf "${GRN}│${R}  %s%*s  ${GRN}│${R}\n" "$content" "$pad" ""
}

# ── 1. Banner ─────────────────────────────────────────────────────────────────
clear
sleep 0.8

# Rich Panel.fit — fits the single content line (73 chars wide total)
printf "${BRD}╭───────────────────────────────────────────────────────────────────────╮${R}\n"
printf "${BRD}│${R}  ${B}${GRN}OpenOSINT${R} ${DIM}v2.8.0${R}  ${DIM}·${R}  ${DIM}Provider: Anthropic (claude-sonnet-4-20250514)${R}  ${BRD}│${R}\n"
printf "${BRD}╰───────────────────────────────────────────────────────────────────────╯${R}\n"
printf "\n"
printf "  Type a target or question. ${DIM}'help'${R} for commands. ${DIM}'exit'${R} to quit.\n\n"
printf "${YLW}⭐${R} ${DIM}If OpenOSINT is useful, star it → https://github.com/OpenOSINT/OpenOSINT${R}\n\n"

sleep 1.2

# ── 2. Investigation 1 — email ───────────────────────────────────────────────
printf "  ${B}${GRN}openosint${R} ${WHT}❯${R} "
type_text "investigate target@example.com" 0.06
printf "\n\n"
sleep 0.4

printf "  ${DIM}Thinking...${R}\n\n"
sleep 0.7

printf "  ${DIM}→${R} ${GRN}generate_dorks${R}${DIM}(target='target@example.com')${R}\n"
spinner "Generating dork URLs..." 2
printf "  ${GRN}✓${R} Generated ${WHT}12${R} targeted Google dork URLs\n\n"
sleep 0.5

printf "  ${DIM}→${R} ${GRN}search_email${R}${DIM}(email='target@example.com')${R}\n"
spinner "Scanning platforms via holehe..." 5
printf "  ${GRN}✓${R} Found accounts on ${WHT}4${R} platforms: ${SLT}Spotify · WordPress · Gravatar · Office365${R}\n\n"
sleep 0.5

printf "  ${DIM}→${R} ${GRN}search_breach${R}${DIM}(email='target@example.com')${R}\n"
spinner "Checking HaveIBeenPwned..." 4
printf "  ${YLW}⚠${R}  Found in ${WHT}2${R} data breaches: ${SLT}LinkedIn (2016) · Adobe (2013)${R}\n\n"
sleep 0.5

printf "  ${DIM}→${R} ${GRN}search_paste${R}${DIM}(query='target@example.com')${R}\n"
spinner "Searching paste dumps..." 3
printf "  ${DIM}✗ No paste results found${R}\n\n"
sleep 0.8

printf "\n"
panel_top
panel_blank
panel_row "${B}${WHT}## Summary${R}"
panel_row "${WHT}Single identity confirmed — high confidence.${R}"
panel_blank
panel_row "${B}${WHT}## Online Presence${R}"
panel_row "${WHT}Spotify · WordPress · Gravatar · Office365${R}"
panel_blank
panel_row "${B}${WHT}## Data Breaches${R}"
panel_row "${YLW}⚠  LinkedIn (2016)${R}${SLT} — leaked: Email addresses, Passwords, Names${R}"
panel_row "${YLW}⚠  Adobe    (2013)${R}${SLT} — leaked: Email addresses, Passwords, Usernames${R}"
panel_blank
panel_row "${B}${WHT}## Conclusion & Recommendations${R}"
panel_row "${WHT}Moderate digital footprint. Credential rotation strongly advised.${R}"
panel_row "${WHT}Monitor HaveIBeenPwned for future breach notifications.${R}"
panel_blank
panel_bottom
printf "\n"

sleep 0.4
printf "  ${DIM}✓ Report saved → reports/2026-05-18_14-32-11_report.md${R}\n"
printf "  ${DIM}✓ PDF saved     → reports/2026-05-18_14-32-11_report.pdf${R}\n\n"
sleep 2.0

# ── 3. Investigation 2 — username ────────────────────────────────────────────
printf "  ${B}${GRN}openosint${R} ${WHT}❯${R} "
type_text "find all accounts for johndoe99" 0.06
printf "\n\n"
sleep 0.4

printf "  ${DIM}Thinking...${R}\n\n"
sleep 0.6

printf "  ${DIM}→${R} ${GRN}search_username${R}${DIM}(username='johndoe99')${R}\n"
spinner "Searching 300+ platforms via sherlock..." 7
printf "  ${GRN}✓${R} Found on ${WHT}8${R} platforms:\n"
printf "    ${SLT}GitHub · Twitter · Reddit · HackerNews · GitLab · Replit · Dev.to · Keybase${R}\n\n"
sleep 0.5

printf "  ${DIM}→${R} ${GRN}search_paste${R}${DIM}(query='johndoe99')${R}\n"
spinner "Searching paste dumps..." 3
printf "  ${DIM}✗ No paste results${R}\n\n"
sleep 0.8

printf "\n"
panel_top
panel_blank
panel_row "${B}${WHT}## Summary${R}"
panel_row "${WHT}Username 'johndoe99' — active developer profile, 8 platforms confirmed.${R}"
panel_blank
panel_row "${B}${WHT}## Online Presence${R}"
panel_row "${WHT}GitHub · Twitter · Reddit · HackerNews${R}"
panel_row "${WHT}GitLab · Replit · Dev.to · Keybase${R}"
panel_blank
panel_row "${B}${WHT}## Conclusion${R}"
panel_row "${WHT}Consistent username across technical platforms. Likely a software developer.${R}"
panel_row "${WHT}Cross-reference GitHub contributions for further identity signals.${R}"
panel_blank
panel_bottom
printf "\n"

sleep 0.4
printf "  ${DIM}✓ Report saved → reports/2026-05-18_14-36-44_report.md${R}\n\n"
sleep 3.0
