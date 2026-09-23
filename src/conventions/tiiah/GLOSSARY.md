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
`src/conventions/tiiah/interpret_clue.cpp:20-32`. CONVENTION.md §1c.

### reverse reactive
TIIAH's dispatch, and the reverse of reactor0's: when Bob holds a known play and
Cathy does not, a clue to **Bob** is reactive — with **Cathy** reacting and Bob
receiving — and a clue to **Cathy** is stable. Not the same as reactor's rule,
where a clue to Bob makes Bob both reacter and receiver, a degenerate reading
reactor scores as a MISTAKE. Specified; not implemented (TODO.md §39).

### double pitch
A reactive clue on which both named cards are played. Every reactive clue in
TIIAH is one of these or a double chuck, because reactive clues are all even
parity. What the two cards ARE is carried by the clue kind: rank means the
receiver's target is one bucket higher than the reacter's, colour one bucket
lower, either wrapping — or the clue is a finesse, or both players know their own
identity exactly. CONVENTION.md §1d.

### double chuck
What a reactive clue becomes when the only playables left are on inverted suits.
Inverted playables and inverted finesses are otherwise skipped as reactive
targets. CONVENTION.md §1d.

### superposition
The state of a player who has played a card without learning what it was: a map
of card order → the identities it could have been. A player may hold several at
once, and everyone tracks everyone's. A superpositioned reacter picks its target
assuming **none** of the superposed cards were played. Specified; not implemented
(TODO.md §41). CONVENTION.md §1e.

### collapsing
Removing a candidate from a superposition — when another player plays that
identity, when another player's clue puts CTP on a playable copy of it, or when
every copy is accounted for between the discard pile and the other hands. At one
remaining candidate the card leaves the map and the believed stacks advance.
CONVENTION.md §1e.
