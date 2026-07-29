# Glossary

Every domain term this project uses, in one alphabetical list. Each entry
gives the definition **as the code actually implements it** plus where to find
it. Terms that mean something different here than in mainstream Hanabi theory
are flagged **⚠**.

For how the terms fit together see [CONVENTION.md](CONVENTION.md); for the
project overview see [README.md](README.md).

Two orientation facts that most entries depend on:

- **Slot 1 is the leftmost, newest card.** Draws prepend
  (`src/basics/game.cpp:378`), so slot 1 has the highest `order`. "Left" =
  newer, "right" = older.
- **Alice / Bob / Cathy / Zelda** are positional, not identities: Alice is the
  player to move, Bob the next player, Cathy the one after, Zelda the previous
  (`src/basics/decide.cpp:90-92`, `:419-421`).

---

### /allplays
Chat toggle that promotes **colour** reactive clues to play+play, matching
rank clues, so both players end up called to play. Default off.
`include/hanabi/basics/game.h:98-102`; `src/net/commands.cpp:797-831`.

### /settings
Chat command that prints the variant's active reactive tables — the
`odd plays: … , even plays: …` line. `reactive_table.cpp:125-174`.

### Ambiguous (variant family)
Variants where several suits share one clue colour (e.g. Tomato and Mahogany
are both cluable as "Red"). The reactive colour table keys on the **clue
colour name**, not the suit name, so partners and bot agree.
`src/basics/variant.cpp:114-128`; `reactive_table.cpp:82-100`.

### anxiety play
When locked but holding a card that *might* be playable, gamble on the slot
with the highest playable probability.
`src/basics/player_game.cpp:441-470`.

### arrangement
In the endgame solver, one concrete assignment of identities to our own
unknown cards. The solver enumerates arrangements, weights them by
probability, and maximises expected win rate. `src/endgame/solver.cpp:638-714`.

### bad touch
Newly-touched cards that are trash or duplicates of already-known cards.
Penalised in clue scoring; **not** used as an inference rule here (see *good
touch*). `src/basics/clue_result.cpp:82-175`; `state_eval.cpp:237-244`.

### base_count
Per-identity count of copies already visible or played, used to compute what
remains unseen. `src/basics/state.h`; used by `effective_possible_for`.

### basic trash
An identity that can never be played again — already on the stack, or behind a
gap that can no longer be filled. Direction-aware for reversed suits.
`include/hanabi/basics/state.h:102-123`.

### BDR (bad discard risk)
`eval_game` term penalising states where a useful identity has copies already
discarded and no visible duplicate. Rank-weighted: `−n²` for 1s, `−3.0` for
2s, `−1.5` for 3s, `−0.5` for 4s, whole term `× 2.5`.
`state_eval.cpp:658-709`.

### bluff
**⚠ Not implemented.** `CardStatus::BLUFFED` / `MAYBE_BLUFFED` /
`F_MAYBE_BLUFFED` and `FinesseKind::BLUFF` exist from the Python/Scala port
and are never set by reactor code.
`include/hanabi/basics/card.h:30-32`; `connection.h:20-26`.

### Bob
The player after the current one. In a reactive clue Bob is normally the
**reacter**. `src/basics/decide.cpp:90-92`.

### brownish
Suit family matched by the substrings Brown, Muddy, Cocoa, Null. Rank clues
never touch these suits. `src/basics/variant.cpp:154`;
`src/conventions/variants/predicates.cpp:20-23`.

### catchup
Flag suppressing note publication and other side effects while replaying
history (reconnect, rewind). `include/hanabi/basics/game.h:88`.

### card_elim
Hard elimination: remove identities from `possible` once all copies are
accounted for, including cross-elimination over sets of cards.
`src/basics/player_elim.cpp:242-315`.

### Cathy
The player two seats after the current one. `decide.cpp:90-92`.

### chimneys
Variant where a rank-K clue touches all cards of rank **≥ K**. Routed through
the *pinkish* convention path. `src/basics/variant.cpp:243-244`;
`predicates.cpp:16`.

### chop
**⚠ Three different definitions coexist in this codebase.**

| Definition | Used by | Cite |
|---|---|---|
| CTD'd card, else the **newest** unclued status-NONE card, gated by `zcs_turn` | `Game::chop` — the discard decision | `decide.cpp:404-417` |
| **Oldest** unclued card (`lock_order`) | `ref_discard`'s lock detection | `interpret_clue.cpp:330-337` |
| **Oldest** unclued card | the pink promise | `pinkish.cpp:31-37` |

Note the first is the *newest* unclued card — the opposite of the H-group
convention.

### chop moved (CM)
`CardStatus::CHOP_MOVED`: the card is protected from discard. Applied to an
entire hand by a LOCK. `card.h:23`, `:124`.

### clue starved
Variant where playing a 5 returns only half a clue token.
`src/basics/state.cpp:125-135`.

### clue-regain rank
The rank whose play returns a clue token: 5 normally, 1 on reversed suits.
Used by condition (3) of the high-value-clue gate.
`include/hanabi/conventions/variants/reversed.h`; `state_eval.cpp:140`.

### colourable suits
One representative suit index per distinct clue colour. Its size equals the
number of colour clues available, which for Ambiguous variants is fewer than
the number of suits. `include/hanabi/basics/variant.h:70-76`.

### common
The common-knowledge perspective — a `Player` with `player_index == -1`.
Convention decisions are made against `common` so every observer reaches the
same reading. `include/hanabi/basics/game.h:73`; `player.h:1-8`.

### connectable
Recursive check that a chain of plays can reach a target card before the
receiver has to act. `src/basics/fix.cpp:157-183`.

### Connection
**⚠ Mostly unused.** The variant type `KnownConn | PlayableConn | PromptConn |
FinesseConn | PositionalConn`, inherited from the H-group port.
`include/hanabi/basics/connection.h:116-117`.

### critical
An identity with exactly one copy left undiscarded that is still useful.
Losing it lowers the maximum achievable score. `src/basics/state.cpp:163-167`.

### critical rank
Variant flag making every card of a given rank single-copy, hence critical.
`src/basics/variant.cpp:183`.

### CTD — called to discard
`CardStatus::CALLED_TO_DISCARD`. The convention has designated this card as
the one to discard. **⚠ A physical action label**: it means "press the discard
button", which on an inverted (Orange) suit is a *play* attempt.
`card.h:25`; `decide.cpp:657-660`.

### CTP — called to play
`CardStatus::CALLED_TO_PLAY`. Queued to be played. Same physical-label caveat
as CTD: on an inverted suit, pressing play sends the card to the discard pile.
`card.h:24`; `decide.cpp:644-656`.

### dark suits
Black, Dark, Gray, Cocoa — one copy each, so always critical.
`src/basics/variant.cpp:155`, `:183`.

### dc-target
The receiver slot a reactive clue designates for discard. Chosen from a
five-tier cascade: `pre_clued_trash` → `unknown_trash` → `known_trash` →
`unknown_dupes` → `sacrifices`. `interpret_reactive.cpp:373-512`.

### deceptive (special rank)
Variant flag where the special rank's touch rule is
`(suit_index % 4) + offset == clue.value`. `src/basics/variant.cpp:235-238`.

### deferral / deferred reactive
The reacter of a pending reactive gave a clue instead of reacting. The old
waiting connection is cancelled and the new clue is forced reactive.
`decide.cpp:38-48`, `:65-70`.

### delayed play
A card that only becomes playable after intervening players make their known
plays. `delayed_plays` returns `(order, successor-identity)` pairs used as
chain connectors. `interpret_clue.cpp:76-128`.

### destroyed play
A card that is CTP in the real game but becomes CTD under a candidate clue's
reading. Costs `−10` each in clue scoring (v1.7.0).
`state_eval.cpp:251-267`.

### distribution clue
**⚠ Implemented but never invoked.** A clue whose only value is spreading
duplicated criticals. `src/basics/fix.cpp:59-109`.

### effective_possible_for
Narrows a card's `possible` set by visibility **from the holder's point of
view** — counting copies visible in every hand *except* the holder's own. Used
instead of raw `possible` wherever a reactive decision must come out the same
for the giver and the reacter. See *POV invariance*.
`src/conventions/reactor/interpret_reactive.cpp:54-69`.

### elim
Umbrella for the post-action inference pass: `card_elim`, optional
`good_touch_elim`, link refresh, and hypo-stack recomputation.
`src/basics/game.cpp:463-563`.

### elim_* matrices
The four functions that eliminate identities from the receiver's slots *left
of* the resolved reactive target, one per (clue kind × reacter action)
combination: `elim_play_play`, `elim_play_dc`, `elim_dc_play`, `elim_dc_dc`.
`interpret_reaction.cpp:115-245`.

### empathy
What a given observer believes about a card: the `Thought` triple `possible` /
`inferred` / `info_lock`. `include/hanabi/basics/card.h:75-110`.

### even plays
`/settings` label for **rank** clues: reacter plays and receiver plays — an
even number of plays. `reactive_table.cpp:167-173`.

### finesse
**⚠ Narrow meaning here.** Reactor's only finesse is the rank-reactive
fallback: when no direct play target resolves, react slots are tried in the
fixed order `{1, 5, 4, 3, 2}` looking for a receiver card exactly one away
from playable. `interpret_reactive.cpp:781-867`. The `FINESSED` card status
and `FinesseConn` type are port leftovers and are never set.

### fix clue
A clue that corrects a wrong earlier inference — a CTP'd card now provably
trash, or a revealed duplicate. `ClueInterp::FIX`; `src/basics/fix.cpp:12-57`.

### focus
See CONVENTION.md §1a.2 — **two distinct notions**:
- **reactive focus slot**: the anchor of the reactive slot arithmetic, from
  `reactive_focus` (`interpret_clue.cpp:35-72`);
- **stable focus**: per-branch, usually `max(newly_touched)`.

### focused
`ConvData::focused` — this card was the clue's focus. `card.h:114`.

### Fraction
Exact rational arithmetic used by the endgame solver so win rates are computed
without floating-point error. `include/hanabi/endgame/fraction.h`.

### funnels
Variant where a rank-K clue touches all cards of rank **≤ K**. Routed through
the *pinkish* path. `src/basics/variant.cpp:243-244`; `predicates.cpp:16`.

### gentleman's discard
Deliberately discarding a known-playable card to hand the play to a teammate.
`DiscardInterp::GENTLEMANS_DISCARD`; `src/basics/sarcastic.cpp:24-56`.

### giver
The player who gave the clue (`ClueAction::giver`).

### good touch (GTP)
The principle that clued cards are never trash, so trash identities can be
eliminated from touched cards. **⚠ Disabled in this bot** — `Game::good_touch`
is left `false` (`game.h:97`), so `good_touch_elim` never runs (v0.39). It
survives only as an evaluation term. `src/basics/player_elim.cpp:319-352`.

### half clue token
Clue-starved bookkeeping: playing a 5 sets a half-token flag; two of them make
a whole clue. `src/basics/state.cpp:125-135`.

### hand size
`kHandSize[num_players] = {–, –, 5, 5, 4, 4, 3}`. Used as the modulus in the
reactive slot arithmetic, **not** the current length of a hand.
`include/hanabi/basics/state.h:29`.

### high-value clue
The strict predicate that lets a clue through the low-clue-count gate: a
unique good chop in danger, **or** a critical low card played, **or** ≥ 2 plays
including a clue-regain rank. `state_eval.cpp:105-146`.

### hypo stacks
The play stacks after simulating everyone's known plays to a fixpoint. Feeds
`playable_away`, `hypo_score`, and the reactive play-target pool.
`src/basics/player_game.cpp:474-601`.

### IdentitySet
Bitset over the ~30 card identities; the workhorse of empathy. Uses
`std::popcount` / `std::countr_zero`, hence the C++20 requirement.
`include/hanabi/basics/identity_set.h`.

### inferred
The convention-narrowed identity set for a card. Falls back to `possible` when
empty. `include/hanabi/basics/card.h:80`, `:96-98`.

### info_lock
A sticky promise the convention made about a card's identity, surviving later
narrowing. Enforced in arrangement validity and playability checks.
`card.h:83`; `decide.cpp:385-390`.

### inverted suits (Orange, Dark Orange)
Suits where the game rule **swaps the play and discard buttons**:
`PerformPlay` discards, `PerformDiscard` is a play attempt. Because CTP/CTD are
physical labels, the convention must stamp CTD to get an orange card onto its
stack. `src/conventions/variants/inverted.cpp`; `src/basics/game.cpp:228-247`.

### known trash (kt)
A card whose every possibility is basic trash, or which is duplicated by an
older card in the same hand. `order_kt`, `src/basics/player_game.cpp:96-113`.

### known playable (kp)
A card whose every non-trash possibility is currently playable. `order_kp`,
`player_game.cpp:134-159`.

### link
A relation between several cards sharing one identity set — `PromisedLink`,
`SarcasticLink`, `UnpromisedLink`, `PlayLink`.
`include/hanabi/basics/player.h:38-75`.

### loaded / unloaded
Loaded = has an obvious playable or known trash, i.e. has something safe to
do. `obvious_loaded`, `player_game.cpp:221-246`.

### lock / locked
A player with no safe action and every unclued card chop-moved. Produced by
the LOCK clue interpretation, which touches the oldest unclued card.
`interpret_clue.cpp:334-359`; `player_game.cpp:221-246`.

### locked discard
The sacrifice choice when locked: minimise critical probability, then maximise
a rank/distance score. `player_game.cpp:401-439`.

### low-clue-count gate
At `clue_tokens < 3` and `pace() >= 3`, with a real play in hand, a clue must
be *high value* or it scores a flat `−1.0`. `state_eval.cpp:483-494`.

### max_score
The best score still achievable given what has been discarded. Losing a point
of it costs `−20` in `eval_state` — the largest single term in the evaluation.
`state_eval.cpp:598-599`.

### meta
`Game::meta[order]`, a `ConvData` — the cross-perspective conventional state
on a card (status, urgent, focused, trash, signal_turn, by).
`include/hanabi/basics/card.h:112-137`.

### MISTAKE
The verdict when no legal reading exists. Scores `−100` in `eval_action` and
is filtered out of candidate clues, which is how the giver rejects clue shapes
partners could not read. `interp.h:12`; `state_eval.cpp:471`.

### Monte-Carlo grouping
Solver optimisation collapsing arrangements by a trash-normalised key and
renormalising probabilities. `src/endgame/solver.cpp:720-752`.

### muddy
Suit family (Muddy, Cocoa) — brownish and rainbowish-adjacent.
`src/basics/variant.cpp:157`.

### newest-demoted
The reactive focus rule: among touched cards take the newest, **except** a
touched slot-1 card is demoted, so the focus prefers an older touched card.
`interpret_clue.cpp:50-54`.

### notes
Text the bot publishes back to hanab.live per card: `turn N: [f] <ids>` on a
new CTP, `[kt]` on a new CTD, `[reset]` when one clears. Card order 0 carries
the bot version. `src/net/notes.cpp:12-70`.

### odd plays
`/settings` label for **colour** clues: reacter discards and receiver plays —
an odd number of plays. `reactive_table.cpp:167-173`.

### old_inferred
Snapshot of `inferred` taken when an urgent call is stamped, so `check_missed`
can revert it if the player doesn't act. `card.h:81`; `game.h:171-173`.

### order
Global deck index, monotonically increasing. Higher order = drawn later =
further left in the hand. `card.h:55`.

### pace
`score + cards_left + num_players − max_score`. How much slack remains before
the team must stop discarding. `include/hanabi/basics/state.h:88`.

### pinkish
Suit family matched by Pink and Omni — **and**, in this codebase, Funnels and
Chimneys. Rank clues touch every rank. `predicates.cpp:12-18`.

### pink promise
In pinkish variants, a rank clue that newly touches the receiver's chop (here
the **oldest** unclued card) promises the chop has that rank. Violating it
kills the stable reading. `src/conventions/variants/pinkish.cpp:18-55`.

### play-target
The receiver slot a reactive clue designates for play. Rightmost copy of each
identity is primary; never stacks on an already-CTP'd card.
`interpret_reactive.cpp:229-270`.

### playable_away
How many ranks short of playable a card is. 0 = playable, 1 = one away (the
finesse-fallback criterion). Direction-aware. `state.h:102-123`.

### possible
The hard-eliminated identity set — what a card could still be given all
visible information. `card.h:79`.

### POV invariance
The design constraint that every observer must decode a clue identically.
Enforced by making convention decisions from `common` knowledge and by
`effective_possible_for`, which narrows from the *holder's* point of view
rather than the computing bot's. `interpret_reactive.cpp:27-69`.

### pre_clued_trash
Top-priority dc-target pool: slots clued before this turn that this clue's
narrowing disambiguates into common-knowledge trash.
`interpret_reactive.cpp:454-472`.

### prism
Suits where colour touch is `(rank − 1) % num_colours == clue.value`.
`src/basics/variant.cpp:211-213`.

### prompt
A clued card asked to play as a link in a chain. `valid_prompt` /
`find_prompt`, with a pink-prompt exception.
`src/basics/player_game.cpp:302-381`.

### rainbowish
Suit family matched by Rainbow and Omni. Every colour clue touches them —
which is why colour clues in these variants name the reactive focus slot from
a table instead of from what they touched. `predicates.cpp:8-10`.

### reacter
The player whose next play or discard **decodes** a reactive clue for the
receiver. Normally Bob. `include/hanabi/basics/game.h:44`;
`decide.cpp:113-133`.

### reactive clue
A clue whose meaning depends on another player's response, resolved by the
slot arithmetic. `interpret_clue.cpp:790-816`.

### reactive focus slot
See *focus*. The anchor of the sum rule.

### reactive value table
Per-variant map from colour clue → focus slot, used in rainbow-ish variants.
Vanilla order Red=1, Yellow=2, Green=3, Blue=4, Purple=5, Teal=6, each
`% hand_size`. `src/conventions/variants/reactive_table.cpp:63-123`.

### react_slot / react_order
The reacter's slot (and the card in it) that the convention calls them to act
on. `interpret_reactive.cpp:275-280`; `game.h:62`.

### receiver
The player the reactive clue was physically given to (`action.target`).

### ref play / ref discard (referential)
Stable interpretations. Referential play targets the card one slot **left**
(newer) of a newly-touched card, skipping touched cards. Referential discard
targets the first unclued slot **right** (older) of the focus.
`interpret_clue.cpp:276-313`, `:317-420`.

### refer
The slot-stepping primitive: move one slot in a direction, skipping touched
cards, wrapping. `src/basics/player_game.cpp:23-34`.

### RemainingMap
Multiset of unseen identities used by the endgame solver.
`include/hanabi/endgame/helper.h`.

### response inversion
When a clue read as *stable* is followed by an "unnatural" reaction from the
next player, the bot rewinds and re-reads that clue as reactive.
`interpret_reaction.cpp:260-295`, `:329-353`.

### re-tasking
A clue given while a reactive is pending on X, where X is the new clue's Bob,
**supersedes** the old one — a player's next action always answers the newest
clue. `decide.cpp:71-87`.

### reversed suits
Suits that play 5→4→3→2→1. Copy counts flip to `{1,2,2,2,3}`. Orthogonal to
*inverted*. `include/hanabi/basics/variant.h:32-35`;
`src/basics/variant.cpp:186-190`.

### rewind
Replay the game from `base` with an `InterpAction` injected at a given turn,
used to re-interpret an earlier clue. Depth-capped at 4.
`src/basics/game.cpp:567-645`.

### sacrifice
A non-critical card discarded when nothing is trash — the last-resort
dc-target pool. `interpret_reactive.cpp:438-452`.

### sarcastic discard
Discarding a card to tell a teammate they hold its duplicate.
`DiscardInterp::SARCASTIC`; `src/basics/sarcastic.cpp`.

### save clue
**⚠ Does not exist in this convention.** `ClueInterp::SAVE` is declared but
never emitted. Card protection comes from `has_ptd()`, LOCK, referential
discard, and giver-side critical filters. See CONVENTION.md §1a.8.

### scarce ones
Variant flag reducing the rank-1 count from 3 to 2.
`src/basics/variant.cpp:184`.

### sieved
An identity that is safe to lose because another copy is held somewhere it
won't be discarded. `src/basics/player_game.cpp:248-276`.

### signal_turn
The turn a CTP/CTD was first stamped. The queue-order tiebreaker throughout —
plays go in signal order, and only the newest CTD is discardable.
`card.h:121`; `decide.cpp:769-777`, `:902-926`.

### special rank
A rank with variant-specific clue-touch behaviour, combined with `rainbow_s` /
`white_s` / `pink_s` / `brown_s` / `deceptive_s`.
`include/hanabi/basics/variant.h:79-84`.

### stable clue
A clue read from its own shape by referential rules, without needing another
player's response. `interpret_clue.cpp:424-618`, `:757-786`.

### stall
A clue conveying no new instruction, burning a token to pass the turn.
`ClueInterp::STALL`; `interpret_clue.cpp:230`, `:340`, `:572`.

### strike / bomb
A misplay. Three ends the game. Scored `−1.5 / −3.5 / −100.0`.
`state_eval.cpp:601-607`.

### sum rule
The reactive slot arithmetic:
**`react_slot + target_slot ≡ focus_slot (mod hand_size)`**, with 0 read as
`hand_size`. Implemented by `calc_slot`, which is an involution in its second
argument — hence usable in both directions.
`src/conventions/reactor/interpret_reaction.cpp:16-19`.

### target_slot
The receiver slot the reactive clue points at.

### TCM (trash chop move)
Brownish-variant rule: a rank clue to an unloaded target that misses their
newest slot reads as REVEAL.
`src/conventions/variants/brownish.cpp:19-40`.

### Thought
Per-observer belief about one card: `possible`, `inferred`, `info_lock`,
`old_inferred`, `reset`. Distinct from `Card` (the physical card) and
`ConvData` (the shared conventional marks).
`include/hanabi/basics/card.h:75-110`.

### touched
A card that is clued, CTP'd, gentleman's-discarded, or finessed.
`Game::is_touched`, `src/basics/game.cpp:121-125`.

### trash push
A rank clue where every touchable identity is basic trash.
`interpret_clue.cpp:447-468`.

### unnecessary focus
A playable-rank focus whose every possibility is either basic trash or already
visible somewhere — the clue would teach nothing, so the branch is skipped.
`interpret_clue.cpp:477-486`.

### urgent
`ConvData::urgent` — act on this card *this turn*. Set by reactive
interpretations on the reacter's called slot; reverted by `check_missed` if
the player doesn't act. `card.h:115`; `game.h:171-173`.

### USELESS
An empty clue in a variant that permits them. `decide.cpp:63-64`.

### waiting connection (WC)
`ReactorWC` — the pending record of a reactive expectation: giver, reacter,
receiver, the receiver's hand snapshot, clue, focus slot, and `react_order`.
Also installed with `inverted = true` by stable readings, to arm response
inversion. `include/hanabi/basics/game.h:43-65`.

### whitish
Suit family matched by White, Gray, Light, Null. No colour clue touches them.
`src/basics/variant.cpp:151`, `:205`.

### zcs_turn (zero-clue-stall turn)
The turn the team ran out of clue tokens. Cards drawn after it are excluded
from the chop, so a player who drew during the stall isn't expected to discard
them. `include/hanabi/basics/game.h:105-106`; `decide.cpp:409-412`.
