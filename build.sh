#!/bin/bash
# Rebuild just the hanabi_bot target.
#
#   ./build.sh          build/hanabi_bot.exe
#   ./build.sh --test   build/hanabi_bot_test.exe -- the same bot from the same
#                       source under another name, so it can be built while a
#                       running hanabi_bot.exe holds its own image locked.
#
# Two failure modes get special handling:
#
#  1. An interrupted build can leave build/.ninja_deps truncated; ninja then
#     aborts with "premature end of file". Clearing the metadata is cheap
#     (ninja regenerates it), so we retry once automatically.
#  2. Windows locks a running executable's image file, so the linker cannot
#     overwrite build/hanabi_bot.exe while a bot is running. This only bites
#     when a relink is actually required — while the binary is up to date the
#     build is a no-op and never opens it for writing — which is why it can
#     appear to start failing "for no reason" after a source change.
#
# pipefail is load-bearing: without it the exit status of the build pipeline
# was the trailing grep's, so a failed build left this script exiting 0.
set -euo pipefail

TARGET=hanabi_bot
case "${1:-}" in
  "") ;;
  --test) TARGET=hanabi_bot_test ;;
  *)
    echo "usage: $0 [--test]" >&2
    exit 2
    ;;
esac
if [ "$#" -gt 1 ]; then
  echo "usage: $0 [--test]" >&2
  exit 2
fi

LOG="$(mktemp)"
trap 'rm -f "$LOG"' EXIT

# Stream the build to the terminal while keeping a copy to classify failures
# against. Returns cmake's status (thanks to pipefail), not tee's.
run_build() {
  local status=0
  cmake --build build --target "$TARGET" -j 2>&1 | tee "$LOG" || status=$?
  return "$status"
}

status=0
run_build || status=$?

if [ "$status" -eq 0 ]; then
  exit 0
fi

# (1) Truncated ninja metadata — clear and retry, propagating the retry's
# status (it used to be discarded).
if grep -q 'premature end of file' "$LOG"; then
  echo "build.sh: ninja metadata truncated by an interrupted build; clearing and retrying" >&2
  rm -f build/.ninja_deps build/.ninja_log
  retry=0
  run_build || retry=$?
  exit "$retry"
fi

# (2) Locked output file — almost always a running bot holding its own image.
if grep -q 'cannot open output file' "$LOG"; then
  {
    echo
    echo "build.sh: the linker could not overwrite the $TARGET binary."
    echo "  A running executable is locked on Windows, so the build cannot"
    echo "  relink it. Stop the running bot(s), then re-run this script."
  } >&2
  if command -v powershell.exe >/dev/null 2>&1; then
    pids=$(powershell.exe -NoProfile -Command \
      "Get-Process $TARGET -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Id" \
      2>/dev/null | tr -d '\r' | tr '\n' ' ' || true)
    if [ -n "${pids// /}" ]; then
      echo "  Holding PIDs: $pids" >&2
    fi
  fi
  exit "$status"
fi

exit "$status"
