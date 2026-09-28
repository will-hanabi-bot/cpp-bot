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
(`src/conventions/reactor/interpret_reactive.cpp:742-756`) — mirrored by
reactor0 (`src/conventions/reactor0/interpret_reactive.cpp:524-529`) —
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
`CALLED_TO_DISCARD` (`reactor0/interpret_reactive.cpp:458-461`), so a genuine
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
(`src/conventions/reactor0/interpret_reactive.cpp:273-283`) — so a react slot
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
`Game::chop`, `src/basics/decide.cpp:433-460`, pinned by
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
  (`src/basics/decide.cpp:1142-1187`) returned an unconditional
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

`src/basics/decide.cpp:903-922` routes a playable orange to `PerformDiscard`
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
  (`src/basics/decide.cpp:579-593`) tests `is_critical`, not "would strike",
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
`src/basics/decide.cpp:1040-1058`) and drops candidates that could still be
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
`src/basics/decide.cpp:952-972`) rejects any candidate whose `possible` contains
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

`src/basics/decide.cpp:1142` — when `all_discards`, `all_clues` and `all_plays`
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

`src/basics/decide.cpp:969` exempts a candidate when
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
(`include/hanabi/conventions/reactor0/interpret_clue.h:67-77`) already defines a
chuck call as "a plain-suit card that is **not playable and not critical**", so
the stamp should be narrowed to that set by `narrow_to_stamped_button` (`:79-85`)
when the reaction resolves. Guarding the obey path instead would have the bot
refuse a partner's explicit instruction, which is the convention's core loop and
would desync the signal.

**Partly addressed in v16.11.0, and deliberately not closed.** The STABLE
referential discard — the one arm that narrowed nothing at all, so its note
listed the criticals outright (replay 2008422 T1) — now drops them, via
`narrow_stable_chuck` (`src/conventions/reactor0/interpret_clue.cpp:609-659`).
That is `target_discard`'s weaker filter, *not* `chuck_candidates`: the ruling
was that a stable "this slot is safe to throw" does not also claim the card is
unplayable, so the playable readings stay. So what this entry asks for is still
open on both counts — the stamp is not `chuck_candidates`-tight, and the CTD arm
at `src/conventions/reactor0/calls.cpp:312-313` still reaches the chuck list
without passing `is_chuckable`.

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
fail. `clue_tier` (`src/conventions/reactor0/state_eval.cpp:537-640`) has never
implemented it. `h1a` is computed at `:588` and read once, inside the H1
conjunction at `:592`; the NOT-LOW block below starts at N5 (`:624`) and never
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
to be blocked.

**v11.0.0 raises the stakes and supplies the corpus.** The stakes, because past
60% of the variant maximum a clue to Bob is STABLE again (CONVENTION.md §1f), so
the kind now selects between the colour and rank ladders rather than being pure
overhead -- denying a partner a kind can deny them a whole class of meaning. The
corpus, because the claim that "the bot has never played one of these 66
variants" is no longer true: there are 42 Alternating Clues games in `logs/` as
of v11.0.0, which is enough to size a term against.

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
dry (`decide.cpp:411`), `chop`'s second pass skips any card drawn after it
(`decide.cpp:797-803`), and `reset_zcs` fires only on an action taken from a
state that still had a clue (`decide.cpp:410`, `:608`, `:670`) — so a discard
that buys the token back does not clear it, which is the "until the player
before Alice has at least one clue" part.

**Settled in v16.23.0: a lock that leaves NO chop.** When every card that
predates the lock is clued, the promise has nothing to protect, and the ruling is
that Alice discards the first card drawn during the stall rather than pitching
slot 1 blind (`12.discard_stall_drawn`, `calls.cpp:533-561`; replay 2011327 T32,
DECISION_MAKING.md rung 12). What follows is the case that is still open: a locked
card that EXISTS, and a nameable trash card competing with it.

What is undefined is what happens when Alice holds a card she can *name* as
trash while the lock is on. Phase 2's chuck and pitch rungs all sit **above**
the `12.discard_chop` floor (`calls.cpp:494`, `:577`), so any chuck candidate
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

---

## 35. `[endgame]` The solver still has no model of private sight

v11.6.0 gave the endgame fork `prefer_known_discard` (DECISION_MAKING.md
precedence 0d), which stops it burning a card it cannot prove is worthless while
one it can sits in the same hand. That is a correction applied to the fork's
ANSWER. The search underneath still reasons from common-knowledge empathy, and
the same blind spot shows up on the play side.

Replay 1977971 T22 is the worked example for both halves. The discard half is
fixed. The play half: will-bot69's slot 1 read `{r4, l5}` and both were
playable, but the only l5 was face-up in will-bot67's hand — so the card was a
*known* r4. `hanabi::endgame::certain_plays` is built on empathy, so it does not
report it, and `prefer_certain_play` therefore cannot offer it.

The principled fix is to narrow the solver's own hypotheses for OUR hand by
`sight_narrowed` (`conventions/reactor0/facts.h`) before the search runs, which
would price the lines correctly rather than patching the answer afterwards. It
was not attempted with v11.6.0 because it changes every hypothesis the search
enumerates — a far larger blast radius than one report justified, and it wants
its own sweep. Note it is reactor0-only machinery today: `sight_narrowed` lives
under `conventions/reactor0/`, though nothing in it is convention-specific.

---

## 36. `[tooling]` `replay_log` cannot trace catch-up interpretation

`--trace` covers `take_action` only. Everything the engine infers while
RECONSTRUCTING the game from a snapshot -- every `interpret_clue`, every
reaction resolution -- happens before the trace sink exists, so it is invisible.

This is not cosmetic. A deferred reaction resolves during catch-up, so
`reactor0.deferred_reaction` NEVER appears in a trace for a real resolution;
the only records that show up come from hypothetical clue evaluation inside the
lookahead. Selecting sweep candidates by grepping the trace for that branch
therefore selects games whose LOOKAHEAD simulated a deferral, which is a
different population entirely -- v12.0.0's first sweep did exactly this and had
to be thrown away, having missed both 1978041 and 1975464, the two games the
feature was known to help.

The workaround used instead is to select from `debug.move_history`, which labels
each move (`{"k":"clue","v":"Reactive"}`) and so lets a deferral -- a reactive
clue answered by a clue -- be found structurally, with no replaying at all. That
works, but it only covers what the log records.

A `--trace-catchup` flag that arms the sink before reconstruction would make
interpretation directly observable and is probably a few lines. It was not done
alongside v12.0.0 because changing the binary mid-measurement is precisely what
the sweep hygiene rules forbid.

---

## 37. `[engine]` Our own empathy can pin an identity we do not hold

At replay 1973410 T66 (`Color Blind (6 Suits)`, reactor0) the bot's own five
cards are each PINNED by `game.me().thoughts[order].id()` -- t2, t3, b1, r2, b1
-- while the hand really contains a y4 and a p1. Empathy does not merely fail to
narrow; it narrows to the wrong answer, and to a *contradiction*: with the deck
empty, the identities it claims cannot all exist alongside what is visible
elsewhere.

Found while fixing the endgame solver's card accounting in v13.1.0. The solver
now DECLINES when the totals disagree rather than throwing, so this no longer
drops a turn -- but declining is a symptom guard, not a fix. Anything that trusts
`me().thoughts[].id()` for our own hand is being lied to in this position, which
includes every "we know what we hold" shortcut in the ladder.

Where to start: `Color Blind` gives colour clues no information, so the colour
half of every elimination is inert and the rank half does all the work. A
mis-cued `card_elim` / link resolution there would produce exactly this. The
position is reproducible from the log, and the accounting guard makes a good
tripwire: log when it fires and every instance is a live contradiction.

---

## 38. `[reactor0]` Rank Phase C never runs the giver-only orange loss guard

Every reactive site that can call the reacter to press a button the giver can
see is wrong for his card runs `variants::would_lose_inverted_reacter`
(`variants/inverted.cpp:31-51`): rank Phase A
(`interpret_reactive.cpp:433-440`), rank Phase B (`:532-540`), colour mode 1
(`:700-708`) and colour mode 2's plain branch (`:836-841`). **Rank Phase C
(`:577-647`) does not.**

Two readings slip through as a result, both giver-only, so §1g says the clue
should be REJECTed rather than walked past:

- an INVERTED dc-target swaps the reacter onto Play, and a react card the giver
  can see is a USEFUL orange is then pitched — the copy is thrown away for
  nothing. `can_pitch_for_free` is the exemption Phase A pairs with the guard,
  and it would carry over unchanged;
- a PLAIN dc-target leaves the reacter on Discard, and a react card the giver
  can see is an orange the stack is not waiting for is a chuck that strikes.

The second is caught today by the decision layer rather than by the convention:
`outcome_of` reads a CTD on an unplayable inverted card as `Outcome::STRIKE`,
so `predicts_a_strike` drops the candidate from every rung. The first is not
caught at all — a pitch reads as `Outcome::DISCARD`, which no rung vetoes.
That makes this a giver-side gap in the CONVENTION, which any seat reading
somebody else's clue also has.

Found while fixing v15.3.0 (Phase C walking on a giver-only REJECT, replay
2005309 T33), and deliberately left out of that change: it needs a replay that
actually exhibits it, and none is known. `test_reactive_inverted_vet.cpp`'s
`RankPhaseAStillRefusesToPitchAUsefulOrange` is the Phase A fixture to mirror.

---

## 39. `[tiiah]` Reverse-reactive dispatch — CLOSED in v16.2.0

`src/conventions/tiiah/CONVENTION.md` §1c. When Bob holds a **known play** and
Cathy does not, a clue to Bob is REACTIVE with Cathy reacting and Bob receiving,
and a clue to Cathy is stable. Bob's target is the next playable in his hand
under stack simulation with every known play in it assumed already played, and a
card already stamped CTP is never retargeted.

Implemented in `src/conventions/tiiah/interpret_reactive.cpp`: the target walk
(direct plays, then one-away finesses, each leftmost-first, over the stacks after
the receiver's known plays), the sum rule for the reacter's slot, and the §1g
split between a shared refusal (walk on) and a giver-only one (reject).

What the two cards ARE — the bucket relation, the finesse inference and the
"both know exactly" licence — followed in §40, v16.3.0.

---

## 40. `[tiiah]` The bucket-encoded reactive — CLOSED in v16.3.0

CONVENTION.md §1d. All reactive clues are even parity; the sum rule picks the
slots as in reactor0 and the clue KIND says what the cards are — rank: a finesse,
or the receiver's target one bucket higher than the reacter's (wrapping), or both
players knowing their identity exactly; colour: the same with one bucket lower.
Inverted playables and inverted finesses are skipped as targets unless they are
the only playables left, where the clue becomes a double chuck.

Implemented in `src/conventions/tiiah/interpret_reactive.cpp`: the bucket
relation gates the target walk and narrows the reacter's card to the playables
of the named bucket (judged after the receiver's queued plays, so a delayed play
counts); a finesse names its connector outright; a pairing that is neither, and
that the two players could not name from their own empathy, is walked past. The
double chuck presses the Discard button on both sides and asks only that the
reacter's card be affordable to chuck (`safe_to_chuck`).

The resolution side needed no new code: reactor0's `wc_even_parity` reads the
`even_parity` bound at clue time and `receiver_button` mirrors the button the
reacter actually pressed, which is what makes a double chuck resolve as a chuck.

---

## 41. `[tiiah]` Superposition — CLOSED in v16.1.0

`ConvData::superposition` holds the candidate set, stamped from `common` by
`note_hidden_action` and narrowed by `collapse_superpositions`
(`src/conventions/tiiah/superposition.cpp`). `State::common_play_stacks` is the
shared view the interpretation rules read; `play_stacks` stays our own belief.
CONVENTION.md §1.3 and §1e.

The two traps this entry recorded were both avoided by keeping the set on
`Game::meta`, which `rewind` restores from `base.meta` and `apply_snapshot`
rebuilds by replay — no new `Game` field, so nothing is carried through a
rewind stale and nothing needs serialising.

**Still open, and inherited by §43:** the reacter's target choice under §1e —
"assume none of the superposed cards were played". §40 gave it something to
read, and it reads the wrong view.

---

## 42. `[tiiah]` Rainbowy colour pinning, and stable clues from a superpositioned giver — CLOSED in v16.4.0

CONVENTION.md §1f. In a rainbowy variant a non-orange colour stable clue pins the
CTP to the next playable of that colour's OWN suit rather than a superposition of
it and the rainbowy suit, unless that is immediately impossible, in which case it
re-pins to the rainbowy suit's next playable. And a stable colour clue from a
superpositioned giver is read under §1e's assumption that none of the superposed
cards were played.

16 of the 44 TIIAH variants carry a rainbowy suit (Rainbow, Omni, Prism, Muddy,
Cocoa).

`pin_rainbowy_colour` (`src/conventions/tiiah/interpret_clue.cpp`) narrows the
new call to one identity once reactor0's ladder has chosen which card it sits
on, re-pinning to the rainbowy suit when the clued colour's own suit has no next
playable. The superpositioned-giver half needed no rule of its own: the whole
stable reading now runs on the shared view (§43), and a superposed play never
advanced it.

---

## 43. `[tiiah]` The reactive target walk runs on the wrong stack view — CLOSED in v16.4.0

CONVENTION.md §1d/§1e. `receiver_targets` simulates the receiver's queued plays
on top of `play_stacks` -- OUR belief, which a partner's superposed play has
already advanced because we watched the card go in. Their own view has not
advanced, and neither has the third seat's if they also could not name it. So
from the first ambiguous play onwards the three seats can walk to different
targets, which is a desync in the one thing every seat has to compute alike.

`common_play_stacks` is by construction "no superposed play counted", so the fix
is to run the walk on it: a `State` whose `play_stacks` are the shared ones,
with `playable_set` rebuilt to match (`with_play` maintains it; a bare
assignment would leave it stale, and `pitch_candidates` reads it).

Inherited from §41, unblocked by §40, and closed in v16.4.0 with §42's other
half of the same rule.

`State::shared_view` (`src/basics/state.cpp`) is that state, with `playable_set`
and `trash_set` rebuilt to match. `stacks_after_queued_plays` starts there, so
the walk is the same at every seat; the stable ladders reach it through
`SharedStacks`, a scoped swap around the delegation in `tiiah::interpret_clue`
that costs nothing until the two views actually differ.

Still on our belief, and left there deliberately: `common.hypo_stacks` and
`Player::hypo_stacks`, which the elim layer rebuilds from `play_stacks` outside
the swap's reach. They feed delayed-play chains rather than the call itself.

---

## 44. `[tiiah]` A suit nobody can account for stays unplayable to every reader

CONVENTION.md §1.3. A pairwise row advances only through cards that row has seen,
in order, so a play the row never saw blocks every play above it. That is the
correct model of what its seat believes — but it means a suit whose LOW card was
thrown in the hole by the one seat that would have needed it can never come back,
because no reader can walk past the hole.

Replay [2008489](https://hanab.live/shared-replay/2008489) T52 is the cost. 23
cards are down, only the `y5` and the `p5` are left, and a rank-1 clue to
yagami_black plays both. will-bot69 chucks instead. Purple is the blocked suit:
`p1` was will-bot67's own blind play and `p2`/`p3`/`p4` were yagami's, so the row
will-bot69 holds for yagami sits on 2 and the `p5` reads as two away — not a
target the reactive walk will take, at any seat. Every seat's belief is right
(purple is on 4); no seat can prove it to the reader.

Two candidate fixes, neither taken in v16.12.0:

- **Widen the back-solve.** §1e's back-solve recovers our own hidden plays from a
  call we can see the answer to (`back_solve_own_plays`,
  `src/conventions/tiiah/superposition.cpp`). It only fires on a STABLE call, and
  only for the suit that call named. A discard that reveals a card the stacks
  cannot explain is the same kind of evidence and is not read.
- **Lean on §1d's `both_know_their_own` licence.** By T52 each of the two holders
  can name their own card from empathy alone, which is exactly the case §1d
  already allows a pairing to rest on. The walk rejects the pairing before that
  licence is ever consulted, because `receiver_targets` filters on playability
  first — and as of **v16.15.0** the licence moved further out of reach, since it
  now sits inside the GIVER-only legality gate rather than in the walk. Waking it
  means relaxing `receiver_targets`' playability filter, which is the shared half
  of the walk and therefore the part every seat has to agree on. That is a bigger
  change than this entry first assumed.

The second is the smaller change and the one that matches the ruling the game was
played under ("both the reacter and receiver know exactly what they are playing").

**v16.18.0 added a third route and it does not reach this case.** A row now takes
what holds in every *surviving* world of its own seat's hole cards, so a row CAN
now walk past a hole — but only when the no-strike argument forces the identities,
and yagami's `p2`/`p3`/`p4` sets are far too wide for that. Verified:
`build/replay_log.exe logs/will-bot69-2008489.log --turn 52 --rerun` still chooses
`discard(order=46)`. The entry stands as written.

---

## 45. `[tiiah]` The receiver's bucket narrowing is wired only into `interpret_play`

CONVENTION.md §1d. `tiiah::narrow_receiver_call` — the receiver's half of the
bucket relation — is called from one place, `Game::interpret_play`
(`src/basics/decide.cpp:650-658`). So it is skipped whenever the reacter's action
reaches a seat as a **discard**, even though the reaction machinery below it gets
the button right: `reacter_button_pressed`
(`src/conventions/reactor0/interpret_reaction.cpp:530-540`) already knows that a
plain card can only reach a strike via the Play button, and stamps the receiver
`CALLED_TO_PLAY` accordingly. Only the narrowing is missing, so the receiver keeps
the generic "every playable the stacks allow" reading.

Two paths still reach it after v16.16.0, which removed the third (a *presumed*
strike):

- a **genuine** misplay by the reacter — it pressed Play, the card was dead, and
  the clue's meaning is unaffected by the outcome;
- a **pitch** — Play on an inverted card, which reaches the engine as a discard by
  design (§1.1's table).

Deferred deliberately while v16.16.0 fixed the cause of replay 2010296 rather than
this symptom of it. The fix is the shape v16.11.0 used for `fire_reaction_elim`:
the wire button, not the resolved action type, is what a reaction is about.

The alternative worth weighing first is moving the hook into
`reactor0::resolve_reaction`, which already has the button in hand — one site that
cannot drift, at the cost of shared code depending on tiiah for the buckets, which
the comment at the seam (`decide.cpp:636-640`) deliberately avoided.

---

## 46. `[tiiah]` `ReactorWC::clue_play_stacks` serves two frames, and can only be one

CONVENTION.md §1.3, §1d. As of v16.18.0 the field carries the stacks the GIVER and
the RECEIVER share (`tiiah/interpret_reactive.cpp:218`), because its main consumer
is the receiver's promise: `reactor0::stamp_receiver_call` rewinds onto it to decide
what the called card may be (`reactor0/interpret_reaction.cpp:365-390`).

The deferral's Rule 3 reads the same field to ask a different question — was the
REACTER's card playable at clue time (`reactor0/interpret_reaction.cpp:702-716`) —
and that one wants the giver-and-reacter pair, the view the target walk already
uses (`tiiah/interpret_reactive.cpp:247`). One field cannot be both, and today the
deferral rule reads the receiver's frame.

It has not been seen to cost anything: the two rows differ only once a seat has
thrown a card in the hole that the other of the pair can name and the third cannot,
and Rule 3 only fires on a DEFERRED reaction whose reacter successfully advanced a
stack. The fix is a second field on the WC plus its snapshot round-trip
(`src/logging/state_snapshot.cpp:425`, `:445`), which is more surface than the bug
currently justifies. Outside TIIAH the question does not arise — reactor0 sets the
field to `play_stacks` and every seat holds the same ones.

---

## 47. `[tiiah]` Our own belief takes no floor across the surviving worlds — CLOSED in v16.19.0

CONVENTION.md §1.3, §1e rule 6. v16.18.0 gives a PARTNER's row the height that
holds in every surviving world of that partner's hole cards
(`advance_rows_from_own_worlds`). Our own belief gets the narrowing half of the
same rule (`presume_own_plays_land` prunes our sets and settles singletons) but not
the floor: two hole cards of ours over `{g1,b1}` must have been one of each, so
green and blue are both on 1, and `State::play_stacks` does not know it.

Why it was left: the two cards cannot be settled INDIVIDUALLY — neither is pinned,
only the pair is — so advancing the stacks would have to be done without settling
either, and `settle` would then double-count the copy if one of them was named
later. Expressing "these two are a `g1` and a `b1` in some order" needs a joint
superposition the data model does not have.

It is a missed deduction rather than a desync: a row is symmetric by construction
(each seat of a pair enumerates the same two seats' hole cards from the same base),
so the frame a clue is READ in is unaffected. What suffers is our own decisions,
which run on `play_stacks`.

**That last paragraph was wrong, and replay 2010512 is the counterexample.** §1.3 gives
the reacter its OWN stacks to read what its card is, so a belief below the floor is not
only a decision problem: will-bot67's `play_stacks` was two plays short of what it
could prove, the receiver's `y2` therefore looked one away, the pairing read as a
finesse demanding a `y1` the reacter's slot could not be, and the call died where it
was stamped.

Closed in **v16.19.0**, and the double-count objection above dissolved rather than
being solved: `presume_own_plays_land` raises the stacks with `State::with_stacks`
instead of `with_play`, so no copy is booked as spent and a later collapse that names
one of the cards still books it exactly once. The accounting lags the stacks by design,
which only ever under-eliminates.

---

## 48. `[engine]` `common_play_stacks` is not the same at every seat

CONVENTION.md §1.3 defines the shared view as what every seat knows every seat
knows, so the vector must be identical at all three seats. In replay
[2010329](https://hanab.live/shared-replay/2010329) it is not: at the same moment
will-bot67 holds `[1,0,0,0,0]` and will-bot69 holds `[0,0,0,0,0]`.

The cause is upstream of the view. `note_hidden_action` advances it when the player
could NAME the card they played, which it asks as
`only_one(common.thoughts[order].possibilities())` — and `Game::common` is not
actually seat-independent under this convention. Several TIIAH rules write a
seat-specific reading into it: `narrow_receiver_call` takes the reacter's card from
`prev.state.deck[react_order].id()` when this seat watched it and from the common
inference when it did not, and `repin_own_call` re-pins a call on the holder's own
belief. Both are deliberate (§1.3's "one sanctioned disagreement"), and both leak
into the singleton test.

Until this is settled, any rule keyed on `common_play_stacks` can mean two things
at two seats. That is the reason v16.12.0 moved clue reading onto the *pairwise
view* and v16.18.0 moved the reactive's promise there too — each of those is a step
away from depending on this vector at all, which may be the real fix.

**v16.19.0 removed one of the contributors, and it was a large one.** The receiver of a
reactive never narrowed the reacter's blind play, so at that seat the play never
advanced `common_play_stacks` while at every other seat it did — a guaranteed divergence
on every reactive the table reads, and the whole of the gap in replay 2010512 (purple 0
against purple 2). What is left there is the genuinely two-wide case: will-bot67's own
`{r1,y1}` pair is two candidates for everyone, so its red 1 is knowledge only it and
the seats watching hold, and `common_play_stacks` is right to lag. Whether any
divergence remains that is NOT of that kind is the open question.

---

## 49. `[tiiah]` A finesse pairing is read wider than it needs to be by the receiver

CONVENTION.md §1d. `narrow_reacter_play` (v16.19.0) reconstructs what the reacter
knows about its own blind play as *the playables of the bucket of the identity the
receiver saw*. That is exact for a direct pairing. For a **finesse** it is not: there
the reacter was told its card outright — it is the connector — while the bucket may
hold more than one playable.

The receiver cannot tell the two apart, because which it is depends on how far off its
OWN target was, and the receiver cannot see its own hand. So it takes the bucket set,
which is a superset of what the reacter actually knows.

The cost is only that the shared stacks lag: a set of two where one would do keeps
`common_play_stacks` waiting a turn or two longer. It never over-claims, which is the
direction that matters. Closing it means the receiver reconstructing its own target
from the sum rule first — `calc_target_slot` already does exactly that at reaction
time (`reactor/interpret_reaction.cpp:27-44`), so the ingredients are in hand; it was
left out of v16.19.0 to keep one mechanism per version.

---

## 50. `[tiiah]` The world cap abandons a whole floor rather than dropping one holder

CONVENTION.md §1.3, §1e. `open_worlds` returns a single flat world when the product of
the superposition sizes exceeds its 64-world cap, which is right for a *reading* — a
partial enumeration is a conditional set missing some of its own conditions. But
`advance_rows_from_own_worlds` enumerates two seats' hole cards together, and there the
all-or-nothing fallback means one seat's wide sets can destroy a deduction that rests
entirely on the other's narrow ones.

Replay 2010512 at v16.18.0 is the shape: yagami's two hole cards carried 20 and 24
candidates, so `20 × 24 × 2 × 2 × 3 = 5760` blew the cap and will-bot67's row for
yagami lost the `r1`/`y1` pair it could otherwise prove. v16.19.0 fixed that game by
resolving yagami's cards instead — they should never have been wide — so the cap is no
longer the reason anything is lost there, and the fragility was left standing.

The fix is to choose the holder list before enumerating: take each holder's own product,
greedily accept them smallest-first while the running product fits, and enumerate
whoever fits rather than giving up. Dropping a holder is conservative in one direction —
an excluded card can neither advance a stack nor strike — and it over-claims only for a
card that strikes in *every* world, which a 20-candidate set cannot do.

---

## 52. `[engine]` A shared call is judged dead on a PRIVATE view

`reactor0/call_invariants.cpp:149-206`. Rule 3, `drop_dead_play_calls`, erases a
standing CALLED_TO_PLAY when common knowledge can see the card is dead. Its own comment
states the contract: *"Common knowledge only. The holder's own view may be narrower,
but a call is a shared commitment and has to die for every seat at the same moment, or
they disagree about what is still standing."*

**It then reads `pitch_candidates(game.state)` — our own BELIEF.** Outside Throw It in a
Hole that is the same vector at every seat and the contract holds by accident. Inside
it, beliefs differ by exactly what each seat threw in the hole, so the rule does the
thing its comment forbids.

Replay [2010512](https://hanab.live/shared-replay/2010512) is the cost, and it is the
game's only strike. will-bot69's order 12 was called reading `{y2,p1}` with the `p1`
already down. At turn 4 will-bot67 erased the call, correctly on its own belief — red
and yellow on 0, purple on 1, so nothing in `{r2,y2,p1}` was playable. will-bot69 KEPT
it, because its own belief had yellow on 1 and so the `y2` was playable. Judged on the
shared view instead, the call dies at both seats and will-bot69 never plays the card.

Two consequences worth stating together:

- the seats disagree about a standing commitment, which is the failure mode the comment
  names;
- and it disarms §1h's fix clue for this game: the giver no longer believes there is a
  call to fix, so `clue_fixes_dead_call` declines. Verified — the fix clue is
  implemented and correct, and 2010512's strike still happens
  (`replay_log logs/will-bot69-2010512.log --turn 12 --rerun` still plays order 12).

**Narrowed in v16.29.0, not closed.** Rule 3 now also keeps a call whose reading is a
valid pitch in some strike-free world of the SHARED view
(`pitch_candidates_in_shared_worlds`, `reactor0/call_invariants.cpp:128-147`). That half is the same at every seat, so a
call that is live on the team's worlds lives everywhere (replay 2012424). What remains
is the other direction: a call dead on the shared worlds that one seat's private belief
still keeps, as in 2010512.

The fix is `pitch_candidates(game.state.shared_view())` under the hole flag, keeping the
queued-plays union as it is. It is small; what it needs is a corpus run, because rule 3
fires on every action of every game.

---

## 53. `[tiiah]` A negative-touch fix discards the clue's other half

CONVENTION.md §1h. A fix clue SUPERSEDES the ordinary stable meanings, as ruled. For a
fix that TOUCHES the dead card that costs nothing — the touched card is the dead one.
For one that fixes by **negative** touch it costs the rest of the clue: a colour yellow
that names a partner's dead purple by missing it still touches their yellows, and
superseding throws away whatever the ladder would have said about those.

The alternative is the shape §1c's refusal uses — an ENVELOPE, where the signal rides
along and the clue still means whatever stable clue it is. That would get both the fix
and a play clue out of one turn, at the cost of two cases in the rule instead of one.
Ruled as supersede-always for now, deliberately; this entry is the record of what it
gives up rather than a disagreement with it.

---

## 54. `[tiiah]` Rule 6's shared collapse cannot check the condition rule 1 insists on

CONVENTION.md §1e. Rules 1 and 2 count only evidence every seat holds, and they say so
explicitly: *"A play whose own identity was a superposition is not common knowledge —
the player who made it does not know what they played — so narrowing on it would desync
them from the seats that watched it."*

Rule 6's shared form (v16.21.0) rests on the same footing — the seat that made the play
has to know what it played, or it cannot run the argument about OUR hole card — and it
does **not** apply the test. It cannot, and replay 2011133 is exactly why: will-bot69's
own reading of order 23 was `{b2}`, a singleton, so it could run it; but will-bot67's
copy of that same reading was **`{g2,b1}`**, because §1.3 has an observer read the
reacter's card on the pair's stacks while the holder reads it on their own, and
will-bot67's pair view had blue on 0. Applying rule 1's test to our copy of the reading
would therefore have stopped the shared collapse firing in the one game it was written
for.

So the shared form is strictly sound only when the player's own reading was a singleton,
and from outside we cannot always tell whether it was. In practice it is close to always
true for a reactive blind play, which is named by §1d's bucket relation.

The cure is not a test here; it is making our copy of a partner's reading right, which is
what TODO 48 wants. Until then this is a known gap rather than an unknown one.

**v16.23.0 narrows it.** Rule 6 now also has a form asked of the SHARED view
(`known_play_lands_in_common`, CONVENTION.md §1e): whenever a card the whole team could
name lands above the shared stacks, every seat refutes the same worlds from the same
inputs. That covers the watcher who never ran the partner form because the play landed
on its own stacks — replay 2011327, where will-bot67's shared view sat on red 0 for 27
turns while will-bot69's had red 4. What is left of this entry is the private-sight form
itself (`presume_play_lands`, `shared=true`) on a blind play whose reading the seats
disagree about; it is kept because 2011133 needs it. `scripts/tiiah_stacks.py` now shows
any such disagreement directly, flagged `≠`.

---

## 55. `[reactor0]` Rung 3.7 counts known trash as "close to playable"

`DECISION_MAKING.md` 3.7 locks Bob when enough of his cards are close to playable, and
`missing_connectors` gives basic trash 0 missing connectors, so a card Bob already holds
as trash counts toward the lock. Replay 2011327 T22: will-bot67's clued r2 (red on 2)
counted, which helped 3.7 qualify. Since v16.23.0 the Red trash reveal wins at 3.3 before
3.7 is asked, so this no longer decided that turn — but a trash card is the opposite of
close to playable, and a hand whose "close" cards are partly trash is not a hand to lock.
Not changed: it is a rung condition, and would move other turns.

---

## 56. `[tiiah]` World feasibility reads only the ordinary reactive, and only its Play

§1e world feasibility (v16.24.0) keeps a `ReactionRecord` for every reactive play clue
that resolves, and rules out a world whose hole cards would have out-ranked the card the
reacter called. Three things are not recorded yet:

- **The reverse arm.** The receiver moves first and the target walk runs on the stacks
  after the receiver's queued plays, which the receiver cannot name from the deck, so no
  frame every seat agrees on is available. `interpret_reactive` records nothing for it.
- **A deferred reaction.** `resolve_deferred_reaction` resolves it turns later; only the
  live seam in `Game::interpret_play` records.
- **A reactive discard.** Its target walk is the discard-target walk, not the play walk
  `world_feasible` models.

Each is a source of refutations the bot leaves on the table, never a wrong one.

Also noted while building it: replay 2011319's views disagree between seats from its
early turns (`scripts/tiiah_stacks.py 2011319 11`), in v16.23.0 as in v16.24.0. Two of
its three seats are simulated there -- will-bot67's log stops at T13 -- so how much of that
is the simulation and how much the bots has not been separated.

---

## 57. `[tiiah]` The playable-dupe conventions cover the plain cases only

§1j (passback) and §1k (discharge), v16.25.0. What is not handled yet:

- **Discharge on a deferred reaction.** `find_discharge` reads `waiting.front()`, so a
  reaction the reacter deferred and resolves turns later (`pending_reactions`) is read
  as an ordinary discard.
- **Discharge on the reverse arm.** The reverse reactive's receiver moves first; a
  discharge there has not been ruled on and is read by the same code, unexamined.
- **Passback when the other holder is not called yet** -- a card that will be called by
  a reaction still in flight. `dupe_passback` only looks at standing CTP stamps.
- **The rule-2 side of a passback.** Narrowing the other copy to {X} is written straight
  into `common` by `read_passback`; the collapse rules that key on a newly CALLED
  identity (rule 2) do not see it as a new call.

---

## 58. `[tiiah]` The striker does not learn what its own dupe strike was

CONVENTION.md §1e rule 8. Rule 8's shared form (v16.27.0, `strike_was_a_watched_dupe`)
floors every view when a partner strikes on a duplicate whose other copy went into the
hole in front of the striker. It runs at the WATCHERS' seats only: the striker cannot see
the card that struck, so its own model of the shared view stays behind.

The striker can often deduce it. Replay 2011854 T28: will-bot69 read its called o29 as
`{g5,p2}`, both of which would have landed on its own stacks, so the strike contradicted
its reading; on the frame yagami gave the call in (purple 0) the bucket named the p1, and
a p1 can only strike as a duplicate. Reading the call on the giver-and-receiver frame
recorded in its `ReactionRecord`, and taking the one candidate that could have struck,
would give the striker the same fact. Until then, will-bot69's shared view sits one step
behind the other two seats' models of it, which only costs anything when will-bot69 is
the one reading a later clue on that view.

---

## 59. `[tiiah]` Hole-card worlds are replayed on today's stacks, not on the stacks of their turn

CONVENTION.md §1e. `enumerate_worlds` replays each hole card in play order (`hole_turn`),
but onto the base view as it stands NOW -- which already holds every card that landed
since, including plays made long after the hole card went in. A world in which the hole
card was a card that could not have landed at the time then survives, because a later
play has since filled the gap it needed.

Replay 2011887: will-bot69's o9 `{r2,g2}` went into the hole at T5 with red on 0, so the
r2 world is a strike and o9 was the g2. yagami's r1 at T15 is in every later base,
though, so the r2 lands on it and the world stands; at T20 will-bot69 reads its called
o21 as `{g2,g3,b3}` rather than `{g3}`, and its shared views stay behind (0013 against a
true 1223). It still plays the card, so nothing was lost there, but every reading that
rests on o9 is wider than it should be.

The fix is to replay a hole card against the view as it stood at `hole_turn` and then
add what landed after, in order. That needs a per-turn record of what each view knew,
which nothing keeps today.
