#!/usr/bin/env python3
"""Pretty-print a single turn from a per-game JSONL log.

Usage:
  scripts/show_turn.py <log_file> <turn> [--filter PATTERN]

Renders:
  * The LIFECYCLE inbound actions leading into the turn
  * The STATE snapshot (play stacks, hands w/ empathy decoded, meta, waiting)
  * The DECIDE trace (indented enter/exit/branch records)
  * The TIMING delta for the turn
  * The LIFECYCLE outbound action chosen

Doesn't invoke any C++ binary — pure log inspection. For re-running with
current code, use bin/replay_log --rerun.
"""
from __future__ import annotations

import argparse
import itertools
import json
import re
import sys
from typing import Any

SUIT_ABBR = ["r", "y", "g", "b", "p", "i"]  # red, yellow, green, blue, purple, ???
# Variant-specific suit letters are in data/suits.json; for snapshot display
# we just lean on the suit_index numeric form when out of range.


def decode_empathy(bits: int) -> str:
    """Decode an IdentitySet bitmask into a compact human-readable string.

    Identity ordinal = suit_index * 5 + (rank - 1), max 30. We render the
    set as e.g. "r1,r2,y3" using the SUIT_ABBR letters above; out-of-range
    suit indices print as "s<idx>r<rank>".
    """
    if bits == 0:
        return "∅"
    ids = []
    for k in range(64):
        if bits & (1 << k):
            suit = k // 5
            rank = (k % 5) + 1
            tag = SUIT_ABBR[suit] if suit < len(SUIT_ABBR) else f"s{suit}"
            ids.append(f"{tag}{rank}")
    return ",".join(ids)


def load_records(path: str) -> list[dict[str, Any]]:
    with open(path) as f:
        return [json.loads(line) for line in f if line.strip()]


def find_turn_state(records: list[dict[str, Any]], turn: int) -> dict[str, Any] | None:
    for r in records:
        if r.get("ch") == "STATE" and r.get("turn") == turn:
            return r
    return None


def render_state(rec: dict[str, Any]) -> None:
    dbg = rec.get("debug", {})
    replay = rec.get("replay", {})
    print(f"--- STATE turn={rec.get('turn')} game_id={rec.get('game_id')} bot={rec.get('bot')} ---")
    print(f"  variant: {replay.get('variant')}")
    print(f"  current_player: {dbg.get('current_player_index')} / {replay.get('num_players')}")
    print(f"  stacks: {dbg.get('play_stacks')}  max_ranks: {dbg.get('max_ranks')}")
    print(f"  clues: {dbg.get('clue_tokens')}  strikes: {dbg.get('strikes')}  "
          f"cards_left: {dbg.get('cards_left')}  endgame_turns: {dbg.get('endgame_turns')}")
    discards = dbg.get("discards", [])
    if discards:
        compact = ",".join(
            f"s{d['suit']}r{d['rank']}#{d['order']}" for d in discards)
        print(f"  discards: {compact}")

    print("  hands:")
    for hand in dbg.get("hands", []):
        name = hand.get("name", f"P{hand.get('player')}")
        print(f"    {name} (player {hand['player']}):")
        for card in hand.get("cards", []):
            slot = card.get("slot")
            order = card.get("order")
            clued = "C" if card.get("clued") else "-"
            focused = "F" if card.get("focused") else "-"
            urgent = "U" if card.get("urgent") else "-"
            status = card.get("status", "NONE")
            empathy = decode_empathy(int(card.get("inferred", 0)))
            possible = decode_empathy(int(card.get("possible", 0)))
            id_ = card.get("id")
            id_str = f"id={id_}" if id_ else "id=?"
            print(f"      slot {slot}: order={order:4d} {clued}{focused}{urgent} {status:20s} "
                  f"{id_str:10s} inf={empathy}  poss={possible}")

    waiting = dbg.get("waiting", [])
    if waiting:
        print("  waiting connections:")
        for wc in waiting:
            print(f"    giver={wc['giver']} reacter={wc['reacter']} receiver={wc['receiver']} "
                  f"focus_slot={wc['focus_slot']} inverted={wc.get('inverted')} "
                  f"all_plays={wc.get('all_plays')} clue={wc.get('clue_kind')}{wc.get('clue_value')}")

    moves = dbg.get("move_history", [])
    if moves:
        tail = moves[-5:]
        print(f"  move_history (last {len(tail)}): " + ", ".join(
            f"{m.get('k')}:{m.get('v')}" for m in tail))


def render_decide_trace(records: list[dict[str, Any]], turn: int,
                          filter_pat: re.Pattern | None) -> None:
    """Print DECIDE records that belong to the turn — defined as records
    between the STATE turn=N and the LIFECYCLE outbound_action that follows."""
    started = False
    print("--- DECIDE TRACE ---")
    for r in records:
        if r.get("ch") == "STATE" and r.get("turn") == turn:
            started = True
            continue
        if not started:
            continue
        if r.get("ch") == "LIFECYCLE" and r.get("event") == "outbound_action":
            break
        if r.get("ch") != "DECIDE":
            continue
        name = r.get("name", "?")
        if filter_pat and not filter_pat.search(name):
            continue
        depth = int(r.get("depth", 0))
        kind = r.get("kind", "?")
        indent = "  " * depth
        extras = {
            k: v
            for k, v in r.items()
            if k not in ("ch", "kind", "depth", "name", "ts", "game_id", "bot")
        }
        extras_str = (
            " " + json.dumps(extras, separators=(",", ":"))
            if extras
            else ""
        )
        marker = {"enter": "→", "exit": "←", "branch": "•", "decision": "★"}.get(kind, "?")
        print(f"  {indent}{marker} {name}{extras_str}")


def render_outbound(records: list[dict[str, Any]], turn: int) -> None:
    seen_state = False
    for r in records:
        if r.get("ch") == "STATE" and r.get("turn") == turn:
            seen_state = True
            continue
        if not seen_state:
            continue
        if r.get("ch") == "LIFECYCLE" and r.get("event") == "outbound_action":
            print("--- OUTBOUND ---")
            print(f"  {r.get('action')}")
            print(f"  elapsed_ms: {r.get('elapsed_ms')}")
            return
        if r.get("ch") == "STATE":
            break  # next turn's state means no outbound was emitted


def render_timing(records: list[dict[str, Any]], turn: int) -> None:
    for r in records:
        if r.get("ch") == "TIMING" and r.get("scope") == "per_turn" and r.get("turn") == turn:
            print("--- TIMING ---")
            scopes = r.get("scopes", {})
            for name, stats in sorted(scopes.items(), key=lambda kv: -int(kv[1].get("total_ns", 0))):
                total_ms = int(stats.get("total_ns", 0)) / 1e6
                calls = stats.get("calls", 0)
                print(f"  {name:40s}  calls={calls:6d}  total_ms={total_ms:8.2f}")
            return


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("turn", type=int)
    ap.add_argument("--filter", default=None,
                     help="Regex applied to DECIDE record names")
    args = ap.parse_args()

    records = load_records(args.log)
    state = find_turn_state(records, args.turn)
    if not state:
        print(f"no STATE record for turn {args.turn} in {args.log}",
                file=sys.stderr)
        return 2

    filter_pat = re.compile(args.filter) if args.filter else None

    render_state(state)
    render_decide_trace(records, args.turn, filter_pat)
    render_timing(records, args.turn)
    render_outbound(records, args.turn)
    return 0


if __name__ == "__main__":
    sys.exit(main())
