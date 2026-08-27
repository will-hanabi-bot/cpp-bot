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
`src/conventions/reactor0/interpret_reactive.cpp:329-340`.
Not to be confused with *clue tier*, which is how worthwhile **giving** a clue
is. "Value" in reactor0 always means this anchor.

### at-risk chop (endangered chop)
A chop the team cannot afford its holder to pitch, judged from Alice's full
visibility. All four must hold: Alice knows the identity and it is not basic
trash; there is **no second copy in the holder's own hand**; no copy is
visible in the third player's hand; and Alice cannot prove she is holding a
copy (see *group elim*). Input to H1 and N2/N3 (DECISION_MAKING.md).
`src/conventions/reactor0/state_eval.cpp:165-196`. Stricter than reactor's
`chop_is_nontrash` (`src/conventions/reactor/state_eval.cpp:44-49`), which
tests basic trash only.

### clue tier
How worthwhile it is to spend a token on a candidate clue: `LOW`, `MEDIUM`,
`HIGH` or `VERY_HIGH`. Consumed by the *pace-clue tier gate*, which compares
with `>=`. **Not** the "clue value" of *anchor (value)* / *colour value* — those
are what a clue *means*, this is what giving it is *worth*.
`include/hanabi/conventions/reactor0/state_eval.h`; definitions in
DECISION_MAKING.md, *Clue Tier Definitions*.

**VERY HIGH** is the only tier that out-ranks a *pending reaction*
(DECISION_MAKING.md Precedence step 1, `choose_very_high_clue`), and VH1 — the
finesse rule — is its only member. Through v9.2.0 that rule was itself called
**H4**; v9.3.0 named the tier instead and reused "H4" for the unrelated
critical-chop rule at HIGH, so an "H4" in an older commit or log means the
finesse and an "H4" today does not.

### spent reaction
A pending reaction whose *target* — the receiver order the reacter's called slot
pairs with — has left the receiver's hand. A reaction is urgent because the
receiver decodes it from which slot the reacter actions; once the paired card is
gone there is nobody left to inform, so the call stops pre-empting the turn while
its reading stands. Only reachable after a deferral, since the reacter normally
acts first. The pairing is recorded at stamp time on
`ConvData::react_target_order` (`include/hanabi/basics/card.h`), written by
`record_react_target` (`src/conventions/reactor0/interpret_reactive.cpp`) and read
by the urgent scan in `src/basics/decide.cpp`; it is stored on the card rather
than on `Game::waiting` because a deferral clears the waiting connection while
deliberately keeping the call. Replay 1972716 T5.

### target parity
The rule, in **Alternating Clues** and **Synesthesia** only, that a clue's
reactive parity comes from **who is clued** rather than from the clue's kind:
a clue to Bob is odd (exactly one play), a clue to Cathy is even (double play
or double discard). Both families take the choice of clue kind away from the
giver — Synesthesia offers colour only, Alternating Clues forces the kind to
alternate — so the kind cannot carry a signal. `variants::uses_target_parity`
(`conventions/variants/predicates.h`); `reactive_assignment_for` is the
target-aware lookup. While it binds there are **no stable clues**: Bob is
always the reacter and Cathy always the receiver, so a clue to Bob touches the
reacter's own hand while identifying a slot in Cathy's.

**It stands down at 60%** (v11.0.0): once `score >= 0.6 * variant maximum` — a
constant 5 per suit, not `max_score()` — a clue to Bob is STABLE again, because
the odd bucket is a reactive discard and forcing one late costs more than it
buys. Clues to Cathy are untouched and stay even. `bob_clue_is_reactive`
(`reactor0/interpret_reactive.cpp:967-976`). In **Synesthesia**, which can never
give a rank clue, those stable clues read off a fixed colour table naming a
button and a slot — see *synesthesia table*. CONVENTION.md §1f.

It is the one place where the **clued seat and the receiver differ**, so both
are read from `clue_is_reactive` and `reactive_receiver`
(`reactor0/interpret_reactive.h`) rather than derived per-site. Replay 1973971
T15: five sites derived the receiver as `action.target`, walked the reacter's
own hand — invisible from his own seat — and a reactive discard clue read as a
MISTAKE.

### synesthesia table
The fixed lookup a **stable** Synesthesia clue is read with, once *target parity*
has stood down at 60%: each clue colour names one button and one slot in Bob's
hand. Red 1 *pitch*, Yellow 2 pitch, Green 3 *chuck*, Blue 2 chuck, Purple 5
pitch, Orange 1 chuck, any other colour 4 pitch. Keyed on the colour NAME, and
deliberately **not** the `colour_clue_value` table, which gives Blue 4.
`synesthesia_call` (`reactor0/synesthesia_stable.cpp:18-31`).

It exists because Synesthesia carries `clueRanks: []`, so the ordinary
colour/rank ladders have no split to express. Alternating Clues keeps those
ladders. A named action that is bad by COMMON knowledge stamps nothing and the
clue is a stall; one that only the seeing seats can tell is bad makes the clue a
MISTAKE (§1g). CONVENTION.md §1f.

### Synesthesia colour
The second colour a card answers to in a Synesthesia variant: a card of rank N
is touched by the **Nth** colour clue on top of its own colour
(`Variant::id_touched`, `rank - 1 == value` since the clue value is 0-indexed).
Brown suits are exempt by the rule; whitish suits are exempt because the rule
sits below the whitish early-return, which makes White indistinguishable from
Null there. Not to be confused with *colour value*, which is a clue's reactive
anchor and is unchanged in these variants.

### alternating clue
A clue in an **Alternating Clues** variant, where the server rejects a clue of
the same kind as the previous one given by anybody. Enforced in
`State::all_valid_clues` off `State::last_clue_kind`, which `Game::on_clue`
records; a play or discard in between does not reset it, because the rule counts
consecutive *clues* rather than consecutive turns.

### group elim (sudoku elim)
Proving Alice holds a copy of an identity without any of her cards being a
singleton: for a subset S of her hand with combined possibilities `u`, if
fewer than |S| copies of `u \ {id}` remain unaccounted for, one of them must
be `id`. |S| = 1 is the ordinary singleton case.
`src/conventions/reactor0/state_eval.cpp:70-111`. Distinct from `cross_elim`
(`src/basics/player_elim.cpp:165-226`), which strips locked identities from
cards *outside* such a group rather than identifying one inside it.

### pace-clue tier gate
Reactor0's replacement for reactor's low-clue-count gate, and the reason a
clue has to earn its token. Two windows, with **different pace thresholds**:

* Alice already holds a card stamped `CALLED_TO_PLAY` (or, in a variant with
  an inverted suit, `CALLED_TO_DISCARD`) — she has something to do either way,
  so the bar is **at least HIGH** while `pace() >= 1 && clue_tokens < 8`;
* otherwise the bar is **at least MEDIUM**, while `pace() >= 3 &&
  clue_tokens <= 3`.

Both compare with `>=`, so VERY HIGH clears either bar.

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
see the card reject the clue otherwise. `interpret_reactive.cpp:289-292`.

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
dc-target (or locks). `interpret_reactive.cpp:276-308`. The giver will not
*offer* one that buys nothing — see *pointless double discard*.

### pointless double discard
A double discard aimed at a receiver who was going to act safely anyway:
their chop is expendable, or they already hold a known play, a
`CALLED_TO_PLAY`, a `CALLED_TO_DISCARD` or known trash. Reactor0 drops such a
clue from its candidate set when a stable clue to Bob would instead get a card
played. The dedicated filter this named was removed in v7.0.0; the rule now
lives in the General Clue Evaluation List's priority 2 admissibility condition,
`discard_is_affordable` (`src/conventions/reactor0/decision.cpp:354-360`).
Reactive locks are exempt. Named from replay 1942181 T41.

### call invariants
The two rules constraining a hand's outstanding calls, enforced after every
reactor0 interpretation **and after every play and discard**: play calls run
newest slot → oldest in play order (a newer call on an older slot **erases** the
earlier call on a newer slot), and a hand holds **at most one**
`CALLED_TO_DISCARD` at a time. Revealed trash (`meta.trash`) is not a call.

The play-order erasure is **asymmetric in the kind of call** doing it, split on
`urgent`: a *receiver* call retires both kinds to its left, a *reacter* call
retires only other reacter calls. A reacter call is actioned by the urgent scan
on the very next turn and never joins the receiver deque, so it has no standing
to retire a receiver call (v10.12.0, replay 1974512).

Enforcing on every play and discard — not only when a reaction resolves — is
what lets the dead-call rules notice that somebody else advanced a stack past
every identity a call could still be (replay 1971981). See CONVENTION.md §1h;
`include/hanabi/conventions/reactor0/call_invariants.h`.

### deferred reaction negative
The inference a reaction makes about the receiver's OTHER slots — "the slots the
walk passed over were not playable". Captured when the reacter acts
(`Game::PendingReactionElim`) and fired when the receiver actions their target
(`Game::fire_reaction_elim`), because which slots and which set depend on what
the receiver does: a different stack means the passed-over slots only, the SAME
stack is a *finesse*, and no stack at all means they discarded. §1d.2.

Each negative is earned only if the **alternative existed** — the clue could
only have named receiver slot `S` by sending the reacter to his slot
`calc_slot(V, S, H)`, so if that slot of his could not have carried the reading,
the negative is unfounded and is not drawn. The test runs against
`effective_possible_for` — the reacter's empathy as every seat reconstructs it —
so all three seats draw the same negatives. Replays 1971882 and 1970589.

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
{1,5,4,3,2}). `interpret_reactive.cpp:211-274`.

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

### sarcastic discard / gentleman's discard
Inherited from the shared engine, not from anything reactor0 defines: a player
who throws a card the team can NAME is telling whoever holds the other copy that
they hold it (`DiscardInterp::SARCASTIC`, or `GENTLEMANS_DISCARD` when the card
is playable). `src/basics/sarcastic.cpp`, reached from `decide.cpp` — which is
**not** gated on convention, so it runs here alongside reactor0's own discard
vocabulary of CTD, chuck and pitch.

**"Can name" is the precondition**, and it is the whole of it: `useful_dc`
(`decide.cpp`) requires the card to have been pinned to one identity in `common`
before it was discarded, and for that to be what it turned out to be. A card
that was merely touched carries no signal.

Replay 1974218 is why. At T8 a clued i4 was discarded whose empathy was all six
4s; no i4 was visible in any hand because the second one was still in the DECK,
so the reading fell back on "then it is in mine" and linked over our own hand.
Fourteen turns later the link collapsed onto a **cardinal 2** and stamped it
`SARCASTIC` with `inferred = {i4}`. At T24 that made a rank-3 reactive play clue
unreadable — `target_play` narrows `inferred`, and `{i4}` has no playable
member, so Phase A walked past the ca2 in silence and Phase B blind-played a
dark 3 onto an empty dark stack. Note the two seats had disagreed ever since T8,
the giver's `common` having no such link, which is what let it offer a clue the
reacter could not read.

### starting required efficiency
`max_score / (8 + starting_pace + num_suits)`, regains halved under Clue
Starved — the hardness measure that picks the rlocks default.
`src/conventions/reactor0/efficiency.cpp`.

### chuck
Pressing **Discard** on an inverted card. The button stacks it, so the call is a
play attempt rather than a throw. The mirror of a *pitch*.

Two predicates share the name and are NOT the same question — see *chuckable*
below for the decision-side one. The interpretation-side one is
**`slot_is_chuckable`** (`reactor0/interpret_reaction.h`): could the reacter
press Discard on this slot at all? *Some* reading is a playable inverted card,
or *some* reading is a non-critical plain one. Existential, not universal, and
it is the union of the two arms of `stamp_react_discard_button`.

**`stamp_react_discard_button`** (`interpret_reactive.cpp:956-964`) is the
shared ladder every Discard-button reacter call goes through — rank Phase A's
and Phase B's inverted-target arms, Phase C's plain arm, and both colour modes.
It tries `stamp_orange_chuck` first (narrowing `inferred` to the identities the
button actually advances) and falls back to `reactor::target_discard` (the
non-critical plain reading). Refusing means *neither* arm applied, which is
exactly `!slot_is_chuckable(possibilities())`.

The ladder is v10.9.0, replay 1974342 T13. Before it the two colour sites
*chose* an arm with a gate that read `possible` while the chuck stamp reads
`possibilities()`; the deferred negatives pull the playables out of `inferred`
and leave `possible` alone, so on any reacter that had already reacted once the
gate picked the chuck, the chuck had nothing to name, and a plain reactive
discard read as a `MISTAKE`. The other three sites had no chuck arm at all. See
CONVENTION.md §1f.

### chuckable
A card safe to press Discard on: every reading is trash on a **plain** suit, or
every reading is an immediately playable **inverted** one (the two arms of
`is_chuckable`, `src/conventions/reactor0/calls.cpp`).

This is the **decision-side** predicate — "would pressing Discard here be safe?"
— and it is UNIVERSAL over the readings. Do not confuse it with the
interpretation-side `slot_is_chuckable` under *chuck* above, which is
existential: "is there a reading this call could mean?" A card can be
`slot_is_chuckable` without being `is_chuckable`.

The readings that count are `possibilities()` — `inferred` when it is non-empty,
else `possible` — narrowed for our own seat by `sight_narrowed`. **Whether a clue
was spent on the card makes no difference**: if every identity it can still be is
trash, it is trash.

v8.8.0 briefly required `possible` to agree for a clued or stamped card, on the
grounds that `inferred` is a deduction and a discard cannot be taken back. That
was withdrawn in **v10.4.0**. It had shipped alongside the real fix for replay
1971788 T29 — an Odds and Evens rank clue read as promising a literal rank
rather than a parity, which is what made a Dark Omni 5 look like a trash 1 — and
`rank_satisfies_promise` already prevents that reading. What the extra demand
cost was every genuinely-known trash card whose raw empathy still admitted
something useful, which after a colour clue is most of them: the chuck list came
back empty and phase 2 threw the chop instead. Replay 1974046 T22 lost a game
that way, discarding a critical b5 while holding a card read `{b2}` with blue on
2.

Not to be confused with *provably trash* (above), which is the same "every
reading is trash" question asked of `sight_narrowed` on the clue-interpretation
side.

### free chuck
A **playable** card on an **inverted** suit. Pressing Discard on it puts it on
its own stack, so losing it is not a loss — it is the play. It is therefore
treated exactly like basic trash or a same-hand dupe wherever a chop is weighed:
not endangered, not a play to arrange, and expendable (`chop_is_free_chuck`,
`reactor0/state_eval.cpp`). Replay 1973974 T10.

### pitch
Pressing **Play** on an inverted card. The button discards it, so the call cannot
strike and the card need not be playable — the only question is whether there is
a copy to spare. Distinct from a *chuck*, which is pressing Discard on an
inverted card to stack it. Replay 1973976 T12 needed both the vet and the stamp
to know the difference.

Two predicates, and which one is asked matters (`reactor0/interpret_reaction.h`):

* **`slot_is_pitchable`** — could the reacter press Play on this slot at all?
  Any playable **plain** reading, or any spare inverted one. This is the vet's
  question, and the deferred negatives'.
* **`slot_has_spare_inverted`** — its inverted half alone: is there a reading
  that is inverted and NOT critical? This is the pitch question proper, asked at
  step 3 of `stamp_react_play_button` once the play reading has been ruled out,
  where the plain half would be answering about a play that cannot happen.

A card whose **every** reading is inverted is a pitch outright (step 1). A clued
or stamped card with a mixed empathy is a pitch only as a **fallback**, after
`target_play` finds nothing that can play — v10.8.0, replay 1974331 T8. An
UNCLUED card is not: its empathy is wide enough to always admit some spare
orange, so allowing it would disable the strike checks across the board.

### provably trash
Every identity still open for a card is basic trash once the copies **this seat
can see** are accounted for (`provably_trash` / `sight_narrowed`,
`reactor0/state_eval.cpp`). Sight-based, so it is per-seat: the holder of a
duplicate cannot see it and reaches a different answer from everyone else.

What follows from it depends on **which seat is asking**, and the two are
opposite (CONVENTION.md §1g):

* **reading** a clue somebody else gave — it declines the play and reads a
  stall. The seat that acts has the better view, so no strike results. Replay
  1967478 T42.
* **giving** one — it REJECTS. The receiver cannot see what proves the card
  dead and will read the play regardless, so a clue whose promise only the giver
  can refute is one the giver must not give. Replay 1973575 T62.

### parity promise
Under Odds and Evens a rank clue names a class, not a rank, so the promise it
makes about a lock slot is that the card is **odd (1, 3, 5)** or **even (2, 4)**
— `variants::rank_satisfies_promise`. The same clue value read literally
narrowed replay 1971788's lock slot to rank 1 and condemned a Dark Omni 5 as
trash. Parity binds an Omni card like any other, because it is a property of the
rank; `Variant::id_touched` is *not* the right question, being true for every
rank of a pinkish suit.

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
worth a clue. `src/conventions/reactor0/state_eval.cpp:197-213`;
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
**Both** buckets try each trash/dupe dc-candidate in turn instead of committing
to the leftmost, skipping a pairing whose react slot is dead **by shared
knowledge** and rejecting the clue outright when only the giver can tell (the
§1g split). Rank Phase C kept a strict-leftmost rule until v10.6.0; the walk is
now the same rule in both. `src/conventions/reactor0/interpret_reactive.cpp`;
CONVENTION.md §1d.

### target priority
The order in which the reading looks for a reactive target, always
leftmost-first within a step, moving on when the reacter's own reaction does not
work: **playable → finesse (even bucket only) → trash**. Target selection comes
first and the reacter's action follows from it — the receiver's button is
whatever sheds or plays that card, and the parity decides the reacter's from
there (even matches it, odd opposes it). `slot_elims`
(`reactor0/interpret_reaction.cpp`) computes the same three categories for the
deferred negatives, and is the compact statement of the rule.

### pitch target
A **trash or same-hand-dupe card on an inverted suit**, named as a reactive
target. The receiver sheds it by pressing **Play** — a pitch throws it away —
not Discard, which on an inverted card is a chuck and would strike on anything
unplayable. So the parity lands the reacter on the *opposite* button to the
plain-trash case: **Discard** in the odd bucket, **Play** in the even one. A
CRITICAL inverted card is never a pitch target, there being nothing to spare;
this is `receiver_ctp_set`'s `!playable && !critical`, applied to selection.
Nameable since v10.6.0 — replay 1974257 T30, where every expendable card the
receiver held was orange, so the pool came back empty and the clue read as a
MISTAKE.

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
`src/conventions/reactor0/interpret_reactive.cpp:187-203`.

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

It has two more consumers since v8.5.0: forced-endgame rule 0, which makes a
certain play a FORCED action once the deck is empty, and the endgame's
reacter-call rule, which stands down while one exists. Since v8.7.0 its ABSENCE
is a precondition too — see *required play*.

### required play
An identity the team cannot score unless we lay it ourselves, this turn.

With the deck empty every other hand is visible, so this is computable rather
than searched: `endgame::best_reachable_plays` (`src/endgame/helper.cpp`) counts
the most cards the remaining seats can still stack, optimistically — each seat
either does nothing or lays a card whose true identity is playable, whether or
not that seat could name it. Price the round as-is, then again with a given
playable identity already laid; if the second is higher, that identity is
**required**.

Forced-endgame **rule 0b** gambles on it when we hold no *certain play*: among
our cards whose reading contains a required identity, the **leftmost clued**
one, else the leftmost of any, on the button the card's suit calls for. Replay
1970943 T24. Confined to `cards_left == 0`, and it carries no strike guard.

Distinct from a certain play in exactly the way the names suggest: a certain
play scores on every reading, a required play merely *might* be the card — the
justification is that nothing else on the turn is worth more.

**Rule 0c** asks it one card early, at `cards_left == 1`, where our play is what
draws the last card and opens the round. The ceiling is a weaker signal there —
an extra turn and an unseen draw — so the candidate must be **clued** with
exactly one non-trash reading, and that reading must be the required identity.
Replay 1972670 T25.

The weaker sibling is **could advance**: the same suit/button pairing asked with
`exists` instead of `forall`, over our STANDING CALLS only
(`endgame::possible_call_actions`). It is tier 2 of the timeout pre-check —
when the solver has run out of time, actioning a call that might score beats
taking a truncated search's preference.
