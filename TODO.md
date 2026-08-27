# TODO — convention that is legal but not implemented

The per-convention CONVENTION.md files
([reactor](src/conventions/reactor/CONVENTION.md),
[reactor0](src/conventions/reactor0/CONVENTION.md)) describe the bot **as it
behaves**. This file is the other half: rules that are part of a convention
and legal to play, but that the current build does not produce or decode, plus a
few places where it decodes something incorrectly and the fix is blocked on an
open question rather than on effort.
Each entry is tagged with the convention(s) it applies to. Anything here is a
known gap, not a disagreement about what the convention says.

Delete an entry when it ships, and update the owning convention's
`CONVENTION.md` / `DECISION_MAKING.md` / `GLOSSARY.md` wording in the same
commit.

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

## 3b. `[endgame]` A truncated search still speaks with a full voice

**Today.** `SolveResult::timed_out` (v8.4.0) tells the caller THAT the search
ran out of time, and `decide.cpp`'s fork uses it to prefer a certain play, then
a standing call that could advance, before deferring to the result. That covers
the turns where we hold something worth doing.

**What it does not cover.** The flag is binary: it says nothing about HOW
truncated the search was, so a run that explored 99% of the tree and one that
explored none are indistinguishable. And when no playable action exists, a
truncated clue or discard is still taken at face value — the pre-check has
nothing to offer there and the ordinary `winrate >= 1%` test applies to a number
that is only a lower bound.

**Why it matters.** 451 of 1207 endgame solves in `logs/` (37%) hit the 6 s
deadline; only 38% finished under 100 ms. The budget is doing real work.

**Possible directions.** Report explored/total node counts alongside the flag so
the fork can scale its trust; or spend the budget breadth-first so a truncated
result is at least uniformly shallow rather than depth-biased toward whichever
action was enumerated first.

## 2. `[reactor, reactor0]` Bluffs are never GIVEN (reactor0 now reads them)

**Convention.** A rare reactive move in which the reacter *believes* they are
playing a card that connects with a one-away-from-playable card in the receiver's
hand, but plays a different card. It is legal so long as the receiver can tell
**after** the reaction that their target is not actually playable — they then mark
it one-away-from-playable and chuck their chop as normal.

**Reading is done for reactor0**, as of v8.0.0: an empty receiver-CTP set after
a reaction that stacked a plain card is read as a bluff, or as a dupe bluff when
not even a one-away reading survives (its `CONVENTION.md` §1d.1,
`tests/test_reactor0/test_reactive_bluff.cpp`). What remains open is the giving
half, in both conventions, and reading under reactor.

**Today.** Neither convention initiates one, and reactor decodes neither. The
POV-invariant
abort in reactor's finesse phase
(`src/conventions/reactor/interpret_reactive.cpp:832-846`) — mirrored by
reactor0 (`src/conventions/reactor0/interpret_reactive.cpp:614-619`) —
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
low-clue-count gate nor reactor0's decision procedure applies inside the
lookahead: the bot models its partners as clueing freely at a low clue count
while throttling itself. Pre-existing for reactor; more pronounced under
reactor0, whose window is a token wider and has no "we hold a play" conjunct.

**v7.0.0 widens this gap rather than closing it.** The asymmetry used to be one
gate; once reactor0 chooses its own clue by an ordered rules procedure while its
partners are still modelled by a `get_result`-shaped score, the self/partner
divergence is the *entire decision rule*. Any future work here should target
reactor0 first, since it is now the convention whose model of its partners is
least like itself.

Closing it means threading the hypothetical giver's Alice/Bob/Cathy assignment
down through `advance`, which has real regression surface for reactor — worth
live-game evidence first.

---

## 4. `[reactor0]` `new_play_facts` undercounts plays on an inverted suit

`new_play_facts` (`src/conventions/reactor0/state_eval.cpp:170-187`) counts
`CALLED_TO_PLAY` transitions only. On an inverted suit a *play* call is stamped
`CALLED_TO_DISCARD` (`reactor0/interpret_reactive.cpp:548-551`), so a genuine
two-play reactive reads as zero plays there.

This matters more, not less, after the v7.0.0 overhaul: `new_play_facts` is
**kept** machinery, and the clue-tier conditions built on it — H3 ("two new
plays, one at the clue-regain rank") and N3 ("two new plays") — inherit the blind
spot and have never been audited in an inverted variant. The old §2b filter sat
those variants out for exactly this reason; the new General Clue Evaluation List
has no such exemption, and its priorities 1 and 2 are defined in terms of plays.

*This entry is what survives of the old "eval tuning for double-discard and lock
clue shapes" entry. The rest of it — missing `get_result` terms for double
discards, reactive locks and blind-play clues — was made moot by v7.0.0, which
deletes `get_result` outright in favour of the ordered priority list in
[reactor0's DECISION_MAKING.md](src/conventions/reactor0/DECISION_MAKING.md).*

---

## 5. `[reactor0]` Colour mode 1 can ask the reacter to discard a known playable

The spec gates the colour play+dc mode's target walk on one condition only:
"if the target would make Bob discard a known **critical** card, target the
next leftmost playable". Implemented literally
(`src/conventions/reactor0/interpret_reactive.cpp:363-373`) — so a react slot
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
`src/conventions/reactor0/interpret_reactive.cpp:59-77`); reactor itself does
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

## 10. `[endgame]` — CLOSED in v6.2.0

The endgame layers are inverted-aware end to end. For the record, the sites and
the versions that fixed them:

- v4.0.0 — the search proper: `possible_actions`' play loop, `perform_to_action`
  deriving `failed`, the direct-win / tie-break predicates.
- v6.1.0 — the feasibility layers, which used to prune a winning chuck or
  hallucinate a win before it was scored: `trivially_winnable`'s `i == 0`
  overwrite, `advance_state` (which had no inverted branch at all, in either
  direction), `player_known_plays`' consumer and `winnable_simpler`'s discard
  fallback.
- v6.2.0 — the top-level discard candidate: `Game::find_all_discards`
  (`src/basics/decide.cpp:1128-1173`) returned an unconditional
  `PerformDiscard`, which on an orange is a chuck, and it is the solver's ONLY
  discard candidate. bug_report_5_0_0.txt. Plus two siblings found with it:
  `two_critical_play_action`'s unconditional `PerformPlay` (which pitched a
  singleton-critical orange away) and `trivially_winnable`'s filler action.

**Correction to what this entry said in v6.1.0.** It claimed
`clueless_winnable`'s discard loop was blind for the same reason. It is not:
that loop offers `PerformDiscard` only for cards whose id is **unknown**
(`winnable.cpp:251-256`), and a holder who cannot tell their card is orange
really would press Discard — modelling that is correct, not a bug. The entry
was a false lead.

## 11. *(closed by the v7.0.0 plan — reactor0 orange tiering)*

Was: "no orange tiering in reactor0's `get_result`" — reactor0 under-valued a
stack-advancing chuck because its `get_result` had no equivalent of reactor's
`1.0` bump.

`get_result` is deleted by v7.0.0. The requirement is carried forward as an
**acceptance criterion** of the new framework rather than as a missing term: *a
chuck that advances an inverted stack must outrank a generic discard by rule.*
See [PLAN.md](PLAN.md) §11.

## 12. `[engine]` An unpinned playable orange is still pitched

`src/basics/decide.cpp:889-908` routes a playable orange to `PerformDiscard`
only when `thoughts[o].id(infer=true)` resolves to a **single** identity. A
card that is empathy-*playable* but ambiguous between an inverted and a
non-inverted suit (say `{o1, r1}` at zero stacks) falls through to
`PerformPlay`, which discards it if it really was orange. Reactor0's §1b
stamps and the §1c orange-only rank stamp sidestep this by narrowing
`inferred` to the inverted playable set, but a bare rank play reveal in an
inverted variant does not.

**Scope, after v7.1.0: reactor only.** Reactor0 no longer reaches
`decide.cpp:889-908` — `choose_action` picks its play or discard first. Its
phase-2 pitch list excludes any card whose `possible` contains an inverted
identity, so the ambiguous `{o1, r1}` case is chucked or left alone rather than
pitched (`reactor0/calls.cpp`, and the regression test
`Reactor0Action.APlayableInvertedCardIsChuckedNotPitched`).

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

## 14. `[engine]` Nothing vetoes a clue whose reading predicts a strike — CLOSED for reactor0

**Closed for reactor0 in v7.x (`predicts_a_strike`) and fully closed in v8.9.0.**
`predicts_a_strike` (`src/conventions/reactor0/decision.cpp`) is the veto this
entry asked for: `select` drops any candidate whose reading stamps a call on a
card that is not actually playable, at every rung of `choose_clue` — and, since
v8.9.0, of `choose_endgame_clue` too. The **endgame fork was the remaining
hole**: it returned the solver's clue without ever building reactor0's candidate
pool, so the veto never ran there. Replay 1971808 T59 lost a point to exactly
that, and `prefer_stall_clue` (`src/basics/decide.cpp`) closes it.

Three exceptions remain by design: §4's floor and `choose_very_high_clue`'s
default-tiebreak fallback do not filter, and rung 4.7 allows a strike
deliberately.

**Still open for reactor**, whose scorer prices a predicted misplay rather than
rejecting it. The original analysis follows; note its `find_all_clues` bullet
now cites a stale line number — `take_action` builds its pool through
`enumerate_clue_candidates`, not at `:851-864`.

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

**v7.0.0 made this tractable for reactor0**, and it shipped: an ordered priority
list is the first structure in the codebase where a candidate can be *rejected*
rather than merely priced — the pool is walked, not summed — so the veto had
somewhere natural to live. Note the second bullet above stays true either way:
the §2b early-return in inverted variants was kept machinery, not part of the
deleted scorer.

## 15. `[engine]` `advance` models every voluntary discard as a chuck

`advance`'s discard paths hand every order to
`variants::make_discard_for_simulation` (`reactor/state_eval.cpp:414-420`,
`:437`, `:446`, `:451`), which sets `failed = inverted && !playable`. That is
right for a CTD (a real chuck) but wrong for an ordinary discard: `take_action`
routes a *known* orange through `PerformPlay` (a pitch,
`src/basics/decide.cpp:1026-1044`) and drops candidates that could still be
orange (`discard_button_is_safe`, `:938-958`). So the lookahead invents misplay
strikes for discards the bot would never physically make, and mis-scores every
inverted-variant line that reaches a discard.

**The mirror of this, found at replay 1961419 T11 (v6.6.0):** for the bot's *own*
top-of-tree action the error runs the other way. `make_discard_for_simulation`
keys on `state.deck[order].id()`, which is null for our own cards, so a candidate
`DiscardAction{us, o, -1, -1, false}` is handed to `Game::on_discard`
(`src/basics/game.cpp:263-305`) with `suit_index == -1`; `inverted_id` is then
false, `failed` is false, and the simulation scores a clean clue-regaining
discard. No strike, no discard-pile entry, no `max_score` loss — the eval cannot
price the chuck risk of its own possibly-orange discard at all. Candidate removal
(§2.3's filter) is the only mechanism available today, which is why v6.6.0 fixed
it there. Related: entry 14, "a predicted misplay is only ever priced, never
rejected".

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

`build/hanabi_decision_tests.exe` crashes with SIGSEGV roughly **1 run in 3-5**,
always at the same point — the last line written is
`[ RUN ] Reactor0PaceClueGate.SilentBelowPaceThree`. Every run that does not
crash passes 47/47 (the five `HighValueClueGate` failures this entry used to
mention were fixture bugs, fixed in v6.5.0/v6.6.0).

Not a regression: reproduced at **v4.1.0** (commit 013580b, before the v5/v6
orange work) in a clean worktree build, 1 crash in 12 runs, at the identical
test. It survives at v6.1.0 and v6.6.0 at the same rate.

Note for the v7.0.0 overhaul: `test_pace_clue_gate.cpp` is rewritten by that
change, so the crash's *landmark* may be renamed or removed. The underlying bug
is state left by an earlier test in the binary, not that test — expect it to
resurface under a new name rather than to disappear.

Notes for whoever picks it up:
- `Reactor0PaceClueGate.*` run **alone** passed 8/8 and never crashed, so it is
  state left by an earlier test in the binary rather than that test's own
  fixture — or a stack depth issue that only the full run reaches.
- No `gdb` in this MSYS2 environment; the build is RelWithDebInfo, so a trace
  needs one installed (`pacman -S mingw-w64-ucrt-x86_64-gdb`) or a WER dump.
- `SilentBelowPaceThree` sets `pace() < 3`, which also makes `in_endgame()`
  true (`pace() < num_players - 1`, `decide.cpp:428`) — the endgame solver and
  the deepest `advance` recursion are both in reach there.

---

## 18. `[engine]` Other consumers still read an inverted CTD as "throw this away"

v6.3.0 taught the reaction-resolution helper that a `CALLED_TO_DISCARD` on an
inverted (Orange / Dark Orange) suit is a **chuck** — a play call — not a
throw-away. Several other consumers of CTD have not learned it. None is
reachable from replay 1959065; all are the same defect class, and all bite
hardest in Dark Orange, where every card is `oneOfEach` and therefore critical.

- **`src/basics/decide.cpp`'s urgent CTD dispatch** fires only when
  `!possible.forall(is_critical)`. Every Dark Orange card fails that, so an
  urgent chuck is *never* dispatched from the urgent scan and the `break` ends
  it. The CTP arm right above already carries a `can_pitch_for_free` exemption
  (v6.0.0); the CTD arm has no counterpart.
- **`Game::find_all_discards`** routes a card whose holder knows it is inverted
  to `PerformPlay` — the pitch — with no CTD check. So the endgame solver models
  a *called* chuck as throwing the critical away. That routing is v6.2.0's fix
  for bug_report_5_0_0 and is right for a trash orange; it wants a "unless the
  card carries a chuck call" arm.
- **`src/conventions/reactor/state_eval.cpp`'s `eval_action` discard branch**
  folds `status == CALLED_TO_DISCARD` into `is_trash`, scoring it `0.0` and
  gating out the orange tiering that would otherwise give a stack-advancing
  discard `1.0`. This binds reactor0 too, but only until v7.1.0: reactor0
  reaches it solely by delegating non-clue actions at
  `reactor0/state_eval.cpp:451-452`, and phase 2 of the decision overhaul
  replaces that delegation. Re-label this bullet **reactor-only** when v7.1.0
  ships.
- **`Player::order_trash`** folds CTD into "trash" outright. Today that is
  masked only by the all-critical early-out immediately above it — remove or
  weaken that early-out and a called Dark Orange chuck becomes discardable as
  ordinary trash.

Related state-hygiene bug, found in the same investigation: **`Game::elim`'s
step-1 sweep clears `status`/`by` with a raw two-field assignment instead of
`ConvData::cleared()`**, so a stale `meta.trash` and `signal_turn` outlive the
status they were stamped with. That is precisely how bug 6.2.0 left its card
branded trash with no call attached. `check_missed`, `erase_call` and the bomb
reset all use `cleared()`; the two `elim` sweeps and `clear_contradicted_call`
do not.

---

## 19. `[engine]` An all-orange discard candidate is dropped where it could be pitched

§2.3's chuck-safety filter (`discard_button_is_safe`,
`src/basics/decide.cpp:938-958`) rejects any candidate whose `possible` contains
an inverted identity. That is exactly right for a set that *straddles* an
inverted and a plain suit — neither button is safe there, so there is nothing to
re-route to. But when **every** possibility is inverted, `PerformPlay` is a pitch
for all of them: a guaranteed clean discard that also regains the token. The
filter drops those too, which is safe but leaves a free pitch on the table.

**Also reactor0's, as of v7.1.0.** Phase 2's floor uses its own
`chuck_button_is_safe` with the same three clauses, so the same free pitch is
left on the table there. Fixing the shared predicate should fix both.

The precedent for the tighter test already exists: `Game::find_all_discards`
(`decide.cpp:1176-1182`) uses `poss.forall(inverted)` over a
`common ∩ per-player` intersection, and `variants::can_pitch_for_free`
(`src/conventions/variants/inverted.cpp:104-110`) is the stricter all-inverted
**and** all-basic-trash form used by the urgent-CTP exemption.

Left out of v6.6.0 deliberately, and it is more than a one-line change: the
emission loop must pair the `PerformPlay` with an `Action` for `eval_for`, and
neither available shape prices a pitch correctly. `PlayAction{us, o, -1, -1}`
scores `+1.5` (`reactor/state_eval.cpp:568`) and `on_play` with `suit_index == -1`
skips the inverted branch, losing the clue regain — overvalued *and*
mis-simulated. `DiscardAction{us, o, -1, -1, false}` models card-gone plus
clue-regain correctly but then meets the `0.5` orange floor, above the `0.0` a
known-trash discard gets — so the bot would prefer pitching a possibly-useful
orange over discarding actual trash, which is the same inversion that produced
the 1961419 bug. Doing this properly needs a pitch tier priced between the two,
which is a reactor-side pricing change: reactor0 stops using this scorer for
its own discards at v7.1.0.

---

## 20. `[engine]` The `locked_discard` fallback presses Discard with no inverted re-route

`src/basics/decide.cpp:1128` — when `all_discards`, `all_clues` and `all_plays`
are all empty, `take_action` returns a bare
`PerformDiscard{m.locked_discard(...)}`. No pitch/chuck routing, unlike both the
ordinary emission loop (`:1026-1044`) and `find_all_discards` (`:1176-1182`). On
an inverted suit that is a play attempt, so the last-resort path is the one place
that can still chuck a card the rest of the engine would have protected.

Pre-existing, but v6.6.0's widened chuck-safety filter makes it marginally more
reachable: emptying `discard_orders` is now easier, and this is where an emptied
pool lands. Note the situation is genuinely forced — with nothing else to do the
bot must discard something — so the fix is to route the button, not to refuse.

---

## 21. `[engine]` `discard_button_is_safe` clause 2 trusts `inferred`, not `possible`

`src/basics/decide.cpp:955` exempts a candidate when
`m.thoughts[o].id(/*infer=*/true)` resolves. `Thought::id`
(`src/basics/card.cpp:29-44`) resolves on `possible.length() == 1` (sound) **or**
`inferred.length() == 1` (not sound — an inference is a convention deduction that
can be wrong). The comment justifying the exemption only covers the case where
the singleton *is* inverted. Two leaks, in opposite directions:

- `inferred = {r4}`, `possible = {r4, o4}` → emitted as `PerformDiscard`
  (`:1039-1042`); if the card really is Orange 4, that is a chuck-strike. This is
  the escape hatch `tests/test_reactor/test_misc/test_replay_1885550.cpp:144-148`
  uses to skip its own assertion.
- `inferred = {o4}`, `possible = {r4, o4}` → re-routed to `PerformPlay`
  (`:1033-1037`); if the card really is Red 4, that is a real play attempt.

The sound formulation keys on `possible` alone: press Discard when
`possible.forall(!inverted)`, press Play when `possible.forall(inverted)`, and
otherwise drop — `possible.length() == 1` is subsumed by whichever `forall`
applies. Not folded into v6.6.0 because it also tightens the empathy-trash pool,
which is a separate behavioural decision deserving its own version bump.

---

## 22. `[reactor]` A called discard scores the same as generic trash, so marginal clues beat it

`reactor/state_eval.cpp:574-575` folds `status == CALLED_TO_DISCARD` into
`is_trash`, scoring it exactly `0.0`, and `:590`'s `if (!is_trash)` then skips the
orange tiering for it entirely. So honouring an explicit discard signal sits at
the same tier as throwing away generic trash, and **any** clue scoring above zero
outranks it.

Replay 1961419 T11 is the worked example. Once v6.6.0 stopped the bot chucking a
no-safe-button card there, it did **not** fall back to the card the rank-4
referential discard had actually called to discard (a basic-trash Muddy Rainbow 1,
a free clue regain). It gave `rank 3 → will-bot69` instead — which `ref_discard`
reads as a **LOCK**, because the clue touches will-bot69's oldest unclued card
(`reactor/interpret_clue.cpp:339-358` locks when `list_` contains the minimum
unclued order). That buys nothing: will-bot69's chop was a Muddy Rainbow 4 whose
own duplicate sat clued in the same hand, so they already had a free discard.

Two contributing gaps: a LOCK has no dedicated `get_result` term and scores as
an ordinary 0-play clue, and a called discard has no term above generic trash.
This is the third bullet of entry 18, promoted here with a reproducing replay.

**Scope, after the v7.0.0 plan.** Both halves that remain are reactor's — the
`0.0` fold is `reactor/state_eval.cpp:574-575` / `:590`, and the LOCK reading is
reactor's `ref_discard` (`reactor/interpret_clue.cpp:339-358`). Fixing it is a
tuning change in reactor's scoring, gated by reactor's 121-test corpus. The
reactor0 half is **not** a tuning problem any more: it converts into an
acceptance criterion of the new framework — *honouring an explicit called discard
must outrank a value-less lock clue by rule* — see
[PLAN.md](PLAN.md) §11.

---

## 23. `[reactor0]` A finesse onto a duplicated inverted card still strikes

**Convention.** An Orange 2 → Orange 3 finesse is permissible, and reactor0 gives
it: at replay 1957905 the reactive rank Phase B clue is priority 1 of the General
Clue Evaluation List, which is correct
([DECISION_MAKING.md](src/conventions/reactor0/DECISION_MAKING.md), priority 1).

**Today.** The line eventually **strikes**, because the duplicate Orange 2 is
chucked later in the game. Nothing in the reactive vetting notices that the
promised inverted card has a second copy whose chuck will misplay once the first
copy advances the stack. The clue itself is sound; the failure is downstream, in
how a duplicated inverted card is tracked after the finesse resolves.

**Why it is not fixed with the clue.** Rejecting the finesse would be the wrong
fix — the convention permits it, and priority 1 has no quality condition by
design. The fix belongs wherever the duplicate chuck is decided, which is the
same machinery entry 12 (an unpinned playable orange is still pitched) and entry
18 (other consumers read an inverted CTD as "throw this away") are about.

**Touchpoints.**
- `tests/test_reactor0/test_misc/test_replay_1957905_orange_chuck_must_be_playable.cpp`
  pins the clue and carries a note pointing here.
- The inverted-suit helpers in `src/conventions/variants/inverted.cpp`, and
  `discard_button_is_safe` (`src/basics/decide.cpp`).

---


---

## 25. `[reactor0]` A deferral keeps the call but loses the waiting connection

**Convention.** From v7.22.0 a reacter may defer — give a clue instead of
reacting — and the reacter call survives it, to be actioned on a later turn
(`CONVENTION.md` §1d.1). The receiver still has to be able to decode that
reaction when it finally happens.

**Today.** Only the *stamp* survives. `Game::interpret_clue`
(`src/basics/decide.cpp`) still runs
`if (!waiting.empty() && waiting.front().reacter == action.giver) waiting.clear();`
a few lines below the `check_missed` guard, so the waiting connection the call
came from is destroyed. When the deferring reacter eventually chucks or pitches,
`react_play` / `react_discard` see an empty (or replaced) `waiting` and decode
nothing — so the receiver may never learn the target the reaction was meant to
reveal.

**Why it was not fixed with the call.** reactor0 stores at most one waiting
connection (`reactor0/interpret_reactive.cpp` clears then pushes), so keeping the
old one is not simply additive. Leaving it in place newly exposes the five
`if (!game.waiting.empty()) game.waiting.front().react_order = react_order;`
writes in `reactor0/interpret_reactive.cpp`, which today are skipped because
`waiting` is empty by the time they run, and would start mutating a connection
that is about to be discarded anyway. Getting this right means deciding what a
reactor0 seat should hold when two reactives are live at once, which is a
convention question and not a code tidy.

**Touchpoints.**
- the `waiting.clear()` in `Game::interpret_clue` (`src/basics/decide.cpp`).
- `reactor0/interpret_reactive.cpp` — the single-WC install and the
  `react_order` writes above it.
- `react_play` / `react_discard` (`reactor0/interpret_reaction.cpp`), which take
  `waiting.front()` rather than searching for the connection whose `reacter`
  matches the acting player.

---

## 26. `[endgame]` The search prunes a winning gamble it can see, because partners' unclued plays are invisible

Replay 1970943 T24 (stacks `[3,5,5]`, deck empty, three turns left). The solver
**did** generate the winning `PerformPlay{12}` — one of its three hypotheses
assigns order 12 = r4 at probability 1/2 — and then dropped it at
`solver.cpp:168`, because the forward walk decided the line was unwinnable.

The reason is an asymmetry inside one predicate. `Player::thinks_playables`
subtracts known trash only from cards that are **touched**:

```cpp
bool et = exclude_trash && game.is_touched(o);   // src/basics/player_game.cpp:186
```

* our order 12 is **clued**, so the subtraction applies and
  `{r2,r4,o1,o2,o3,o4}` collapses to `{r4}` → offered as a play;
* p1's r5 is **unclued**, so the subtraction is switched off, `{r1..r5}` never
  collapses to `{r5}`, and `player_known_plays`
  (`src/endgame/winnable.cpp:265-303`) reports p1 has no play at all.

So the search can imagine our gamble but not the partner cashing it. p1 in the
actual game blind-played the r5; it failed only because the r4 was never laid.

v8.7.0's forced-endgame rule 0b answers the position without a search. Fixing
the predicate would let the solver find it unaided, but it makes the whole
search optimistic about blind plays, which changes every endgame winrate it
reports — and §9 already documents how fragile that ranking is. The narrower
shape worth trying first: at `cards_left == 0` only, credit a seat that holds
the unique remaining copy of a `find_must_plays` identity
(`src/endgame/helper.cpp:93-112` already computes exactly that set).

---

## 27. `[reactor0]` The ladder's pitch list uses a weaker playability notion than the solver's

Same asymmetry as §26, one layer up. `action_lists`
(`src/conventions/reactor0/calls.cpp:249`) calls

```cpp
game.players[player].thinks_playables(game, player)
```

with **no `exclude_trash` argument**, so `et` is false and no trash subtraction
happens. `EndgameSolver::possible_actions` (`src/endgame/solver.cpp:186-192`)
passes `exclude_trash=true` for the same question.

At 1970943 T24 that is why order 12 never reached `lists.pitch` and rung **8
`leftmost_unpinned`** — which would have played it — never fired; the turn fell
through to rung **11 `chuck_leftmost`** and threw a known-trash b1.

Passing `exclude_trash=true` there would align the two, but it widens the pitch
list on every turn of every game, not just endgames, so it needs its own
corpus run rather than being folded into a bug fix.

---

## 28. `[reactor0]` A chuck call is obeyed even when `possible` says the card may be critical

v8.8.0 made `is_chuckable` require `possible` — not just `inferred` — to be all
trash before throwing a card the team invested in. That guard does **not** cover
`CALLED_TO_DISCARD` cards: they join the chuck list through their own arm
(`src/conventions/reactor0/calls.cpp:289-290`) and never reach `is_chuckable`.

Across the log corpus, **123 of the 154** discards whose `inferred` read as trash
while `possible` disagreed came in through that arm, and two of them cost max
score (1957932 T42, 1966558 T25).

The fix belongs where the call is READ, not where it is obeyed. `chuck_candidates`
(`include/hanabi/conventions/reactor0/interpret_clue.h:63-67`) already defines a
chuck call as "a plain-suit card that is **not playable and not critical**", so
the stamp should be narrowed to that set by `narrow_to_stamped_button` (`:69-75`)
when the reaction resolves. Guarding the obey path instead would have the bot
refuse a partner's explicit instruction, which is the convention's core loop and
would desync the signal.

---

## 29. `[endgame]` At one card left the search only sees PINNED playables

`EndgameSolver::possible_actions` picks its play candidates like this
(`src/endgame/solver.cpp:186-192`):

```cpp
if (infer || game.good_touch || state.endgame_turns) {
  playables = players[p].thinks_playables(game, p, /*exclude_trash=*/true);
} else {
  playables = players[p].obvious_playables(game, p);
}
```

At `cards_left == 1` **all three disjuncts are false** — `endgame_turns` is only
set when a draw empties the deck (`src/basics/game.cpp:511`) — so the root uses
`obvious_playables`, which does no trash subtraction at all
(`src/basics/player_game.cpp:174-180`, `exclude_trash` defaults false). A card
that is clued and whose non-trash readings collapse to a single playable is
therefore **invisible to the search's first pass**. It reappears only through
the `infer=true` retry at `solver.cpp:879-882`, which runs *only when the first
pass came back empty* — i.e. contingent on every discard candidate being pruned.

Replay 1972670 T25 is the case: our slot 4 read `{r2,r4}` from our own view with
red on 3, so the r4 was one trash-subtraction away from being a candidate, and
the winning line needed it. v9.2.0's rule 0c works around this from the forced
layer; the search itself still cannot see it.

Same family as §26 (`thinks_playables` hiding a partner's blind play) and the
v8.9.0 finding about the clue model. A fix here would cover all three rather
than adding forced rules one position at a time. `tests/test_reactor/test_endgame/
test_replay_1885467.cpp:129` already pins a `winrate == 0` that has this shape.

---

## 30. `[reactor0]` H1a alone has never lifted a clue out of LOW

`DECISION_MAKING.md`'s *Clue Tier Definitions* lists **H1a** among the NOT-LOW
conditions: Bob is unlocked, has no safe action, and his chop is *endangered*,
and that alone should be worth MEDIUM even when the Cathy conditions H1b/H1c
fail. `clue_tier` (`src/conventions/reactor0/state_eval.cpp:425-515`) has never
implemented it. `h1a` is computed at `:464` and read once, inside the H1
conjunction at `:468`; the NOT-LOW block below starts at N5 (`:485`) and never
consults it.

So a position where Bob is stuck on an endangered chop, but Bob could have
colour-clued Cathy himself (H1c false), reads **LOW** — and at 3 or fewer tokens
inside the unoccupied gate window that suppresses every clue on the turn.

Found while adding H4 in v9.3.0, whose fixture is exactly this shape: with H4
disabled, `Reactor0ClueTier.BobCriticalChopIsHighEvenWhenH1cFails` reads LOW
despite H1a holding. H4 now answers the *critical* case; the *endangered* case
the spec describes is still unhandled.

Not fixed in v9.3.0 because it widens what the gate admits on a class of turns
the change was not measured against, and the corpus A/B for that is a separate
piece of work. Either the arm is added or the spec line goes — they should not
keep disagreeing.

---

## 31. `[reactor0]` Alternating Clues: the bot does not reason about denying its partner a kind

In an Alternating Clues variant the kind you clue decides which kind your
partner may **not** clue next. A rank clue can therefore deny a partner the rank
clue they needed, and giving the "wrong" kind is a real cost even when the clue
itself is good.

v10.0.0 makes the bot play LEGALLY — `State::all_valid_clues` drops the blocked
kind off `State::last_clue_kind`, so the bot never proposes a clue the server
would reject and the endgame solver never costs out a line built on one — and
read clues correctly. It does not weigh the constraint it imposes: the clue
evaluation list has no term for "this leaves my partner only colour, and the
card that needs saving is only reachable by rank".

The natural home is a term in the General Clue Evaluation List's tiebreak, or a
tier condition, keyed on whether the partner has a needed clue of the kind about
to be blocked. Needs a corpus to size it against, and the bot has never played
one of these 66 variants, so there is nothing to measure yet.

---

## 32. `[reactor0]` A waiting connection's parity is recomputed from the clue kind, ignoring `wc.even_parity`

`ReactorWC::even_parity` is a snapshot of the clue's parity bucket taken when
the connection is created, so that a `/set` landing mid-game cannot change what
an already-given clue meant — the same insulation `rlocks` gets.

Two decision-layer sites do not read it. `decision.cpp` (the receiver button in
`shape_after_reaction`) and `state_eval.cpp` (the same button in the new-play
walk) both recompute the parity from `wc.clue.kind` and the CURRENT overrides.
A `/set` that moves a clue between buckets after the connection was created
makes them disagree with the reading every seat already agreed on.

v10.0.0 routed both through `reactive_assignment_for` so they are correct for
the target-parity variants, which was what forced the question. It did not
change where they read the parity FROM, because that is a behavioural change for
existing `/set` games and wants its own measurement. The fix is to prefer
`wc.even_parity` when it is set and fall back to the computed value only when it
is not — the same shape `reactor0::wc_is_even_parity`
(`interpret_reaction.cpp`) already uses.

---

## 33. `[reactor0]` The zero-clue safety promise and the chuck list have no defined interaction

The **zero-clue safety promise**: the chop is locked in on the turn a clue takes
the team down to zero clues, and is not reset until the player before Alice has
at least one clue remaining. Cards drawn during the stall are not chop, so the
player who drew them is not expected to throw them, and the promise is that the
locked card is safe to lose.

The lock itself is implemented. `Game::zcs_turn` records the turn the team ran
dry (`decide.cpp:352`), `chop`'s second pass skips any card drawn after it
(`decide.cpp:609-614`), and `reset_zcs` fires only on an action taken from a
state that still had a clue (`decide.cpp:351`, `:468`, `:497`) — so a discard
that buys the token back does not clear it, which is the "until the player
before Alice has at least one clue" part.

What is undefined is what happens when Alice holds a card she can *name* as
trash while the lock is on. Phase 2's chuck and pitch rungs all sit **above**
the `12.discard_chop` floor (`calls.cpp:468`, `:528`), so any chuck candidate
pre-empts the locked chop. v10.4.0 made that far more common by removing the
`possible`-must-agree guard from `is_chuckable`, and the two readings genuinely
conflict:

* **Honour the promise.** Partners have modelled the locked card as the one
  leaving. Throwing something else desynchronises the hand they think Alice has,
  which is exactly what the lock exists to prevent.
* **Take the free out.** The locked chop is only *promised* safe, not *known*
  safe, and a known-trash card in hand is a strictly cheaper thing to lose.

**Worked example — replay 1972691 T24**, "Odds and Evens & Light Pink (4 Suits)",
stacks `[4,4,4,2]`, `zcs_turn = 20` with one clue back on the counter.
will-bot67's locked chop was order 23, a **b5 — playable and critical**; order 27,
drawn during the stall, was correctly skipped. It threw the b5. From v10.4.0 it
throws order 13 instead, an r1 it reads as `{r1,r3,r4}` with red on 4, i.e.
known trash — saving the b5 but breaking the promise.

Deliberately left alone in v10.4.0: it predates that change, it is a convention
question rather than a bug in the chuck list, and resolving it means ruling on
which of the two readings wins. If the promise wins, the gate belongs in
`choose_action` ahead of the chuck rungs, keyed on `zcs_turn` and on the locked
chop still being in hand.
---

## 34. `[reactor0]` The bot can READ a spare-orange pitch but would never GIVE one

v10.8.0 taught the reacter to read a Play-button call on a clued slot with a
spare inverted reading as a **pitch** (step 3 of `stamp_react_play_button`,
CONVENTION.md §1f, replay 1974331 T8). The giver side did not move with it, and
two tests in `vet_react_slot` (`interpret_reactive.cpp`) still ask only about
playing:

* **The playability retarget** — `effective_possible_for(react_order).exists(is_workable)`.
  Shared knowledge, so a failure retargets. A clued slot whose `possible` holds
  nothing playable and nothing that connects, but which does hold a spare
  inverted reading, is retargeted away before any stamp runs and the pitch never
  fires. 1974331 does not exercise this: its slot 4 held a playable y1 in
  `possible` (Deceptive-Ones let a rank-3 clue touch it), which is exactly why
  the vet passed.
* **The giver-only reject** — `react_actual_id && !is_workable(*react_actual_id)`
  → `REJECT`. The giver CAN see the react card, and a spare orange is visibly
  unable to play, so the giver rejects the whole clue. This is the sharper half:
  **will-bot67 and will-bot69 can decode this clue from each other but neither
  can ever offer it.** It is only readable today because a human gave it.

Both want the same amendment — the vet's question must swap to affordability
when the call would be a pitch, exactly as the stamp's now does — and both are
strike checks, so widening them carelessly is the opposite failure: letting a
bare existential through would disable the strike tests for every clued card in
an Orange variant. That is why v10.8.0 changed the stamp only, where the
`target_play`-first ordering makes the fallback safe by construction.

The shape of the fix is probably to give the vet the same three-step ladder the
stamp has: ask playability first, and fall back to `slot_has_spare_inverted` for
a clued or stamped slot only when the playability answer is no. Deliberately not
attempted blind — it wants its own ruling and its own replay.

`tests/test_reactor0/test_orange_chop_and_pitch.cpp` documents the asymmetry:
its `pitch_pair_opts` fixture has to run from BOB's seat, because from the
giver's chair the clue is rejected before the stamp is reached.
