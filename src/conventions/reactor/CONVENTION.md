# The Reactor Convention, as implemented

This is the **ruling reference** for how this bot interprets clues and how it
chooses actions **when a game runs the reactor convention**. Where this
document and the code disagree, that is a bug in one of them — every rule
below cites the `file:line` that implements it, so the disagreement can be
settled.

Each convention owns its docs in `src/conventions/<name>/`. The sibling
[reactor0](../reactor0/CONVENTION.md) is the simpler convention. Which one a
game runs is `Game::convention`; see the repo-root README for selection.

Terminology is defined in [GLOSSARY.md](GLOSSARY.md). A high-level overview is
in [README.md](../../../README.md). Convention that is legal but not yet
implemented is tracked in [TODO.md](../../../TODO.md) — this document
describes only what the code does.

Two conventions of the document itself:

- **Slot 1 is the leftmost, newest card.** Draws prepend to the hand
  (`src/basics/game.cpp:378`), so slot 1 is the most recently drawn and the
  highest `order`. "Left" means newer; "right" means older.
- **Alice / Bob / Cathy** are positional: Alice is the player to move (the
  clue giver in most of this document), Bob the next player, Cathy the one
  after that.

Structure:

- [1. Convention](#1-convention) — what a clue *means*.
  - [1a. Base Interpretation](#1a-base-interpretation)
  - [1b. Variant Specific](#1b-variant-specific)
- [2. Decision Making](#2-decision-making) — everything else the bot chooses.

---

# 1. Convention

Every clue is read as exactly one of two families:

- a **stable** clue — read from the clue's own shape (which cards it touched,
  in whose hand), using referential play/discard rules. Self-contained.
- a **reactive** clue — read *jointly with the next player's response*. The
  clue is given to a **receiver**, but a third player, the **reacter**,
  decodes it: the reacter's chosen slot plus the clue's focus slot together
  name the slot the receiver must act on.

The reactive family is what makes this "reactor" rather than a purely stable,
referential convention. The rest of section 1a builds up to it.

## 1a. Base Interpretation

Everything in this section describes **No Variant** (5 normal suits, ranks
1–5, copies 3/2/2/2/1). Section 1b covers how each rule changes elsewhere.

### 1a.1 The interpretation vocabulary

Six terms are used throughout without further comment; the rest are in
[GLOSSARY.md](GLOSSARY.md).

- **critical** — an identity with exactly one copy left undiscarded that is still
  useful. Losing it lowers the maximum achievable score
  (`src/basics/state.cpp:163-167`).
- **loaded / unloaded** — loaded means the player has an obvious playable or a
  known trash, i.e. something safe to do (`obvious_loaded`,
  `player_game.cpp:233-236`).
- **CTP** — `CardStatus::CALLED_TO_PLAY`. **CTD** —
  `CardStatus::CALLED_TO_DISCARD` (`card.h:21-33`). Both name a *button*, not an
  outcome; see the next two terms.
- **pitch / chuck** — a pitch presses the **Play** button, a chuck presses the
  **Discard** button. On a normal suit a pitch reaches the stack and a chuck
  reaches the discard pile; on an inverted suit that is reversed (§1b.5).

`ClueInterp` (`include/hanabi/basics/interp.h:11-23`) enumerates every verdict
the interpreter can reach:

| Interp | Meaning |
|---|---|
| `PLAY` | A referential play clue; some card is now called to play. |
| `DISCARD` | A referential discard clue; some card is now called to discard. |
| `LOCK` | Every card in the hand is stamped `CHOP_MOVED`; nothing is safe to discard. |
| `REACTIVE` | A reactive clue; resolution waits on the reacter's next action. |
| `REVEAL` | An already-clued card became newly actionable. |
| `FIX` | The clue corrects a wrong earlier inference. |
| `STALL` | The clue conveys no instruction; it burns a token. |
| `USELESS` | An empty clue, where the variant permits them. |
| `MISTAKE` | No legal reading. Scores −100 and is filtered out of candidates. |
| `SAVE`, `DISTRIBUTION` | Declared (`interp.h:15`, `:21`) and parsed back from logs (`src/logging/state_snapshot.cpp:93`, `:99`), never emitted. See §1a.8. |

### 1a.2 Focus

Two different notions of "focus" coexist, and conflating them is the most
common way to misread this codebase.

**Reactive focus slot** — `reactive_focus`, `src/conventions/reactor/interpret_clue.cpp:35-72`.
A 1-based slot number in the receiver's hand, used only as the anchor for the
reactive slot arithmetic. In No Variant it is computed as:

> Among the touched cards, take the one with the **highest order** (newest),
> **except** that a touched card in slot 1 is demoted to the back of that
> ordering. Return its slot.
> (`interpret_clue.cpp:41-57`; the slot-1 demotion is the `ka = (a.first == hand[0]) ? -1 : a.first` key at `:50-54`.)

So the reactive focus is "the newest touched card that isn't the just-drawn
slot-1 card" — falling back to slot 1 only when slot 1 is the sole touched
card. If nothing is touched at all, the focus is slot 1 (`:47`).

> **Worked example.** 4-player game (hand size 4). Alice clues Cathy "4",
> touching Cathy's slots 2 and 3. Neither is slot 1, so no demotion applies;
> the newest touched card is slot 2, and the **reactive focus slot is 2**.
> Had the clue instead touched slots 1 and 3, slot 1 would be demoted and the
> focus would be **slot 3**.

**Stable focus** — each stable branch picks its own:

- `ref_play`: there is no single focus. Every **newly-touched** card is mapped
  through `refer(..., left=true)`, and the **maximum** of those referents is
  the play target (`interpret_clue.cpp:285-289`).
- `ref_discard`: the focus is the newest newly-touched card,
  `max(newly_touched)` (`interpret_clue.cpp:361`).
- `trash_push` / `playable_rank`: likewise `max(newly_touched)`
  (`interpret_clue.cpp:461`, `:474`).

`refer` itself (`src/basics/player_game.cpp:23-34`) steps one slot in the
given direction and **skips over already-touched cards**, wrapping around the
hand.

### 1a.3 The stable/reactive dispatcher

`Game::interpret_clue` (`src/basics/decide.cpp:33-227`) decides which family
applies. First match wins:

| # | Condition | Route | Cite |
|---|---|---|---|
| 0 | Clear `urgent` flags the giver was supposed to act on but didn't; drop a pending WC whose reacter is the giver, remembering `was_deferring` | — | `decide.cpp:35`, `:43-48` |
| 1 | A rewind forced `next_interp` | forced reactive or stable | `decide.cpp:59-69` |
| 2 | Empty clue, and the variant allows empty clues | `USELESS` | `decide.cpp:70-71` |
| 3 | **Deferral**: the pending reactive's reacter clued instead of reacting | reactive(Bob) | `decide.cpp:72-77` |
| 4 | **Re-tasking**: a reactive is pending on X, X is this clue's Bob, and the clue is not aimed at X — the newest clue supersedes | reactive(Bob) | `decide.cpp:78-94` |
| 5 | **Stall context**: giver obviously locked, or `in_endgame()`, or `clue_tokens == 8`. If the clue targets Cathy and Bob is unloaded → reactive(Bob); else stable with `stall=true` | stall / reactive | `decide.cpp:95-107` |
| 6 | **Default**: find the reacter — scan players from the giver forward for the first whose previously-obvious playables were *all* invalidated by the clue | see below | `decide.cpp:109-140` |

Rule 4 deserves emphasis: **a player's next action always answers the newest
clue**. If a reactive is pending on Bob and someone then clues anyone other
than Bob, Bob's response decodes the *new* clue, and the old waiting
connection is discarded (`decide.cpp:78-94`; replay 1916791 T27).

The reacter search at `decide.cpp:120-140` carries a **vacuous-truth guard**:
a player with no prior obvious playables trivially satisfies "kept none of
them". That vacuous match is suppressed only when it would name the clue's
own target as reacter while the target isn't Bob (`decide.cpp:133-136`) —
otherwise a reactive-shaped clue would be misrouted to stable and
`ref_discard` would stamp a spurious CTD on the receiver (replay 1899623 T16).

Terminal routes once the search finishes:

| Case | Route | Cite |
|---|---|---|
| No reacter + a `check_fix` hit on Bob | `FIX` | `decide.cpp:155-156` |
| No reacter + target ≠ Bob | stable (with the `bad_stable` escape hatch) | `decide.cpp:157-170` |
| No reacter + target == Bob | reactive(Bob) — degenerate, scores `MISTAKE`, which is how the giver's eval rejects unreadable clue shapes | `decide.cpp:171-178` |
| reacter == target | stable | `decide.cpp:179-180` |
| `check_fix` hit that was in the target's prior playables | `FIX` | `decide.cpp:192-193` |
| otherwise | reactive(reacter) | `decide.cpp:195-196` |

Finally, a post-check: the number of newly-signalled CTPs is counted before
and after `elim()`; if elimination destroyed any of them, the move is
**overwritten as `MISTAKE`** (`decide.cpp:204-223`).

### 1a.4 Stable interpretations

`try_stable` (`src/conventions/reactor/interpret_clue.cpp:424-618`) runs an
ordered chain. The first branch that yields a verdict wins.

**1. Pink-promise gate** (`:443-445`) — variant-only, see §1b.2.

**2. Trash push** (`:447-468`). A **rank** clue with new touches where *every*
identity the clue could touch is basic trash. The focus's `inferred` is
intersected with the trash set and `meta.trash` is set. No status is stamped —
the card is known garbage, and **no play is called**. The convention says this
should instead be read as a referential play clue, as if the trash cards had been
touched by a colour clue; that is not implemented, see
[TODO.md](../../../TODO.md). Note the loop iterates
`variant->touch_possibilities(kind, value)` rather than assuming
`Identity(s, clue.value)`, so e.g. Pink-Fives can't be misread as a trash push
(`:450-458`).

**3. Playable rank** (`:469-510`). A **rank** clue with new touches where every
non-trash touchable identity is currently playable.

> **Reactor0 diverges here as of v3.0.0.** It classifies over what the cards
> the clue actually TOUCHED can hold (`effective_possible_for` per card, plus
> the pink promise) rather than over the variant-wide touch set — see
> `src/conventions/reactor0/CONVENTION.md` §1c. The variant-wide set contains
> an omni suit at every rank, which blocked reactor0's direct-play reading
> entirely in those variants. Reactor keeps the behaviour described here; the
> shared `playable_rank_focus` fix (leftmost, not rightmost) does apply to
> both. The focus is narrowed to
its playable possibilities, `info_lock` is set to that narrowing, and the focus
is stamped `CALLED_TO_PLAY`. Skipped when the focus is *unnecessary* — every
possibility is either basic trash or already visible somewhere
(`:477-486`).

**4. Response-inversion hook** (`:513-519`). If no waiting connection exists
and the clue's target isn't the giver's Bob, a `ReactorWC` with
`inverted = true` is installed. This does not change the current reading; it
arms the rewind described in §1a.6.

**5. FIX** (`:521-525`). `check_fix` (`src/basics/fix.cpp:12-57`) reports a
`FixResultNormal` when a previously-clued card either was blind-playing/CTP'd
but its `info_lock` is now entirely basic trash, or is now known trash after
having been clued and never reset, or when the clue reveals a duplicate.

**6. REVEAL** (`:552-613`), five distinct triggers:
  - a touched card became newly playable in the giver→target chain
    (`find_reveal`, `:552-560`, `:606-607`);
  - no new touches, but the clue creates a new safe action — a playable or a
    known trash (`:562-570`);
  - no new touches, not stalling, but a new connectable appears (`:574-582`);
  - no new touches, connecting through an unknown one-away playable
    (`:585-602`);
  - the brownish trash-reveal rule (`:611-613`, §1b.4).

**7. STALL** — three sites: `try_stable` with no new touches while stalling
(`:572`); `ref_discard` when a chop-touching clue comes from the player
immediately after the receiver (`:340-342`); and `target_play` when the
narrowing came out empty but the target is known trash (`:230`).

**8. Referential play** (`ref_play`, `:276-313`). Taken for **colour** clues,
and for rank clues whose newest newly-touched card is known trash
(`:610-616`). The target is
`max over newly_touched of refer(hand, o, left=true)` — one slot *left* of a
newly-touched card, skipping touched cards (`:285-289`). Rejected if the
target is already blind-playing (`:291`), if it is CTD'd and not currently
playable (`:295-298`), or if it is on an inverted suit (`:309-311`, §1b.5).
The target then goes through `target_play`.

**9. Referential discard** (`ref_discard`, `:317-420`). Taken for **rank**
clues. Two outcomes:

  - **LOCK** (`:334-359`): if the clue touches the **lock slot** — `lock_order`,
    the *lowest* order (oldest, rightmost) unclued card — every card in the
    receiver's hand is stamped `CHOP_MOVED`. If the receiver was already locked,
    this is a `MISTAKE` (`:343`). A lock protects the whole hand; together with
    the referential discard below, which implicitly protects everything it does
    not name, this is how the convention keeps cards alive.
  - **DISCARD** (`:361-419`): otherwise the target is the **first unclued slot
    to the right (older) of the focus** (`:364-371`). It is stamped
    `CALLED_TO_DISCARD`. If no such slot exists, the branch fails (`:371`).

`target_play` (`:132-242`) is the shared "call this card to play" primitive:
it narrows `inferred` to `playable_set ∪ delayed-play connectors`, sets
`info_lock`, and stamps `CALLED_TO_PLAY`. It contains a **chain-consistency
guard** (`:148-192`): if the giver can see that a connector card is *not* the
predecessor the chain requires, it bails with `nullopt` so the caller can try
a different target rather than committing to a striking CTP (replay 1890204).

`target_discard` (`:246-272`) is the counterpart: it filters **critical**
identities out of `inferred` before stamping `CALLED_TO_DISCARD` (`:254-255`).

### 1a.5 Reactive interpretations

#### Roles

- **giver** — `action.giver`.
- **receiver** — `action.target`, the player physically clued.
- **reacter** — the player whose next play or discard decodes the clue.
  Normally **Bob**, the giver's next player; the only exception is the
  reacter search at `decide.cpp:120-140`.

`reacter == receiver` is a legal degenerate case that resolves to `MISTAKE`;
this is deliberate, and is how the giver's evaluation rejects clue shapes the
partners could not read cleanly (`decide.cpp:171-178`).

#### The slot arithmetic

`calc_slot` (`src/conventions/reactor/interpret_reaction.cpp:16-19`):

```cpp
int calc_slot(int focus_slot, int slot, int hand_size) {
  int other = (focus_slot + hand_size - slot) % hand_size;
  return other == 0 ? hand_size : other;
}
```

Equivalently, and this is the rule worth memorising:

> **react_slot + target_slot ≡ focus_slot  (mod hand_size)**, with a result of
> 0 read as `hand_size`.

`calc_slot` is an involution in its second argument, which is why the same
function serves both directions: the giver computes `react_slot` from a
candidate `target_slot` (`interpret_reactive.cpp:275`, `:522`, `:664`), and
the reacter's actual action is decoded back to `target_slot` from the
`react_slot` they chose (`interpret_reaction.cpp:32`). `hand_size` comes from
`kHandSize[num_players]` (`include/hanabi/basics/state.h:29`), not from the
current length of anyone's hand.

#### The parity table

Dispatch is at `interpret_clue.cpp:811-815`.

| Clue kind | Reacter's physical action | Receiver's action | Encoded at | Decoded at |
|---|---|---|---|---|
| **RANK** | play | **play** | `interpret_reactive.cpp:686-689` | `interpret_reaction.cpp:361-363` |
| **RANK** | discard | discard | — | `interpret_reaction.cpp:314-315` |
| **COLOUR** | discard | **play** | `interpret_reactive.cpp:308-311` | `interpret_reaction.cpp:311-312` |
| **COLOUR** | play | discard | `interpret_reactive.cpp:553-556` | `interpret_reaction.cpp:365-366` |

The mnemonic the bot publishes over `/settings`
(`src/conventions/variants/reactive_table.cpp:167-173`):

> **odd plays** = colour clue → one play + one discard.
> **even plays** = rank clue → two plays.

With `/allplays` on, colour clues are promoted to play+play as well
(`include/hanabi/basics/game.h:98-102`; `interpret_clue.cpp:811`).

#### Shared setup: `reactive_context`

`interpret_reactive.cpp:81-203` computes three things before either path runs:

- `possible_conns` — delayed-play connectors from `delayed_plays`.
- `known_plays` — receiver cards whose **strict** common-knowledge identity
  (`thought.id()`, no `infer`, no trash-elim narrowing) is already playable.
  These are excluded from the play-target pool: the receiver already knows to
  play them, so the clue shouldn't be spent re-signalling (v0.37, `:89-104`).
- `hypo_state` — the play stacks advanced through (a) each intervening
  player's obvious play, (b) the receiver's own strictly-identified self-plays,
  and (c) **all of the receiver's pending CTP'd cards**, to a fixpoint
  (`:191-201`). Step (c) is what makes "a play target never stacks on an
  already-queued play" work: the queued card stops being hypo-playable and its
  successor becomes the eligible target (v1.5, replay 1916815).

#### Colour reactive — `interpret_reactive_colour` (`interpret_reactive.cpp:209-567`)

**Phase A — play targets** (receiver plays, reacter discards), `:229-370`.

Pool construction (`:229-270`):
1. Every receiver slot whose visible identity is playable in `hypo_state`,
   excluding `known_plays` and any slot already `CALLED_TO_PLAY` (`:232-244`).
2. Walking **slot-descending**, the **rightmost (oldest) copy** of each
   identity is *primary*; lefter duplicates are demoted to `dupe_targets`
   (v0.33, `:245-264`).
3. Primaries are then ordered **leftmost-first**, with the dupes appended
   right-to-left (`:265-270`).

Each candidate is then vetted (`:272-311`):
- `react_slot` must land inside the reacter's hand (`:273-279`);
- when `looks_stable`, skip if the reacter's slot is known trash and the
  reacter has no obvious playables (`:281-285`);
- skip if the reacter slot's `possible` is **entirely critical** — you cannot
  ask someone to discard a card that must be critical (`:286-289`);
- skip if this would lose an inverted-suit reacter card (`:295-299`, §1b.5).

On commit: `target_discard(react_order, urgent=true)` (`:308-311`), the
receiver's target is narrowed to
`hypo_state.playable_set ∪ delayed-play connectors` (`:336-352`), the target
is stamped CTP (`:356-367`), and `REACTIVE` is returned.

**Phase B — discard targets** (receiver discards, reacter plays), `:372-566`.
Reached only if no play target resolved.

Five candidate pools, tried as a strict cascade (`:474-481`):

| Rank | Pool | Definition | Cite |
|---|---|---|---|
| 1 | `pre_clued_trash` | Slots clued *before* this turn, not known trash pre-clue, but common-knowledge trash *after* it. The clue's whole point is that disambiguation. | `:454-472` |
| 2 | `unknown_trash` | Not in `prev_kt`; either basic trash or duplicated by an *older* card in the same hand. Stable-sorted so **clued** ones come first, preserving leftmost-within-group. | `:373-410` |
| 3 | `known_trash` | Any slot whose visible identity is basic trash. | `:429-436` |
| 4 | `unknown_dupes` | Duplicated anywhere under `infer=true` matching. | `:412-427` |
| 5 | `sacrifices` | Non-critical slots, sorted by `−playable_away×10 + (5 − rank)`. | `:438-452` |

Two ordering rules here have their own regression tests and are easy to get
backwards:

- **Clued unknown-trash outranks unclued** (v1.4, replay 1916813): marking a
  card the receiver was *keeping because it was clued* teaches more than
  marking an unclued one (`:391-410`).
- **Unknown trash outranks pre-known trash** (v1.6, replay 1916888): the
  `prev_kt` guards at `:377`, `:415`, `:441`, `:469` mean a slot that was
  *already* globally known to be trash is case 2.1 — it is targeted only when
  the receiver has no genuinely unknown trash at all.

A **blocking filter** then drops any candidate that is already CTD'd, or whose
identity duplicates an existing CTD'd card in the same hand — double-signalling
the identity would lose it outright (v0.30, `:483-512`). If that empties the
chosen pool, filtered sacrifices are used instead.

Surviving candidates are vetted (`:516-544`): CTP'd targets are skipped unless
the reacter is Bob (`:517-520`); the reacter's slot must not already be an
obvious playable, which would carry no information (`:528-529`); the reacter's
`possible` must intersect `playable_set ∪ possible_conns` (`:530-537`); and
the inverted-suit veto applies (`:540-544`). Commit is
`target_play(react_order, urgent=true)` (`:553-556`).

#### Rank reactive — `interpret_reactive_rank` (`interpret_reactive.cpp:571-872`)

**Phase A — play targets** (both play), `:582-779`. Same pool construction as
the colour path (`:591-624`), plus two extra rules:

- **Older-CTP guard** (`:629-663`). If the receiver holds an already-CTP'd
  card at a *higher* slot index (older) than this target, they will play that
  one first and break the chain — so skip the target. The guard is waived when
  the target is only playable *because of* those pending CTPs
  (`target_needs_pending`, `:651-654`): then the queue enables the target
  rather than blocking it (replay 1892197 T9 vs. 1899623 T7).
- Candidate vetting uses `effective_possible_for` rather than raw `possible`
  (`:672`).

On commit (`:686-778`): `target_play` on the reacter, the receiver's target
narrowed against `hypo_state.playable_set ∪ connectors` — falling back to
`possible` when `inferred` is already empty (v0.37, `:718-731`) — the target
stamped CTP, and the target's identity **removed from the reacter's
`inferred`** (`:768-776`).

**Phase B — finesse**, `:781-867`. A **finesse** is a reactive move in which the
reacter must play a card that *connects with* a one-away-from-playable card in
the receiver's hand.

Reached when no play target resolved. Candidates are receiver slots exactly one
away from playable (`state.playable_away(id) == 1`, `:782-789`). React slots are
then tried in the **fixed order `{1, 5, 4, 3, 2}`** (`:792`), mapping each
through `calc_slot` to see whether a finesse target sits at the resulting slot.

Requirements (`:808-823`): the reacter's slot wasn't already an obvious
playable; its `effective_possible` intersects `playable_set ∪ possible_conns`;
and it contains `prev_id`, the predecessor of the finesse target.

The **POV-invariant abort** (`:824-838`) is the subtle part: if the observer
can see the reacter's actual card and it is *not* `prev_id`, the entire
reactive interpretation returns `nullopt`. There is deliberately **no "try the
next slot"** — the reacter, reasoning from her own POV, will still pick this
slot, so no later iteration could rescue it.

That abort is also what rules out the **bluff**: a rare reactive move in which
the reacter believes they are playing a card that connects with a
one-away-from-playable card in the receiver's hand, but plays a different card.
It is legal convention — the receiver can tell *after* the reaction that their
target is not actually playable, so they mark it one-away-from-playable and chuck
their chop as normal — but this implementation neither initiates nor decodes one,
because `:824-838` is exactly the bluff case and returns `nullopt`. Tracked in
[TODO.md](../../../TODO.md).

**Phase C — orange chop rescue**, `:870-871` → §1b.5.

#### POV invariance

`effective_possible_for` (`interpret_reactive.cpp:54-69`) narrows a card's
`possible` by visibility **from the holder's point of view**: it counts copies
visible in every hand *except the holder's own*. The long comment at `:27-53`
records why — using the computing bot's own POV made the giver and the reacter
reach *different* convention interpretations of the same clue (replays 1892112
and 1884192). Every observer must decode a clue identically, so any check that
feeds a convention decision has to be expressible from common knowledge.

This principle recurs throughout: `target_play`'s reactive path uses
`common.thoughts[target].id()` instead of `state.deck[target].id()`
(`interpret_clue.cpp:208-227`), and the critical-discard filter lives in
clue *selection* rather than inside `target_discard` (`decide.cpp:538-567`).

### 1a.6 Resolving the reaction

When the reacter finally plays or discards, `Game::interpret_play` /
`interpret_discard` (`decide.cpp:272-284`, `:319-327`) route into
`react_play` / `react_discard` (`interpret_reaction.cpp:249-369`).

`calc_target_slot` (`:26-43`) maps the played/discarded order back to a
`react_slot`, through `calc_slot` to a `target_slot`, and validates that the
resulting card is still in the receiver's hand.

- `target_i_play` (`:77-111`) stamps the receiver's slot `CALLED_TO_PLAY` and
  intersects `inferred` with `playable_set ∪ next-ranks-of-obvious-playables`.
  The CTP is stamped **unconditionally**, even when that intersection is empty,
  so delayed chains survive; the narrowing applies only when non-empty.
- `target_i_discard` (`:54-75`) stamps `CALLED_TO_DISCARD` and removes
  `critical_set` from `inferred`, marking `meta.trash` if that empties it.

**The four `elim_*` matrices** (`:115-245`) then run over the receiver's slots
*left of* (newer than) the resolved target. Their shared logic: had the
receiver held a different identity in one of those earlier slots, a *different*
slot would have been the target — so identities inconsistent with the actual
outcome can be eliminated.

| Function | Case | Rule | Cite |
|---|---|---|---|
| `elim_play_play` | rank, reacter played | For each earlier slot, look at the reacter slot it would have mapped to. If that card's `possible ∩ playable_set` is a singleton, keep only that identity among playables; otherwise drop all playables. | `:115-148` |
| `elim_play_dc` | colour, reacter played | Run `elim_play_play` over *all* slots, then drop `trash_set` from earlier slots whose mapped reacter card could have been playable. | `:150-185` |
| `elim_dc_play` | colour, reacter discarded | Drop `playable_set` from earlier slots whose mapped reacter card isn't entirely critical. | `:187-210` |
| `elim_dc_dc` | rank, reacter discarded | Run `elim_play_play` over all slots, then drop `trash_set` similarly. | `:212-245` |

All four skip slots already CTP'd or CTD'd, and the two `_dc` variants
additionally skip a slot when the target was pre-clued and that slot wasn't
(`:166-170`, `:227-231`).

#### Response inversion

When a waiting connection was installed by a *stable* reading
(`inverted == true`, from `interpret_clue.cpp:513-519`) and the reacter's
action is **unnatural**, the bot concludes the stable reading was wrong and
rewinds.

"Unnatural" means (`interpret_reaction.cpp:260-275` for discards,
`:329-336` for plays):
- a **play** of a card that was not among the reacter's known playables; or
- a **discard** while the reacter had obvious playables, or of a card that is
  neither their chop nor known trash.

The response is `game.rewind(wc.turn, InterpAction{ClueInterp::REACTIVE})`
(`:283-284`, `:344-345`), which replays the game from `base` with the earlier
clue forced into the reactive reading. Rewind depth is capped at 4
(`src/basics/game.cpp:585`). When a rewind succeeds the replay has already
processed the current action end-to-end, so the caller must not touch it again
— that is what `react_play` / `react_discard` returning `true` signals
(`include/hanabi/conventions/reactor/interpret_reaction.h:44-49`).

### 1a.7 Rejecting a stable reading: `bad_stable`

`interpret_stable` (`interpret_clue.cpp:757-786`) does not simply trust
`try_stable`. It snapshots the game, runs the stable reading, and — **when the
clue's target is not Bob** — asks `bad_stable` whether that reading is
defensible. If not, it restores, re-simulates the clue, and re-reads it as
reactive with Bob as reacter.

`bad_stable` (`:703-751`) returns true if any of:

1. the reading is already `MISTAKE`;
2. it is turn 1, the clue is a rank clue, and a play-only alternative clue
   exists (`:708-711`);
3. some card anywhere became `CALLED_TO_PLAY` with inconsistent inferences
   that were consistent before (`:714-729`);
4. a newly-CTD'd card in the target's hand is **visibly critical**, or — in
   stall mode — visibly useful while an alternative clue exists (`:731-746`);
5. the reading is `LOCK` and an alternative clue exists (`:748`);
6. in stall mode, the reading is `STALL` and an alternative clue exists
   (`:750`).

`alternative_clue` (`:624-701`) is the "was there something better?" search: a
colour clue whose ref-play target is actually playable and whose new touches
are all useful (or all-possibly-basic-trash), or a rank clue whose ref-discard
target is actually basic trash and whose new touches are all useful.

Note the asymmetry, spelled out at `decide.cpp:157-170`: Cathy **cannot** run
this check on a clue given to herself, because it depends on seeing her own
hand. She reads stable provisionally; if she was wrong, Bob's unexpected
reaction triggers the response-inversion rewind.

### 1a.8 Inherited machinery and where rules hide

- **Machinery from the Python/Scala port that is never set.** These names exist
  in the type system and will mislead anyone reading the code as if they were
  moves this convention plays. Nothing writes any of them:
  - `CardStatus::FINESSED`, `BLUFFED`, `MAYBE_BLUFFED`, `F_MAYBE_BLUFFED`
    (`include/hanabi/basics/card.h:27-32`);
  - the entire `Connection` variant set, including `PromptConn`, `FinesseConn`
    and `PositionalConn` (`include/hanabi/basics/connection.h:20-117`);
  - `Player::valid_prompt` / `Player::find_prompt`
    (`src/basics/player_game.cpp:302-381`) — dead code with **no callers**;
    `find_prompt` only calls `valid_prompt`, and nothing calls `find_prompt`;
  - `ClueInterp::SAVE` and `ClueInterp::DISTRIBUTION` (§1a.1);
  - `good_touch_elim` (`src/basics/player_elim.cpp:319-352`), unreachable because
    `Game::good_touch` is left `false` (`include/hanabi/basics/game.h:97`) — so
    Good Touch draws no inferences here (v0.39, commit `6219f17`) and survives
    only as the clue-scoring term of §2.4 and via `bad_touch_result`.

  The contrast matters: `CardStatus::SARCASTIC` and `GENTLEMANS_DISCARD` **are**
  live (`decide.cpp:307-316`, `game.cpp:522-523`). They are not port leftovers.
- **Some conventional rules live outside the `interpret_*` files** — notably
  the critical-discard clue filter (`decide.cpp:538-567`), most-recent-CTD
  enforcement (`decide.cpp:932-956`), and the force-play override
  (`decide.cpp:978-1042`). They are covered in §2.

---

## 1b. Variant Specific

Variants are loaded from `data/variants.json` (2 426 of them) and classified
by **substring match on suit names** in `SuitType::of_name`
(`src/basics/variant.cpp:149-161`):

| Flag | Triggering substrings |
|---|---|
| `whitish` | White, Gray, Light, Null |
| `rainbowish` | Rainbow, Omni |
| `pinkish` | Pink, Omni |
| `brownish` | Brown, Muddy, Cocoa, Null |
| `dark` | Black, Dark, Gray, Cocoa |
| `prism` | Prism |
| `muddy` | Muddy, Cocoa |
| `inverted` | **Orange** |
| `reversed` | **Reversed** |

Variant-level flags (`include/hanabi/basics/variant.h:77-90`) add
`critical_rank`, `clue_starved`, `special_rank`, `rainbow_s`, `white_s`,
`pink_s`, `brown_s`, `deceptive_s`, `scarce_ones`, `funnels`, `chimneys`.

### 1b.1 Rainbowish (Rainbow, Omni) — colour clues name the slot

**The single biggest change to clue interpretation.** In a rainbow-ish
variant, a colour clue's reactive focus slot is *not* derived from which cards
it touched. It is read straight off a fixed per-colour table:

```
focus_slot = reactive_value_table(variant, hand_size)[clue.value]
```
(`interpret_clue.cpp:59-65`.)

The table (`src/conventions/variants/reactive_table.cpp:63-123`) assigns
slots from the vanilla order **Red=1, Yellow=2, Green=3, Blue=4, Purple=5,
Teal=6**, each taken `% hand_size` (`:78-80`). Colours not in that list (Pink,
Brown, …) take the next free slot, cycling from the previously assigned one
(`:105-114`).

The lookup is keyed by **clue colour name, not suit name** (`:82-100`). This
matters for Ambiguous variants, where several suits collapse onto one colour
and the representative suits are named Tomato/Berry/… — the partner actually
says "Red", so the reactive value must anchor to the colour. *Ambiguous &
Rainbow (5 Suits)* maps `{Red, Blue} → {1, 4}`, not the positional `{1, 2}`.

### 1b.2 Pinkish (Pink, Omni, **Funnels, Chimneys**) — rank clues name the slot

The mirror-image rule: a rank clue's reactive focus slot is the **clue value
itself** (`interpret_clue.cpp:68-70`).

Note the membership test. `includes_pinkish`
(`src/conventions/variants/predicates.cpp:12-18`) returns true not only for
Pink and Omni but also for **Funnels and Chimneys**, because those variants
share the "one rank clue touches several ranks" property. Any rule in this
section applies to them too — easy to miss.

Also pinkish-only:

- **Pink promise** (`pinkish.cpp:18-55`). A rank clue that newly touches the
  receiver's **lock slot** — the oldest unclued card (`:30-37`), *not* the chop —
  promises that card has that rank. If the observer can see it and the rank
  doesn't match, the whole stable reading is aborted before any branch can
  stamp a partial interpretation (`interpret_clue.cpp:443-445`). With
  `pink_s`, a special rank of 5 also permits a spoken 4, and a special rank of
  1 permits a spoken 2 (`pinkish.cpp:45-49`).
- **`apply_rank_promise`** (`pinkish.cpp:57-68`) narrows the promised card's
  `inferred` to the clued rank; used by both the lock path
  (`interpret_clue.cpp:346-348`) and the referential-discard path (`:382-385`).
- **`playable_rank_focus`** (`pinkish.cpp:70-82`) replaces the usual
  `max(newly_touched)` focus for the playable-rank branch with the
  **minimum-order** (rightmost, oldest) newly-touched-and-previously-unclued
  card (`interpret_clue.cpp:471-473`).
- **Blocked ranks** — with `pink_s`, `brown_s`, or `deceptive_s`, the special
  rank cannot be used as a clue value at all (`state.cpp:212-231`), and the
  `/settings` table renders those slots as `-`
  (`reactive_table.cpp:35-38`, `:156-165`).

### 1b.3 Reversed suits — direction flips

Reversed suits play 5→4→3→2→1. The convention layer needs almost no changes
because it only ever calls direction-aware helpers: `is_basic_trash`,
`is_useful`, `playable_away`, `played_count`, `max_played`
(`include/hanabi/basics/state.h:97-138`). Copy counts flip to `{1,2,2,2,3}`
(`variant.cpp:186-190`).

What *does* change is the decision-making gate of §2.5:
`is_first_or_second_rank` becomes ranks 5 and 4, and `is_clue_regain_rank`
becomes rank 1 (`include/hanabi/conventions/variants/reversed.h:14-25`, used
at `state_eval.cpp:137-140`).

`reversed` is orthogonal to `inverted` — a suit could in principle be both
(`include/hanabi/basics/variant.h:32-35`).

### 1b.4 Brownish (Brown, Muddy, Cocoa, Null) — trash reveal

`brownish_trash_reveal` (`src/conventions/variants/brownish.cpp:19-40`): a
**rank** clue to an **unloaded** target that does **not** touch their newest
slot, in a game where some brown suit still has cards left to play, is read as
`REVEAL` (a trash reveal) instead of falling through to `ref_play`
(`interpret_clue.cpp:611-613`).

### 1b.5 Inverted suits (Orange, Dark Orange) — the buttons swap

This is the most invasive variant in the codebase. For an inverted suit the
*game rule* swaps what the two buttons do: a **pitch** (press Play,
`PerformPlay`) sends the card to the discard pile and regains a clue, and a
**chuck** (press Discard, `PerformDiscard`) is a play attempt onto the stack
(`src/basics/game.cpp:228-247`, `:312-326`).

Critically: **`CALLED_TO_PLAY` and `CALLED_TO_DISCARD` name buttons, not
outcomes** (`decide.cpp:692-708`). CTP means *pitch*, CTD means *chuck*, and the
game rule decides where the card lands. So to get an orange card onto its stack
the convention must stamp **CTD**.

A third orientation lurks on the wire. The **server reports outcomes** — an
orange chuck of a playable card arrives as `type: "play"` — while the engine's
`on_play` / `on_discard` are button-oriented, so `orient_action_for_engine`
(`src/basics/action.cpp:56-72`) flips inverted-suit actions before dispatch
(`src/net/commands.cpp:393-397`). An inbound `type: "play"` on an orange card
therefore means the player *chucked* it.

Every place the convention compensates, all in
`src/conventions/variants/inverted.cpp`:

| Helper | Role | Cite |
|---|---|---|
| `is_inverted_id` / `target_is_inverted` | Is this identity / this card on an inverted suit? | `:21-29` |
| `called_focus_status` | Return CTD instead of CTP when the focus could be orange. | `:53-61` |
| `would_lose_inverted_reacter` | Reject a candidate whose resolution would `target_play` an orange reacter card — that would *pitch* it into the discard pile, losing the copy for nothing. Also rejects `target_discard` on a *non-playable* orange, which would strike. | `:31-51` |
| `orange_chop_save` | Rank-reactive Phase C. Only fires when the receiver's chop is orange; encodes "receiver **pitches** their chop" — a clean voluntary loss that avoids the misplay strike a chuck of a non-playable orange would cause. Non-orange chops deliberately bail, because the observer can't run the critical check on their own card. The chop here is `Game::chop`'s positional fallback: `:91-96` scans for the newest unclued status-`NONE` card. | `:85-148` |
| `make_discard_for_simulation` | In `advance()`'s simulation, an inverted non-playable card must use `failed=true`, or `with_play` would jump the stack to a non-playable rank and corrupt the simulated state. Since v5.0.0 `advance`'s **play** branch also routes a playable orange through it, so a chuck advances the stack instead of being simulated as a pitch (§2.6). | `:63-71` |
| `discard_advances_stack`, `possible_has_inverted` | Used by the chuck-safety filters of §2.3. | `:73-83` |

In both reactive paths, `target_is_inverted(target)` **swaps the reacter's
intended action** so that the receiver's standard reading of
(clue kind + reacter action) still lands on the physically correct move
(`interpret_reactive.cpp:308-311`, `:553-556`, `:686-689`, `:854-857`).
Stable `ref_play` simply refuses orange targets (`interpret_clue.cpp:309-311`),
so that the giver's eval scores the clue as a `MISTAKE` and prefers the rank
clue that reaches the same card through `ref_discard`.

**Reactor0 diverges here as of v4.0.0** — a cross-version compatibility break,
so a reactor player and a reactor0 player read an orange clue differently.
Reactor keeps everything in this section unchanged; reactor0 replaces the
stable side with its own ladder (pitch vs chuck selected by `pace()` and by
whether the inverted suit is dark, plus a giver-side reject when the chuck
target is visibly unplayable) and, since **v5.0.0**, routes a rank direct play
clue through `called_focus_status` only when *every* useful identity of the
rank is orange — otherwise `CALLED_TO_PLAY`, where reactor calls the helper
unconditionally. See
[reactor0's §1b and §1f](../reactor0/CONVENTION.md). The **reactive** swaps
above are still shared by both conventions.

The endgame solver is convention-neutral and now knows about the swap in three
places: `possible_actions` emits a stack-advancing orange as a
`PerformDiscard` (`src/endgame/solver.cpp`), `perform_to_action` derives the
`failed` flag instead of hardcoding `false` (so a chuck of a non-playable
orange models as a misplay rather than a stack jump), and the direct-win /
tie-break predicates count an orange chuck as a play. Before v4.0.0 the solver
enumerated every orange play as a `PerformPlay`, i.e. as a pitch into the
discard pile, so any line needing an orange play scored as a loss
(bug_report_3.txt 3.2, replay 1942723 T42).

### 1b.6 Clue-touch rules

`Variant::id_touched` (`src/basics/variant.cpp:199-246`) determines which
cards a clue touches, which upstream of everything else changes what "newly
touched" means:

- **Colour**: rainbowish → always touched; whitish → never; prism →
  `(rank − 1) % num_colours == value` (`:211-213`); with `special_rank`,
  `rainbow_s`/`white_s` override that rank's behaviour; otherwise match by
  clue-colour name, which handles Ambiguous (several suits, one colour) and
  multi-colour suits like Lime = Yellow + Green (`:218-226`).
- **Rank**: pinkish → always; brownish → never; with `special_rank`, `pink_s`
  means "touched by every rank except its own", `brown_s` means no rank
  touches, `deceptive_s` means `(suit_index % 4) + offset == value`
  (`:235-238`); then **funnels** (`rank ≤ value`) or **chimneys**
  (`rank ≥ value`) (`:243-244`); otherwise exact rank match.

### 1b.7 Other variant flags

| Flag | Effect | Cite |
|---|---|---|
| `dark` suits, `critical_rank` | `card_count == 1` — every such card is critical. | `variant.cpp:183` |
| `scarce_ones` | Rank-1 count is 2 instead of 3. | `variant.cpp:184` |
| `clue_starved` | Playing a 5 returns half a clue token. | `src/basics/state.cpp:127-133` |
| Ambiguous | Several suits share a clue colour; see §1b.1. | `variant.cpp:114-128`, `:218-226` |

### 1b.8 Unsupported variants

`variant_from_json` (`src/basics/variant.cpp:300-324`) reads only the flags
listed above. The following fields present in `data/variants.json` are
**never read**, so those variants will load and be played with **incorrect
rules**:

`upOrDown`, `sudoku`, `synesthesia`, `alternatingClues`, `oddsAndEvens`,
`throwItInAHole`, `cowAndPig`, `duck`, `colorCluesTouchNothing`,
`rankCluesTouchNothing`, `stackSize`, `clueRanks`.

Note in particular that **"Up or Down" is not supported**; the separate
`Reversed` suit family (§1b.3) is.

---

# 2. Decision Making

Section 1 governs what a clue *means* — a question with a single correct
answer that every player must reach identically. This section governs what the
bot *chooses to do*, where there is no single correct answer, only better and
worse play.

## 2.1 The `take_action` ladder

`Game::take_action` (`src/basics/decide.cpp:650-1103`). Each stage that
returns short-circuits the rest. It scores actions through the convention seam
`eval_for` (`:628-634`), which routes reactor0 games to
`reactor0::eval_action` and everything else to `reactor::eval_action`.

| Stage | What it does | Cite |
|---|---|---|
| 0 | **Compute** (not yet return) the urgent action: the first card in our hand with `meta.urgent`, converted to a Play or Discard. Guarded by empathy sanity checks — never play a card whose every possibility is basic trash, never discard one whose every possibility is critical. | `:605-662` |
| 0b | **Urgent Bob-protection override.** If we can clue, a reactive is pending with us as reacter, the receiver isn't Bob, Bob is unloaded, and Bob's chop is *actually* critical from our full visibility → replace our urgent action with the best clue to Bob. | `:615-640` |
| 1 | **Endgame fork**, when `rem_score() <= num_suits + 1`: first `forced_endgame_action`, then the endgame solver. | `:664-684` |
| 2 | **Return the urgent action.** Note the ordering: the endgame solver *outranks* the convention's urgent signal. | `:686` |
| 3–5 | Build the candidate lists: plays, clues, discards. | `:688-820` |
| 6 | Discard gating. | `:822-946` |
| 7 | **Force-play override.** | `:948-1021` |
| 8 | **Global argmax of `eval_action`** over all clues + plays + discards. | `:1023-1039` |
| 9 | Fallback: play slot 1 at 8 clues, else `locked_discard`. | `:1028-1031` |

One rule worth isolating: **the bot does not clue while it is the receiver of
a pending reactive** (`can_clue_now`, `:782-783`). Its job that turn is to
answer the clue it was given.

## 2.2 Choosing a play

Candidates are built by three mutually exclusive branches (`:688-780`):

1. **Connector-first** (`:692-726`). If we are the receiver of a pending
   reactive and hold a common-playable whose successor sits in the reacter's
   hand, the play set is exactly that one card — lowest `signal_turn` wins.
2. **Known playables** (`:727-777`), starting from `obvious_playables` and
   then:
   - drop non-CTP cards that have a same-`possible` focused duplicate in our
     own hand (`:733-741`);
   - **definite-singleton filter** (`:750-759`): with more than one candidate,
     prefer only those whose inferred identity is a definite singleton
     playable on the current stacks — this stops an ambiguous `{b2,n3}` CTP
     from being played ahead of an empathy-known prerequisite;
   - **signal-turn tiebreak** (`:769-777`): still more than one → lowest
     `signal_turn`, with `value_or(0)` so unsignalled empathy-plays run ahead
     of convention-marked ones.
3. **Fallback**: `thinks_playables` (`:779`).

There is **no explicit "play 5s first" rule**. Rank enters the play score only
weakly, through `0.02 * (5 - rank)` (`state_eval.cpp:524`) — which actually
prefers *low* ranks — and through `eval_game`'s future-value term, where a
CTP'd 5 is worth `+0.8` against `+0.4` for anything else
(`state_eval.cpp:641-642`).

**Force-play override** (`:948-1021`) is the strongest "just play it" rule: if
no available clue creates a playable anywhere, *and* Bob is safe (he has known
trash, or is locked, or his chop is visibly non-critical, `:970-980`), return
the best play immediately without comparing it against clues. It is skipped
when the card's identity is already CTP'd or visibly held elsewhere
(`:989-1015`).

In the endgame the solver breaks all win-rate ties toward plays
(`src/endgame/solver.cpp:35-38`, `:336-337`, `:853-856`).

## 2.3 Choosing a discard

**Suppression** (`:843-845`): no discard candidates at all when
`clue_tokens == 8`, when `pace() == 0` and any clue or play exists, or when
`potential_forced_play` — we hold a play whose successor is visibly in the
pending reacter's hand, so discarding would break the chain (`:822-841`).

**`chop()`** (`:404-431`): an explicitly CTD'd card wins — the one signalled most
recently by `signal_turn`, with hand order breaking ties (v1.11.0); otherwise the
**newest** unclued card with status NONE, gated by `zcs_turn` so cards drawn after
the team hit zero clues are excluded. Any non-NONE status disqualifies a card, so
a locked hand has no chop. An earlier CTD may be a sacrifice while a later one is
not, and never the reverse, which is why the newest *signal* wins; this also makes
`chop()` agree with the most-recent-CTD filter below rather than disagree with it.

**A player always chucks their chop**, except when its inference is a known
orange card, which is **pitched** instead (`:934-943`). Note the "except" is
narrower than it looks and is enforced *structurally*: the chop only enters the
candidate pool when `all_plays` is empty (`:875-878`), so a chop is never
playable, and a known-orange **playable** is routed through `all_plays` as a chuck
that advances the stack (`:808-813`). Don't add a playability test at `:934` — and
don't remove the `all_plays.empty()` gate without noticing that `:934` would then
pitch a playable orange away.

**`has_ptd()`** — "does Bob have permission to discard?" (`:419-458`), the
discard-permission gate, one ladder:

| Condition | Verdict | Cite |
|---|---|---|
| Bob is obviously loaded | yes | `:452` |
| Bob's chop is **critical** | **no** | `:453` |
| Bob's chop is basic trash | yes, unless the previous player just made an unknown play of that identity | `:454` |
| A known duplicate of Bob's chop is elsewhere in Bob's hand | yes | `:455` |
| Otherwise | **no** iff the chop is playable or is a **rank 2** | `:456-457` |

Candidate construction (`:848-945`), in order: known trash → the
**orange-safety filter**, which drops empathy-trash candidates whose
`possible` still contains an inverted-suit identity, since chucking those is
a play attempt that can strike (`:860-871`) — note it guards only the
known-trash pool, the chop being added after it → chop discard, only when not
locked, no plays available, and `has_ptd()` (`:875-879`) → ordering, where a
pending reactive restricts us to the `expected` set (`:880-901`).

**Most-recent-CTD enforcement** (v0.30, `:902-926`): if the hand holds several
CTD'd cards, only the one with the largest `signal_turn` is discardable this
turn. Older CTDs stay marked but are removed from the pool.

**`locked_discard`** (`src/basics/player_game.cpp:401-439`) picks the sacrifice
when locked: minimise `|possible ∩ critical_set| / |possible|`, then maximise a
score rewarding basic-trash possibilities and high rank / distance from the
stack.

## 2.4 Scoring a clue: `get_result`

`src/conventions/reactor/state_eval.cpp:152-291`. **Reactor only** since
v2.3.0 — reactor0 ports this into `reactor0::get_result` and drops the flat
bad-touch penalty (`reactor0/CONVENTION.md` §2c). Changes here must be
considered for that copy too.

**Hard rejections** (all return `−100`):
- any newly-CTP'd card that isn't in `hypo.me().hypo_plays` — and, in the
  endgame, isn't actually playable (`:181-188`);
- a `PLAY` reading that produces no playables outside the endgame (`:196`);
- a `REVEAL` reading with no playables where all revealed trash was already
  clued (`:197-206`);
- a non-reactive clue where every newly-touched card is bad touch and there
  are no playables (`:207-216`).

**The score** (`:269-275`):

```
value = good_touch                                  // [0, .125, .25, .35, .45, .55] on (new_touched − bad_touch), capped at 5
      + (playables − 2 × duped_playables)
      + 0.2  × untouched_plays                      // plays gained without touching the card
      + 0.05 × revealed_trash                       // 0.01 in endgame
      + 0.05 × fill                                 // 0.10 in endgame
      + 0.02 × elim                                 // 0.05 in endgame
      − 0.1  × bad_touch
      − 10.0 × destroyed_plays
```

Then: `MISTAKE → value − 10` (`:277`); `FIX → value + 1` (`:278`); and a
**`REACTIVE` reading with ≥ 2 playables gets a flat `+10`** (`:279-289`),
sized explicitly to survive the damping in `eval_action` so a genuine two-play
reactive beats any one-play alternative.

**`destroyed_plays`** (`:251-267`) is the v1.7.0 rule: every card that is
`CALLED_TO_PLAY` in the real game but becomes `CALLED_TO_DISCARD` under the
candidate clue's reading costs **−10**. Exempt when the giver can see the card
is basic trash, in which case the CTD is a favour. Origin: replay 1916933 T31,
where a red reactive's leftmost play target mapped onto a queued t3 and called
the reacter to discard it — killing the t4/t5 chain behind it — merely to CTP
a card a plain stable push gets for free.

## 2.5 Scoring any action: `eval_action`

`state_eval.cpp:463-591`. Simulate, then:

- `MISTAKE` interpretation → `−100` (`:470-483`).
- **Clue branch** (`:486-507`): the low-clue-count gate (below), then
  `clue_branch_value` (`:456-461`) —
  `mult = playables_us.empty() ? 0.5 : (in_endgame ? 0.1 : 0.25)` and
  `value = get_result × (result > 0 ? mult : 1.0) − 0.5`. So **positive clue
  value is damped hard once we already hold a play, negative value is not
  damped, and every clue pays a flat 0.5 tempo tax.** `clue_branch_value` is
  factored out because reactor0 reuses it behind its own gate.
- **Play branch** (`:508-524`): unknown identity `+1.5`; known
  `0.02 × (5 − rank)`; a known dupe `−0.25`.
- **Discard branch** (`:525-586`): base tiers — endgame `−1.0`, known trash
  `0.0`, own chop `−0.25`, unknown identity `−1.5`, else `−0.5`. Orange
  tiering raises a stack-advancing discard to `1.0` and floors a
  possibly-orange unknown at `0.5` (`:543-568`). A **`−10.0` block penalty**
  applies if we hold a real obvious playable that this discard would skip
  (`:570-585`).
- Every action finally adds `advance(game, hypo_game, 1)` (`:590`) — a
  one-full-round-forward lookahead.

### The low-clue-count gate

**Reactor only.** Reactor0 replaces this wholesale with its pace-clue tier
gate — a wider window, a three-way tier and no "we hold a play" conjunct (see
`src/conventions/reactor0/CONVENTION.md` §2a). Under reactor0 this gate never
runs, because `reactor0::eval_action` owns the clue branch. **As of v2.3.0
reactor0 also has its own `get_result`** (`reactor0` §2c) — a port of §2.4
below that does not charge the flat bad-touch penalty — so nothing in §2.4 or
this section applies to reactor0 games. Still common to both conventions:
`eval_action`'s play and discard branches, `advance`, `eval_state` and
`eval_game`.

`state_eval.cpp:495-506`. When `clue_tokens < 3` **and** `pace() >= 3` **and**
we hold a real (non-duplicated) obvious playable, a clue must be *high value*
or `eval_action` returns a flat `−1.0`, below any play. The gate is skipped
when every play is a dupe (`:479-482`), because suppressing the clue would
just force a wasted dupe-play.

`is_high_value_clue` (`:105-146`, spec at `:58-80`) — high value iff **any**:

1. **Unique good chop in danger**: Bob is not locked, has no safe discard (no
   obvious play, no known trash, no CTD), and his chop is non-trash with a
   **unique** identity — no visible copy in Cathy's hand, and no
   singleton-inferred copy in the giver's own hand
   (`chop_id_is_unique`, `:82-103`).
2. The clue gets a **critical low card** played — rank 1 or 2 normally, 4 or 5
   reversed (`:137`).
3. The clue gets **≥ 2 new plays** and at least one is the clue-regain rank —
   5 normally, 1 reversed (`:140`, `:144`).

## 2.6 The lookahead: `advance`

`state_eval.cpp:314-456` recursively models what each subsequent player would
do, terminating at `eval_game` when play comes back around **or once the game
has ended** (`:336-339`). The `state.ended()` half of that guard is new in
v5.0.0: without it the recursion kept simulating actions past a strike-out and
manufactured leaves with four or more strikes.

- Playables and no urgent discard → simulate every non-dominated play; take
  the **minimum** if any of them strikes (pessimism), else
  `max(best_play, force_clue_inner(...))` (`:341-396`). A **playable inverted
  (orange) card is simulated with the Discard button** —
  `variants::make_discard_for_simulation` (`:378-382`) — because that is what
  advances an inverted stack, and what `take_action` really issues
  (`src/basics/decide.cpp:885-894`). Simulating it as `PerformPlay` ran the
  game-rule inversion and scored every good chuck as a card thrown away
  (v5.0.0; replay 1957905 #31).
- Locked → discard if clueless, else clue (`:398-405`). At 8 clues, forced
  clue (`:407`). Urgent CTD → discard it (`:409-412`).
- Otherwise `try_discard` (`:414-434`), a **probabilistic** blend:
  `clue_prob × clue_value + (1 − clue_prob) × discard_value`, where
  `clue_prob` is 0.2 if Bob is loaded, 0.2 if his chop is trash, 0.7 if his
  chop is useful, 0.5 with no chop, and 0.8 at deeper offsets (`:419-431`).

`force_clue_inner` (`:295-308`) adds `+1.0` (2 players) or `+0.5` (3+) — a
mild bias making "a clue exists" attractive.

## 2.7 The leaf evaluation

`eval_state` (`:604-636`):
- score term `min(score, 2 × num_suits) × 0.5 + score`;
- clue term: 0 in endgame / at 0 tokens / when unable to clue;
  `3.0 + (tokens − 6) × 0.25` above 6; else `tokens / 2`;
- **`−20.0` per point of `max_score` lost** — by a wide margin the dominant
  term, and the reason the bot treats discarding a critical as near-fatal;
- strikes: `0` / `−1.5` / `−3.5`, and `−100.0` for **three or more**
  (`:626-631`). The switch keys `0` explicitly and puts `−100.0` in `default`;
  until v5.0.0 `default` was `0.0`, so a leaf with four strikes — reachable
  because `advance` used to recurse past the strike-out — scored *better* than
  a clean two-strike one and the argmax chased it (replay 1957905 #31).

`eval_game` (`:637-780`) adds:
- an instant `+100` on reaching the original max score (`:640`);
- `future_val` (`:647-690`): CTP `+0.4`, CTP'd 5 `+0.8`, CTP'd trash `−1.5`;
  **a CTD on an inverted (orange) card is a play call — a chuck — and is
  priced like a CTP: `+0.4`, `+0.8` for a 5, and `−1.5` when the chuck could
  not reach the stack** (`:661-676`, v5.0.0). Only a non-inverted CTD walks
  the discard ladder: CTD trash `+0.3`, CTD sieved `+0.2`, **CTD critical
  `−(5 − playable_away) × 10.0`**, CTD useful-to-us `−(5 − playable_away) × 0.5`.
  Charging a chuck the useful-card penalty (and the `× 10.0` critical one,
  i.e. always in Dark Orange) was the opposite of what a chuck is worth;
- `bdr_val` — bad-discard risk (`:692-745`): per non-duplicated useful
  identity with copies already discarded, `−n²` (rank 1), `−3.0` (rank 2),
  `−1.5` (rank 3), `−0.5` (rank 4), the whole term scaled `× 2.5`;
- `lock_penalty` `0 / −1 / −3 / −10` for 0/1/2/3+ locked players (`:747-757`);
- `endgame_penalty`: projects the final round's plays and charges
  `(stacks_sum − max_score) × 5.0` (`:759-777`).

## 2.8 Enumerating clues: `find_all_clues`

`decide.cpp:495-609`. Used by the endgame solver and forced-endgame, **not**
by `take_action`'s main path. It simulates each clue and:

- drops `MISTAKE`s (`:492-495`);
- drops clues that only re-touch already-clued trash, keeping at most one
  representative "useless" stall clue so the solver retains a stalling option
  (`:475-487`, `:559-565`);
- applies the **critical-discard guard** (`:523-537`): reject any clue whose
  reading would stamp CTD on a card the giver can see is critical. This lives
  here, at selection time, rather than inside `target_discard`, so that every
  observer runs the identical POV-invariant interpretation pipeline
  (narrative from replay 1892428 T47 at `:506-522`);
- sorts survivors by `get_result` descending (`:539`, `:569-570`).

## 2.9 Endgame

**Triggers.** `take_action` forks to the endgame when
`rem_score() <= num_suits + 1` (`:665`). Separately, `in_endgame()`
(`:398-402`) is `pace() < num_players - 1` — deliberately one turn earlier
than the base-game definition — and is what switches the various eval
constants to their endgame values.

**Forced-endgame rules** (`src/endgame/forced_endgame.cpp:249-281`), only when
`cards_left == 1` (`:253`). Both **short-circuit the solver** — whatever they
return is `take_action`'s answer — so a rule that fires wrongly is expensive.

- **Rule 2 — two-critical play** (`:154-210`), checked first. Fires when
  `clue_tokens < num_players` and the current player holds ≥ 2
  singleton-critical cards with ≥ 1 playable. Returns a play, preferring the
  playable critical whose successor is held by another player — the "unblock"
  bonus (`:182-208`, replay 1899527 T47).
- **Rule 1 — 5-lockout** (`:48-115`), needs `clue_tokens > 0`. Fires for a suit
  when its stack is below 4, its 5 is still in someone's hand, and **every
  4-holder's cycle offset is ≥ the 5-holder's** — playing now would empty the
  deck and lock the 5-holder out. The response is to give the top
  `find_all_clues` result (`:274-275`), falling back to any legal clue.

  **CP has two opportunities, and which one counts depends on the card**
  (`:106-110`). Drawing the last card sets `endgame_turns = num_players`
  (`src/basics/game.cpp:328`), so the final round runs offsets `1..n-1` and
  then **CP again** — CP acts last. A **4** in CP's hand can be played right
  now, offset 0, which is the long-standing CP-holds-the-4 exemption. A **5**
  cannot: the rule's own precondition is `play_stacks[suit] < 4`. Its offset is
  therefore `n`, and since no 4-holder can come later the rule never fires when
  CP holds the 5. Scoring it at 0 instead made `offset(fh) < 0` unsatisfiable,
  so the rule fired unconditionally in exactly the case where no lockout can
  exist — bug_report_4_1_0.txt 4.1.0a, replay 1957936 T41, where it returned a
  stall clue and hid a 20/20 orange chuck chain from the solver.

**The solver** (`src/endgame/solver.cpp`) is a probability-weighted DFS over
arrangements of our own unknown cards and deck draws, optimising **exact win
probability** (`Fraction`), not score. Key parameters:

| Parameter | Value | Cite |
|---|---|---|
| Time budget at the call site | **6 seconds** (class default is 30) | `decide.cpp:700`; `include/hanabi/endgame/solver.h:47` |
| Accept threshold | win rate **≥ 1/100** | `decide.cpp:708` |
| Recursion depth cap | 20 — each `simulate_action` costs 10–50 ms through the convention pipeline | `solver.cpp:443` |
| Bail-out | more than **3** fully-unseen useful identities → give up | `solver.cpp:604-610` |
| Consecutive-clue cap | `num_players + 1` since the last draw | `solver.cpp:148-164` |
| Monte-Carlo grouping | arrangements collapse by trash-normalised key, probabilities renormalise | `solver.cpp:720-752` |

Move ordering inside the search (`possible_actions`, `:147-330`): urgent
first, then plays, then the clue cap, then a discard gate `ignore_dc` when
`pace() == 0`, at 8 clues, when a 5 is playable, or while protecting a
critical (`:266-302`); finally plays always first, with **discards before
clues when no other player has a visible playable**, otherwise clues before
discards (`:314-330`).

**Both candidate kinds route the inverted (Orange / Dark Orange) button.**
Plays: a chuck (`PerformDiscard`) only when the orange is known *and currently
playable*, else the ordinary `PerformPlay` (`solver.cpp:201-203`). Discards:
the sole candidate comes from `Game::find_all_discards`
(`src/basics/decide.cpp:1128-1173`), which emits the **pitch**
(`PerformPlay`) when every identity the holder thinks the card could be is
inverted — knowing the suit is enough to know which button to press, and
pressing Discard there would be a play attempt that strikes on trash. Keyed on
`common ∩ per-player` so a holder who *cannot* tell their card is orange still
models the real risk of pressing Discard. Until v6.2.0 this was an
unconditional `PerformDiscard`, and since it is the only discard the solver
ever sees, a known-trash orange had a guaranteed misplay as its sole option —
bug_report_5_0_0.txt, replay 1957953 T30, which struck.

The same routing now applies to forced Rule 2 (`forced_endgame.cpp:209-218`):
its candidates are filtered by `is_playable`, which on an inverted suit means
*the chuck advances the stack*, so returning `PerformPlay` pitched a
singleton-critical card into the discard pile.

## 2.10 Risk management

There is no urgency enum. Priority is expressed three ways: the binary
`ConvData::urgent` flag (`include/hanabi/basics/card.h:115`), the hardcoded
branch order of §2.1, and the **magnitudes** in `eval_action`, which act as
the real priority ladder:

| Score | Meaning |
|---|---|
| `−100` | Mistake / illegal reading |
| `−10` | Discard that blocks a play; clue that destroys a queued play |
| `−1.5 … −0.25` | Ordinary discards |
| `−1.0` | Low-value clue veto |
| `0.02 … 0.08` | Known plays |
| `0.5 … 1.5` | Unknown play; orange discard-play |
| `+10` | Two-play reactive bonus (inside `get_result`) |

Concrete protections:

- Strikes cost `−1.5 / −3.5 / −100.0` (`state_eval.cpp:601-607`), and
  `advance()` turns pessimistic — taking `min` rather than `max` — as soon as
  any candidate teammate play strikes (`:382-384`).
- Post-strike, the discard handler clears convention info **except** on
  explicitly CTP'd cards (`decide.cpp:239-264`), pinned by
  `tests/test_basics/test_strike_preserves_ctp.cpp`.
- Critical protection: the `−20`-per-lost-point term, the CTD-on-critical
  penalty, the `find_all_clues` critical guard, the urgent Bob-protection
  override, `bdr_val`,
  and `locked_discard`'s critical minimisation.

**Where the bot accepts risk**: it will take an endgame line at a 1% win rate
(`decide.cpp:708`); it scores playing an unknown-identity card at `+1.5`, the
highest non-endgame play value (`state_eval.cpp:513`); `anxiety_play`
(`player_game.cpp:441-470`) gambles on the highest playable-probability card
when locked; and `advance()`'s `clue_prob` model is an explicit probabilistic
bet on teammate behaviour rather than a worst-case assumption.

## 2.11 Time budgets

Timing is instrumented via `hanabi::instr::ScopedTimer`
(`include/hanabi/instrumentation/timer.h:65-83`), emitted as per-turn `TIMING`
records (`src/net/commands.cpp:523-550`) and summarised by
`scripts/log_summary.py`. Instrumented scopes: `take_action`,
`reactor.eval_action`, `reactor.advance`, `reactor.eval_game`,
`reactor.interpret_stable` / `interpret_reactive` (and the colour/rank
sub-paths), `reactor.react_play` / `react_discard`, `endgame.solve`,
`endgame.winnable_simpler`, `endgame.forced_endgame_action`, and `game.rewind`.

`take_action` runs on a dedicated compute thread against a snapshot, so
network traffic cannot mutate state mid-decision
(`src/net/commands.cpp:478-497`).

---

## Test coverage

Behavioural rules above are pinned by:

| Folder | Pins |
|---|---|
| `tests/test_reactor/` | Reactive table, all-plays, CTD revival, dc-target retargeting, same-hand dupes, reversed suits, orange dispatch, discard penalty |
| `tests/test_basics/test_reactive.cpp` | Focus rules, slot arithmetic |
| `tests/test_basics/test_chop.cpp` | `Game::chop` selection (§2.3): most-recent CTD by `signal_turn`, tie-breaks, status ineligibility, locked hand |
| `tests/test_reactor/test_bad_reactive_target/` | dc-target pool ranking (§1a.5): 1916813 clued-over-unclued, 1916888 unknown-over-pre-known |
| `tests/test_reactor/test_stacked_plays/` | "Never stack on a queued CTP" (1892197, 1916815) |
| `tests/test_reactor/test_bad_reaction/` | Stable-first routing when Bob is loaded (1915981) |
| `tests/test_reactor/test_receiver_misinterpretation/` | Re-tasking a pending reacter (1916791) |
| `tests/test_reactor/test_decision_making/` | Low-clue-count gate, high-value-clue conditions, the v1.7.0 destroyed-play rule |
| `tests/test_reactor/test_endgame/` | 10 endgame replay regressions: solver winrate, forced-endgame 5-lockout / two-critical, final-round stall-vs-play |
| `tests/test_reactor/test_misc/` | 36 mid-game convention replay regressions: empathy narrowing, focus/target selection, pink promise, play-queue order, clue eval |
| `tests/test_endgame/` | Convention-neutral solver unit tests (forced-endgame rules, helper, smoke) |

See `CLAUDE.md` for how to turn a bug report into a regression test.
