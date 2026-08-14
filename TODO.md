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

## 10. `[endgame]` `clueless_winnable`'s own discard offer is still blind

v4.0.0 taught the *search* about orange; v6.1.0 taught the feasibility layers,
which used to prune a winning chuck (or hallucinate a win) before it was ever
scored:

- `trivially_winnable` (`src/endgame/helper.cpp:114-150`) now emits the button
  that matches the stack it credits;
- `advance_state` (`src/endgame/winnable.cpp:89-152`) runs the button swap in
  both directions — it had no inverted branch at all, so a physical Play on an
  orange advanced the stack;
- `player_known_plays`' consumer and `winnable_simpler`'s discard fallback
  (`winnable.cpp:325-352`) emit a chuck for a playable orange and a pitch for
  an orange being thrown away.

What is left: `clueless_winnable`'s discard loop (`winnable.cpp:245-250`) still
offers `PerformDiscard` only for cards whose id is unknown. For an orange that
is a chuck, i.e. a play attempt that strikes on trash — the pitch (PerformPlay)
is the safe way to throw one away, and it is never offered there. Reachable
only in the clueless branch, where every hand is known, so it has not bitten
yet.

## 11. `[reactor0]` No orange tiering in reactor0's `get_result`

Reactor's clue scoring bumps a discard that advances a stack to `1.0`
(`src/conventions/reactor/state_eval.cpp:545-575`, via
`variants::discard_advances_stack`). Reactor0's own `get_result`
(`src/conventions/reactor0/state_eval.cpp`, §2c) has no equivalent, so in the
heuristic path reactor0 under-values a chuck relative to reactor. The §1b
orange ladder and the §1c orange-only rank chuck make this more visible, since
reactor0 now issues clues whose whole point is to produce a chuck.

v5.0.0 fixed the *lookahead* half of this — `advance` now simulates a playable
orange with the Discard button and `eval_game` prices an orange CTD as a play
call — but `get_result` itself still counts no play for a chuck, because
`playables_result` reads `hypo_plays` and `new_play_facts` counts
`CALLED_TO_PLAY` transitions only.

## 12. `[engine]` An unpinned playable orange is still pitched

`src/basics/decide.cpp:885-894` routes a playable orange to `PerformDiscard`
only when `thoughts[o].id(infer=true)` resolves to a **single** identity. A
card that is empathy-*playable* but ambiguous between an inverted and a
non-inverted suit (say `{o1, r1}` at zero stacks) falls through to
`PerformPlay`, which discards it if it really was orange. Reactor0's §1b
stamps and the §1c orange-only rank stamp sidestep this by narrowing
`inferred` to the inverted playable set, but a bare rank play reveal in an
inverted variant does not.

## 13. `[reactor]` Reactive vetting does not follow the inverted swap

The defect reactor0 fixed in v4.1.0 (bug_report_4.txt 4.1) exists unfixed in
reactor. Its four reactive sites all swap the reacter's action for an inverted
receiver target — `src/conventions/reactor/interpret_reactive.cpp:317`, `:549`,
`:690`, `:850` — while vetting the react slot for the un-swapped call, exactly
as reactor0 used to. Two consequences, one per direction:

- a reacter called to **discard** (because the target is orange) is rejected
  for not being possibly playable, so the clue degrades to a weaker reading;
- a reacter called to **play** is accepted on a criticality check alone, with
  no playability vet and no giver-only strike guard.

Left alone deliberately: fixing it is a second cross-version behaviour change
and puts reactor's 120-test corpus at risk. reactor0's `vet_react_slot`
(`reactor0/interpret_reactive.cpp:186-244`) is the shape to copy.

## 14. `[engine]` Nothing vetoes a clue whose reading predicts a strike

A predicted misplay is only ever *priced*, never rejected, anywhere in the
candidate pipeline:

- `advance`'s strike pessimisation (`reactor/state_eval.cpp:374-396`) is
  confined to the **play** branch; the discard branches (`:409-434`) never
  compare `advanced.state.strikes` to `game.state.strikes`, so a simulated
  chuck-strike is charged only the flat `eval_state` term and `try_discard`
  can dilute even that by its `clue_prob` blend.
- Neither `get_result` reads `hypo.state.strikes` — reactor's
  (`reactor/state_eval.cpp:152-292`) or reactor0's (§2c) — and reactor0's §2b
  filter returns early in any inverted variant
  (`reactor0/state_eval.cpp:532`), which is exactly where a chuck can strike.
- `find_all_clues`'s `reacter_critical_discard` guard
  (`src/basics/decide.cpp:565-579`) tests `is_critical`, not "would strike",
  and is unreachable from `take_action`, which builds its own pool at
  `:851-864`.

What actually stops the bad clue today is the *interpretation* layer's §1g
rejects, one per site. A single strike-predicting veto over the candidate pool
would be the general answer.

## 15. `[engine]` `advance` models every voluntary discard as a chuck

`advance`'s discard paths hand every order to
`variants::make_discard_for_simulation` (`reactor/state_eval.cpp:401`, `:410`,
`:415`), which sets `failed = inverted && !playable`. That is right for a CTD
(a real chuck) but wrong for an ordinary discard: `take_action` routes a
*known* orange through `PerformPlay` (a pitch,
`src/basics/decide.cpp:1007-1024`) and drops empathy-trash candidates that
could still be orange (`trash_is_orange_safe`, `:940-947`). So the lookahead
invents misplay strikes for discards the bot would never physically make, and
mis-scores every inverted-variant line that reaches a discard.

---

## 16. `[reactor]` A free pitch is still skipped on reactor's reactive side

v6.0.0 taught reactor0 that a play-type reaction on a card the holder knows is
an expendable orange is a **pitch**, not a blind play, and is unconditionally
safe (`variants::can_pitch_for_free`, CONVENTION.md §1d). Reactor's four
reactive sites (`src/conventions/reactor/interpret_reactive.cpp:316`, `:548`,
`:689`, `:849`) still hand every such candidate to
`would_lose_inverted_reacter`, whose blanket "a play-type call on an orange
loses the copy for nothing" is false for a trash one — so reactor still walks
past the pairing exactly as reactor0 did at replay 1957942 T19.

Reactor also lacks reactor0's `vet_react_slot` entirely, so the second half of
the fix has nowhere to live yet. Left alone deliberately: this is a
cross-version behaviour change on top of entry 13's, and reactor's 120-test
corpus is the gate. Fix entry 13 first — the two share the same call sites.

---

## 17. `[engine]` `hanabi_decision_tests` segfaults intermittently

`build/hanabi_decision_tests.exe` crashes with SIGSEGV roughly **1 run in 5**,
always at the same point — the last line written is
`[ RUN ] Reactor0PaceClueGate.SilentBelowPaceThree`. The remaining runs exit 1
with the five known `HighValueClueGate` failures and no crash.

Not a regression: reproduced at **v4.1.0** (commit 013580b, before the v5/v6
orange work) in a clean worktree build, 1 crash in 12 runs, at the identical
test. It survives at v6.1.0 at the same rate.

Notes for whoever picks it up:
- `Reactor0PaceClueGate.*` run **alone** passed 8/8 and never crashed, so it is
  state left by an earlier test in the binary rather than that test's own
  fixture — or a stack depth issue that only the full run reaches.
- No `gdb` in this MSYS2 environment; the build is RelWithDebInfo, so a trace
  needs one installed (`pacman -S mingw-w64-ucrt-x86_64-gdb`) or a WER dump.
- `SilentBelowPaceThree` sets `pace() < 3`, which also makes `in_endgame()`
  true (`pace() < num_players - 1`, `decide.cpp:428`) — the endgame solver and
  the deepest `advance` recursion are both in reach there.
