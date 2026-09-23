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
they did not. Read by everything deciding what a clue MEANS — both stable
ladders, the reactive target walk and §1f's pin all run on `State::shared_view`,
and `State::shared_score` / `shared_pace` / `Game::shared_in_endgame` answer the
same questions of it. Empty, and free, outside TIIAH. CONVENTION.md §1.3.

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

### known play
A card stamped `CALLED_TO_PLAY` whose inference still holds at least one good
playable identity, **or** a card whose global empathy is entirely playable
identities. Read from `common`, so every seat agrees. It is what the
reverse-reactive dispatch keys on. `has_known_play`,
`src/conventions/tiiah/interpret_clue.cpp:24-36`. CONVENTION.md §1c.

### reverse reactive
TIIAH's dispatch, and the reverse of reactor0's: when Bob holds a known play and
Cathy does not, a clue to **Bob** is reactive — with **Cathy** reacting and Bob
receiving — and a clue to **Cathy** is stable. Not the same as reactor's rule,
where a clue to Bob makes Bob both reacter and receiver, a degenerate reading
reactor scores as a MISTAKE. `tiiah::interpret_clue`
(`src/conventions/tiiah/interpret_clue.cpp:180-184`). CONVENTION.md §1c.

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
