# Throw It in a Hole Glossary

Terms **specific to TIIAH** or that **mean something different** here.
Everything not listed (chop, lock slot, pitch / chuck, CTP / CTD, critical,
loaded, empathy, sum rule mechanics, …) carries its meaning from the
[reactor glossary](../reactor/GLOSSARY.md), and the stable-clue vocabulary from
the [reactor0 glossary](../reactor0/GLOSSARY.md). How the terms combine into
rules is [CONVENTION.md](CONVENTION.md).

### the hole
Where a played card goes. Nobody sees what it was — the player who played it
included — and nobody is told whether it landed. The discard pile is *not* the
hole: discards stay visible. On an inverted suit the two swap, because a chuck
reaches the stack and a pitch reaches the pile. CONVENTION.md §1.

### believed stacks
`State::play_stacks` under TIIAH. We resolve each hidden action against the card
we watched in a partner's hand (`resolve_hidden_action`,
`src/basics/action.cpp:82-130`), so our stacks are right about everything except
our own plays. `score()`, `strikes` and `State::ended()` are beliefs for the same
reason. A wrong belief is invisible until the game ends.

### common stacks
`State::common_play_stacks`: the stacks as everyone-knows-everyone-knows them.
A play advances them only when its identity was common knowledge at the time,
or when a superposition later collapses on evidence every seat shares. They lag
the *believed stacks*, which also move on the partners' plays we watched and
they did not. `State::shared_score` / `shared_pace` / `Game::shared_in_endgame`
ask the ordinary questions of it, through `State::shared_view`. Empty, and free,
outside TIIAH. CONVENTION.md §1.3.

Since v16.12.0 it is the FLOOR rather than the reading: what a clue means is read
against the *pairwise view* below, and the common stacks are what is left when we
are not one of the two seats a clue is between.

### pairwise view
`State::pairwise_play_stacks[p]`: what we know seat `p` knows — the *common
stacks* plus every hidden play we can name that `p` did not make. A play goes
into the hole, so it is known to every seat except the one who made it; ours are
absent from every row, because `p` watched them but we cannot say what they were.

It is the view a clue is read against, since a clue only has to mean one thing to
the two seats it is between, and holding the whole team to what all three know
lets one seat's ignorance stall the reading. `State::stacks_known_to_both(a, b)`
is the accessor and is symmetric; it answers for the pair only when WE are one of
them, and hands back the common stacks otherwise — an outsider is missing exactly
its own plays and recovers them by the *back-solve*. Like any stack it advances
through the prefix, so a play a row never saw blocks everything above it.
CONVENTION.md §1.3, `tests/test_tiiah/test_pairwise_stacks.cpp`.

### refusal
Bob answering a reactive by giving Cathy a **stable** clue instead of reacting,
which says *the card Alice named is already played*. Alice cannot see it — her
stacks are short by exactly what she threw in the hole — so she collapses the
superposition that admits it (§1e rule 5). Stable is what tells it apart from a
*deferral*, which carries the reactive intent forward and is itself reactive; and
it is an envelope, so the clue still means whatever stable clue it is. Bob's own
call stands. Read by `read_refusal` and given via `clue_refuses_dead_target`,
which joins Precedence step 1 because refusing is done instead of reacting.
CONVENTION.md §1c, v16.14.0, replay 2009367 T6–T7.

### world
One assignment of an identity to each card a seat still has in the hole. A seat
that threw a card without naming it does not know its own stacks, so a call on a
later card of theirs means something different in each world, and the reading is
the **union** over them — with a note of which worlds each candidate needed, so
that settling the earlier card withdraws the rest. `open_worlds` /
`refute_worlds` (`src/conventions/tiiah/superposition.cpp`), stored in
`ConvData::ConditionalReading`. The worlds have a second consumer since v16.16.0:
§1e rule 6 asks which of them lets a partner's play LAND, and refutes the rest
(`presume_play_lands`) — which is how a seat stops inventing strikes out of
stacks that are short by a card it threw in the hole. Capped at 64: beyond that the call is read flat,
because a partial list of worlds is a conditional reading missing some of its own
conditions. CONVENTION.md §1e.

### conditional reading
A reading that holds only in some *worlds* — e.g. `{r1, y1, r2, y2}` where the
`r2` needs the earlier hole card to have been the `r1`. The candidates every
world agrees on are unconditional and are not recorded. Replay 2009367 T4.

### back-solve
Reading our own hidden plays off a clue between two other seats. They called a
card we can SEE, and a call says it is playable, so they hold that suit one below
it; anything their stack has above ours can only be what we threw in the hole.
`back_solve_own_plays`, `src/conventions/tiiah/superposition.cpp:119-183`. Rule 4
of §1e's collapse, and the only one that tells a seat about its OWN past.

### shared view
`State::shared_view()` (`src/basics/state.cpp:143-156`): this state with the
*common stacks* in place of `play_stacks`, and `playable_set` / `trash_set`
rebuilt to match. Since a superposed play never advanced the common stacks, it
is also the state "assuming none of the superposed cards were played" — §1e's
rule and §1.3's shared reading are one thing. Returns the state unchanged
outside TIIAH.

### bucket
One of three groups of suits, used by the reactive clues to say what a card IS
when the stacks cannot show it. Inverted suits are dropped first, the rest are
re-indexed from 0, and the map is keyed on how many remain — 3 → one suit each,
4 → `{[0,1],[2],[3]}`, 5 → `{[0,1],[2,3],[4]}`, 6 → `{[0,1],[2,3],[4,5]}`.
`suit_buckets` / `bucket_of`, `src/conventions/tiiah/buckets.cpp`.
CONVENTION.md §1a.

The **relation** between two buckets — the receiver's target one higher than the
reacter's card for a rank clue, one lower for a colour one — does two jobs, and
since v16.15.0 they are kept apart. It is a **legality** test on the GIVER: a
pairing that breaks it (and is neither a finesse nor a double chuck, and which
the two players could not each name anyway) is a clue Alice may not give. It is
**not** a filter on the walk, because it reads the reacter's own card, which the
reacter cannot see — steering the walk with it made the giver and the reacter
name different slots (replay 2010246 T2). For readers it is purely the
**inference**: what the two called cards are. CONVENTION.md §1d.

### known play
A card stamped `CALLED_TO_PLAY` whose inference still holds at least one good
playable identity, **or** a card whose global empathy is entirely playable
identities. Read from `common`, so every seat agrees. It is what the
dispatch keys on — it is what decides which seat's clue carries the reaction.
`has_known_play`, `src/conventions/variants/hole.cpp:51-67`. CONVENTION.md §1c.

### reverse reactive
The second of TIIAH's two dispatches. When Bob holds a known play and Cathy does
not — the *reverse-reactive position* — a clue to **Bob** is reactive, with
**Cathy** reacting and Bob receiving, and a clue to **Cathy** is stable. Outside
that position reactor0's positional rule stands: a clue to Cathy is the reactive
one and Bob answers it. Not the same as reactor's rule, where a clue to Bob makes
Bob both reacter and receiver, a degenerate reading reactor scores as a MISTAKE.
`reverse_reactive_position` / `reverse_reactive`
(`src/conventions/variants/hole.cpp:69-85`), read by `tiiah::interpret_clue` and
by the decision layer's `dispatch_is_reactive`. CONVENTION.md §1c.

### double pitch
A reactive clue on which both named cards are played — both players press the
**Play** button. Every reactive clue in TIIAH is one of these or a double chuck,
because reactive clues are all even parity. What the two cards ARE is carried by
the clue kind: rank means the receiver's target is one bucket higher than the
reacter's, colour one bucket lower, either wrapping — or the clue is a finesse,
or both players know their own identity exactly. CONVENTION.md §1d.

### double chuck
What a reactive clue becomes when the receiver's only playables are on inverted
suits: both players press **Discard**, which is the button that stacks an
inverted card. Inverted playables and inverted finesses are otherwise skipped as
reactive targets. The reacter's own card has to be **affordable to chuck** —
inverted and playable, so the button plays it, or simply not critical — and a
pairing that fails that is refused outright, since the reacter cannot see their
own hand. `safe_to_chuck`, `src/conventions/tiiah/interpret_reactive.cpp:72-79`.
CONVENTION.md §1d.

### delayed call
A `CALLED_TO_PLAY` card that is not playable yet and will be once the *receiver*
has played what they already know — what a reverse-reactive finesse creates.
Alive but not actionable: reactor0's dead-call invariant judges it against the
stacks after those queued plays, and leaves the call itself out of that
simulation (`src/conventions/reactor0/call_invariants.cpp:144-165`).
CONVENTION.md §1c.

### superposition
The state of a player who has played a card without learning what it was: the
identities it could have been, stamped on `ConvData::superposition` at the moment
of the play and built from `common`, so all three seats hold the same set. A
player may hold several at once, and everyone tracks everyone's. A superpositioned
reacter picks its target assuming **none** of the superposed cards were played.
`note_hidden_action`, `src/conventions/tiiah/superposition.cpp`. CONVENTION.md §1e.

### collapsing
Removing a candidate from a superposition. The first two rules — another player
plays that identity, or a clue puts CTP on a playable copy of it — are **shared**:
both say the identity was still needed, every seat sees them, and a collapse on
either moves the *common stacks* as well. The third — every copy accounted for
between the discard pile and the hands that seat can see — is **private**, so it
moves that seat's believed stacks alone and never the set partners predict from.
At one remaining candidate the card leaves the map and the stack it belongs to
advances. `collapse_superpositions`, `src/conventions/tiiah/superposition.cpp`.
CONVENTION.md §1e.
