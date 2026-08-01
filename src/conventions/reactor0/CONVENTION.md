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
- **Shared with reactor, unchanged**: most of the decision layer —
  `advance` / `eval_state` / `eval_game`, the `take_action`
  ladder, `chop()`, `has_ptd()`, `find_all_clues`, the endgame solver, and
  `eval_action`'s play and discard branches (see reactor's §2 Decision
  Making); the interpretation primitives `target_play`,
  `target_discard`, `ref_discard`, `check_fix`, `delayed_plays`,
  `effective_possible_for`; the reaction-resolution machinery (`calc_slot`,
  `calc_target_slot`, `target_i_play/discard`, the four `elim_*` matrices);
  and most variant layers (pink promise, brownish trash reveal, the inverted
  pitch/chuck compensation on the **reactive** side, reversed suits).
  **Exception as of v4.0.0**: the *stable* side of the inverted (orange)
  compensation is reactor0's own — see §1b and the divergence table in §1f.
- **Not shared**: everything about *which clue to give*. Reactor0 owns
  `eval_action`'s clue branch, its own `get_result` (§2c) and a candidate
  filter run before the argmax (§2b), all in
  `src/conventions/reactor0/state_eval.cpp`. `src/basics/decide.cpp:628-634`
  dispatches on `Game::convention`; `:877-878` runs the filter.
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

`reactor0::interpret_clue` (`src/conventions/reactor0/interpret_clue.cpp:540-576`):

- empty clue in an empty-clues variant → `USELESS`;
- **clue to Bob → always stable**, even when Bob is loaded (`:568-573`);
- **clue to anyone else → always reactive with Bob as reacter**, even when
  the giver is locked, in the endgame, or at 8 clue tokens (`:574-575`).

The stall context (`giver obviously locked || in_endgame() ||
clue_tokens == 8`, `:565-566`) is passed to the stable branches only as the
`stall` flag that reactor's `ref_discard` already honours.

## §1b Stable colour — a direct play clue

`stable_colour` (`interpret_clue.cpp:227-329`). **There is no referential
play in reactor0.** Priority:

1. **Fix** — `check_fix` reports a reset/duplicate → `FIX` (`:237-240`).
2. **Play reveal** — a previously-clued card the clue fills in as a new
   obvious playable/connectable → `REVEAL` (`find_play_reveal`, `:60-85`,
   mirroring reactor's fill-in machinery). Normally no stamp; empathy carries
   it. **Exception:** if the revealed card is a known playable *orange*, it is
   stamped `CALLED_TO_DISCARD` (`:244-252`), because that is the button which
   advances an inverted stack — see the orange ladder below.
3. **Orange play reveal** (`:255-266`) — an orange colour clue that reveals a
   playable orange is a play reveal, and the receiver **chucks** the revealed
   card. `find_play_reveal` alone does not cover it: on a colour clue it only
   considers cards that were *already* clued (`:78-83`), because a newly
   touched card becoming obviously playable is the ordinary direct-play
   reading — which is exactly the case orange has to change. `REVEAL`.
4. **The orange ladder** (`:271-302`), reached only when no playable orange
   was revealed. An orange colour clue names one orange card to get rid of or
   to stack:
   - non-dark orange at `pace() > 3` → **pitch** the leftmost touched orange
     the receiver does not know is critical (`holder_knows_critical`,
     `src/basics/player_game.cpp`). A pitch presses Play, which for an
     inverted suit sends the card to the discard pile and regains a clue.
     Stamped `CALLED_TO_PLAY`; returns `DISCARD`, the semantic outcome.
   - `pace() <= 3`, **or** the inverted suit is **dark**
     (`variants::includes_dark_inverted`) → **chuck** the leftmost touched
     orange that could still reach the stacks from the receiver's POV. A
     chuck presses Discard, which advances the orange stack. Stamped
     `CALLED_TO_DISCARD`; returns `PLAY`. Dark forces this reading because
     every dark card is a singleton (`src/basics/variant.cpp:183`), so a
     pitch is an unrecoverable loss.
   - all-critical fallback: the chuck loop runs regardless of what the pitch
     branch found, so if every touched orange is known critical the leftmost
     one that could still reach the stacks is chucked. If none could →
     `STALL` (`:301`).
5. **Direct play** — the **leftmost card touched by this clue** whose common
   empathy could be playable (playable set ∪ delayed-play successors) is
   called to play via `target_play` (`leftmost_could_be_playable` `:203-223`,
   guards + call `:305-326`). The guards are reactor's `ref_play` rejections:
   blind-playing target, CTD'd-and-not-visibly-playable target. There is **no
   longer an inverted-target reject** — it read `state.deck[*target].id()`,
   which is POV-asymmetric (nullopt for the receiver's own card), and the
   orange ladder above now claims every touched card that could be orange.
6. Otherwise the receiver knows none of the touched cards can play →
   `STALL` (`:329`).

The stamps are bespoke rather than reused: `reactor::target_play` narrows
`inferred` to the playable set, but a pitched orange is being thrown away and
need not be playable; `reactor::target_discard` narrows to the *non-critical*
ids, which is the opposite of a chuck and empties outright in Dark Orange
(`stamp_orange_pitch` `:174-195`, `stamp_orange_chuck` `:143-167`).

Selection and narrowing cannot disagree about their baseline, because no
layer is ever handed a card with an empty `inferred`: the engine resets a
contradicted card to its global empathy the moment the contradiction happens
(§1i).

## §1c Stable rank — seven priorities

`stable_rank` (`interpret_clue.cpp:334-536`). The pink-promise gate runs
first (`:346-348`), then the rank is classified (`:350-424`).

**The classification is over what the touched cards can actually be — this is
a deliberate divergence from reactor**, which still scans
`variant->touch_possibilities` (reactor's §1c). Three steps, all
load-bearing:

1. **the pink promise.** A rank-N clue in a pinkish variant promises rank N,
   so the omni suit's *other* ranks are off the table (`:393-404`). The
   promise set is `{clue.value}` ∪ `{special_rank}` when `pink_s` is set:
   `pink_s` is exactly `specialRankAllClueRanks` (`src/basics/variant.cpp:318`),
   which means the special rank is touched by **every** clue rank, so a rank-N
   clue promises rank N *or* the special rank. The gate is the **flag** test,
   not the name-based `includes_pinkish` — the same distinction
   `variants::violates_pink_promise` makes, and for the same reason. Filtering
   to N alone dropped the special rank and, at replay 1942709 in "Pink-Ones &
   Orange", turned a lock into a play call (bug_report_3.txt 3.1).
2. **per-card visibility**, via `reactor::effective_possible_for` (`:379-391`)
   — the card's `possible` narrowed by the copies visible in every non-holder
   hand. That is POV-invariant by construction (defined from the **holder's**
   viewpoint, so every seat computes the same set), which is why it is safe to
   read here at all. Plain `common.thoughts` is not enough.
3. **a useful inverted identity is not playable by this clue** (`:416`). A
   rank direct play clue *always* means pitch, and pitching an orange card
   sends it to the discard pile rather than its stack. So a rank-1 clue cannot
   be used to get an Orange 1 onto the stacks, whatever the stacks say. This
   is confined to priority 1: the play reveal at priority 2 is actioned as a
   chuck and still picks a playable orange up.

Why it matters: an omni suit is pinkish, so `Variant::id_touched` returns true
for it on **every** rank clue (`src/basics/variant.cpp:230`), and the
variant-wide set for a rank-N clue therefore contains the omni suit at ranks
1-5. A single useful-but-unplayable omni rank made `playable_rank` false, so
priority 1 essentially never fired in omni variants and every rank clue
degraded to the referential discard at the bottom of this ladder. Replays
1942517 #1 (a rank 1 at all-zero stacks read as a referential discard) and
1942525 T53 (a playable Sky 4 never called) are the worked examples.

An empty narrowed set teaches nothing and is treated as neither
all-trash nor playable-rank (`:420-424`), rather than vacuously true.

1. **Direct play clue** (`:426-486`) — every remaining useful identity the
   touched cards can hold is playable (assuming good touch). Focus = leftmost
   **newly** touched card — highest order, since slot 1 is newest — in both
   the plain branch (`:436`) and the pinkish one
   (`variants::playable_rank_focus`, `:431`; that helper returned the
   *rightmost* until v3.0.0); with no
   newly touched cards, the leftmost **touched** card that could be playable.
   The focus is narrowed to its playable identities, `info_lock` set, and
   stamped **`CALLED_TO_PLAY` unconditionally** (`:466-478`). reactor0 does
   *not* call `variants::called_focus_status` here — that helper returns CTD
   (a chuck) for a possibly-orange focus, which contradicts "a rank direct
   play clue always pitches". Step 3 of the classification already keeps a
   useful orange out of `new_inferred`, so the stamp states the rule rather
   than leaving it a consequence. **reactor keeps `called_focus_status` at its
   own call site** (`src/conventions/reactor/interpret_clue.cpp:504`).
   Returns `PLAY`. An *unnecessary* focus (every possibility trash or
   visible elsewhere) makes the clue a `STALL` instead (`:443-455`).
2. **Play reveal** (`:488-491`) — the clue fills a previously-clued card in
   as a new obvious playable, without the whole rank being playable →
   `REVEAL`, no stamp; empathy carries it. **Terminal, and ranked above every
   other reveal and above the referential readings**: the receiver has a play
   to make, which outranks being told what to discard. So a rank clue that
   both fills in a playable and touches the lock slot is a `REVEAL`, not a
   `LOCK`.
3. **Trash reveal** (`:491-508`) — every touchable identity is trash. The
   leftmost newly touched card is marked known trash (`inferred ∩= trash_set`,
   `meta.trash`); `REVEAL`. No newly touched cards → `STALL`. **Terminal**:
   never falls through to a referential reading (unlike reactor, where an
   all-trash rank clue is the trash push and this is where reactor0 and
   reactor deliberately diverge — see TODO.md's reactor-only trash-push
   entry).
4. **Previously-clued trash / dupe reveal** (`:510-528`) — `check_fix` →
   `FIX`; a previously-clued card newly known trash → `REVEAL`; the brownish
   trash reveal (`variants::brownish_trash_reveal`) → `REVEAL`.
5. **Lock** and 6. **Referential discard** (`:530-532`) — a clue touching at
   least one new card falls into reactor's `ref_discard`
   (`src/conventions/reactor/interpret_clue.cpp:317-420`): touching the lock
   slot (oldest unclued) stamps the whole hand `CHOP_MOVED` → `LOCK`;
   otherwise the first unclued slot right of the focus is stamped
   `CALLED_TO_DISCARD` → `DISCARD`, pink promise included.
7. **Stall** (`:535`) — no new cards and nothing above fired.

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
- **Mode 2 — no playable** (`:476-521`): the reacter **blind-plays** the
  react slot. The dc-candidates are **walked**, not fixed (`:492`): a pairing
  whose react slot every seat can already see cannot play teaches nothing, so
  the reading moves on to the next trash/dupe candidate rightward. The split
  is §1g's, and it is the whole reason this is safe:
    - `effective_possible_for(react_order)` holds no playable → **retarget**
      (`:503-508`). Shared: the reacter computes the same set for its own
      card, so every seat walks in step.
    - the react slot's **actual** identity is unplayable, or
      `would_lose_inverted_reacter` → **reject the clue** (`:510-521`), never
      retarget. Giver-only: the reacter sees no identity in its own hand and
      would still compute the original pairing, blind-play it and strike.

  Before v3.0.0 mode 2 committed to `dc_candidates().front()` with no walk,
  so replay 1942458 T47 — where the leftmost target mapped onto a react slot
  every seat knew was dead, while the next candidate mapped onto a live one —
  was rejected outright as a `MISTAKE`.

The receiver disambiguates the two modes by the reacter's **action type**:
discard → mode 1 (play your target), play → mode 2 (discard your target /
lock).

### The dc-target

`dc_candidates` (`interpret_reactive.cpp:123-176`):

1. cards whose actual identity is **basic trash** or a **same-hand dupe**,
   slot-ascending — regardless of cluedness or of any status already stamped
   on them. This is a deliberate divergence from reactor, which reorders and
   filters its pool: here the receiver derives the target from hand position
   alone, so a standing `CALLED_TO_DISCARD` cannot skip a card. The one
   exclusion is inverted-suit cards, which can never be named at all (a CTD
   on orange is a chuck that strikes on trash).
   **Only colour mode 2 sees more than the leftmost** — it passes
   `all_trash_targets=true` (`:491`) so it has something to walk. Rank Phase C
   passes false (`:381`) and keeps the strict leftmost rule, so a second copy
   further right still cannot move its target;
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
compensation on the **reactive** side (`would_lose_inverted_reacter`,
target-play/discard swaps at every reactive site), and reversed suits (free
from `State`'s direction-aware helpers, plus `variants::connector_of` for the
finesse prerequisite, which runs **up** the ranks on a reversed suit).

**Not inherited: the stable side of the orange compensation.** From v4.0.0
reactor0 owns its own readings for inverted suits, and they are a
cross-version compatibility break with reactor:

| | reactor | reactor0 |
|---|---|---|
| Stable colour naming an orange | `ref_play`; `target_play` on an inverted target is rejected as a mistake | the §1b orange ladder — play reveal, then pitch or chuck |
| Rank direct play with a possibly-orange focus | `called_focus_status` → CTD (a chuck) | always `CALLED_TO_PLAY` (a pitch); a useful orange is excluded from the reading in the first place |
| `pace()` | not consulted for orange | selects pitch vs chuck |
| Dark Orange | no special handling | always chucks |

The new predicate is `variants::includes_dark_inverted`
(`src/conventions/variants/predicates.cpp:32-37`), which like
`includes_inverted` reads the real `SuitType` flags rather than matching suit
names.
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
- double-discard and lock clues still have no dedicated `get_result` term —
  they score as ordinary 0-play clues. What reactor0 does instead is refuse
  to *offer* a double discard that buys nothing (§2b); reactive locks and
  blind-play clues are untouched, and the remaining tuning is in TODO.md.

## §2a The pace-clue tier gate

`src/conventions/reactor0/state_eval.cpp`. **Reactor's low-clue-count gate
(reactor's §2.5) does not run under reactor0** — this replaces it wholesale.
Everything else in reactor's §2 still applies, and non-clue actions delegate
straight back to `reactor::eval_action` (`:420-423`).

Naming: the worth of *giving* a clue is its **tier**, never its "value" —
"clue value" already means the reactive anchor (GLOSSARY: *anchor (value)*,
*colour value*).

**The window** (`:443-445`): `pace() >= 3 && clue_tokens <= 3`. Two deliberate
differences from reactor's gate: it is one token wider (`<= 3`, not `< 3`),
and it has **no "we hold a real play" conjunct** — it fires whether or not
Alice has anything queued.

**The required tier** (`requires_high_tier`, `:391-403`):

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

**The tiers** (`clue_tier`, `:405-447`). HIGH iff **any** of:

1. **H1 — Bob's chop is endangered.** Bob is not locked, has no safe action
   (no obvious play, no known trash, no CTD — all three are covered by
   `thinks_trash`, `player_game.cpp:115-132`), and his chop is *endangered*
   (below). `:392-397`.
2. **H2** — the clue gets a **critical 1 or 2** played (5 or 4 on a reversed
   suit, via `variants::is_first_or_second_rank`). `:398-399`.
3. **H3** — the clue gets **two new plays**, at least one at the clue-regain
   rank (5 normally, 1 reversed, `variants::is_clue_regain_rank`). `:400-401`.

NOT-LOW iff any of H1–H3, or:

6. **N5 — Bob's chop is playable** and is not duplicated in his own hand
   (`has_playable_chop`, `:151-161`; applied at `:428-432`). Like H1 this is a
   property of the **position, not of the candidate clue**, so it lifts every
   clue that turn to at least MEDIUM. Deliberately weaker than
   `at_risk_chop`: it asks only "playable, and Bob cannot just pitch a spare
   copy", and does *not* care whether a copy sits in Cathy's hand or is
   provable in Alice's — the point is not that the card is in danger but that
   it is a play the team should be collecting, and Cathy is already expecting
   Alice to save it or get it played.

…or, when **Cathy's** chop is endangered (`:434-445`):

4. **N3** — the clue gets two new plays. `:436-437`.
5. **N2** — the clue is **reactive** and Bob has no colour stable play clue he
   could give Cathy. Reactive is a single integer compare, `action.target !=
   bob`, since dispatch is positional (§1a). `:438-444`.

MEDIUM is NOT-LOW and not HIGH; LOW is everything else. "New plays" are counted
as CTP-status transitions between the real game and the clue's hypo
(`new_play_facts`, `:170-187`) — the same walk reactor's `is_high_value_clue`
uses.

**Endangered chop** (`at_risk_chop`, `:121-145`), judged from Alice's full
visibility. All four must hold: the identity is known to Alice and not basic
trash; there is **no second copy in the holder's own hand**; no copy sits in
the third player's hand; and Alice cannot prove she holds a copy herself. The
first of those is stricter than reactor's `chop_is_nontrash`
(`reactor/state_eval.cpp:44-49`), which tests only `is_basic_trash` — a chop
the holder can safely pitch because they hold the other copy is not in danger.

**"Alice provably holds a copy"** (`alice_provably_holds`, `:60-101`) extends
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

**Bob's colour play clue for Cathy** (`has_colour_play_clue_for`, `:194-212`)
is a structural check, not a simulation: for each colour clue Bob could give
Cathy it replays `stable_colour`'s target choice (§1b step 2,
`interpret_clue.cpp:135`) plus its three guards (`:142-150`), then asks whether
the named card actually plays. Two known approximations, both deliberate: it
skips the FIX branch above the direct-play read (`interpret_clue.cpp:126-129`),
and it does not model a play reveal (`:132`). It is evaluated **last** in
`clue_tier`, behind the O(1) reactive test, because it is the only costly term.

**Known gap.** `advance` / `force_clue` score hypothetical *partner* clues via
`get_result`, never `eval_action`, so the lookahead does not apply this gate —
the bot models partners as clueing freely at low tokens while throttling
itself. This is pre-existing for reactor and more pronounced here; tracked in
TODO.md.

## §2b The pointless-double-discard filter

`src/conventions/reactor0/state_eval.cpp:467-566`, invoked once per turn from
`Game::take_action` (`src/basics/decide.cpp:867-878`) immediately after the
candidate clues are built and **before** the argmax.

A rank Phase C reactive (§1d, `interpret_reactive.cpp:366-393`) says only
"you two both discard". That is worth a token when the receiver would
otherwise lose something — and worth nothing when they were going to act
safely anyway. Reactor0 therefore **drops** such a clue from its own candidate
set, but only when a stable clue to Bob would get a card *played* instead.

A candidate is dropped when **all** of (`is_pointless_double_discard`, `:486`):

1. we are the giver, and the clue is **reactive** — under positional dispatch
   that is just `target != Bob` (§1a);
2. the clue is a **rank** clue — colour reactives always call a play, mode 1
   the receiver's and mode 2 the reacter's blind play, so they can never be
   0-play;
3. the variant contains **no inverted suit**. On an inverted suit a *play*
   call is stamped `CALLED_TO_DISCARD` (`interpret_reactive.cpp:198-200`), so
   the CTP-transition count below would read a genuine two-play Phase A as
   zero plays. Rather than mis-fire, the rule sits those variants out;
4. the interpretation is `REACTIVE` and it produces **zero** new
   `CALLED_TO_PLAY` stamps (`new_play_facts`, `:170-187`) — the discriminator,
   since Phase C returns the same `ClueInterp` as a two-play reactive;
5. it is **not a reactive lock** (`predicts_reactive_lock`,
   `interpret_reaction.cpp:31-49`). A lock protects a whole hand, so it keeps
   competing on its own merits;
6. the **receiver is safe** (`receiver_is_safe`, `:467`): their chop is
   expendable (`!at_risk_chop` — trash, same-hand dupe, covered elsewhere, or
   locked) **or** they already hold a safe action — known trash or a standing
   CTD/PTD (`thinks_trash` folds all three together), a known play, or a
   `CALLED_TO_PLAY` call.

…and the set also contains a clue to Bob that is **stable, produces a play,
and is worth giving** (`is_stable_play_clue_for_bob`, `:514`): interpretation
`PLAY` or `REVEAL` *and* a non-empty `playables_result`. The interp test is
what excludes a referential discard, a LOCK or a FIX that merely frees a
playable through elimination; the `playables_result` test is what catches the
**play reveal**, which stamps nothing at all. The alternative must also score
`eval_action > 0` — demoting a double discard on the strength of a clue the
argmax would never pick could leave the bot chucking its own chop instead.

The filter **erases** rather than re-scores: there is no "rank just below X"
primitive, and erasing is the honest encoding of "reactor0 does not offer this
clue this turn". It can never empty `all_clues`, because the qualifying Bob
clue is itself in the set.

**Boundaries.** It does not run in the endgame — `take_action` returns from
the forced-endgame and solver paths before `all_clues` exists — and
`find_all_clues` (the solver's own enumerator) is not filtered, so a `/clues`
listing still shows the double discard. Known one-sidedness: the rule judges
only the *receiver's* half, so it will drop a double discard whose real value
was the reacter's half.

Motivated by replay 1942181 T41, where Cathy's chop was already basic trash
and the alternative played a Berry 5 off a stack of 4.

## §2c Clue scoring — reactor0's own `get_result`

`src/conventions/reactor0/state_eval.cpp:225-358`, ported from
`reactor::get_result` and kept deliberately line-for-line comparable with it.
`clue_branch_value` (`:360`) wraps it exactly as reactor's does;
`advance`, `eval_state` and `eval_game` are still reactor's.

The one intentional divergence is **bad touch is not charged a flat penalty**
(`kBadTouchPenalty = 0.0`, `:216`; reactor uses `0.1`). Reactor charges a clue
twice for sweeping up a card that turns out to be trash — once by shrinking
the `good_touch` bonus, which indexes on `new_touched − bad_touch`, and again
through `−0.1 × bad_touch`. Under reactor0 that left a clue which *gets a card
played* scoring below clues that achieve nothing. At 1942181 T41 the four
live candidates sat within 0.14 of each other and the only playing clue came
**last**, purely because it also touched two dead cards.

`good_touch` still discounts bad touch (`kGoodTouchDiscountsBadTouch = true`,
`:221`), so trash never *earns* the touch bonus — it simply stops being billed
twice. The `−100` rejection for a non-reactive clue whose every newly-touched
card is bad touch with no playables is retained in full (`:310-321`): dropping
the penalty must not make a clue that only buries trash look acceptable.

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
| `tests/test_reactor0/test_orange.cpp` | §1b/§1c in inverted variants — bug 3.1's rank-2 lock, a rank clue still revealing a playable orange, pitch at pace > 3, chuck at pace <= 3, chuck in Dark Orange, play reveal outranking the pitch, and the stall when nothing can reach the stacks |
| `tests/test_endgame/test_orange_chuck.cpp` | bug 3.2 — the solver offers a known playable orange as a chuck, and `perform_to_action` models a chuck of a non-playable orange as a misplay |
| `tests/test_reactor0/test_stable_rank_omni.cpp` | §1c in an omni variant — rank 1 and rank 4 read as direct plays, an unplayable useful identity still blocks, and the pinkish focus is the leftmost |
| `tests/test_reactor0/test_misc/test_replay_1942525_omni_rank_reads_as_direct_play.cpp` | bug 1.3 end to end |
| `tests/test_reactor0/test_misc/test_replay_1942458_colour_mode2_walks_dc_targets.cpp` | bug 1.1 — mode 2 walks to a live dc-target |
| `tests/test_reactor0/test_efficiency.cpp` | efficiency formula + rlocks defaults |
| `tests/test_reactor0/test_giver_filters.cpp` | MISTAKE clues never offered |
| `tests/test_basics/test_snapshot_convention.cpp` | convention/rlocks snapshot round-trip + reactor back-compat |
| `tests/test_reactor0/test_decision_making/test_clue_tier.cpp` | §2a tiers — H1/H2 and each endangered-chop disqualifier, incl. the same-hand dupe and both singleton and group elim |
| `tests/test_reactor0/test_decision_making/test_pace_clue_gate.cpp` | §2a window (both boundaries, incl. `clue_tokens == 3`), required tier, and that reactor's own gate is unchanged |
| `tests/test_reactor0/test_decision_making/test_double_discard_filter.cpp` | §2b predicates — every `receiver_is_safe` clause, and the negatives that keep the filter off stable clues, colour reactives and endangered receivers |
| `tests/test_reactor0/test_decision_making/test_replay_1942181_prefers_stable_play_over_double_discard.cpp` | §2b/§2c end to end on the replay that motivated them, plus both predicates on a real Phase C |
| `tests/test_reactor0/test_decision_making/test_replay_1942330_playable_chop_lifts_clue_tier.cpp` | §2a N5 end to end — a playable non-duped chop on Bob lifts every clue, so a direct play clue beats the lock the flattened gate used to pick |
