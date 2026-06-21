#!/usr/bin/env bash
# Usage: scripts/bug_to_test.sh <log_file> <turn>
#
# End-to-end bug-report → reproducing test pipeline.
#   1. Locate the STATE snapshot at <turn> in the log.
#   2. Re-run with current code (replay_log --rerun) and print the action.
#   3. Emit a regression-test scaffold at tests/test_endgame/test_replay_<gid>.cpp.
#   4. Build + run that one test.
#
# Pre-requisites: replay_log binary built (cmake --build build --target replay_log).
set -euo pipefail
if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <log_file> <turn>" >&2
  exit 2
fi
LOG=$1
TURN=$2

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$REPO_ROOT/build/replay_log"
if [[ ! -x "$BIN" ]]; then
  echo "replay_log not built. Run: cmake --build build --target replay_log" >&2
  exit 2
fi

GAME_ID=$(basename "$LOG" .log | awk -F- '{print $NF}')
OUT="$REPO_ROOT/tests/test_endgame/test_replay_${GAME_ID}.cpp"

echo "=== rerun ==="
"$BIN" "$LOG" --turn "$TURN" --rerun

echo
echo "=== emit-test → $OUT ==="
"$BIN" "$LOG" --turn "$TURN" --emit-test "$OUT"

echo
echo "=== building + running new test ==="
(cd "$REPO_ROOT" && cmake --build build -j 8 --target hanabi_tests)
(cd "$REPO_ROOT/build" && ctest -R "Game${GAME_ID}Turn${TURN}" --output-on-failure)
