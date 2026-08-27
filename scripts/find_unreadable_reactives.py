#!/usr/bin/env python3
"""Find turns where the bot was the REACTER of a reactive clue it could not read.

An unread reactive is reactor0's quietest failure. `interpret_reactive` pushes
the waiting connection before any phase runs, so a MISTAKE leaves one behind
with `react_order == -1` and no urgent stamp on any card -- and `take_action`'s
urgent scan keys on the stamp, not on `waiting`. The turn falls through to the
ordinary ladder and nothing is said. Meanwhile the RECEIVER's POV returns
REACTIVE unconditionally, so he is still waiting for a reaction and will decode
whatever the reacter does next as one. Replay 1974342 T13 is what that costs:
the reacter chucked its chop (a Blue 5) and the receiver read that as the
reaction, stamping CALLED_TO_PLAY on a Red 4 with red on 0.

`react_order == -1` on a WC whose `reacter` is our seat is the durable trace of
that, and it is what this script keys on -- so it works on logs written before
the `reactor0.reactive_unreadable` DECIDE branch existed (v10.9.0).

Usage:
    scripts/find_unreadable_reactives.py [logs/*.log]
    scripts/find_unreadable_reactives.py --by-variant
    scripts/find_unreadable_reactives.py --rerun build/replay_log.exe

With --rerun, each hit is replayed through the given binary so you can see what
the current build does with it. That does NOT tell you whether the clue now
READS -- only what action comes out; the interpretation itself is pinned by
tests/test_reactor0/test_bad_reactive_target/.
"""
import argparse
import collections
import glob
import json
import os
import re
import subprocess
import sys


def hits(paths):
    """Yield one record per (log, turn) where we are the reacter and the
    reactive was never read."""
    for path in paths:
        try:
            fh = open(path, encoding="utf-8", errors="replace")
        except OSError as exc:
            print("skipping %s: %s" % (path, exc), file=sys.stderr)
            continue
        with fh:
            for line in fh:
                # Cheap pre-filter: these logs are large and mostly not STATE.
                if '"react_order":-1' not in line:
                    continue
                try:
                    rec = json.loads(line)
                except ValueError:
                    continue
                if rec.get("ch") != "STATE":
                    continue
                replay = rec.get("replay", {})
                us = replay.get("our_player_index")
                for wc in rec.get("debug", {}).get("waiting", []):
                    if wc.get("react_order") != -1 or wc.get("reacter") != us:
                        continue
                    yield {
                        "log": path,
                        "turn": rec.get("turn"),
                        "database_id": rec.get("database_id"),
                        "convention": replay.get("convention", "reactor"),
                        "variant": replay.get("variant"),
                        "clue": "%s%s" % (wc.get("clue_kind"),
                                          wc.get("clue_value")),
                        "giver": wc.get("giver"),
                        "receiver": wc.get("receiver"),
                        "anchor": wc.get("focus_slot"),
                    }


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("logs", nargs="*", default=None,
                    help="log files (default: logs/*.log)")
    ap.add_argument("--convention", choices=("reactor", "reactor0"),
                    help="only this convention")
    ap.add_argument("--by-variant", action="store_true",
                    help="summarise counts per variant instead of listing")
    ap.add_argument("--rerun", metavar="BINARY",
                    help="replay each hit through this replay_log binary")
    args = ap.parse_args()

    paths = args.logs or sorted(glob.glob("logs/*.log"))
    if not paths:
        print("no logs found (run from the repo root?)", file=sys.stderr)
        return 2

    found = sorted({(h["log"], h["turn"]): h for h in hits(paths)}.values(),
                   key=lambda h: (h["log"], h["turn"] or 0))
    if args.convention:
        found = [h for h in found if h["convention"] == args.convention]

    if not found:
        print("no unreadable reactives in %d log(s)" % len(paths))
        return 0

    if args.by_variant:
        per_variant = collections.Counter(h["variant"] for h in found)
        per_conv = collections.Counter(h["convention"] for h in found)
        print("%d turn(s) across %d log(s), %d log file(s) scanned"
              % (len(found), len({h["log"] for h in found}), len(paths)))
        print("\nby convention:")
        for name, n in per_conv.most_common():
            print("  %4d  %s" % (n, name))
        print("\nby variant:")
        for name, n in per_variant.most_common():
            print("  %4d  %s" % (n, name))
        return 0

    for h in found:
        line = ("%-34s T%-4s %-9s %-4s anchor=%-2s giver=%s receiver=%s  %s"
                % (os.path.basename(h["log"]), h["turn"], h["convention"],
                   h["clue"], h["anchor"], h["giver"], h["receiver"],
                   h["variant"]))
        if args.rerun:
            proc = subprocess.run(
                [args.rerun, h["log"], "--turn", str(h["turn"]), "--rerun"],
                capture_output=True, text=True)
            match = re.search(r"rerun chose: (.*)", proc.stdout)
            line += "\n    -> %s" % (match.group(1).strip() if match
                                     else "replay_log failed")
        print(line)

    print("\n%d turn(s) across %d log(s)"
          % (len(found), len({h["log"] for h in found})))
    return 0


if __name__ == "__main__":
    sys.exit(main())
