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
- **Shared with reactor, unchanged**: nearly all of the decision layer —
  `get_result` / `advance` / `eval_state` / `eval_game`, the `take_action`
  ladder, `chop()`, `has_ptd()`, `find_all_clues`, the endgame solver, and
  `eval_action`'s play and discard branches (see reactor's §2 Decision
  Making); the interpretation primitives `target_play`,
  `target_discard`, `ref_discard`, `check_fix`, `delayed_plays`,
  `effective_possible_for`; the reaction-resolution machinery (`calc_slot`,
  `calc_target_slot`, `target_i_play/discard`, the four `elim_*` matrices);
  and the variant layers (pink promise, brownish trash reveal, inverted
  pitch/chuck compensation, reversed suits).
- **Not shared**: `eval_action`'s *clue* branch. Reactor0 owns its own
  (`src/conventions/reactor0/state_eval.cpp`), because it gates which clues it
  is willing to give differently — see §2a. `src/basics/decide.cpp:628-634`
  dispatches on `Game::convention`.
- **Absent by design** (present in reactor): the reactive focus, referential
  play for colour clues, response inversion and rewinds, the loadedness
  dispatcher, deferral-carries-reactive, re-tasking. Reactor0's dispatcher is
  the whole of §1a.
- The dispatch fork lives at the single engine seam:
  `src/basics/decide.cpp:55-64` (clues), `:282-289` (discards), `:343-350`
  (plays). Each fork also runs `enforce_call_invariants` (§1h) for reactor0
  games only.
- **`/allplays` is reactor-only.** It promotes reactor's colour reactives to
  play+play; reactor0's parity is fixed by clue kind, so the flag has no
  meaning here. It is never set on a reactor0 game
  (`src/net/commands.cpp:292-302`), `chat_allplays` skips reactor0 games when
  retro-applying (`:914-933`), and a reactor0 waiting connection always stores
  `all_plays = false` (`interpret_reactive.cpp:524-530`).

## §1a Dispatch — purely positional

`reactor0::interpret_clue` (`src/conventions/reactor0/interpret_clue.cpp:295-331`):

- empty clue in an empty-clues variant → `USELESS`;
- **clue to Bob → always stable**, even when Bob is loaded (`:321-326`);
- **clue to anyone else → always reactive with Bob as reacter**, even when
  the giver is locked, in the endgame, or at 8 clue tokens (`:327-328`).

The stall context (`giver obviously locked || in_endgame() ||
clue_tokens == 8`, `:318-319`) is passed to the stable branches only as the
`stall` flag that reactor's `ref_discard` already honours.

## §1b Stable colour — a direct play clue

`stable_colour` (`interpret_clue.cpp:116-157`). **There is no referential
play in reactor0.** Priority:

1. **Fix** — `check_fix` reports a reset/duplicate → `FIX` (`:122-128`).
2. **Play reveal** — a previously-clued card the clue fills in as a new
   obvious playable/connectable → `REVEAL`, no stamp; empathy carries it
   (`find_play_reveal`, `:59-84`, mirroring reactor's fill-in machinery).
3. **Direct play** — the **leftmost card touched by this clue** whose common
   empathy could be playable (playable set ∪ delayed-play successors) is
   called to play via `target_play` (`leftmost_could_be_playable` `:92-112`,
   guards + call `:135-153`). The guards are reactor's `ref_play` rejections:
   blind-playing target, CTD'd-and-not-visibly-playable target, inverted
   (orange) target.
4. Otherwise the receiver knows none of the touched cards can play →
   `STALL` (`:156`).

Selection and narrowing cannot disagree about their baseline, because no
layer is ever handed a card with an empty `inferred`: the engine resets a
contradicted card to its global empathy the moment the contradiction happens
(§1i).

## §1c Stable rank — seven priorities

`stable_rank` (`interpret_clue.cpp:161-291`). The pink-promise gate runs
first (`:172-175`), and the rank is classified over
`variant->touch_possibilities` exactly as reactor does (`:177-186`).

1. **Direct play clue** (`:188-237`) — every remaining useful identity of
   the rank is playable (assuming good touch). Focus = leftmost **newly**
   touched card (pinkish variants: `variants::playable_rank_focus`); with no
   newly touched cards, the leftmost **touched** card that could be playable.
   The focus is narrowed to its playable identities, `info_lock` set, and
   stamped via `variants::called_focus_status` (CTD for a possibly-orange
   focus). Returns `PLAY`. An *unnecessary* focus (every possibility trash or
   visible elsewhere) makes the clue a `STALL` instead (`:207-217`).
2. **Play reveal** (`:239-244`) — the clue fills a previously-clued card in
   as a new obvious playable, without the whole rank being playable →
   `REVEAL`, no stamp; empathy carries it. **Terminal, and ranked above every
   other reveal and above the referential readings**: the receiver has a play
   to make, which outranks being told what to discard. So a rank clue that
   both fills in a playable and touches the lock slot is a `REVEAL`, not a
   `LOCK`.
3. **Trash reveal** (`:246-257`) — every touchable identity is trash. The
   leftmost newly touched card is marked known trash (`inferred ∩= trash_set`,
   `meta.trash`); `REVEAL`. No newly touched cards → `STALL`. **Terminal**:
   never falls through to a referential reading (unlike reactor, where an
   all-trash rank clue is the trash push and this is where reactor0 and
   reactor deliberately diverge — see TODO.md's reactor-only trash-push
   entry).
4. **Previously-clued trash / dupe reveal** (`:259-280`) — `check_fix` →
   `FIX`; a previously-clued card newly known trash → `REVEAL`; the brownish
   trash reveal (`variants::brownish_trash_reveal`) → `REVEAL`.
5. **Lock** and 6. **Referential discard** (`:283-287`) — a clue touching at
   least one new card falls into reactor's `ref_discard`
   (`src/conventions/reactor/interpret_clue.cpp:317-420`): touching the lock
   slot (oldest unclued) stamps the whole hand `CHOP_MOVED` → `LOCK`;
   otherwise the first unclued slot right of the focus is stamped
   `CALLED_TO_DISCARD` → `DISCARD`, pink promise included.
7. **Stall** (`:290`) — no new cards and nothing above fired.

## §1d Reactive — the clue value is the anchor

`reactor0::interpret_reactive` (`interpret_reactive.cpp:503-544`). There is
no reactive focus. The anchor is:

> **react_slot + target_slot ≡ anchor (mod hand size)** where
> **anchor = the rank** for rank clues, and **the fixed colour value** for
> colour clues (`:515-517`; `calc_slot` is reactor's,
> `src/conventions/reactor/interpret_reaction.cpp:16-19`).

The waiting connection stores the anchor in `ReactorWC::focus_slot` plus a
clue-time snapshot of `allow_reactive_locks` in `ReactorWC::rlocks`
(`:518-531`). The receiver never selects targets — their POV returns
`REACTIVE` immediately and decodes positionally at reaction time (`:538`).

**Candidate walks are transactional** (`Rollback`, `:53-72`). Every phase
snapshots the game immediately before its first mutation and restores on each
abandoning path, including the terminal `MISTAKE`. Without it, `target_play` /
`target_discard` — which mutate even when they fail — leave a play call that
no clue ever made; being same-turn, such a call competes with the real one for
being actioned first, and when it is played the receiver has nothing to
interpret.

### Colour values

`colour_clue_value` (`src/conventions/reactor0/colour_value.cpp:89-95`), whose
table is built by `build_table` (`:18-76`), keyed on the clue colour NAME
(`Variant::clue_colour_names`):

Applied **in this order** — the order matters, because each rule marks the
value it takes:

| # | Rule | Assignment |
|---|---|---|
| 1 | Fixed | Red=1, Yellow=2, Green=3, Blue=4, Purple=5, Teal=1 (collisions allowed) |
| 2 | Orange | first untaken of {2, 5, 4, 3, 1}; all taken → 2 |
| 3 | Black, then Pink, then Brown | first untaken of {4, 3, 2, 5, 1}; all taken → 1 |
| 4 | any other name | as the Black rule, assigned **last**, in `clue_colour_names` order |

Orange sits ahead of the darks, so it gets first refusal on whatever the
fixed colours left: in `Brown & Orange (6 Suits)` (R,Y,G,B + Brown + Orange)
Orange takes 5 and Brown falls back to 1, where assigning Brown first would
have given Brown=5 and Orange the exhausted 2.

Worked example — Red/Blue/Brown/Orange (`Brown & Orange (4 Suits)`):
Red=1, Blue=4, Orange=2, Brown=3. Pinned by
`tests/test_reactor0/test_colour_value.cpp`.

### Rank reactive — an even number of plays (2 or 0)

`reactive_rank` (`interpret_reactive.cpp:211-398`):

- **Phase A — double play** (`:223-299`). The target pool is the receiver's
  playable cards, slots ascending — **including already-CTP'd cards**
  (`play_pool`, `:90-101`; the include-CTP'd rule is a deliberate reactor
  divergence). For each target leftmost-first: compute the react slot; vet it
  POV-invariantly (`effective_possible_for` must intersect playables ∪
  delayed-play connectors, `:234-243`) — a failed vet advances to the
  next-leftmost target; inverted-suit swaps as in reactor; the reacter is
  stamped urgent CTP (blind play) and the receiver target CTP
  (`stamp_receiver_play`, `:172-207`). A react card the **giver can see** is
  neither playable nor a connector rejects the clue (`:249-268`) — it would
  strike.
- **Phase B — finesse** (`:301-364`). Walked by **target**, leftmost one-away
  first (reactor walks react slots in a fixed order instead). The reacter
  must hold the connector — direction-aware, so `next()` on a reversed suit
  and `prev()` elsewhere (`variants::connector_of`,
  `include/hanabi/conventions/variants/reversed.h`) — via
  `effective_possible_for(react).contains(connector)`, else the next one-away
  is tried.
- **Phase C — double discard** (`:366-398`). Zero plays: the reacter
  **discards** the react slot (urgent CTD; a known-critical react slot
  advances to the next dc-candidate, `:377-381`) and the receiver's
  dc-target — chosen by the same rules as colour mode 2 below — resolves at
  reaction time.

### Colour reactive — one play

`reactive_colour` (`interpret_reactive.cpp:400-500`):

- **Mode 1 — receiver has a playable** (`:411-456`): the reacter **discards**
  the react slot and the receiver plays the target (leftmost playable;
  a react slot holding a known critical advances the target, `:424-429`).
- **Mode 2 — no playable** (`:458-500`): the reacter **blind-plays** the
  react slot. The single dc-candidate is determined by the receiver's hand
  (below); the giver only gives this clue when the react-slot card is
  visibly playable — any observer who can see the reacter's card and finds
  it unplayable reads `MISTAKE` (`:479-482`), while the reacter's own POV
  sees no identity and trusts the giver.

The receiver disambiguates the two modes by the reacter's **action type**:
discard → mode 1 (play your target), play → mode 2 (discard your target /
lock).

### The dc-target

`dc_candidates` (`interpret_reactive.cpp:119-170`):

1. the **leftmost** card whose actual identity is **basic trash** or a
   **same-hand dupe** — *always*, regardless of cluedness or of any status
   already stamped on it. This is a deliberate divergence from reactor, which
   reorders and filters its pool: here the receiver derives the target from
   hand position alone, so a second copy further right cannot move it and a
   standing `CALLED_TO_DISCARD` cannot skip it. The one exclusion is
   inverted-suit cards, which can never be named at all (a CTD on orange is a
   chuck that strikes on trash);
2. no such card and **rlocks on** → the single candidate is the **oldest
   slot**, flagged as the lock;
3. no such card and **rlocks off** → reactor's sacrifice ordering
   (`reactor::sacrifice_targets`).

Under rlocks, a trash candidate that happens to sit on the oldest slot is
*also* flagged as the lock — see §1e.

## §1e The reactive lock and `allow_reactive_locks`

**Resolution** (`src/conventions/reactor0/interpret_reaction.cpp`): when the
reacter acts, `calc_target_slot` maps their slot to the receiver's target;
the standard table applies — reacter play + RANK ⇒ receiver plays
(`elim_play_play`); play + COLOUR ⇒ receiver discards (`elim_play_dc`);
discard + COLOUR ⇒ receiver plays (`elim_dc_play`); discard + RANK ⇒
receiver discards (`elim_dc_dc`) (`react_play` `:51-83`, `react_discard`
`:85-132`). Parity keys on `wc.clue.kind` **alone** (`:66`, `:115`);
`wc.all_plays` is deliberately not consulted, because reading it let
resolution contradict the reading every seat agreed on at clue time. Should a
waiting connection carry the flag anyway (a replayed snapshot, or a reactor WC
resolved under reactor0), the agreement is play+play, so the reacter has no
discard available to them at all: a discard is then a **known mistake**
(`DiscardInterp::MISTAKE`) that applies no marks (`:96-106`). Reactor0 never
rewinds — both entry points always return false.

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
from `State`'s direction-aware helpers, plus `variants::connector_of` for the
finesse prerequisite, which runs **up** the ranks on a reversed suit).
Reactive anchors ignore the rainbowish/pinkish focus tables entirely — the
anchor is the clue value in every variant. `/allplays` is a reactor concept
and never reaches a reactor0 game at all (§0).

## §1g POV invariance — shared knowledge retargets, giver-only knowledge rejects

Selection inputs are (a) common thoughts, and (b) deck identities invisible
only to the receiver, who never runs selection. The line that matters is
whether the **reacter** shares the knowledge:

- **Shared between giver and reacter** → the walk may **retarget**. If both
  can see that a candidate leads to an inactionable slot, the reacter will
  themselves walk on to the next candidate, so the two stay in step. The
  `effective_possible_for` vet (Phase A `:234-243`, Phase B `:314-316`) and
  the known-critical react slot (Phase C `:377-381`, colour mode 1 `:424-429`)
  are these: they `continue`.
- **Giver-only** — anything reading `state.deck[react_order]`, which the
  reacter cannot see — may only **reject** the clue (nullopt → `MISTAKE` →
  `find_all_clues` drops it). Retargeting on it desyncs the table: the reacter
  would compute the original pairing and act on it. These sites `return
  nullopt`: the Phase A playability veto (`:249-268`), the Phase B connector
  abort (`:328-332`), `would_lose_inverted_reacter` in Phase A (`:269-276`),
  Phase B (`:339-346`) and colour mode 1 (`:435-443`), and the blind mode-2
  rejection (`:479-487`).

Worked example: Alice clues red, and the first target maps to Bob's slot 5,
which Alice can see is an unplayable orange 3. Bob cannot see it. He will not
walk on — he will chuck it and strike. So Alice must reject the clue, not
quietly retarget to Cathy's next playable.

## §1h Call invariants — the shape a hand's outstanding calls may take

`enforce_call_invariants`
(`include/hanabi/conventions/reactor0/call_invariants.h`,
`src/conventions/reactor0/call_invariants.cpp`), run after every reactor0
interpretation at the engine seam (`src/basics/decide.cpp:63`, `:285`,
`:346`) rather than at each stamping site, so no path can forget it.

1. **Play calls run in play order.** A hand may carry **several**
   `CALLED_TO_PLAY` cards at once, and the holder actions them
   **most-recently-stamped first**, skipping any it knows from empathy must be
   trash (`src/basics/decide.cpp:695-710`). There is no unwinding: if the
   holder plays an older call, the receiver does not interpret that play.
   To keep stamp order and slot order from disagreeing, a newer call on an
   **older** slot **erases** the earlier call on any newer slot — a newer clue
   would not have pointed past a card that was still playable. The invariant
   is therefore that CTP cards run newest slot → oldest slot in exactly play
   order, which is what lets the shared urgent scan pick by slot without
   consulting signal turns.
2. **At most one discard call.** Unlike play calls, `CALLED_TO_DISCARD` does
   not stack: a new call replaces the standing one. Cards merely *revealed* to
   be basic trash (`meta.trash`) are not calls and are untouched — they
   outrank the chop for discard purposes without occupying the CTD slot.

## §1i Contradicted inferences reset immediately (engine-wide)

When a card's `inferred` empties, its inference chain has been contradicted.
The engine resets that card to its **global empathy** at the moment it
happens, in `Game::on_clue` (`src/basics/game.cpp:196-215` touched,
`:236-244` untouched) — **before any convention interprets the clue**, and
for both touched and untouched cards. Because `possible` is already
clue-narrowed at that point, this is exactly "reset to global empathy, then
apply this clue's empathy". The call resting on the contradicted chain is
voided with it (`clear_contradicted_call`, `:163-168`), matching the meta half
of `elim`'s long-standing step-1 reset.

This is **convention-agnostic** and applies to reactor too. Its consequence
here: no interpretation layer is ever handed a card with an empty `inferred`,
so target selection and target narrowing cannot disagree about their baseline.

## §2 Decision making

Reactor's §2 applies **except for `eval_action`'s clue branch** — reactor0
emits the same `ClueInterp`/`CardStatus` vocabulary, so `get_result`, the
`take_action` ladder, `chop()`/`has_ptd()`, `advance`, `eval_game` and the
endgame solver score it identically. What differs is §2a below: which clues
reactor0 is willing to give at a low clue count. Reactor0-specific notes:

- the `+10` two-play bonus in `get_result` fires only for rank double plays
  (colour reactives are 1-play) — correct by construction;
- the urgent Bob-protection override is inert under reactor0 (its guard
  requires the receiver ≠ the giver's next player; reactor0's receiver as
  seen from us-as-reacter *is* our next player);
- double-discard and lock clues have no dedicated eval terms yet — they
  score as ordinary discards; tuning is tracked in TODO.md.

## §2a The pace-clue tier gate

`src/conventions/reactor0/state_eval.cpp`. **Reactor's low-clue-count gate
(reactor's §2.5) does not run under reactor0** — this replaces it wholesale.
Everything else in reactor's §2 still applies, and non-clue actions delegate
straight back to `reactor::eval_action` (`:243-246`).

Naming: the worth of *giving* a clue is its **tier**, never its "value" —
"clue value" already means the reactive anchor (GLOSSARY: *anchor (value)*,
*colour value*).

**The window** (`:266-268`): `pace() >= 3 && clue_tokens <= 3`. Two deliberate
differences from reactor's gate: it is one token wider (`<= 3`, not `< 3`),
and it has **no "we hold a real play" conjunct** — it fires whether or not
Alice has anything queued.

**The required tier** (`requires_high_tier`, `:190-202`):

| Alice's hand | Required |
|---|---|
| holds ≥1 card stamped `CALLED_TO_PLAY`; **or**, in a variant containing an inverted suit (`variants::includes_inverted`), ≥1 stamped `CALLED_TO_DISCARD` | **HIGH** |
| otherwise | **HIGH or MEDIUM** |

The stamp is read **literally** off `game.meta[o].status`. An empathy-known
playable carrying no stamp does *not* raise the bar — the rule is about
outstanding *calls* Alice can fall back on, not about what she happens to
know. (Reactor's gate keys on `obvious_playables` instead; this is the
deliberate divergence.) A clue below its required tier scores a flat `-1.0`,
reactor's rejection convention: below any play, above `-100`.

**The tiers** (`clue_tier`, `:204-240`). HIGH iff **any** of:

1. **H1 — Bob's chop is endangered.** Bob is not locked, has no safe action
   (no obvious play, no known trash, no CTD — all three are covered by
   `thinks_trash`, `player_game.cpp:115-132`), and his chop is *endangered*
   (below). `:215-220`.
2. **H2** — the clue gets a **critical 1 or 2** played (5 or 4 on a reversed
   suit, via `variants::is_first_or_second_rank`). `:221-222`.
3. **H3** — the clue gets **two new plays**, at least one at the clue-regain
   rank (5 normally, 1 reversed, `variants::is_clue_regain_rank`). `:223-224`.

NOT-LOW iff any of H1–H3, or, when **Cathy's** chop is endangered (`:227-238`):

4. **N3** — the clue gets two new plays. `:229-230`.
5. **N2** — the clue is **reactive** and Bob has no colour stable play clue he
   could give Cathy. Reactive is a single integer compare, `action.target !=
   bob`, since dispatch is positional (§1a). `:231-237`.

MEDIUM is NOT-LOW and not HIGH; LOW is everything else. "New plays" are counted
as CTP-status transitions between the real game and the clue's hypo
(`new_play_facts`, `:144-161`) — the same walk reactor's `is_high_value_clue`
uses.

**Endangered chop** (`at_risk_chop`, `:107-133`), judged from Alice's full
visibility. All four must hold: the identity is known to Alice and not basic
trash; there is **no second copy in the holder's own hand**; no copy sits in
the third player's hand; and Alice cannot prove she holds a copy herself. The
first of those is stricter than reactor's `chop_is_nontrash`
(`reactor/state_eval.cpp:44-49`), which tests only `is_basic_trash` — a chop
the holder can safely pitch because they hold the other copy is not in danger.

**"Alice provably holds a copy"** (`alice_provably_holds`, `:58-99`) extends
reactor's singleton test (`reactor/state_eval.cpp:96-101`) to **group
("sudoku") elim**. For any subset S of Alice's hand, let `u` be the union of
what those |S| cards could be; if fewer than |S| copies of `u \ {id}` are still
unaccounted for, then at least one of them must be `id`. |S| = 1 reduces to
exactly the singleton rule. Inference sets come from `common` (the view
reactor reads, and the one the engine and test harness both maintain);
availability counts come from `me()`, which can see the other hands. It errs
safe in both directions — a false "Alice holds it" would kill a save clue, so
the bound deliberately over-counts. A 3-player hand is 5 cards, so all 31
non-empty subsets are enumerated directly; `cross_elim`
(`src/basics/player_elim.cpp:165-226`) solves the dual problem (it strips
locked ids from cards *outside* the group) and cannot answer this.

**Bob's colour play clue for Cathy** (`has_colour_play_clue_for`, `:168-186`)
is a structural check, not a simulation: for each colour clue Bob could give
Cathy it replays `stable_colour`'s target choice (§1b step 2,
`interpret_clue.cpp:134`) plus its three guards (`:141-149`), then asks whether
the named card actually plays. Two known approximations, both deliberate: it
skips the FIX branch above the direct-play read (`interpret_clue.cpp:125-128`),
and it does not model a play reveal (`:131`). It is evaluated **last** in
`clue_tier`, behind the O(1) reactive test, because it is the only costly term.

**Known gap.** `advance` / `force_clue` score hypothetical *partner* clues via
`get_result`, never `eval_action`, so the lookahead does not apply this gate —
the bot models partners as clueing freely at low tokens while throttling
itself. This is pre-existing for reactor and more pronounced here; tracked in
TODO.md.

## Test coverage

| File | Pins |
|---|---|
| `tests/test_reactor0/test_dispatch.cpp` | positional dispatch vs. loadedness/stall contexts |
| `tests/test_reactor0/test_stable_colour.cpp` | play reveal, direct play, no-ref-play, stall |
| `tests/test_reactor0/test_stable_rank.cpp` | the rank priority ladder |
| `tests/test_reactor0/test_stable_colour_baseline.cpp` | §1i — a contradicted inference resets and the clue reads afresh, touched and untouched |
| `tests/test_reactor0/test_call_invariants.cpp` | §1h — CTP play order, single CTD, revealed trash left alone |
| `tests/test_reactor0/test_candidate_rollback.cpp` | a clue stamps exactly the card it names; abandoned candidates roll back |
| `tests/test_reactor0/test_pov_reject.cpp` | §1g — giver-only knowledge rejects rather than retargets |
| `tests/test_reactor0/test_dc_target_leftmost.cpp` | leftmost trash/dupe always, over a standing CTD and over a second copy |
| `tests/test_reactor0/test_reversed_finesse.cpp` | the connector runs up the ranks on a reversed suit |
| `tests/test_reactor0/test_allplays_parity.cpp` | `/allplays` never reaches a reactor0 WC; a reacter discard under one is a known mistake |
| `tests/test_reactor0/test_urgent_skips_known_trash.cpp` | a play call known to be trash yields to the next call |
| `tests/test_net/test_allplays_scope.cpp` | `/allplays` reaches reactor games only, at init and on retro-apply |
| `tests/test_reactor0/test_reactive_rank.cpp` | anchor arithmetic, include-CTP'd, vet walk, finesse, double discard, resolution |
| `tests/test_reactor0/test_reactive_colour.cpp` | both colour modes, critical skip, dupe targets, MISTAKE rejection |
| `tests/test_reactor0/test_reactive_lock.cpp` | lock in both modes, rlocks-off sacrifice, trash-on-oldest-slot |
| `tests/test_reactor0/test_colour_value.cpp` | the colour-value table incl. the spec's worked example |
| `tests/test_reactor0/test_efficiency.cpp` | efficiency formula + rlocks defaults |
| `tests/test_reactor0/test_giver_filters.cpp` | MISTAKE clues never offered |
| `tests/test_basics/test_snapshot_convention.cpp` | convention/rlocks snapshot round-trip + reactor back-compat |
| `tests/test_reactor0/test_decision_making/test_clue_tier.cpp` | §2a tiers — H1/H2 and each endangered-chop disqualifier, incl. the same-hand dupe and both singleton and group elim |
| `tests/test_reactor0/test_decision_making/test_pace_clue_gate.cpp` | §2a window (both boundaries, incl. `clue_tokens == 3`), required tier, and that reactor's own gate is unchanged |
