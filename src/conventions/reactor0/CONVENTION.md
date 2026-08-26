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
  `calc_target_slot`, `target_i_play/discard`). Reactor0 does **not** use the
  four `elim_*` matrices — see §0 and §1d.2 for what replaced them;
  and most variant layers (pink promise, brownish trash reveal, the inverted
  pitch/chuck compensation on the **reactive** side, reversed suits).
  **Exception as of v4.0.0**: the *stable* side of the inverted (orange)
  compensation is reactor0's own — see §1b and the divergence table in §1f.
  v5.0.0 added the ladder's giver-side chuck veto and the orange-only rank
  chuck to that exception, and changed three shared decision-layer terms:
  `advance` simulates a playable orange with the Discard button, `eval_state`
  scores 3+ strikes at `−100`, and `eval_game` prices an orange CTD as a play
  call (reactor's §2.6/§2.7).
  **v10.0.0** adds a second exception, in the other direction: in the 102
  **Alternating Clues** and **Synesthesia** variants reactor0 gives no stable
  clues at all and takes a clue's reactive parity from its TARGET rather than
  its kind. Reactor is unchanged there and would read those clues differently.
  See §1f, *Alternating Clues and Synesthesia*.
- **Not shared**: WHEN the reactive negative inference runs. Whenever the
  receiver is called to **chuck**, the elim reasons "the slots before the target
  were passed over, so they are not playable". That is true only if the chuck is
  a real DISCARD — on an inverted suit a chuck puts the card on its stack, so it
  is a PLAY, the walk passed over nothing, and the inference is unfounded.

  Reactor applies all four elims at reaction time. **Reactor0 no longer has
  them.** Since v8.0.0 a reaction's whole negative inference is captured at
  reaction time (`Game::PendingReactionElim`) and fired when the RECEIVER
  actions their target (`Game::fire_reaction_elim`), because what the negatives
  say depends on what the receiver does — see §1d.2 for the three readings.

  This subsumes the old deferral, which released on the receiver's card
  IDENTITY: it answered only "was the chuck a play?", and could not tell a
  finesse from an ordinary double play at all.

  Replay 1966710 is the case the old deferral was built for. A rank double
  discard applied `elim_dc_dc` at once and stripped a playable b2 from the
  receiver's slot 3, but the receiver's called card was an Orange 1 with the
  orange stack on 0 — chucking it scored, so the reaction was never a double
  discard and no negative was ever owed.

- **Not shared**: how a card's **inferred** set may shrink. Once an
  interpretation has built it, reactor0 narrows it only through `card_elim` --
  that is, only on evidence that every copy of an identity is accounted for
  under COMMON knowledge. In particular `Game::update_turn`'s re-intersection of
  a standing call with the CURRENT `playable_set`
  (`src/basics/decide.cpp`) is skipped: it fires on every turn advance, so
  another player's play narrows a card their play says nothing about. Reactor
  keeps it. Good-touch elimination does not apply either, and in fact never runs
  for any convention -- `Game::good_touch` is false everywhere in the tree.

- **Not shared**: everything about *which clue to give*. As of v7.0.0 reactor0
  chooses a clue by walking the ordered **General Clue Evaluation List**
  (`choose_clue`, `src/conventions/reactor0/decision.cpp`) rather than by
  scoring candidates and taking an argmax. Its fork of `eval_action` /
  `get_result` / `clue_branch_value`, and the standalone
  pointless-double-discard filter, are all deleted. `src/basics/decide.cpp`
  splices the walk into `take_action` behind a `Convention::REACTOR0` guard.
  See [DECISION_MAKING.md](DECISION_MAKING.md), which is the ruling reference.
- **Absent by design** (present in reactor): the reactive focus, referential
  play for colour clues, response inversion and rewinds, the loadedness
  dispatcher, deferral-carries-reactive, re-tasking. Reactor0's dispatcher is
  the whole of §1a.
- The dispatch fork lives at the single engine seam:
  `src/basics/decide.cpp:57-65` (clues), `:281-288` (discards), `:342-349`
  (plays). Each fork also runs `enforce_call_invariants` (§1h) for reactor0
  games only.
- **`/allplays` is reactor-only.** It promotes reactor's colour reactives to
  play+play; reactor0's parity is fixed by clue kind, so the flag has no
  meaning here. It is never set on a reactor0 game
  (`src/net/commands.cpp:292-302`), `chat_allplays` skips reactor0 games when
  retro-applying (`:914-933`), and a reactor0 waiting connection always stores
  `all_plays = false` (`interpret_reactive.cpp:626-635`).

## §1a Dispatch — purely positional

`reactor0::interpret_clue` (`src/conventions/reactor0/interpret_clue.cpp:588-624`):

- empty clue in an empty-clues variant → `USELESS`;
- **clue to Bob → always stable**, even when Bob is loaded (`:616-621`);
- **clue to anyone else → always reactive with Bob as reacter**, even when
  the giver is locked, in the endgame, or at 8 clue tokens (`:622-623`).

The stall context (`giver obviously locked || in_endgame() ||
clue_tokens == 8`) is passed to the stable branches only as the `stall` flag
that reactor's `ref_discard` already honours.

**One family of variants overrides all of this**: under
`variants::uses_target_parity` there are no stable clues at all, the target
picks the parity, and the clued seat is **not** the receiver. See §1f,
*Alternating Clues and Synesthesia*. The dispatch reads
`clue_is_reactive` / `reactive_receiver`
(`reactor0/interpret_reactive.h`) rather than testing `action.target == bob`
directly, and so must every other site that asks the same question — everything
written above is the ordinary case those two functions reduce to.

## §1b Stable colour — a direct play clue

`stable_colour` (`interpret_clue.cpp:227-344`). **There is no referential
play in reactor0.** Priority:

1. **Fix, but only the CORRECTING kind** — `check_fix` reports a blind-play
   correction (a `CALLED_TO_PLAY` card the clue proves is trash) or a duplicate
   reveal → `FIX` (`interpret_clue.cpp`, `src/basics/fix.cpp`). Both prevent a
   strike or a wasted duplicate, so they outrank every play reading.

   A fix whose *only* content is "a previously clued card is now known trash"
   (`FixResultNormal::trash_reveal_only`) does **not**. That is a trash reveal
   in all but name, and a colour clue's primary meaning is *action the leftmost
   card you can*; being told a card is dead keeps. So it is held back and
   returned in place of the `STALL` at the bottom of this ladder — priority 6
   below, and the two earlier stall exits — i.e. only once playing has been
   ruled out. The empathy half of a fix is applied by the engine either way;
   this decides only which reading the convention reports and what it stamps.

   Replay 1969696 T34: an Orange clue in "Orange Reversed (4 Suits)" at pace 1
   touched slots 1/3/4/5, and slot 5 was a known-dead `o5`. The whole clue read
   `FIX`, the ladder never ran, and slot 1 — which could be the `o3` the
   reversed stack was waiting for — went unchucked.
2. **Play reveal** — a previously-clued card the clue fills in as a new
   **actionable** playable/connectable → `REVEAL` (`find_play_reveal`), and the
   revealed card is stamped `CALLED_TO_PLAY`, entering the receiver-CTP queue.
   **Exception:** a revealed known playable *orange* is stamped
   `CALLED_TO_DISCARD` instead, because that is the button which advances an
   inverted stack — see the orange ladder below.

   **Actionable, not merely playable.** `obvious_playables` answers "could this
   card play", which in an inverted variant is a different question from "can
   the holder act on it". A card reading `{r2, o2}` with both stacks on 1 is
   playable either way, but `r2` needs the **Play** button and `o2` needs
   **Discard**, so the holder cannot move. A clue that resolves WHICH BUTTON is
   a genuine reveal even though playability did not change, so both the before
   and after sets are filtered to the cards whose every reading sits in one
   button's candidate set (`pitch_candidates` / `chuck_candidates`).

   Replay 1967279 T8 is the worked example. A red clue pinned an already
   `{r2, o2}` card to `r2`; the old test saw no new playable, priority 2 was
   skipped, and priority 5 stamped the **leftmost touched** card with a `{m2}`
   promise the clue never made — while the r2 it actually resolved got nothing.

   The stamp is new in v7.25.0. Priority 2 used to stamp nothing and leave the
   action to empathy, which meant phase 2 had to rediscover the card through
   `thinks_playables` and the reveal carried no queue position at all.
3. **Orange play reveal** (`:255-269`) — an orange colour clue that reveals a
   playable orange is a play reveal, and the receiver **chucks** the revealed
   card. `find_play_reveal` alone does not cover it: on a colour clue it only
   considers cards that were *already* clued (`:78-83`), because a newly
   touched card becoming obviously playable is the ordinary direct-play
   reading — which is exactly the case orange has to change. `REVEAL`.
4. **The orange ladder** (`:274-314`), reached only when no playable orange
   was revealed. An orange colour clue names one orange card to get rid of or
   to stack:
   Every call reactor0 stamps is then narrowed by `narrow_to_stamped_button` to
   the identities its own button handles correctly — `pitch_candidates` for a
   CTP, `chuck_candidates` for a CTD. See DECISION_MAKING.md, "the stamp is the
   instruction"; `reactor::target_play` narrows to the PLAYABLE set, which is
   the wrong set for a pitch on an inverted suit.

   The ladder applies only to a colour clue that **names the inverted suit**
   (`names_inverted_suit`). Selecting on "touched a card that could be orange"
   is a different question wherever a variant lets a non-orange card be touched
   alongside the oranges — under Rainbow-Ones every colour clue touches every 1,
   so a blue clue would otherwise claim a Blue 1, chuck it, and pin its
   inference to Orange 1 (replay 1966119 T1).

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
   - **The chuck target is then vetted against the giver's own sight**
     (`:310-312`). `could_reach_stacks` reads common knowledge, so the
     receiver walks the same list and lands on the same card; if the giver
     can see that card is **not currently playable**, the receiver's chuck is
     a misplay strike. That is giver-only knowledge, so §1g allows only a
     **reject** — `nullopt` → `MISTAKE` → the clue is dropped from the
     candidate pool. Walking on to the next orange would desync, because the
     receiver still computes the first one. The **pitch** branch needs no such
     veto: a pitch cannot strike. Replay 1957905 #31 is the worked example —
     the ladder chucked an Orange 1 that was already on the stacks, for the
     third strike (bug_report_4_1_0.txt).
   - all-critical fallback: the chuck loop runs regardless of what the pitch
     branch found, so if every touched orange is known critical the leftmost
     one that could still reach the stacks is chucked. If none could →
     `STALL` (`:315`).
5. **Direct play** — the **leftmost card touched by this clue** whose common
   empathy could be playable (playable set ∪ delayed-play successors) is
   called to play via `target_play` (`leftmost_could_be_playable` `:203-223`,
   guards + call `:318-339`). The guards are reactor's `ref_play` rejections:
   blind-playing target, CTD'd-and-not-visibly-playable target. There is **no
   longer an inverted-target reject** — it read `state.deck[*target].id()`,
   which is POV-asymmetric (nullopt for the receiver's own card), and the
   orange ladder above now claims every touched card that could be orange.
6. Otherwise the receiver knows none of the touched cards can play →
   `STALL` (`:343`).

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

`stable_rank` (`interpret_clue.cpp:348-584`). The pink-promise gate runs
first (`:360-362`), then the rank is classified (`:364-450`).

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
2. **per-card visibility**, via `reactor::effective_possible_for` (`:393-405`)
   — the card's `possible` narrowed by the copies visible in every non-holder
   hand. That is POV-invariant by construction (defined from the **holder's**
   viewpoint, so every seat computes the same set), which is why it is safe to
   read here at all. Plain `common.thoughts` is not enough.
3. **a MIXED useful set is not playable by this clue** (`:442-443`). A rank
   direct play clue means pitch by default, and pitching an orange card sends
   it to the discard pile rather than its stack. When the rank's useful
   identities include both orange and non-orange ones the receiver cannot
   tell which button to press, so the reading declines — a rank-1 clue at
   stacks of 0 still cannot be used to get an Orange 1 onto the stacks.
   **When every useful identity is orange it can** (`orange_only`, `:442`):
   all other suits' copies of the rank are already on the stacks, so the clue
   names the orange card and nothing else, the button is unambiguous, and the
   clue is a direct play clue actioned as a **chuck** (priority 1 below).
   Replay 1957905 #31 is the worked example: at stacks `[5, 2, 5, 1]` a rank-2
   clue can only mean the playable Orange 2 (bug_report_4_1_0.txt).

   The orange-only reading is the one case where priority 1 **defers to the
   play reveal of priority 2** (`defer_to_reveal`, `:452-462`). When the clue
   pins a previously-clued orange to a playable one the reveal already says
   everything and empathy carries the chuck
   (`src/basics/decide.cpp:889-908` routes an empathy-pinned playable orange
   through PerformDiscard). Claiming it at priority 1 would also trip the
   `unnecessary_focus` test, which counts the focus's **own** pinned identity
   as "visible elsewhere" (`Thought::matches` is `id() == other`,
   `src/basics/card.cpp:48-52`) and would turn the reveal into a `STALL`. An
   ordinary direct play clue still outranks the reveal, as before.

Why it matters: an omni suit is pinkish, so `Variant::id_touched` returns true
for it on **every** rank clue (`src/basics/variant.cpp:230`), and the
variant-wide set for a rank-N clue therefore contains the omni suit at ranks
1-5. A single useful-but-unplayable omni rank made `playable_rank` false, so
priority 1 essentially never fired in omni variants and every rank clue
degraded to the referential discard at the bottom of this ladder. Replays
1942517 #1 (a rank 1 at all-zero stacks read as a referential discard) and
1942525 T53 (a playable Sky 4 never called) are the worked examples.

An empty narrowed set teaches nothing and is treated as neither
all-trash nor playable-rank (`:446-450`), rather than vacuously true.

1. **Direct play clue** (`:452-531`) — every remaining useful identity the
   touched cards can hold is playable (assuming good touch). Focus = leftmost
   **newly** touched card — highest order, since slot 1 is newest — in both
   the plain branch (`:474`) and the pinkish one
   (`variants::playable_rank_focus`, `:470`; that helper returned the
   *rightmost* until v3.0.0); with no
   newly touched cards, the leftmost **touched** card that could be playable.

   **Odds and Evens focuses from the RIGHT** (`rightmost_could_be_playable`).
   One rank clue there names a whole parity class, so an odd clue sweeps up
   1s, 3s and 5s together; the promise is the **rightmost newly touched card
   whose empathy is not entirely unplayable**, and with none newly touched,
   the rightmost touched one. Note the *condition* above already ranges over
   the parity class — `touchable` is filtered through `Variant::id_touched`
   (`:623-626`) — so only the focus differs. Every other variant, pinkish ones
   included, keeps the leftmost rule.
   The focus is narrowed to its playable identities, `info_lock` set, and
   stamped **`CALLED_TO_PLAY` — a pitch — unless the classification said
   `orange_only`, in which case `variants::called_focus_status` supplies
   `CALLED_TO_DISCARD`, a chuck** (`:496-525`).

   **The inferences narrow to whatever the stamped button advances**
   (`:593-605`). Under `orange_only` that is the **inverted** playables, the
   same narrowing `stamp_orange_chuck` does, which keeps TODO #12's
   unpinned-playable-orange hazard away from the focus. Otherwise it is the
   **plain** playables: pressing Play on an inverted card sends it to the
   discard pile, so an inverted identity is never what a pitch call means.

   A **mixed** useful set — plain and inverted identities both playable — is a
   direct play clue like any other. It used to set `playable_rank = false` and
   decline outright, on the grounds that the receiver could not tell which
   button to press. They can: the default is Play, and the inferences narrow to
   match. Declining cost replay 1966696 an entire clue — a rank-1 on
   `{r1,b1,o1}` at stacks `[0,0,0]` degraded to a bare `REVEAL` that stamped
   nothing, and will-bot67 carried an unstamped playable 1 for six turns before
   discarding its chop on T8.

   The giver carries the matching duty: they can see the receiver's hand, so
   they must not give a mixed rank clue whose focus is actually inverted — the
   receiver will pitch it into the discard pile
   (`RankDirectPlayPitchesAMixedUsefulSet` keeps that hazard concrete).

   **reactor calls `called_focus_status` unconditionally at its own
   site** (`src/conventions/reactor/interpret_clue.cpp:504`).
   Returns `PLAY`. An *unnecessary* focus (every possibility trash or
   visible elsewhere) makes the clue a `STALL` instead (`:482-494`).
2. **Play reveal** (`:532-537`) — the clue fills a previously-clued card in
   as a new obvious playable, without the whole rank being playable →
   `REVEAL`, no stamp; empathy carries it. **Terminal, and ranked above every
   other reveal and above the referential readings**: the receiver has a play
   to make, which outranks being told what to discard. So a rank clue that
   both fills in a playable and touches the lock slot is a `REVEAL`, not a
   `LOCK`.
3. **Trash reveal** (`:539-556`) — every touchable identity is trash. The
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

   **Under Odds and Evens the rank promise is a PARITY promise**
   (`variants::rank_satisfies_promise`, `src/conventions/variants/pinkish.cpp`).
   The clue value names a class, not a rank: an odd clue promises the lock-slot
   card is a **1, 3 or 5**, an even clue a **2 or 4**. Reading it as the literal
   rank collapsed replay 1971788's lock slot onto `{r1,y1,g1,b1,p1}` — all
   already trash — when the card was a Dark Omni 5, and sixteen turns later it
   was thrown as known trash. Parity is a property of the rank, so it binds an
   Omni card like any other; it is deliberately not routed through
   `Variant::id_touched`, which is true for every rank of a pinkish suit and
   would make the promise vacuous on exactly that suit.
7. **Stall** (`:535`) — no new cards and nothing above fired.

## §1d Reactive — the clue value is the anchor

`reactor0::interpret_reactive` (`interpret_reactive.cpp:605-646`). There is
no reactive focus. The anchor is:

> **react_slot + target_slot ≡ anchor (mod hand size)** where
> **anchor = the rank** for rank clues, and **the fixed colour value** for
> colour clues (`:617-625`; `calc_slot` is reactor's,
> `src/conventions/reactor/interpret_reaction.cpp:16-19`).

The waiting connection stores the anchor in `ReactorWC::focus_slot` plus a
clue-time snapshot of `allow_reactive_locks` in `ReactorWC::rlocks`
(`:626-635`). The receiver never selects targets — their POV returns
`REACTIVE` immediately and decodes positionally at reaction time (`:643-645`).

**Candidate walks are transactional** (`Rollback`, `:53-72`). Every phase
snapshots the game immediately before its first mutation and restores on each
abandoning path, including the terminal `MISTAKE`. Without it, `target_play` /
`target_discard` — which mutate even when they fail — leave a play call that
no clue ever made; being same-turn, such a call competes with the real one for
being actioned first, and when it is played the receiver has nothing to
interpret.

### The reactive table — two parity buckets

Every clue sits in one of two **parity buckets** and carries a **reactive
value** (its anchor) there:

| bucket | meaning |
|---|---|
| **even** | a double play, or a double discard |
| **odd** | exactly one play |

Normally rank clues are the even bucket and colour clues the odd
(`variants::uses_even_parity`); Odds and Evens swaps them. A rank clue's value
is the rank (odd→3 / even→4 under Odds and Evens); a colour clue's comes from
the table below.

`reactive_assignment` (`reactor0/reactive_assignment.h`) answers both questions
for one clue and is what the interpretation reads — the anchor and the
`reactive_rank` / `reactive_colour` dispatch both come from it.

**`/set <clue> odd|even <value>`** moves a single clue between buckets and gives
it a value, overriding the variant's assignment
(`Game::reactive_overrides`). `/settings` prints the result:

```
reactor0 — even reactive values: {1=1, 2=2, 3=3, 4=4, 5=5}, odd reactive values: {Red=1, Yellow=2, Green=3, Blue=4, Purple=5}, rlocks: on
```

after `/set Yellow even 4`:

```
reactor0 — even reactive values: {1=1, 2=2, 3=3, 4=4, 5=5, Yellow=4}, odd reactive values: {Red=1, Green=3, Blue=4, Purple=5}, rlocks: on
```

Rows are enumerated rank-first then colour, so a moved clue appears at the END
of its new bucket. An **empty** override list reproduces the built-in table
exactly, which is every game that has not used the command.

The command retro-applies to running games, but a reaction already in flight
keeps the meaning it was given with: the parity binds into `ReactorWC::even_parity`
at clue time, the same insulation `wc.rlocks` provides.

**The settings persist between games.** They are stored bot-wide on
`BotClient::reactive_overrides_` and copied into every new `Game` by `on_init`
(`src/net/commands.cpp:325`), so a value set in the lobby — or inside an open
replay — holds for the games that follow, across game starts, reconnects and
reattends. `/set` works from a PM or a table room, and inside a replay, which
never appears in the lobby table list (`table_info`, `commands.cpp:824`, falls
back to the game's own variant and seat count).

**A change of variant resets them.** `ReactiveOverride::clue_value` is a raw
index into `Variant::clue_colour_names`, so an entry authored against one
variant would point at a different colour on another. `overrides_for`
(`commands.cpp`, called from `on_init` and `chat_settings`) is the single
applicability test: a game on another variant gets that variant's built-in
table. It is non-destructive — the stored list survives, so opening a replay of
a different variant does not cost the settings — but the first `/set` ON a new
variant clears the list rather than merging into it, since the dedupe key is
`(kind, clue_value)` and stale entries would collide by index.

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

`reactive_rank` (`interpret_reactive.cpp:284-460`):

- **Phase A — double play** (`:296-355`). The target pool is the receiver's
  playable cards, slots ascending — **including already-CTP'd cards**
  (`play_pool`, `:83-92`; the include-CTP'd rule is a deliberate reactor
  divergence). For each target leftmost-first: compute the react slot, vet it
  (`vet_react_slot`, below), then stamp — the reacter urgent CTP (blind play)
  and the receiver target is left for reaction time (§1d).
  **Inverted-suit swap:** when the receiver's target is on an inverted suit the
  reacter is stamped **CTD** instead (`:338-341`), so that the receiver's
  standard even-parity reading ("the reacter discarded → I discard my target")
  lands on the chuck that advances the orange stack. The receiver's own stamp
  is made at reaction time (§1d), so the swap only decides selection here.
- **Phase B — finesse** (`:357-420`). Walked by **target**, leftmost one-away
  first (reactor walks react slots in a fixed order instead). The reacter
  must hold the connector — direction-aware, so `next()` on a reversed suit
  and `prev()` elsewhere (`variants::connector_of`,
  `include/hanabi/conventions/variants/reversed.h`) — via
  `effective_possible_for(react).contains(connector)`, else the next one-away
  is tried.
- **Phase C — the trash targets** (`:422-457`). Normally a double discard,
  zero plays: the reacter **discards** the react slot (urgent CTD) and the
  receiver's dc-target resolves at reaction time. The dc-candidates are
  **walked** leftmost-first, exactly as colour mode 2 walks them — a
  known-critical react slot advances to the next candidate.
  **Inverted-suit swap:** an inverted dc-target is shed by **pitching** it
  (Play), so even parity puts the reacter on **Play** too — a blind play, or
  `stamp_orange_pitch` when his own slot is a known orange, mirroring Phase A's
  inverted arm.

### Vetting the react slot follows the swap

`vet_react_slot` (`:187-262`). Every reactive path swaps the reacter's action
when the receiver's target is inverted — rank Phase A goes play → **discard**
and Phase C discard → **play**, colour mode 1 goes discard → **play** and
mode 2 play → **discard** — so the question asked of the react slot has to swap
with it. The swap direction differs between the play targets and the trash ones
because the button that ADVANCES an orange stack (Discard) is not the button
that THROWS an orange away (Play). A third case sits above both: when the react card is one
the holder knows is an **expendable orange**, a "play" call is neither a blind
play nor a chuck but a **pitch**, and a pitch is unconditionally safe.

| the reacter will | the vet asks | on failure |
|---|---|---|
| **pitch** (play call on a react card the holder knows is an expendable orange) | nothing — a pitch is always legal | — |
| **play** | `effective_possible_for(react)` intersects playables ∪ delayed-play connectors | RETARGET (shared) |
| **play** | the react card's **actual** id is playable or a connector | REJECT (giver-only) |
| **discard** | not every possibility is critical | RETARGET (shared) |

The three outcomes are §1g's split, and each call site derives `reacter_plays`
from the same `variants::target_is_inverted` test that drives its swap
(`:306-320` rank, `:490-505` colour).

Vetting the un-swapped call is bug_report_4.txt 4.1, in both directions.
Asking a **discard** call for playability throws good clues away: at replay
1942777 #10 ("Funnels & Orange") the receiver's only playable was an Orange 2,
whose react slot held a clued Blue card with both Blue 3s visible across the
table — the vet failed, Phase A skipped the only play target, and the clue
collapsed into Phase C's lock, so the reacter discarded slot 3 instead of
slot 5. Asking a **play** call for criticality is worse: colour mode 1 would
blind-play a react slot with no playability check at all, which strikes.
`variants::would_lose_inverted_reacter` was already swap-aware at both sites,
which is why only this vet was wrong.

**"Safe to throw away" excepts a playable inverted reading.** The discard vet
asks whether every reading of the react slot is critical, and retargets when
one is (`every_reading_loses` `interpret_reactive.cpp:232-239`). That is the
PLAIN-suit reading of the Discard button: on an inverted suit Discard is a
CHUCK, which puts the card on its stack, so a reading that is inverted *and*
playable is not a loss at all — it is the play the call is asking for. Without
the exception the vet was unsatisfiable in Dark Orange, where every card is
one-of-each and therefore critical by construction: a react slot that could be
dark was *always* walked past, which is precisely the shape where the chuck is
most clearly right. Replay 1967491 T36 — will-bot67's slot 5 was `{d2, d4}`
with the dark stack on 1, so chucking it stacks the `d2`.

**A re-targeted slot gets its inferences rebuilt.** When the reacter acts,
`target_i_play` narrows the receiver's target to its playable identities. If
`inferred` no longer admits any playable, reactor0 rebuilds the narrowing from
`possible` rather than skipping it (`reactor/interpret_reaction.cpp:150-172`).
The call is direct evidence of playability and outranks the derived negative
that emptied the set. Skipping it made the call evaporate silently: the stamp
landed, and `enforce_call_invariants` rule 3 dropped it again as a dead play
call, so the card ended the turn with no stamp and no note. Replay 1966710 —
will-bot67's blue 2 on slot 3 lost b2 to a T6 negative, was re-targeted at T8,
and discarded its chop at T13 instead of playing.

**The resolution side has to honour the swap too.** When the reacter acts, the
receiver's target is re-stamped by the shared `target_i_play` /
`target_i_discard` (reactor's §2 "Resolving the reaction"). For an inverted
target the physical label is CTD, so resolution lands in `target_i_discard` —
correctly, since pressing Discard is what stacks an orange. What was wrong is
that the helper applied *throw-this-away* semantics to it: narrowing `inferred`
to the non-critical ids, which in Dark Orange (every card `oneOfEach`, so every
card critical) empties the set, marks the card trash and lets `elim` void the
call. bug_report_6_2_0.txt, replay 1959065 T5-T6 — the chuck signal was
destroyed one turn after it was given, and the receiver's playable Dark Orange
2 was left neither playable nor discardable. **Reactor still vets the un-swapped call
at all four of its reactive sites** — see TODO.md.

**The pitch row is bug_report_4_1_0.txt 4.1.0b** (replay 1957942 T19). A play
call on a card the holder knows is orange sends it to the discard pile, so the
only question worth asking is "can the team afford to lose it?" — but the vet
asked the play question, which a basic-trash identity can never answer yes to
(it is in neither `playable_set` nor the connectors). Phase A therefore skipped
the pairing and walked on to one whose react slot was a critical Yellow 5.
Three gates had to move together, all scoped to `variants::can_pitch_for_free`
(`variants/inverted.cpp:85-92`) — every possibility inverted **and** basic
trash, read off `common`, so the reacter walks with the giver:

- the vet itself short-circuits to `OK` (`:230-232`);
- `would_lose_inverted_reacter` is skipped (`:342-350`). Its blanket "a
  play-type call on an orange loses the copy for nothing" is true of a *useful*
  orange only; a trash one has no copy to lose. The guard is POV-asymmetric by
  design and may only reject, so the exemption that bypasses it has to read
  `common` — which is why it is a separate predicate rather than a change to
  the guard;
- the stamp is `stamp_orange_pitch` with `urgent` (`:358-366`), not
  `target_play` — the latter narrows `inferred` to the playable set and bails
  when that empties, so it cannot stamp a trash card at all.

A fourth gate lives outside the convention: `decide.cpp:715-726` skips an
urgent `CALLED_TO_PLAY` whose empathy is all basic trash, which would have
swallowed the stamp on the reacter's own turn. It carries the same exemption.

Scoping to **trash** rather than to "orange" is deliberate and is what keeps
`test_pov_reject.cpp:24`'s rejection correct: that fixture's react slot is an
Orange 3 at a stack of 0 — useful, so pitching it still loses a copy.

### Colour reactive — one play

`reactive_colour` (`interpret_reactive.cpp:462-599`):

- **Mode 1 — receiver has a playable** (`:477-533`): the reacter **discards**
  the react slot and the receiver plays the target (leftmost playable;
  a react slot holding a known critical advances the target). **Inverted-suit
  swap:** for an orange target the reacter is called to **play** instead
  (`:522-525`), and the vet swaps with it — see above.
- **Mode 2 — the trash targets** (`:535-598`): the reacter **blind-plays** the
  react slot. **Inverted-suit swap:** an inverted dc-target is shed by
  **pitching** it (Play), so odd parity puts the reacter on **Discard** —
  `stamp_orange_chuck` when his own slot could be a playable orange, otherwise
  `target_discard`. The playability checks below are skipped there; they exist
  only for the blind play. Replay 1974257 T30.
  The dc-candidates are **walked**, not fixed (`:492`): a pairing
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

`dc_candidates` (`interpret_reactive.cpp:123-174`):

1. cards whose actual identity is **basic trash** or a **same-hand dupe**,
   slot-ascending — regardless of cluedness or of any status already stamped
   on them. This is a deliberate divergence from reactor, which reorders and
   filters its pool: here the receiver derives the target from hand position
   alone, so a standing `CALLED_TO_DISCARD` cannot skip a card.
   **Inverted-suit cards are in the pool**, flagged `DcTarget::inverted`; a
   critical one is not, since a pitch throws the card away and there would be
   nothing to spare. Until v10.6.0 they were excluded outright, which was right
   about the Discard button and blind to the other one — see §1f.
   **Both buckets walk** the pool leftmost-first
   (`all_trash_targets=true`); rank Phase C kept a strict-leftmost rule until
   v10.6.0;
2. no such card and **rlocks on** → the single candidate is the **oldest
   slot**, flagged as the lock;
3. no such card and **rlocks off** → reactor's sacrifice ordering
   (`reactor::sacrifice_targets`).

Under rlocks, a trash candidate that happens to sit on the oldest slot is
*also* flagged as the lock — see §1e.

## §1d.1 A deferral keeps the reacter call

**Giving a clue does not cancel your own pending reacter call.** The call stands
until it is ACTIONED, and the holder remains *occupied* — so the HIGH-tier gate
applies to any clue they do offer, and the urgent return chucks or pitches the
called slot as soon as nothing outranks it.

**But it stops being URGENT once its target is gone.** The reaction interrupts
the turn because the RECEIVER decodes it — he learns which of his own slots the
clue named from which of ours we action — so once that paired card has left his
hand there is nobody left to inform. The call and its reading survive; only the
urgency lapses, and the turn falls through to the ordinary clue and play phases.
Because a deferral clears `Game::waiting`, the pairing is recorded on the card
itself when the call is stamped (`ConvData::react_target_order`, written by
`record_react_target` in `interpret_reactive.cpp`, read by the urgent scan in
`basics/decide.cpp`). Replay 1972716 T5: will-bot69 deferred at T2, the receiver
played the paired card at T3, and at T5 the spent call still pre-empted the
stable play clue Bob's chop was owed — costing a playable g3 and a playable y1.

This is a reactor0-only rule (`basics/decide.cpp`, the `convention !=
REACTOR0` guard on the giver-side `check_missed`). Reactor cancels: there a clue
instead of a reaction means the chain broke, and `check_missed`
(`basics/game.cpp:95-118`) clears the stamp and reverts `inferred` to
`old_inferred`. Reactor0 cannot do that, because its Precedence puts a **VERY HIGH
clue above the urgent return** — a clue is exactly what a legitimate deferral
looks like, so cancelling on one punishes the convention's own rule.

The play/discard call sites are untouched for both conventions: there the player
really did act and skipped their urgent card.

Replay 1966745: will-bot69 was stamped an urgent CTD on an Orange 1 with the
orange stack on 0, deferred at T2 to give a finesse, and lost the call. At T5
`requires_high_tier` therefore reported it unoccupied, the unoccupied MEDIUM gate
admitted a non-HIGH clue instead of the chuck, and the team struck on that same
card at T16.

**Known gap.** The waiting connection is still cleared on a deferral
(`decide.cpp`), so the receiver may never learn the target its reaction was meant
to reveal. See `TODO.md`.

**The endgame solver breaks ties toward a standing call.** Precedence step 0 is
the endgame fork, which sits above the urgent return, so in the endgame the
solver — not this convention — chooses the action. Among lines it rates equal it
now takes the one the standing reacter call names (`src/endgame/solver.cpp`,
reactor's §1b.5). It may still deviate when deviating genuinely wins more often.
Replay 1966757 T25 is the case: the target scan was correct, and the solver
overrode it with an equally-rated chuck that struck.

**Every reacter stamp narrows to its button.** A call is stamped by reactor's
shared `target_play` / `target_discard`, whose narrowing is the PLAIN-suit
reading — `target_discard` drops the criticals ("do not throw away a critical"),
which on an inverted suit is exactly backwards, since a chuck there is a play
attempt and the playable orange 5 is critical. `narrow_to_stamped_button`
(`interpret_clue.cpp`) corrects it to `chuck_candidates` / `pitch_candidates`,
and **all four** reacter sites call it: Phase A, Phase B, Phase C, and colour
mode 1.

Phase C and colour mode 1 did not until v7.28.0. Replay 1967376: an Odds and
Evens rank clue runs the odd bucket, which is colour mode 1's ruleset, and the
reacter-CTD kept a trash `o1` while dropping the playable `o5`.

Colour mode 1 goes further as of v7.30.0: when the react slot could be a
playable inverted card it is stamped by `stamp_orange_chuck` outright rather
than by `target_discard` (`interpret_reactive.cpp:587-598`). Narrowing after
the fact is not enough there, because `target_discard` *refuses to stamp at
all* when no non-critical id survives — which in Dark Orange is always — and
the whole clue then reads as a `MISTAKE`. This is the same rule as the vet
above, one layer down, and the same stamp the stable orange ladder uses
(`interpret_clue.cpp:266`).


**The receiver is stamped only AFTER the reacter acts.** Until v8.0.0 two of
the five reactive paths — rank Phase A and colour mode 1 — stamped the
receiver's target at CLUE time, via `stamp_receiver_play`. The other three
(Phase B, Phase C, colour mode 2) already waited. All five now wait, and the
clue-time function that remains, `receiver_call_is_viable`
(`interpret_reactive.cpp`), is a **target-selection veto only**: it says whether
a pairing is worth choosing and writes nothing.

The reason is that the early stamp had to be rebuilt later anyway, and the
rebuild was not a narrowing. Replay 1967558: will-bot67's slot 1 was stamped at
clue time and narrowed to `{r1,y1,b1,p1,w1}`; the reacter then played a `p1`, and
`target_i_play` re-derived from the live `playable_set` to give
`{r1,y1,b1,p2,w1}` — `p1` correctly left, but **`p2` was added**. Building the
set once, when the reaction has actually happened, removes the whole class.

**The receiver's set is read against TWO states.** A play has to land on the
stacks the reacter LEAVES BEHIND; what the receiver can afford to throw away was
settled when the clue was given. So, with OLD as of the clue and NEW as of after
the reaction (`receiver_ctp_set` / `receiver_ctd_set`, `interpret_clue.cpp`):

```
receiver-CTP = {plain ∧ playable(NEW)} ∪ {inverted ∧ ¬playable(OLD) ∧ ¬critical(OLD)}
receiver-CTD = {plain ∧ ¬playable(OLD) ∧ ¬critical(OLD)} ∪ {inverted ∧ playable(NEW)}
```

intersected with the card's own empathy. Passing the same state twice recovers
`pitch_candidates` / `chuck_candidates`, so the stable side and call-invariant
rules 3-4 are literally the same function. Clue-time selection evaluates the
identical reading via `state_after_reacter` (`decision.cpp`), which `read_clue`
also uses — so prediction and resolution cannot disagree about what "after the
reaction" means.

reactor0 owns this stamp rather than wrapping reactor's `target_i_play` /
`target_i_discard`. Wrapping would itself be an illegal widen: `target_i_discard`
narrows to the NON-critical ids and the inverted arm of a CTD is frequently
critical, and `target_i_play` pins `info_lock` to a set built the wrong way — an
`info_lock` survives every reset, so a wrapper could not undo it.

**The parity row is the BUTTON, never the outcome.** What matters is whether the
reacter pitched or chucked, independently of whether an inverted card happened to
reach its stack: a chuck that strikes was still a chuck. The engine already
applies the inversion (`Game::on_discard` advances the stack for a physical
discard of an inverted card), so **the hook IS the button** and no suit test is
needed — `react_play` means Play was pressed, `react_discard` means Discard was.
The one ambiguity is a misplay, which arrives as a failed discard whichever
button produced it; there the suit decides, since a plain card can only strike
via Play and an inverted one only via Discard
(`reacter_button_pressed`, `interpret_reaction.cpp`).

This closes TODO 24. It could not be read off the stamp: from the RECEIVER's own
seat `interpret_reactive` returns before any phase runs, so the reacter carries
no status and `wc.react_order` is `-1`. For the same reason the dupe-bluff below
reads the order the engine hook was given, not `wc.react_order`.

### §1d.1 Bluff and dupe bluff

If the receiver-CTP set comes out **empty** and the reaction advanced a
**non-inverted** stack, the reacter did not play what the pairing predicted.
Two accounts remain, in order:

- **Bluff.** Unwind to the OLD stacks and take every identity exactly one away
  from playable on a non-inverted suit, intersected with the card's empathy.
  Example (`r1` on the stacks, No Variant): Alice clues 3 to Cathy touching all
  five, Bob plays a `b1` from slot 1, and `calc_slot(3,1,5) = 2` names her slot
  2. No 3 is playable after the `b1`, so unwinding gives one-aways
  `{r3,y2,g2,b2,p2}`, and only the `r3` is a 3.
- **Dupe bluff.** If not even that survives, the receiver holds the other copy
  of the card the reacter just played. Example (stacks `1 1 1 3 1`): Alice clues
  4 to Cathy naming a lone 4 on slot 3 and Bob blind-plays `b4`. No 4 is
  playable, and the old one-aways `{r3,y3,g3,b5,p3}` contain no 4 — so the card
  is the other `b4`.

In **both** the CALL IS DROPPED and only the inference is kept: neither card is
playable now, so pressing the button would strike or throw a card away. The
ordinary machinery collects each later — the bluff target when its connector
lands, the dupe via the chuck list. The withdrawal is noted with
`NoteMark::RESET`, because the receiver was never stamped and so there is no
CTP → NONE transition for `notes.cpp` to catch on its own.

Plays only. An empty receiver-CTD set has no such account and goes straight to
§1i's ladder.

### §1d.2 The negative inference waits for the receiver

A reaction's negatives say "the slots the walk passed over were not playable".
Which slots, and which set, depend on what the receiver does with their target,
so the whole inference is captured at reaction time
(`Game::PendingReactionElim`) and fired when the receiver actions that card
(`Game::fire_reaction_elim`). What they put on the table decides between three
readings:

| the receiver | reading | eliminates |
|---|---|---|
| advanced a stack, NOT the reacter's | ordinary double play | direct playables, on the passed-over slots only |
| advanced the SAME stack the reacter did | **finesse** | direct playables across the whole hand, **and** one-away identities on the passed-over slots |
| advanced no stack | they discarded | direct playables across the whole hand |

An inverted CHUCK advances a stack exactly as a plain play does and a PITCH
stacks nothing exactly as a plain discard does, so the stacks answer this
without any button or suit test. Everything is read as of the REACTION —
"playable" and "one away" describe the position the clue was given into, not
whatever the stacks look like when the receiver gets round to acting. A card
carrying its own call is left alone; that call speaks for it.

**And the alternative has to have existed.** Every one of these negatives is an
argument of the form *"if that slot had been an X, the clue would have named it
instead"* — which only holds if the clue COULD have named it. The pairing
`react_slot + target_slot ≡ anchor` means naming receiver slot `S` would have
required the REACTER to action his slot `calc_slot(V, S, H)`, pressing the
button that reading needs, on a card that could bear it. When his paired slot
could not, no such clue was ever available and the negative is unfounded.

So each slot's eliminable identities are worked out at capture time and stored
already filtered:

| the negative | earned only if the reacter's paired slot could |
|---|---|
| **directly playable** `N` | press the button the parity pairs with the receiver's. The receiver advances a stack with `N` by pressing Play on a plain suit and Discard on an inverted one; **even** parity means the reacter matches that button, **odd** that he opposes it. "Could press Play" means some candidate is a plain playable or an expendable inverted card; "could press Discard" means some candidate is a non-critical plain card or a playable inverted one. |
| **one away** `N` (finesse) | hold the exact connector — `variants::connector_of`, so reversed suits follow their own direction. Only ever in the **even** bucket: a finesse is a double play. |
| **trash** `N` | the mirror of the first row: the receiver sheds trash with the *other* button, so the parity test flips. |

The candidate set is `reactor::effective_possible_for` — the reacter's own
empathy as every seat reconstructs it, common knowledge minus the copies
visible outside his hand. Using that rather than what one seat can SEE is what
keeps this POV-invariant: Alice, Bob and Cathy draw the same negatives, so
nobody's model of the receiver's hand drifts from the receiver's own. The
asymmetry cannot be resolved perfectly — Alice and Bob know things about Cathy's
hand that Cathy does not — but this is the sharpest test all three can agree on.

Replay 1971882 is the case. An r4 was struck off the receiver's slot 4 as "not
one away", but the anchor was 3, so naming that slot would have needed the
reacter to blind-play an r3 from his slot `calc_slot(3, 4, 5) = 4` — whose
empathy was `{g1, g3, g5, d3}`. Twenty-three turns later the bot could not see
the red finesse that would have won the game. Replay 1970589 is the same defect
costing a Dark Orange 4: the receiver's p3 was written off, so with no playable
in sight the bot chucked its urgent CTD, and on an inverted suit a chuck is a
play attempt.

This replaces the four `elim_*` matrices and the knowledge-triggered
`PendingDcElim` hold, which released as soon as anyone could prove the called
card was or was not a playable inverted. That answered a narrower question and
could not tell a finesse from an ordinary double play at all.

## §1e The reactive lock and `allow_reactive_locks`

**Resolution** (`src/conventions/reactor0/interpret_reaction.cpp`): when the
reacter acts, `calc_target_slot` maps their slot to the receiver's target, and
`receiver_button` decides which button the receiver is handed — **even** parity
means they press the same one the reacter did (double play or double discard),
**odd** means the opposite (exactly one play). Reactor0 calls none of reactor's
four `elim_*` matrices; the whole negative inference is captured and deferred,
see §1d.2.

Parity keys on **`wc.even_parity`**, snapshotted at clue time
(`wc_even_parity`), not on `wc.clue.kind` — that is what lets Odds and Evens
swap the two kinds and what insulates an in-flight reaction from a mid-game
`/set`. It is otherwise `variants::uses_even_parity` on the clue kind;
`wc.all_plays` is deliberately not consulted, because reading it let
resolution contradict the reading every seat agreed on at clue time. Should a
waiting connection carry the flag anyway (a replayed snapshot, or a reactor WC
resolved under reactor0), the agreement is play+play, so the reacter has no
discard available to them at all: a discard is then a **known mistake**
(`DiscardInterp::MISTAKE`) that applies no marks (`:96-106`). Reactor0 never
rewinds — both entry points always return false.

**The lock reading** (`is_lock_target` `:26-29`, `reactive_lock` `:49-63`):
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

### Matryoshka — no rule change

Suits Ruby, Yam, Geas, Beatnik, Plum, Taupe, with **nested** colour touches:
Red reaches every suit, Yellow everything from Yam on, Green from Geas on, Blue
from Beatnik on, Purple only Plum and Taupe, Teal only Taupe.

No convention rule implements this, and none needs to. The nesting is data:
`data/suits.json` gives each suit exactly the colours that reach it (Ruby
`["Red"]` … Taupe `["Red","Yellow","Green","Blue","Purple","Teal"]`), and
`Variant::id_touched`'s colour branch matches the clue colour NAME against that
list. `make_variant` derives `clue_colour_names` in order of first appearance,
which for these six yields `[Red, Yellow, Green, Blue, Purple, Teal]`. None of
the six names trips a `SuitType::of_name` substring flag, so every suit is
plain. Pinned in `tests/test_basics/test_variants.cpp`.

### Odds and Evens — the two clue kinds swap reactive roles

A rank clue names a **parity**, not a rank: value 1 is "odd" and touches ranks
1/3/5, value 2 is "even" and touches 2/4 (`Variant::id_touched`, guarded by the
`oddsAndEvens` flag and sitting below the pinkish / brownish / special-rank
branches, which keep their own rules). `clueRanks` restricts the offered values
to `{1, 2}` (`State::all_valid_clues`).

The reactive roles swap with it:

| | normally | Odds and Evens |
|---|---|---|
| **even** parity — double play, or double discard | RANK | COLOUR |
| **odd** parity — exactly one play | COLOUR | RANK |

One predicate owns the swap — `variants::uses_even_parity`
(`conventions/variants/predicates.h`) — and every reactive site reads it rather
than testing `ClueKind` directly: the clue-time dispatch and the resolution
parity in both conventions.

**Anchors.** A rank clue's value can no longer serve as its own anchor, so it
maps **odd → 3, even → 4** (`variants::rank_reactive_value`). Colour anchors are
unchanged. Anchors 3 and 4 therefore become reachable from either clue kind;
that is unambiguous because the clue KIND still selects which parity ruleset
applies. Both conventions use the same mapping, so they read a given clue
identically — reactor's anchor is normally POSITIONAL for rank, and Odds and
Evens overrides it exactly as pinkish already does.

**Stable clues are NOT swapped.** A colour clue to Bob is still read by
`stable_colour` and a rank clue by `stable_rank`. Only §1c's touchable filter
changes, and only under this flag: `rank == value` is meaningless for a parity
class, so it defers to `id_touched`.

**Not inherited: the stable side of the orange compensation.** From v4.0.0
reactor0 owns its own readings for inverted suits, and they are a
cross-version compatibility break with reactor. **v5.0.0 widens the second
row**: a rank clue whose useful identities are all orange is now a direct play
clue actioned as a chuck, which is a further break with any partner on an
older build.

| | reactor | reactor0 |
|---|---|---|
| Stable colour naming an orange | `ref_play`; `target_play` on an inverted target is rejected as a mistake | the §1b orange ladder — play reveal, then pitch or chuck, then a §1g reject if the giver can see the chuck target is unplayable |
| Rank direct play with a possibly-orange focus | `called_focus_status` → CTD (a chuck) | `CALLED_TO_PLAY` (a pitch) for a mixed useful set, with the inverted identities dropped from the inferences; `called_focus_status` → CTD only when **every** useful identity of the rank is orange |
| `pace()` | not consulted for orange | selects pitch vs chuck |
| Dark Orange | no special handling | always chucks |

The new predicate is `variants::includes_dark_inverted`
(`src/conventions/variants/predicates.cpp:32-37`), which like
`includes_inverted` reads the real `SuitType` flags rather than matching suit
names.

#### Pressing Play on a known orange is a PITCH (v10.3.0)

When **every** reading of the reacter's card is inverted, the Play button cannot
strike — it discards — so the call is a pitch and two things follow, both of
which the code had wrong on the odd-parity side:

* **The vet asks affordability, not playability.** `slot_is_pitchable`
  (`reactor0/interpret_reaction.h`) is the one definition, shared with the
  deferred negatives: *any* playable plain reading, or *any* NON-CRITICAL
  inverted one. It replaces `variants::can_pitch_for_free`, which demanded that
  every reading be a dead orange. The swap is **gated on the card being a known
  orange**, because the tests below it in `vet_react_slot` are about striking —
  the playability retarget and the giver-only reject — and letting a bare
  existential short-circuit those would disable the strike checks for nearly
  every unclued card in an Orange variant.
* **The stamp must be `stamp_orange_pitch`.** `reactor::target_play` narrows
  `inferred` to the playable set and bails when that empties, so it can never
  stamp a pitch. Phase A of `reactive_rank` already reached for the pitch stamp;
  `reactive_colour` mode 1 did not.

Replay 1973976 T12 needed **both**, and reverting either alone puts it back.
Orange on 1, and will-bot69's slot 3 was a known orange whose last o2 was
already accounted for — effective empathy `{o1, o3, o4}`, nothing playable and
nothing a connector, `inferred {o3, o4}`. The vet retargeted; had it not, the
stamp would have refused. The pitch that would have chucked will-bot67's
playable o2 onto the stack was skipped and the clue degraded to naming his r1.

#### A playable orange on chop is expendable (v10.3.0)

Discard CHUCKS an orange onto its own stack, so a playable orange on a chop is
the one card its holder should be throwing: it scores itself. It therefore joins
basic trash and the same-hand dupe in all three chop predicates
(`chop_is_free_chuck`, `state_eval.cpp`):

| | with a playable inverted chop |
|---|---|
| `at_risk_chop` | **false** — nothing is at risk |
| `has_playable_chop` | **false** — no play to arrange |
| `chop_is_expendable` | **true** |

All three had to change together, because §3 fires on either of the first two
(`priority_3_applies`). Replay 1973974 T10: will-bot69's chop was a playable o2
and will-bot67 spent a clue LOCKING him over it.
Reactive anchors ignore the rainbowish/pinkish focus tables entirely — the
anchor is the clue value in every variant. `/allplays` is a reactor concept
and never reaches a reactor0 game at all (§0).

### Alternating Clues and Synesthesia — the target carries the parity

**102 variants**, and the only two families where reactor0 gives **no stable
clues at all**. Both take the choice of clue KIND away from the giver, so the
kind cannot carry a signal:

- **Synesthesia** (36 variants) offers colour clues only — it carries
  `clueRanks: []` as well as its own touch rule, so there is only ever one kind
  to give.
- **Alternating Clues** (66 variants) forbids two consecutive clues of the same
  kind, so on any given turn at most one kind is even legal.

`variants::uses_target_parity` (`conventions/variants/predicates.h`) is the one
predicate; every site reads it rather than testing the two flags.

Every clue is reactive, and **the roles are fixed regardless of who is clued**:

| Clue | Parity | Reacter | Receiver |
|---|---|---|---|
| to **Bob** | **odd** — exactly one play | Bob | **Cathy** |
| to **Cathy** | **even** — double play / double discard | Bob | **Cathy** |

Bob is always the reacter because he is the seat that acts next; Cathy is always
the receiver. So a clue to Bob **touches the reacter's own hand** while still
identifying a slot in Cathy's: Bob acts, Cathy reads which slot he chose, and
the turn order works out. Nothing in the reactive branches reads `action.list_`,
so the touched hand needs no further special-casing.

**This is the only place in the convention where the clued seat and the
receiver are different players**, and every site that needs either must read it
from the one pair of functions that owns the rule
(`reactor0/interpret_reactive.h`):

| | |
|---|---|
| `clue_is_reactive(state, action, bob)` | `uses_target_parity(v) \|\| action.target != bob` |
| `reactive_receiver(state, action, reacter)` | the clued seat, or Cathy under target parity |

`interpret_reactive` therefore takes the receiver as an argument rather than
deriving it, and so do `reactive_rank` and `reactive_colour`. **The waiting
connection's `clue.target` records the seat that was CLUED, not the receiver** —
every later parity lookup keys on it, and here the two come apart.

**Replay 1973971 T15 is what re-deriving it costs.** v10.0.0 threaded the
receiver into `interpret_reactive` but left five sites computing it as
`action.target` for themselves: both reactive branches, `read_clue`'s
stable/reactive fork, and the two `wc_is_fresh` calls. yagami clued rank 5 to
will-bot69 — a reactive discard clue, anchor 5, so playing slot 3 designates
will-bot67's slot 2, his leftmost trash. The branches walked **will-bot69's own
hand** instead, where every deck id is nullopt from its own seat, so the pool
came back empty and the clue read as a MISTAKE. will-bot69 discarded its chop.
Fixed in v10.2.0.

**Anchors are unchanged.** `1=1 … 5=5` and `Red=1, Yellow=2, Green=3, Blue=4,
Purple=5`, from the same `colour_clue_value` / `rank_reactive_value` tables as
everywhere else. Only the bucket moves, via `reactive_assignment_for` — the
target-aware sibling of `reactive_assignment`, which stays target-blind for the
`/settings` table. `/set` still moves a clue's value.

**Consequence for the tier rules**: `has_colour_play_clue_for` models a *stable*
colour play clue and therefore returns false outright here, which makes H1c's
second arm and N2's second arm vacuous (DECISION_MAKING.md).

#### Synesthesia's touch rule

On top of its own colour, a card of rank N answers to the **Nth colour clue**
(`Variant::id_touched`, guarded by the `synesthesia` flag). The clue value is
0-indexed and the rank 1-indexed, so the test is `rank - 1 == value`. Two
carve-outs, from different places:

| Suit | Touched by |
|---|---|
| Red / Blue / Black / … | its own colour, **and** the colour of its rank |
| Rainbow, Dark Rainbow | every colour (returns true before the rule is reached) |
| Brown, Dark Brown | **Brown only** — never the colour of its rank |
| White, Gray, Null, Dark Null | **nothing** |

Brown is excluded by the rule itself. White is excluded by the rule **sitting
below** the existing whitish early-return, which matches how hanab.live
currently behaves: White in a Synesthesia variant is indistinguishable from
Null. Null is excluded twice over, being both.

#### Alternating Clues' legality

A server rule, not a convention, so it lives in the clue enumeration:
`State::all_valid_clues` drops the kind of the previous clue, keyed on
`State::last_clue_kind`, which `Game::on_clue` records for every clue real or
simulated. Filtering there rather than in the callers is what keeps the endgame
solver and `eval.cpp` from costing out lines built on clues the server would
reject. A play or discard in between does **not** reset it — the rule is about
consecutive *clues* — and it tracks the last clue by any player, not by one
seat.

The bot plays legally and reads clues correctly, but does **not** yet reason
about the fact that its own clue constrains what its partner may clue next.
`TODO.md`.

## §1g POV invariance — shared knowledge retargets, giver-only knowledge rejects

**A clue whose reading predicts a strike is not legal to give.** The rules
below are written per-site, but the general form is enforced once, in the
decision layer: `predicts_a_strike` (`src/conventions/reactor0/decision.cpp`)
asks whether the reading stamps a call on a card that is not actually playable,
and `select` drops any such candidate from every rung of both clue choosers.
"Actually playable" is judged from the giver's full sight of the target's hand,
which is what makes it a legality test rather than a score.

Until v8.9.0 the endgame fork was a hole in this: it returned the solver's clue
without ever building reactor0's candidate pool, so the veto never ran on those
turns. Replay 1971808 T59 lost a point to exactly that. `choose_endgame_clue`
closes it — see DECISION_MAKING.md "The endgame stall list".

**And when nothing survives, the endgame gives no clue at all** (v10.1.0).
`prefer_stall_clue` used to fall back on the solver's own pick whenever
`choose_endgame_clue` declined, which handed back the unvetted clue the pool had
just rejected — the solver builds its clues from `all_valid_clues` and has never
consulted the convention. It now returns nullopt instead and the fork falls
through to the ordinary ladder, where `choose_clue` reads the same filtered pool
and also declines, landing on the play/discard phase. Burning a card beats
handing a partner a promise known to be false.

**One reading is deliberately PER-SEAT: a direct play whose focus the holder can
prove is trash.** §1b priority 5 and §1c priority 1 both decline when
`provably_trash` (`reactor0/facts.h`) says every identity still open for the
focus is basic trash once the copies THIS SEAT can see are accounted for.

The empathy layer cannot express that. Per-player views are copied from `common`
(`basics/game.cpp`) and re-elim'd, and `card_elim` accounts for a copy only when
a **clue** pins it — never when a player merely looks at it. So a duplicate
sitting UNCLUED in another hand is invisible to every `Player` object. Private
sight is `state.deck[o].id()`, which is `nullopt` for one's own cards, and
`sight_narrowed` skips our own hand for that reason.

The receiver and the giver therefore both decline, while the player *holding*
the duplicate cannot see it and still reads a play. That divergence is accepted:
the seat that ACTS has the right reading, so no strike results, and the holder's
model corrects itself the moment anyone clues that card.

**But the GIVER may not decline — it must REJECT** (v10.1.0). "Declining" is a
reading, and a reading is a claim about what the *receiver* will conclude. The
receiver is exactly the seat that cannot see the duplicate, so he reads a play
whatever the giver privately works out. A giver that quietly downgrades the clue
to a stall has handed over a promise it has itself decided is false. So when the
seat evaluating is the giver (`action.giver == our_player_index`), both sites
return `std::nullopt` instead — a MISTAKE, which `analyse_clues` drops, removing
the clue from every rung of both clue choosers at once. Reading a clue somebody
else gave is untouched.

This is §1g's headline rule in its sharpest form, and it is the same device the
orange ladder already uses (§1f's giver-side chuck veto, which reads
`state.deck[o].id()` and is therefore giver-side by construction).

Replay 1973575 T62 is the cost of having conflated the two. Purple on 4, and
Purple to will-bot67 named his slot 1, whose empathy was `{p3, p5}`. will-bot69
could see the only p5 — in will-bot67's *own* slot 5 — proved slot 1 was the dead
p3, read the clue as a stall and gave it. The reading was OTHER, so
`predicts_a_strike` had nothing to veto and the endgame stall list's rung 3
selected it. will-bot67, who cannot see his own p5, read the play and bombed.
The rank half was `odd` to the same seat, illegal for the same reason: under
Odds and Evens the promise is the rightmost *newly* touched card, and slots
3/4/5 were already clued, so it landed on slot 1 again.

`is_chuckable` (`reactor0/calls.cpp`) applies the same narrowing for our own
seat, so a card we can prove is worthless reaches the chuck list — empathy alone
still admits the duplicate.

Replay 1967478 T42: blue on 4, so `b5` was the only useful blue and will-bot67
held it unclued. will-bot69's two clued blues were both trash, yet the leftmost
carried a CTP narrowed to `{b5}`, and T42 played it.


**A reactive must not call two copies of the same card to play**
(`calls_two_copies_to_play`, `reactor0/decision.cpp`). The reacter acts first,
so their copy stacks the identity and the receiver's is trash by the time they
act — but the receiver still reads their card as "the playable one", which by
then is the NEXT rank, and bombs.

Replay 1967363 T1 (Odds and Evens & Orange, where a colour clue is the
even-parity family): both halves of a double play were an Orange 1. will-bot67
chucked theirs onto the stack, then yagami_black chucked theirs for a strike,
reading it as the o2 the stack was now waiting for.

Two things make this a §1g **reject** rather than a retarget. It needs the
giver's sight of two hands, which no other seat has, so a retarget would desync;
and it is enforced as a candidate FILTER in `analyse_clues` — beside the MISTAKE
drop — rather than as a reading, so every observer interpreting an
already-given clue still runs the identical convention pipeline. The clue is
simply never offered.

**Exception:** the receiver whose card is already pinned to one identity in
common knowledge cannot be fooled — they will see the first copy land and know
theirs is dead — so the clue is allowed.

It is **variant-independent**. The hazard is the double-play SHAPE, not the
inverted suit; a vanilla rank reactive fails the same way. And it covers PLAYS
only: two cards called to be *discarded* that happen to match is not a bomb, and
throwing a spare copy is often right.


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
  the clue is dropped from the candidate pool). Retargeting on it desyncs the
  table: the reacter would compute the original pairing and act on it. These
  sites `return nullopt`: the Phase A playability veto (`:249-268`), the
  Phase B connector abort (`:328-332`), `would_lose_inverted_reacter` in
  Phase A (`:269-276`), Phase B (`:339-346`) and colour mode 1 (`:435-443`),
  the blind mode-2 rejection (`:479-487`), and — on the **stable** side — the
  orange ladder's chuck veto (`interpret_clue.cpp:310-312`, §1b).
  `would_lose_inverted_reacter` carries one **exemption**, not an exception to
  this rule: rank Phase A skips it for a react card `can_pitch_for_free`
  accepts (§1d). That predicate reads `common`, so it belongs to the *shared*
  half — giver and reacter both conclude the pitch is free, and no reject is
  needed because nothing is lost.

Worked example: Alice clues red, and the first target maps to Bob's slot 5,
which Alice can see is an unplayable orange 3. Bob cannot see it. He will not
walk on — he will chuck it and strike. So Alice must reject the clue, not
quietly retarget to Cathy's next playable. Replay 1957905 #31 is the same
shape on the stable side, where the veto was missing until v5.0.0.

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

## §1i The static inferred set, and the escalation ladder

**Once a card's inferred set has been created it may only ever be NARROWED.**
It is never reset by a re-derivation, by a dropped call, or by a strike. This is
what makes the inferred set — rather than the CTP/CTD stamp — the thing reactor0
reasons with: a stamp is a *signal*, which can come and go, while an inference is
*permanent*.

The one exception is a **genuine contradiction** — a narrowing that would leave
nothing — and it escalates (`Game::narrow_thought`, `src/basics/game.cpp`):

1. Narrow. If something survives, done.
2. If not, reset that card to its **global empathy** and re-derive. Because
   `possible` is already clue-narrowed, this is exactly "reset to global
   empathy, then apply what we know".
3. If *that* is still empty, hard-reset, drop the call, and mark **`[?]`** in
   the notes. Nothing explains the card; the caller refuses to interpret rather
   than invent a reading.

Step 2 is the long-standing engine behaviour, in `Game::on_clue`
(`src/basics/game.cpp` touched and untouched branches, before any convention
interprets the clue) and in `elim`'s step-1 sweep. Those remain
convention-agnostic and apply to reactor too. What v8.0.0 adds is the rule that
nothing *else* may widen, and step 3.

**What this forbids, concretely.** `erase_call`
(`reactor0/call_invariants.cpp`) used to revert `inferred` to `old_inferred`,
and `check_missed` (`src/basics/game.cpp`) still does under reactor.
`narrow_to_stamped_button` rebuilt from `possible`, which despite its name could
push a card back UP to `possible ∩ allowed`. All three are now narrowing-only
under reactor0. The strike exemption (`src/basics/decide.cpp`) is one case of
the same rule, not a special case.

Replay 1967558 is what it costs to get this wrong. yagami_black's slot 4 was
stamped `CALLED_TO_PLAY` and narrowed to `{p1}`; will-bot69 then played the
other `p1`, so rule 3 erased the dead call — and took `{p1}` with it, restoring
all five purples. yagami_black no longer knew the card was trash, so
`has_no_safe_action` was true, §3 fired, and will-bot67 spent a clue on Bob's
chop instead of playing its own reactive-CTP.

**One intermediate write is exempt, deliberately.** Rule 1 constrains the NET
effect of an interpretation, not every write inside it. reactor's shared
`target_play` / `target_discard` narrow to the PLAIN-suit reading of their
button, which `narrow_to_stamped_button` exists to correct — so it re-baselines
on the inference as it stood *before* the stamp (`reset_thought_to`) and then
narrows from there.

Its consequence is unchanged, and stronger: no interpretation layer is ever
handed a card with an empty `inferred`, so target selection and target narrowing
cannot disagree about their baseline.

## §2 Decision making — moved

Reactor0's decision making now lives in its own ruling document:
**[DECISION_MAKING.md](DECISION_MAKING.md)**.

This file remains the ruling reference for what a clue **means** under reactor0
(§0-§1i above). How the bot chooses what to do on its turn — the clue tiers, the
General Clue Evaluation List, and the actionable-card priority — is specified
there, because it is being replaced wholesale: the tuned-constant layer in
`state_eval.cpp` gives way to an ordered rules procedure over v7.0.0 and v7.1.0.
See [PLAN.md](../../../PLAN.md) for the staged scope.

What used to be documented here, and where it went:

| Was | Now |
|---|---|
| §2 (what reactor0 shares with reactor's §2) | DECISION_MAKING.md — *Framework* and *Precedence* |
| §2a The pace-clue tier gate | DECISION_MAKING.md — *Clue Tier Definitions* and *Decision phase 1* |
| §2b The pointless-double-discard filter | Deleted in v7.0.0 — the General Clue Evaluation List's priority 2 admissibility condition subsumes it |
| §2c Clue scoring — reactor0's own `get_result` | Deleted in v7.0.0 — replaced by the ordered priority list |

As of v7.1.0 the **code** runs both phases: the priority list chooses the clue,
and the Actionable Card Priority list chooses the play or discard. Reactor0 no
longer reaches the shared ladder in `src/basics/decide.cpp` at all. Only rungs
§4.5 and §4.6 remain specification, and DECISION_MAKING.md's *Not yet
implemented* table says so. Reactor is unaffected throughout; its decision rules stay in
[reactor's CONVENTION.md §2](../reactor/CONVENTION.md).

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
| `tests/test_reactor0/test_reactive_assignment.cpp` | the two parity buckets, the `/settings` lines verbatim, and `/set` label parsing |
| `tests/test_reactor0/test_reactive_override_effect.cpp` | a `/set` override changing the ruleset and anchor, and the in-flight insulation |
| `tests/test_net/test_set_persistence.cpp` | `/set` surviving a game start, the variant reset, and `/set` inside a replay |
| `tests/test_reactor0/test_reactive_inverted_vet.cpp` | §1d — the react-slot vet follows the inverted swap, each swapped case paired with its un-swapped control, plus the giver-only reject |
| `tests/test_reactor0/test_bad_reactive_target/test_replay_1942777_orange_reactive_vet_follows_swap.cpp` | bug 4.1 end to end — the reacter discards slot 5, not the Phase C lock's slot 3 |
| `tests/test_reactor0/test_bad_reactive_target/test_replay_1957942_trash_orange_pitch_is_a_valid_reaction.cpp` | bug_report_4_1_0.txt 4.1.0b end to end — the reacter pitches the trash orange instead of discarding a critical 5 |
| `tests/test_reactor0/test_orange.cpp` | §1b/§1c in inverted variants — bug 3.1's rank-2 focus (now a pitch narrowed to the plain playables), a rank clue still revealing a playable orange, pitch at pace > 3, chuck at pace <= 3, chuck in Dark Orange, play reveal outranking the pitch, the stall when nothing can reach the stacks, the §1b giver-side reject of an unplayable chuck target, and the §1c orange-only rank chuck alongside the mixed-set pitch |
| `tests/test_endgame/test_orange_chuck.cpp` | bug 3.2 — the solver offers a known playable orange as a chuck, and `perform_to_action` models a chuck of a non-playable orange as a misplay |
| `tests/test_reactor0/test_stable_rank_omni.cpp` | §1c in an omni variant — rank 1 and rank 4 read as direct plays, an unplayable useful identity still blocks, and the pinkish focus is the leftmost |
| `tests/test_reactor0/test_misc/test_replay_1942525_omni_rank_reads_as_direct_play.cpp` | bug 1.3 end to end |
| `tests/test_reactor0/test_misc/test_replay_1957905_orange_chuck_must_be_playable.cpp` | bug_report_4_1_0.txt end to end — no orange colour clue, and the rank-2 chuck is chosen |
| `tests/test_reactor0/test_misc/test_replay_1942458_colour_mode2_walks_dc_targets.cpp` | bug 1.1 — mode 2 walks to a live dc-target |
| `tests/test_reactor0/test_target_parity.cpp` | §1f Alternating Clues / Synesthesia — the parity follows the target and not the kind, a clue to Bob is odd reactive with Cathy still the receiver, the WC records the clued seat, no stable interpretation is ever produced, `has_colour_play_clue_for` is false, the `/settings` line; and that the clued seat and the receiver come apart — the dispatch predicates, a clue to OUR seat designating the third seat, `read_clue` classifying it reactive, and N2 reaching it |
| `tests/test_reactor0/test_orange_chop_and_pitch.cpp` | §1f — a playable orange chop reads expendable in all three chop predicates, with a dead orange, a plain playable, a plain useful and an unplayable orange as controls; and `slot_is_pitchable` on a known orange, a plain card and Dark Orange |
| `tests/test_reactor0/test_misc/test_replay_1973976_known_orange_is_pitchable.cpp` | Replay 1973976 T12 end to end — the pitch needed BOTH the vet and the stamp; reverting either puts it back |
| `tests/test_reactor0/test_misc/test_replay_1974046_known_trash_is_chucked.cpp` | Replay 1974046 T22 end to end — a card read {b2} with blue on 2 is chucked instead of the critical b5 chop |
| `tests/test_reactor0/test_misc/test_replay_1974052_known_trash_after_colour_clue.cpp` | Replay 1974052 T6 — the same, reached by a colour clue narrowing a reactive inference to {y1} |
| `tests/test_reactor0/test_inverted_trash_target.cpp` | §1d — the 2x2: an inverted trash target puts the reacter on Discard in the odd bucket and Play in the even one, with a plain trash target as the control in each; plus the even bucket walking past an unusable react slot |
| `tests/test_reactor0/test_misc/test_replay_1974257_inverted_trash_target_is_pitched.cpp` | Replay 1974257 T30 end to end — every expendable card the receiver held was orange, so the target pool came back empty and the clue read as a MISTAKE |
| `tests/test_reactor0/test_misc/test_replay_1974218_sarcastic_needs_a_known_identity.cpp` | Replay 1974218 T25 end to end — a sarcastic discard invented from a merely-touched card had pinned a cardinal 2 to {i4}, hiding it from the reactive play clue that asked for it |
| `tests/test_reactor0/test_misc/test_replay_1973974_playable_orange_chop_is_not_saved.cpp` | Replay 1973974 T10 end to end — a partner locked over a playable orange on his chop |
| `tests/test_reactor0/test_misc/test_replay_1973971_reactive_receiver_is_not_the_target.cpp` | Replay 1973971 T15 end to end — a reactive discard clue to the reacter read as a MISTAKE because the branch walked his own hand |
| `tests/test_basics/test_synesthesia.cpp` | The Synesthesia touch matrix — the rank rule and its off-by-one, brown answering only to brown, white and null untouched, rainbow unaffected, and no rank clues offered |
| `tests/test_basics/test_alternating_clues.cpp` | The legality rule — the first clue is free, each kind blocks its own repeat, a play or discard does not reset it, it alternates across players, hypos carry it, and plain variants are unaffected |
| `tests/test_reactor0/test_giver_sight_reject.cpp` | §1g — the giver rejects a promise only it can refute, on both the colour and the rank side; the same clue from another seat still reads a stall; a common-knowledge stall is untouched; and `analyse_clues` removes the rejected clue from every rung |
| `tests/test_reactor0/test_misc/test_replay_1973575_giver_sight_must_not_downgrade.cpp` | Replay 1973575 T62 end to end — Purple to will-bot67 promised a dead p3 because the giver could see the real p5 in his own hand |
| `tests/test_reactor0/test_misc/test_replay_1972716_spent_reaction_is_not_urgent.cpp` | A deferred call whose target the receiver has since played no longer pre-empts the turn — the pairing is read off `react_target_order`, and the clue Bob's chop is owed is given instead |
| `tests/test_reactor0/test_efficiency.cpp` | efficiency formula + rlocks defaults |
| `tests/test_reactor0/test_giver_filters.cpp` | MISTAKE clues never offered |
| `tests/test_basics/test_snapshot_convention.cpp` | convention/rlocks snapshot round-trip + reactor back-compat |
| `tests/test_reactor0/test_decision_making/test_clue_tier.cpp` | Clue tiers (DECISION_MAKING.md) — H1/H2 and each endangered-chop disqualifier, incl. the same-hand dupe and both singleton and group elim; VH1 reading VERY HIGH; and H4, which reads HIGH and not VERY HIGH so that a pending reaction still outranks it |
| `tests/test_reactor0/test_decision_making/test_pace_clue_gate.cpp` | The two gate windows (DECISION_MAKING.md, *Decision phase 1*) — both token boundaries incl. `clue_tokens == 3`, the HIGH row reaching past 3, the 8-token exemption, the no-stamp negative that pins the split, and the two pace boundaries (occupied `>= 1`, unoccupied `>= 3`, both silent at pace 0), plus the locked exemption inside the unoccupied window — plus required tier, and that reactor's own gate is unchanged |
| `tests/test_reactor0/test_decision_making/test_double_discard_filter.cpp` | Which reactives priority 2 refuses to propose — all three arms of `discard_is_affordable` and its negative, plus the shape facts the rungs select on (a clue to Bob is never reactive; a colour reactive is never a double discard; a playless clue to Bob is not a stable play) |
| `tests/test_reactor0/test_decision_making/test_clue_shape.cpp` | Clue-shape classification — result-orientation on inverted suits, and the receiver judged against the stacks the reacter leaves behind |
| `tests/test_reactor0/test_decision_making/test_clue_priority.cpp` | The General Clue Evaluation List itself — the default tiebreak, all three gate windows, `discard_is_affordable`, `missing_connectors` against the spec's worked example, rung 1 outranking the lower rungs, the §4 floor and its empty-set counterpart, and `choose_very_high_clue` as Precedence step 1 |
| `tests/test_reactor0/test_decision_making/test_replay_1942181_prefers_stable_play_over_double_discard.cpp` | Prefers a stable play over a double discard, end to end on the replay that motivated it, plus both shapes read off a real Phase C |
| `tests/test_reactor0/test_decision_making/test_replay_1942330_playable_chop_lifts_clue_tier.cpp` | N5 end to end — a playable non-duped chop on Bob lifts every clue, so a direct play clue beats the lock the flattened gate used to pick |
