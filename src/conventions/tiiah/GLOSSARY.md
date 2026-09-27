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

### gentleman's discard (does not exist here)
The shared engine reads a named playable thrown away as "you hold the other copy"
(reactor0 GLOSSARY). In TIIAH a partner can see the hole, so the same discard means
the card **was already played** — by us, since our stacks can only be short by
what we threw in the hole — and it collapses our superpositions instead
(`presume_discard_was_played`, CONVENTION.md §1e rule 7). `useful_dc` excludes a
playable card in a hole variant (`src/basics/decide.cpp:517-526`). Replay 2011319.

### believed stacks
`State::play_stacks` under TIIAH. We resolve each hidden action against the card
we watched in a partner's hand (`resolve_hidden_action`,
`src/basics/action.cpp:82-130`), so our stacks are right about everything except
our own plays. `score()`, `strikes` and `State::ended()` are beliefs for the same
reason. A wrong belief is invisible until the game ends.

### common stacks
`State::common_play_stacks`: the stacks as everyone-knows-everyone-knows them.
A play advances them only when its identity was common knowledge at the time,
or when a superposition later collapses on evidence every seat shares. A play
the team could name that lands ABOVE them raises them to at least that card and
settles the hole cards that must sit under it (§1e rule 6's shared-view form,
`known_play_lands_in_common`, v16.23.0). They lag
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
its own plays and recovers them by the *back-solve*.

**A row is a floor (v16.23.0).** A play both seats of the pair can name — the one
neither of them made, or one its player could name — raises the row to AT LEAST
that card, whatever lower card neither of them can name: they watched it land
(never presume a strike), and a stack only goes up. What they name is the card
they WATCHED, not our copy of the player's reading. A row is never below the
*common stacks*. Before v16.23.0 a row advanced through the prefix, one card at a
time, and a play that arrived above a card the row had not yet named was dropped
for good. `State::with_pairwise_at_least`, `tiiah::note_hidden_action`; replay
2011327. CONVENTION.md §1.3.

Since v16.18.0 a row also takes what the seat behind it can WORK OUT about its own
plays: across the *worlds* those plays leave open, the *surviving* ones agree on a
height, and that height is the seat's (`advance_rows_from_own_worlds`). The
enumeration spans that seat's hole cards and ours together, which is both what
makes the answer the pair's and what stops it exceeding what the seat believes.
CONVENTION.md §1.3, `tests/test_tiiah/test_pairwise_stacks.cpp`,
`test_row_from_own_worlds.cpp`.

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

### fix clue
A stable clue whose net information — positive touch or negative — narrows a partner's
standing *call* to exactly one identity that the *common stacks* say is dead. It means
"that call is a duplicate": the holder withdraws it and treats the card as known trash.

Two halves, and where each is checked is the design. That the identity is dead is
COMMON knowledge, so every seat reads the clue alike; that THIS card is that identity
is the giver's sight, and the clue is what transfers it. `dead_call_fix`
(`src/basics/fix.cpp`) is the shared half; `clue_fixes_dead_call`
(`reactor0/decision.cpp`) adds the giver's.

It **supersedes** the clue's ordinary stable meaning, where a *refusal* is an envelope
and rides along with it. Priority: Precedence step 1, between rung 2 and rung 3
(`rung_2b`, logged `2b.fix`), with an exemption from the tier gate the refusal does not
have. CONVENTION.md §1h, v16.20.0, replay 2010512.

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
stacks that are short by a card it threw in the hole. **That collapse is SHARED as of
v16.21.0**: every seat watched the card, holds its candidate set and applies the same
rule, so the *common stacks* and every *pairwise view* move with our belief. Rule 6
asked of our OWN plays stays private, because it is judged against our own belief.
And since v16.17.0 the RECEIVER's half of §1d reads them too, not just the reacter's. Capped at 64: beyond that the call is read flat,
because a partial list of worlds is a conditional reading missing some of its own
conditions. CONVENTION.md §1e.

### surviving world
A world in which none of the plays it assigns STRUCK — `OpenWorld::struck` is set
when an assignment is not playable on the stacks the world has reached. §1e rule 6
refutes a striking world whenever a strike-free one is available, and stands them
all up again when every world has a strike in it: then the strike is not an
assumption anybody made. `strike_free`
(`src/conventions/tiiah/superposition.cpp:561-571`), read by
`presume_own_plays_land` for our own belief and by `advance_rows_from_own_worlds`
for a partner's row. v16.18.0.

What the survivors agree on is the **floor**: the least advanced height any of them
reaches, per suit (`world_floor`, `:542-557`). It raises a partner's row (v16.18.0)
and, since v16.19.0, our own `play_stacks` — two cards each reading `{g1,b1}` were one
of each, so both suits are on 1 even though neither card can be named. The copies stay
unbooked, since which card was which is exactly what is unknown.

### frame
The stacks a reactive's target is walked in: the **minimum**, suit by suit, across
every world the reacter can live in, from the giver's perspective (v16.24.0). For the
pair it is their row; for an outside seat, the shared view floored over every seat's
worlds. `reacter_frame`, `src/conventions/tiiah/interpret_reactive.cpp:171-183`.
CONVENTION.md §1e. It replaced "assume none of the superposed cards were played".

### feasible world
A *world* the targeting rules allow (v16.24.0). Every reactive play clue that
resolves is kept as a `ReactionRecord`; a world in which two or more of the
receiver's cards from that clue sit, and one of them would have out-ranked the card
the reacter called in the target walk (a direct playable before a finesse, each
leftmost first), is not the world we are in. `world_feasible`,
`src/conventions/tiiah/superposition.cpp:609-655`; `open_worlds` drops such worlds
and `prune_infeasible_worlds` settles on the rest, shared. Replay 2011397 T6.

### evidence / band
Every view keeps two vectors (v16.24.0): the view itself, which floors and a card
named above a gap raise to AT LEAST a height, and its **evidence**, which only
advances by the next card it can name (`State::pairwise_evidence`,
`common_evidence`, `play_evidence`). The gap between them is the **band**: cards the
view counts as down without knowing which card each was. A world replayed on a view
absorbs a card whose rank falls in the band — once per rank — instead of striking it
(`State::with_band`, `enumerate_worlds`). Without it a floor replayed on itself
struck the very world that produced it (replay 2011327 T36).

### conditional reading
A reading that holds only in some *worlds* — e.g. `{r1, y1, r2, y2}` where the
`r2` needs the earlier hole card to have been the `r1`. The candidates every
world agrees on are unconditional and are not recorded. Replay 2009367 T4.

### back-solve
Reading our own hidden plays off a clue between two other seats. They called a
card we can SEE, and a call says it is playable, so they hold that suit one below
it; anything their stack has above ours can only be what we threw in the hole.
`back_solve_own_plays`, `src/conventions/tiiah/superposition.cpp:347-401`. Rule 4
of §1e's collapse, and the only one that tells a seat about its OWN past.

### shared view
`State::shared_view()` (`src/basics/state.cpp:279-297`): this state with the
*common stacks* in place of `play_stacks`, and `playable_set` / `trash_set`
rebuilt to match. As of v16.24.0 the common stacks are the MINIMUM across every
seat's worlds (`advance_common_from_worlds`, §1e), not "assuming none of the
superposed cards were played". Returns the state unchanged outside TIIAH.

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
`has_known_play`, `src/conventions/variants/hole.cpp:57-73`. CONVENTION.md §1c.

### reverse reactive
The second of TIIAH's two dispatches. When Bob holds a known play and Cathy does
not — the *reverse-reactive position* — a clue to **Bob** is reactive, with
**Cathy** reacting and Bob receiving, and a clue to **Cathy** is stable. Outside
that position reactor0's positional rule stands: a clue to Cathy is the reactive
one and Bob answers it. Not the same as reactor's rule, where a clue to Bob makes
Bob both reacter and receiver, a degenerate reading reactor scores as a MISTAKE.
`reverse_reactive_position` / `reverse_reactive`
(`src/conventions/variants/hole.cpp:75-91`), read by `tiiah::interpret_clue` and
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
own hand. `safe_to_chuck`, `src/conventions/tiiah/interpret_reactive.cpp:94-100`.
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
player may hold several at once, and everyone tracks everyone's. A reactive's
target is walked on the *frame*: the minimum across every world the reacter can
live in (v16.24.0; it was "assuming none of the superposed cards were played").
`ConvData::hole_turn` records when the card went in, the order its worlds are
replayed in.
`note_hidden_action`, `src/conventions/tiiah/superposition.cpp`. CONVENTION.md §1e.

A **reactive blind play is often not a superposition at all**: §1d's bucket relation
names the reacter's card, and where the bucket holds one playable that is a single
identity, so `note_hidden_action` advances the *common stacks* instead of stamping
anything. The receiver used to be the exception — it returns before the target walk,
so its copy was never narrowed and it kept the whole pre-clue empathy — until
v16.19.0 gave it `narrow_reacter_play` at reaction time. "All three seats hold the
same set" is the intent; keeping it true takes work at each of them.

### collapsing
Removing a candidate from a superposition. The first two rules — another player
plays that identity, or a clue puts CTP on a playable copy of it — are **shared**:
both say the identity was still needed, every seat sees them, and a collapse on
either moves the *common stacks* as well. The third — every copy accounted for
between the discard pile and the hands that seat can see — is **private**, so it
moves that seat's believed stacks alone and never the set partners predict from.
Since v16.18.0 the third also has a **pair form**: count only the copies outside
BOTH our hand and one partner's, and the partner reaches the same answer, so that
partner's *pairwise view* may take it (`all_copies_visible_to_pair`). It is the
private form with one more hand struck out, hence strictly the stronger test — a
row can never learn what our own belief has not.
At one remaining candidate the card leaves the map and the stack it belongs to
advances. `collapse_superpositions`, `src/conventions/tiiah/superposition.cpp`.
CONVENTION.md §1e.
