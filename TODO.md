# TODO — convention that is legal but not implemented

The per-convention CONVENTION.md files
([reactor](src/conventions/reactor/CONVENTION.md),
[reactor0](src/conventions/reactor0/CONVENTION.md)) describe the bot **as it
behaves**. This file is the other half: rules that are part of a convention
and legal to play, but that the current build does not produce or decode.
Each entry is tagged with the convention(s) it applies to. Anything here is a
known gap, not a disagreement about what the convention says.

Delete an entry when it ships, and update the owning convention's
`CONVENTION.md` / `GLOSSARY.md` wording in the same commit.

---

## 1. `[reactor]` Trash push should read as a referential play

**Convention.** A rank clue whose every touchable identity is basic trash is
interpreted **exactly like a referential play clue**, as if the trash cards had
been touched by a colour clue: it calls the card one slot to the **left** (newer)
of a newly-touched card to play.

**Reactor0 explicitly does NOT adopt this** — its trash reveal (stable rank
priority 3) is terminal by spec: the leftmost newly touched card is marked
trash and no play is called
(`src/conventions/reactor0/interpret_clue.cpp:245-256`).

**Today (reactor).** `try_stable`'s branch 2
(`src/conventions/reactor/interpret_clue.cpp:447-468`) only intersects the
focus's `inferred` with the trash set and sets `meta.trash`. No status is
stamped and **no play is called** — the clue conveys "this is garbage" and
nothing more.

**Touchpoints.**
- The `try_stable` branch order (`interpret_clue.cpp:424-618`): trash push is
  branch 2, `ref_play` is branch 8. Making branch 2 defer to `ref_play` is the
  shape of the fix.
- `variant->touch_possibilities(kind, value)` (`:450-458`) — the loop that keeps
  Pink-Fives from being misread as a trash push. Any rewrite must preserve it.
- `bool trash_push` (`:448`) is the local flag; the name stays, since the term is
  reactor convention.

---

## 2. `[reactor, reactor0]` Bluffs are legal but never played or read

**Convention.** A rare reactive move in which the reacter *believes* they are
playing a card that connects with a one-away-from-playable card in the receiver's
hand, but plays a different card. It is legal so long as the receiver can tell
**after** the reaction that their target is not actually playable — they then mark
it one-away-from-playable and chuck their chop as normal.

**Today.** Neither convention initiates or decodes one. The POV-invariant
abort in reactor's finesse phase
(`src/conventions/reactor/interpret_reactive.cpp:832-846`) — mirrored by
reactor0 (`src/conventions/reactor0/interpret_reactive.cpp:328-332`) —
returns `nullopt` whenever the observer can see that the reacter's card is
*not* the required `prev_id`, which is exactly the bluff case; there is
deliberately no "try the next slot" retry.

**Touchpoints.**
- `CardStatus::BLUFFED`, `MAYBE_BLUFFED`, `F_MAYBE_BLUFFED`
  (`include/hanabi/basics/card.h:30-32`) and `FinesseKind::BLUFF`
  (`include/hanabi/basics/connection.h:20-26`) already exist but are never set.
- Decoding requires the receiver to re-derive their target's playability *after*
  the reaction — i.e. work in the reaction-resolution layer, not just a relaxed
  abort.

---

## 3. `[reactor0]` N-player support

Reactor0 is specified and tested for 3 players only. Games with 4+ players
fall back to reactor at game init (`src/net/commands.cpp:292-299`). Extending
it means auditing the anchor arithmetic at hand sizes 4 and 3, the colour
value table's mod behaviour, and the reacter definition when the receiver is
not the seat after the reacter.

---

## 3b. `[reactor, reactor0]` The clue gates are invisible to the lookahead

`advance` / `force_clue` score hypothetical **partner** clues through
`get_result`, never `eval_action` (`src/basics/eval.cpp:11-52`,
`src/conventions/reactor/state_eval.cpp:295-308`). So neither reactor's
low-clue-count gate nor reactor0's pace-clue tier gate applies inside the
lookahead: the bot models its partners as clueing freely at a low clue count
while throttling itself. Pre-existing for reactor; more pronounced under
reactor0, whose window is a token wider and has no "we hold a play" conjunct.

Closing it means threading the hypothetical giver's Alice/Bob/Cathy assignment
down through `advance`, which has real regression surface for reactor — worth
live-game evidence first.

---

## 4. `[reactor0]` Eval tuning for double-discard and lock clue shapes

Reactor0 now owns `eval_action`'s clue branch and its own `get_result` (see
reactor0 CONVENTION.md §2a-§2c), but the shape-specific terms are still
missing:

- **Double discards** still score as ordinary 0-play clues. §2b refuses to
  *offer* one that buys nothing, which is a candidate filter rather than an
  eval term — a double discard competing against a play, a discard or another
  reactive is still scored as though it were any other 0-play clue.
- **Reactive locks** have no dedicated `get_result` term, and are deliberately
  exempt from §2b. `eval_game`'s `lock_penalty` never fires for the clue that
  causes a lock, because the `CHOP_MOVED` stamps land a turn later.
- **Blind-play clues** (colour mode 2) still rely on the urgent CTP being
  visible to `hypo_plays`.

Also latent: `new_play_facts` counts `CALLED_TO_PLAY` transitions only, so on
an inverted suit — where a play call is stamped `CALLED_TO_DISCARD` — it
undercounts. §2b sits out inverted variants for exactly this reason; `§2a`'s
tier conditions H3/N3 have the same blind spot and have not been audited.

---

## 5. `[reactor0]` Colour mode 1 can ask the reacter to discard a known playable

The spec gates the colour play+dc mode's target walk on one condition only:
"if the target would make Bob discard a known **critical** card, target the
next leftmost playable". Implemented literally
(`src/conventions/reactor0/interpret_reactive.cpp:424-429`) — so a react slot
the reacter *knows* is playable is still eligible to be discarded, losing a
play. Reactor's equivalent loop additionally skips react slots in the
reacter's `obvious_playables`
(`src/conventions/reactor/interpret_reactive.cpp:536-537`).

Note as of v3.0.0 colour **mode 2** now walks its dc-candidates, skipping a
pairing dead by shared knowledge (§1d). Mode 1 still walks only past a
known *critical* react slot, which is the gap described here.

Deliberately not "fixed" without a ruling: adding the guard changes which
clue the convention selects, which is a convention change, not a bug fix.
Decide whether reactor0 adopts the known-playable skip.

---

## 6. `[reactor]` `target_discard` stamps before it checks

`reactor::target_discard` (`src/conventions/reactor/interpret_clue.cpp:246-272`)
writes `CALLED_TO_DISCARD` + `urgent` + `signal_turn` and only *then* tests
whether the narrowing emptied, returning `std::nullopt` with the stamp left
behind. `target_play` has the same shape. Reactor0 works around this by
snapshotting and rolling back its candidate walks (`Rollback`,
`src/conventions/reactor0/interpret_reactive.cpp:53-72`); reactor itself does
not, so an abandoned candidate can keep a call no clue ever made.

Fixing it in the shared primitives would touch every reactor path at once, so
it wants its own change with the 122-replay corpus as the gate.

---

## 7. `[reactor]` Giver-only knowledge retargets instead of rejecting

Reactor's reactive walks `continue` past a candidate rejected by
`would_lose_inverted_reacter`
(`src/conventions/reactor/interpret_reactive.cpp:316, 548, 689, 849`). That
guard reads `state.deck[react_order]`, which the reacter cannot see, so the
giver silently retargets on knowledge the reacter lacks and the two decode
different pairings. Reactor0 rejects instead (CONVENTION.md §1g); reactor
should be reviewed the same way, but its replay corpus pins the current
behaviour, so it needs its own change.

---

## 8. `[reactor0]` `stable_rank`'s no-newly-touched play promise

`stable_rank`'s direct-play branch can pick a focus via
`leftmost_could_be_playable` and then empty its `inferred` when filtering to
playables, returning `std::nullopt` → `MISTAKE`
(`src/conventions/reactor0/interpret_clue.cpp:201`, `:218-220`). Since §1i now
guarantees no layer is handed an empty `inferred`, the remaining empty case
means "this focus genuinely cannot be playable", where `MISTAKE` may still be
right — but the branch has no test either way. Decide and pin it.

---

*(Chop selecting the most recent CTD by `signal_turn` shipped in v1.11.0 —
`Game::chop`, `src/basics/decide.cpp:419-446`, pinned by
`tests/test_basics/test_chop.cpp`.)*

---

## 9. `[endgame]` The solver cannot safely be made to enumerate all 100% lines

behavioral_changes_2.txt 2.1 asked for: run to the time limit, collect every
action with winrate 1, and prefer one where the acting player plays. Attempted
and **reverted** — the three early-exits on `Fraction(1)` (`solver.cpp`
`optimize_full`'s `stop`, `optimize`'s per-action return, and `solve`'s
multi-hypo `break`) are load-bearing for *accuracy*, not just speed. Removing
them made `EndgameReplay1885375.Turn35EndgameWinrateIsOne` report **2/15
instead of 1** and take 12.5 s instead of 1.4 s: without the short-circuit the
search runs into the 6 s deadline, and a timed-out sub-search is scored **0**
and is indistinguishable from a loss (`:301-303`, `:394-396`). The whole
reactor suite went 19.2 s → 39.1 s.

Prerequisites before this can be revisited:

1. **Make truncation distinguishable from a loss.** Until a timed-out branch
   stops contributing 0, any longer search can report a *worse* winrate than a
   shorter one, which is what bit here.
2. **The draw-filter asymmetry.** Clues are scored against `undrawn` — one
   `GameArr` with `drew == nullopt`, never filtered — while plays lose
   probability mass to the `winnable_draws` filter (`:293-296`, `:386-390`).
   A genuinely 100% play can therefore score below a 100% clue, which defeats
   the preference from underneath.
3. Several paths return an action at winrate 1 without ranking at all:
   `trivially_winnable` defaults to a **discard** (`helper.cpp:121`),
   `solver.cpp:491` returns an arbitrary `find_all_clues().front()`, and
   `:551` returns `performs.back()`, which by the ordering at `:270-278` is
   never a play.

A cheaper shape worth trying first: once a NON-play reaches the ceiling,
continue evaluating only the remaining **play** candidates rather than the
whole list — that bounds the extra work to what the preference actually needs.

---

## 10. `[endgame]` The feasibility layers are still blind to inverted suits

v4.0.0 taught the *search* about orange (`possible_actions` emits a
stack-advancing orange as a `PerformDiscard`, `perform_to_action` derives
`failed`, and the direct-win / tie-break predicates count a chuck as a play).
The layers that *prune* before scoring were left blind on purpose, so a
winning chuck can still be pruned before it is ever evaluated:

- **`trivially_winnable`** (`src/endgame/helper.cpp:113-141`) emits
  `PerformPlay{first}` at `:130` while advancing its modelled stack at `:131`.
  For an orange card those two disagree: the modelled stack goes up, but the
  emitted action pitches the card into the discard pile. It claims a win it
  cannot achieve. Only reachable with `endgame_turns` set, which is why
  `EndgameOrange.KnownPlayableOrangeIsOfferedAsAChuck` does not cover it.
- **`player_known_plays`** (`src/endgame/winnable.cpp`) counts known plays
  without asking which button performs them.

## 11. `[reactor0]` No orange tiering in reactor0's `get_result`

Reactor's clue scoring bumps a discard that advances a stack to `1.0`
(`src/conventions/reactor/state_eval.cpp:539-566`, via
`variants::discard_advances_stack`). Reactor0's own `get_result`
(`src/conventions/reactor0/state_eval.cpp`, §2c) has no equivalent, so in the
heuristic path reactor0 under-values a chuck relative to reactor. The §1b
orange ladder makes this more visible, since reactor0 now issues clues whose
whole point is to produce a chuck.

## 12. `[engine]` An unpinned playable orange is still pitched

`src/basics/decide.cpp:885-894` routes a playable orange to `PerformDiscard`
only when `thoughts[o].id(infer=true)` resolves to a **single** identity. A
card that is empathy-*playable* but ambiguous between an inverted and a
non-inverted suit (say `{o1, r1}` at zero stacks) falls through to
`PerformPlay`, which discards it if it really was orange. Reactor0's §1b
stamps sidestep this by narrowing `inferred` to the inverted playable set, but
a bare rank play reveal in an inverted variant does not.
