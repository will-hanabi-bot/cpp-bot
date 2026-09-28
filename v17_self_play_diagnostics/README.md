# v17 self-play diagnostics

A local harness that has three copies of the current bot play **Throw It in a Hole
(5 Suits)** against each other, and a set of detectors that report where the
bots' readings go wrong. It is the v17 experiment: fix what it finds, commit, and
rerun, until the critical classes below are zero and at least 25 of 100 games
reach 25/25.

## Running it

```bash
cmake --build build -j --target self_play
build/self_play.exe --seeds 1..100 --jobs 12 \
    --out v17_self_play_diagnostics/runs/<version> \
    --report v17_self_play_diagnostics/results/<version>.md
```

| Flag | Default | Meaning |
|---|---|---|
| `--seeds A..B` | `1..100` | one game per seed; the seed fixes the deck |
| `--jobs N` | half the cores | games run in parallel, one per thread |
| `--out DIR` | none | `games.jsonl` (one line per game) and `issues.jsonl` (one line per issue) |
| `--report FILE` | none | the Markdown summary; it is also printed to stdout |
| `--log-dir DIR` | `logs` | per-seat game logs; `''` turns them off |
| `--variant NAME` | `Throw It in a Hole (5 Suits)` | any hanab.live variant name |
| `--players N` | `3` | seat count |

`results/` is committed, with one file per version's 100-game run on seeds 1–100.
`runs/` holds the raw JSONL and is ignored by git.

A game takes 0.3–7 s. A 100-game run takes under a minute at 12 jobs.

## What the simulator is

`sim.{h,cpp}` stands in for the hanab.live server. It keeps the true deck, hands,
stacks, clue tokens, strikes and score. It builds one `Game` per seat exactly as
`BotClient::on_init` does: the convention comes from `resolve_table_convention`,
the reactive-lock default is the variant's, and the endgame timeout is lowered to
1 s. It feeds each seat the server's `action` JSON through the live client's own
path: `action_from_json`, then `orient_action_for_engine`, then
`Game::handle_action`. On its turn a seat's `take_action` is asked for a move, and
the simulator resolves that move against the truth.

**Visibility is keyed on `Variant::throw_it_in_a_hole`.** The simulator is
variant-generic, but only TIIAH is run for now.

| Action | TIIAH | Any other variant |
|---|---|---|
| Clue | `clue` → `status` → `turn` | same |
| Landed play | `play` with `suitIndex`/`rank` −1 to **every** seat, the player included → `draw` → `status` → `turn` | `play` with the identity |
| Missed play | **identical to a landed play** | `strike` → failed `discard` with the identity |
| Discard | `discard` with the identity | same |

- In TIIAH, strikes and the score are **not public**. Only the simulator, as the
  external observer, knows that a play missed. It ends the game on the third
  strike. `status` carries the true clue count but a constant score of 0/25.
- In other variants, `status` carries the true score and max score.
- The drawer receives its own card as −1/−1. No draw is sent once the deck is
  empty. The last draw gives every seat one more turn. The game ends at max
  score, at the third strike, or after that final round.
- Hanab.live's wire in fact sends TIIAH bots a `strike` record and a failed
  `discard`. The simulator deliberately does not, because players are not
  entitled to that information.
- An illegal move is recorded as a `harness_error`, and the game is aborted. That
  covers a clue with no tokens, a clue touching nothing (outside the Blind
  variants), a card not in hand, and a discard at 8 clues. A thrown exception is
  recorded as a `crash`.

Each seat writes a per-game log in the live format as
`logs/sim-<seat>-<game_id>.log`, where the game id is `9000000 + seed`. So
`replay_log --rerun/--trace/--stacks`, `scripts/show_turn.py`,
`scripts/tiiah_stacks.py` and `scripts/bug_to_test.sh` all work on a simulated
game unchanged, and `tiiah_stacks.py` has all three seats' logs. A rerun
reproduces the logged action, and a seed always replays the same game.

## The detectors (`diagnostics.{h,cpp}`)

Every check runs after each action, once all three seats have processed it. An
issue is reported once, at its first occurrence per (class, kind, seat, card).
`turn` is 1-based, the numbering the STATE records use. The `replay` field in
`issues.jsonl` reconstructs the position the action produced.

| Class | Kind | What it means |
|---|---|---|
| **1** | `common_possible`, `common_inferred` | a seat's common reading of a card in some hand excludes its true identity |
| **1** | `own_possible`, `own_inferred` | a seat's own reading of its own card excludes its true identity |
| **1** | `superposition`, `shared_left`, `named_in_hole` | a hole card's candidate set, or its shared set, or its team-named identity, excludes the truth |
| **2** | `dropped` | the holder's own view cleared a `CALLED_TO_PLAY` while the card was still in hand and still needed |
| **2** | `not_at_holder` | another seat has a call on a card that its holder's view does not |
| **2** | `discarded_call` | the holder discarded a card it held called to play, and the card was still needed |
| **2** | `unactioned_at_end` | the game ended normally with a call standing that its holder had had a turn to play |
| 2w | `idle` | warning: a truly playable call stood through two of its holder's turns |
| 2-trash | `dropped`, `not_at_holder` | as above, but the card was truly trash by then |
| **3** | `dispatch_disagreement` | the seats read a clue as different kinds (e.g. `Reactive` against `Mistake`) |
| **3** | `pairing_mismatch` | the giver's and the reacter's waiting connections name different reacter or receiver cards |
| **3** | `receiver_stamp_mismatch` | once the reacter acted, the receiver's newly called card is not the one the giver targeted |
| **4** | `private_below`, `pair_below`, `common_below` | a stack view is below what every party to it saw land (see below) |
| 4-above | `*_above` | a stack view is above the true stack; informational, but a strike risk |
| 5 | `strike` | each strike, with the striker's reading and which teammates saw the card was dead |

Classes **1–4** are critical, and they are what the stop criterion counts.

Every call is tracked for class 2, whatever made it: a stable clue, the receiver's
or the reacter's side of a reactive, a fix or a refusal re-call.

**Stack floors (class 4).**
- **Private and pair views.** A party *saw* a landed sN if they were not the
  player, or they were the player and could name the card: their own reading
  named it at play time, or their view has since settled it. floor(P, suit) is
  the highest landed rank every member of P saw.
- **Common view: the best deductions all three can make.** A landed sN counts
  when the player's common reading named it at play time, or any seat has since
  settled it for the team (`ConvData::named_in_hole`).
- **Only plays that truly landed count.** Strikes are hidden, so a watcher cannot
  tell from the wire whether a card landed. It is still entitled to the floor,
  because the convention presumes a partner's play lands (tiiah/CONVENTION.md
  §1e, rule 6). A miss never raises a floor.
- Reversed suits are skipped.
- Outside TIIAH every stack is public, and the check is simply that each seat's
  stacks equal the truth.

## Stop criterion

A 100-game run on seeds 1–100 with **zero class 1–4 issues** and **at least 25
games at 25/25**. It is then confirmed on seeds 101–200, as a check against
fixing to the seeds.

## Tests

`tests/test_selfplay/` is part of `hanabi_tests`:
- `test_sim_wire.cpp` checks what each seat is shown in TIIAH and in No Variant,
  the deal, and the deck built from the variant's counts. It works at the message
  level only; no non-TIIAH game is simulated.
- `test_diagnostics.cpp` injects a wrong inference into one seat of a three-turn
  game and checks it is reported.
