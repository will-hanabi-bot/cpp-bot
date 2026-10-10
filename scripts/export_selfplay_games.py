#!/usr/bin/env python3
"""Export self-play games as hanab.live replay JSONs, for a human to review.

Usage:
  scripts/export_selfplay_games.py <out_dir> <arm>=<games.jsonl>,<log_dir> ... [--score N]

Each arm is one self-play run: its `games.jsonl` (`self_play --out`) and the
directory its per-seat logs went to (`self_play --log-dir`). The games exported are
those that scored exactly `--score` (default 29, the near misses) with no 5 in the
discard pile -- a missed play goes to the pile too -- since a game that lost a 5 is
not a near miss worth reading.

Files are named `<variant>_<major>_<minor>_<game id>.json`, e.g.
`darknull_23_22_9000002.json` (the user's rule, from v23.25.0): self-play game ids
are `9000000 + seed` in every build, so without the version two runs' games
collide. The variant is the part of the name after "&", lowercased, without the
suit count ("Throw It in a Hole & Dark Null (6 Suits)" -> `darknull`; plain
"Throw It in a Hole (6 Suits)" -> `tiiah`). The version is the ARM's name when it
reads `vX.Y` (`v23.24=...`), else the `bot_version` of the logs' `game_init` record.
The arm name wins because an A/B candidate is usually run before its version bump,
so its logs still carry the base's number. A game identical in every arm gets one
file, named after the FIRST arm's version; otherwise each arm gets its own.
`index.tsv` lists every seed looked at and why it was or was not exported.
"""
from __future__ import annotations

import argparse
import json
import os
import re

SUITS = "rygbpu"
SEATS = ["alice", "bob", "cathy"]


def records(path):
    with open(path, encoding="utf-8") as f:
        for line in f:
            if '"inbound_action"' in line or '"game_init"' in line:
                yield json.loads(line)


def game_init(log_dir, game_id):
    for r in records(f"{log_dir}/sim-alice-{game_id}.log"):
        if r.get("event") == "game_init":
            return r
    raise SystemExit(f"{log_dir}/sim-alice-{game_id}.log: no game_init record")


def build_game(log_dir, game_id):
    deck = {}
    for seat in SEATS:
        for r in records(f"{log_dir}/sim-{seat}-{game_id}.log"):
            a = r.get("action", {})
            if a.get("t") == "draw" and a.get("rank", -1) >= 0:
                deck[a["order"]] = (a["suit"], a["rank"])
    actions = []
    for r in records(f"{log_dir}/sim-alice-{game_id}.log"):
        a = r.get("action", {})
        t = a.get("t")
        if t == "clue":
            kind = 2 if a["kind"] == "C" else 3
            actions.append({"type": kind, "target": a["target"], "value": a["value"]})
        elif t == "play" or (t == "discard" and a.get("failed")):
            actions.append({"type": 0, "target": a["order"], "value": 0})
        elif t == "discard":
            actions.append({"type": 1, "target": a["order"], "value": 0})
    return deck, actions


def simulate(deck, actions, num_suits):
    stacks = [0] * num_suits
    trash = []
    for a in actions:
        if a["type"] == 0:
            s, r = deck[a["target"]]
            if r == stacks[s] + 1:
                stacks[s] = r
            else:
                trash.append((s, r))
        elif a["type"] == 1:
            trash.append(deck[a["target"]])
    return sum(stacks), trash


def variant_slug(variant):
    name = re.sub(r"\(\d+ Suits?\)", "", variant)
    part = name.split("&", 1)[1] if "&" in name else "tiiah"
    return re.sub(r"[^a-z0-9]", "", part.lower())


def version_tag(arm, bot_version):
    for text in (arm, bot_version):
        m = re.fullmatch(r"v?(\d+)\.(\d+)(?:\.\d+)?", text)
        if m:
            return f"{m.group(1)}_{m.group(2)}"
    raise SystemExit(f"cannot read a version from arm {arm!r} or {bot_version!r}")


def card(s, r):
    return (SUITS[s] if s < len(SUITS) else str(s)) + str(r)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out_dir")
    ap.add_argument("arms", nargs="+", help="<arm>=<games.jsonl>,<log_dir>")
    ap.add_argument("--score", type=int, default=29)
    args = ap.parse_args()

    arms = []
    for spec in args.arms:
        name, rest = spec.split("=", 1)
        games, log_dir = rest.split(",", 1)
        rows = {}
        with open(games, encoding="utf-8") as f:
            for line in f:
                g = json.loads(line)
                rows[g["seed"]] = g
        arms.append((name, rows, log_dir))

    os.makedirs(args.out_dir, exist_ok=True)
    seeds = sorted({s for _, rows, _ in arms for s, g in rows.items()
                    if g["score"] == args.score})
    index = []
    for seed in seeds:
        game_id = 9000000 + seed
        kept = []  # (arm, actions, export, summary, file stem)
        for name, rows, log_dir in arms:
            g = rows.get(seed)
            if not g or g["score"] != args.score:
                continue
            init = game_init(log_dir, game_id)
            variant = init["variant"]
            deck, actions = build_game(log_dir, game_id)
            n = len(deck)
            assert sorted(deck) == list(range(n)), f"seed {seed} {name}: deck has gaps"
            num_suits = 1 + max(s for s, _ in deck.values())
            score, trash = simulate(deck, actions, num_suits)
            assert score == g["score"], f"seed {seed} {name}: simulated {score} != {g['score']}"
            fives = [card(s, r) for s, r in trash if r == 5]
            if fives:
                index.append(f"{seed}\t{name}\tskipped: a 5 was discarded {','.join(fives)}")
                continue
            export = {
                "id": game_id,
                "players": [f"sim-{seat}" for seat in SEATS],
                "deck": [{"suitIndex": deck[o][0], "rank": deck[o][1]} for o in range(n)],
                "actions": actions,
                "options": {"variant": variant},
                "notes": [[] for _ in SEATS],
            }
            summary = (f"score {g['score']}, reachable {g.get('reachable')}, "
                       f"strikes {g.get('strikes')}, turns {g.get('turns')}, trash "
                       + " ".join(card(s, r) for s, r in trash))
            stem = f"{variant_slug(variant)}_{version_tag(name, init['bot_version'])}_{game_id}"
            kept.append((name, actions, export, summary, stem))
        if not kept:
            continue
        same = len(kept) == len(arms) and all(k[1] == kept[0][1] for k in kept)
        for name, _, export, summary, stem in (kept[:1] if same else kept):
            path = os.path.join(args.out_dir, stem + ".json")
            with open(path, "w", encoding="utf-8") as f:
                json.dump(export, f, indent=2)
            arm = "+".join(a[0] for a in arms) if same else name
            index.append(f"{seed}\t{arm}\t{os.path.basename(path)}\t{summary}")
    with open(os.path.join(args.out_dir, "index.tsv"), "w", encoding="utf-8") as f:
        f.write("seed\tarm\tfile\tdetail\n")
        f.write("\n".join(index) + "\n")
    print("\n".join(index))


if __name__ == "__main__":
    main()
