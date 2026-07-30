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

### blind play (react slot)
The reacter playing the computed react slot without knowing its identity.
Reactor0 uses it in the rank double play, the finesse, and colour mode 2;
the giver guarantees playability from their own view and observers who can
see the card reject the clue otherwise. `interpret_reactive.cpp:379-382`.

### colour value
The fixed value a colour name contributes as the reactive anchor: Red=1,
Yellow=2, Green=3, Blue=4, Purple=5, Teal=1; Black/Pink/Brown fill in from
{4,3,5,2,1}, Orange from {2,5,4,3,1}. Keyed on the clue colour name, so
Ambiguous variants resolve by what a partner actually says. Collisions are
legal (Teal duplicates Red). `src/conventions/reactor0/colour_value.cpp`.

### direct play clue
A stable clue whose meaning is "play this touched card" — reactor0's
stable colour reading and rank priority 1. Contrast reactor, where a stable
colour clue is *referential* (points one slot left of a touched card).
There is **no referential play in reactor0**.
`src/conventions/reactor0/interpret_clue.cpp:115-156`, `:187-238`.

### double discard (clue)
The rank-reactive fallback when no play or finesse target exists: zero
plays — the reacter discards the react slot and the receiver discards the
dc-target (or locks). `interpret_reactive.cpp:282-309`.

### finesse
As in reactor — the reacter plays a card that connects with a
one-away-from-playable card in the receiver's hand — but reactor0 walks
**targets** leftmost-first (reactor walks react slots in the fixed order
{1,5,4,3,2}). `interpret_reactive.cpp:229-280`.

### play reveal
A stable clue that fills in a previously-clued card as a new obvious
playable. The receiver just plays it; no status is stamped.
`interpret_clue.cpp:59-84`.

### positional dispatch
Reactor0's whole dispatcher: clue to Bob ⇒ stable, clue to anyone else ⇒
reactive with Bob as reacter — regardless of loadedness, stall context, or
pending reactives. `interpret_clue.cpp:292-328`.

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
`ReactorWC::rlocks`. `src/conventions/reactor0/interpret_reaction.cpp:28-49`.

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
`interpret_clue.cpp:240-273`.

### Absent by design
Concepts reactor has that reactor0 deliberately lacks: **reactive focus**
(the anchor replaces it), **referential play** (stable colour is direct),
**response inversion / rewinds** (dispatch is unambiguous), **loadedness
dispatch**, **deferral-carries-reactive**, **re-tasking**, **/allplays**
(mechanically tolerated, not part of the convention).
