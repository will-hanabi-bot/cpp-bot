# PLAN — reactor0's rules-based decision framework (v7.0.0 / v7.1.0)

**Status: proposal for review. No implementation has started.**

This document scopes the replacement of reactor0's tuned-constant decision layer
with the ordered, rules-based procedure specified in
[`src/conventions/reactor0/DECISION_MAKING.md`](src/conventions/reactor0/DECISION_MAKING.md).
**Reactor is not changed by any part of this work.**

---

## 1. Why

Reactor0's decision layer is a fork of reactor's scorer. Every clue is collapsed
into a `double` and compared by a global argmax:

```
get_result            reactor0/state_eval.cpp:249-382   16 tuned literals
clue_branch_value     reactor0/state_eval.cpp:384-389   damping + flat tempo tax
eval_action           reactor0/state_eval.cpp:449-510   gate + advance() lookahead
        ↓
Game::take_action argmax          src/basics/decide.cpp:1132-1137
```

The numbers are not reasonable-about. Replay 1961419 T11 is the worked example
already in the tree: a lock clue that bought nothing beat an explicit called
discard, because one term read `+0.5` and the other `0.0` — and no amount of
reading the code tells you that was going to happen. `TODO.md` currently carries
five separate entries that are all really "this term is missing or mistuned".

The replacement is an **ordered priority list with explicit tiebreaks**: the bot
does the first thing that applies, and the reason it did so is a rule you can
point at.

---

## 2. Staging

| Version | Scope |
|---|---|
| **v7.0.0** | **Phase 1 — clue selection.** The General Clue Evaluation List replaces reactor0's clue scoring. Play/discard selection keeps today's ladder. |
| **v7.1.0** | **Phase 2 — action selection.** The per-player call-tracking structures and the Actionable Card Priority list replace the play/discard ladder for reactor0. |

Staged because the two halves together disturb ~140 reactor0 and decision tests
at once; split, a behaviour change is attributable to one of them.

---

## 3. What is deleted (v7.0.0)

All in `src/conventions/reactor0/state_eval.cpp` unless noted. Every tuned
literal in the removed code, so the scale of the change is explicit:

### `get_result` — `:249-382`

| Line | Constant |
|---|---|
| `:283`, `:293`, `:303`, `:315` | four separate `return -100.0` rejections |
| `:339` | `good_touch = -bad_count` |
| `:341` | good-touch table `{0.0, 0.125, 0.25, 0.35, 0.45, 0.55}` |
| `:345` | `std::min(touched, 5)` clamp |
| `:368` | `playables.size() - 2.0 * duped_playables` |
| `:369` | `0.2 * untouched_plays` |
| `:370` | `(in_endgame ? 0.01 : 0.05) * revealed_trash` |
| `:371` | `(in_endgame ? 0.1 : 0.05) * fill.size()` |
| `:372` | `(in_endgame ? 0.05 : 0.02) * elim.size()` |
| `:373` | `-kBadTouchPenalty * bad_count - 10.0 * destroyed_plays` |
| `:375` | `MISTAKE` → `value - 10.0` |
| `:376` | `FIX` → `value + 1.0` |
| `:380` | `REACTIVE && playables >= 2` → `value += 10.0` |

### `clue_branch_value` — `:384-389`

`:386` `mult = !we_hold_a_play ? 0.5 : (in_endgame ? 0.1 : 0.25)`;
`:388` `result * (result > 0 ? mult : 1.0) - 0.5`.

### `eval_action` — `:449-510`

`:464` `-100.0` on a MISTAKE hypo; `:489-490` the gate window
(`pace() >= 3` and `clue_tokens < 8` / `<= 3`); `:503` the flat `-1.0`
below-tier rejection; `:509` `+ reactor::advance(game, hypo_game, 1)`.

### Tuning constants — `:236-247`

`:240` `kBadTouchPenalty = 0.0`; `:245` `kGoodTouchDiscountsBadTouch = true`,
plus the §2c rationale block `:216-235`.

### The §2b pointless-double-discard filter

`receiver_is_safe` `:514-531`, `is_pointless_double_discard` `:533-559`,
`is_stable_play_clue_for_bob` `:561-576`, `drop_pointless_double_discards`
`:578-613`, and its engine seam `src/basics/decide.cpp:886-888`.

Deleted rather than ported: the new priority 2 **already** requires the discarded
card be trash, a same-hand-dupe or visibly duped, so a pointless double discard is
never proposed in the first place. The filter also depends on
`eval_action(...) > 0.0` at `:596` — a function this change removes.

---

## 4. What is kept

The clue-tier machinery is **rules, not tuning**, and the spec builds on it:

| Symbol | Range | Role |
|---|---|---|
| `alice_provably_holds` | `:60-101` | group ("sudoku") elim — can Alice prove she holds a copy? |
| `has_same_hand_dupe` | `:111-119` | second copy elsewhere in the same hand |
| `at_risk_chop` | `:121-143` | the *endangered chop* predicate |
| `has_playable_chop` | `:151-159` | N5 |
| `NewPlayFacts` / `new_play_facts` | `:164-187` | CTP-transition walk; feeds H2, H3, N3 |
| `has_colour_play_clue_for` | `:194-212` | structural "could Bob colour-clue Cathy a play?" |
| `requires_high_tier` | `:391-403` | the *occupied* test |
| `clue_tier` | `:405-447` | H1/H2/H3(/H4) → HIGH; N5/N3/N2 → MEDIUM; else LOW |

Two edits land inside the KEEP group in v7.0.0: **H1b/H1c** (the new Cathy-chop
conditions) and **H4** (finesse) join `clue_tier`.

---

## 5. The seam

New `src/conventions/reactor0/decision.cpp` + `include/hanabi/conventions/reactor0/decision.h`, exporting

```cpp
std::optional<PerformAction> choose_clue(const Game& game,
    const std::vector<std::pair<PerformAction, Action>>& all_clues);
```

spliced into `Game::take_action` **after** the candidate clue build
(`decide.cpp:859-873`) and **before** the force-play override (`:1057`). When it
returns a value, `take_action` returns it directly, short-circuiting both
`eval_for` argmaxes (`:1086`, `:1135`). When it returns `nullopt`, the existing
play/discard path runs with `all_clues` **emptied**, so reactor0 clues never
re-enter the argmax.

It sits **below** two things that keep their current precedence:

- the endgame fork `:739-758` (`forced_endgame_action`, then `EndgameSolver`) —
  convention-neutral and win-probability based, not magic constants;
- the urgent return `:760`.

`eval_for` (`:632-636`) keeps routing reactor0's **non-clue** actions to
`reactor::eval_action` until v7.1.0 replaces them.

### Precedence, as settled

```
0.  endgame fork                        (unchanged)
1.  H4 clue available                   -> give it
2.  reacter-CTP / reacter-CTD pending   -> action it
      (no inverted suit: reacter-CTP only;
       inverted suit: CTP and CTD equally urgent)
3.  phase 1 — General Clue Evaluation List, tier-gated
4.  phase 2 — actionable card priority  (v7.1.0; today's ladder in v7.0.0)
```

---

## 6. Building blocks already in the tree

The implementation should invent none of these:

| Need | Existing | Where |
|---|---|---|
| reactive vs stable | positional compare `action.target != bob` | `reactor0/interpret_clue.cpp:620-631` |
| two new plays | `new_play_facts(...).count >= 2` | `reactor0/state_eval.cpp:170-187` |
| finesse (H4) | rank Phase B | `reactor0/interpret_reactive.cpp:383-447` |
| double discard | rank Phase C | `reactor0/interpret_reactive.cpp:448-482` |
| reactive lock | `predicts_reactive_lock` | `reactor0/interpret_reaction.cpp:31-47` |
| "does this clue create a play" | `hanabi::playables_result` | `src/basics/clue_result.cpp:177` |
| new touches / fill-ins / elims | `elim_result` | `src/basics/clue_result.cpp` |
| trash touched (default tiebreak) | `bad_touch_result` | `src/basics/clue_result.cpp` |
| stable-colour target, without simulating | `leftmost_could_be_playable` | `reactor0/interpret_clue.cpp:211-231` |
| candidate clue enumeration | `State::all_valid_clues` | `src/basics/state.cpp:212-231` |
| colour-only subset | `State::all_colour_clues` | `src/basics/state.cpp:201-210` |
| chop | `Game::chop` | `src/basics/decide.cpp:432-461` |
| safe discard button (inverted suits) | `discard_button_is_safe` | `src/basics/decide.cpp:938-958` |

---

## 7. v7.1.0 — what phase 2 needs that does not exist yet

The spec's per-player structures (reacter-CTP, receiver-CTP deque, reacter-CTD,
receiver-CTD) have **no analogue** in the tree:

- **There is no per-player convention container at all.** Everything player-scoped
  is derived on the fly by iterating `state.hands[p]` and reading
  `Game::meta[o]` (`include/hanabi/basics/game.h:96`).
- **Reacter vs receiver is not distinguished** anywhere except the single `urgent`
  bool (`include/hanabi/basics/card.h:115`), which collapses reacter-CTP and
  reacter-CTD into one flag with no way to tell them apart.
- **Nothing pops a reacter stamp when the reaction happens.** The reacted card has
  already left `state.hands` by the time resolution runs
  (`game.cpp:453-493` runs `on_play` before `interpret_play`), so its `meta` entry
  is simply orphaned. "The reaction is over" is expressed three different ways
  today, none of them a data structure: the card left the hand; `check_missed`
  wipes a stale `urgent` sibling (`game.cpp:95-117`); `update_turn` clears
  `waiting` (`decide.cpp:378-381`).
- **Dependence does not exist.** Nothing computes whether two queued CTP cards
  could share a suit under their non-global inferences.
- **Ordering is inconsistent.** `signal_turn` is the only timestamp, it is
  *set-once* (`card.cpp:109-114`), it is **absent** on three stamping paths
  (`reactor0/interpret_clue.cpp:530-533`, `reactor0/interpret_reaction.cpp:57-61`,
  `reactor/interpret_clue.cpp:352-356`), and its missing-value convention differs
  across call sites (`99` / `99` / `0` / `-1` / `-1`).

The hooks the new structures must attach to are `react_play` /
`react_discard` (reactor0: `interpret_reaction.cpp:65-97` and `:99-146`, where the
reaction is confirmed at `:71` and `:105`), the stamp sites, and every clear site
(`check_missed`, `erase_call`, `clear_contradicted_call`, the three `Game::elim`
sweeps, the bomb reset at `decide.cpp:246-271`).

**One piece of good news:** `apply_snapshot` reads only `record["replay"]` and
replays the action list (`src/logging/state_snapshot.cpp:409-412`) — it never
reads `meta` or `waiting`. Any new structure that is a pure function of action
history therefore needs **zero serialisation support**; replay rebuilds it.
`tests/test_basics/test_snapshot_roundtrip.cpp:46-54` should gain an assertion for
it anyway.

---

## 8. Test impact

### Rewritten

| File | Why |
|---|---|
| `tests/test_reactor0/test_decision_making/test_pace_clue_gate.cpp` | 11 assertions hard-code the removed `-1.0` (`:81, :95, :112, :127, :243, :261, :279, :298`). They become assertions on the new admissibility predicate. `:145` calls **reactor's** `eval_action` and stays. |
| `tests/test_reactor0/test_decision_making/test_double_discard_filter.cpp` | Its four subject predicates are deleted; rewritten against priority 2's admissibility rule. |

### Survives untouched

`test_clue_tier.cpp` calls `clue_tier` directly (`:51`) — the KEEP path. It gains
cases for H1b/H1c/H4.

### The phase-1 regression corpus — three clue-asserting replays

| Replay | Pins |
|---|---|
| 1957905 | `clue rank 2 → player 1` (orange chuck must be playable) |
| 1942181 | prefers a stable play clue over a double discard |
| 1942330 | a playable chop lifts the clue tier |

**If the new rules pick a different clue in any of these, work stops on that test
and the divergence is reported** — old action, new action, and the rule that
fired — for adjudication before the test is touched.

### Expected inert in v7.0.0

The nine play/discard-asserting reactor0 replays (1942458, 1942525, 1942777,
1957936, 1957942, 1957953, 1959065, 1961419, plus
`test_urgent_skips_known_trash.cpp`) resolve through the urgent and endgame paths,
which phase 1 does not touch. They become the phase-2 corpus for v7.1.0.

---

## 9. Open spec items

These need a ruling before phase-1 code is written. Proposed wording will be added
to `DECISION_MAKING.md`; each is flagged there and here until confirmed.

1. **§4 fallback.** At 8 clue tokens a clue is forced, but §4 lists only 4.1-4.4.
   What is given when none of them is available? (Proposal: the best clue by the
   default tiebreak, ignoring tier.)
2. **Does the `>= N clues**` relaxation apply to 3.6?** It is the only sub-item of
   priority 3 with no explicit clue-count condition.
3. **H1b's viewpoint.** Is "Cathy's chop is playable or critical" judged from
   Alice's full visibility, like `at_risk_chop`, or from common knowledge?
4. **H4 vs a pending reaction when Alice is the *receiver*.** The settled rule
   covers Alice as reacter; does an H4 finesse also outrank a receiver-side call?

---

## 10. Documentation moves (landing with this proposal)

- `src/conventions/reactor0/DECISION_MAKING.md` becomes the **ruling document**
  for reactor0 decision making — tracked in git, citations corrected, ambiguities
  resolved, and carrying `file:line` the way `CONVENTION.md` does.
- `src/conventions/reactor0/CONVENTION.md` **§2 block (`:594-818`)** is deleted and
  replaced by a pointer. `CONVENTION.md` keeps §0-§1i (what clues *mean*).
- `CLAUDE.md` and `README.md` are amended so a version bump defaults to a **minor**
  increment, and so `DECISION_MAKING.md` is named wherever decision-making docs are
  required.
- `TODO.md` is triaged: entries 4 and 11 are made moot by the overhaul (with the
  still-live parts of 4 rescued), and 3b, 14, 18, 19, 22 are re-scoped.

---

## 11. Verification protocol

Each staged commit must show:

```
cmake --build build -j
build/hanabi_tests.exe            # 255   convention-neutral
build/hanabi_reactor_tests.exe    # 121   must be byte-identical — reactor is untouched
build/hanabi_reactor0_tests.exe   #  93
build/hanabi_decision_tests.exe   #  47   run several times
```

`hanabi_reactor_tests` staying at 121/121 is the **primary safety property** of the
whole overhaul: it proves the change did not leak across the convention seam.

Then the reactor0 replay corpus, and for any behaviour change,
`build/replay_log.exe logs/<bot>-<id>.log --turn <N> --rerun` recorded
before/after in the commit message.

Known noise: `hanabi_decision_tests` carries an intermittent segfault at
`Reactor0PaceClueGate.SilentBelowPaceThree` (TODO entry 17, ~2 runs in 6). Compare
the failure *set*, not a single run — and note that this rewrite may rename that
landmark.

### Acceptance criteria carried over from deleted TODO entries

- A chuck that advances an inverted stack must outrank a generic discard **by
  rule** (was entry 11).
- Honouring an explicit called discard must outrank a value-less lock clue **by
  rule** (was the reactor0 half of entry 22).
