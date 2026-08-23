# Reactor0 Glossary

Terms **specific to reactor0** or that **mean something different** here
than in reactor. Everything not listed (chop, lock slot, pitch / chuck,
CTP / CTD, critical, loaded, empathy, sum rule mechanics, …) carries its
meaning from the [reactor glossary](../reactor/GLOSSARY.md). How the terms
fit together: [CONVENTION.md](CONVENTION.md).

---

### anchor (value)
The right-hand side of the reactive slot arithmetic
`react_slot + target_slot ≡ anchor (mod hand size)`. In reactor0 the anchor
is the **clue value** — the rank for rank clues, the *colour value* for
colour clues. Stored in `ReactorWC::focus_slot` (the field name is
reactor's; reactor0 never computes a focus).
`src/conventions/reactor0/interpret_reactive.cpp:419-430`.
Not to be confused with *clue tier*, which is how worthwhile **giving** a clue
is. "Value" in reactor0 always means this anchor.

### at-risk chop (endangered chop)
A chop the team cannot afford its holder to pitch, judged from Alice's full
visibility. All four must hold: Alice knows the identity and it is not basic
trash; there is **no second copy in the holder's own hand**; no copy is
visible in the third player's hand; and Alice cannot prove she is holding a
copy (see *group elim*). Input to H1 and N2/N3 (DECISION_MAKING.md).
`src/conventions/reactor0/state_eval.cpp:121-145`. Stricter than reactor's
`chop_is_nontrash` (`src/conventions/reactor/state_eval.cpp:44-49`), which
tests basic trash only.

### clue tier
How worthwhile it is to spend a token on a candidate clue: `LOW`, `MEDIUM` or
`HIGH`. Consumed by the *pace-clue tier gate*. **Not** the "clue value" of
*anchor (value)* / *colour value* — those are what a clue *means*, this is
what giving it is *worth*. `include/hanabi/conventions/reactor0/state_eval.h`;
definitions in DECISION_MAKING.md, *Clue Tier Definitions*.

### group elim (sudoku elim)
Proving Alice holds a copy of an identity without any of her cards being a
singleton: for a subset S of her hand with combined possibilities `u`, if
fewer than |S| copies of `u \ {id}` remain unaccounted for, one of them must
be `id`. |S| = 1 is the ordinary singleton case.
`src/conventions/reactor0/state_eval.cpp:60-101`. Distinct from `cross_elim`
(`src/basics/player_elim.cpp:165-226`), which strips locked identities from
cards *outside* such a group rather than identifying one inside it.

### pace-clue tier gate
Reactor0's replacement for reactor's low-clue-count gate, and the reason a
clue has to earn its token. Two windows, with **different pace thresholds**:

* Alice already holds a card stamped `CALLED_TO_PLAY` (or, in a variant with
  an inverted suit, `CALLED_TO_DISCARD`) — she has something to do either way,
  so the bar is **HIGH** while `pace() >= 1 && clue_tokens < 8`;
* otherwise the bar is **at least MEDIUM**, while `pace() >= 3 &&
  clue_tokens <= 3`.

A candidate below its bar is dropped from the list outright, so no rung ever
sees it. The `< 8` bound is the forced-clue exemption: at 8 tokens discarding
is illegal, and rejecting every clue there would just hand the choice to an
arbitrary tie-break; an unoccupied Alice who is LOCKED is exempt for the same
reason. The MEDIUM window is one token wider than reactor's and, unlike
reactor's, fires even when Alice holds no play — the gate keys on the *stamp*,
not on what she knows.

**Why the pace thresholds differ.** An occupied Alice has her call to fall back
on at any pace, so a LOW clue is never the better use of the turn — replay
1966119 T5, where she sat at pace 2 and a LOW reactive discard was admitted
ahead of the pending call. An unoccupied Alice at low pace has nothing to fall
back on: holding her to the MEDIUM bar with the deck nearly out just sends her
to the discard pile. Pace 0 is exempt in both, since there every remaining turn
must produce a play or the game cannot finish.
`src/conventions/reactor0/decision.cpp`, `clue_is_admissible`;
DECISION_MAKING.md, *Decision phase 1*.

### blind play (react slot)
The reacter playing the computed react slot without knowing its identity.
Reactor0 uses it in the rank double play, the finesse, and colour mode 2;
the giver guarantees playability from their own view and observers who can
see the card reject the clue otherwise. `interpret_reactive.cpp:379-382`.

### colour value
The fixed value a colour name contributes as the reactive anchor: Red=1,
Yellow=2, Green=3, Blue=4, Purple=5, Teal=1; then **Orange** fills in from
{2,5,4,3,1}; then Black/Pink/Brown from {4,3,2,5,1}; then any other name from
the same list. The order matters — each rule marks the value it takes, so
Orange gets first refusal on what the fixed colours left. Keyed on the clue
colour name, so Ambiguous variants resolve by what a partner actually says.
Collisions are legal (Teal duplicates Red).
`src/conventions/reactor0/colour_value.cpp`.
Not to be confused with *clue tier* — this is a decoding input, not a measure
of how good the clue is.

### direct play clue
A stable clue whose meaning is "play this touched card" — reactor0's
stable colour reading and rank priority 1. Contrast reactor, where a stable
colour clue is *referential* (points one slot left of a touched card).
There is **no referential play in reactor0**.
A rank direct play clue means **pitch** (press Play) by default, which is why
it cannot put an orange card onto its stack while the rank's useful
identities are mixed. When **every** useful identity of the rank is orange —
every other suit's copy of that rank is already stacked — the button is
unambiguous and the clue is actioned as a **chuck** instead, stamped
`CALLED_TO_DISCARD` via `variants::called_focus_status`. See *orange ladder*
and *touched-card rank classification*.
`src/conventions/reactor0/interpret_clue.cpp:227-344`, `:452-531`.

### double discard (clue)
The rank-reactive fallback when no play or finesse target exists: zero
plays — the reacter discards the react slot and the receiver discards the
dc-target (or locks). `interpret_reactive.cpp:366-398`. The giver will not
*offer* one that buys nothing — see *pointless double discard*.

### pointless double discard
A double discard aimed at a receiver who was going to act safely anyway:
their chop is expendable, or they already hold a known play, a
`CALLED_TO_PLAY`, a `CALLED_TO_DISCARD` or known trash. Reactor0 drops such a
clue from its candidate set when a stable clue to Bob would instead get a card
played (the pointless-double-discard filter, removed in v7.0.0;
`src/conventions/reactor0/state_eval.cpp:516-559`). Reactive locks are
exempt. Named from replay 1942181 T41.

### call invariants
The two rules constraining a hand's outstanding calls, enforced after every
reactor0 interpretation: play calls run newest slot → oldest in play order
(a newer call on an older slot **erases** the earlier call on a newer slot),
and a hand holds **at most one** `CALLED_TO_DISCARD` at a time. Revealed
trash (`meta.trash`) is not a call. See CONVENTION.md §1h;
`include/hanabi/conventions/reactor0/call_invariants.h`.

### connector
The card that must play immediately **before** another — the prerequisite of
a finesse target. Direction-aware: rank−1 on a normal suit, rank**+1** on a
reversed one, since a reversed stack runs 5 → 1.
`variants::connector_of`, `include/hanabi/conventions/variants/reversed.h`.

### contradicted inference
An inference chain proven false, detected as a card's `inferred` becoming
empty. The card is reset to its global empathy immediately — before any
convention interprets the action — and the call resting on the chain is
voided with it. Engine-wide, not reactor0-specific. See CONVENTION.md §1i;
`src/basics/game.cpp:163-168`, `:196-215`, `:236-244`.

### finesse
As in reactor — the reacter plays a card that connects with a
one-away-from-playable card in the receiver's hand — but reactor0 walks
**targets** leftmost-first (reactor walks react slots in the fixed order
{1,5,4,3,2}). `interpret_reactive.cpp:301-364`.

### play reveal
A stable clue that fills in a previously-clued card as a new obvious
playable. The receiver just plays it; no status is stamped. Terminal in both
stable branches, and on the rank ladder it sits at **priority 2** — ahead of
every other reveal and ahead of the lock / referential discard, because a
play to make outranks being told what to discard.

In an inverted variant the revealed card is **chucked**, not pitched, and the
colour branch stamps `CALLED_TO_DISCARD` to say so; it also recognises a
*newly* touched card that the clue pins to a playable orange, which
`find_play_reveal` does not (it requires a previously-clued card on a colour
clue). This reading takes priority over the orange ladder.
`interpret_clue.cpp:60-85`, `:244-266`, `:488-491`.

### positional dispatch
Reactor0's whole dispatcher: clue to Bob ⇒ stable, clue to anyone else ⇒
reactive with Bob as reacter — regardless of loadedness, stall context, or
pending reactives. `interpret_clue.cpp:583-619`.

### reactive clue
As in reactor, a clue decoded jointly with the reacter's next action — but
anchored on the clue value (see *anchor*) and with fixed parity: **rank =
even plays** (double play / finesse = 2, double discard = 0), **colour =
one play** (reacter discards → receiver plays, or reacter blind-plays →
receiver discards/locks). The receiver reads the reacter's **action type**
to pick the mode.

### reactive lock
The rlocks reading: a reactive dc-target on the receiver's **oldest slot**
locks the whole hand (`CHOP_MOVED` on every card) instead of calling a
discard. Applies uniformly in both dc modes and conservatively even when
the oldest slot is actually trash. Bound at clue time via
`ReactorWC::rlocks`. `src/conventions/reactor0/interpret_reaction.cpp:24-49`.
Exempt from the pointless-double-discard filter — a lock protects a whole hand,
so it is never a
*pointless double discard*. Because the `CHOP_MOVED` stamps land a turn later
at resolution, the giver predicts the reading at clue time instead, via
`predicts_reactive_lock` (`:31-49`).

### rlocks (`allow_reactive_locks`)
The flag enabling the reactive lock. Default per variant: on iff
*starting required efficiency* ≤ 1.42. Overridden process-wide by the
`/rlocks on|off` chat command (retro-applies to running games; in-flight
reactives keep their clue-time snapshot). `Game::allow_reactive_locks`;
`src/net/commands.cpp` (`chat_rlocks`).

### sacrifice (reactive)
With rlocks off and a receiver hand of all good/unique/unplayable cards,
the dc-target falls back to reactor's sacrifice ordering
(`reactor::sacrifice_targets`: non-critical, deepest-away and highest rank
first). `interpret_reactive.cpp:111-122`.

### starting required efficiency
`max_score / (8 + starting_pace + num_suits)`, regains halved under Clue
Starved — the hardness measure that picks the rlocks default.
`src/conventions/reactor0/efficiency.cpp`.

### trash reveal
Reactor0's reading of an all-trash rank clue: the leftmost newly touched
card is marked known trash and nothing else happens — it is **terminal**,
never a referential discard, and (unlike the aspirational reactor
trash-push entry in TODO.md) never a play. Priority 2 of the stable rank
ladder; priority 3 extends it to previously-clued cards revealed as trash.
`interpret_clue.cpp:241-274`.

### Absent by design
Concepts reactor has that reactor0 deliberately lacks: **reactive focus**
(the anchor replaces it), **referential play** (stable colour is direct),
**response inversion / rewinds** (dispatch is unambiguous), **loadedness
dispatch**, **deferral-carries-reactive**, **re-tasking**, **/allplays**
(mechanically tolerated, not part of the convention).

### playable chop (N5)
Bob holding a **playable** card on his chop that is not duplicated in his own
hand. A NOT-LOW condition of the *pace-clue tier gate*: it lifts every clue
that turn to at least MEDIUM, because the team is expecting that card to be
saved or played. Weaker than *at-risk chop* — it ignores copies in Cathy's
hand and in Alice's — which is the point: the card need not be in danger to be
worth a clue. `src/conventions/reactor0/state_eval.cpp:151-161`;
DECISION_MAKING.md, *Clue Tier Definitions*. Added after replay 1942330 T33.

### touched-card rank classification
How reactor0 decides whether a rank clue is a direct play clue (§1c priority
1): over the identities the **cards this clue actually touched** can hold —
`reactor::effective_possible_for` per card, then the pink promise in pinkish
variants — rather than over the variant-wide `touch_possibilities`.
**Reactor still uses the variant-wide set**, so the two conventions diverge
here. Introduced in v3.0.0 because an omni suit is touched by *every* rank
clue, which made the variant-wide set contain the omni suit at all five ranks
and stopped priority 1 firing at all in those variants.

Three narrowing steps: per-card `effective_possible_for`, the pink promise —
whose set is `{clue rank}` ∪ `{special rank}` when `pink_s`
(= `specialRankAllClueRanks`) is set, gated on the **flag** rather than the
name-based `includes_pinkish` — and, as of v5.0.0, "a **mixed** useful set is
not playable by this clue". A set holding both orange and non-orange useful
identities leaves the receiver unable to tell which button to press, so the
reading declines; an all-orange one does not, and becomes a *chuck*.
`src/conventions/reactor0/interpret_clue.cpp:364-450`.

### dc-target walk
Colour mode 2 tries each trash/dupe dc-candidate in turn instead of committing
to the leftmost, skipping a pairing whose react slot is dead **by shared
knowledge** and rejecting the clue outright when only the giver can tell (the
§1g split). Rank Phase C does *not* walk — it keeps the strict leftmost
dc-target. `src/conventions/reactor0/interpret_reactive.cpp:476-521`;
CONVENTION.md §1d.

### orange ladder
Reactor0's reading of a colour clue naming an inverted (Orange / Dark Orange)
suit, new in v4.0.0 and given a giver-side veto in v5.0.0. After the fix and
the play-reveal steps:

* **non-dark orange at `pace() > 3` → pitch.** The receiver presses **Play**
  on the leftmost touched orange they do not know is critical, which for an
  inverted suit sends it to the discard pile and regains a clue. Stamped
  `CALLED_TO_PLAY`; the clue interp is `DISCARD`.
* **`pace() <= 3`, or a dark inverted suit → chuck.** The receiver presses
  **Discard** on the leftmost touched orange that could still reach the
  stacks, which advances the orange stack. Stamped `CALLED_TO_DISCARD`; the
  clue interp is `PLAY`.
* **the giver then vets the chuck target against its own sight.** The target
  is picked from common knowledge, so the receiver lands on the same card; if
  the giver can see it is not currently playable the chuck is a misplay
  strike, and §1g permits only a **reject** (`nullopt` → `MISTAKE`), never a
  walk on to the next orange. Added in v5.0.0 after replay 1957905 #31. The
  pitch branch needs no veto — a pitch cannot strike.
* nothing reachable → `STALL`.

Dark forces the chuck because every dark card is a singleton, so a pitch
throws away the only copy. **Reactor does none of this** — it rejects a
stable colour clue on an orange target outright.
`src/conventions/reactor0/interpret_clue.cpp:274-314`; stamps at `:143-195`.

### includes_dark_inverted
True when the variant has a suit that is both `inverted` and `dark`, i.e.
Dark Orange. Like `includes_inverted` it reads the real `SuitType` flags
rather than matching suit names. Selects the chuck branch of the *orange
ladder* at any pace.
`src/conventions/variants/predicates.cpp:32-37`.

### holder_knows_critical
"Does the holder of this card know it is critical" — every identity in
`common.thoughts[order].possible` is critical. Uses `common` and raw
`possible` rather than `inferred`, which makes it POV-invariant: giver,
holder and every observer compute the same answer, so a convention rule may
branch on it without desyncing (§1g). Selects which touched orange the pitch
branch names.
`src/basics/player_game.cpp`, declared in `include/hanabi/basics/player.h`.

### react-slot vetting
The check a reactive path runs on the reacter's slot before committing to a
pairing, and the reason it has three outcomes rather than two: `OK`,
`RETARGET` (shared knowledge — the reacter walks to the next candidate too, so
every seat stays in step) and `REJECT` (giver-only knowledge — the reacter
would still act on this pairing, so the clue must not be offered at all). The
split is §1g.

**It follows the inverted swap.** Rank Phase A normally calls the reacter to
play but calls it to *discard* for an orange target; colour mode 1 does the
reverse. A play call is vetted for playability (POV-invariantly via
`effective_possible_for`, plus a giver-only "would strike" reject); a discard
call is vetted only for "not every possibility is critical". Vetting the
un-swapped call is bug_report_4.txt 4.1.

**A play call on an expendable orange is a PITCH, and is exempt.** When every
identity the react card could still be is inverted *and* basic trash
(`variants::can_pitch_for_free`), pressing Play throws it into the discard pile
at no cost, so the vet short-circuits to `OK`. The playability question can
never be answered yes by a trash identity, which is how a free pitch came to be
skipped — bug_report_4_1_0.txt 4.1.0b. The scope is *trash*, not *orange*:
pitching a useful orange still loses a copy and is still rejected.
`src/conventions/reactor0/interpret_reactive.cpp:187-262`.

### bluff
A reaction in which the reacter plays a card the pairing did not predict, so the
receiver's call set comes out empty and the reaction advanced a **non-inverted**
stack. Read by unwinding to the stacks as they were before the reaction and
taking the identities exactly one away from playable on a non-inverted suit.
The card is not playable yet, so the CALL IS DROPPED and only the inference is
kept. `CONVENTION.md` §1d.1.

### dupe bluff
The rarer half of the same branch: not even a one-away reading survives, so the
receiver must be holding the other copy of the card the reacter just played. It
is trash now, and the chuck list collects it by the ordinary rules.
`CONVENTION.md` §1d.1.

### static inferred set
The rule that a card's `inferred` may only ever be narrowed, never reset or
widened — not by a strike, not by a re-derivation, and not by withdrawing the
call that installed it. A stamp is a signal, an inference is permanent.
The one exception is a genuine contradiction, which escalates through
`CONVENTION.md` §1i's ladder.

### `[?]`
The note segment for ladder step (b): a narrowing emptied the set, resetting to
global empathy and re-deriving emptied it again, so no reading explains the
card. Left for diagnosis. `ConvData::note_mark`, `src/net/notes.cpp`.

### certain play
A card that advances a stack for EVERY reading its holder still has, pressed
with the button that does so — Play on a plain suit, Discard (a chuck) on an
inverted one. Readings spanning both kinds are never certain, since the two
halves need opposite buttons.

Deliberately wider than the two notions the endgame used before it:
`obvious_playables` is clue-derived, and `Thought::id(infer=true)` needs a
pinned singleton, so both miss a card read as `{a5, d5}` with both stacks on 4.
`endgame::certainly_advances` / `certain_plays`
(`src/endgame/helper.cpp`); the rule that uses them is DECISION_MAKING.md's
Precedence step 0.

The weaker sibling is **could advance**: the same suit/button pairing asked with
`exists` instead of `forall`, over our STANDING CALLS only
(`endgame::possible_call_actions`). It is tier 2 of the timeout pre-check —
when the solver has run out of time, actioning a call that might score beats
taking a truncated search's preference.
