#!/usr/bin/env bash
# Usage: scripts/find_game.sh <id>
# Prints all log files matching the given id, newest mtime first. Accepts
# either id a game has: the hanab.live *database id* (the big number in a
# replay URL, which a finished log is named after) or the live *table id*
# (the small number the log ran under).
#
# Three lookups, in order:
#   1. filename `logs/{bot}-{id}.log` — tolerates any bot name;
#   2. log contents — matches a `database_id` or `game_id` record, which
#      finds logs written before v1.12.0 started renaming them;
#   3. the raw `bot-*.log` transcripts — translates a database id to its
#      table id via the `finishOngoingGame` line that carries both, then
#      retries (1) and (2).
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

by_mtime() {
  while IFS= read -r p; do stat_mtime "$p"; done 2>/dev/null | sort -rn | cut -d' ' -f2-
}

# (1) Filename match.
find_by_name() {
  find "$LOG_DIR" -name "*-${1}.log" -type f 2>/dev/null | by_mtime
}

# (2) Content match on a database_id or game_id record. Excludes the raw
# bot-*.log transcripts, which mention every id the process ever saw.
find_by_content() {
  find "$LOG_DIR" -name '*.log' -type f 2>/dev/null \
    | grep -v '/bot-[0-9]*\.log$' \
    | while IFS= read -r p; do
        if grep -qE "\"(database_id|game_id)\":${1}([,}])" "$p" 2>/dev/null; then
          echo "$p"
        fi
      done | by_mtime
}

report() {
  if [[ -n "$1" ]]; then
    printf '%s\n' "$1"
    exit 0
  fi
}

report "$(find_by_name "$GAME_ID")"
report "$(find_by_content "$GAME_ID")"

# (3) Translate a database id to its table id. The raw transcripts carry
# `finishOngoingGame {"databaseID":1940573,...,"tableID":1136}`, which is the
# only place the two ids appear together for a log written before the
# rename landed.
TABLE_ID=$(grep -ho "\"databaseID\":${GAME_ID},[^}]*\"tableID\":[0-9]*" \
             "$LOG_DIR"/bot-*.log 2>/dev/null \
           | grep -o '"tableID":[0-9]*' | head -1 | cut -d: -f2)
if [[ -n "$TABLE_ID" ]]; then
  echo "note: database id ${GAME_ID} ran as table ${TABLE_ID}" >&2
  report "$(find_by_name "$TABLE_ID")"
  report "$(find_by_content "$TABLE_ID")"
fi

echo "no per-game log found for ${GAME_ID} in ${LOG_DIR}/" >&2
exit 1
