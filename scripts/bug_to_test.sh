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

GAME_ID=$(basename "$LOG" .log | awk -F- '{print $NF}')
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
