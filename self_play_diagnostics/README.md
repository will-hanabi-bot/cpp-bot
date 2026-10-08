# Self-play diagnostics

A local harness that has three copies of the current bot play **Throw It in a Hole
& Black (6 Suits)** against each other, and a set of detectors that report where the
bots' readings go wrong. It is used to check that a change does not damage play
elsewhere: run the previous version and the candidate on the same seeds and compare.
What good play *is* comes from the human diagnostics in
`v18_human_vs_bot_diagnostics/`, not from these numbers.

## Running it

```bash
cmake --build build -j --target self_play
build/self_play.exe --seeds 1..100 --jobs 12 \
    --out self_play_diagnostics/runs/<version> \
    --report self_play_diagnostics/results/<version>.md
```

| Flag | Default | Meaning |
|---|---|---|
| `--seeds A..B` | `1..100` | one game per seed; the seed fixes the deck |
| `--jobs N` | half the cores | games run in parallel, one per thread |
| `--out DIR` | none | `games.jsonl` (one line per game) and `issues.jsonl` (one line per issue) |
| `--report FILE` | none | the Markdown summary; it is also printed to stdout |
| `--log-dir DIR` | `logs` | per-seat game logs; `''` turns them off |
| `--variant NAME` | `Throw It in a Hole & Black (6 Suits)` | any hanab.live variant name |
| `--players N` | `3` | seat count |

`results/` is committed, with one file per version's 100-game run on seeds 1–100.
`runs/` holds the raw JSONL and is ignored by git.

A game takes 0.3–7 s. A 100-game run takes under a minute at 12 jobs.

A seed replays the same game, except where the endgame solver hits its wall-clock
deadline. The harness gives it the live bot's 6 s (`--endgame-timeout`). Under a
parallel run a solve that finishes near the deadline can go either way, so two runs
of one build can differ in a few games. Compare runs in aggregate, and use
`replay_log --rerun` on a single log to check one decision.

If a game crashes, the harness prints the seed it was playing and a stack of image
offsets before it exits. Pass them to `addr2line -f -C -e build/self_play.exe` after
adding the image base, which is `0x140000000`. A `-D_GLIBCXX_ASSERTIONS` build, in a
separate build directory, turns an out-of-bounds access into an immediate abort at
the faulting line. That is how v18.0.0 found a use-after-free in
`stacks_after_queued_plays`, which had crashed about one multi-threaded run in four.

## What the simulator is

`sim.{h,cpp}` stands in for the hanab.live server. It keeps the true deck, hands,
stacks, clue tokens, strikes and score. It builds one `Game` per seat exactly as
`BotClient::on_init` does: the convention comes from `resolve_table_convention`,
the reactive-lock default is the variant's, and the endgame timeout is the live 6 s. It feeds each seat the server's `action` JSON through the live client's own
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
| **3** | `dispatch_disagreement` | the seats read a clue as different kinds, where it touches reactive target selection: some seat reads `Reactive` and another does not, or the giver and the clue's target disagree |
| 3-outside | `dispatch_disagreement` | only the seat outside a stable clue's pair reads it differently (informational) |
| **3** | `pairing_mismatch` | the giver's and the reacter's waiting connections name different reacter or receiver cards |
| **3** | `receiver_stamp_mismatch` | once the reacter acted, the receiver's newly called card is not the one the giver targeted |
| **4** | `private_below`, `pair_below`, `common_below` | a stack view is below what every party to it saw land (see below) |
| 4-above | `*_above` | a stack view is above the true stack; informational, but a strike risk |
| 5 | `strike` | each strike, with the striker's reading, who called the card, which teammates saw the card was dead, the striker's belief and shared rank for the suit against the true one, and whether the deck had run out |
| div | `common`, `pair`, `call` | informational, the first time in a game that the seats disagree about the common view, one pair view between its two members, or which cards are called and what the common reading of a called card is (the receiver of a pending reactive is exempt for the reacter's hand) |
| onset | `common`, `pair` | informational, every action after which a view that agreed across seats no longer does, with the action that did it (and for a play or miss, every seat's common reading of the card beforehand, whether it was the reacter's urgent call, and every seat's shared view afterwards); `analysis` groups these to find where divergence starts |
| stat | `divergence` | one per game: how many actions left the common views, or some pair view, in disagreement |
| stat | `called_play` | every play of a card its holder held `CALLED_TO_PLAY`: whether it landed, whether it was the reacter's urgent call, whether the holder's reading held the truth, and how many cards stood superposed (all, and the holder's own) |

Classes **1–4** are critical, and they are what the stop criterion counts.

**Where wrong inferences come from.** Every class-1 issue carries an `origin`: the
action just processed (`clue`, `play`, `miss` or `discard`), its actor and, for a
clue, its target. It also records whether the clue was stable, and whether the
three seats read it as different kinds. It records the wrong seat's part too:
`giver`, `target` or `outside` for a clue, and `player` or `watcher` for a play or
a discard.

The report counts **cards ever read wrongly**, which is each card with a class-1
issue at any seat, per 100 games. It tabulates each card's first wrong inference,
and every class-1 event, by origin. That table is where to look for which rule to
change.

Every call is tracked for class 2, whatever made it: a stable clue, the receiver's
or the reacter's side of a reactive, a fix or a refusal re-call.

**Stack floors (class 4).**
- **Private and pair views.** A party *saw* a landed sN if they were not the
  player, or they were the player and could name the card. For a pair view that
  means the player's own reading named it at play time, or a seat has since
  settled it for the team (`named_in_hole`). The player's PRIVATE settle does not
  count toward a pair view, because the other party cannot know of it, but it does
  count toward the player's own private view. For a pair view, the other member
  must also be able to attribute the player's knowledge: its own common reading
  named the card too. A pair view is one object that both members compute, so it
  cannot hold what one of them has no way to know. The case is a reacter's blind
  play that only the reacter and the giver can name. floor(P, suit) is the highest
  landed rank every member of P saw.
- **Named for the team** means `named_in_hole` holds the identity at EVERY seat; a name one seat wrote alone is that seat's.
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

## How it is used

To compare a candidate with the previous version, freeze the previous
`build/self_play.exe` under another name before rebuilding. Then run both on the
same seeds (seeds 1–200 at the live 6 s endgame is the usual check since v22.10.0;
1–300 before) and compare
max-score games, the cards ever read wrongly, and strikeouts. A version's own
100-game run on seeds 1–100 is saved as `results/<version>.md`. Benchmarks use the
6-suit variants: they are harder and catch longer-term accumulated desyncs. The
default is TIIAH & Black (6 Suits) since v22.9.0 (the user's call, for the hard TIIAH
variants); from v20.24.0 to v22.8.0 it was TIIAH (6 Suits), and reports up to v20.23.0
are 5 Suits.

## Tests

`tests/test_selfplay/` is part of `hanabi_tests`:
- `test_sim_wire.cpp` checks what each seat is shown in TIIAH and in No Variant,
  the deal, and the deck built from the variant's counts. It works at the message
  level only; no non-TIIAH game is simulated.
- `test_diagnostics.cpp` injects a wrong inference into one seat of a three-turn
  game and checks it is reported.
