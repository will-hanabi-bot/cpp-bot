#!/usr/bin/env python3
"""Print every stack view of a Throw It in a Hole game at one turn.

Usage:
  scripts/tiiah_stacks.py <game_id> <turn> [--logs DIR]

In TIIAH no two seats hold the same stacks (tiiah/CONVENTION.md §1.3), so a
turn is not understood until all of these are on the page:

  * each seat's PRIVATE stacks (its belief: every play but its own unnamed ones),
  * each PAIR's stacks (what the two of them both know), from BOTH members' side,
  * the COMMON stacks (what all seats know), from every seat's side,
  * and the TRUE stacks, for reference.

Alice is the seat to act at <turn>, then Bob, then Cathy. Stacks print in short
form, one digit per suit in suit-index order (e.g. 43133).

Bot seats are read from their own per-game logs (`logs/<bot>-<id>.log`). A seat
with no log -- the human -- is SIMULATED: a bot log's action history is replayed
from that seat, with that seat's own draws hidden and everyone else's filled in
from the other logs. It is what a bot sitting there would compute, marked `*`.
Views that disagree are flagged `≠` -- a pair or the common view is by
definition one thing, so any disagreement is a bug in somebody's bookkeeping.

Needs build/replay_log (`--stacks`, which also cuts a later STATE back to any
turn a seat did not act on).
"""
from __future__ import annotations

import argparse
import glob
import json
import os
import pathlib
import subprocess
import sys
import tempfile
from typing import Any

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.dont_write_bytecode = True  # importing show_turn must not leave a __pycache__
sys.path.insert(0, str(ROOT / "scripts"))
from show_turn import suit_abbrs_for  # noqa: E402

REPLAY_LOG = ROOT / "build" / ("replay_log.exe" if os.name == "nt" else "replay_log")
ROLES = ["Alice", "Bob", "Cathy", "Donald", "Emily", "Frank"]


def load(path: str) -> list[dict[str, Any]]:
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


def states(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    return [r for r in records if r.get("ch") == "STATE"]


def run_stacks(log_path: str, turn: int) -> dict[str, Any]:
    out = subprocess.run([str(REPLAY_LOG), log_path, "--turn", str(turn), "--stacks"],
                         capture_output=True, text=True, encoding="utf-8")
    if out.returncode != 0:
        raise RuntimeError(f"replay_log failed on {log_path}: {out.stderr.strip()}")
    return json.loads(out.stdout.strip().splitlines()[-1])


def cut_actions(actions: list[dict[str, Any]], turn: int) -> list[dict[str, Any]]:
    """The history up to the `turn` marker that opens `turn` (as replay_log does)."""
    kept = []
    for a in actions:
        if turn <= 1 and a.get("t") not in ("draw", "status"):
            return kept
        kept.append(a)
        if a.get("t") == "turn" and a.get("num") == turn - 1:
            return kept
    raise RuntimeError(f"turn {turn} is past the end of the logged history")


def simulate(base: dict[str, Any], seat: int | None, ids: dict[int, tuple[int, int]],
             turn: int, tmpdir: str) -> dict[str, Any]:
    """Replay `base` from `seat`'s chair. `seat=None` is the fully sighted table."""
    rec = json.loads(json.dumps(base))
    replay = rec["replay"]
    actions = cut_actions(replay["actions"], turn)
    for a in actions:
        if a.get("t") != "draw":
            continue
        if seat is not None and a["p"] == seat:
            a["suit"], a["rank"] = -1, -1
        elif a.get("suit", -1) == -1 and a["order"] in ids:
            a["suit"], a["rank"] = ids[a["order"]]
    replay["actions"] = actions
    replay["our_player_index"] = seat if seat is not None else 0
    replay.pop("zcs_turn", None)
    rec.pop("debug", None)
    rec["turn"] = turn
    path = os.path.join(tmpdir, f"seat{seat}.log")
    with open(path, "w", encoding="utf-8") as f:
        f.write(json.dumps(rec) + "\n")
    return run_stacks(path, turn)


def short(stacks: list[int]) -> str:
    return "".join(str(x) for x in stacks) if stacks else "-"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("game_id")
    ap.add_argument("turn", type=int)
    ap.add_argument("--logs", default=str(ROOT / "logs"))
    args = ap.parse_args()

    paths = sorted(glob.glob(os.path.join(args.logs, f"*-{args.game_id}.log")))
    paths = [p for p in paths if states(load(p))]
    if not paths:
        print(f"no per-game logs with STATE records for {args.game_id}", file=sys.stderr)
        return 2

    by_seat: dict[int, str] = {}
    later: list[dict[str, Any]] = []
    ids: dict[int, tuple[int, int]] = {}
    for p in paths:
        st = states(load(p))
        by_seat[st[0]["replay"]["our_player_index"]] = p
        after = [s for s in st if s["turn"] >= args.turn]
        if after:
            later.append(after[0])
        for a in st[-1]["replay"]["actions"]:
            if a.get("t") == "draw" and a.get("suit", -1) != -1:
                ids[a["order"]] = (a["suit"], a["rank"])
    if not later:
        print(f"turn {args.turn} is past every log's last STATE", file=sys.stderr)
        return 2
    base = max(later, key=lambda r: len(r["replay"]["actions"]))
    names = base["replay"]["names"]
    n = len(names)
    variant = base["replay"].get("variant")

    views: dict[int, dict[str, Any]] = {}
    simulated: set[int] = set()
    with tempfile.TemporaryDirectory() as tmp:
        for seat in range(n):
            try:
                if seat not in by_seat:
                    raise RuntimeError("no log")
                views[seat] = run_stacks(by_seat[seat], args.turn)
            except RuntimeError:
                # No log, or a log that stops before this turn: simulate the seat.
                views[seat] = simulate(base, seat, ids, args.turn, tmp)
                simulated.add(seat)
        truth = simulate(base, None, ids, args.turn, tmp)

    alice = views[next(iter(views))]["current_player_index"]
    order = [(alice + k) % n for k in range(n)]
    role = {seat: ROLES[k] for k, seat in enumerate(order)}

    def who(seat: int) -> str:
        return names[seat] + ("*" if seat in simulated else "")

    def line(label: str, readings: list[tuple[int, list[int]]]) -> str:
        vals = [short(v) for _, v in readings]
        flag = "  ≠" if len(set(vals)) > 1 else ""
        body = "  ".join(f"{v} [{who(s)}]" for (s, _), v in zip(readings, vals))
        return f"  {label:<22}{body}{flag}"

    suits = " ".join(suit_abbrs_for(variant)[: len(truth["play_stacks"])])
    print(f"game {args.game_id} turn {args.turn} -- "
          + "  ".join(f"{role[s]}={who(s)}" for s in order)
          + f"   (suits {suits}; * = simulated, no log)")
    for s in order:
        print(f"  {role[s] + ' private':<22}{short(views[s]['play_stacks'])}")
    for i, a in enumerate(order):
        for b in order[i + 1:]:
            print(line(f"pair({role[a]},{role[b]})",
                       [(a, views[a]["pairwise_play_stacks"][b]),
                        (b, views[b]["pairwise_play_stacks"][a])]))
    print(line("common", [(s, views[s]["common_play_stacks"]) for s in order]))
    print(f"  {'true':<22}{short(truth['play_stacks'])}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
