#!/usr/bin/env bash
# Usage: scripts/bug_to_test.sh <log_file> <turn> [category] [slug] [convention]
#
# End-to-end bug-report → reproducing test pipeline.
#   1. Locate the STATE snapshot at <turn> in the log.
#   2. Re-run with current code (replay_log --rerun) and print the action.
#   3. Emit a regression-test scaffold at
#      tests/<category>/test_replay_<gid>[_<slug>].cpp
#      (category defaults to the convention's test_misc folder; slug is the
#      bug report's short snake_case issue description, per CLAUDE.md).
#   4. Build + run that one test.
#
# Pre-requisites: replay_log binary built (cmake --build build --target replay_log).
set -euo pipefail
if [[ $# -lt 2 || $# -gt 5 ]]; then
  echo "Usage: $0 <log_file> <turn> [category] [slug] [convention]" >&2
  exit 2
fi
LOG=$1
TURN=$2
CATEGORY=${3:-}
SLUG=${4:-}
CONVENTION=${5:-}

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
# Which convention did this game actually run? The log is authoritative --
# it records what executed -- so read it from the contents unless the caller
# overrode it. Missing key means a log written before conventions were
# recorded, which was always reactor.
if [[ -z "$CONVENTION" ]]; then
  CONVENTION=$(grep -o '"convention":"[a-z0-9]*"' "$LOG" | head -1 | cut -d'"' -f4)
fi
CONVENTION=${CONVENTION:-reactor}
case "$CONVENTION" in
  reactor0) TARGET=hanabi_reactor0_tests; LABEL='^reactor0$'; DEFAULT_CATEGORY=test_reactor0/test_misc ;;
  reactor)  TARGET=hanabi_reactor_tests;  LABEL='^reactor$';  DEFAULT_CATEGORY=test_reactor/test_misc ;;
  *) echo "unknown convention '$CONVENTION' (expected reactor or reactor0)" >&2; exit 2 ;;
esac
CATEGORY=${CATEGORY:-$DEFAULT_CATEGORY}
echo "convention: $CONVENTION -> target $TARGET, category $CATEGORY"

mkdir -p "$REPO_ROOT/tests/${CATEGORY}"
OUT="$REPO_ROOT/tests/${CATEGORY}/test_replay_${GAME_ID}${SLUG:+_$SLUG}.cpp"

echo "=== rerun ==="
"$BIN" "$LOG" --turn "$TURN" --rerun

echo
echo "=== emit-test → $OUT ==="
"$BIN" "$LOG" --turn "$TURN" --emit-test "$OUT"

echo
echo "NOTE: add ${OUT#$REPO_ROOT/} to the ${TARGET} source list in CMakeLists.txt."
if [[ "$CONVENTION" == "reactor0" ]]; then
  echo "NOTE: the emitted snapshot must carry \"convention\": \"reactor0\", or"
  echo "      apply_snapshot replays it under reactor (the missing-key default)."
fi

echo
echo "=== building + running new test ==="
(cd "$REPO_ROOT" && cmake --build build -j 8 --target "$TARGET")
(cd "$REPO_ROOT/build" && ctest -L "$LABEL" -R "Game${GAME_ID}Turn${TURN}" --output-on-failure)
