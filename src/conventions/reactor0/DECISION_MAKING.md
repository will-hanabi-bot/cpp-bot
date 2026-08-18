# Reactor0 decision making

> **STATUS — phase 1 implemented in v7.0.0; phase 2 is specification only.**
> This document is the ruling reference for **how reactor0 decides what to do on
> its turn**. `CONVENTION.md` remains the ruling reference for what a clue
> *means*.
>
> *Clue Tier Definitions* and *Decision phase 1* describe the build. *Decision
> phase 2*, and rungs §4.5 and §4.6, are **not yet implemented** — see
> *Not yet implemented* at the foot of this document and
> [`PLAN.md`](../../../PLAN.md). Until phase 2 lands, reactor0 chooses what to
> play or discard with the shared ladder in `src/basics/decide.cpp`.

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

A clue tier (`clue_tier`, `state_eval.cpp:405-447`) is HIGH iff **any** of:

1. **H1** — ALL of H1a, H1b, and H1c.
    - **H1a — Bob's chop is endangered.** Bob is not locked, and has no safe
      action (no obvious play, no known trash, no CTD — all three are covered by
      `thinks_trash`, `player_game.cpp:115-132`), and his chop is *endangered*
      (below). `:417-421`.
    - **H1b** — Cathy's chop is not playable or critical, **judged from Alice's
      full visibility** (the same viewpoint as *endangered chop* below, not
      common knowledge). If Cathy has no chop, this condition is vacuously true.
    - **H1c** — Cathy's chop is either a trash or a same-hand-dupe, *or* Bob does
      not have a colour stable clue to give to Cathy.
2. **H2** — the clue gets a **critical 1 or 2** played (5 or 4 on a reversed suit,
   via `variants::is_first_or_second_rank`). `:423`.
3. **H3** — the clue gets **two new plays**, at least one at the clue-regain rank
   (5 normally, 1 reversed, `variants::is_clue_regain_rank`). `:425`.
4. **H4** — Cathy's chop is not trash or a same-hand-dupe, and the clue **gets a
   finesse**.

NOT-LOW iff any of H1a, H2, H3, H4, or:

5. **N5 — Bob's chop is playable** and is not duplicated in his own hand
   (`has_playable_chop`, `:151-159`; applied at `:428-432`). Like H1 this is a
   property of the **position, not of the candidate clue**, so it lifts every
   clue that turn to at least MEDIUM. Deliberately weaker than `at_risk_chop`: it
   asks only "playable, and Bob cannot just pitch a spare copy", and does *not*
   care whether a copy sits in Cathy's hand or is provable in Alice's — the point
   is not that the card is in danger but that it is a play the team should be
   collecting, and Cathy is already expecting Alice to save it or get it played.

…or, when **Cathy's** chop is endangered (`:434-445`):

6. **N3** — the clue gets two new plays. `:436-437`.
7. **N2** — the clue is **reactive** and Bob has no colour stable play clue he
   could give Cathy. Reactive is a single integer compare, `action.target != bob`,
   since dispatch is positional (§1a, `interpret_clue.cpp:620-631`). `:438-444`.

MEDIUM is NOT-LOW and not HIGH; LOW is everything else. "New plays" are counted as
CTP-status transitions between the real game and the clue's hypo
(`new_play_facts`, `:170-187`) — the same walk reactor's `is_high_value_clue`
uses.

**"Gets a finesse"** (H4) means the clue's interpretation is reactive rank
**Phase B** — the blind-play phase that walks one-away targets and calls the
reacter onto the prerequisite (`interpret_reactive.cpp:383-447`). Phase A (double
play, `:307-382`) and Phase C (double discard, `:448-482`) are not finesses.

**Endangered chop** (`at_risk_chop`, `:121-143`), judged from Alice's full
visibility. All of the following must hold:

1. the identity is known to Alice and not basic trash;
2. there is **no second copy in the holder's own hand**;
3. no copy sits in the third player's hand, and Alice cannot prove she holds a
   copy herself.

The first of those is stricter than reactor's `chop_is_nontrash`
(`reactor/state_eval.cpp:45-50`), which tests only `is_basic_trash` — a chop the
holder can safely pitch because they hold the other copy is not in danger.

**"Alice provably holds a copy"** (`alice_provably_holds`, `:60-101`) extends
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

**Bob's colour play clue for Cathy** (`has_colour_play_clue_for`, `:194-212`) is a
structural check, not a simulation: for each colour clue Bob could give Cathy it
replays `stable_colour`'s target choice (§1b step 5,
`interpret_clue.cpp:327`) plus its three guards (`:340-345`), then asks whether
the named card actually plays. Two known approximations, both deliberate: it skips
the FIX branch above the direct-play read (`interpret_clue.cpp:245-248`), and it
does not model a play reveal (`:256-261`). It is evaluated **last** in `clue_tier`,
behind the O(1) reactive test, because it is the only costly term.

---

## Framework

The priority of evaluation on a given player (Alice)'s turn is:

1. What clue should Alice give (if any)?
2. If Alice cannot or should not give a clue, what card should Alice play or
   discard?

### Precedence

Two things outrank the phases below, and one thing sits between them:

```
0.  Endgame.  The forced-endgame rules and the exact solver run first and are
    convention-neutral (`decide.cpp:739-758`).  They are unchanged by this spec.

1.  An H4 clue, if one is available.

2.  A pending REACTION.  If Alice holds a reacter-CTP (or, in a variant with an
    inverted suit, a reacter-CTD), she actions it.  Only H4 clues outrank this.
      - no inverted suit in the variant: reacter-CTP is the urgent one;
      - inverted suit present:  reacter-CTP and reacter-CTD are equally urgent.

3.  Decision phase 1 — giving a clue.

4.  Decision phase 2 — deciding what to play or discard.
```

Step 2 is **reacter-only**. A pending reaction is urgent because the receiver is
decoding against it, so it interrupts everything below it and only an H4 clue
outranks it.

A **receiver**-side call carries no such urgency. It makes Alice *occupied*, which
is what phase 1 rule 1a keys on — so Alice may give **any HIGH-tier clue** while
holding one, not only an H4. The call itself is actioned in phase 2.

---

## Decision phase 1 — giving a clue

We'll keep the current clue tier evaluation thresholds from the reactor decision
making framework.

Alice is **occupied** when she holds ≥1 card stamped `CALLED_TO_PLAY`, *or* — in a
variant containing an inverted suit — ≥1 stamped `CALLED_TO_DISCARD` that she
knows is **not** the next card on that inverted suit's stack. This knowledge need
not be global: it is Alice's own inference that counts. Note *occupied* is not the
same as *loaded*.

Concretely, a `CALLED_TO_DISCARD` card counts toward *occupied* when **no**
identity it could still be is both on an inverted suit and currently playable —
i.e. actioning it cannot advance a stack, so it is a discard rather than a
deferred play. This is the predicate
`variants::possible_chuck_advances_stack` (`variants/inverted.cpp:96-102`), negated.

Whether a clue should be given is then:

1. **Alice is occupied.**
    - 1a. If `pace() >= 3 && clue_tokens < 8`, Alice gives the best clue from the
      General Clue Evaluation List satisfying the **HIGH** tier requirements.
    - 1b. Otherwise, Alice gives the best clue from the General Clue Evaluation
      List below.
2. **Alice is not occupied.**
    - 2a. If `pace() >= 3 && clue_tokens < 4`, Alice gives the best clue from the
      General Clue Evaluation List satisfying the **HIGH** or **MEDIUM** tier
      requirements.
    - 2b. Otherwise, Alice gives the best clue from the General Clue Evaluation
      List below.

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

2. **Alice has a reactive discard clue available**, where the discarded card is
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

   This admissibility condition is what makes a separate "pointless double
   discard" filter unnecessary: a double discard whose discarded card is none of
   trash / same-hand-dupe / visibly duped is simply never proposed.

3. **Bob has a non-trash card on chop** (so in particular is not locked) **and no
   safe play or discard.** Every condition marked `>= N clues**` below means the
   condition also applies at `>= N-1` clues if Cathy has a safe discard, or if
   Cathy has on chop a trash, a same-hand-dupe, or a good card that Alice sees at
   least one dupe of in any other player's hand (including her own). Tiebreak by
   the following:
    1. Give a stable play clue to Bob if there are `>= 2 clues**`
    2. Give a stable discard clue or trash reveal clue to Bob that stamps CTD on a
       trash card or same-hand-dupe, or a CTP to a trash card in an inverted suit
    3. Give a stable discard clue to Bob that stamps CTD on a card for which a
       dupe exists in Cathy's hand (or Alice's hand, if known by Alice), or a CTP
       to a card in an inverted suit whose dupe is seen by Alice.
    4. Give a lock clue to Bob if all of Bob's cards are critical and there are
       `>= 2 clues**`
    5. Compute the quantity `L = (# of 1-away-from-playable cards) + 2 * (# of
       trash cards)` in Bob's hand. If `L >= 3` and there are `>= 3 clues**`, then
       give a lock clue to Bob.
    6. Give a reactive discard that stamps CTD on a non-critical card in Bob's
       hand, tiebreak by the largest number of **missing connectors** Alice can
       see leading up to that card, or a reactive play that stamps CTP on a
       non-critical inverted card in Bob's hand, tiebreak by the same criteria.
       **This rung is unconditional** — it carries no clue-count condition, and
       the `**` relaxation does not reach it.
    7. Give a lock clue to Bob if there are `>= 2 clues**`

   **Missing connectors** of a card `X` = the number of identities strictly
   between the top of `X`'s stack and `X` that are **not** visible to Alice in any
   hand. Worked example, no cards on the stacks, Alice sees Bob as
   `r2 g3 r4 g4 b4` and Cathy as `r3 r3 b5 g5 p5`: `b4` needs `b1 b2 b3`, none
   visible → 3; `g4` needs `g1 g2 g3`, and `g3` is in Bob's hand → 2; `r4` needs
   `r1 r2 r3`, and `r2` is in Bob's hand and `r3` in Cathy's → 1. So `b4` wins.

4. **Alice is at 8 clues and is forced to clue or pitch.** Tiebreak by the following:
    1. Same as 3.1
    2. Same as 3.2
    3. Same as 3.3
    4. Same as 3.7
    5. *(**NOT YET IMPLEMENTED** — v7.1.0.)* Give a fill-in clue (which is a
       stable clue that narrows down the identity of existing unplayable clued
       cards in Bob's hand). Prioritize cards that are duplicated in either
       Cathy's hand or Alice's own hand, followed by cards ranked by lowest
       number of connectors and lowest stack rank.
    6. *(**NOT YET IMPLEMENTED** — v7.1.0.)* Give any other stall clue that
       cannot be misinterpreted by Bob as some other type of stable clue that
       would cause a strike or a discard of a critical card.
    7. If at < 2 strikes, give a stable clue to Bob that will cause him to pitch a trash/duped
       non-inverted suit or chuck a trash/duped inverted suit (explicitly allowing a strike here).
    8. Give a reactive discard/double discard clue that stamps CTD on a non-critical card
       in Bob's hand, tiebreak by the largest number of **missing connectors** Alice can
       see leading up to that card, or a reactive play/reactive discard that stamps CTP on a
       non-critical inverted card in Bob's hand, tiebreak by the same criteria.

   **§4 always returns a clue.** At 8 clue tokens a discard is illegal, so
   something must be given. If none of 4.1-4.8 applies, give the clue that
   maximises the default tiebreak, **ignoring tier**. This floor sits beneath the
   whole of §4, so the branch can never fall through to the play/discard phase and
   leave the engine to burn a blind slot-1 play.

---

## Decision phase 2 — deciding what to play or discard

**Not yet implemented — v7.1.0.** The tracking structures below do not exist in
the code today; see [`PLAN.md` §7](../../../PLAN.md) for what has to be built.

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

We introduce the concept of **dependence**, which only applies to receiver-CTP as
it is the only structure that might contain more than one card. Card A is said to
be dependent on Card B in the receiver-CTP deque if Card B is in front of card A
*and* it is possible based on the (non-global) inferences of both cards that they
could share the same suit.

### Actionable card priority

Given all empathy and inferences from Alice's point of view, construct a list of
lists corresponding to pitchable card orders such that each card in a sublist is
dependent on the cards before it in the list. We will call the list consisting of
the first elements of each list the **pitch list**. Take all of the cards
currently stamped CTD, and add all chuckable cards (either trash non-inverted or
playable inverted) to it to form the **chuck list**.

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

If Alice has no sufficiently good clues to give, Alice evaluates the following
list by priority:

1. If Alice has a reacter-CTP or a reacter-CTD card, she must immediately action
   the most recent one. (Per the Precedence section, only an H4 clue outranks
   this.)
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
12. **Floor — both lists are empty.** Alice discards her chop (`Game::chop`,
    `decide.cpp:432-461`: the most-recently-signalled CTD, else the newest unclued
    status-`NONE` card), pressing whichever button is safe for it
    (`discard_button_is_safe`, `decide.cpp:938-958`). If Alice is in an endgame
    state where she must give a clue instead, the endgame rules at step 0 of the
    Precedence section have already returned and this step is not reached.
13. If Alice is at 8 clues where she has no known CTPs or CTDs, she should pitch
    her chop card instead (as reaching this point means she also did not have a
    clue to give).

---

## How this maps to code

Nothing in this section is implemented yet; it records the machinery the
implementation is expected to reuse rather than reinvent.

| Rule | Existing machinery | Where |
|---|---|---|
| reactive vs stable | positional compare `action.target != bob` | `interpret_clue.cpp:620-631` |
| two new plays (H3, N3) | `new_play_facts(...).count >= 2` | `state_eval.cpp:170-187` |
| finesse (H4) | reactive rank Phase B | `interpret_reactive.cpp:383-447` |
| double discard clue | reactive rank Phase C | `interpret_reactive.cpp:448-482` |
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

Everything above is the ruling convention. These parts of it are **not yet in the
build**, and the sections that describe them say so inline:

| Rule | Status | What happens instead today |
|---|---|---|
| §4.5 fill-in clue | v7.1.0 | falls through to 4.7 / 4.8 / the floor |
| §4.6 safe stall clue | v7.1.0 | falls through to 4.7 / 4.8 / the floor |
| Decision phase 2 (all of it, incl. the tracking structures) | v7.1.0 | the shared `take_action` ladder in `src/basics/decide.cpp` chooses the play or discard |
| Phase 2 item 12 — floor, discard the chop | v7.1.0 | the shared ladder's own chop rule (`Game::chop`, `decide.cpp:432-461`) |
| Phase 2 item 13 — at 8 clues, pitch the chop | v7.1.0 | the shared ladder plays **slot 1**, not the chop (`decide.cpp:1127-1130`) |

§4.5 and §4.6 are deferred because they are the only rungs needing genuinely new
machinery — a fill-in detector, and a simulation of Bob's reading judged safe
against a strike or a critical discard — and they fire only at 8 clue tokens
after 4.1-4.4 have all failed. The §4 floor guarantees the branch still returns a
legal clue without them.
