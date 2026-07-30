# The Reactor0 Convention, as implemented

This is the **ruling reference** for how this bot interprets clues and how it
chooses actions **when a game runs the reactor0 convention**. Where this
document and the code disagree, that is a bug in one of them — every rule
below cites the `file:line` that implements it.

Reactor0 is the minimalist sibling of [reactor](../reactor/CONVENTION.md).
Terminology is defined in [GLOSSARY.md](GLOSSARY.md); terms not defined there
(chop, lock slot, pitch/chuck, CTP/CTD, critical, loaded…) carry their
[reactor glossary](../reactor/GLOSSARY.md) meanings. Convention that is legal
but not yet implemented is tracked in [TODO.md](../../../TODO.md).

Reading conventions (same as reactor's doc): **slot 1 is the leftmost,
newest card**; **Alice / Bob / Cathy** are positional — Alice is the clue
giver, Bob the next player, Cathy the one after.

## §0 Status and relationship to reactor

- Reactor0 is a **3-player** convention. Which convention a game runs is
  `Game::convention`, resolved at game init: reactor0 selected AND exactly 3
  players → reactor0; otherwise that game runs reactor
  (`src/net/commands.cpp:292-299`). The choice is recorded in the `game_init`
  log record and in the snapshot (`src/logging/state_snapshot.cpp`), so
  replays rerun under the convention they were played with.
- **Shared with reactor, unchanged**: the whole decision layer — `eval_action`
  / `get_result` / `advance` / `eval_state` / `eval_game`, the `take_action`
  ladder, `chop()`, `has_ptd()`, `find_all_clues`, the endgame solver (see
  reactor's §2 Decision Making); the interpretation primitives `target_play`,
  `target_discard`, `ref_discard`, `check_fix`, `delayed_plays`,
  `effective_possible_for`; the reaction-resolution machinery (`calc_slot`,
  `calc_target_slot`, `target_i_play/discard`, the four `elim_*` matrices);
  and the variant layers (pink promise, brownish trash reveal, inverted
  pitch/chuck compensation, reversed suits).
- **Absent by design** (present in reactor): the reactive focus, referential
  play for colour clues, response inversion and rewinds, the loadedness
  dispatcher, deferral-carries-reactive, re-tasking. Reactor0's dispatcher is
  the whole of §1a.
- The dispatch fork lives at the single engine seam:
  `src/basics/decide.cpp:54-58` (clues), `:274-278` (discards), `:332-336`
  (plays).

## §1a Dispatch — purely positional

`reactor0::interpret_clue` (`src/conventions/reactor0/interpret_clue.cpp:292-328`):

- empty clue in an empty-clues variant → `USELESS`;
- **clue to Bob → always stable**, even when Bob is loaded (`:320-325`);
- **clue to anyone else → always reactive with Bob as reacter**, even when
  the giver is locked, in the endgame, or at 8 clue tokens (`:326-327`).

The stall context (`giver obviously locked || in_endgame() ||
clue_tokens == 8`, `:317-318`) is passed to the stable branches only as the
`stall` flag that reactor's `ref_discard` already honours.

## §1b Stable colour — a direct play clue

`stable_colour` (`interpret_clue.cpp:115-156`). **There is no referential
play in reactor0.** Priority:

1. **Fix** — `check_fix` reports a reset/duplicate → `FIX` (`:122-127`).
2. **Play reveal** — a previously-clued card the clue fills in as a new
   obvious playable/connectable → `REVEAL`, no stamp; empathy carries it
   (`find_play_reveal`, `:59-84`, mirroring reactor's fill-in machinery).
3. **Direct play** — the **leftmost card touched by this clue** whose common
   empathy could be playable (playable set ∪ delayed-play successors) is
   called to play via `target_play` (`leftmost_could_be_playable` `:89-109`,
   guards + call `:134-152`). The guards are reactor's `ref_play` rejections:
   blind-playing target, CTD'd-and-not-visibly-playable target, inverted
   (orange) target.
4. Otherwise the receiver knows none of the touched cards can play →
   `STALL` (`:155`).

## §1c Stable rank — six priorities

`stable_rank` (`interpret_clue.cpp:160-288`). The pink-promise gate runs
first (`:171-174`), and the rank is classified over
`variant->touch_possibilities` exactly as reactor does (`:176-185`).

1. **Direct play clue** (`:187-238`) — every remaining useful identity of
   the rank is playable (assuming good touch). Focus = leftmost **newly**
   touched card (pinkish variants: `variants::playable_rank_focus`); with no
   newly touched cards, the leftmost **touched** card that could be playable.
   The focus is narrowed to its playable identities, `info_lock` set, and
   stamped via `variants::called_focus_status` (CTD for a possibly-orange
   focus). Returns `PLAY`. An *unnecessary* focus (every possibility trash or
   visible elsewhere) makes the clue a `STALL` instead (`:206-216`).
2. **Trash reveal** (`:240-251`) — every touchable identity is trash. The
   leftmost newly touched card is marked known trash (`inferred ∩= trash_set`,
   `meta.trash`); `REVEAL`. No newly touched cards → `STALL`. **Terminal**:
   never falls through to a referential reading (unlike reactor, where an
   all-trash rank clue is the trash push and this is where reactor0 and
   reactor deliberately diverge — see TODO.md's reactor-only trash-push
   entry).
3. **Previously-clued trash / dupe reveal** (`:253-273`) — `check_fix` →
   `FIX`; a previously-clued card newly known trash → `REVEAL`; the brownish
   trash reveal (`variants::brownish_trash_reveal`) → `REVEAL`. A rank play
   reveal also fires here (`:278`).
4. **Lock** and 5. **Referential discard** (`:282-284`) — a clue touching at
   least one new card falls into reactor's `ref_discard`
   (`src/conventions/reactor/interpret_clue.cpp:317-420`): touching the lock
   slot (oldest unclued) stamps the whole hand `CHOP_MOVED` → `LOCK`;
   otherwise the first unclued slot right of the focus is stamped
   `CALLED_TO_DISCARD` → `DISCARD`, pink promise included.
6. **Stall** (`:287`) — no new cards and nothing above fired.

## §1d Reactive — the clue value is the anchor

`reactor0::interpret_reactive` (`interpret_reactive.cpp:404-441`). There is
no reactive focus. The anchor is:

> **react_slot + target_slot ≡ anchor (mod hand size)** where
> **anchor = the rank** for rank clues, and **the fixed colour value** for
> colour clues (`:416-418`; `calc_slot` is reactor's,
> `src/conventions/reactor/interpret_reaction.cpp:16-19`).

The waiting connection stores the anchor in `ReactorWC::focus_slot` plus a
clue-time snapshot of `allow_reactive_locks` in `ReactorWC::rlocks`
(`:419-430`). The receiver never selects targets — their POV returns
`REACTIVE` immediately and decodes positionally at reaction time (`:435`).

### Colour values

`colour_clue_value` (`src/conventions/reactor0/colour_value.cpp:89-95`), whose
table is built by `build_table` (`:18-76`), keyed on the clue colour NAME
(`Variant::clue_colour_names`):

| Rule | Assignment |
|---|---|
| Fixed | Red=1, Yellow=2, Green=3, Blue=4, Purple=5, Teal=1 (collisions allowed) |
| Black, then Pink, then Brown | first untaken of {4, 3, 5, 2, 1}; all taken → 1 |
| Orange | first untaken of {2, 5, 4, 3, 1}; all taken → 2 |
| any other name | as the Black rule, assigned **last** (after Orange), in `clue_colour_names` order |

Worked example — Red/Blue/Brown/Orange (`Brown & Orange (4 Suits)`):
Red=1, Blue=4, Brown=3, Orange=2. Pinned by
`tests/test_reactor0/test_colour_value.cpp`.

### Rank reactive — an even number of plays (2 or 0)

`reactive_rank` (`interpret_reactive.cpp:168-309`):

- **Phase A — double play** (`:179-227`). The target pool is the receiver's
  playable cards, slots ascending — **including already-CTP'd cards**
  (`play_pool`, `:46-57`; the include-CTP'd rule is a deliberate reactor
  divergence). For each target leftmost-first: compute the react slot; vet it
  POV-invariantly (`effective_possible_for` must intersect playables ∪
  delayed-play connectors, `:190-199`) — a failed vet advances to the
  next-leftmost target; inverted-suit swaps as in reactor; the reacter is
  stamped urgent CTP (blind play) and the receiver target CTP
  (`stamp_receiver_play`, `:129-164`).
- **Phase B — finesse** (`:229-280`). Walked by **target**, leftmost one-away
  first (reactor walks react slots in a fixed order instead). The reacter
  must hold the connector: `effective_possible_for(react).contains(prev_id)`,
  else the next one-away is tried. The POV-invariant abort is kept verbatim
  from reactor: an observer who can SEE the reacter's card reject the whole
  clue (`MISTAKE`) when it isn't the connector — no "try the next slot",
  because the reacter cannot see their own card and would still act on this
  pairing (`:248-255`).
- **Phase C — double discard** (`:282-309`). Zero plays: the reacter
  **discards** the react slot (urgent CTD; a known-critical react slot
  advances to the next dc-candidate, `:293-297`) and the receiver's
  dc-target — chosen by the same rules as colour mode 2 below — resolves at
  reaction time.

### Colour reactive — one play

`reactive_colour` (`interpret_reactive.cpp:313-398`):

- **Mode 1 — receiver has a playable** (`:323-361`): the reacter **discards**
  the react slot and the receiver plays the target (leftmost playable;
  a react slot holding a known critical advances the target, `:335-340`).
- **Mode 2 — no playable** (`:363-401`): the reacter **blind-plays** the
  react slot. The single dc-candidate is determined by the receiver's hand
  (below); the giver only gives this clue when the react-slot card is
  visibly playable — any observer who can see the reacter's card and finds
  it unplayable reads `MISTAKE` (`:379-382`), while the reacter's own POV
  sees no identity and trusts the giver.

The receiver disambiguates the two modes by the reacter's **action type**:
discard → mode 1 (play your target), play → mode 2 (discard your target /
lock).

### The dc-target pool

`dc_candidates` (`interpret_reactive.cpp:75-123`), slots ascending:

1. every card whose actual identity is **basic trash** or a **same-hand
   dupe** (already-CTD'd cards and inverted-suit cards excluded — CTD on
   orange is a chuck that strikes on trash);
2. pool empty and **rlocks on** → the single candidate is the **oldest
   slot**, flagged as the lock;
3. pool empty and **rlocks off** → reactor's sacrifice ordering
   (`reactor::sacrifice_targets`).

Under rlocks, a trash candidate that happens to sit on the oldest slot is
*also* flagged as the lock — see §1e.

## §1e The reactive lock and `allow_reactive_locks`

**Resolution** (`src/conventions/reactor0/interpret_reaction.cpp`): when the
reacter acts, `calc_target_slot` maps their slot to the receiver's target;
the standard table applies — reacter play + RANK ⇒ receiver plays
(`elim_play_play`); play + COLOUR ⇒ receiver discards (`elim_play_dc`);
discard + COLOUR ⇒ receiver plays (`elim_dc_play`); discard + RANK ⇒
receiver discards (`elim_dc_dc`) (`react_play` `:51-80`, `react_discard`
`:82-117`). Reactor0 never rewinds — both entry points always return false.

**The lock reading** (`is_lock_target` `:28-31`, `reactive_lock` `:35-49`):
a dc-target on the receiver's **oldest slot**, with `wc.rlocks` bound at
clue time, stamps every still-held card of the clue-time hand `CHOP_MOVED`
instead of CTD. This applies **uniformly** in both dc modes (colour
play→dc and rank dc→dc), and **conservatively**: even when the oldest slot
actually holds trash, the receiver cannot tell the two cases apart and must
lock (pinned by `tests/test_reactor0/test_reactive_lock.cpp`).

**The flag**: `Game::allow_reactive_locks` defaults per variant —
`starting_required_efficiency(variant, num_players) <= 1.42`
(`src/conventions/reactor0/efficiency.cpp:7-26`; the formula is
`max_score / (8 + starting_pace + num_suits)`, the regain pool halved under
Clue Starved). `/rlocks on|off` overrides process-wide and retro-applies to
running games (`src/net/commands.cpp`, `chat_rlocks`); in-flight reactives
are insulated because the reading binds at clue time via `ReactorWC::rlocks`.

## §1f Variant layers

Inherited from reactor, by construction rather than reimplementation: the
pink promise (via `violates_pink_promise` + `ref_discard` +
`playable_rank_focus`), the brownish trash reveal, the inverted (orange)
compensation (`called_focus_status`, `would_lose_inverted_reacter`,
target-play/discard swaps at every reactive site), and reversed suits (free
from `State`'s direction-aware helpers). Reactive anchors ignore the
rainbowish/pinkish focus tables entirely — the anchor is the clue value in
every variant. `/allplays` is a reactor concept; under reactor0 the
`wc.all_plays` snapshot still routes rank-style resolution but the flag is
not part of the convention (see TODO.md).

## §1g POV invariance

The reactor discipline carries over unchanged: selection inputs are (a)
common thoughts, and (b) deck identities invisible only to the receiver —
who never runs selection. Giver-only knowledge (the reacter's actual card)
is used exclusively to **reject** a clue (nullopt → `MISTAKE` →
`find_all_clues` drops it), never to select a different target: the blind
mode-2 rejection (`interpret_reactive.cpp:379-382`) and the finesse abort
(`:248-255`) are the two sites.

## §2 Decision making

Reactor's §2 applies unchanged — reactor0 emits the same
`ClueInterp`/`CardStatus` vocabulary, so `get_result`, `eval_action`, the
`take_action` ladder, `chop()`/`has_ptd()` and the endgame solver score it
identically. Reactor0-specific notes:

- the `+10` two-play bonus in `get_result` fires only for rank double plays
  (colour reactives are 1-play) — correct by construction;
- the urgent Bob-protection override is inert under reactor0 (its guard
  requires the receiver ≠ the giver's next player; reactor0's receiver as
  seen from us-as-reacter *is* our next player);
- double-discard and lock clues have no dedicated eval terms yet — they
  score as ordinary discards; tuning is tracked in TODO.md.

## Test coverage

| File | Pins |
|---|---|
| `tests/test_reactor0/test_dispatch.cpp` | positional dispatch vs. loadedness/stall contexts |
| `tests/test_reactor0/test_stable_colour.cpp` | play reveal, direct play, no-ref-play, stall |
| `tests/test_reactor0/test_stable_rank.cpp` | all six rank priorities |
| `tests/test_reactor0/test_reactive_rank.cpp` | anchor arithmetic, include-CTP'd, vet walk, finesse, double discard, resolution |
| `tests/test_reactor0/test_reactive_colour.cpp` | both colour modes, critical skip, dupe targets, MISTAKE rejection |
| `tests/test_reactor0/test_reactive_lock.cpp` | lock in both modes, rlocks-off sacrifice, trash-on-oldest-slot |
| `tests/test_reactor0/test_colour_value.cpp` | the colour-value table incl. the spec's worked example |
| `tests/test_reactor0/test_efficiency.cpp` | efficiency formula + rlocks defaults |
| `tests/test_reactor0/test_giver_filters.cpp` | MISTAKE clues never offered |
| `tests/test_basics/test_snapshot_convention.cpp` | convention/rlocks snapshot round-trip + reactor back-compat |
