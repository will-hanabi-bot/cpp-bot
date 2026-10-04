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
playable card in a hole variant (`src/basics/decide.cpp:545-554`). Replay 2011319.

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
are not one of the two seats a clue is between. The one exception is the
*global play reveal* below (v18.20.0).

### global play reveal
A colour play reveal whose card is also revealed as playable on the *common
stacks*, not just the *pairwise view* the clue is read on. Only a global one
outranks the leftmost newly touched card; otherwise the colour clue is read as a
direct play of that card. The receiver cannot know the giver knows the stack is
high enough. `reveal_frame` in `reactor0::stable_colour`, which TIIAH passes
`common_play_stacks`. CONVENTION.md §1b, replay 2015013 T35 and T37.

### pinkish re-touch (global frame)
reactor0's pink tempo, pink trash and pink identity clues (reactor0 GLOSSARY),
read here on the *common stacks*: the tempo clue's next playable pink and the
trash clue's "rank down in every pinkish suit" are judged on what every seat
knows. `pink_frame` in `reactor0::stable_rank`. CONVENTION.md §1b, replay
2015070 T18–T19 (v19.0.0).

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
superposition that admits it (§1e rule 5), or, when several do, records that one of
them was it and floors every view (v16.27.0). Stable is what tells it apart from a
*deferral*, which carries the reactive intent forward and is itself reactive; and
it is an envelope, so the clue still means whatever stable clue it is. Bob's own
call stands. Read by `read_refusal` and given via `clue_refuses_dead_target`,
which joins Precedence step 1 because refusing is done instead of reacting, is
exempt from the tier gate, and prefers a stable play clue that names its card
(`refusal.stable_play`, v16.27.0). CONVENTION.md §1c, v16.14.0, replays 2009367 T6–T7
and 2011854 T27.

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
(`rung_2b`, logged `2b.fix`), exempt from the tier gate (as the refusal is, since
v16.27.0). CONVENTION.md §1h, v16.20.0, replay 2010512.

### dupe strike
A partner's strike on a card that was already down, which every seat can tell was a
duplicate: the watchers saw it, and the copy that is down went into the hole in front
of the striker. Common knowledge, so every view is floored at it and a `HoleRequirement`
records which hole cards could have been the copy — §1e rule 8's shared form,
`strike_was_a_watched_dupe`, v16.27.0, replay 2011854 T28. The striker itself does not
yet take it (TODO.md 58).

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
(`src/conventions/tiiah/superposition.cpp:704-714`), read by
`presume_own_plays_land` for our own belief and by `advance_rows_from_own_worlds`
for a partner's row. v16.18.0.

What the survivors agree on is the **floor**: the least advanced height any of them
reaches, per suit (`world_floor`, `:655-670`). It raises a partner's row (v16.18.0)
and, since v16.19.0, our own `play_stacks` — two cards each reading `{g1,b1}` were one
of each, so both suits are on 1 even though neither card can be named. The copies stay
unbooked, since which card was which is exactly what is unknown.

### frame
The stacks a reactive's target is walked in: the **minimum**, suit by suit, across
every world the reacter can live in, from the giver's perspective (v16.24.0). For the
pair it is their row; for an outside seat, the shared view floored over every seat's
worlds. `reacter_frame`, `src/conventions/tiiah/interpret_reactive.cpp:176-190`.
CONVENTION.md §1e. It replaced "assume none of the superposed cards were played".

### feasible world
A *world* the targeting rules allow (v16.24.0). Every reactive play clue that
resolves is kept as a `ReactionRecord`; a world in which two or more of the
receiver's cards from that clue sit, and one of them would have out-ranked the card
the reacter called in the target walk (a direct playable before a finesse, each
leftmost first), is not the world we are in. `world_feasible`,
`src/conventions/tiiah/superposition.cpp:752-814`; `open_worlds` drops such worlds
and `prune_infeasible_worlds` settles on the rest, shared. Replay 2011397 T6.

### evidence / band
Every view keeps two vectors (v16.24.0): the view itself, which floors and a card
named above a gap raise to AT LEAST a height, and its **evidence**, which only
advances by the next card it can name (`State::pairwise_evidence`,
`common_evidence`, `play_evidence`). The gap between them is the **band**: cards the
view counts as down without knowing which card each was. A world replayed on a view
absorbs a card whose rank falls in the band — once per rank — instead of striking it
(`State::with_band`, `enumerate_worlds`). Without it a floor replayed on itself
struck the very world that produced it (replay 2011327 T36). The band never absorbs an
identity the team has *named in the hole* (v16.28.0): that card is a duplicate.

### named in the hole
A hole card the WHOLE TEAM can name — it was known when it was played, or a shared
argument settled it since — recorded as `ConvData::named_in_hole` (v16.28.0). Never
set by a private settle. World replay reads the set of them so that a view's band
never absorbs a second card of a named identity (§1e; replay 2011885 T17).

### row replay
How a pairwise row replays hole-card worlds (`open_worlds(..., shared=true, row=true)`,
v18.1.0). Both seats' hole cards are replayed with the sets both seats hold, so a
card we settled privately goes in with its `shared_left`. The exception is a card
whose *private name* the row already counts: that card is left out, since the row
has counted it already. Before v18.1.0 our private settles were left out altogether
(v16.28.0), and the two copies of a row could reach different heights (§1e; replay
2013616 T20).

### private name
The identity WE name a hole card as privately (v18.1.0), recorded as
`ConvData::private_named` beside `shared_left`. It is written by
`settle(..., shared=false)`, and by `note_hidden_action` for a reaction's card that the
giver and the reacter name exactly while the team holds a wider set (the *team
reading*). A shared argument that names the card clears it. Only the *row replay*
reads it: a card whose private name the row already counts is left out.

### passback
The playable-dupe passback (§1j, v16.25.0): a seat that can name its card as X, and
sees another seat's called card that really is X while that seat reads it wider,
discards its own copy instead of playing it — only when the other holder would still
play theirs once ours landed. The other holder reads the throw as "yours is X".
`dupe_passback` / `read_passback`, `src/conventions/tiiah/dupes.cpp`. Replay 2011475 T20.

### discharge
The playable-dupe discharge (§1k, v16.25.0): a reacter whose reaction card is X,
already played by the giver into the hole without knowing it, discards it instead of
playing; the receiver still plays when X is a candidate of one of the giver's hole
cards, and that hole card settles to X. `discharge_hole_card` / `find_discharge` /
`discharge_instead`, `src/conventions/tiiah/dupes.cpp`. Replay 2011475 T7 (the example).

### shared world set
The worlds of the SHARED view (v16.25.0): every seat's hole cards with the candidate
set every other seat still allows. A seat that settled or narrowed one of its own
privately keeps the shared set in `ConvData::shared_left`, and the shared-view callers
enumerate with it (`open_worlds(..., shared=true)`), so the shared view never strikes a
world for want of a card only one seat can name.

### hole requirement
A joint fact about our own hole cards (v16.25.0): "one of these WAS X", recorded by rule
7 (a named playable thrown away) and rule 8 (a playable card that struck), when no
single card's candidate set can carry it. `HoleRequirement`, `Game::hole_requirements`;
`world_feasible` drops any world that breaks one. Replay 2011475 T43.

### conditional reading
A reading that holds only in some *worlds* — e.g. `{r1, y1, r2, y2}` where the
`r2` needs the earlier hole card to have been the `r1`. The candidates every
world agrees on are unconditional and are not recorded. Replay 2009367 T4.

### back-solve
Reading our own hidden plays off a clue between two other seats. They called a
card we can SEE, and a call says it is playable, so they hold that suit one below
it; anything their stack has above ours can only be what we threw in the hole.
`back_solve_own_plays`, `src/conventions/tiiah/superposition.cpp:426-481`. Rule 4
of §1e's collapse, and the only one that tells a seat about its OWN past. What it
learns is private: it moves our belief and no pairwise row (v17.3.0).

### team reading
What the WHOLE TEAM can name a reacter's blind play as: what the receiver will be able
to reconstruct at reaction time (a proven finesse's connector, or the reacter's bucket
on the shared frame), within the card's reading before the clue. The giver and the
reacter know the card exactly; unless the team reading is that one card, they keep it
privately and the shared view carries the team's set (v17.2.0).
`reaction_team_reading`, `src/conventions/tiiah/interpret_reactive.cpp:1269-1315`.
CONVENTION.md §1d.

### proven finesse
A reactive pairing the RECEIVER can show was a finesse without seeing its own card. Its
called card could be the card after the one the reacter played, and could not be any
card of the bucket half in any of its worlds. The reacter's card is then named, for the
whole team, as the connector it was (v17.1.0). `proven_finesse`,
`src/conventions/tiiah/interpret_reactive.cpp:960-995`. CONVENTION.md §1d.

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
A card whose clue-touch empathy (`possible` in `common`) allows only identities
playable on the **shared view** (v18.0.0). Inferences and a seat's own stacks do
not count, because every seat must reach the same answer. Not enough for a
*standing play* since v20.3.0: that needs a *sure play*.
`has_known_play`, `src/conventions/variants/hole.cpp:61-81`. CONVENTION.md §1c.

### sure play
A known play none of whose identities could already be in the hole: no unsettled
hole card's team set holds an identity of that suit at or beyond it (v20.3.0, the
user's ruling). There is no good touch on an ancillary touched card, and the shared
view is the minimum across the hole's worlds, so a touched-but-uncalled card may
be a dupe. It need not be called, nor a singleton: a yellow card filled in as a 1
is one. `possibly_in_the_hole` / `is_standing_play`,
`src/conventions/variants/hole.cpp:101-146`. CONVENTION.md §1c, replay 2018428.

### standing play
A *sure play*, or a card called to play, whatever its inference and touches, whose
call every seat stamps alike: a **clued** call (v18.3.0), which a stable colour clue
stamps by its nature, or a **settled** one, which is no longer urgent (v18.10.0; a
receiver's call once its reaction is played). A pending reaction call is left out.
It decides the *reverse-reactive position* (v18.3.0).
`has_standing_play`, `src/conventions/variants/hole.cpp:83-99`. CONVENTION.md §1c.

### ASCR (Actionable Superposition Collapse Rule)
The user's broad rule (v20.6.0; human diagnostic 2018541): a player who seems to be
actioned to play an unplayable card -- as reacter or receiver, and not only after a
clue has resolved -- first checks whether some world of the hole cards makes it
playable, and if so collapses the worlds to those in which it works; a receiver tries
the bucket + finesse suits first. Only then does it move on to the next
interpretation. One shared routine, `ascr_find` (`src/conventions/tiiah/superposition.cpp`),
read by the reactive walk and the receiver's reaction. CONVENTION.md §1e.

### world fallback
How a reactive clue is read when no pairing reads on the walk's frame: the
receiver's one-away cards, leftmost first, are tried in the worlds of the hole
cards. The first that plays outright in some world, with a reaction valid there,
is the pairing, and the worlds collapse to those (CONVENTION.md §1d, v19.3.0;
replay 2017491). Since v20.6.0 subsumed by *ASCR* in the walk, which tries every
target in the worlds before the next. `interpret_reactive`, `src/conventions/tiiah/interpret_reactive.cpp`.

### receiver world fallback
How a reaction is read when the shared stamp finds nothing the receiver's target can
play on the frame it reads (v20.5.0, the user's ruling; replays 2018517, 2018535).
Every seat that watched the reacter's card reads the target in the worlds of the
hole cards: first the bucket rule (bucket and finesse halves), then any one-away
identity. The first reading that plays in some, but not every, world is called, and
the worlds collapse to those that make it. Otherwise the stamp's bluff or mistake
reading stands. The receiver's half of *ASCR*, on `ascr_find` since v20.6.0.
`receiver_world_fallback`, `src/conventions/tiiah/interpret_reactive.cpp`. CONVENTION.md §1d.

### rebased call
A stable call that could be the card a call stamped earlier in another hand
names, once that earlier call is played and every seat can name it: that identity
is replaced by the next one up its suit, so a role-inverted `{p1}` on Cathy becomes
`{p2}` after Bob plays his called p1. If Bob throws his copy, plays something else,
or plays a call nobody can name, nothing is rebased.
`rebase_calls_on_a_played_call`, `src/conventions/tiiah/superposition.cpp`.
CONVENTION.md §1c (v19.2.0), replay 2017459.

### fix precedence
A clue to Bob that names his called card as an identity a seat **knows** is dead
(trash on that seat's own stacks) is a *fix* to that seat, even when the call is
what puts the table in the *reverse-reactive position* (v18.10.0; v18.11.0; the
reviewer's rule). A seat that cannot tell reads the reverse reactive, and the
*reverse-reactive confirmation* settles it. `clue_would_fix_dead_call` and
`dead_call_fix`, `src/basics/fix.cpp`. CONVENTION.md §1c. Replay 2013726 T5.

### reverse-reactive confirmation
A reverse reactive stands only if its receiver's next non-clue action plays one of
his *standing plays* -- one of those that made the position, recorded before the
clue (`ReactorWC::receiver_standing`, v20.3.0). A play of any other card, including
one the reverse clue itself newly made playable, or a discard, withdraws it
(v18.11.0; the reviewer's rule; replay 2018428 T17). This is how a Cathy who cannot tell a fix from a
reverse reactive learns which it was. `tiiah::confirm_reverse_reactive`,
`src/conventions/tiiah/interpret_reactive.cpp:208-245`. CONVENTION.md §1c.

### role inversion
The clue-to-Cathy half of the *reverse-reactive position*: Bob holds a *standing
play* and Cathy does not, so a clue to Cathy is STABLE rather than reactor0's
ordinary reactive (v18.2.0). `inverted_stable`,
`src/conventions/variants/hole.cpp:148-156`; replay 2013645 T11. CONVENTION.md §1c.

### gotten
A card already called to play: a reactive's target walk passes over it, since a
new reactive has to get something new. When every playable and finesse target in
the receiver's hand is gotten, the walk takes the leftmost gotten one instead
(v18.5.0; human diagnostic 2013726 T38). `receiver_targets`,
`src/conventions/tiiah/interpret_reactive.cpp:247-301`. CONVENTION.md §1c.

### reverse reactive
The second of TIIAH's two dispatches. When Bob holds a *standing play* and Cathy
does not — the *reverse-reactive position* (v18.3.0; a touch-known play only from
v18.0.0 to v18.2.0) — a clue to **Bob** is reactive, with
**Cathy** reacting and Bob receiving, and a clue to **Cathy** is stable. Outside
that position reactor0's positional rule stands: a clue to Cathy is the reactive
one and Bob answers it. Not the same as reactor's rule, where a clue to Bob makes
Bob both reacter and receiver, a degenerate reading reactor scores as a MISTAKE.
`reverse_reactive_position` / `reverse_reactive`
(`src/conventions/variants/hole.cpp:158-183`), read by `tiiah::interpret_clue` and
by the decision layer's `dispatch_is_reactive`. CONVENTION.md §1c.

### double pitch
A reactive clue on which both named cards are played — both players press the
**Play** button. Every reactive clue in TIIAH is one of these or a double chuck,
because reactive clues are all even parity. What the two cards ARE is carried by
the clue kind: rank means the receiver's target is one bucket higher than the
reacter's, colour one bucket lower, either wrapping — or the clue is a finesse,
or both players know their own identity exactly, or it is the endgame (pace ≤ 1,
v18.4.0), where any pairing is legal to give. CONVENTION.md §1d.

### double chuck
What a reactive clue becomes when the receiver's only playables are on inverted
suits: both players press **Discard**, which is the button that stacks an
inverted card. Inverted playables and inverted finesses are otherwise skipped as
reactive targets. The reacter's own card has to be **affordable to chuck** —
inverted and playable, so the button plays it, or simply not critical — and a
pairing that fails that is refused outright, since the reacter cannot see their
own hand. `safe_to_chuck`, `src/conventions/tiiah/interpret_reactive.cpp:98-104`.
CONVENTION.md §1d.

### delayed call
A `CALLED_TO_PLAY` card that is not playable yet and will be once the *receiver*
has played what they already know — what a reverse-reactive finesse creates.
Alive but not actionable: reactor0's dead-call invariant judges it against the
stacks after those queued plays, and leaves the call itself out of that
simulation (`src/conventions/reactor0/call_invariants.cpp:170-209`). A call
whose reading is a valid pitch in some strike-free world of the shared view is
not dead either (v16.29.0). CONVENTION.md §1c.

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

### deferred collapse
A collapse that waits for a card's holder to act (v18.12.0). A COLOUR clue can call
a card that was **already clued** and is playable in only some worlds of the hole
cards. The clue may be asking for that card to be thrown as the dupe, so it is not
yet evidence that the team still needs the identity. Nobody collapses on it: the
outside seat's world narrowing is skipped, and so is the second collapsing rule.
The holder's play of the card is evidence in its own right. Its discard is §1e rule
7's shared form: only the worlds in which the card was trash survive, at every seat.
A call on a card **not** clued before is not deferred. CONVENTION.md §1e, human
diagnostic 2014076 T14.

### safe-to-lose chop / occupied save
Cathy's chop is **safe to lose** when it is not critical, and it is not playable
unless it is duplicated. Duplicated means a second copy in Cathy's own hand, or a
called card in anyone else's hand whose common reading is that one identity. A
locked Cathy is safe. When it is safe, Bob has no safe action (`priority_3_applies`),
and his chop is a playable card at risk that no known call duplicates, a clue touching
Bob's chop without calling it to discard is an **occupied save**
(`saves_stuck_bob_chop`). It is exempt from the tier gate, so an
Alice holding a call of her own still gives it (v18.13.0,
`cathy_chop_is_safe_to_lose` in `reactor0/decision.cpp`). CONVENTION.md §2e, human
diagnostic 2014076 T18.

### lockable chop / far chop
A chop is **lockable** when it is critical, playable or one away from playable on
Alice's own stacks. Anything further away is a **far chop**. Rung 3.7 does not lock
Bob over a far chop in this variant (v20.2.0, `chop_worth_a_lock` in
`reactor0/state_eval.cpp`). CONVENTION.md §2h, human diagnostic 2018365 T7.

### stable play hierarchy
How competing stable play clues are ranked (v18.17.0). All four keys come from the
giver's model of the receiver once the clue has landed, and each is judged over
what the key above it left:
1. the fewest identities left on the called card;
2. the most ancillary value: 1.99 per newly touched good card besides the called
   one, less 1 per newly touched trash card the receiver cannot tell is trash;
3. the smallest product of candidate counts over the receiver's other good cards;
4. colour over rank, unless the colour could mistake a rainbow card.

The default tiebreak comes last. **Unknown trash** is a trash card the receiver
cannot identify as trash, judged on the stacks the called play leaves.
`settle_stable_play` in `reactor0/decision.cpp`. CONVENTION.md §2a, human
diagnostic 2014561 T50. It ranks only the clues that survive
`misreads_its_called_card`: a colour clue that calls a rainbow card is dropped
before the hierarchy sees it (CONVENTION.md §1f, v18.19.0). A rainbow card it
merely touches is allowed (v18.20.0).

### named call / watched dupe
A **named call** is a stable play call whose holder can read it back to exactly one
identity once the clue has landed. The giver's own-dupe veto (CONVENTION.md §2c)
spares a named call (v18.18.0). The holder watched the giver's hole cards go in, so
it can check the call against them. When the giver's frame leaves the call one
identity X, and the holder watched the giver throw an X into the hole unnamed, the
call is a **watched dupe**. The holder reads the card as X (trash), withdraws the
call and throws it (`repin_own_call`, CONVENTION.md §1.3). Human diagnostic 2014561
T56.
