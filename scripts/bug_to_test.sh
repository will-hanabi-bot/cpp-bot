#!/usr/bin/env bash
# Usage: scripts/bug_to_test.sh <log_file> <turn> [category] [slug]
#
# End-to-end bug-report → reproducing test pipeline.
#   1. Locate the STATE snapshot at <turn> in the log.
#   2. Re-run with current code (replay_log --rerun) and print the action.
#   3. Emit a regression-test scaffold at
#      tests/<category>/test_replay_<gid>[_<slug>].cpp
#      (category defaults to test_endgame; slug is the bug report's short
#      snake_case issue description, per CLAUDE.md "Replay-test standards").
#   4. Build + run that one test.
#
# Pre-requisites: replay_log binary built (cmake --build build --target replay_log).
set -euo pipefail
if [[ $# -lt 2 || $# -gt 4 ]]; then
  echo "Usage: $0 <log_file> <turn> [category] [slug]" >&2
  exit 2
fi
LOG=$1
TURN=$2
CATEGORY=${3:-test_endgame}
SLUG=${4:-}

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$REPO_ROOT/build/replay_log"
if [[ ! -x "$BIN" ]]; then
  echo "replay_log not built. Run: cmake --build build --target replay_log" >&2
  exit 2
fi

# Take the id from the log's *contents*, not its filename: a finished log
# carries both `database_id` (the hanab.live replay id) and `game_id` (the
# live table id), and replay_log names the emitted TEST after the database
# id when present. Deriving it from the filename instead would break the
# ctest -R match below the moment a log is finalized under its replay id.
GAME_ID=$(grep -o '"database_id":[0-9]*' "$LOG" | head -1 | cut -d: -f2)
if [[ -z "$GAME_ID" ]]; then
  GAME_ID=$(grep -o '"game_id":[0-9]*' "$LOG" | head -1 | cut -d: -f2)
fi
if [[ -z "$GAME_ID" ]]; then
  echo "no database_id or game_id record found in $LOG" >&2
  exit 2
fi
mkdir -p "$REPO_ROOT/tests/${CATEGORY}"
OUT="$REPO_ROOT/tests/${CATEGORY}/test_replay_${GAME_ID}${SLUG:+_$SLUG}.cpp"

echo "=== rerun ==="
"$BIN" "$LOG" --turn "$TURN" --rerun

echo
echo "=== emit-test → $OUT ==="
"$BIN" "$LOG" --turn "$TURN" --emit-test "$OUT"

echo
echo "NOTE: add ${OUT#$REPO_ROOT/} to the hanabi_tests source list in CMakeLists.txt."

echo
echo "=== building + running new test ==="
(cd "$REPO_ROOT" && cmake --build build -j 8 --target hanabi_tests)
(cd "$REPO_ROOT/build" && ctest -R "Game${GAME_ID}Turn${TURN}" --output-on-failure)
