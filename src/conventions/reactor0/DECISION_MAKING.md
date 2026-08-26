# Reactor0 decision making

> **STATUS — phase 1 shipped in v7.0.0, phase 2 in v7.1.0, §4's stall rungs in
> v7.2.0.**
> This document is the ruling reference for **how reactor0 decides what to do on
> its turn**. `CONVENTION.md` remains the ruling reference for what a clue
> *means*.
>
> **This whole document now describes the build.** Reactor0 chooses its clue
> with the General Clue Evaluation List and its play or discard with the
> Actionable Card Priority list, and no longer uses the shared ladder in
> `src/basics/decide.cpp` at all.

This spec sheet replaces the mechanism for decision making that was done with
hardcoded heuristics imported from the previous reactor code. Often times, the
hardcoded heuristics result in unintuitive behavior with no easy way for a human
to reason about or to fine tune the underlying decision making mechanism. The new
version is meant to introduce a more rules-based algorithm mirroring how humans
actually play.

Reading conventions are `CONVENTION.md`'s: **slot 1 is the leftmost, newest
card**, and **Alice / Bob / Cathy** are positional — Alice is the clue giver, Bob
the next player, Cathy the one after.

---

## Clue Tier Definitions

We will mostly borrow the existing implementations of high/medium/low tier clues.
Note the change to H1 to also require that Cathy's chop be either playable or
critical.

A clue tier (`clue_tier`, `state_eval.cpp:424-520`) is VERY HIGH iff:

1. **VH1** — Cathy's chop is not trash or a same-hand-dupe, and the clue **gets a
   finesse**. A finesse is reactive Phase B, which belongs to the **even-parity
   ruleset** rather than to a clue kind: normally that is the rank clue, but
   Odds and Evens makes it the colour clue and `/set` can move an individual
   one, so the test reads `reactive_assignment(...).even`. Testing the kind
   instead made VH1 unreachable in those variants (replay 1967416 T1). A reactive **lock** is explicitly not a finesse, however its
   predicted slot looks at clue time (`clue_is_vh1`, `:411-422`, applied at `:458`;
   the finesse detector itself is `clue_gets_finesse`, `:317-366`).

VERY HIGH is the tier that out-ranks a **pending reaction** (Precedence step 1),
and VH1 is deliberately its only member.

**A note on the name.** Through v9.2.0 this rule was itself called **H4**, and
Precedence step 1 singled it out by name. v9.3.0 named the tier instead, which
freed "H4" for the unrelated rule now listed fourth under HIGH below. An "H4" in
an older commit, log or comment means the finesse; an "H4" here does not.

Otherwise, a clue tier is HIGH iff **any** of:

1. **H1** — ALL of H1a, H1b, and H1c.
    - **H1a — Bob's chop is endangered.** Bob is not locked, and has no safe
      action (no obvious play, no known trash, no CTD — all three are covered by
      `thinks_trash`, `player_game.cpp:115-132`), and his chop is *endangered*
      (below). `:469-478`. The "no safe action" half is shared with H4a
      verbatim (`bob_stuck`, `:441-443`); H1a and H4a differ only in how bad the
      chop is.
    - **H1b** — Cathy's chop is not playable or critical, **judged from Alice's
      full visibility** (the same viewpoint as *endangered chop* below, not
      common knowledge). If Cathy has no chop, this condition is vacuously true.
    - **H1c** — Cathy's chop is either a trash or a same-hand-dupe, *or* Bob does
      not have a colour stable clue to give to Cathy. In a **target-parity**
      variant (Alternating Clues, Synesthesia) there are no stable clues at all,
      so `has_colour_play_clue_for` returns false outright and this arm is
      vacuously satisfied — see CONVENTION.md §1f.
2. **H2** — the clue gets a **critical 1 or 2** played (5 or 4 on a reversed suit,
   via `variants::is_first_or_second_rank`). `:481`.
3. **H3** — the clue gets **two new plays**, at least one at the clue-regain rank
   (5 normally, 1 reversed, `variants::is_clue_regain_rank`). `:483`.
4. **H4** — BOTH of the following must hold (`:495-498`, with `cathy_can_wait`
   at `:447-448`):
    - **H4a — Bob's chop is critical.** Bob is not locked, and has no safe
      action (no obvious play, no known trash, and no CTD). Same predicate as
      H1a's first half; H4a asks for a strictly worse chop.
    - **H4b** — Cathy's chop is not playable or critical, **judged from Alice's
      full visibility** (the same viewpoint as *endangered chop* below, not
      common knowledge). If Cathy has no chop, this condition is vacuously true.

   **H4 is HIGH and not VERY HIGH, deliberately — a pending reaction still
   outranks it.** Like H1 and N5 this is a property of the **position**, not of
   the candidate clue, so it lifts *every* legal clue that turn. At VERY HIGH it
   would therefore fire Precedence step 1 unconditionally and out-rank the
   pending reaction, phase 1 and phase 2 together: measured over the corpus turns
   that action a reaction, that moved **171 of 3332**, and **137 of those gave up
   a known play** — including replay 1970589 T42, where the seat's own urgent p3
   was replaced by a rank clue. At HIGH it only widens what `clue_is_admissible`
   will pass, which is the intent.

NOT-LOW iff any of VH1, H2, H3, H4, or:

5. **N5 — Bob's chop is playable** and is not duplicated in his own hand
   (`has_playable_chop`, `:160-168`; applied at `:505`). Like H1 this is a
   property of the **position, not of the candidate clue**, so it lifts every
   clue that turn to at least MEDIUM. Deliberately weaker than `at_risk_chop`: it
   asks only "playable, and Bob cannot just pitch a spare copy", and does *not*
   care whether a copy sits in Cathy's hand or is provable in Alice's — the point
   is not that the card is in danger but that it is a play the team should be
   collecting, and Cathy is already expecting Alice to save it or get it played.

   §3's precondition takes this same predicate as its second arm, for the same
   reason. Tier and priority agree: a safe-but-playable chop is worth a clue.

…or, when **Cathy's** chop is endangered (`:507-518`):

6. **N3** — the clue gets two new plays. `:509`.
7. **N2** — the clue is **reactive** and Bob has no stable color play clue he
   could give Cathy. Reactive is a single integer compare, `action.target != bob`,
   since dispatch is positional (§1a, `interpret_clue.cpp:620-631`). `:514-517`.
   In a **target-parity** variant the second arm is vacuously true (no stable
   clues exist) while the first still asks who was clued, so only a clue to
   Cathy reaches N2 there.

**H1a is NOT on that list, though the spec has always said it should be.**
`clue_tier` has never had an `if (h1a) return MEDIUM` arm: a position where Bob
is stuck on an endangered chop but H1c fails reads **LOW**, not MEDIUM. See
`TODO.md` §30 — the note is here rather than a quiet correction because the rule
as specified is the one a reader should expect.

MEDIUM is NOT-LOW and not HIGH; LOW is everything else. "New plays" are counted as
CTP-status transitions between the real game and the clue's hypo
(`new_play_facts`, `:173-224`) — the same walk reactor's `is_high_value_clue`
uses.

**"Gets a finesse"** (VH1) means the clue's interpretation is reactive rank
**Phase B**, and *only* Phase B — the blind-play phase that walks one-away
targets and calls the reacter onto the prerequisite
(`interpret_reactive.cpp:383-447`). Phase A (double play, `:307-382`) and Phase C
(double discard, `:448-482`) are not finesses. A reactive lock has to be excluded
explicitly (`predicts_reactive_lock`): it stamps CHOP_MOVED a turn later, so at
clue time the receiver's predicted slot carries no status and looks exactly like
an un-stamped Phase B target. Since VERY HIGH is the one thing that outranks a
pending reaction, a lock misread as a finesse lets Alice abandon a reaction to
give it — replay 1966091 T10, which cost a strike.

**Endangered chop** (`at_risk_chop`, `:130-152`), judged from Alice's full
visibility. All of the following must hold:

1. the identity is known to Alice and not basic trash;
2. there is **no second copy in the holder's own hand**;
3. no copy sits in the third player's hand, and Alice cannot prove she holds a
   copy herself.

The first of those is stricter than reactor's `chop_is_nontrash`
(`reactor/state_eval.cpp:45-50`), which tests only `is_basic_trash` — a chop the
holder can safely pitch because they hold the other copy is not in danger.

**"Alice provably holds a copy"** (`alice_provably_holds`, `:69-110`) extends
reactor's singleton test (`reactor/state_eval.cpp:99-102`) to **group ("sudoku")
elim**. For any subset S of Alice's hand, let `u` be the union of what those |S|
cards could be; if fewer than |S| copies of `u \ {id}` are still unaccounted for,
then at least one of them must be `id`. |S| = 1 reduces to exactly the singleton
rule. Inference sets come from `common` (the view reactor reads, and the one the
engine and test harness both maintain); availability counts come from `me()`,
which can see the other hands. It errs safe in both directions — a false "Alice
holds it" would kill a save clue, so the bound deliberately over-counts. A
3-player hand is 5 cards, so all 31 non-empty subsets are enumerated directly;
`cross_elim` (`src/basics/player_elim.cpp:165-226`) solves the dual problem (it
strips locked ids from cards *outside* the group) and cannot answer this.

**Bob's colour play clue for Cathy** (`has_colour_play_clue_for`, `:231-249`) is a
structural check, not a simulation: for each colour clue Bob could give Cathy it
replays `stable_colour`'s target choice (§1b step 5,
`interpret_clue.cpp:327`) plus its three guards (`:340-345`), then asks whether
the named card actually plays. Two known approximations, both deliberate: it skips
the FIX branch above the direct-play read (`interpret_clue.cpp:245-248`), and it
does not model a play reveal (`:256-261`). It is evaluated **last** in `clue_tier`,
behind the O(1) reactive test, because it is the only costly term.

---

## The static inferred set

Once an inferred set is constructed it is only ever **narrowed**, and never
reset — not by a strike, not by a re-derivation, and not by a dropped call. A
CTP or CTD stamp is a *signal*, which can be withdrawn; the inference it
installed is *permanent*. The full rule, including the escalation ladder for a
genuine contradiction, is
[CONVENTION.md §1i](CONVENTION.md).

The decision layer depends on this directly. `has_no_safe_action` reads
`common.thinks_trash`, which reads `inferred`; if withdrawing a dead play call
also withdrew its inference, the holder would stop knowing the card is trash and
§3 would spend a clue rescuing a chop that needed no rescue. Replay 1967558 is
that position.

A strike in particular does not reset anything (`src/basics/decide.cpp`).
Reactor resets on a bomb,
because there a strike means a finesse or dupe chain was misread and the
convention chain that produced the misplayed card is broken. Under reactor0 a
strike is a **normal outcome of the convention**: the stamp is the instruction,
so a card stamped CTD is chucked, and on an inverted suit a chuck that turns out
unplayable simply bombs. Nothing was miscommunicated, so the promises other
clues made about other cards still stand.

Replay 1966687: will-bot67 chucked an Orange 4 on T14, and the resulting strike
wiped will-bot69's slot 1 from `{o2}` back to `{o1,o2,o3}` and dropped its CTD
stamp with it.

An inferred set is narrowed by `card_elim`, by the reactive negative inferences
(now deferred until the receiver acts — CONVENTION.md §1d.2), and by a
convention stamp; never by good-touch elimination. Link resolution
(`elim_link`, `refresh_links` in `src/basics/player_elim.cpp`) also pins an
identity outright, which is positive evidence of the same class as `card_elim`.

## Framework

The priority of evaluation on a given player (Alice)'s turn is:

1. What clue should Alice give (if any)?
2. If Alice cannot or should not give a clue, what card should Alice play or
   discard?

### Precedence

Two things outrank the phases below, and one thing sits between them:

0.  Endgame.  The forced-endgame rules and the exact solver run first
    (`decide.cpp:892`, the `rem_score() <= num_suits + 1` fork).  The SOLVER
    additionally needs `pace() <= num_players` (`:990`); the forced rules do
    not.  They are unchanged by this spec, with one guard on top: a CERTAIN
    play outranks a speculative one — see below.  The endgame decides
    WHETHER to clue; when its answer IS a clue, the endgame stall list below
    decides which one.

0b. **The endgame may decline to clue.** When step 0's answer is a clue but
    every candidate has been dropped as undecodable or vetoed as predicting a
    strike, `prefer_stall_clue` returns nothing and the fork falls through to
    the steps below rather than giving an unvetted clue (v10.1.0, replay
    1973575 T62). `choose_clue` reads the same filtered pool, so in that
    position it declines too and step 4 plays or discards.

1.  **A VERY HIGH tier clue**, if one is available.

2.  **A pending REACTION.**  If Alice holds a reacter-CTP — or, in a variant
    with an inverted suit, a reacter-CTD — she actions it.  Only a VERY HIGH
    tier clue outranks this.
      - no inverted suit in the variant: only a reacter-CTP is urgent;
      - inverted suit present:  reacter-CTP and reacter-CTD are equally urgent.

    A reaction stops being urgent once its **target has left the receiver's
    hand** (`ConvData::react_target_order`, applied in `decide.cpp`'s urgent
    scan).  The call and its inference stand; only the urgency lapses.

3.  **Decision phase 1** — giving a clue.

4.  **Decision phase 2** — deciding what to play or discard.


**The solver needs the deck to be running out (step 0).** `rem_score() <=
num_suits + 1` counts the points still missing, not how close the deck is to
empty, so on a 6-suit variant it opens around the halfway mark and stays open;
303 turns in the log corpus sat inside it with 8–16 cards left. The second gate
is `pace() <= num_players` (`decide.cpp:990`), which scales with the seat count
because pace already does — at 3 seats it is exactly `pace() <= 3`. The
forced-endgame rules sit ABOVE it and keep running on the points condition
alone; a closed gate falls through to the phases below.

**A required play, when none is certain (step 0).** With the deck EMPTY and
nothing in hand certain, the bot will gamble on a card that *could* be the one
the team needs. `best_reachable_plays` (`src/endgame/helper.cpp`) prices the
rest of the final round with full sight of every other hand — once as-is, once
per currently-playable identity. An identity that raises the ceiling is
REQUIRED: nobody else is going to cash it in time. Among our cards whose
reading contains one, the rule takes the **leftmost CLUED** card, else the
leftmost of any, on the button that suits the card (Discard on an inverted
suit). Rule 0 above still wins when a card certainly scores — a sure point
outranks a gamble — and the whole rule is confined to `cards_left == 0`.

Replay 1970943 T24: stacks `[3,5,5]`, deck empty, three turns left. The next
seat's hand was all trash and the seat after held BOTH r4 and r5 with one turn
to spend, so only our laying the r4 let the r5 score. Our slot 4 was clued and
read `{r2,r4,o1,o2,o3,o4}` — it was the r4. The bot chucked trash and the game
ended 13/15. Note the clued-first priority did the work: slots 1 and 2 could
also have been the r4 and sit further left, and slot 1 was an Omni 1.

It deliberately carries **no strike guard** — it fires even when a miss would
be the game-ending third strike. On these turns the alternative is nearly
always a trash discard.

**A required play one card early (step 0).** The same rule, asked with ONE card
still in the deck. Our play draws the last card and opens the final round, so
the window is the next `num_players` seats — every seat once, and our own twice,
though our hidden cards count for nothing in the ceiling.

With a card still to come the ceiling test is a much weaker signal, so the
CANDIDATE carries the confidence instead: it must be **clued**, and everything
it could be that is not already trash must be the **single** required identity.
Replay 1972670 T25 — slot 4 read `{r2,r4,ra2,ra4}` and the stacks killed all but
the r4, while slot 1 had the identical reading but was unclued and so was not a
candidate.

It fires on 16 turns across the log corpus, of which 8 score and 8 strike. That
is a deliberate trade: when it fires the alternative could not reach that score
either. It carries no strike guard, so on rare occasions it is the action that
ends a game.

**A certain play outranks a speculative one (step 0).** When the endgame's
answer is a play, and we hold a card that certainly advances a stack, the answer
must be one of those. "Certainly advances" asks the question of the BUTTON and
of every reading the holder still has (`endgame::certainly_advances`), so it
sees a card read as `{a5, d5}` with both stacks on 4 — which scores whichever it
is, and which neither `obvious_playables` (clue-derived) nor `Thought::id` (a
pinned singleton) can recognise. Clues and ordinary discards are untouched: the
solver is often right to stall or to throw.

**A standing reacter call outranks the endgame search (step 0).** The receiver
acts NEXT and decodes their target from WHICH slot we actioned, so deviating
from the call does not merely spend a different card — it redirects them. The
solver prices that at zero, because it never models the convention reading our
own action; it assumes the other seats play what they know. Replay 1969860 T55:
the solver returned slot 2 where slot 5 was called, and `calc_slot(4, 2, 5) = 2`
redirected will-bot69 onto a Null 5 that was not playable.

Two things sit above it. `forced_endgame_action` runs first — a forced action
takes precedence over any conventional interpretation — and the rule **stands
down whenever we hold a card that certainly scores**, because a guaranteed point
is worth more than the signal and sequencing it is what the search is for
(replay 1957936 T41: an urgent CTD of plain trash beside a pinned Orange 2 whose
chuck wins 20/20). A call the holder can see is dead is skipped as always, so
the solver keeps those turns too.

**A timed-out search does not outrank an action we can see is good (step 0).**
Roughly a third of endgame solves hit the 6 s deadline in practice, and a
truncated search still answers — its result looks exactly like a completed one.
When the solver reports `timed_out`, take an action whose value we can establish
ourselves before deferring to a search that never finished comparing its
options:

1. a card every reading of which advances a stack (`certain_plays`);
2. a standing CTP/CTD whose button COULD advance one (`possible_call_actions`);
3. otherwise the ordinary handling.

This runs ABOVE the fork's `winrate >= 1%` accept test, not inside it: the most
degenerate timeouts do not come back with a usable result at all, and those are
exactly the turns where the search knows least. The one exception is a reported
certainty — a timeout only ever makes a position look WORSE (every deadline
check scores its branch as a loss), so `winrate == 1` from a truncated search is
a genuine proven win and is left alone.

The guard sits at the fork rather than inside one routine because several of
them resolve a choice by hand order and any can be the one that answers —
`trivially_winnable` walks `obvious_playables` and takes `.front()`, and
equal-winrate plays fall to enumeration order in the solver's `optimize`.
Replay 1969779 T68: on the final turn at 28/30, will-bot67 held that `{a5, d5}`
card and played a different one reading `{a1, a5, b1, d5, e1}`. It was the b1 —
a second strike, and the d5 never played.

Step 2 is **reacter-only**. A pending reaction is urgent because the receiver is
decoding against it — he learns which of *his* slots the clue named from which of
*ours* we action — so it interrupts everything below it, and only a VERY HIGH
clue outranks it.

That justification is also its expiry date. **Once the paired card has left the
receiver's hand there is nobody left to inform**, and the call stops being urgent:
the urgent scan skips it exactly as it skips a call the holder can now see is
trash. The reading on the card stands; only the urgency lapses. The paired
receiver order is recorded on the reacter's card when the call is stamped
(`ConvData::react_target_order`, `card.h`; written by `record_react_target` in
`interpret_reactive.cpp`) rather than read back off `Game::waiting`, because a
**deferral clears the waiting connection while deliberately keeping the call**.

In practice this only bites after a deferral, since the reacter normally acts
before the receiver ever gets another turn. Replay 1972716 T5: will-bot69 was
called to discard slot 1, deferred at T2, the receiver played the paired card at
T3 — and at T5 the spent call still pre-empted the clue that had to be given to
save Bob's chop. Two playables were lost.

**A spent call is not a dropped one.** The stamp, the narrowed `inferred` and the
status all stand, and a `CALLED_TO_PLAY` card is an *obvious playable* whatever
its urgent flag (`player_game.cpp:147-153`), so phase 2 still actions it on a
turn with nothing better to do. What lapses is only its claim on steps 3 and 4.
Measured against the build before the rule, over all 1885 corpus turns whose
trace contains `precedence.urgent_reaction`, it moves **15** — 0.8%.

**Reactor0 only**, although the test itself sits in shared code. Only reactor0's
`interpret_reactive` records `react_target_order`; reactor leaves it at -1, where
the check reads as "no pairing recorded" and stands down. Reactor cancels a call
on a deferral outright (`check_missed`), so it never reaches the position this
rule is about.

Step 1 is implemented as `choose_very_high_clue` (`decision.cpp`), spliced into
`Game::take_action` **above** the urgent return that actions the reaction. It
deliberately does not apply §4's floor: the floor exists so §4 always returns
something at 8 tokens, and applying it here would let an arbitrary clue outrank
a reaction. The candidate set is built and analysed once per turn and shared
with step 3, so the pre-check costs no extra simulation. It reads
`ClueCandidate::tier`, so **both** ways into VERY HIGH pre-empt the reaction;
through v9.2.0 it was `choose_h4_clue` and only the finesse could.

A **receiver**-side call carries no such urgency. It makes Alice *occupied*, which
is what phase 1 rule 1a keys on — so Alice may give **any HIGH-tier clue** while
holding one, not only a VERY HIGH one. The call itself is actioned in phase 2.

---

## Decision phase 1 — giving a clue

We'll keep the current clue tier evaluation thresholds from the reactor decision
making framework.

Alice is **occupied** when she holds ≥1 card stamped `CALLED_TO_PLAY`, *or* — in a
variant containing an inverted suit — ≥1 stamped `CALLED_TO_DISCARD` that she
knows is **not** the next card on that inverted suit's stack. This knowledge need
not be global: it is Alice's own inference that counts. Note *occupied* is not the
same as *loaded*.

Concretely, `requires_high_tier` (`state_eval.cpp:253-265`) reads the stamp
literally and counts a `CALLED_TO_DISCARD` **only in a variant that contains an
inverted suit** — there, pressing Discard is how an inverted card is played, so
the call is a deferred play. In a plain variant a reacter-CTD does not occupy
Alice at all.

It does not itself consult `variants::possible_chuck_advances_stack`
(`variants/inverted.cpp:96-102`). A chuck call that can no longer advance a stack
stops occupying Alice because **call invariant 4 erases the call**, not because
this predicate is tested here.

Whether a clue should be given is then:

1. **Alice is occupied.**
    - 1a. If `pace() >= 1 && clue_tokens < 8`, Alice gives the best clue from the
      General Clue Evaluation List satisfying at least the **HIGH** tier requirements.
    - 1b. Otherwise, Alice gives the best clue from the General Clue Evaluation
      List below.
2. **Alice is not occupied.**
    - 2a. If `pace() >= 3 && clue_tokens < 4`, and Alice is not locked, Alice gives the best clue from the
      General Clue Evaluation List satisfying at least the **MEDIUM** tier
      requirements.
    - 2b. Otherwise, Alice gives the best clue from the General Clue Evaluation
      List below.

**On 2a's locked clause.** An unoccupied Alice who is LOCKED is exempt from the
MEDIUM bar. She has no chop to discard, so cluing is the only thing she can do
that is not burning a card — the same reasoning that exempts 8 clue tokens,
where a discard is illegal. Without it the gate can empty the candidate set
before §4 is reached, and §4 is exactly the branch that promises to return a
clue when she is out of alternatives. Rule 1a carries no such clause: an
occupied Alice holds a call she can action, so she is not out of alternatives.

**On the pace conditions.** The two rules take different thresholds, and the
difference is the point.

**1a is `pace() >= 1`.** It was `pace() >= 3` through v7.3.0, inherited from
reactor's low-clue-count gate, and that left a hole: at replay 1966119 T5 an
**occupied** Alice sat at pace 2, the gate stood down, and a LOW-tier reactive
discard became admissible — neither a HIGH clue nor the pending call. An
occupied Alice always has that call to fall back on, so a LOW clue is never the
better use of the turn, whatever the pace.

**2a is `pace() >= 3`.** An unoccupied Alice has no such fallback. Holding her
to the MEDIUM bar with the deck nearly out does not buy a better clue later —
there is no later — it just sends her to the discard pile. Nothing about 1966119
argues for this window; that replay was the occupied rule. Note the consequence
at replay 1966687 T14 (pace 2, 3 tokens): the bot now gives a LOW reactive where
it used to fall through and chuck a known duplicate. That is the intended shape
of the change.

**Pace 0 is exempt in both**: there every remaining turn must produce a play or
the game cannot finish, and hoarding a token for a better clue is pointless.

If Alice does not have a clue available from the General Clue Evaluation List,
Alice moves to the play/discard decision phase.

### General Clue Evaluation List

We evaluate the following list by priority. The default tiebreak is defined as the
clue that maximizes the quantity

```
1.99 * (# of new useful cards touched)  -  (# of new trash cards touched)
```

and will be listed below in some priority. If there are still ties after all
tiebreaks have been applied then we simply choose the first available clue.

- **New** means the card was unclued before this clue
  (`!prev.state.deck[o].clued`), matching how `elim_result` counts new touches.
- **Trash** means basic trash from the giver's full visibility; **useful** means
  every other newly touched card.
- The `1.99` rather than `2` is deliberate: two useful cards plus one trash
  (`2.98`) still beats one useful card alone (`1.99`), but one useful plus one
  trash (`0.99`) does not.

All uses of "play" and "discard" below are **result-oriented**. A play refers to a
pitch of a non-inverted suit and a chuck of an inverted suit, while a discard
refers to a chuck of a non-inverted suit and a pitch of an inverted suit.

A **reactive play clue** is a reactive clue that stamps two cards so that when
actioned on, two cards are played.

A **reactive discard clue** is a reactive clue that stamps two cards so that when
actioned on, one card is played and one card is discarded.

A **double discard clue** is a reactive clue that stamps two cards so that when
actioned on, two cards are discarded.

**"Card X connects to card Y"** means Y is the immediate successor of X on X's
suit — playing X makes Y playable. Where a rule says "connects to another card in
Alice's hand (that Alice knows, not necessarily globally known)", the connection
is judged from Alice's own inference, not common knowledge.

---

1. **Alice has a reactive play clue available.** If there are multiple such clues,
   tiebreak by the following:
    1. Bob's card connects to another card in either Bob's or Cathy's hand
    2. Bob's card connects to another card in Alice's hand (that Alice knows, not
       necessarily globally known)
    3. Bob's card is a critical 1 (or 5 in a reversed variant)
    4. Bob's card is a critical 2 (or 4 in a reversed variant)
    5. Bob's card is a clue-regain card (5's in normal variants, 1 in reversed)
    6. Default tiebreak.

2. **Alice has a reactive discard clue available where Bob plays a card**, where Cathy's discarded card is
   trash, a same-hand-dupe, or a good card that Alice sees at least one dupe of in
   any other player's hand (including her own). Tiebreak by the following:
    1. If Bob plays, then Bob's card connects to another card in either Bob's or
       Cathy's hand
    2. If Bob plays, then Bob's card connects to another card in Alice's hand
       (that Alice knows, not necessarily globally known)
    3. If Bob plays, then Bob's card is a critical 1 (or 5 in a reversed variant)
    4. If Bob plays, then Bob's card is a critical 2 (or 4 in a reversed variant)
    5. If Bob plays, then Bob's card is a clue-regain card (5's in normal
       variants, 1 in reversed)
    6. The discarded card is a same-hand-dupe.
    7. The discarded card is trash.
    8. Default tiebreak.

   A **double discard is not a reactive discard clue** and never enters here — a
   reactive discard plays one card and discards one, by the definition above.
   Double discards are ranked by priority 3 rungs 2 and 4, and by §4.8.

   Together those placements are what make a separate "pointless double discard"
   filter unnecessary. The filter existed to stop a zero-play double discard
   beating a stable play clue to Bob; now it cannot, because every rung a double
   discard can reach sits below rung 3.1.

3. **Bob's chop is worth a clue** (so in particular he is not locked) **and he
   has no safe play or discard — or `pace() <= 2`.** The pace arm waives only
   the safe-action half: at low pace there are few turns left to collect that
   chop in, and spending one on a discard Bob could have made anyway is how a
   playable card ends up buried. A locked Bob, and a chop that is neither
   endangered nor playable, still skip §3 — those ask whether the clue is worth
   giving at all, which low pace does not change.

   Worth a clue means either of two things, and
   "non-trash" — which is what this said until v7.22.0 — was too weak to
   separate them from the case with neither:

   - **endangered** (`at_risk_chop`, the same test H1a uses), or
   - **playable** (`has_playable_chop`, N5's test).

   A chop that is neither earns nothing. Replay 1966745 T5: Bob's chop was an r2
   with red on 0, and Cathy held the other r2 — not endangered *and* not
   playable — yet §3 fired and spent a clue on it. The second arm is equally
   load-bearing: at replay 1942330 T33 Bob's chop was a **playable** Navy 2 also
   duplicated in Cathy's hand, so it was in no danger, and the Blue play clue
   that collects it is still right (`priority_3_applies`, `decision.cpp`). Every condition marked `>= N clues**` below means the
   condition also applies at `>= N-1` clues if Cathy has a safe discard, or if
   Cathy has on chop a trash, a same-hand-dupe, or a good card that Alice sees at
   least one dupe of in any other player's hand (including her own). Tiebreak by
   the following:
    1. Give a stable play clue to Bob if there are `>= 2 clues**`. This includes
       direct rank or color play clues and play reveals given with either rank or color. 
    2. If pace is >= 3 and Cathy's chop is not a trash card or a same-hand-dupe, give a double discard clue
       that stamps CTD on two trash cards or same-hand-dupes, or CTP to a trash or same-hand-dupe
       in an inverted suit.
    3. If Bob does not already have a safe discard that is common knowledge between Alice and Bob,
       give a stable discard clue or trash reveal clue to Bob that stamps CTD on a trash card
       or same-hand-dupe, or a CTP to a trash card in an inverted suit
    4. If pace is `>= 3`, give a double discard clue that stamps CTD on two trash
       cards or same-hand-dupes, or CTP to a trash or same-hand-dupe in an
       inverted suit. Same pace condition as 3.2, for the same reason: a double
       discard spends a clue to clear cards the team could have thrown anyway,
       and when turns are the scarce resource it is not worth one. §4.8 carries
       no such condition, because §4 must always return a clue.
    5. Give a stable discard clue to Bob that stamps CTD on a card for which a
       dupe exists in Cathy's hand (or Alice's hand, if known by Alice), or a CTP
       to a card in an inverted suit whose dupe is seen by Alice.
    6. Give a lock clue to Bob if all of Bob's cards are critical and there are
       `>= 2 clues**`
    7. If there are >= 3 cards in Bob's hand with at most one **missing connector**
       Alice can see in Bob's own hand, **and** Bob cannot give a stable play clue to Cathy,
       **and** Cathy's chop is not critical, then give a lock clue to Bob.
    8. If Bob's chop is critical, give a reactive discard that stamps CTD on a non-critical card in Bob's
       hand, tiebreak by the largest number of **missing connectors** Alice can
       see leading up to that card, or a reactive play that stamps CTP on a
       non-critical inverted card in Bob's hand, tiebreak by the same criteria.
       **This rung is unconditional** — it carries no clue-count condition, and
       the `**` relaxation does not reach it.
    9. If Bob's chop is critical, give a stable discard that stamps CTD on a non-critical card in Bob's
       hand, tiebreak by the largest number of **missing connectors** Alice can
       see leading up to that card.

   **Missing connectors** of a card `X` = the number of identities strictly
   between the top of `X`'s stack and `X` that are **not** visible to Alice in any
   hand. Worked example, no cards on the stacks, Alice sees Bob as
   `r2 g3 r4 g4 b4` and Cathy as `r3 r3 b5 g5 p5`: `b4` needs `b1 b2 b3`, none
   visible → 3; `g4` needs `g1 g2 g3`, and `g3` is in Bob's hand → 2; `r4` needs
   `r1 r2 r3`, and `r2` is in Bob's hand and `r3` in Cathy's → 1. So `b4` wins.

4. **Alice is locked or at 8 clues and is forced to clue or pitch.** Tiebreak by the following:
    1. Same as 3.1
    2. Same as 3.3
    3. Same as 3.5
    4. Give a fill-in clue (which is a stable clue that narrows down the identity
       of existing unplayable clued cards in Bob's hand). Prioritize cards that
       are duplicated in either Cathy's hand or Alice's own hand, followed by
       cards ranked by lowest number of connectors and lowest stack rank.
    5. Give any other stall clue that cannot be misinterpreted by Bob as some
       other type of stable clue that would cause a strike or a discard of a
       critical card.
    6. Give a lock clue to Bob.

   **When §4 is reachable at all.** §3 sits above §4, and its own last rung (3.9)
   is a lock with the same `>= 2 clues**` condition — which 8 tokens always
   satisfies. So whenever §3's precondition holds (Bob's chop is endangered or
   playable, and he has no safe play or discard), 3.9 fires and none of §4 runs. §4 is therefore the
   branch for a forced clue when **Bob is not in trouble**: his chop is neither
   endangered nor playable, he already has something safe to do, or he is
   locked. That is the right
   precedence — an endangered chop outranks a stall — but it is worth stating,
   because it means the rungs below are rarer than their position suggests.

   Rungs 4.4 and 4.5 sit **above** the lock, unlike their counterparts in §3.
   Two reasons. A lock commits Bob's whole hand, so at a forced clue it is worth
   less than information or than a harmless stall. And at 8 tokens a re-clue of
   already-clued cards *reads* as a lock, so a lock candidate exists in
   essentially every position — putting it above these two would make both
   unreachable.
    7. If at < 2 strikes, give a stable clue to Bob that will cause him to pitch a trash/duped
       non-inverted suit or chuck a trash/duped inverted suit (explicitly allowing a strike here).
    8. Give a reactive discard or double discard clue that stamps CTD on a non-critical card
       in Bob's hand, tiebreak by the largest number of **missing connectors** Alice can
       see leading up to that card, or a reactive play or reactive discard that stamps CTP on a
       non-critical inverted card in Bob's hand, tiebreak by the same criteria.

   **§4 always returns a clue.** Both entry conditions are positions where the
   ordinary list has run out and Alice still has to do something: at 8 clue
   tokens a discard is illegal, and locked she has no chop to discard, so her
   only alternative to cluing is burning a card. If none of 4.1-4.8 applies, give the clue that
   maximises the default tiebreak, **ignoring tier**. This floor sits beneath the
   whole of §4, so the branch can never fall through to the play/discard phase and
   leave the engine to burn a blind slot-1 play.

---

## The endgame stall list

**Shipped in v8.9.0** (`reactor0::choose_endgame_clue`,
`src/conventions/reactor0/decision.cpp`). Used when the endgame fork has already
decided the turn is a clue — `prefer_stall_clue` in `src/basics/decide.cpp`
applies it to the forced layer's answer and to the solver's.

Neither of those layers has a model of clue quality. `find_all_clues` ranks with
**reactor's** `get_result` even in a reactor0 game, `clueless_winnable` prices
every clue as a dummy token burn, `possible_actions` may keep only
`all_clues.front()`, and a partner whose call would strike is modelled as free
to ignore it — so a clue that makes him bomb costs the search nothing. The
five-lockout rule is franker still: it asks for `any_legal_clue`.

reactor0 already knew better and was simply never asked. Its candidate pool is
not built until phase 1, far below the fork, so `analyse_clues` never ran on
these turns at all.

The order is **not** the General Clue Evaluation List's. Those rungs are tuned
for a game still being played and put a reactive discard at rung 2, above any
stable play, because keeping the discard engine turning is worth more than one
extra card down. In a forced endgame there is no long run left to feed:

1. A double reactive clue that gets two cards to play (`REACTIVE_PLAY`).
2. A legal stable colour or rank play clue to Bob (`STABLE_PLAY`). "Legal" means
   the card the clue NAMES is the one that is actually playable. Contextual
   eliminations are handled for free, because the reading comes from a full
   simulation of what Bob will know after the clue — so a colour clue whose
   leftmost touched card has been eliminated as unplayable becomes legal exactly
   when it should.
3. Any clue to Bob that singles out a useful card in his hand by empathy —
   `ClueCandidate::newly_useful`: some card of his reads as entirely useful when
   it did not before. **Negative information counts**: a colour clue that strips
   the last trash candidates off a card it did *not* touch qualifies.
4. A valid reactive discard clue to Cathy (`REACTIVE_DISCARD`, affordable).
5. Any other legal stall clue to Bob that cannot be misread as a play clue —
   the `rung_safe_stall` test, narrowed to Bob.
6. Any other legal clue to Cathy.
7. **Floor** — anything left that does not predict a strike.

Every pool goes through the same `select` the ordinary rungs use, so
`predicts_a_strike` vetoes apply at **every** level. Unlike `choose_clue` the
stall list does not consult the tier gate: Alice is already committed to
cluing, so a tier threshold has nothing left to decide. It declines only when
every candidate predicts a strike, in which case the endgame's own answer
stands.

**Replay 1971808 T59** is why it exists. Stacks `[4,5,4,5,5,5]`, 28 of 30, one
card left, and both missing cards visible: r5 in Bob's slot 4, g5 in Cathy's
slot 1. Purple to Cathy is a reactive colour clue — the EVEN parity under Odds
and Evens, so a double play — and the pairing `react_slot + target_slot ≡ anchor
(mod 5)` with Purple's value of 5 gives `4 + 1`. Both lay; 30. The solver
returned red to Bob instead, which names his leftmost touched card that could be
playable — an r1 — and the game ended 29 on the strike.

---

## Decision phase 2 — deciding what to play or discard

**Shipped in v7.1.0** (`reactor0/calls.{h,cpp}`). The four tracking structures
are **derived** from `ConvData` rather than stored: `urgent` already marks the
reacter's call and no other, and `enforce_call_invariants` already keeps a
hand's CTP calls in play order and its CTD calls unique. `calls_of` reads them
back out. See that header for why deriving is exact, and `PLAN.md` §7 for what
this replaced.

* Each player will track the following for *every* player at the table: a
  **reacter-CTP** card, a **receiver-CTP** deque, a **reacter-CTD** card, and a
  **receiver-CTD** card. The default state for these structures is null / empty.
* When a player is called to pitch/chuck a card from the reacter seat from a
  reactive clue, the card occupies the reacter-CTP/CTD slot respectively and is
  immediately popped out when the reacter reacts. In addition, the other reacter
  slot is popped out and pushed onto the front of the corresponding receiver
  deque/card (so reacter-CTP and reacter-CTD can hold at most one card **between**
  them — i.e. there is only one pending reaction being tracked at any given time).
* When a player is signaled to pitch a card as the receiver, the card is pushed to
  the front of the receiver-CTP deque.
* When a player is signaled to chuck a card as the receiver, the card occupies the
  receiver-CTD card slot.
* If the card in reacter-CTP/CTD is the same as the frontmost card in the
  receiver-CTP/CTD deque, the frontmost card of the receiver-side deque is
  removed.
* When a card newer than the top card in the receiver-CTP stack is stamped CTP, it
  is pushed to the front of the deque. A new card stamped CTD replaces any
  existing card in the receiver-CTD slot.
* When a card equal to or older than the front card in the receiver-CTP deque is
  stamped CTP, cards are popped out from the front until either there are no more
  cards or the front card is strictly older than the newly-stamped CTP card.

**Insertion order is convention-specific, and it decides which end is the front.**
In **reactor0 new cards enter the receiver-CTP deque at the FRONT**, so the deque
runs newest slot first and its front is the most recently drawn called card —
which is also the order `enforce_call_invariants` rule 1 maintains. Other
rulesets insert at the **back** (reactor), and there the front is the oldest
called card. The worked example below is written to generalise across both, and
its `[r1, r2]` queue is a *back*-insertion one; under reactor0 that same position
gives `[r2, r1]`. The dependence machinery is identical either way — only which
card heads a chain differs.

We introduce the concept of **dependence**, which only applies to receiver-CTP as
it is the only structure that might contain more than one card. Card A is said to
be dependent on Card B in the receiver-CTP deque if Card B is in front of card A
*and* it is possible based on the (non-global) inferences of both cards that they
could share the same suit.

**The stamp is the instruction.** A player **chucks** a card stamped
`CALLED_TO_DISCARD` — presses the Discard button — and **pitches** a card
stamped `CALLED_TO_PLAY` — presses Play — always, until some later information
makes that button a misplay. A chuck misplays only on an inverted card that is
not next for its stack; a pitch misplays only on a *plain* card that is not
playable. "Makes it a misplay" means every remaining possibility misplays;
anything less and the holder still has a reading under which the call is sound.

**A dead call is dropped.** A call is only as good as the card. Once common
knowledge leaves the stamped button with no identity it handles correctly, every
seat drops the stamp — which also takes the card out of the reacter-CTP and
receiver-CTP structures, since those are derived from it. Judged against the
candidate sets below, not against playability: a CTP on an inverted card is a
*pitch*, and being unplayable is exactly what makes that call sensible.

**"A safe discard" is not "known trash".** In an inverted variant Discard is a
play attempt, so a card its holder knows is a dead Orange 1 is known trash and
still has no safe discard button — chucking it strikes. Only a card every one of
whose readings is trash on a **plain** suit can simply be thrown away. This is
what rungs 3.3 and 4.2 test.

**A call's inferred set is built to match its button**, which is what makes the
rule above safe. The set contains exactly the identities the stamped button
handles correctly:

* **CTP / pitch** (press Play) — every **plain** identity that is currently
  playable, plus every **inverted** identity that is *not* playable and *not*
  critical, i.e. one the team can afford to throw away. The playable inverted
  card is excluded: Play would pitch away the very card its stack is waiting
  for.
* **CTD / chuck** (press Discard) — every **plain** identity that is *not*
  playable and *not* critical, plus every **inverted** identity that *is*
  immediately playable, since Discard is what puts an orange on its stack.

Worked examples, both from live replays. At stacks r1/b1/o1 an untouched card
stamped CTP admits `{r2, b2, o1, o3, o4}`. At stacks r1/m0/o3 a card stamped CTD
admits `{r1, r3, r4, m2, m3, m4, o4}`. These are `pitch_candidates` and
`chuck_candidates` (`reactor0/interpret_clue.h`), applied by
`narrow_to_stamped_button` wherever reactor0 stamps a call.

So a standing call is on its list whatever the card looks like on its own
merits, and `call_is_actionable` (`calls.h`) is the only thing that removes it.
Worked example: an Orange 1 stamped CTD is chucked happily; if the other Orange
1 then plays and the holder learns their card is orange, the chuck would strike
and the call is no longer actionable.

### Actionable card priority

Given all empathy and inferences from Alice's point of view, construct a list of
lists corresponding to pitchable card orders such that each card in a sublist is
dependent on the cards before it in the list. We will call the list consisting of
the first elements of each list the **pitch list**. Take all of the cards
currently stamped CTD, and add all chuckable cards (either trash non-inverted or
playable inverted) to it to form the **chuck list**.

**An INVESTED card needs proof, not an inference.** Chuckability is judged from
`possibilities()`, which prefers `inferred` — a convention deduction that can be
wrong. For an untouched card that costs nothing, since nothing narrowed it. For
a card that is **clued or carries a stamp**, the trash arm additionally requires
**`possible`** to be all trash (`is_chuckable`, `calls.cpp`), because the throw
is irreversible. Replay 1971788 T29: a lock's rank promise had narrowed slot 5
to `{r1,y1,g1,b1,p1}` — all trash — while `possible` still held `d3,d4,d5`, each
a single copy. It was the `d5`; the max score fell 30 to 29 while an unclued
chop sat in slot 1. A `CALLED_TO_DISCARD` card is exempt: it joins the list
through its own arm, and refusing a partner's explicit instruction would break
the signal they spent a clue on.

**"Trash non-inverted" means non-inverted.** A card that is trash but *could* be
on the inverted suit is not chuckable: pressing Discard on an inverted card is a
play attempt, and a trash orange is by definition not playable, so the chuck
strikes. Such a card is pitched. Likewise a chuck only advances a stack when the
card is currently **playable**, which is what rungs 3 and 9 of the Actionable
Card Priority require of a "known inverted suit" (replay 1966569 T10).

**Known same-hand duplicates also join the chuck list**
(`reactor0/calls.cpp:105-142`, `:222`). A player who can pin two of their own
cards to the same identity holds one card too many: discarding either loses
nothing, since the other copy still carries it. Such a card is *not* trash — the
identity is still needed — so "chuckable" above does not reach it, and a hand
made entirely of known dupes would otherwise have an **empty chuck list** and
fall through to the rung-12 floor. Two conditions bound the arm:

- **The leftmost copy only.** Put both on the list and the team can throw the
  identity away entirely. `state.hands` runs newest slot first, so the left copy
  is the first one the walk meets.
- **Plain suits only.** On an inverted suit Discard is a play attempt, so
  chucking a duplicated orange strikes unless it happens to be playable — and
  when it is playable the "playable inverted" arm has already taken it.

Replay 1966687 T14 is the motivating case, and it cost a strike: will-bot67 held
b4 in slots 3 and 5 with blue on 1, found nothing chuckable, and threw its
unknown slot-1 chop instead. That card was an Orange 4.

When a card is pitched from the pitch list, the front of the corresponding list is
popped out, and the pitch list is reconstructed. This may seem overkill but will
eventually be used for variants that rely on chaining of playables or in the
original reactor convention. For example, suppose that only `g1` has been played
on the stacks and Bob's hand consists of the following:

```
r2 r1 p5 g5 g2
```

where `g2` has been fully clued (previously touched with both 2 and green), while
the receiver-CTP queue consists of `[r1, r2]` with `r1` at the front. Then the
list of lists would look like (replacing the card orders with actual identities
here)

```
[ [g2], [r1, r2] ]
```

and the pitch list would be the front of each sublist — `[g2, r1]`.

*(Back-insertion, per the note above. Under reactor0's front insertion the same
hand gives `[ [g2], [r2, r1] ]` and a pitch list of `[g2, r2]`; the test
`Reactor0Calls.DependenceChainsPartitionThePitchList` pins that.)*

If Alice has no sufficiently good clues to give, Alice evaluates the following
list by priority:

1. If Alice has a reacter-CTP or a reacter-CTD card, she must immediately action
   the most recent one. (Per the Precedence section, only a VERY HIGH clue
   outranks this — and the call is no longer urgent at all once its target has
   left the receiver's hand.)
2. Alice pitches a card in the pitch list that is known to connect with a card in
   either Bob's or Cathy's hand.
3. Alice chucks a known inverted suit in the chuck list that is known to connect
   with a card in either Bob's or Cathy's hand.
4. Alice pitches a card in the pitch list corresponding to a sublist with size > 1
   (i.e. has dependencies)
5. Alice pitches a critical 1 (or 5 in a reversed suit)
6. Alice pitches a critical 2 (or 4 in a reversed suit)
7. Alice pitches a critical clue-regain card (5 in a normal suit, 1 in a reversed
   suit)
8. Alice pitches the leftmost card of the lowest stack rank (1 = reversed 5,
   2 = reversed 4, etc.)
9. Alice chucks a known inverted suit in the chuck list.
10. Alice chucks a card in the chuck list that could potentially be an inverted
    suit.
11. Alice chucks the leftmost card in the chuck list.
11b. Alice **pitches** a card every one of whose readings is on an inverted
    suit, not playable, and not critical (`calls.cpp`,
    `11b.pitch_dead_inverted`). Such a card is on neither list — chucking it
    strikes, since Discard on an inverted suit is a play attempt and it is not
    the next for its stack, and it is not playable so `thinks_playables` never
    offers it. Pressing **Play** sends it to the discard pile, which is the only
    safe way to be rid of it.

    Deliberately here rather than on the pitch list: a dead `o1` has
    `direction_rank` 1, so on the pitch list it would win rung 8's "leftmost
    card of the lowest stack rank" against a genuine playable. It is disposal,
    so it ranks with the other disposal rungs. Not critical, because pitching
    is a permanent loss.
12. **Floor — both lists are empty.** Alice **chucks** her chop (`Game::chop`:
    the most-recently-signalled CTD, else the newest unclued status-`NONE`
    card). Discard is the default button, and she deviates from it only when
    chucking would certainly strike — every reading an inverted card that is not
    playable — in which case the chop is a known dead orange and she pitches it,
    which throws it away harmlessly. Note this is NOT
    `discard_button_is_safe` (`decide.cpp`): that predicate FILTERS discard
    candidates, asking "is this one provably safe", and an unknown chop is never
    provably safe. Using it to choose a button pitches into a strike on any
    plain card that is not playable. If Alice is in an endgame
    state where she must give a clue instead, the endgame rules at step 0 of the
    Precedence section have already returned and this step is not reached.
13. If Alice is at 8 clues where she has no known CTPs or CTDs, she should pitch
    her chop card instead (as reaching this point means she also did not have a
    clue to give).

---

## How this maps to code

The machinery *Decision phase 1* reuses rather than reinvents. The list itself
lives in `src/conventions/reactor0/decision.cpp`:

| Piece | Symbol |
|---|---|
| one analysed candidate | `ClueCandidate` |
| the per-turn simulation pass | `analyse_clues` |
| the tier gate (phase 1 items 1 and 2) | `clue_is_admissible` |
| Precedence step 1 | `choose_very_high_clue` |
| Precedence step 3 — the walk | `choose_clue` |
| shape classification | `read_clue` / `outcome_of` |
| priority 2's admissibility | `discard_is_affordable` |
| the §3.2 / §3.4 double discard | `pool_double_discard` |
| §3.2's Cathy-chop gate | `chop_is_expendable` |
| the four phase-2 call structures | `calls_of` (`calls.h`) |
| dependence | `depends_on` |
| the pitch and chuck lists | `action_lists` |
| Actionable Card Priority, rungs 2-13 | `choose_action` |
| rung 1 (action a pending reaction) | `take_action`'s urgent return, above the clue phase |
| the §3.8 / §4.8 tiebreak | `missing_connectors` |

| Rule | Existing machinery | Where |
|---|---|---|
| reactive vs stable | positional compare `action.target != bob` | `interpret_clue.cpp:620-631` |
| two new plays (H3, N3) | `new_play_facts(...).count >= 2` | `state_eval.cpp:173-229` |
| finesse (VH1) | reactive rank Phase B | `interpret_reactive.cpp:383-447` |
| double discard clue | reactive rank Phase C | `interpret_reactive.cpp:448-482` |
| a play REVEAL (stamps nothing; still a play clue) | `playables_result` | `src/basics/clue_result.cpp:177` |
| reactive play / discard clue | reactive rank Phase A; colour modes 1 and 2 | `interpret_reactive.cpp:307-382`; `:503-560`, `:561-626` |
| lock clue | `predicts_reactive_lock` | `interpret_reaction.cpp:31-47` |
| "this clue creates a play" | `hanabi::playables_result` | `src/basics/clue_result.cpp:177` |
| new touches, for the default tiebreak | `elim_result` / `bad_touch_result` | `src/basics/clue_result.cpp` |
| stable-colour target, without simulating | `leftmost_could_be_playable` | `interpret_clue.cpp:211-231` |
| candidate clue enumeration | `State::all_valid_clues` | `src/basics/state.cpp:212-231` |
| colour-only subset | `State::all_colour_clues` | `src/basics/state.cpp:201-210` |
| chop | `Game::chop` | `src/basics/decide.cpp:432-461` |
| safe discard button on inverted suits | `discard_button_is_safe` | `src/basics/decide.cpp:938-958` |
| Bob's safe action (H1a) | `thinks_trash` / `Player::order_trash` | `src/basics/player_game.cpp:115-132` |

## Not yet implemented

Nothing. Every rule above is in the build as of v8.9.0. `TODO.md` carries the
gaps that remain, which are about how the engine executes a decision rather than
about which decision reactor0 makes.
