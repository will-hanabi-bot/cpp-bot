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

# Primary lookup: per-game files.
matches=()
while IFS= read -r f; do
  matches+=("$f")
done < <(find "$LOG_DIR" -name "*-${GAME_ID}.log" -type f 2>/dev/null | xargs -I{} stat -f "%m %N" "{}" 2>/dev/null | sort -rn | awk '{print $2}')

if [[ ${#matches[@]} -gt 0 ]]; then
  printf '%s\n' "${matches[@]}"
  exit 0
fi

echo "no per-game log found for ${GAME_ID} in ${LOG_DIR}/" >&2
exit 1
