#!/usr/bin/env bash
# Usage: scripts/backfill_database_ids.sh [--apply]
#
# Retro-fits per-game logs written before v1.12.0, when the bot never
# learned a game's hanab.live database id and left every log named after
# its live table id (`will-bot67-1136.log` rather than
# `will-bot67-1940573.log`).
#
# The mapping is recoverable: the raw process transcripts `logs/bot-*.log`
# record the line the bot used to ignore —
#   <- finishOngoingGame {"databaseID":1940573,...,"tableID":1136}
# For each per-game log whose table id appears there, this appends a
# `database_id` LIFECYCLE record and renames the file, matching what
# GameLogger::finalize_with_database_id now does live.
#
# Difference from the live path: this only *appends* the mapping record, it
# does not stamp `database_id` onto every pre-existing record. Rewriting
# historical logs in place is not worth the risk, and both replay_log and
# scripts/bug_to_test.sh find the id from the appended record either way.
#
# Dry run by default. Pass --apply to actually move files.
set -euo pipefail

APPLY=0
if [[ $# -gt 1 ]]; then
  echo "Usage: $0 [--apply]" >&2
  exit 2
fi
if [[ $# -eq 1 ]]; then
  if [[ "$1" == "--apply" ]]; then
    APPLY=1
  else
    echo "Usage: $0 [--apply]" >&2
    exit 2
  fi
fi

LOG_DIR="${LOG_DIR:-logs}"
if [[ ! -d "$LOG_DIR" ]]; then
  echo "no ${LOG_DIR}/ directory" >&2
  exit 2
fi

shopt -s nullglob
transcripts=("$LOG_DIR"/bot-*.log)
if [[ ${#transcripts[@]} -eq 0 ]]; then
  echo "no ${LOG_DIR}/bot-*.log transcripts — nothing to recover the mapping from" >&2
  exit 1
fi

# table_id -> database_id, harvested from every transcript.
declare -A DB_OF_TABLE
while IFS= read -r line; do
  db=$(echo "$line" | grep -o '"databaseID":[0-9]*' | head -1 | cut -d: -f2)
  tbl=$(echo "$line" | grep -o '"tableID":[0-9]*' | head -1 | cut -d: -f2)
  if [[ -n "$db" && -n "$tbl" ]]; then
    DB_OF_TABLE[$tbl]=$db
  fi
done < <(grep -h 'finishOngoingGame' "${transcripts[@]}" 2>/dev/null || true)

if [[ ${#DB_OF_TABLE[@]} -eq 0 ]]; then
  echo "no finishOngoingGame lines found in ${transcripts[*]}" >&2
  exit 1
fi
echo "recovered ${#DB_OF_TABLE[@]} table->database mappings"

[[ $APPLY -eq 1 ]] || echo "(dry run — pass --apply to move files)"

renamed=0
skipped=0
for log in "$LOG_DIR"/*.log; do
  base=$(basename "$log" .log)
  # Skip the raw transcripts themselves.
  [[ "$base" =~ ^bot-[0-9]+$ ]] && continue

  table_id=${base##*-}
  [[ "$table_id" =~ ^[0-9]+$ ]] || continue

  db_id=${DB_OF_TABLE[$table_id]:-}
  if [[ -z "$db_id" ]]; then
    echo "  skip  $log — no mapping for table ${table_id}"
    skipped=$((skipped + 1))
    continue
  fi

  if grep -q '"database_id"' "$log" 2>/dev/null; then
    echo "  skip  $log — already carries a database_id record"
    skipped=$((skipped + 1))
    continue
  fi

  bot=${base%-*}
  target="$LOG_DIR/${bot}-${db_id}.log"
  if [[ -e "$target" ]]; then
    echo "  skip  $log — target ${target} already exists"
    skipped=$((skipped + 1))
    continue
  fi

  echo "  move  $log -> $target  (table ${table_id} -> database ${db_id})"
  if [[ $APPLY -eq 1 ]]; then
    printf '{"bot":"%s","ch":"LIFECYCLE","database_id":%s,"event":"database_id","game_id":%s,"ts":"%s"}\n' \
      "$bot" "$db_id" "$table_id" "$(date +%Y-%m-%dT%H:%M:%S.000)" >> "$log"
    mv "$log" "$target"
  fi
  renamed=$((renamed + 1))
done

echo
if [[ $APPLY -eq 1 ]]; then
  echo "renamed ${renamed}, skipped ${skipped}"
else
  echo "would rename ${renamed}, skip ${skipped}"
fi
