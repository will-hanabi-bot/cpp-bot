#!/usr/bin/env bash
# Usage: scripts/find_game.sh <game_id>
# Prints all log files that contain the given game id, newest mtime first.
# The convention is `logs/{bot_name}-{game_id}.log`; this script tolerates
# variations in bot name and also greps inside `bot-*.log` files for the
# game id (in case a fallback log captured it).
set -euo pipefail
if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <game_id>" >&2
  exit 2
fi
GAME_ID=$1
LOG_DIR="${LOG_DIR:-logs}"

# mtime + path for one file. BSD stat (macOS) spells this `-f "%m %N"`;
# GNU coreutils stat (Linux, MSYS2/Git-Bash on Windows) spells it
# `-c "%Y %n"`. Probe once rather than sniffing $OSTYPE — MSYS2 and Git
# Bash both report a Windows-ish OSTYPE but ship GNU stat.
if stat -c '%Y' . >/dev/null 2>&1; then
  stat_mtime() { stat -c '%Y %n' "$@"; }
else
  stat_mtime() { stat -f '%m %N' "$@"; }
fi

# Primary lookup: per-game files.
matches=()
while IFS= read -r f; do
  matches+=("$f")
done < <(find "$LOG_DIR" -name "*-${GAME_ID}.log" -type f 2>/dev/null | while IFS= read -r p; do stat_mtime "$p"; done 2>/dev/null | sort -rn | cut -d' ' -f2-)

if [[ ${#matches[@]} -gt 0 ]]; then
  printf '%s\n' "${matches[@]}"
  exit 0
fi

echo "no per-game log found for ${GAME_ID} in ${LOG_DIR}/" >&2
exit 1
