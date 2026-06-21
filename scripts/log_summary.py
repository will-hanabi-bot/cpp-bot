#!/usr/bin/env python3
"""Print a one-line summary per turn + final TIMING aggregate for a game log.

Usage:
  scripts/log_summary.py <log_file>

Output:
  game_id=<n>  bot=<name>  variant=<name>  players=<n>
  turn  player  action                 elapsed_ms
  ...
  per-game scopes: <scope> total_ms calls
"""
from __future__ import annotations

import argparse
import json
import sys


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    args = ap.parse_args()

    with open(args.log) as f:
        records = [json.loads(line) for line in f if line.strip()]

    init = next((r for r in records if r.get("event") == "game_init"), {})
    game_id = init.get("game_id")
    bot = init.get("bot")
    variant = init.get("variant")
    print(f"game_id={game_id}  bot={bot}  variant={variant}  "
          f"players={init.get('num_players')}  our_pi={init.get('our_player_index')}")
    print(f"  bot_version={init.get('bot_version')}  all_plays={init.get('all_plays')}")
    print()

    # Per-turn: STATE → outbound. We only see our-turn pairs since we only
    # log STATE before our take_action.
    print(f"  {'turn':>4}  {'action':40s}  {'elapsed_ms':>11s}")
    state_for_turn: dict[int, dict] = {}
    for r in records:
        if r.get("ch") == "STATE":
            state_for_turn[r["turn"]] = r
        elif r.get("ch") == "LIFECYCLE" and r.get("event") == "outbound_action":
            t = r.get("turn")
            action = r.get("action", {})
            action_str = json.dumps(
                {k: v for k, v in action.items() if k != "tableID"},
                separators=(",", ":"))
            print(f"  {t:>4}  {action_str:40s}  {r.get('elapsed_ms', 0):>11.2f}")
    print()

    # Per-game TIMING aggregate (one record near end).
    per_game = next(
        (r for r in records
          if r.get("ch") == "TIMING" and r.get("scope") == "per_game"),
        None)
    if per_game:
        print("per-game timing (ms):")
        scopes = per_game.get("scopes", {})
        rows = sorted(scopes.items(),
                       key=lambda kv: -int(kv[1].get("total_ns", 0)))
        print(f"  {'scope':40s}  {'calls':>8s}  {'total_ms':>10s}  {'max_ms':>10s}")
        for name, stats in rows:
            total_ms = int(stats.get("total_ns", 0)) / 1e6
            max_ms = int(stats.get("max_ns", 0)) / 1e6
            print(f"  {name:40s}  {stats.get('calls', 0):>8d}  "
                  f"{total_ms:>10.2f}  {max_ms:>10.2f}")

    # Game-over record.
    over = next((r for r in records if r.get("event") == "game_over"), None)
    if over:
        print()
        print(f"game_over: end_condition={over.get('end_condition')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
