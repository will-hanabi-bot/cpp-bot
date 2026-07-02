# Phase 7 — Verification (historical)

> **Status update (v1.0):** this document is a snapshot from the Phase 7
> port era and its "pending" items have since shipped. The reactor
> `interpret_clue` / `interpret_reactive` / `interpret_reaction` triple is
> fully implemented (`src/conventions/reactor/`), the live-bot game
> lifecycle runs in `src/net/commands.cpp`, and replay-based verification
> is handled by per-game JSONL logs + `build/replay_log` plus the
> `tests/test_endgame/test_replay_*.cpp` regression suite (which superseded
> the planned `cli/replay`). The sections below are kept for the
> microbenchmark data and methodology; read status claims as historical.

Status as of Phase 7: the foundation is solid (154 GoogleTests passing,
binary builds, primitives measured), but full end-to-end behavioral parity
against Python is blocked on the remaining Phase 4 work (the reactor
`interpret_clue` / `interpret_reactive` / `interpret_reaction` triple).

This document records what *was* verified at that point, what was pending,
and the measurement methodology.

## What's verified

### Unit-level (GoogleTest, ported 1-to-1 from pytest)

- 154 tests across `test_basics/`, `test_reactor/`, `test_endgame/`,
  `test_net/`, plus the `test_harness` smoke set.
- Coverage:
  - `IdentitySet` arithmetic / iteration / set ops (29 tests)
  - `Clue`, `Action`, `Variant` (regex predicates + per-suit clue-touch),
    `State` (with_play / with_discard / regain_clue / expand_short)
  - `Game` dispatcher + action handlers (clue / play / discard / draw,
    is_touched, is_blind_playing, ...)
  - `endgame::Fraction`, `RemainingMap` ops, `find_must_plays`,
    `unwinnable_state`, `trivially_winnable`
  - `endgame::EndgameSolver::solve` on the trivial-win path (score == max-1)
  - `reactor::reactive_value_table` for vanilla + special-suit variants
  - `net::codec` round-trip + decoder error paths
  - Test harness's `setup` / `take_turn` / `parse_action` / `fully_known`

### Microbenchmark — C++ vs Python primitives

Measured on Apple M-series with RelWithDebInfo + `-O2`:

| Op | Python | C++ | Speedup |
|---|---|---|---|
| `Fraction +` (with construction) | 630 ns | 12.0 ns | **53×** |
| `Fraction ×` (with construction) | 605 ns | 11.3 ns | **54×** |
| `Fraction <` (with construction) | 536 ns | 11.5 ns | **47×** |
| `IdentitySet &` (int bitset) | 28.7 ns | 0.75 ns | **38×** |

Source: `apps/bench_endgame.cpp` (C++) and inline `timeit` for Python
(see `scripts/` if added later).

The Plan's target of "30–80× end-to-end" is well-supported by these
primitive-level numbers — Fraction arithmetic alone is the dominant cost
in Python's solver and is now ~50× cheaper per op. With the snapshot-restore
optimization (deferred, see below), the end-to-end win should be at the
upper end of that range.

### End-to-end smoke

- `EndgameSmoke.SolverConstructsAndAttemptsTrivialWin` solves a
  score=24/max=25 position in **9 ms** (`./build/hanabi_bot` test run).
  The same position in Python solves in ~30 ms minimum due to per-op
  Fraction overhead, even on the easy path.
- `./build/hanabi_bot index=0` (the live-bot entry point) rejects missing
  `HANABI_USERNAME0` cleanly.

## What was pending at Phase 7 — gates to "30 s → < 1 s" claim (since shipped)

The verification target from the plan was:

> Pick the slowest endgame test replay (closest to 30 s in Python). Time
> in both languages. Pass criterion: C++ wall-clock ≤ 1 s, chosen
> `PerformAction` matches Python's.

This requires three things, none of which are complete:

1. **Phase 4 tail — reactor convention `interpret_*`** (~2.5 k LOC Python
   → ~3.5 k C++). The endgame solver invokes `Game.simulate_action`,
   which dispatches into `interpret_clue` / `interpret_reactive` /
   `interpret_reaction` / `update_turn`. With these stubbed to no-ops
   (current state), the solver produces wrong winrates on any scenario
   that depends on convention propagation (essentially all of them
   except the trivial-win path).

2. **Phase 6 tail — `cli/replay`** (189 LOC Python). Needed to load a
   recorded game from `python-bot/seeds/<i>.json` and seek to the
   endgame turn so the C++ solver can be timed on the same input as
   Python. Approximately the same complexity as the existing test
   harness.

3. **Snapshot-restore optimization** (the Plan's "Option B"). Currently
   the solver uses `Game::simulate_action`'s copy-on-call semantics,
   which is faithful but does a full `Game` copy per arrangement × per
   action × per draw. The Plan's recommendation was to snapshot just
   the mutated fields at the recursion-frame boundary and restore via
   memcpy. This is the *biggest* remaining perf win — primitive
   speedups give ~50× but the allocator pressure from per-action
   cloning dominates the Python solver's wall-clock. With snapshot-
   restore landed, the ratio should be ~80× rather than ~50×.

Each of these is its own focused work item; collectively they're another
~5 k LOC of careful translation + a perf-tuning pass.

## Methodology for the future parity harness

When the above gates are in place:

1. Build `cli/replay` so it accepts the same input shape as Python's
   `python -m hanabi_bot replay file=seeds/<i>.json index=<player>`.
2. Pick `python-bot/tests/test_endgame/test_replay_*.py` — the slowest
   replay in Python.
3. Add a C++ test `tests/test_endgame/test_replay_<id>.cpp` that loads
   the same seed, advances to the same turn, and asserts the chosen
   `PerformAction` matches Python's.
4. Run both with `time ./build/...` and assert:
   - C++ wall-clock ≤ 1 s
   - Same `PerformAction` chosen
   - Winrate within a small tolerance (Fractions are exact, so this
     should be exact equality)

## Per-phase status snapshot (as of v1.0)

| Phase | Verification status |
|---|---|
| 0 — CMake | ✅ build clean, tests green |
| 1 — basics/ leaves | ✅ ported pytest 1-to-1 |
| 2 — basics/ core | ✅ ported pytest 1-to-1 |
| 3 — Test harness | ✅ harness smoke green |
| 4 — reactor convention | ✅ interpret_* triple implemented; covered by test_reactor/ + replay regression suite |
| 5 — endgame solver | ✅ full solver live (6 s budget) + forced-endgame rules; test_endgame/ incl. per-game replay regressions |
| 6 — net + CLI | ✅ live bot in production on hanab.live; codec/command parsing unit-tested (WebSocket transport itself untested) |
| 7 — this document | ✅ historical; ongoing verification is per-game JSONL logs + replay_log reruns (see CLAUDE.md) |
