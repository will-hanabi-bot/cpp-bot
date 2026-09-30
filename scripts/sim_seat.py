#!/usr/bin/env python3
"""Rerun a turn from a seat that has no log -- what a bot in that chair would do.

Usage:
  scripts/sim_seat.py <game_id> <seat> <turn> [--trace] [--keep FILE] [--logs DIR]

A human seat leaves no per-game log, so `replay_log --rerun` cannot be pointed at
it. This builds one the way `tiiah_stacks.py` simulates a seat: the most complete
bot log's action history, cut back to <turn>, replayed from <seat>'s chair -- that
seat's own draws hidden, everyone else's filled in from the other logs. Then it
runs `replay_log --turn <turn> --rerun` (with `--trace` if asked) on it.

<seat> is the player index (0-based). <turn> must be a turn on which <seat> acts.
`--keep FILE` writes the synthetic log there instead of a temporary directory, so
it can be fed to `replay_log` or `bug_to_test.sh` by hand.
"""
from __future__ import annotations

import argparse
import glob
import json
import os
import subprocess
import sys
import tempfile

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tiiah_stacks import REPLAY_LOG, ROOT, cut_actions, load, states  # noqa: E402


def synthetic_record(game_id: str, seat: int, turn: int, logs: str) -> dict:
    paths = sorted(glob.glob(os.path.join(logs, f"*-{game_id}.log")))
    paths = [p for p in paths if states(load(p))]
    if not paths:
        raise SystemExit(f"no per-game logs with STATE records for {game_id}")
    later, ids = [], {}
    for p in paths:
        st = states(load(p))
        after = [s for s in st if s["turn"] >= turn]
        if after:
            later.append(after[0])
        for a in st[-1]["replay"]["actions"]:
            if a.get("t") == "draw" and a.get("suit", -1) != -1:
                ids[a["order"]] = (a["suit"], a["rank"])
    if not later:
        raise SystemExit(f"turn {turn} is past every log's last STATE")
    base = max(later, key=lambda r: len(r["replay"]["actions"]))
    rec = json.loads(json.dumps(base))
    replay = rec["replay"]
    actions = cut_actions(replay["actions"], turn)
    for a in actions:
        if a.get("t") != "draw":
            continue
        if a["p"] == seat:
            a["suit"], a["rank"] = -1, -1
        elif a.get("suit", -1) == -1 and a["order"] in ids:
            a["suit"], a["rank"] = ids[a["order"]]
    replay["actions"] = actions
    replay["our_player_index"] = seat
    replay.pop("zcs_turn", None)
    rec.pop("debug", None)
    rec["turn"] = turn
    rec["bot"] = f"sim-seat{seat}"
    return rec


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("game_id")
    ap.add_argument("seat", type=int)
    ap.add_argument("turn", type=int)
    ap.add_argument("--trace", action="store_true")
    ap.add_argument("--keep", help="write the synthetic log here")
    ap.add_argument("--logs", default=str(ROOT / "logs"))
    args = ap.parse_args()

    rec = synthetic_record(args.game_id, args.seat, args.turn, args.logs)
    with tempfile.TemporaryDirectory() as tmp:
        path = args.keep or os.path.join(tmp, f"sim-seat{args.seat}-{args.game_id}.log")
        with open(path, "w", encoding="utf-8") as f:
            f.write(json.dumps(rec) + "\n")
        cmd = [str(REPLAY_LOG), path, "--turn", str(args.turn), "--rerun"]
        if args.trace:
            cmd.append("--trace")
        out = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8")
        sys.stdout.write(out.stdout)
        sys.stderr.write(out.stderr)
        return out.returncode


if __name__ == "__main__":
    sys.exit(main())
