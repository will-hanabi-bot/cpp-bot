# The Throw It in a Hole Convention

This is the **ruling reference** for how this bot interprets clues **when a game
runs the TIIAH convention**. Where this document and the code disagree, that is
a bug in one of them — every rule that is implemented cites the `file:line` that
implements it, and every rule that is not says so in the same breath.

Terminology is in [GLOSSARY.md](GLOSSARY.md); terms not defined there (chop,
pitch / chuck, CTP / CTD, critical, loaded, …) carry their
[reactor glossary](../reactor/GLOSSARY.md) meanings. Convention that is legal
but not yet implemented is tracked in [TODO.md](../../../TODO.md), where this
convention's open entries are **44** — a suit whose low card went into the hole
unseen can read as unplayable to every seat at once (§1.3) — **45**, the
receiver's bucket narrowing being wired only into the play path (§1d), **46**, one
WC field serving two frames (§1.3), **48**, the shared view not being identical at
every seat (§1.3), **49**, a finesse pairing read wider than it needs to be (§1d),
**50**, the world cap abandoning a whole floor (§1.3), **52**, a shared call judged dead on
a private view — the reason 2010512's strike is still unfixed (§1h) — **53**, the
envelope form of a negative-touch fix (§1h), **54**, rule 6's shared collapse not
checking the condition rule 1 insists on (§1e), **58**, the striker not learning
what its own dupe strike was (§1e rule 8), and **59**, hole-card worlds replayed on
today's stacks rather than on the stacks of their turn (§1e).

Reading conventions, as in the other two documents: **slot 1 is the leftmost,
newest card**; **Alice / Bob / Cathy** are positional — Alice is the clue giver,
Bob the next player, Cathy the one after.

## §0 Status

**The bot PLAYS these games as of v16.6.0.** From v16.0.0 to v16.5.0 it read
them and refused to act — `Game::take_action` threw — because the decision layer
it would otherwise have used was reactor0's, which priced clues by *reactor0's*
meanings. Before v16.0.0 the flag was not read at all and these tables were
played by reactor's rules, which reactor's own §1b.8 said so; stopping that is
what v16.0.0 was for, and finishing it is what v16.6.0 is.

| Part | State |
|---|---|
| The engine rules (§1) | implemented; the three stack views (v16.12.0); a row reads its own seat's worlds (v16.18.0); a watched card one above a row floors it, struck or not (v20.14.0) |
| Buckets (§1a) | implemented |
| Stable clues (§1b) | implemented, by delegation to reactor0; a colour play reveal must be global (v18.20.0); a pinkish re-touch reads the global stacks (v19.0.0); a rank clue on the lock slot is a referential discard when it has a target (v20.0.0); a colour clue focuses its newly touched cards first (v20.4.0) |
| The dispatch, both arms (§1c) | reverse implemented (v16.2.0), ordinary (v16.8.0); the refusal (v16.14.0); the refusal given past the tier gate, and ranked (v16.27.0); the known play behind the position read from clue touches on the shared view, alike at every seat (v18.0.0); in the endgame (pace ≤ 1) a pairing may break the bucket relation (v18.4.0); with every target gotten, the walk takes the leftmost called one (v18.5.0); the own-dupe filter spares a stuck Bob's chop (v18.7.0); a reacter reads its own card by elimination when its bucket reading is ruled out by sight (v18.9.0); a settled call is a standing play, and a fix takes precedence over a reverse reactive (v18.10.0); a reverse reactive stands only if its receiver plays a standing play, and a fix is read by a seat that knows the call is dead (v18.11.0); role inversion — a standing call on Bob keeps a clue to Cathy stable (v18.2.0), and a stable call overlapping a played call builds on it (v19.2.0); the reverse position keyed on a standing play again (v18.3.0, human diagnostic 2013726 T30); an uncalled card is a standing play only as a sure play, none of whose identities the hole could hold, and the reverse reactive is confirmed only by a standing play recorded before the clue (v20.3.0); a newer call never erases a call standing to its left (reactor0 rule 1 off, v20.11.0); a clue whose position rests on a reaction owed to us waits for that reaction, then re-reads, and a fixed reaction card is thrown at once or the reaction is off (v20.12.0) |
| The bucket-encoded reactive (§1d) | implemented (v16.3.0); the receiver's half (v16.9.0); its held negative (v16.11.0); the relation as a giver-side legality test (v16.15.0); read in the giver-and-receiver frame (v16.18.0); the receiver reads the reacter's card too (v16.19.0); the receiver's reading no longer dropped for missing the stamp (v16.26.0); an undeferred call no longer dropped as stale (v16.28.0); a finesse the receiver can prove, and the reacter's frame without its own card (v17.1.0); the reacter's card named for the team only as far as the receiver can (v17.2.0); a deferred reaction read the same way (v17.4.0); the receiver read before the call invariants, and a withdrawn target disarms the held negative (v18.14.0); the reacter's card is read only off the reacter's own play, not the reverse receiver's standing play (v20.16.0); an unreadable reactive falls back to the worlds (v19.3.0), and so does an unread reaction at the receiver (v20.5.0); the walk tries each target in the worlds before the next, on the shared ASCR routine (v20.6.0); a receiver reading conditional on every world keeps its call (v20.8.0); the reacter's own card reads its bucket in its own worlds, judged before the stamp (v20.10.0); a reacter whose finesse runs through the giver's own unknown hole play throws it (§1k), forward or reverse, rather than re-reading it on its own stacks (v20.1.0) |
| Superposition (§1e) | implemented (v16.1.0); the back-solve (v16.12.0); conditional readings (v16.13.0); never presume a strike (v16.16.0); the receiver reads the worlds too (v16.17.0); rule 3's pair form and rule 6 on one's own plays (v16.18.0); rule 6 raising our own stacks (v16.19.0); rule 6 SHARED (v16.21.0); rule 7, a named playable discarded was already played, and no gentleman's discard (v16.22.0); rule 6 against the shared view, and rows as floors (v16.23.0); the frame is the minimum across worlds, stable calls read in every world, world feasibility from reactions, play-order replay, evidence bands and a floored shared view (v16.24.0); the shared view settles on what every world agrees, the receiver's promise read on its own frame, rule 8 (a strike was already down) and hole requirements, notes in the team's reading (v16.25.0); a refusal with several candidates, and a watched dupe strike, floor the shared view (v16.27.0); the band never absorbs a named card (v16.28.0); a call live in some shared world is not dead (v16.29.0); an outside seat tries its own hole cards for a stable call (v17.1.0); a private deduction about our own hole cards stays out of the rows (v17.3.0); the target of a call on a card it can name settles its own hole cards (v17.5.0); a row replays our private settles with the set both seats of the pair hold (v18.1.0); a colour re-touch of a clued card defers the collapse to its holder, and rule 7 has a shared form (v18.12.0); a pink identity clue names a card for rule 7 (v19.0.0); a watched hole play is named by sight, not by a stale common copy (v19.1.0); a stable call is not widened by the giver's own hole cards (v20.4.0); the Actionable Superposition Collapse Rule, one shared routine (v20.6.0); a watched play that lands only if one of several hole cards was the card below it records that joint fact (v20.15.0); a card the team names above the shared view explains the gap below it (v20.16.0); a hole card is noted as its reading narrows (v20.17.0) |
| Rainbowy colour pinning (§1f) | implemented (v16.4.0); the giver's half, a colour clue never calls a rainbow card (v18.19.0), and may touch one it does not call (v18.20.0); a rank play clue on a new slot-1 card pins the rainbow (v20.13.0) |
| Decision making (§2) | implemented (v16.6.0), by delegation to reactor0; among reactive plays, the fewest receiver candidates (§2b, v16.28.0); never call a card we may already have played (§2c, v17.2.0), unless the call is named, whose holder reads a watched dupe (v18.18.0); never call a critical card to discard (§2d, v17.4.0); a critical chop nothing can ditch around is locked (reactor0 rung 3.10, v17.5.0); a locked hand below 8 tokens throws its least-critical card rather than pitch blind (reactor0 rung 12, v17.6.0); an occupied Alice saves a stuck Bob's chop when Cathy's is safe to lose (§2e, v18.13.0); a locked Alice's stable play may go to a role-inverted Cathy (§2f, v18.15.0); a stuck Bob gets a play before a lock of a non-critical chop (§2g, reactor0 rung 3.6b, v18.16.0); no lock over a chop worse than one away (§2h, reactor0 rung 3.7, v20.2.0); no forced clue at 8 tokens while Alice can play (§2i, reactor0 §4, v20.2.0); a spent reaction stops being urgent, its pairing now recorded (§2j, reactor0 Precedence step 2, v20.7.0); a call that plays in every world of our hole cards is a known play (§2k, v20.9.0), and so is one that plays on the shared view (§2k, v20.11.0) |
| The fix clue (§1h) | implemented (v16.20.0); deadness on the giver-and-holder view, and the call withdrawn by the fix (v17.2.0) |
| Naming the called card (§2a) | implemented (v16.7.0); the stable play hierarchy — fewest inferences, ancillary value, the rest of the hand, colour (v18.17.0) |

What has **not** been decided is the endgame: the solver declines to solve a
TIIAH position rather than solving it wrongly, because our own hidden plays make
its card accounting inconsistent (`src/endgame/solver.cpp`). The stall list and
the ordinary rungs carry those turns. That is a deliberate hold, to be settled
after the convention has been played in anger.

Which convention a game runs is `Game::convention`, resolved at game init
(`resolve_table_convention`, `src/basics/convention.cpp:23-28`, called from `src/net/commands.cpp:384-385`). TIIAH is resolved from the **variant** and
never from `/setall`: only these 44 variants can be played under it, and no
other convention can be played at them. `parse_convention` accepts the name
`"tiiah"` so a snapshot round-trips, but `/setall tiiah` is not a way to select
it.

The convention is a **3-player** one, like reactor0 — the sum rule in §1d takes
its modulus from the hand size. A TIIAH table of any other seat count resolves
here too (`commands.cpp` keys on the variant, not on the count), which is a gap
rather than a claim: the reverse-reactive dispatch names a Bob and a Cathy, and
at four seats there is a fourth player the rules say nothing about.

## §1 The rules of the variant, and what they cost the engine

`throwItInAHole` (`Variant::throw_it_in_a_hole`,
`include/hanabi/basics/variant.h:129`, parsed at `src/basics/variant.cpp:128`)
marks 44 variants. In them:

- **A play goes in the hole.** Pressing Play hides the card from everyone, the
  player who played it included, and nobody is told whether it landed.
- **Discards are visible**, as in any other variant.
- **A 5 returns no clue token.**
- **The score and the strike count are hidden** while the game runs. It ends the
  moment max score is reached or the third strike lands.
- **The inverted suits invert the hiding too.** A *chuck* — Discard on an orange
  — reaches the orange stack, so it goes in the hole; a *pitch* — Play on an
  orange — reaches the discard pile, so it is visible.

The engine consequences are **keyed on the variant flag, not on
`Game::convention`**. A TIIAH table the convention will not act on — 4+ seats,
per §0 — is still tracked as a spectator would track it, and its stacks have to
be right whatever the bot does with its turn.

### §1.1 We fill in what the server withholds

Each seat knows every card played **except its own**: we watched a partner's card
sit in their hand, so `state.deck[order]` still knows what it was even though the
action carries no identity.

`resolve_hidden_action` (`src/basics/action.cpp:82-133`) rebuilds the
button-oriented action the engine would have received in an ordinary variant,
deciding the outcome from the identity and the stacks rather than from the wire's
claim:

| The wire says | The card is | The engine is given |
|---|---|---|
| it reached the hole | plain, playable | Play — the stack advances |
| it reached the hole | plain, dead **in every world** | a failed Discard — a strike |
| it reached the hole | inverted | Discard (a chuck), failed if dead |
| it reached the pile | inverted | Play (a pitch) |
| it reached the pile | plain | Discard |

It runs as a pre-step in `Game::handle_action` (`src/basics/game.cpp:581`), which
is the only choke point live play, snapshot replay, `rewind` and `simulate` all
share — and it has to be there rather than inside `on_play`, because resolving a
hidden card can change the action's **type** and `on_play` cannot rewrite itself
into `on_discard`. `add_action` still records the RAW action, so the log says
what the wire said and the resolution stays a pure function of the action history
plus our own sight, which is what lets `rewind` and `apply_snapshot` reproduce
it without serialising anything.

**"Dead" is asked of the WORLDS, not of one stack vector (v16.16.0).** Our stacks
can be short by exactly what we threw in the hole, so a partner's play that looks
dead to us may be landing on a card we played without knowing it. §1e rule 6 runs
immediately before the resolution and settles that question first; a strike is
what is left when **no** world rescues the card. Replay 2010296 is what the old
order cost: will-bot69 invented a strike, booked a partner's `y2` into the discard
pile, and — the action now being a failed *discard* — skipped everything wired to
`interpret_play`.

**Our own play is the one the state cannot name**, because `deck[order].id()`
is `nullopt` for our seat. Our own EMPATHY often can, though, and a play we can
name is a play we can account for: `hidden_own_play_id`
(`src/basics/game.cpp:555-577`) hands that identity to the resolution. What is
left when empathy cannot name it is §1e's superposition — the hand updates and
the stacks wait.

Because of all this, `State::play_stacks` in a TIIAH game is **what we believe**,
not what is. So are `score()`, `strikes` and therefore `State::ended()`. A wrong
belief is invisible until the game ends; that is the variant, not a defect.

### §1.3 Three views of the stacks

A seat knows every play but its own, so no two seats hold the same stacks — and
we know more about a partner's play than they do, having watched the card leave
their hand. A clue still has to mean ONE thing, so the engine keeps three views:

| | advances on |
|---|---|
| `State::play_stacks` | **our belief** — every play we can name, which is every partner's and our own identified ones |
| `State::common_play_stacks` (`state.h:59`) | **the shared view** — only plays whose identity was common knowledge, plus collapses on evidence every seat shares |
| `State::pairwise_play_stacks[p]` (`state.h:60-73`) | **the pairwise view** — what we know seat `p` knows: the shared view, plus every hidden play we can name that `p` did not make |

**A clue is read against the PAIRWISE view, not the shared one (v16.12.0).** The
shared view is what ALL THREE seats know, and a hidden play is known to everyone
except the seat who made it — so one seat's ignorance holds the reading back for
the whole team. A clue only has to mean one thing to the two seats it is between.

Replay [2008489](https://hanab.live/shared-replay/2008489) T33 is the case.
yagami clues blue to will-bot69, whose next blue is the `b4` it is holding: `b1`
and `b2` were will-bot69's own plays and it knew both, and the `b3` was
will-bot67's. Both the giver and the receiver therefore hold blue on 3 — only
will-bot67 does not, because it cannot see what it threw. Read against the shared
view the call was a `b3` that had already gone in, so will-bot69 read a stall and
left the card unplayed.

Three properties make the row the right object rather than a patch:

- **It is computable exactly, and only, by the two seats it is about.** Being one
  of the pair is what lets us name the third seat's plays; a seat outside the pair
  is missing precisely its OWN plays and cannot name them. `State::stacks_known_to_both`
  (`state.cpp:309-317`) is symmetric for that reason, and falls back to the shared
  view for an outsider — who recovers the rest by the back-solve (§1e).
- **It is a FLOOR (v16.23.0).** A play both seats of the pair can name raises the
  row to AT LEAST that card — they watched it land (never presume a strike), and a
  stack only goes up, so they know it stands that high whatever lower card neither
  of them can name. "Both can name" is every play neither of them made, and a play
  its own player could name (the `known` case: a card the whole team could name
  is in every pair involving its player, because the partner watched it). And what
  they name is the card they WATCHED, not our copy of the player's reading
  (`tiiah::note_hidden_action`, `tiiah/superposition.cpp:1467-1633`;
  `State::with_pairwise_at_least`, `state.cpp:222-239`). A play our own belief
  resolves as a STRIKE is booked into nobody's view, with one exception, a hard
  floor (v20.14.0, the user's ruling, below). A row is never below the shared view
  (`State::with_rows_at_least_common`, `state.cpp:241-254`).
  `tests/test_tiiah/test_pairwise_stacks.cpp`, `test_known_play_floor.cpp`.
- **A watched card one above a row floors it, struck or not (v20.14.0).** If both
  seats of a pair watched a card go into the hole that is at most one above their
  row for its suit, the row rises to it, whatever our own stacks say. A landed card
  raised the stack, and a struck one was a duplicate: one above a floor can only
  strike by being down already. For a 1 that is every time, so however many copies
  of a 1 go in, that stack is at least 1.
  - Asked of each row alone (`superposition.cpp:1561-1600`), and only where both of
    the pair SAW the card: a third seat's, named by sight. A card one of the two
    played is named only by its player's reading, and a misread would floor the
    row above the truth.
  - The row's STACK rises, never its evidence, so the rank joins the row's band.
    The pair cannot say whether this card landed or a hole card of theirs was the
    one that did, and a band card is one a hole card can be (§1e). Raising the
    evidence too named the watched card as the one down, and every world where an
    earlier hole card was that 1 read as a strike. Self-play seed 77: Bob's o9 was
    the y1 and Cathy's watched y1 the duplicate; the replay flipped o9 to the r1
    and raised the row's red above the truth.
  - The common view's half, a card its own player could name, was already
    unconditional (`known_play_lands_in_common`).
  - Replay [2019249](https://hanab.live/shared-replay/2019249) T33:
    - will-bot69 and will-bot67 both watched yagami's y1 go in, but will-bot69's own
      stacks booked it as a strike, since it had settled its own o11 as the y1.
    - So their row kept yellow on 0, and will-bot69 gave a Green finesse.
    - will-bot67, reading yellow on 1, answered it on o1 into a strike.
  - `tests/test_tiiah/test_watched_one_floors_the_row.cpp`.

  Until v16.23.0 a row advanced through the PREFIX, one card at a time, and a play
  that arrived above a card the row had not yet named was dropped for good.
  Replay [2011327](https://hanab.live/shared-replay/2011327#38) is what that cost.
  At T11 will-bot67 played an r2 it knew, while its row for will-bot69 still had
  red on 0 behind will-bot69's unnamed r1; the r2 never entered that row, and red
  stuck on 1 for the rest of the game. At T19 will-bot69 played a y2 that
  will-bot67 had read as `{y1}` on a stale shared view; the rows took the y1,
  missed the y2, and the y3 at T25 could not land on top of it. By T38 will-bot67
  held its row for yagami at `41133` against the truth's `43133`, gave a reactive
  Yellow to will-bot69 whose pairing assumed yellow on 1, and yagami — reading the
  y4 as directly playable — reacted on a trash p1 and struck.

  The floor does not reach TODO.md 44 (replay 2008489 T52, still `discard(46)`):
  there the cards above the gap are the row's own seat's blind plays, which that
  seat cannot name, so they never enter its row at any height.
- **Our own row is never consulted.** Against ourselves there is nothing we do not
  know, so `pairwise_view(us)` hands back our belief.

**A row also takes what the seat behind it can WORK OUT about its own plays
(v16.18.0).** The row leaves those plays out because that seat cannot name them —
but it can still reason about them. Across the worlds they leave open, §1e rule 6
refutes the ones in which a play struck, and a height every survivor reaches is one
the seat holds. `tiiah::advance_rows_from_own_worlds`
(`tiiah/superposition.cpp:1357-1404`), run at the end of every
`collapse_superpositions`, and `tests/test_tiiah/test_row_from_own_worlds.cpp`.
The replay covers both seats' hole cards and uses the candidate sets both seats hold.
A card we settled privately goes in with the team's set, not our narrowing
(v18.1.0, §1e), so the partner's copy of the row and ours reach the same height.

**Our own belief takes the same floor (v16.19.0)**, by the same `world_floor` over our
own hole cards — see §1e rule 6. Between v16.18.0 and v16.19.0 only the rows had it,
which meant we could read a partner's card against a frame more advanced than the one
we read our OWN card against; replay 2010512 is the game that lost.

Replay [2010329](https://hanab.live/shared-replay/2010329) T14 is the case.
yagami's orders 6 and 7 are each `{g1,b1}`, so `(g1,g1)` and `(b1,b1)` each strike
and green and blue are on 1 whichever way round the other two were. Its row read
`[3,0,0,0,0]`, and the rank 2 it gave against that frame promised a card that was
already trash — the reading is written up in §1d.

The enumeration takes **our** hole cards as well as that seat's, which is what
makes the answer the *pair's* rather than ours about them: the seat watched our
cards go in, so what it can work out depends on them too, and a seat computing the
same pair's row runs over the same two seats' cards from the same base. It cuts
both ways — our own hole card can withhold a claim the row alone would support,
because a card of ours may have duped the one the seat is reasoning about.

What a row must never do is hold more than the seat behind it. Two things keep it
honest, and both are about whose eyes an answer rests on: §1e rule 3 has a **pair
form** that only counts copies neither of the pair is holding, and the world
enumeration above spans both seats. What our own belief alone can work out — rule
3's private form, `all_copies_visible` — stays out of every row.

Anything deciding what a clue MEANS reads a shared view — `State::shared_view`
(`src/basics/state.cpp:279-297`) is that state, with `playable_set` and
`trash_set` rebuilt to match, and `State::shared_score` / `shared_pace`
(`state.h:121-124`) and `Game::shared_in_endgame` are the questions asked of it.
Outside TIIAH the second vector is empty, `shared_view` returns the state
unchanged and the accessors are the ordinary ones, so no other variant pays for
any of this.

As of v16.4.0 the whole interpretation runs on a shared view rather than a rule
here and there, and as of v16.12.0 each rule takes the NARROWEST view the seats it
concerns all hold:

| What | Which view, and how |
|---|---|
| which slot a stable clue names (§1b) | the giver and the RECEIVER's pairwise view — `SharedStacks`, a scoped swap of `play_stacks` / `playable_set` / `trash_set` around the delegation (`tiiah/interpret_clue.cpp:31-59`, installed by `run_ladder` `:530-558`), plus a `prev` swapped to match |
| which slots a reactive clue pairs (§1c, §1d) | the giver and the REACTER's, since the reacter is the seat that must act — `stacks_known_to_both` into `reacter_faces` / `stacks_after_queued_plays` (`reacter_frame`, `tiiah/interpret_reactive.cpp:176-190`, used at `:324-328`; `variants/hole.cpp:10-24`) |
| what a reactive PROMISES the receiver (§1d) | the giver and the RECEIVER's, since the promise is about the receiver's card — `ReactorWC::clue_play_stacks`, bound at clue time (`tiiah/interpret_reactive.cpp:352`), corrected when we name one of our own hole plays (`correct_frozen_frames`, v16.21.0), and rewound by `reactor0::stamp_receiver_call` (`reactor0/interpret_reaction.cpp:365-390`) |
| what a call SAYS the card is | the HOLDER's own belief when the holder is us — `repin_own_call` for a stable call (`tiiah/interpret_clue.cpp:83-180`) and the reacter's own reading of the pairing (`tiiah/interpret_reactive.cpp:679-756`) |
| the §1f pin | `shared_view()` directly (`tiiah/interpret_clue.cpp:268`) |
| §1b's stall context | `Game::shared_in_endgame` (`tiiah/interpret_clue.cpp:518-520`) |
| the stable orange ladder's pitch-vs-chuck test | `State::shared_pace` (`reactor0/interpret_clue.cpp:545`), reached by delegating |
| the reaction's held negative (§1d) | asks the BUTTON, not the stacks — `Game::fire_reaction_elim` (`basics/decide.cpp:114-139`) |

The swap costs nothing until the views actually differ — `SharedStacks::needed`
— which is every game until somebody plays into the hole without knowing what
they played. What a seat DECIDES still reads its own belief, which is the best of
the three: the other two are for meanings only.

**The one sanctioned disagreement.** Which slot a clue names is the same at every
seat, because it rests on a view the giver and the actor both hold and the
receiver has no freedom left once the reacter has moved. What the called card IS
can differ: the holder reads it on their own stacks, which are at least as
advanced as anything the pair shares, and an observer reads it on the pair's. The
holder's is the one that governs, since they are the one who plays it — **except** a
reacter whose pairing is a finesse through a connector the GIVER threw into the hole
unknowingly: that card is the connector, and is thrown (§1k's discharge, v20.1.0),
not re-read as a bucket on the holder's stacks.

**But the holder never drops what the giver could have meant** (v17.2.0, the user's
ruling). The holder's stacks can be ahead of the giver's by exactly the giver's own
hole cards, which the holder watched and the giver cannot name. A card that is dead
on the holder's stacks only because of one of those is one the giver may well have
called: a duplicate it threw in the hole without knowing. So `repin_own_call` keeps
every identity that is playable in some strike-free world of the giver's hole cards,
on the frame the two of them share (`tiiah/interpret_clue.cpp:83-180`). The reading
then stays true, and a partner who can see the card can fix the call (§1h). If
nobody does, the holder plays as before. For example, Bob's `{g1,b1}` was the g1,
his Rank 1 named Cathy's other g1, and she reads `{r1,y1,b1,p1}` rather than an
empty or dead call.

**...unless the giver named it exactly, and the holder watched the dupe go in
(v18.18.0).** What the giver *meant* is what is playable on the frame the two of
them share: that frame is the giver's own belief, and the giver cannot know which
world of its hole cards it is in. Suppose that reading leaves the holder's call one
identity X, and the holder watched the giver throw an X into the hole unnamed. Then
the call can only be the dupe. The holder:
- narrows the card to X, which is trash to it;
- withdraws the call;
- throws the card instead of holding it.

The code is in `repin_own_call` (`tiiah/interpret_clue.cpp`). A call the giver did not
name so exactly keeps every reading, as above.

This is the receiver's half of §2c's named-call exception, which lets the giver call
a card its own hole card may already have played. Human diagnostic 2014561 T56
(`v18_human_vs_bot_diagnostics/2014561.md`): "yagami_black will toss it if
yagami_blue already played the other copy."
`tests/test_tiiah/test_named_call_on_a_watched_dupe.cpp`:
- Cathy's Yellow on our card after we watched her y4 go in reads `{y4}`, not
  called and known trash;
- the control is a b1 in her hole, where the y4 call stands.
The reacter's own reading of a reactive (§1d) is unchanged.

The reaction's negative was the third entry in that table only as of v16.11.0,
and it was the one that cost a game: it asked "did a stack advance?" to tell a
receiver's play from their discard, which in this variant is the one question
the stacks cannot answer. Replay 2008422 is written up in §1d.

Three things still read our belief where they arguably should not, all inside
reactor0 and all out of reach of the swap: `common.hypo_stacks`, rebuilt from
`play_stacks` by the elim layer; `Player::hypo_stacks`; and
`reactor0::enforce_call_invariants` (`basics/decide.cpp:295-299`), which runs
*after* the swap has unwound and so judges rules 3 and 4 — whether a standing
call still has a button that works — against one seat's private stacks. Its own
comment says a call "has to die for every seat at the same moment"
(`reactor0/call_invariants.cpp:175-177`), so that one is a real gap rather than
a tolerable one. Since v16.29.0, rule 3 keeps a call alive if any strike-free
world of the shared view still allows it (§1c). That half is the same at every
seat, but the private half still decides the rest, so the gap (TODO.md 52) is
narrowed rather than closed. The first two matter for delayed-play chains rather
than for the call itself.

### §1.2 A 5 pays nothing

`State::with_play` skips the clue-token refund on the final rank
(`src/basics/state.cpp:138`). Gated there rather than inside `regain_clue`,
because an ordinary discard still refunds: the rule is about the hole.

### §1a Buckets

Drop the inverted suits, re-index what is left from 0, and map on the count that
**remains** (`suit_buckets`, `src/conventions/tiiah/buckets.cpp:7-34`;
`bucket_of`, `:36-48`):

| Suits remaining | Bucket 0 | Bucket 1 | Bucket 2 |
|---|---|---|---|
| 3 | `[0]` | `[1]` | `[2]` |
| 4 | `[0,1]` | `[2]` | `[3]` |
| 5 | `[0,1]` | `[2,3]` | `[4]` |
| 6 | `[0,1]` | `[2,3]` | `[4,5]` |

So `& Orange (6 Suits)` uses the five-suit row, `& Orange (5 Suits)` the
four-suit row, and `& Orange (4 Suits)` — Red, Green, Blue — the three-suit one.
An inverted suit is in **no** bucket: it is never a reactive target (§1d).

Three buckets is what makes the ±1 of §1d unambiguous.

### §1b Stable clues — reactor0's, with two exceptions

A stable clue means what it means under reactor0, and the code delegates rather
than forking a copy that would drift: `tiiah::interpret_clue`
(`src/conventions/tiiah/interpret_clue.cpp:530-558`, `run_ladder`) calls
`reactor0::stable_colour` / `reactor0::stable_rank`. Read
[reactor0's §1b and §1c](../reactor0/CONVENTION.md) for what they do.

One of reactor0's rules applies here by the variant (v20.0.0): a rank clue that
touches the **lock slot** is a referential discard whenever it has a target, and a
lock only when it has none. It is reactor0 §1c priorities 5/6 (replay 2017568,
where a 4 touching will-bot69's slots 2, 4 and 5 calls slot 3 to discard).

**The exception (v18.20.0): a colour play reveal must be global.** reactor0's
colour ladder puts a play reveal (a previously clued card the clue fills in as a
new obvious playable) above the direct play on the leftmost newly touched card.
In this variant the ladder runs on the stacks the giver and receiver share (§1.3),
and on those a clued card can be revealed as playable when it is not on the stacks
every seat knows. There the reveal does not outrank the leftmost newly touched
card, and the clue is read as a direct play. The receiver cannot know the giver
knows the stack is high enough. TIIAH passes `common_play_stacks` to
`stable_colour` as its `reveal_frame`
(`src/conventions/tiiah/interpret_clue.cpp:552-553`), and the reveal is kept only
if it is also a reveal on that frame
(`src/conventions/reactor0/interpret_clue.cpp:473-482`). A rank clue is unaffected.

Replay [2015013](https://hanab.live/shared-replay/2015013#35):
- **T35:** yagami_black clued Blue to will-bot69, touching its clued o14 (the b2)
  and a new o33. The b1 was still superposed, since yagami's o5 could be g1 or b1,
  so blue on 1 was not global. The clue is not a reveal of the b2; it calls the
  leftmost newly touched o33.
- **T36:** will-bot69 played the b2 in full view, and blue on 2 became global.
- **T37:** Blue on yagami's clued 3 is a reveal of the b3 on every seat's stacks,
  and outranks the unclued ra3 it also touches.

`tests/test_tiiah/test_colour_reveal_frame.cpp` has the rule both ways: a reveal
only the pair can see yields to the leftmost newly touched card, and a global one
does not.

**A caveat.** "The stacks every seat knows" is each seat's own `common_play_stacks`,
and in self-play the seats disagree about it after 44% (5 Suits) to 60% (Rainbow)
of actions. The rule can therefore still split a giver from its receiver. In
self-play seed 59 at T14, the giver's common view had red on 1 and gave Red as a
global reveal of an r2. The receiver's had red on 0; it read the leftmost newly
touched card as `{r2}`, and struck on an r3. Against the same build without the
exception (seeds 1–300), it scored:
- **5 Suits:** the same mean (19.44), 97 → 93 strikeouts, 1335 → 1317 cards ever
  read wrongly;
- **Rainbow:** mean 18.62 → 18.93, 174 → 172 strikeouts, 1939 → 1920 cards.

**The second exception (v19.0.0): a pinkish re-touch reads the global stacks.**
reactor0's pink tempo, pink trash and pink identity clues (its §1c priority 0)
rest on the stacks: the tempo clue calls the next playable pink, and the trash
clue needs the clued rank to be down in every pinkish suit. Here they read the
stacks **every** seat knows, not the pair's. TIIAH passes `common_play_stacks` to
`stable_rank` as its `pink_frame`
(`src/conventions/tiiah/interpret_clue.cpp:544-545`), the same frame in every §1e
world rerun. Which card is "rank identity known exactly" is read from `common`,
the global knowledge, as it already is.

Replay [2015070](https://hanab.live/replay/2015070) (TIIAH & Pink): will-bot67's
T5 hole card was the i1, unnamed, so pink was on 1 for the seats that watched it
and on 0 globally. At T18 yagami's 1 re-touched barakeel's known-pink i1. On the
global stacks 1 is not down, so it is a pink **identity** clue naming the card
the i1, not a pink trash clue. barakeel's discard of it at T19 then settled
will-bot67's hole card (§1e rule 7). `tests/test_tiiah/test_pink_rank_clues.cpp`
has the frame: a 1 that is down only for the pair reads as identity, not trash.

The TIIAH dispatcher differs from reactor0's in two ways only: there is no
blind-family arm (no TIIAH variant is a Blind one — all 44 carry
`throwItInAHole` and no other behavioural flag) and no target-parity arm.

### §1c The dispatch: both reactives, and the position that switches them

A **standing play** is either of two things:
- a card called to play, whatever its inference and touches, whose call every seat
  stamps alike: one that is **clued** (a stable colour clue stamps its call by the
  nature of the clue), or one that is **settled**, meaning no longer urgent. A
  receiver's call whose reaction has been played is settled (v18.10.0);
- a **sure play** (v20.3.0): a card, called or not, every identity of whose
  clue-touch empathy (`possible` in `common`) is playable on the shared view **and
  could not already be in the hole**. That is, no unsettled hole card's team set
  (`shared_left`, else `superposition`) holds an identity of that suit at or beyond it
  (`possibly_in_the_hole`). The identities need not be a singleton: a yellow card
  filled in as a 1 is a sure play, and so is a `{y1,g1}` the hole cannot hold.

A **pending** reaction call (`ConvData::urgent`) is left out, being the kind a seat
can stamp differently (`has_standing_play` / `is_standing_play` /
`possibly_in_the_hole`, `src/conventions/variants/hole.cpp:83-146`).

**Why "sure" and not merely "playable on the shared view" (v20.3.0, the user's
ruling).** There is no good touch on an ancillary touched card, so a card a clue
touched but did not call may be trash. And the shared view is the *minimum* across
the hole's worlds, so an identity playable there may already be down. Replay
[2018428](https://hanab.live/shared-replay/2018428):
- **T7:** black's 1 to will-bot69 called o15 and only touched o5, `{r1,y1,g1,b1,p1,ra1}`.
- **T16:** black's 1 re-touched o5 and touched the new o23. Read as a known play, o5
  put the table in the reverse position, and will-bot67 read a reverse reactive.
- **T18:** will-bot67 blind-played its o11 as a g2, into a strike.

o8 `{g1,b1}`, o13 `{r1,y1}` and o15 (any 1) could each hold one of o5's 1s, so o5
was no standing play. The 1 was stable, a call on o23, as will-bot69 read it.
`tests/test_tiiah/test_replay_2018428_reverse_reactive_needs_a_called_play.cpp`.

**A fix takes precedence over a reverse reactive (v18.10.0; v18.11.0).** The rule
is the reviewer's:
- Cathy played a card matching one of the candidates of Bob's called card, so from
  her view Bob's call is in superposition with a dead identity.
- Alice then gives Bob a clue that identifies the called card as that dead card.
- A seat that **knows** the identity is dead reads the clue as a **fix**, not a
  reverse reactive, even though Bob's call is what put the table in the reverse
  position.

Knowing means the identity is trash on the seat's **own** stacks. Before the clue,
from its touches, both the dispatcher and `dispatch_is_reactive` ask this through
`clue_would_fix_dead_call` (`src/basics/fix.cpp:113-141`). So:
- The giver and Bob watched Cathy's blind play, so they know.
- Cathy knows when she can name that play privately. In replay 2013726 T5, black
  knew its o0 was the r1 from blue's call for a finesse into red 2, so all three
  seats read green's 2 as a fix.
- The fix itself is then read by `dead_call_fix` (`:87-111`). It judges deadness
  on the stacks the giver and the holder share, or on a candidate of a hole card
  someone other than the giver played (`hole_candidates_not_by`, `:74-83`).
- On the giver's side, a fix is still given only for a card the giver sees is down
  (`clue_fixes_dead_call`, `reactor0/decision.cpp`).

**A reverse reactive stands only if its receiver plays a standing play (v18.11.0).**
This is the reviewer's rule for the Cathy who **cannot** tell.
- **The case.** Suppose green's o7 in 2013726 had been a b2 rather than the r2.
  Then black's o0 would stay `{r1,g1}` to black, and green's 2 to blue would be a
  fix to green and blue but ambiguous to black.
- **What Cathy does.** She reads the clue as the reverse reactive and waits for
  Bob.
- **What confirms it.** Only a play by Bob of one of the standing plays that **made
  the position**: a sure play, or a clued or settled call, recorded from the
  position *before* the clue (`ReactorWC::receiver_standing`, filled on the reverse
  arm of `tiiah::interpret_reactive`, v20.3.0).
- **What withdraws it.** Any other non-clue action by Bob means the clue was
  something else, and the reverse reactive is off: a play of an uncalled card (the
  b2 on o11), or a discard of the dead card. That includes a card **the reverse
  clue itself made playable** (v20.3.0, the user's ruling). In replay 2018428 T17,
  will-bot69 played the newly clued o23; even had o5 been a standing call, that
  play would have withdrawn the reverse reactive. Until v20.3.0 the check asked
  `is_standing_play` after the clue, which the newly touched 1 passed.

The check is `tiiah::confirm_reverse_reactive`
(`src/conventions/tiiah/interpret_reactive.cpp:208-245`). `Game::handle_action`
(`src/basics/game.cpp:617-622`) calls it before any hole bookkeeping books the
play, because that bookkeeping would move the known play off the shared view.
- **Withdrawing** clears the waiting connection, retires its pending copy, and drops
  the reacter's call on its reaction card, marking the card RESET.
- **A clue** from Bob changes nothing.
- **It applies to every reverse reactive.** In replay 2013963 T11, will-bot67 plays
  its called o17, so the reactive stands, and will-bot69 answers with its b2.

Tests: `tests/test_tiiah/test_fix_before_reverse_reactive.cpp` covers the giver's
seat, a Cathy who cannot tell, a Cathy who knows, and the control;
`tests/test_tiiah/test_reverse_reactive_confirmation.cpp` covers the standing play,
the uncalled play, and the discard; `tests/test_tiiah/test_reverse_reactive_standing_play.cpp`
covers the sure play and a play of the card the reverse clue newly touched (v20.3.0).

TIIAH runs **both** dispatches — reactor0's positional one and the reverse — and
the **position** decides which seat's clue carries the reaction. The position
holds when **Bob has a standing play and Cathy does not**
(`reverse_reactive_position`, `src/conventions/variants/hole.cpp:158-174`):

| position | a clue to **Bob** | a clue to **Cathy** |
|---|---|---|
| holds | **REACTIVE** — Cathy reacts, Bob receives (the reverse reactive) | STABLE (role inversion) |
| otherwise | STABLE | **REACTIVE** — Bob reacts, Cathy receives (reactor0's) |

The clue-to-Cathy half is **role inversion**, `inverted_stable` (`:99-107`), which
the giver's side asks too in `reactor0::dispatch_is_reactive`
(`src/conventions/reactor0/interpret_reactive.cpp:1048-1068`).

**A stable call that overlaps a standing call builds on it once that call is
played (v19.2.0, the user's ruling).** Under role inversion a clue to Cathy is
stable *because* Bob holds a call, and the two can name the same card: Bob's
`{p1}`, then a Purple to Cathy read on the same stacks, `{p1}` again. What
Cathy's card is depends on what Bob does next. Bob acts before Cathy, so she
always knows:
- **Bob plays his called card, every seat can name it, and hers could be that
  card.** Hers builds on it. That identity is replaced by the next one up its
  suit, if her card can be that: `{p1}` becomes `{p2}`, and the call stands.
  Bob's call must have been stamped by a clue, and its common reading must be
  **one** identity. Cathy cannot build on a card she cannot say. Self-play under
  the looser "the readings overlap" rebased a `{y1}` call to `{y2}` after a
  played `{r1,y1,g1,b1}` that was the r1. It lost 0.25 points a game over 300
  seeds; the named form is neutral there (31 = 31 perfect games, 95 = 95
  strikeouts).
- **Bob plays an unrelated card, or throws his copy.** Hers is played on the
  stacks as they were, `{p1}`.
- Either way, Cathy is meant to play the card just clued.

`rebase_calls_on_a_played_call` (`src/conventions/tiiah/superposition.cpp:1656-1695`)
applies this to every stable (non-urgent) call stamped after the played card's
own. `Game::handle_action` calls it before the play is dispatched
(`src/basics/game.cpp:648`), while `common` still holds the played card's
reading. Without it, rule 3 of the call invariants erases the overlapping call
as dead the moment the first copy lands (reactor0 CONVENTION.md §1h). It reads
only `common` and the stamps, so every seat rebases alike. The giver side is
unchanged: `calls_two_copies_to_play` still keeps the bot from giving such a
clue itself.

Replay [2017459](https://hanab.live/shared-replay/2017459):
- **T1:** yagami's Purple called will-bot67's o9, the p1.
- **T4:** her Purple to will-bot69, stable by role inversion, called o13 `{p1}`.
- **T5:** will-bot67 played its p1. will-bot69's call was erased, and it gave a
  clue. Rebased, o13 is the p2, and will-bot69 plays it.

`tests/test_tiiah/test_replay_2017459_stable_call_rebases_on_bobs_played_call.cpp`;
`tests/test_tiiah/test_rebase_on_played_call.cpp` covers the play, the throw and
an unrelated play.

How the position has been read:
- **Through v16:** a called card with one playable identity left, or an
  all-playable empathy, judged on the seat's own stacks. A seat whose belief had
  blue on 2 could count Bob's called b3 as a known play while the others, with blue
  on 1, did not.
- **v17.4.0:** every standing call.
- **v18.0.0 to v18.2.0:** a touch-known play only, because a reacter's or a
  receiver's call can be stamped at some seats and not others. That turned the
  reverse reactive off in practice, since a stable colour clue calls a card whose
  touches still allow unplayable identities. v18.2.0 restored standing calls for
  role inversion alone.
- **v18.3.0 (now):** every clued standing call, for both halves. Two cases
  motivated this:
  - *Human diagnostic 2013726 T30* (`v18_human_vs_bot_diagnostics/2013726.md`).
    Black held the r4 that blue's T27 Red had called, touched as r1–r5. A human
    gives Brown to black: a reverse-reactive finesse of green's n3 into black's n4.
    Under the touch-only test the position never held, and blue gave a stable 3.
    v16.29.0 gave the Brown.
    `tests/test_tiiah/test_replay_2013726_reverse_reactive_finesse_on_a_called_play.cpp`.
  - *Replay [2013645](https://hanab.live/shared-replay/2013645#11) T11.* yagami's
    T8 Blue called will-bot69's o12 as the b3, touched as b1–b5, and yagami's 4 to
    will-bot67 was read as an ordinary reactive. will-bot69 blind-played its o14 as
    the reaction; it was an r3, and struck. By role inversion the 4 is stable: it
    calls will-bot67's chop to discard, and will-bot69 plays its b3.
    `tests/test_tiiah/test_role_inversion.cpp`,
    `tests/test_tiiah/test_replay_2013645_role_inversion_keeps_the_clue_stable.cpp`,
    `tests/test_tiiah/test_replay_2013645_stable_four_under_role_inversion.cpp`.
- **The self-play cost.** Seeds 1–300 at 6 s, v18.2.0 → v18.3.0: 25/25 35 → 34,
  cards ever read wrongly 383 → 422 per 100 games, strikeouts 74 → 88. Counting
  every call, unclued ones included, was worse: 27/300, 436, and 95. It was shipped
  by ruling, as the diagnostic asks for it, and which reverse reactives go wrong
  in self-play is still open.
- **v18.10.0 (now):** clued calls and **settled** calls, i.e. any call no longer
  urgent. A receiver's call whose reaction has been played is settled.
  - *Replay [2013963](https://hanab.live/shared-replay/2013963#10) T10*
    (`v18_human_vs_bot_diagnostics/2013963.md`). will-bot67's only call was an
    unclued, settled receiver's call on o17. yagami's 1 to will-bot67, a
    reverse-reactive finesse of will-bot69's b2 into will-bot67's b3 on the
    `10011` the giver and the reacter share, read as a MISTAKE.
    `tests/test_tiiah/test_replay_2013963_reverse_reactive_finesse_on_a_settled_call.cpp`.
  - *Replay 2009367 T9* reverts to its pre-v18.0.0 answer, a reaction with the b1
    (o16), because yagami's called b1 is a settled call.
  - *Self-play*, seeds 1–300 at 6 s, v18.9.0 → v18.10.0:

    | | 25/25 | Cards read wrongly (per 100 games) | Strikeouts | Dispatch disagreements (per 100 games) |
    |---|---|---|---|---|
    | v18.9.0 | 39 | 473 | 118 | 177 |
    | v18.10.0 (clued or settled, fix first) | 35 | 485 | 114 | 188 |
    | every call, pending ones included | 34 | 495 | 118 | 191 |

    Counting every call was worse on every measure.

The ordinary square is reactor0's rule unchanged, and it was **missing until
v16.8.0**: the dispatcher only ever added the reverse arm, so every clue to Cathy
fell through to the stable ladders. Replay
[2008177](https://hanab.live/shared-replay/2008177#4) T3 is what that cost — a
rank 2 that named a double play read as a lock, and the reacter discarded.
Whichever way the clue goes, it is read by the same §1d rules: even parity, the
sum rule, and the buckets.

`tiiah::interpret_clue` (`src/conventions/tiiah/interpret_clue.cpp:376-461`) is
the table: one `if` for each reactive square, and every other square falls
through to the stable ladders.

**A sure play goes when the hole could hold its dupe** (confirmed for replay
[2019249](https://hanab.live/shared-replay/2019249) T2, v20.12.0). will-bot67's 1 had
touched two of will-bot69's cards. With nothing in the hole every 1 was a sure play,
so yagami's 4 to will-bot67, his Cathy, read as a stable discard: will-bot69 was
loaded with a safe action. Had a 1 already gone into the hole, those 1s would not be
safe (possible dupes), and the same 4 would be a reactive. This is the v20.3.0
sure-play test (`is_standing_play`), unchanged.
`tests/test_tiiah/test_dupe_ones_flip_the_position.cpp`.

#### The deferred read: a clue whose position rests on a reaction owed to us (v20.12.0)

The user's ruling. A clue to Bob is a reverse reactive with us reacting when Bob holds a
standing play. Sometimes Bob still owes **us** a reaction. The card he will answer with
is fixed by our own target, which we cannot see, so we cannot tell whether he holds a
standing play, or which. **So we wait.**
- **At the clue** (`interpret_clue.cpp:378-437`), the clue is deferred when all of
  these hold:
  - it goes to Bob, and we are the one who would react;
  - Bob owes us a reaction (`pending_reactions`);
  - neither Bob nor we hold a standing play.

  We record a `Game::DeferredRead` (`include/hanabi/basics/game.h:334-341`) and stamp
  nothing.
- **When Bob next plays or discards,** that settles his reaction to us. `Game::handle_action`
  then rewinds to the clue (`src/basics/game.cpp:703-737`, with `Game::rewind`, as
  reactor re-reads a clue its reaction explains). The card a PLAY pressed, a strike
  included, is counted as his standing call: stamped `CALLED_TO_PLAY`, not urgent, on
  the position and on the game. The clue is then read as the dispatch reads any clue.
- **A discard** leaves no standing play, so the clue reads as whatever it is in that
  position.
- The record survives the rewind, which is what tells the replayed clue not to wait
  again.

The seats that can name Bob's owed card read the clue at once. They hold it as a
settled call already, once its target has left the receiver's hand.

**When the clue to Bob was a FIX of his owed reaction card** (the user's ruling,
self-play seed 259), Bob either throws the fixed card at once or clues. Either way
the original reactive is off.
- **The fix stamps a discard.** A fix of a reaction call (§1h) stamps an urgent
  `CALLED_TO_DISCARD` on the card instead of only withdrawing the call
  (`interpret_clue.cpp:487-507`, `ConvData::fixed_reaction`), so the reacter throws
  it next.
- **A discard settles the read as the fix.** The waiting receiver reads the clue as
  FIX (`interpret_clue.cpp:409-423`), and Bob's reaction to it is void.
  - The identity Bob threw was already played, by a hole card that admits it.
    `tiiah::team_learns_a_hole_card_was` (`superposition.cpp:880-896`) settles that
    card, or records the joint fact for the worlds.
  - Seed 259: Cathy had played her o10, the b1, into the hole, so Bob's owed o7, also
    a b1, was dead and Alice's Green fixed it.
- **A clue from Bob turns the reaction off** (`src/basics/decide.cpp:258-282`).
  - The seats that saw the fix drop the card's discard call.
  - The waiting receiver learns that the clue was no reverse reactive, since its
    receiver would have had to play, and stops waiting.
  - All of them retire the reaction Bob owed.
  - Until v20.12.0, Bob kept the reaction owed and played an unrelated r1 at T8,
    which Cathy took for his standing call.

Replay [2019249](https://hanab.live/shared-replay/2019249) T4:
- will-bot67's Yellow to yagami paired will-bot69's o15 with yagami's o9, the r2: the
  leftmost playable once yagami's owed T1 reaction, the r1 (o7), had played.
- will-bot69 had been that T1 reaction's receiver. It could not name the o7, read the
  Yellow as a stable MISTAKE, and pitched o11 at T6.
- It now waits. At T5, yagami's o7 settles the read, and o15 is called
  `{g1,g2,b1,b2}`, as will-bot67 reads it.

`tests/test_tiiah/test_replay_2019249_receiver_of_owed_reaction_waits.cpp`,
`tests/test_tiiah/test_deferred_read.cpp`.

#### The refusal: Bob says the card is already played (v16.14.0)

Alice reads her own reactive against **her own** stacks, and those are stale in
exactly one way — she cannot see what she threw in the hole. So she can name a
card of Cathy's that is already down, and neither she nor Cathy can tell. Bob
can. The convention gives him a way to say so:

> **Alice gives Cathy an ordinary reactive, and Bob answers by giving Cathy any
> STABLE clue ⟹ the card Alice named is already played.** Alice collapses the
> superposition that must have been it (§1e rule 5).

Read at `read_refusal` (`interpret_clue.cpp:201-225`), ahead of the dispatch
table because it has to pre-empt it: Bob's clue to Cathy is `giver=bob,
target=cathy`, which the ordinary row would otherwise read as a fresh reactive.

Four things this rests on:

- **Stable is the discriminator.** A reacter who clues instead of reacting is
  normally *deferring*, and a deferral carries the reactive intent forward — so
  it is itself reactive. A refusal is stable. Without that test the two are the
  same event, since the engine already lets any clue by the reacter clear the
  waiting connection (`basics/decide.cpp:251-256`), which is also why the arm
  reads `prev`.
- **It is an ENVELOPE.** The signal is *that* the clue was stable and aimed at
  Cathy, so the clue still means whatever stable clue it is, and control falls
  through to the ladders. Replay 2009367 T7's refusal is also a lock.
- **Bob's own call stands.** His card was named because it is playable in the
  right bucket, which is still true whatever Cathy holds; only Cathy's half of
  the pairing is void.
- **Ordinary reactives only**, as ruled. Under the reverse the receiver moves
  first and there is nothing to refuse yet.

**The bot gives it too**, because a human partner can name a dead card just as
easily. `clue_refuses_dead_target` (`reactor0/decision.cpp:862-885`) marks the
candidates, and they join **Precedence step 1** — `choose_very_high_clue` — rather
than a rung: refusing is done *instead of* reacting, and every rung sits below the
urgent return, which is the very thing being declined. The tier itself
(`clue_tier`) is left alone so that no non-hole game can see any of this.

Two more things make it actually get given (v16.27.0):

- **The tier gate does not apply to it** (`clue_is_admissible`,
  `reactor0/decision.cpp:1214`), as it does not to the fix (§1h). A refusal stamps
  nothing, so it is always LOW, and an OCCUPIED reacter — which a reacter holding
  the urgent call always is — had every refusal rejected before its priority was
  consulted. Replay [2011854](https://hanab.live/shared-replay/2011854#27) T27:
  yagami's Rank 3 named will-bot69's o29, a p1, while his own o24 p1 was already in
  the hole; will-bot67 had five refusals on offer, the gate dropped them all, it
  answered the reaction, and will-bot69 struck at T28.
- **Which refusal** (`refusal.stable_play`, `reactor0/decision.cpp:2208-2218`). Any
  stable clue to the receiver will do, so the one chosen should also be worth giving:
  a stable PLAY clue, ranked by §2a's stable play hierarchy (`settle_stable_play`, the
  same tiebreak every stable-play rung uses; before v18.17.0 its single term, "names
  its card"). Before v16.27.0 the default tiebreak alone decided, and at T27 it took
  a Rank 5 lock over the Purple that names will-bot69's p2.

**Reading it with several candidates** (v16.27.0). The refused card is down, and one
of the giver's hole cards put it there — but when more than one of them admits it,
the refusal does not say which. §1e rule 5 then records the joint fact rather than
settling one; see there.

[2009367](https://hanab.live/shared-replay/2009367) T6–T7 is the case.
will-bot67 threw a `{r2, p1}` into the hole at T3; it was the `p1`, so its stacks
still read purple 0 and its T6 reactive named will-bot69's slot 4 — another `p1`,
a duplicate. yagami could see the first one go down, and at T7 gave will-bot69 a
rank 4. Before v16.14.0 that was a lock and nothing else: will-bot67 went on
believing it might have played the `r2`, and at T9 played a `y3` into a strike.
Now it settles on the `p1`, and T9 is a playable `b1` instead.

Under the REVERSE arm, Bob's target is the next playable in his hand under
stack simulation, where every known play in his hand is assumed already
played. Worked example: Bob holds
`[r3] r5 y1 p5 g5` with the `r3` called to play and all 2s on the stacks. Once
the `r3` is played the r4 is next, Bob does not hold it, so Cathy is called onto
the r4 as a finesse and Bob's `r5` is the target.

**A card that is already CTP is never retargeted** — reactor's rule, which
reactor0 deliberately reversed and which TIIAH restores. It has one exception
(v18.5.0), for every reactive, ordinary or reverse.

When every playable and finesse target in the receiver's hand is already
**gotten** (called), the walk runs again as if nothing were. It takes the leftmost
called playable, then the leftmost called finesse target, before the double-chuck
fallback, as outside TIIAH (`receiver_targets`,
`src/conventions/tiiah/interpret_reactive.cpp:247-301`).

On the reverse arm's frame the receiver's own called plays are already
simulated, so those cards read as played and never qualify.

The motivating case is human diagnostic 2013726 T38
(`v18_human_vs_bot_diagnostics/2013726.md`). Black's only target is a g4 an
earlier reactive already called, and a human gives Brown to black to get blue's n5
against it (`tests/test_tiiah/test_all_targets_gotten.cpp`).

**The sum rule applies here exactly as to an ordinary reactive clue**
(§1d): `react_slot + target_slot ≡ anchor (mod hand size)`, with Cathy's slot as
the react slot and Bob's as the target. The walk above finds Bob's target; the
sum rule then fixes which of Cathy's cards is called.

Dispatch is decided by the position **before** the clue. Asking the post-clue
game instead makes every play clue to Bob answer "Bob has a known play", because
the clue itself just gave him one.

`interpret_reactive` (`src/conventions/tiiah/interpret_reactive.cpp:311-784`)
installs the waiting connection, stamps the reacter's blind play and leaves the
receiver's own call for reaction time — the resolution machinery is reactor0's,
shared.

**The target walk** (`receiver_targets`) runs over the **frame** — the minimum
across every world the reacter can live in, from the giver's perspective
(`reacter_frame`, §1e) — as it will stand *when the reacter acts* (`reacter_faces`,
`src/conventions/tiiah/interpret_reactive.cpp:67-71`). Which stacks those are is
the one thing the two arms disagree about, and it follows from who moves
first:

- **reverse** — the receiver moves first, playing the known play that made the
  clue reactive, so his queued plays are simulated in
  (`stacks_after_queued_plays`, a fixpoint so a chain advances in order);
- **ordinary** — the reacter answers on his very next turn and nobody has
  moved, so the simulation stands down and the shared stacks are the answer.

Two kinds of card qualify, and they are walked in this order — reactor0's Phase A before Phase B, which is
the order every seat walks:

1. one that plays outright on those stacks;
2. one that is **one away**, which is a finesse: the reacter holds the bridge.
   §1c's own worked example is this case.

The giver walks the candidates and takes the first whose reacter side works. A
pairing refused on SHARED knowledge is walked past, because the reacter walks
past it too; one refused on what only the GIVER can see kills the clue (§1g),
because the reacter cannot see it and would act on that pairing anyway.

A clue whose walk finds nothing reads as a MISTAKE and stamps nothing. Guessing
would be worse than refusing: it would call a partner onto a card nobody named.

**A call here can be DELAYED.** The finesse arm names the reacter a card that is
only playable once the receiver has played what they already know, so the call
stands while being unactionable. Reactor0's dead-call invariant would otherwise
erase it the moment it was stamped, and under this variant it judges "dead"
against the stacks after the queued plays as well as the live ones
(`drop_dead_play_calls`, `src/conventions/reactor0/call_invariants.cpp:178-217`).
The call under test is **left out** of that simulation: counting it would spend
its own identity, and it would read dead exactly when it is most alive
(`stacks_after_queued_plays`'s `except_order`,
`src/conventions/variants/hole.cpp:9-58`). Nothing follows for the holder's turn
— the call is alive, not yet actionable.

**A call is also alive in a WORLD** (v16.29.0). A reading made over the hole's
worlds (§1d's receiver half, §1e's stable re-read) can name, say, the `r3` if our
hole card was the `r2` and the `y3` if another was the `y2`. Our belief is the
**minimum** across those worlds, so it can find no identity in the reading
playable while every world has one. Before rule 3 calls a play call dead it
therefore also asks whether any strike-free world of the **shared** view, meaning every
seat's hole cards as the team reads them, makes one of the card's identities a
valid pitch. If one does, the call stands
(`src/conventions/reactor0/call_invariants.cpp:200-215`, with
`pitch_candidates_in_shared_worlds` at `:144-158`).
The shared worlds rather than our own, so the answer does not depend on the
seat. Replay [2012424](https://hanab.live/shared-replay/2012424) T33: will-bot69's
p1 called will-bot67's o24 as `{r3,y3}`, on a belief of red 0 and yellow 1 that
the play's collapse had not yet raised. Rule 3 erased the call, and at T35
will-bot67 discarded its chop instead of playing the r3
(`tests/test_tiiah/test_replay_2012424_reaction_call_live_in_a_world.cpp`).

**A newer call never erases a call standing to its left (v20.11.0, the user's
ruling).** reactor0's rule 1 (`enforce_play_order`,
`src/conventions/reactor0/call_invariants.cpp:57-112`) erases every older play call
in a newer slot than the most recent one, since a later clue would not have pointed
past a card still playable. Here that premise is false. The target walk passes over
a standing call because the card is already gotten, not because it is dead. So rule
1 is off in this variant (`:58-65`), for receiver and reacter calls alike, and a
call dies only by rule 3. Several calls can then stand out of slot order; the holder
actions them most recently stamped first, as reactor0 already allows.

Replay [2018874](https://hanab.live/shared-replay/2018874):
- T46: yagami's Blue stable-called will-bot69's o44, the b5 in slot 1.
- T48: will-bot67's Green passed over it, as already gotten, to finesse yagami's r4
  into will-bot69's o23, the r5 in slot 5.
- T49: when yagami reacted, rule 1 erased o44's call. Its `{b5}` inference stayed,
  but the call was gone.

`tests/test_tiiah/test_replay_2018874_standing_call_survives_a_new_call_right.cpp`,
`tests/test_tiiah/test_standing_call_survives_and_plays.cpp`.

### §1d The reactive clue

All reactive clues are **even parity**: whichever button the reacter presses, the
receiver is called to the same one (`wc.even_parity = true`,
`src/conventions/tiiah/interpret_reactive.cpp:332`). The two slots are picked by
the sum rule as in reactor0 — `react_slot + target_slot ≡ anchor (mod hand size)`
(`interpret_reactive.cpp:522-523`) — and **the anchor is reactor0's**: the rank value
for a rank clue, and the colour's value from the fixed table
(`include/hanabi/conventions/reactor0/colour_value.h`) for a colour clue
(`anchor_of`, `interpret_reactive.cpp:31-37`).

The clue KIND then carries what the two cards ARE, which is the part a hidden
stack cannot otherwise convey:

- a **rank** clue means a finesse, *or* the receiver's target sits one bucket
  **higher** than the reacter's card (wrapping);
- a **colour** clue means a finesse, *or* one bucket **lower** (wrapping).

**The finesse and the bucket relation are disjoint by definition**, so a clue is
never both and there is no precedence between them to settle. `required_target_bucket`
/ `bucket_relation_holds` (`src/conventions/tiiah/interpret_reactive.cpp:77-91`)
are the relation; the finesse is a target one away, whose *connector* is the one
card that bridges to it (`interpret_reactive.cpp:538-552`).

What the reacter writes down is therefore one of three things
(`interpret_reactive.cpp:536-756`):

| The pairing is | The reacter's card is read as |
|---|---|
| a finesse | exactly the connector |
| a bucket relation | the **playable** identities of the named bucket |
| neither | whatever the stamp left — the superposition below |

"Playable" in the middle row is judged **after the receiver's queued plays**, the
same stack simulation §1c's target walk runs on, so a card that only comes live
once the receiver plays what they already know counts. Worked example: red on 2,
six suits, the receiver holding a known `r3` and the target a green card under a
rank clue. Green is bucket 1, so the reacter's bucket is 0 — red and yellow — and
what they write is `{r4, y1}`: the `r4` because the queued `r3` will have gone in
first, the `y1` because yellow has not started. They cannot act on the `r4` until
that `r3` actually plays, which is what makes their call a **delayed** one.

Worked example, six suits, red/yellow/green/blue/purple/teal: a rank clue can get
a teal 1 to play into a teal 2 (a finesse), or a purple/teal card to play into a
yellow 1 — purple and teal are bucket 2, yellow is bucket 0, which is one higher
wrapping.

#### The reacter's own card: the bucket first, in its own worlds (v20.10.0)

The bucket row above is read over the reacter's own open worlds, not only on the
frame: `bucket_over_worlds` (`interpret_reactive.cpp:125-147`), called in the
`own_away == 0` branch (`:711`). **That reading replaces whatever the stamp wrote** when
any identity the card could be **before the stamp** lies in it
(`interpret_reactive.cpp:714-754`):
- The test set is the card's pre-stamp reading (`old_inferred ∩ possible`, else its
  possibilities in `prev`). `stamp_react_play_button` has already narrowed the card to
  the playables of ONE state, usually the frame, which can be the other buckets'
  playables.
- When the bucket reading plays in some worlds and not others, the worlds collapse to
  those, shared, as ASCR does (§1e). The reacter is being called to play a card that
  plays only there.
- The tiers are never mixed: the stamp's other-bucket playables stand only when no
  bucket identity plays in any world.

Replay [2018857](https://hanab.live/shared-replay/2018857#17) T17 (yagami's 5 to
will-bot69, a reactive with will-bot67 reacting): the target o23 is the g2 (bucket 1),
so a rank clue names bucket 0 for will-bot67's o12. o12 had negative yellow, so its
bucket-0 reading is `{r2}`, playable in the worlds where its own o11 `{r1,g2}` or o20
`{r1,ra1}` was the r1. Until v20.10.0 the bucket test ran against the stamped
`{g2,b2}`, found nothing in common with `{r1,r2,y2}`, and left o12 noted as `{g2,b2}`,
the target's own bucket. It now reads `{r2}`.

#### ASCR in the walk: each target is tried in the worlds before the next (v20.6.0)

The walk applies the **Actionable Superposition Collapse Rule** (§1e) per target.
The walk runs on a frame that is a **minimum** over the worlds of the hole cards
(§1e), so a pairing that fails on it may work in a world some hole card leaves
open. Two shapes of failure qualify:
- the reacter's card cannot play on the frame;
- the target is one away, and the reacter's card cannot be its connector.

Before walking on to the next target, every seat that walks (giver, reacter,
outside) asks whether some strike-free world lets **this** target play, after the
receiver's queued plays on a reverse reactive, **and** the reacter's card play there.
- If so, this is the pairing. The reacter's card reads as the playables of the
  bucket the relation names across those worlds, or failing that any playable.
- The worlds **collapse** to them (`collapse_to_worlds`, shared), which settles the
  hole cards and moves every stack view.
- Only when no world works does the walk go on.

The collapse comes **before** the reacter's call is stamped, because the stamp
narrows the card to what plays on our stacks. The §1g giver-sight rejection (the
card the giver sees must work in some world) and the giver's bucket legality test
apply as in the walk. `ascr_pairing` (`src/conventions/tiiah/interpret_reactive.cpp:402-513`,
called at `:539` and `:553`) over `ascr_find`; branch `tiiah.ascr`, site `walk`.

Human diagnostic [2018541](../../../v18_human_vs_bot_diagnostics/2018541.md) T26:
- will-bot67 had thrown o16 `{y3,r3}`, the r3, into the hole at T23. On the frame it
  shares with yagami, red was 2 and blue 3.
- Yagami's 1 to will-bot69 walked the b4 first, paired with will-bot67's o6
  `{r4,ra4}`. That card cannot play on red 2, so the walk went on to the g2, called
  o26, and will-bot67 struck.
- In the world where o16 was the r3 the r4 plays. The b4 pairing is the reading,
  and will-bot67 plays the r4.

`tests/test_tiiah/test_replay_2018541_ascr_reacter_card_plays_in_a_world.cpp`,
`tests/test_tiiah/test_ascr_walk.cpp`.

**History: the v19.3.0 world fallback.** Until v20.6.0 this ran only when **no**
pairing read on the frame, and only for one-away targets directly playable in a
world. The per-target rule above subsumes it. The replay and measurements below are
that version's.
branch `tiiah.reactive_world_fallback`.

Replay [2017491](https://hanab.live/shared-replay/2017491):
- **T12:** will-bot67 reacted with o18, the b1, named only privately, so every other
  view kept blue on 0.
- **T13:** will-bot69's 2 to yagami, who held a standing p1, was a reverse reactive:
  will-bot67's slot 4 (o12, the r3) into her slot 3 (o7, the b2).
  - On blue 0 the b2 is one away. As a finesse it needs o12 to be the b1, which its
    3 clue rules out, so will-bot67 read the clue as unreadable and discarded.
  - In the world where o18 was the b1, the b2 plays. o12 reads `{r3}`, the worlds
    collapse to that one, and blue is on 1 in the common view and both rows, and
    will-bot67 plays o12.
- (By the user's note a refusal would also have been right, since the b2 is
  duplicated by will-bot69's called o15. The bot reacts instead.)

The collapse reaches **every** hole card, by the user's ruling. The cost is measured
here, so it can be revisited: on TIIAH 5 Suits (seeds 1–300, against v19.2.0)
- perfect games 31 → 34, mean 19.58 → 19.49, strikeouts 95 → 100;
- hole cards wrongly named from reactive clues 45 → 64. When the true world lies
  outside the model, the kept worlds can settle cards the clue says nothing about.
  Self-play seed 100 T7 is an example: a red target settled an unrelated g1 as a b1.
- A collapse limited to the target's suit measured 34 perfect, 19.56, 99 strikeouts
  and 58 namings, and was not chosen.

`tests/test_tiiah/test_replay_2017491_reverse_reactive_world_fallback.cpp`;
`tests/test_tiiah/test_reactive_world_fallback.cpp` (and its control, where no world
explains the target).

#### The receiver world fallback: a reaction read in the worlds (v20.5.0)

The receiver's half of the **Actionable Superposition Collapse Rule** (§1e), on the
same shared `ascr_find` since v20.6.0. At the
reaction, reactor0's shared stamp reads the receiver's target on the frame the
giver and the receiver share. That frame is a minimum over the hole's worlds, so it
can find nothing playable and fall to its bluff or mistake reading although the
giver meant a plain pairing. Before that stands, every seat that watched the
reacter's card reads the receiver's slot in the worlds of the hole cards (the same
worlds as the receiver's ordinary reading, below):

1. **The bucket rule first.** The bucket half and the finesse half of §1d, as
   `receiver_reading` gives them. The reading is the target's identities that one of
   them names and that **play in some world**.
2. **Otherwise any one-away reading.** The target's identities that are one away on
   the frame, bucket or not, and play in some world.
3. Otherwise the stamp's bluff or mistake reading stands.

A reading is refused when **one identity of it holds in every world**. It then does
not rest on the hole, and the stamp failed for some other reason (replay 2011885
T10: a `{b3}` playable in both worlds).

A reading whose identities each need some worlds is taken, even when together they
cover them all (v20.8.0). Nothing collapses then, and the conditional half is
recorded so a later fact can settle it. Replay
[2018766](https://hanab.live/shared-replay/2018766):
- yagami's T21 Green made will-bot69 the receiver, with will-bot67's b3 reacting.
- At will-bot69's own seat its hole card o7 `{r1,y1}` was unnamed, so the stamp found
  nothing o9 could play and dropped the call.
- In the worlds, o9 reads the r2 where o7 was the r1 and the y2 where it was the y1.
- The old guard refused that as covering every world, and the reading "reset". The
  call on `{r2,y2}` now stands. The target is narrowed to the reading and called to play, and the
worlds **collapse** to those that make it (`collapse_to_worlds`, shared), settling
the hole cards. The REACTER keeps the stamp's reading: it cannot name what it
played, and a fallback read off its inference would collapse on a guess (replay
2015109 T9). `receiver_world_fallback`,
`src/conventions/tiiah/interpret_reactive.cpp:1030-1140`, called from
`narrow_receiver_call` when no call was stamped (`:1165`); branch
`tiiah.receiver_world_fallback`.

Replay [2018517](https://hanab.live/shared-replay/2018517):
- **T18:** will-bot67 threw o21 `{r2,y1}` into the hole unnamed. It was the r2.
- **T19:** yagami's 3 to will-bot67 was a reactive: will-bot69's m3 into will-bot67's
  slot 1, o23, the r3. The m reacts into the red/yellow bucket.
- **T20–T21:** on the shared frame red was 1, so the stamp read a bluff and
  will-bot67 discarded.
  - In the world where o21 was the r2, the r3 plays (step 1). o21 settles as the
    r2, red goes to 2 for every seat, and will-bot67 plays the r3.

Replay [2018535](https://hanab.live/shared-replay/2018535):
- **T5:** will-bot67 threw o9 `{r2,ra2}` into the hole unnamed.
- **T6–T7:** yagami's Green to will-bot67 was answered by will-bot69's y1.
  - A colour clue names one bucket down, the rainbow bucket. On the frame's rainbow
    1 the m3 is one away.
  - In the world where o9 was the m2 it plays, so will-bot67's o8 reads `{m3}` and
    is called (step 1, the user's "finesse path").

`tests/test_tiiah/test_replay_2018517_receiver_world_fallback_collapses_hole.cpp`,
`tests/test_tiiah/test_replay_2018535_receiver_finesse_through_a_hole_card.cpp`,
`tests/test_tiiah/test_receiver_world_fallback.cpp` (step 1, step 2, and a control
with no hole card).

#### The receiver reads it too

**THE FRAME: the giver's and the receiver's (v16.18.0).** Before any of the
readings below, the call has to be about a card that is *playable* — and playable
on which stacks is a question §1.3 answers. The promise is about the receiver's
card, so it binds to the row the giver and the receiver share:
`ReactorWC::clue_play_stacks`, taken from
`State::stacks_known_to_both(giver, receiver)` at clue time
(`src/conventions/tiiah/interpret_reactive.cpp:352`) and rewound onto by
`reactor0::stamp_receiver_call` (`reactor0/interpret_reaction.cpp:365-390`).

Until v16.18.0 it was the SHARED view, which is what all three seats know — so one
seat's ignorance priced the promise for the whole team. Replay
[2010329](https://hanab.live/shared-replay/2010329#10) T14 is what that cost.
yagami rank-2s will-bot67 and will-bot69 answers with a `y2`; will-bot67's called
card was a `b2` and playable. Read against the shared `[1,0,0,0,0]` the promise came
out `{r2,y2}` — red was on 3 and yellow on 2 by then, so both were trash, and
`stamp_receiver_call`'s own Rule 5 correctly dropped the whole call as a stale
reading. The card kept its bare rank-2 empathy and will-bot67 discarded its chop.
Against the pair's row the promise is `{g2,b2}` and the call stands.
`tests/test_tiiah/test_replay_2010329_the_frame_is_the_pair_not_the_team.cpp`.

**Rule 5 now reads only a DEFERRED reaction** (v16.28.0,
`reactor0/interpret_reaction.cpp:414`). The frame is a view two seats share and the
live stacks it was vetted against are our own belief, so on the undeferred path the
two can be disjoint with nothing stale about either — and the call was dropped before
`narrow_receiver_call` could read it. Replay
[2011887](https://hanab.live/shared-replay/2011887#19) T19: will-bot67's b2 called
will-bot69's o21, the g3; on the shared frame (0013) the promise was `{g1,b2}`, on
will-bot69's own stacks (1123) `{r2,g2,b3,n4}`, so Rule 5 dropped it and at T20
will-bot69 discarded chop. It now plays the g3, read `{g2,g3,b3}` over its own
worlds. (The two cases above lost their calls the same way; the frame fixes they
describe stand on their own.)
`tests/test_tiiah/test_replay_2011887_undeferred_call_not_dropped.cpp`.

**...and the call invariants wait for it too (v18.14.0).** On the live path the
TIIAH reading (`record_reaction`, `narrow_reacter_play`, `narrow_receiver_call`) now
runs straight after `reactor0::react_play` and **before**
`reactor0::enforce_call_invariants`, as it already did on the deferred path. The
code is in `src/basics/decide.cpp:710-759`. The shared stamp reads the call on the
pair's frame plus the reacter's card. That frame can lag the reacter's card, so the
stamp can name the very card the reacter just played. Rule 3 then erased it as dead
before the finesse half could read the card after it.

Replay [2014402](https://hanab.live/shared-replay/2014402#26) T26 is the case:
- black's i2 answered blue's 4 on green's o10, the i3;
- on the pair's pink 0 (green's T24 i1 was a superposition there) the stamp read
  `{i2}`, and rule 3 erased it with a `[reset]`;
- at T27 green threw the i3 away as a dead i2.

It now reads `{i3}` and plays it.
`tests/test_tiiah/test_replay_2014402_finesse_read_before_dead_call_check.cpp`.

**And the frozen frame is CORRECTED when we name one of our own hole plays
(v16.21.0).** Freezing it is v12.0.0's rule and stands — a deferred reaction must be
read as it was meant. But the frozen vector is our *estimate* of the stacks the giver
chose the target in, and our own hole plays are the one thing that estimate can be wrong
about: the giver could see the card all along, so their frame already counted it and only
ours did not. `correct_frozen_frames`
(`src/conventions/tiiah/superposition.cpp:256-267`), called from `settle`'s own-card
branch, raises `clue_play_stacks` on every live `waiting` and `pending_reactions` entry
for the identity just named. Raises only: this restores what was frozen rather than
moving it.

Replay [2011133](https://hanab.live/shared-replay/2011133#17) T16–T17 is the case, and
the arithmetic is worth writing down because it turns on one subtlety. will-bot67's frame
froze with blue on 0, because it could not yet name the `b1` it threw at T6. At T17 §1e
rule 6 named it — but `prev` is snapshotted in `Game::handle_action` **after**
`presume_play_lands`, so the collapse is already inside it and the reacter's advance adds
only `+1`. The promise therefore came out `{b2}`, already played, and Rule 5 dropped the
whole call as stale; order 14, a `b3`, was never called and never played. With the frame
corrected the promise is `{b3}` — what the card is.
`tests/test_tiiah/test_replay_2011133_rule_six_is_shared.cpp`.

The relation has two ends, and the receiver reads theirs when the reaction
resolves — not at clue time, because until the reacter acts they do not know
which of their cards the sum rule names. Their card is the **union** of the same
two readings, intersected with what it could already be
(`narrow_receiver_call`, `src/conventions/tiiah/interpret_reactive.cpp:1170-1276`,
called from the engine seam at `src/basics/decide.cpp:725-759`):

- **the bucket** the reacter's sits one step from — one *lower* for a rank clue,
  one *higher* for a colour one, the relation above read backwards;
- **the continuation** of the card the reacter played — the next card up its suit,
  which on a reversed suit is `prev()` rather than `next()`, and which is what a
  finesse leaves behind.

Both halves are read **in every world the receiver's own hole cards leave open**
(§1e), as the reacter's half has been since v16.13.0. Until v16.17.0 this one read a
single stack vector, so a candidate playable only in some other world was never
offered. Replay [2010329](https://hanab.live/shared-replay/2010329#10): will-bot69
had thrown an `{r2,g1,b1}` into the hole and its called card was an `r3` — playable
only where that card was the `r2` — and it read `{g2}`. The per-candidate support is
recorded, so a later collapse withdraws the conditional part.

The baseline it narrows from is `prev`'s inference, not `old_inferred`: unlike
`reactor::target_play`, `stamp_receiver_call` writes through `narrow_thought` and
leaves no `old_inferred` to roll back to, and without a rollback `narrow_thought`
can only ever intersect *inside* the stamp's single-frame set — so the wider reading
could never land.

The same baseline is what the "never empty the card" guard checks against (v16.26.0):
the reading is skipped only when it misses everything the card could be **before** the
stamp (`prev`'s inference ∩ `possible`), never merely because it misses the stamp's own
set. That set is the bucket-blind, single-frame reading this one replaces, so the two
are routinely disjoint whenever the receiver knows more than the pair does. Replay
[2011830](https://hanab.live/shared-replay/2011830#16) T16: will-bot69 had named its own
hole card o7 as the `g3` (it could see both `y3`s), so its stacks had green on 3 while
the pair frame had green on 2. yagami's `p2` into a Purple clue called o15 (bucket 1: the
`g4` or the `b1`; the `b1` was ruled out); the stamp had written the pair frame's
playables `{r2,y3,g3}`, and the old guard compared `{g4,b1,p3}` with *that*, found
nothing in common, and kept the stamp. o15 went into the hole as `{r2,y3,g3}`, green
stayed at 3 in one of its worlds, and at T23 will-bot69 refused the `g5` it had been
called to play and discarded chop. `tests/test_tiiah/test_replay_2011830_receiver_bucket_reading_not_the_stamp.cpp`,
`tests/test_tiiah/test_receiver_reading_beats_the_stamp.cpp`.

Replay [2008217](https://hanab.live/shared-replay/2008217#2) T2 is both halves at
once. Alice clues Blue to Cathy; Bob answers on slot 4 with a `b1`. Blue is
bucket 1 and the clue is a colour one, so Cathy is bucket 0 — `{r1, y1}`. The
continuation would be the `b2`, and a card the Blue clue did not touch cannot be
blue, so it drops. Had Bob played the `g1` instead — green is bucket 1 too — the
bucket half is the same and the `g2` survives: `{r1, y1, g2}`.

**Whose eyes.** Both halves read the reacter's card as *this seat* can: its
identity when the seat watched it, and the inference the clue left on it when the
seat IS the reacter and cannot name their own card — the POV rule
`resolve_hidden_action` already follows (§1.1). So the receiver and the giver
read `{r1, y1}` while the reacter reads the wider set their own superposition
allows. Seats knowing different amounts is this variant, not a defect; the
receiver's reading is the one that matters, since it is their card.

The receiver's reading cannot come from the reacter's *inference* alone, which
would be the tidier rule: at clue time the receiver returns before the target
walk runs — they cannot see their own hand to find the target — so in their game
the reacter's card was never narrowed at all. Which is a thing to be fixed in its
own right, not just worked around: see below.

#### ...and the receiver reads the REACTER's card too (v16.19.0)

The sentence above was a dead end for eight versions. `interpret_reactive` narrows
the reacter's card at clue time and writes it into `common`, so
`note_hidden_action` later finds a singleton, advances the shared stacks and never
stamps a superposition at all. **The receiver, returning early, gets none of that** —
so at its seat every reactive blind play left a superposition as wide as the pre-clue
empathy, for the rest of the game, and its shared stacks stayed behind every one of
them.

It can be put right at reaction time, and only then, because that is when the
receiver learns which slot answered:

> **The reacter's card is the playables of `bucket_of(the identity the receiver
> saw)`**, in the frame the pairing was judged in and over the reacter's own open
> worlds — the same reading clue time computes, recovered from the card instead of
> from the target. The reacter derived that bucket from the receiver's target, and
> the relation is what put the card in it, so the two agree without the receiver
> ever knowing its own target.

`narrow_reacter_play` (`tiiah/interpret_reactive.cpp:819-884`), called from the
engine seam just before `narrow_receiver_call` (`src/basics/decide.cpp:739-754`)
since it can move the shared stacks the receiver's own reading then rests on. Both
readers share `bucket_over_worlds` (`:125-147`); the narrowing and its settle are
`tiiah::narrow_superposition` (`tiiah/superposition.cpp:568-580`), shared like §1e
rules 1 and 2 because every seat computes it alike. A no-op at the giver's and the
reacter's seats, where the clue-time reading already resolved the card.

**The REACTER's card only (v20.16.0)** (`:829-834`). The play hook calls this on any
play while a reaction is pending. On the reverse arm the receiver moves first, and
its standing play is not the reaction.
- Self-play seed 270 T41: Bob's called o34 `{b1,b2}` (the b1), played as the reverse
  reactive's standing play, was read as the reacter's card on the frame the giver and
  the reacter share (blue on 1 there), and settled for the team as the b2.
- `tests/test_tiiah/test_receiver_standing_play_is_not_the_reaction.cpp`.

Replay [2010512](https://hanab.live/shared-replay/2010512#14) is what it cost.
yagami answered two rank-1 reactives by playing its `p1` and then its `p2`; bucket 2
is `{p}` alone, so each reading was a single identity and will-bot69, the giver,
resolved both. will-bot67, the **receiver**, kept 24- and 20-candidate sets, so its
shared stacks read purple 0 instead of 2 and its row for yagami never moved off
`[0,0,0,1,0]`. At T13 the human's reactive yellow named a `y2` playable only once
yellow is on 1: against that row it read one away, the pairing came out a FINESSE
demanding a `y1` the reacter could not hold, the walk found no pairing at all and
the clue was recorded a **MISTAKE**. will-bot67 then gave a stable clue while
will-bot69 went on waiting for its reaction — the asymmetry `decide.cpp:1100-1108`
warns about, arrived at from the one direction nobody had closed.

**A finesse the receiver can PROVE is read as one (v17.1.0).** In a finesse the
reacter knows its card outright: it is the connector. The bucket, by contrast, may hold
more than one playable. The receiver cannot see its own target, but its called card
carries the clue's public touch information, and that is often enough:

> If the called card could be the card after the one the reacter played (the finesse
> half), and could not be any card of the bucket half in any of the receiver's worlds,
> then the pairing was a finesse. The reacter's card is then exactly the card we
> watched, for the whole team.

The test is `proven_finesse` (`tiiah/interpret_reactive.cpp:993-1005`), over
`finesse_from_the_card` (`:742-755`). It runs over the same two halves `receiver_reading`
builds, on the shared view over every seat's hole cards. That is a frame the giver and the
reacter compute alike, so they can predict the answer (v17.2.0). It is applied first in
`narrow_reacter_play` (`:569-580`). For example, Cathy's Red names Bob's r2 and Alice
answers with the r1. Bob's red card cannot be a purple, so he names her card the r1
rather than `{r1,y2}`; without it his shared view and his row for Alice stay on red 0
while theirs move.
When the card could be either half, the bucket set is still taken (TODO.md 49).

**The giver and the reacter name the card only as far as the receiver can**
(v17.2.0). Those two know the reacter's card exactly, because they saw the target the
walk paired it with. The receiver did not. When the card is played,
`note_hidden_action` asks `reaction_team_reading` what the receiver will be able to
name (`tiiah/interpret_reactive.cpp:1319-1365`). That is the proven finesse's connector,
or else the reacter's bucket on the shared frame, intersected with what the card was
read as before the clue (`ReactorWC::react_before`). When that is a single identity,
the card is named for the team as before. Otherwise the giver and the reacter keep it
privately (`tiiah/superposition.cpp:1529-1593`):
- the shared view carries the team's set for it (`ConvData::shared_left`), exactly as
  after a private settle;
- the giver's rows take it, since the reacter knew it and the receiver watched it;
- the reacter's row for the receiver does not, because the receiver cannot know the
  reacter knew it.

The test that proves a finesse, and the bucket reading, run on a frame every seat
computes alike: the shared view over every seat's hole cards, as the team reads them
(`finesse_from_the_card`, `:742-755`), and leave the card itself out. Otherwise the
giver and the reacter move the shared view on the reacter's card (a g1, say) while
the receiver does not, and the three shared views never agree again.

**A deferred reaction is read the same way** (v17.4.0). A reacter who clues first
answers later, through `pending_reactions`, when no live connection is left. Until
v17.4.0 that path skipped `record_reaction`, `narrow_reacter_play` and
`narrow_receiver_call`. The fix runs them for a deferred play too
(`src/basics/decide.cpp:695-707`). The reacter's card is read even when the receiver's
target has already left its hand, since the relation reads the reacter's side from
the card alone (`reactor0::resolve_deferred_reaction`, `no_target`). Without that, a
receiver of a deferred reaction whose target had already been played held the
reacter's card as every identity, while the other two seats held the bucket reading.

**The frame leaves the card being read out (v17.1.0).** By reaction time the reacter's
card is itself in the hole. A frame floored across worlds would count it as already
down, and read it one card too far. `reacter_frame` takes an `except_order` for this
(`:173-187`), and `narrow_reacter_play` passes the card (`:582-591`). Without it, a g3
answering a Red made the frame read green 3 instead of 2, the reading came out
`{g4}`, and a row went to green 4.

#### What the receiver's OTHER slots learn, and whose eyes that is NOT

The reaction also lays a **negative** on the rest of the receiver's hand — *"if
that slot had been playable, the clue would have named it instead"* — held until
the receiver acts and then fired by `Game::fire_reaction_elim`
(`src/basics/decide.cpp:84-220`). reactor0 picks between three strengths of it by
asking which stack the receiver's action advanced (reactor0/CONVENTION.md §1d.2).

**Here the stacks cannot answer that, and the button does.** A play goes into
the hole, so the seat that made it never learns which stack moved and its own
stacks do not advance — its own play would read as a *discard*, the strongest of
the three readings, and strip a hand that earned none of it. So:

> **A receiver PLAY takes the ordinary double-play reading, in every seat**:
> only the slots the target walk passed over, and only that they were not
> directly playable. A receiver DISCARD keeps the ordinary reading, since a
> discard is public and shows the card.

The suit is not asked for at all, and that is the point. A seat that watched the
card leave could name it, but the *team* cannot, so reading the reaction by it
would put that seat's hand-model out of step with the others'. The ordinary
reading is the weakest of the three — passed-over ⊂ whole hand, nothing ⊂
passed-over — hence the sound intersection of the readings a play could have
been.

This is the line "Whose eyes" above does not cross. Seats knowing different
amounts about the receiver's **own card** is this variant working; a **negative**
laid on the rest of that hand is a shared commitment, and is read from what every
seat knows or not at all (§1g).

[2008422](https://hanab.live/shared-replay/2008422) is the cost of getting it
wrong. will-bot67 played its reactive target (a `g1`) into the hole at T5 and, in
its own seat only, read that as a discard: order 5 lost its playables, its
one-aways and its trash and was left as `{y5, g5, b5, p5}`. At T7 yagami's rank-3
clue called that very slot to discard; every seat stamped it, and then rule 4
(`drop_dead_chuck_calls`) found nothing chuckable in an all-critical inference
and erased the call in will-bot67's seat alone. It threw its newest card instead.
`tests/test_tiiah/test_replay_2008422_receiver_play_in_the_hole_is_not_a_discard.cpp`.

**A withdrawn target disarms the negative (v18.14.0, reactor0/CONVENTION.md
§1d.2).** Once the call on the target is withdrawn, by the call invariants or by a
§1h fix, whatever the receiver later does with that card is no answer to the
reaction. At 2014402 T27 green threw its erased o10, and the "receiver discarded"
row left its o29 `{y5,g2,g3,g5}` and its o25 `{y5,g3,g5}` at every seat.

#### Legality and reading are different jobs

**What Alice may give** is the wider question. She may give a clue that is
neither a finesse nor a bucket relation **when she knows that Bob and Cathy will
each know the identity of the card they are about to play** — judged from *their*
views, not hers, since it is their empathy that has to settle it.

**What the others read** is fixed: the reacter and the receiver combine the
finesse possibility and the bucket relation to pin their inferences. When that
combination yields nothing, each falls back to a **superposition over every
playable identity their own empathy still allows** — which is exactly what makes
Alice's licence above work, and is where a player can know their card even though
the clue encoded no identity.

That fallback is a *reading* rule, so it creates superposed cards with nobody
having played anything, and §1e's collapsing rules govern them the same way.

#### The relation is a legality test on the GIVER, not a filter on the walk

The bucket relation is computed from the **reacter's own card**, which the
reacter cannot see. So it splits in two, and v16.15.0 is where it was put on the
right side of the line:

| | who applies it | what it does |
|---|---|---|
| **legality** | the **giver**, alone | a pairing that is neither a finesse nor a bucket relation, and which the two players could not each name from their own empathy, makes the clue **illegal**, except in the endgame (pace ≤ 1, v18.4.0). Alice may not give it (`interpret_reactive.cpp:569-607`) |
| **inference** | every reader | what the reacter's and the receiver's cards ARE, from the bucket the *receiver's target* sits in — which every seat can see (`:366-391`, `narrow_receiver_call`). A reacter that can rule the bucket reading of its own card out by sight reads it as any playable (core rule 2, v18.9.0; below) |

So **the walk runs on shared information alone**, and every seat — the reacter
included — lands on the same pairing. That is §1g: the relation is exactly the
kind of giver-only fact that may refuse a clue and may not choose among its
slots, which is the rule the line above it has always followed (if what the
reacter is actually holding cannot fit the pairing at all, the clue is refused
outright rather than retargeted).

Until v16.15.0 a failed relation was a `continue` — it retargeted. Replay
[2010246](https://hanab.live/shared-replay/2010246#2) T2 is what that cost.
will-bot67's rank 2 had two pairings: yagami's directly playable `r1`, anchoring
will-bot69's slot 2 which held a `y1` — both bucket 0, so illegal under a rank
clue — and behind it a `y2` finesse anchoring slot 1. will-bot67 could see the
`y1`, skipped the first pairing and gave the clue meaning the second. will-bot69
could not see its own card, never evaluated the relation, and played **slot 2**:
the pairing the giver had skipped. One clue, two seats, two slots.

**In the endgame the relation does not bind the giver (v18.4.0).** At pace ≤ 1
(`State::pace`), a pairing may break the bucket relation even when the players
cannot name their cards. Each reader still reads by the core rule: the bucket and a
finesse if they leave anything, else any playable. The exception is a legality
change only, so every seat still walks to the same pairing.

The motivating case is human diagnostic 2013726 T27
(`v18_human_vs_bot_diagnostics/2013726.md`). At pace 1, 4 to green is a reactive
that gets black's r4 (o30) and green's g1 (o23), but the pairing breaks the bucket
relation. Blue gave a stable Red to black instead. It now gives the 4
(`tests/test_tiiah/test_replay_2013726_endgame_pairing_may_break_the_bucket.cpp`).

**Black reads its own card by elimination (v18.9.0).** Through the bucket, the team
reads black's card as the n3. Black sees both n3s, green's and the one in the
discard pile. So for black the bucket reading is empty, and core rule 2 makes the
card any playable. Less the g1 that green's target is (the pair would both play
it), that is exactly the r4, as the reviewer put it: black "knows that they hold
the r4 exactly because they see all copies of n3".

How the code does it:
- When the team's reading of our own called card is empty in our own view,
  `Game::elim` sets our view to `tiiah::own_called_fallback`
  (`src/basics/game.cpp:808-816`; `src/conventions/tiiah/superposition.cpp:1342-1355`).
  That is the card's remaining identities that are playable on our stacks, less
  the receiver target's identity when we are the reacter. It replaces the whole
  empathy.
- Our own blind play is then named by our view, not the team's
  (`hidden_own_play_id`, `src/basics/game.cpp:556-578`). So our stacks take the
  r4, where they used to take the n3.

Test: `tests/test_tiiah/test_replay_2013726_reacter_rules_out_the_n3.cpp`, a
black-seat snapshot of the hypothetical.

Still open: the **shared view** is built from the team's reading, so it books the
n3 at every seat. Only the reacter's own view knows better.

#### Inverted targets, and the double chuck

**Inverted suits are skipped** as reactive targets, playables and finesses
alike (`receiver_targets`, `src/conventions/tiiah/interpret_reactive.cpp:247-301`),
unless they are **the only playables left in the receiver's hand** — that test is
over the receiver's hand, not the whole table — in which case the clue is a
**double chuck** instead: both players press **Discard**, which is the button
that stacks an inverted card.

A double chuck asks something different of the reacter, because they are not
playing. What they hold has to be **affordable to chuck**
(`safe_to_chuck`, `interpret_reactive.cpp:97-103`): either the button plays it —
an inverted card the stack is waiting for — or losing it costs the team nothing,
which is any card that is not critical (trash included, since trash is never
critical). Alice may not name a slot that fails this, and the refusal is a
giver-only one, so it kills the clue rather than moving to the next target.

Nothing else changes. The bucket relation does not apply — an inverted suit is in
no bucket — and does not need to, since no identity has to reach the reacter for
them to press Discard; a double chuck over a one-away target still names its
connector. The receiver's own call comes from the ordinary even-parity mirror at
resolution time (`receiver_button`, `src/conventions/reactor0/decision.cpp:99`),
which reads the button the reacter actually pressed
(`reacter_button_pressed`, `src/conventions/reactor0/interpret_reaction.cpp:521`)
rather than the hook that fired.

### §1e Superposition

A player who played a card without knowing its identity is **superpositioned**:
they keep a map of card order → the identities it could have been, and so does
everyone else on their behalf. A player may hold several at once.

**A reactive's target is walked on the MINIMUM, suit by suit, across every world
the reacter can live in, from the giver's perspective (v16.24.0).** Neither the
giver nor the reacter can name their own hole cards, so each assignment of those
cards is a set of stacks the reacter might hold, and only a height every one of
them reaches is one the giver can count on. `reacter_frame`
(`src/conventions/tiiah/interpret_reactive.cpp:176-190`) is that frame: for the pair
itself it is the pair's row, which `advance_rows_from_own_worlds` floors over both
seats' worlds; for an outside seat it is the shared view floored on the fly over
every seat's. `stacks_after_queued_plays` starts from it
(`src/conventions/variants/hole.cpp:10-24`), so every seat walks the same simulation.

It replaced "assume none of the superposed cards were played" (v16.4.0), which is
the minimum only when no two worlds share a height. Worked example, replay
[2011397](https://hanab.live/shared-replay/2011397#14) T14: will-bot69's two hole
cards left the worlds 10131 (o9 = b2, o8 = b3) and 10122 (o9 = p2, o8 = b2). Their
minimum, 10121, makes the p2 on will-bot67's slot 2 the target of yagami's Rank 3;
the old rule had settled on 10122, read the p2 as trash, walked on to the y1 on slot
4, and will-bot69 pitched a g3 from its slot 4 into a strike. (World feasibility,
below, then rules the second world out altogether, and the frame is 10131.)

**Every view is the minimum across its seats' worlds**, which is the same rule
asked of each view: the rows (`advance_rows_from_own_worlds`, v16.18.0), our own
belief (`presume_own_plays_land`, v16.19.0) and, as of v16.24.0, the shared view —
`advance_common_from_worlds` (`src/conventions/tiiah/superposition.cpp:1714-1738`),
at the end of every `collapse_superpositions`, over every seat's hole cards. Every
input is shared, so every seat writes the same floor.

#### Worlds are replayed in play order, on a view's band (v16.24.0)

A world is replayed onto a base view card by card, and two details decide whether
it strikes:

- **Play order, not card order.** Card order is draw order, and a card drawn early
  can be played late. `ConvData::hole_turn` (`include/hanabi/basics/card.h:177`) is
  stamped by `note_hidden_action` and `enumerate_worlds` sorts on it
  (`superposition.cpp:590-692`). Replay 2011397: o8 went in at T11, after o9 at T8,
  and (o9 = b2, o8 = b3) replayed as b3-then-b2 looked struck.
- **The band.** A view's floors, and a card named above a gap, raise it past what
  it can NAME in sequence. Each view therefore keeps an **evidence** vector that
  only advances by the next card (`State::pairwise_evidence`, `common_evidence`,
  `play_evidence`; `advance_evidence`, `src/basics/state.cpp:151-165`), and the gap
  between the two is a **band** of cards the view counts without knowing which
  card each was. A world's card whose rank falls in the base's band is absorbed as
  one of them — once per rank — rather than read as a duplicate strike.
  Without it a floor replayed on itself struck the world that produced it: replay
  2011327 T36, where yagami's `{b3,p3}` and `{b3,b4}` floored will-bot67's row for
  her at blue 3, and the next pass struck the (p3, b3) world and lifted blue to 4.
  `State::with_band`, `private_base`, `with_pairwise_floor`, `with_common_floor`
  (`state.cpp:182-277`).
- **But never an identity the team has NAMED** (v16.28.0). A band rank is a card the
  view holds without naming it; a hole card the whole team named — known when it was
  played, or settled by a shared argument since — is not that, and a world card of the
  same identity is its duplicate, so it strikes. The name is kept on the card as
  `ConvData::named_in_hole` (`include/hanabi/basics/card.h:183-189`), written by
  `note_hidden_action`'s known branch, `settle(..., shared)` and `settle_shared_only`.
  Replay [2011885](https://hanab.live/shared-replay/2011885#17): will-bot69's o4, known
  to be the g3, landed at T16 above an unnamed g2 (its own o18), so the shared view held
  green 3 with a band over 2–3; at T17 will-bot67's `{g3,n1}` was absorbed as that g3
  instead of striking, the n1 never reached the shared view, and at T32 brown still
  read 0 — so will-bot67 could not see yagami's n2 and will-bot69's n3 as a reactive,
  and discarded. `tests/test_tiiah/test_replay_2011885_named_card_not_absorbed.cpp`,
  `tests/test_tiiah/test_world_replay_band.cpp`.
- **A pairwise row replays the hole cards with the sets BOTH seats of the pair hold**
  (v18.1.0, `open_worlds(..., shared=true, row=true)`, called by
  `advance_rows_from_own_worlds`, `superposition.cpp:1357-1404`). Each copy of a row is
  computed by one seat of the pair. Our partner replays our hole cards with the team's
  set, because it cannot know what we settled privately, so our copy must replay them
  the same way. A card we settled privately is therefore replayed with its
  `ConvData::shared_left`, not left out. The one exception is a card whose private
  name the row already counts. The private name is `ConvData::private_named`
  (`include/hanabi/basics/card.h:190-197`), written by `settle(..., shared=false)`
  (`superposition.cpp:291-331`) and by the reaction-card branch of
  `note_hidden_action` (`:1317-1328`), whose card the giver and the reacter name
  exactly while the team holds a wider set (§1d). The row has then counted that
  card, and replaying it would strike its own identity as a duplicate
  (`enumerate_worlds`, `superposition.cpp:613-622`). The reaction branch matters for a
  reacter's card called as the r1: it is already booked on the reacter–giver row by
  `with_pairwise_at_least`, and replayed with its team set, its r1 world would strike
  and the row claim the other identity (a y2, say) with yellow on 1. Replay
  [2013616](https://hanab.live/shared-replay/2013616#20) is the case:
  - will-bot69 had settled its o6 `{r1,g1}` as the r1 privately, since it could see
    both other g1s, and left the card out of its row replay.
  - will-bot67 replayed o6. After both had watched yagami's o23 land as a g1 at T18,
    the g1 world struck at will-bot67's seat, and its row for will-bot69 went to
    red 1. will-bot69's row for will-bot67 stayed on red 0.
  - will-bot67's T19 Green to yagami was an ordinary reactive, and the two seats
    paired it on different rows. will-bot69 read it as a MISTAKE and discarded o15
    instead of playing o24, the r2.

  `tests/test_tiiah/test_replay_2013616_reacts_after_the_pair_settles.cpp`.
  Until v18.1.0 the card was simply left out of the row replay (v16.28.0). A card
  waiting on it was then not treated as a strike, which is replay
  [2011887](https://hanab.live/shared-replay/2011887#18) T18, where will-bot69's o9
  `{r2,g2}` waited on its privately named o6, the g1. Replayed with its shared set,
  o6 now lands the g1 in its own world, so that case needs no rule of its own.

#### The shared view settles on what every world agrees (v16.25.0)

Never presume a strike, asked of the SHARED view over every seat's hole cards
jointly: what every strike-free world agrees a card WAS, the team knows it was.
`advance_common_from_worlds` (`src/conventions/tiiah/superposition.cpp:1714-1738`)
prunes to those worlds, shared, as well as raising the floor. Replay
[2011475](https://hanab.live/shared-replay/2011475#25): yagami's o4 went into the hole
at T4 as `{g1,b1}` (the g1). Nothing names it until will-bot67's T24 play (o23,
`{g2,b1}`) leaves (g1, g2) and (g1, b1) as the only strike-free worlds — so o4 settles
to the g1 at T25, and not before.

The worlds this runs over are the **shared** set. A seat that settles one of its own
hole cards PRIVATELY (rule 3, rule 6 on its own plays, the back-solve) knows more than
the table; if that card simply left the enumeration, the shared view would be missing
a card no other seat can name, and could strike a sound world for want of it. So a
private settle or narrowing leaves `ConvData::shared_left` behind
(`include/hanabi/basics/card.h`) — the set every other seat still allows — and the
shared-view callers (`open_worlds(..., shared=true)`) enumerate the card with it.
`prune_to_worlds` (`:368-430`) narrows `shared_left` on a shared argument and our own
set on a private one, and `settle_shared_only` (`:337-350`) teaches the shared view and
the rows a card we had already named ourselves.

**A card the team names ABOVE the shared view explains the gap below it (v20.16.0).**
A shared settle used to put a card on the shared view only when it was the next card
there. A card named above an unnamed gap went on nowhere, and its set was cleared, so
no later enumeration counted it.
- Now the settle hands it to `known_play_lands_in_common`, which keeps only the worlds
  of the other hole cards in which it lands and floors the shared view
  (`settle_above_the_shared_view`, `:280-289`, called from both settles).
- Only as far as the worlds explain it. A card named after the fact may have been a
  strike, unlike a play its player knew, so with no world in which it lands nothing
  moves (`known_play_lands_in_common`'s `presume_unexplained = false`).

Replay [2019408](https://hanab.live/shared-replay/2019408):
- T16: will-bot67's o13 `{y2,b1}` settled as the y2, with yellow on 0 in common: its o7,
  the y1, was unnamed.
- The y2 left the team's accounting. At T22 yagami's o3 `{r1,y3}` (the y3) could only be
  strike-free as the r1, and red went to 1 everywhere.
- At T34 his Red read o36, a real r1, as `{r2}`.
- Now o7 settles as the y1 and yellow is on 2 at T17. o3 stays `{r1,y3}`, and o36 reads
  `{r1}`.

`tests/test_tiiah/test_named_hole_card_above_the_shared_view.cpp`,
`tests/test_tiiah/test_replay_2019408_named_hole_card_above_shared_view.cpp`.

**A reactive's promise is read on what the RECEIVER knows** (v16.25.0). At the
receiver's own seat that is its belief; at every other seat it is the frame the giver
and the receiver share — `narrow_receiver_call`
(`src/conventions/tiiah/interpret_reactive.cpp:1170-1276`), holders {receiver, giver}.
Replaying the receiver's hole cards on OUR belief strikes the world in which they are
what we watched them be: at 2011475 T18 will-bot67 read yagami's called o21 as
`{r4,b1}`, the g1 world gone because it had seen her o4 land as the g1. It is
`{g1,b1,r4}`.

**Notes show the team's reading** (v16.25.0). A `[f]`/`[d]` note on a partner's card is
`common`'s reading of it, not our sight: yagami's o4 was noted `[f] g1` at T3 by both
bots, which read as though her superposition had collapsed on the spot
(`src/net/notes.cpp`, `compute_note_segments`).

**A hole card is noted as it narrows (v20.17.0, the user's request).** A card that has
left its hand into the hole is noted each time the team's reading of it changes: its
superposition, then the identity it settles as (`named_in_hole`, else our own
`private_named`). Every seat's hole cards are noted, since nobody can see one, within
the same six-candidate cap as an unstamped card of ours. A card that left the hand
with a known identity gets no note.
- Replay [2019408](https://hanab.live/shared-replay/2019408): will-bot67's o13 reads
  `turn 2: [f] y2,g1,b1`, then `turn 9: y2,b1` as it goes in, then `turn 16: y2`.
- `replay_log --notes` prints a log's note segments.
- `tests/test_net/test_notes.cpp` (`AHoleCardIsNotedAsItsReadingNarrows`).

#### Rule 8: a card that STRUCK was already down (v16.25.0)

A strike arrives as a failed discard with its identity withheld, but every seat except
the striker watched the card. If it looks playable on our stacks, our stacks are short
— by a card of ours in the hole — and the worlds in which it is not already down are
refuted (`presume_discard_was_played`, `superposition.cpp:1055-1154`, with no "named"
requirement: the card physically failed to land). The conclusion is the watchers', so
it prunes privately.

Which hole card it was need not be decidable card by card, so the joint fact is kept:
a `HoleRequirement` (`include/hanabi/basics/game.h`) says "one of these cards WAS X",
and `world_feasible` (`:722-784`) drops any world that breaks it. Rule 7 records one
too. Replay [2011475](https://hanab.live/shared-replay/2011475#47) T43: yagami's b3
struck with blue on 2 in will-bot67's belief, and its own o7 `{b3,p3}` and o33
`{g5,b3}` left (g5,b3) and (b3,p3) — blue on 3 either way. At T47 it gave a rank 2
revealing trash instead of the Blue for will-bot69's b4; it gives the Blue now.

**Rule 8's shared form: a watched dupe strike is common knowledge** (v16.27.0,
`strike_was_a_watched_dupe`, `superposition.cpp:1014-1051`). The form above is about
*our* hole cards and stays private. A strike can also tell the whole table something:
when the card is X, X is already down on our own stacks, the shared view has X's suit
at X.rank−1 or above (so no seat can read the strike as "not playable yet" — a 1
always qualifies), and the copy that is down is a hole card we can name as X **held
by a seat other than the striker**. Both watchers saw X strike, and the striker —
who cannot see what struck — watched the other copy go in, so every seat knows X is
down. The shared view is floored at X on its suit, every pairwise row with it
(`team_learns_already_played`, `:798-809`), and a `HoleRequirement` records that one
of the superpositions admitting X was the copy; the worlds collapse the rest when
they fit under the cap. Replay [2011854](https://hanab.live/shared-replay/2011854#41):
yagami's o24 `{r3,p1,p2}` was the p1, and at T28 will-bot69's o29 — a second p1 —
struck. The shared view stayed on purple 0 for the rest of the game, so at T41
yagami's Rank 5, pairing will-bot67's b4 with will-bot69's p2, was unreadable to
will-bot67 (`reactor0.reactive_unreadable`) and it discarded chop; it plays the b4
now. The striker's own seat does not take this: it would have to deduce X from the
reading of the call it played (TODO.md 58).

#### World feasibility: the targeting rules are evidence (v16.24.0)

A world must be one the targeting rules allow. Every reactive play clue that
resolves is kept as a `ReactionRecord` (`include/hanabi/basics/game.h`): the
receiver's hand at clue time, the cards the walk passed over as already called, the
frame the receiver could reconstruct (the shared view then, the same at every seat)
and the card the reacter's answer named by the sum rule. `record_reaction`
(`interpret_reactive.cpp:191-205`), from the reaction seam in `Game::interpret_play`
(`src/basics/decide.cpp:744-750`), only on the ordinary arm — on the reverse arm the
receiver moves first and the frame rests on plays it cannot name.

`world_feasible` (`superposition.cpp:789-851`) then asks, of every record whose
receiver held two or more of a world's cards: the called card, if it is one of
them, must be direct or one away in that world; no other of them may be a direct
playable to its left, nor a direct playable at all when the called card is a
finesse; and none may be a finesse to its left when the called card is one too.
`open_worlds` drops infeasible worlds (keeping them all if none survives), and
`prune_infeasible_worlds` (`:1370-1394`) settles on the survivors, shared, at the
start of every collapse.

Replay 2011397 T6: will-bot67's Green reactive, yagami reacting, and her slot 1
called will-bot69's slot 2. Slots 2 and 3, o9 and o8, later went into the hole as
`{b2,p2}` and `{b2,b3}`. In the world (p2, b2) slot 2 is a finesse and slot 3 a
direct playable, so yagami would have played slot 5 to call slot 3 — that world is
refuted, o9 was the b2 and o8 the b3, and every view reads 10131.

#### What a card means depends on what you threw away (v16.13.0)

A seat that threw a card into the hole without naming it **does not know its own
stacks**. So a call on a later card of theirs does not name one identity — it
names a different one in each world the earlier card leaves open, and what they
may write down is the **union**:

> **worlds** = every assignment of one identity to each of the holder's cards
> still in the hole, applied in play order. Read the call in each; the reading is
> the union, and each candidate remembers which worlds supported it.

[2009367](https://hanab.live/shared-replay/2009367) T4 is the case. will-bot69
threw an `{r1, y1}` at T2, and yagami's rank-5 reactive then names bucket 0 on
its slot 4:

| if the T2 card was… | red | yellow | bucket 0 offers |
|---|---|---|---|
| `r1` | 1 | 0 | `{r2, y1}` |
| `y1` | 0 | 1 | `{r1, y2}` |

so the reading is **`{r1, y1, r2, y2}`**, with the `r2` living only in the first
world and the `y2` only in the second. Before v16.13.0 it read `{r1, y1}` — the
answer in the one world nobody had established they were in. (It was in fact the
`y1`, so the narrow reading was lucky rather than earned.)

A candidate every world agrees on is unconditional and nothing is recorded for
it. The rest are kept in `ConvData::ConditionalReading`
(`include/hanabi/basics/card.h:200-226`) beside the superposition itself, which
is what lets a later fact **withdraw** them: when the earlier card settles, the
worlds it contradicts die and the candidates with no world left go with them
(`refute_worlds`, `src/conventions/tiiah/superposition.cpp:21-89`, called from
the collapse below and from `settle` before it clears the set).

Two limits, both deliberate. The enumeration is **capped at 64 worlds**
(`enumerate_worlds` / `open_worlds`, `:500-625`) and reads the call flat beyond that, because a
partial list of worlds would be a conditional set missing some of its own
conditions — worse than an unconditional one. And the collapse now runs to a
**fixpoint**, since settling one card can refute another's worlds, leave *it* a
singleton, and settle it in turn; a single pass over the orders only caught a
cascade that happened to run in increasing order.

#### The Actionable Superposition Collapse Rule (ASCR, v20.6.0)

The user's ruling, a broad rule for reactor0 in this variant (human diagnostic
[2018541](../../../v18_human_vs_bot_diagnostics/2018541.md)). Whenever a player seems
to be actioned to play an unplayable card, it must first check whether some world
of the hole cards makes that card playable, before dismissing the action. This
applies to the reacter or the receiver of a clue, and not only after a clue has
resolved.
- If some world does, it **immediately collapses** the worlds to those in which the
  card it was called to play works.
- As a **receiver**, it evaluates the bucket + finesse suits before the others.
- Only if that fails does it move on to the next interpretation, if there is one.

**One shared routine** decides it: `ascr_find`
(`src/conventions/tiiah/superposition.cpp:514-552`).
- It takes the card's candidate identities in **tiers**, in priority order, and a
  test of whether an identity works in a world.
- The first tier with an identity that works in some world is the reading, and the
  kept worlds are those in which any of it works.
- With `require_evidence`, a reading one of whose identities works in **every** world
  is refused: it does not rest on the hole. A reading whose identities each need some
  worlds is kept, even when together they cover them all (v20.8.0, replay 2018766).
- The caller then collapses with `collapse_to_worlds(..., shared)` and makes its call.
- Every site logs `tiiah.ascr` with its `site`.

The sites on it:

| Site | When | Tiers | Since |
|---|---|---|---|
| the reactive walk, per target (§1d) | the reacter's card cannot play on the frame, or a one-away target has no connector it can be | the bucket the relation names, then any | v20.6.0 (absorbs v19.3.0's no-pairing fallback) |
| the receiver at the reaction (§1d) | the shared stamp found nothing the target can play on its frame | bucket + finesse, then any one-away | v20.5.0, on `ascr_find` since v20.6.0 |
| the reacter's own reading of a bucket pairing (§1d) | always, at clue time; the bucket identities are judged in the reacter's own worlds against the pre-stamp reading | the bucket the relation names, then the stamp's frame playables | v20.10.0, on `ascr_find` (`interpret_reactive.cpp:736-752`), site `reacter_bucket` |

**The cost, measured so it can be revisited.** On TIIAH 5 Suits (seeds 1–300, against
v20.5.0): perfect games 47 → 41, mean 20.14 → 19.95, strikeouts 100 → 105; 49 seeds
moved (15 up, 34 down). About half the movers follow directly from the rule:
- A reacter whose card is clued narrowly, so that it cannot play on the frame, now
  takes a target's world pairing whenever any identity it might be plays in some world.
- A giver who can see that the card's true identity plays in no world must not give
  that clue (§1g: the reacter would misread it).
- Clues the old walk handled, with the reacter walking past that target, are no longer
  givable.

The user chose the rule as stated.

**Not yet on it** (`TODO.md`):
- the stable-call world re-run below, which takes the first world that makes the
  call and narrows only privately;
- the call invariants' "call alive in a world" (§1e, v16.29.0), which keeps a call
  without collapsing.

**A STABLE call is read in every world too (v16.24.0).** The stable ladder reads a
clue on one frame, the minimum across the worlds of the two seats' hole cards; in
each of those worlds it names a different card. `read_stable_over_worlds`
(`superposition.cpp:1259-1340`) re-reads a card the clue has just called to play in
every strike-free world of the **TARGET's** hole cards — the target's only: a third
seat's hole cards are not ambiguous to the pair, who watched them go in — and writes
the union, the next card of each named suit in each world, with the conditional half
recorded. One world, nothing changes.

**The giver's own hole cards do not widen it (v20.4.0, the user's ruling).** A
stable clue is read on the giver's own stacks. The giver cannot know which world of
its own hole cards it is in, so it meant the card its frame names. Had its hole card
been that very card, the receiver, who watched it go in, throws the call as the dupe
(`repin_own_call`'s named dupe, v18.18.0). The receiver's own hole cards still
widen it, because the giver watched those and the receiver cannot name them.

Replay [2018435](https://hanab.live/shared-replay/2018435) T11, from will-bot69
(locked) to yagami_black:
- **Yellow** calls black's o13, the y1, under the newly-touched focus (reactor0 §1b
  priority 5, v20.4.0). It is still `{y1,y2}`, widened by black's own o12
  `{r1,y1,g2,g3}`. Black cannot name its card, so §2c drops Yellow.
- **Green** calls o14 as `{ra1}`. Widened by will-bot69's own `{y1,g1,b1,ra1}`, it
  was `{ra1,ra2}` and §2c dropped it too.

Green is now named and given, which the user accepted over Yellow. Until v20.4.0
neither survived, and will-bot69 locked black with a 3. The replay's own test was
deleted in v20.6.0, with the user's approval: under ASCR (below) its T4 Blue reads
differently, so the T11 position no longer arises. The rule is pinned by
`tests/test_tiiah/test_rainbowy.cpp` and `tests/test_tiiah/test_decision_making/test_own_dupe_filter.cpp`.

A seat that can SEE the card judges the call against it, so on the minimum frame it
may refuse a call that is sound in the world the pair is actually in. Then the
ladder is re-run in each world and the first that makes the call is taken
(`tiiah::interpret_clue`, `interpret_clue.cpp:569-675`).

**An outside seat also tries its own hole cards (v17.1.0).** The pair watched the third
seat's cards go in, so a world of them is one the pair may well be in. When no world of
the pair's own cards makes the call, the third seat also enumerates its own hole cards
(`interpret_clue.cpp:584-592`). The call then says what those cards were: the seat keeps
only the worlds that make the call, privately. This is rule 4's shape, and what it learns
the pair already knew. The narrowing is `narrow_own_privately`
(`superposition.cpp:1867-1885`). It moves our belief and no row (v17.3.0): the other
seats watched the card, but they cannot know that we now know it. The case: Cathy's
Blue names Alice's b3 on the b2 Bob blind-played a turn earlier. Bob cannot name his
b2, so on every frame of theirs he would find no call and read a MISTAKE.

**So does the target, when it can name the card called (v17.5.0).** The giver watched
the target's hole cards go in, so a call that the ladder makes in only some worlds of
them tells the target which: it keeps only those worlds, privately, exactly as the
outside seat does (`interpret_clue.cpp:612-670`). The condition is that every card the
clue calls is one the target can already name from its empathy. The giver judges a
call against the card it can see and the target against its empathy, so with an
unnamed card the worlds that make the call at the target's seat can be ones the giver
never meant: a Green that is a trash reveal to the giver can be, to the target, a
play call on an unnamed `{g2,g3,g4,g5}` made only in the world where one of its hole
cards was the g1. The rule is for a card the target CAN name. A Blue that makes the
target's card a known b2 and calls it is a call only where the target's `{b1,p1}` hole
card was the b1; without the rule the target's belief keeps blue on 0, the call never
looks playable, and it discards instead of playing the b2.

**Deferred collapse: a colour re-touch of a clued card waits for its holder
(v18.12.0).** A COLOUR clue whose call singles out a card that was **already
clued**, and that is playable only in some of the worlds, may be asking for the card
to be thrown as the dupe rather than played. So nobody collapses on it yet:

- the outside seat's narrowing above is skipped (`interpret_clue.cpp:636-655`);
- rule 2 (below) takes no evidence from it when a hole card could still be that
  identity (`evidence_from`, `superposition.cpp:170-196`).

The holder's action resolves it. A play is evidence by itself (rule 1). A discard is
rule 7's shared form: the card was already played, so only the worlds in which it
is trash survive, at every seat. Cluedness is the condition: the same Red on a card
**not** clued before can only be a play call, and it collapses at once.

Human diagnostic [2014076](../../../v18_human_vs_bot_diagnostics/2014076.md) T14:
green's hole cards were `{r1,r2}` and `{r1,y1}`. Light's Red re-touched blue's r2,
clued by the T12 2. That r2 was playable only where green had played the y1 and
then the r1. Green narrowed to those worlds at once, and at T16 blue threw the r2.
The throw meant green had played the r1 and then the r2, but the collapse could not
be undone: green's stacks at T17 read red 1, yellow 2 against the true red 2,
yellow 1 (`tiiah_stacks.py 2014076 17`), and the seats' pair and common views disagreed.

And our OWN call keeps an
identity playable in any world of our own hole cards (`repin_own_call`,
`interpret_clue.cpp:83-180`, via `playable_in_some_own_world`).

Replay 2011397 T10: yagami's Blue named will-bot69's o8, the b3, with its o9
`{b2,p2}` in the hole. On the frame (blue 1) it read exactly `{b2}`; rule 2 then told
every seat "the b2 is still needed, so o9 was the p2" — the wrong world — and
will-bot67, who could see the b3, read a MISTAKE. It now reads `{b2, b3}` at every
seat, and nothing collapses o9 on it.

A card named up to the worlds still counts as named for §1e rule 7 (v16.24.0): a
CALLED card whose reading contains the identity its discard reveals. Replay 2011319:
our Purple on yagami's o14 reads `{p1, p2}` — p2 where our o5 was the p1 — and her
discard of the p1 settles o5.

The set is stamped on the card's `ConvData` (`include/hanabi/basics/card.h:173`)
at the moment of the play, by `note_hidden_action`
(`src/conventions/tiiah/superposition.cpp:1467-1633`), and is built from
**`common`** — the one view all three seats compute alike, which is what lets
everyone hold the same set on the player's behalf. A play whose common empathy
already names one identity is no superposition at all: the player knew, so the
SHARED stacks advance with it.

**Unless we can see it was something else (v19.1.0).** Our `common` copy of a
partner's call can be stale. An outside seat reads a call between two partners
on the shared floor (§1.3), and the plays that floor lacks may be our own. When
we are not the player, we can **see** the card, it landed, and the copy names a
different identity, the card is named **by sight**: `named_in_hole` and the shared
view take what we saw (`superposition.cpp:1484-1504`).

Replay [2015109](https://hanab.live/replay/2015109) (TIIAH & White):
- **T6, T9:** will-bot67's own y1 and y2 went in unnamed.
- **T28:** yagami's Yellow called barakeel's o31. The pair had watched those two
  plays, so the call meant the y3. will-bot67 read the call on yellow 0: the ladder
  narrowed o31 to `{y1}`, its own sight refused the call as a MISTAKE, and the
  `{y1}` stayed. Its y1 and y2 were privately settled, so no world of them could
  re-read the call.
- **T29:** barakeel played o31. The shared view booked a y1, and the y3 vanished
  from every shared world.
- **T43:** yagami's y4 could then be playable in no world, so it settled as the w4,
  and common white went to 4 against a true 3.
- **T47:** barakeel's 1 to yagami was a reactive clue pairing her w4 with
  will-bot67's b3 (o17). On white 4 the w4 looked like trash, so the clue read as a
  MISTAKE at will-bot67's seat, and it gave a clue instead of playing the b3.
- **T53:** barakeel's stable Blue to will-bot67 was misread the same way.

With the card named by sight, common yellow reaches 3 at T29 and white stays on
3. o17 is called `{b3}` at T48 and played, and at T54 the Blue's o45 (`{b3}`) is
played.
`tests/test_tiiah/test_replay_2015109_reactive_after_stale_common_naming.cpp`
and `tests/test_tiiah/test_hole_play_named_by_sight.cpp`.

**Collapsing** (`collapse_superpositions`,
`src/conventions/tiiah/superposition.cpp:1768-1865`). A candidate leaves a
superposition when:

1. another player plays a card of that identity;
2. another player's clue, stable or reactive, puts CTP on a playable card of
   that identity — **except a COLOUR clue re-touching a card that was already
   clued**, whose identity a hole card could still be: that collapse is deferred to
   the card's holder (v18.12.0, see *Deferred collapse* below);
3. every copy of it is accounted for in the discard pile and the other hands;
   and its PAIR form — every copy is accounted for outside BOTH our hand and one
   partner's, which is a deduction that partner makes too;
4. **a clue between two OTHER seats calls a card we can see, and the card is
   further up its suit than our own stacks are** — the back-solve;
5. **a reactive we gave was REFUSED** (§1c): the card we named is already played,
   so a superposition of ours that admits it was it;
6. **a partner's play would not land on our stacks** — so it is landing on
   something of ours, and the worlds in which it strikes are refuted; and the same
   rule asked of OUR OWN plays, so a world in which one of them struck is refuted
   too; and the same rule asked of the SHARED view — **a card the whole team could
   name lands above the shared stacks**, so the worlds of every seat's hole cards
   that leave it striking are refuted (v16.23.0);
7. **a partner discards a card the team had named, and it is playable on our
   stacks** — they can see it has already been played, so it was one of ours, and
   the worlds in which it is still needed are refuted (v16.22.0); and its SHARED
   form, at every seat, the discarder's included — **a card the team had named is
   thrown away**, so the worlds of every seat's hole cards in which it is still
   needed are refuted (v18.12.0);
8. **a partner's card STRUCK, and it is playable on our stacks** — it can only have
   failed because it was already down, by a card of ours in the hole (v16.25.0).
   Rules 7 and 8 also record the joint fact as a hole requirement.

Rules 1 and 2 say the same thing — that identity was still NEEDED, so the
superposed card was not it — and both are **shared**: every seat sees them and
narrows alike, so a collapse on either moves the shared stacks too. Only
evidence every seat holds counts, which is why a play that was itself a
superposition is not evidence: the seat that made it does not know what it was.

**Rule 3 is private, and has a PAIR form (v16.18.0).** Counting copies is done
with our own eyes, so the plain form moves our belief and no row: a partner cannot
follow an argument about cards it cannot see. But a copy in a hand **neither of the
pair holds** is one both of us see, and each of us can see that the other sees it
— so that much of the count is the pair's, and their row may take the answer.
`all_copies_visible_to_pair` (`src/conventions/tiiah/superposition.cpp:554-566`)
is the plain form with one more hand struck out, which makes it strictly the
stronger test: whatever a row learns this way, our own belief has already learned.
Replay 2010329 — our order 3 read `{r4,y1}` with both `r4` copies in will-bot69's
hand, so yagami ruled the `r4` out as we did and held yellow on 1.
`tests/test_tiiah/test_pair_visible_copies.cpp`.

**Rule 4 is how a seat recovers from the hole (v16.12.0).** A third seat cannot
compute what a pair shares — the plays missing from the shared view are its own,
and it cannot name them (§1.3). But it can SEE the card the pair called, and a
call says "this is playable", so the pair holds that suit one below the card.
Anything their stack has above ours can only be our own hidden plays: nobody
else's are missing from our belief. Replay 2008489 T33 — yagami calls
will-bot69's next blue and will-bot67 can see it is a `b4`, so the two of them
hold blue on 3; will-bot67 has it on 2, so the card it threw at T29 was the `b3`.
It then re-reads the call itself, which is how order 34 comes out `{b4}` rather
than the shared view's `{b3}`. `back_solve_own_plays`
(`src/conventions/tiiah/superposition.cpp:450-505`) keeps it narrow on purpose:
only a STABLE call, since a reactive one can name a card that is one away rather
than playable, and only a plain suit, since the arithmetic is a plain prefix.

**What the back-solve learns stays out of the rows** (v17.3.0). Every other seat
watched our card go in and knows what it was, but none of them can know that we now
know it too. A row is one view the two seats of a pair both compute, so it may not
hold that. Until v17.3.0 the back-solve raised every row, and the pair views split on
the spot: Alice's back-solve on Bob's R2 to Cathy raised her rows to red 1, while
Bob's and Cathy's rows for her stayed on red 0. Across
seeds 1–100 this cut the actions that start a pair-view disagreement from 552 to 503.
The row still replays such a card, but only with the set every seat holds for it
(v18.1.0, §1e "worlds are replayed"), so a later argument that both seats of the
pair can follow still reaches the row.

**Rule 5 is the mirror of rule 4** (v16.14.0). The back-solve learns from a call
being HIGHER up its suit than we thought; the refusal learns from one being
BEHIND the stacks altogether. Both reduce to the same accounting: a seat's own
stacks can only be short by what that seat threw in the hole.
`collapse_refused_target` (`src/conventions/tiiah/superposition.cpp:898-922`),
driven from §1c's `read_refusal` rather than from the collapse pass, because the
evidence is the clue being given rather than anything about the cards. Shared,
like rules 1 and 2: every seat watches the refusal.

When **several** of the giver's hole cards admit the refused card, none is settled
(v16.27.0): the refusal says one of them was it, not which. The shared view and every
row are floored at the refused card on its suit — a card that is down has its whole
suit prefix down — and a `HoleRequirement` over those cards keeps the joint fact for
the worlds (`team_learns_already_played`, `:798-809`, shared with rule 8's shared
form). Settling the first by card order was a guess, and replay
[2011854](https://hanab.live/shared-replay/2011854#27) T27 shows the cost: yagami's
o5 `{r1,y1,b1,p1}` (the b1), o18 `{r1,g1,p1}` and o24 `{r3,p1,p2}` (the p1) all
admitted the refused p1, the guess named o5, o24 fell to the r3, and the shared view
went from 23300 to 34301 with red really on 2.

**Rule 6: never presume a strike (v16.16.0).** The default assumption must never
be that a partner's play failed. Our stacks can be short by exactly one thing —
what we threw in the hole — and every partner's play is one we *watched*, so ours
are the only plays missing from them. A partner's play that looks dead is therefore
evidence about **us**:

> Ask which of the worlds our own hole cards leave open would let the card land.
> If any would, those are the only worlds left: every world in which it strikes is
> refuted. Narrow our superpositions to what survives, settle the ones that come
> out singletons, and the play lands.

`presume_play_lands` (`src/conventions/tiiah/superposition.cpp:924-991`), called
from `Game::handle_action` **before** `resolve_hidden_action`, since its answer is
what the resolution's "dead" test then reads (§1.1). It reuses `open_worlds`
whole, so chains and the 64-world cap come for free.

**What every surviving world agrees on, though no card is named (v20.15.0, the
user's ruling).** Pruning narrows each hole card to the identities some surviving
world gives it. When two of our cards could each be the card below the play, neither
narrows, and our stacks, the minimum over every assignment of those sets, still read
the play as a strike.
- So each rank the play needs below it, if several of our cards could be it, is
  recorded as the joint fact the worlds honour (`team_learns_already_played`).
- Our own stacks then take the surviving floor (`presume_own_plays_land`) before the
  resolution reads them (`superposition.cpp:961-990`).

Replay [2019249](https://hanab.live/shared-replay/2019249) T20-T24:
- yagami's b2 went in. will-bot69's o11 and o12, each `{y1,g1,b1}`, could each have
  been the b1 (o11 was).
- will-bot69 booked the b2 as a strike, and then the b3 yagami played at T23 as
  will-bot67's T22 reaction.
- The reaction never resolved as a play: o10 was stamped `{r3}`, and at T24
  will-bot69 chucked it.
- Now blue is on 3 by T24, and o10 is called on the bucket's rainbow, `{m2}`.

`tests/test_tiiah/test_watched_play_lands_on_a_joint_fact.cpp`,
`tests/test_tiiah/test_replay_2019249_watched_plays_land_reaction_reads_rainbow.cpp`.

**SHARED, like rules 1 and 2 (v16.21.0)** — it was private until then, and that cost
a game. Everything the argument needs is held by every seat: they all *watched* our
card go into the hole, they all hold its candidate set (it is built from `common`),
the seat that made the play knows its own reading of what it played, and *never
presume a strike* is the convention rather than one seat's opinion. So the conclusion
"the card they threw was the `b1`" is reachable from every chair, and
`common_play_stacks` and every pairwise row move with our belief.

Replay [2011133](https://hanab.live/shared-replay/2011133#17) is the game the private
form lost. will-bot67 threw a `b1` in the hole at T6 not knowing what it was; at T17
will-bot69's blind `b2` collapsed it, and will-bot67's own blue went `0 → 2` exactly as
the rule intends. But every view a clue is read against stayed on blue **0** — including
will-bot67's own row — so the Blue clue on its `b3` could not mean what the giver meant,
and it discarded its chop, threw the other `b2` away two turns later, and sat on the
`b3` through a second identical Blue clue. All three seats already believed blue was on
1 privately; only the shared vector disagreed.

**And asked of the SHARED view itself (v16.23.0).** The partner form above fires
only when a play looks dead to *our own* stacks — so the seat that could SEE the
missing card never runs it, and its shared view falls behind everybody else's. The
shared view needs a form whose every input is shared:

> A card the whole team could name was played. If it is the next card on the shared
> stacks, they take it. If it lands ABOVE them, ask every seat's hole cards — the
> superpositions all seats hold alike — which strike-free worlds let it land;
> refute the rest, raise the shared stacks to the floor the survivors reach, and
> put the card on top. Whether or not a world names the gap, the team watched a
> card it could name go down, and presumes it landed.

`known_play_lands_in_common` (`src/conventions/tiiah/superposition.cpp:1429-1463`),
from `note_hidden_action`'s known branch. It narrows with `prune_to_worlds(...,
shared=true)` and raises every row to at least the new shared height.

Replay [2011327](https://hanab.live/shared-replay/2011327#11) T11 is the case.
will-bot67 played o17 knowing it was the r2, with red on 0 in the shared view:
will-bot69's two hole cards were o4 `{r1,y1}` and o18 `{r1,r2,y1,y2}`, and only the
worlds in which they were the r1 and the y1, either way round, let the r2 land
strike-free. So red AND yellow are on 1 and the r2 makes red 2. will-bot69 had long
since reached that by the partner form; will-bot67's shared view stayed on red 0
for the rest of the game, and every clue it read against that view with it. After
the fix all three seats hold `22121` at T22 and `43133` at T38.
`tests/test_tiiah/test_known_play_floor.cpp`,
`test_replay_2011327_a_known_play_is_in_every_view.cpp`.

Rule 6's OTHER form stays **private**, and the split is the whole of it: a partner's
play is a public event, while our own plays are judged against our own belief, which no
partner can reproduce. `prune_to_worlds` (`:324-386`) takes the flag.

**And the same rule, asked of our OWN plays (v16.18.0).** A card we threw in the
hole landed or struck and nobody told us which, so it gets the same default: it
landed. Every world in which one of our own hole cards failed is refuted, as long
as some world has none failing — and when every world has a strike in it, the
strike is not an assumption anybody made and there is nothing to refute.
`presume_own_plays_land` (`:1011-1051`) over `strike_free` (`:674-684`), which is the
`OpenWorld::struck` flag `open_worlds` now sets; both forms share the narrowing
half, `prune_to_worlds` (`:324-386`).

**And the height every survivor reaches is one we HOLD (v16.19.0).** Narrowing the
cards is only half of it: two cards each reading `{g1,b1}` were one of each, so green
and blue are both on 1 — a fact about the stacks that no fact about either card
carries. `world_floor` (`:655-670`) is that height, and `presume_own_plays_land`
raises `play_stacks` to it. A row has had the same treatment since v16.18.0 (§1.3),
and until now our own belief did not, which left us reading our own cards on stacks we
could prove were too low.

It raises the stacks with `with_stacks` and **not** `with_play`, so the copies are not
booked as spent: we cannot say WHICH card was the `g1`. A later collapse that names one
settles it and books it exactly once. The accounting therefore lags the stacks by
design, in the direction that only ever under-eliminates.

Replay [2010512](https://hanab.live/shared-replay/2010512#14) is why it has to be our
own belief and not only the rows. §1.3 gives the reacter its OWN stacks to read what
its card is; will-bot67's were two plays short of what it could prove, so the
receiver's `y2` looked one away, the pairing read as a finesse demanding a `y1` the
reacter's slot could not be, and the call died where it was stamped. With the floor its
belief reads `[1,1,0,1,2]`, the `y2` is a direct play, and the bucket names the `b2` it
was really holding.

This reaches cases the partner form cannot, because that one only ever asked how to
rescue a play that looked dead. Here the partner's play is perfectly healthy and it
is OUR card the argument lands on: a partner plays an `r1` while our hole card reads
`{r1,y1,g1,b1,p1}`, and if ours had been the other `r1`, one of the two must have
struck — so it was not.
`tests/test_tiiah/test_own_plays_presumed_to_land.cpp`, and
`test_presumed_landing.cpp` covers the two forms not treading on each other.

It is also what a pairwise row leans on: §1.3 has a partner's row take what holds
in every surviving world of that partner's own plays, and a seat that would not run
this argument about its own cards would not be behind the row we built for it.

Replay [2010296](https://hanab.live/shared-replay/2010296#8) is the case, and the
whole chain of damage from one missing deduction. will-bot69 threw an `{r3, y1}`
at T6; it was the `y1`, so yellow really was on 1 and will-bot67's `y2` at T8
landed. Reading yellow 0, will-bot69 called that a strike — which made the action a
failed *discard*, which skipped `narrow_receiver_call`, which left its called card
reading `{r3, g1, b1, p1}` instead of `{p1}`, which meant it could not name the
card it played at T9, which meant purple never advanced for it, which meant T14's
clue was read against the wrong stacks. Rule 6 settles the `y1` and the rest
follows.

**Rule 7: a named playable thrown away was already played (v16.22.0).** It is rule
6's discard twin. A partner sees every card in the hole but their own, so when they
throw a card the whole team had named — pinned in `common` before the discard, the
same test `useful_dc` asks (`src/basics/decide.cpp:569-584`) — and it looks playable
to us, our stacks are short, and they can only be short by what we threw in the hole:

> Ask which strike-free worlds our own hole cards leave open would have the card
> already played. If any would, those are the only worlds left. If none would, the
> partner simply threw a useful card, and nothing is refuted.

`presume_discard_was_played` (`src/conventions/tiiah/superposition.cpp:1055-1154`),
called from `Game::handle_action` beside `presume_play_lands`
(`src/basics/game.cpp:611-626`), before the dispatch, so `interpret_discard` reads
the advanced stacks. It narrows through `prune_to_worlds` with `shared=true`, for rule
6's reasons: the discard is public, the card's identity was common knowledge, and the
candidate sets are built from `common`.

**Rule 7's shared form (v18.12.0).** The same argument, asked by every seat — the
discarder included — over the strike-free worlds of **every** seat's hole cards on
the shared view (`superposition.cpp:1071-1089`). The card's common reading must be
one identity. If it is trash in some of those worlds, only those survive
(`prune_to_worlds`, shared). If it is trash in all of them or in none, nothing is
learnt. This is how a deferred collapse resolves when the holder throws the card:
at 2014076 T16 every seat learns from blue's throw of the r2 that green's `{r1,r2}`
was the r2 (see *Deferred collapse*, above).

A pink identity clue (§1b) is one way a card comes to be named. Replay
[2015070](https://hanab.live/replay/2015070): at T18 yagami's 1 named barakeel's
known-pink o3 the i1, and at T19 barakeel threw it, so every seat learns that
will-bot67's `{p1,i1}` hole card was the i1, and pink goes to 1. Until v19.0.0
the 1 read as a STALL and o3 kept a stale `{i4}`, so this rule tested the i4 and
learnt nothing.
`tests/test_tiiah/test_replay_2015070_pink_identity_discard_collapses_superposition.cpp`.

**There is no gentleman's discard in TIIAH.** The shared engine reads a named
playable thrown away as "you hold the other copy" (reactor0 GLOSSARY, *sarcastic
discard / gentleman's discard*), and with no copy visible it falls back on "then it
is in mine". Here the same discard means rule 7 instead, so `useful_dc` excludes a
playable card in a hole variant (`src/basics/decide.cpp:571-580`). A sarcastic
reading of a useful card that is NOT playable is unchanged.

Replay [2011319](https://hanab.live/shared-replay/2011319#14) is the case.
will-bot69 threw order 5 (`{g1,b1,p1}`, really the `p1`) and order 9 (`{r2,p2}`,
really the `p2`) into the hole; at T8 its Purple clue called yagami's order 14 as `p1`,
and at T12 she threw it away — correctly, since she could see purple on 2. Read
against purple 0 it looked playable, the gentleman's-discard reading found no `p1` in
any hand, and it pinned will-bot69's only unknown card, order 16, to `{p1}`. At T14 it
played it: a `b2`, a strike. With rule 7 the discard settles order 5 to `p1`, and T14
plays the Red clue's `r2` instead. (Until v16.24.0 order 9 then settled to `p2` as well,
off a Red on order 20 read as exactly `{r2}`; read in every world of our hole cards it is
`{r2, r3}`, so order 9 stays `{r2, p2}` and purple sits on 1, the minimum of the two.)

Rule 3 is **private** — it is `reactor0::sight_narrowed`'s shape, and it reads
our own eyes. It narrows what WE believe and never the set partners predict
from, so it may move `play_stacks` and never `common_play_stacks`, and the
stored set is left as it stands. That split is what keeps a reacter's target
choice predictable: it is made on the shared rule, not on what one seat can see.

When one candidate is left the card leaves the map and the stack it belongs to
advances (`settle`, `:252-291`). A card that did not land is gone all the same,
so it is booked as spent — our accounting has to know, or every count built on
`base_count` stays wrong for the rest of the game. In the example above, Bob
sees Alice put CTP on a purple 1, drops `p1`, is left with `{t1}`, and knows
what he played.

### §1f Rainbowy variants

In a rainbowy variant a non-orange **colour** stable clue pins the CTP to exactly
the next playable card of that colour's **own** suit — not a superposition of it
and the rainbowy suit. Clue red on turn 1 and the receiver writes red 1, and is
*not* superpositioned between red 1 and rainbow 1.

The exception is when that is immediately impossible: if red 5 is already played
and red is clued, the CTP is re-pinned to the rainbowy suit's next playable.
"Impossible" also covers a suit that is dead above its stack — red on 3 with both
red 4s discarded re-pins the same way.

A stable colour clue given **by a superpositioned player** is read under §1e's
rule: if the giver is superpositioned between a purple 1 and a teal 1 and clues
purple, the receiver assumes the purple 1 was not played.

`pin_rainbowy_colour` (`src/conventions/tiiah/interpret_clue.cpp:252-294`) is a
post-step over reactor0's ladder rather than a fork of it: the ladder decides
WHICH card is called and this decides what that card is, by narrowing the new
call to one identity. It runs on the shared view (§1.3). A superpositioned giver
is handled where every stable call is: the reading is widened over the worlds of
the pair's hole cards afterwards (§1e, v16.24.0), so the pin is one identity per
world and the call is their union.

Only a **new** call is pinned. An older one was pinned by its own clue, and §1i
forbids widening an inferred set back out.

**A rank clue on a new slot-1 card is the rainbow (v20.13.0, the user's ruling).**
A rank stable play clue that calls a newly touched card in slot 1 makes it the
rainbowy suit's playable, unless that is directly impossible.
- "Directly impossible" means the rainbowy suit's next card on the shared view is
  not of the clue's rank, is trash, or is not among the card's possibilities.
- A rank clue that calls any other slot is read by reactor0's rank ladder, unpinned.
- `pin_rainbowy_rank` (`src/conventions/tiiah/interpret_clue.cpp:301-322`) runs after
  `stable_rank`, the same post-step shape as the colour pin.

Replay [2019249](https://hanab.live/shared-replay/2019249) T14: yagami's 1 to
will-bot69 touched only its new slot-1 card, o20. It read `{y1,g1,b1,m1}`; it now
reads `{m1}`, its true identity. Until v20.13.0 a rank clue was never pinned (the old
`TiiahRainbowy.ARankClueIsNotPinned`, rewritten with the user's approval).
`tests/test_tiiah/test_rainbowy.cpp`,
`tests/test_tiiah/test_replay_2019249_rank_one_on_new_slot_one_is_rainbow.cpp`.

The rainbowy suit is the one carrying `rainbowish` (Rainbow, Omni), `muddy`
(Muddy Rainbow, Cocoa Rainbow) or `prism` — 16 of the 44 variants have exactly
one. The non-orange proviso is defensive: no TIIAH variant pairs an inverted suit
with a rainbowy one, so the guard has nothing to exclude today.

**The giver's half (v18.19.0): a colour clue never *calls* a rainbow card.**
Since a colour clue's call is read as its own suit, a colour stable play clue whose
receiver would misread the card it calls is never given. It is dropped from the
candidates outright, as an undecodable clue is, so no rung can propose it
(`misreads_its_called_card`, `src/conventions/reactor0/decision.cpp:830-852`,
applied at `:989`). There are two cases:

- the reading of the **called** card, once the clue has landed, excludes the card
  the giver can see;
- the called card is rainbowy and the clue is a colour, unless the reading names
  it outright. That is the re-pin above, when the colour's own suit is impossible.

Replay [2014884](https://hanab.live/shared-replay/2014884#4) T4 (TIIAH & Rainbow):
Bob held an unclued ra1 in slot 1, with a y3 and a p2 beside it. We gave Yellow.
Bob reads that as the y1, so the ra1 would have landed as a y1, and the pin also
made Yellow look like the clue that names its card (§2a criterion 1). Purple, the
next choice, called the same ra1 as `{p2,ra1}`: the pin reads the shared stacks
(purple 0, so the p1), the pair's reading had already ruled the p1 out, and so
nothing was narrowed. Every colour that calls the ra1 is now dropped, and the 1 is
given.
`tests/test_tiiah/test_decision_making/test_replay_2014884_rainbow_one_clued_by_rank.cpp`.

**A rainbow card merely touched is allowed (v18.20.0).** §1f pins only the call,
so a rainbow card the clue touches beside it is not misread. v18.19.0 had a third
case refusing any colour clue that newly touched a useful rainbow card. Replay
[2015013](https://hanab.live/shared-replay/2015013#37) T37 is why it went: Blue to
yagami_black was a play reveal of the clued b3 (§1b) and also touched an unclued
ra3, and refusing it left will-bot67 stalling with a 5 for the third time.
`tests/test_tiiah/test_decision_making/test_replay_2015013_colour_play_reveal_touching_rainbow.cpp`.
In self-play (seeds 1–300, TIIAH & Rainbow) removing the third case took the mean
score from 17.24 to 18.62 and strikeouts from 188 to 174.

### §1h The fix clue (v16.20.0)

A standing call can go bad, and in this variant it can go bad without the holder
being able to tell. A card called to play may be a **duplicate** — the identity has
since gone down — while the holder's own reading still admits a good identity beside
the dead one. Left alone they play it and strike.

> **A card in a partner's hand carries a standing call; the card the giver can see is
> dead; the holder's inference still admits more than one identity, one of them that
> dead card. Then ANY stable clue whose net information — positive touch or negative —
> narrows that card to exactly the dead identity is a FIX CLUE.** It supersedes the
> other stable meanings; the holder withdraws the call and treats the card as known
> trash.

The condition divides in two, and where each half sits is the whole design:

- **that the identity is dead is known to the giver AND the holder**, asked of the
  view those two share (v17.2.0; the shared view until then). A clue only has to mean
  one thing to the two seats it is between (§1.3), and the pair can know a card is
  down that the third seat cannot: that seat's own blind play, which both of them
  watched. That is the duplicate a fix exists for, when the third seat is the one who
  gave the call. A third seat reads the pair's view as the shared one, the floor it
  can compute;
- **that THIS card is that identity is the giver's sight**, and the clue is the thing
  that transfers it.

`dead_call_fix` (`src/basics/fix.cpp:87-111`) is the shared half, read by
`tiiah::interpret_clue` (`tiiah/interpret_clue.cpp:481-510`) **after** the dispatch,
so only a stable clue can be one — the discriminator §1c's refusal uses. The giver's
half is `clue_fixes_dead_call` (`reactor0/decision.cpp:902-912`). It lives in the
engine's fix module rather than in either convention because both of them ask it and
neither may reach into the other.

**The fix withdraws the call itself** (v17.2.0). `Game::on_clue` has already narrowed
the card: its untouched branch differences out the clue's identities exactly as its
touched branch intersects them, so a clue that MISSES the card narrows it just as one
that hits it does. Until v17.2.0 `drop_dead_play_calls` was left to withdraw the call.
It no longer can, because a call live in some shared world is not dead there (§1c,
v16.29.0), and a duplicate's identity is live in the world where the giver's hole card
was something else. So the reading erases the call on the fixed card
(`tiiah/interpret_clue.cpp:481-510`), and it also decides the one thing only it can:
that the clue means the fix and **not** what the ladders would have said.
A fixed REACTION call is not just erased (v20.12.0). It becomes an urgent discard,
which the reacter throws at once, unless he clues and turns the reaction off. That
discard is how the reaction's receiver, who cannot name the card, learns the clue
was this fix (§1c, the deferred read).

Worked from the four clues replay [2010512](https://hanab.live/shared-replay/2010512)
offered will-bot67 at T11, against will-bot69's `{y2,p1}` call on a `p1` with purple
already on 2. **Rank 1** and **colour purple** touch the card and narrow it
positively; **colour yellow** and **rank 2** never touch it and narrow it by negative
touch. All four leave `{p1}`.

**It supersedes rather than riding along**, unlike the refusal, which is an envelope.
That is the ruling, and the cost is real: a negative-touch fix discards whatever the
ladder would have read into the cards it *did* touch, so a colour yellow that fixes
one card cannot also call the yellow it touched. TODO.md 53 records the envelope form.

**Priority: Precedence step 1**, with the refusal, and within step 1 between rung 2
and rung 3 — `rung_2b` (`reactor0/decision.cpp:1722-1726`), logged as `2b.fix`. Step 1
is above the pending reaction because a fix is not an alternative to anything: left
ungiven it is a strike. It also carries an exemption from the tier gate
(`clue_is_admissible`), because a fix stamps nothing, satisfies no arm of `clue_tier`,
and would otherwise be dropped for every OCCUPIED Alice — which is precisely the
position at T11. The refusal has carried the same exemption since v16.27.0 (§1c).

**What this does NOT fix, and why 2010512's strike still happens.** By the time
will-bot67 could have given one, its own model of will-bot69's call had already been
withdrawn: `drop_dead_play_calls` judged the call dead on **will-bot67's private
belief** at T4, while will-bot69 kept it because *its* belief made the `y2` playable.
So the giver no longer believed there was a call to fix, and will-bot69 played the card
from its own pitch list rather than from a call at all. The rule above is sound and
implemented; the game needs the invariant to judge a shared commitment on the shared
view, which is TODO.md 52.

### §1j Playable dupe passback (v16.25.0)

A card in the hole can be a copy nobody has named, so two seats can both be about to
play the same card. The seat that can see it passes its own copy back:

> A seat that can NAME its card as X, and is about to play it, and sees another seat's
> called card that really is X while that seat's reading is wider than {X}, **discards
> its own copy** instead of playing it.

Only when a strike is really coming: with our X on the stacks we share with the other
holder, the rest of that seat's reading must still hold a playable card, or they would
throw theirs away themselves. Replay 2010296 T9 is the control — the other copy read
`{y2, p1}` with yellow on 2, so the p1 is simply played.

**The other holder reads the throw** (`read_passback`): a partner discards a card the
team could name as X while a third seat holds a called card whose reading contains X
but is wider → that card is exactly X, for every seat, and rule 7 stays silent — the
discard explained itself.

`dupe_passback` / `read_passback` (`src/conventions/tiiah/dupes.cpp:57-110`), the first
from `Game::take_action` ahead of the pending reaction (`src/basics/decide.cpp:1685-1694`),
the second from `Game::handle_action` ahead of rule 7 (`src/basics/game.cpp:633-637`).

Replay [2011475](https://hanab.live/shared-replay/2011475#20): at T18 will-bot69's
reaction called yagami's o21, the r4, reading `{g1,b1,r4}` — the g1 or the b1 in the
world her o4 left open. At T19 she named will-bot67's o22 with Red, and it knew that was
the r4. Playing it would have left yagami to play hers as a g1 or a b1 into a strike;
at T20 will-bot67 now throws its r4 instead.

### §1k Playable dupe discharge (v16.25.0; read from the rule, v20.1.0)

A reactive's reacter may find that the card the pairing names is already down: the
giver threw it into the hole without knowing. The reacter **discards** it instead of
playing (`discharge_instead`, and the stamp in `tiiah::interpret_reactive`). The rule:
the reacter **knows the card's precise identity X**, and knows the giver both played
X and is still superposed for it. Ordinary and reverse reactives alike:

- **A finesse on the frame** the reacter shares with the giver names the card exactly
  — the connector. When that connector is already down on the reacter's own stacks
  and is a candidate of one of the giver's hole cards, the card IS the connector: it is
  narrowed to X and the discard button stamped, before any play stamp, and the
  own-stacks reading (§1d) is skipped
  (`src/conventions/tiiah/interpret_reactive.cpp:610-631`, v20.1.0). Replay
  [2018316](https://hanab.live/shared-replay/2018316#9) T8: yagami_black (pink 0 on its
  own frame, having thrown the i1 unknown at T2 as `{p1,i1}`) gave a 5 to yagami_blue,
  holding a standing b2 — a reverse reactive, with yagami_green reacting. Green's slot
  4 paired with blue's i2, one away on black's frame: a finesse naming the i1. Green
  had watched the i1 land, re-read the card on its own stacks as the bucket's `{b3}`,
  and played the i1 into a strike; it now throws it.
- **Otherwise**, when the reacter's empathy leaves a single playable X on that frame
  and X is down on its own stacks, the play button cannot be stamped and the discard
  button is (`interpret_reactive.cpp:637-655`, v16.25.0).

The RECEIVER still **plays** its target when
the thrown card X is a candidate of one of the GIVER's hole cards, and that hole card
settles to X for every seat; otherwise the discard reads as an ordinary reaction
(`discharge_hole_card` / `find_discharge`, `dupes.cpp:112-141`, found in
`Game::handle_action` before rule 7 can settle the same hole card, and resolved through
`reactor0::react_discard(..., as_play=true)`, `src/basics/decide.cpp:600-620`).

The example, replay [2011475](https://hanab.live/shared-replay/2011475#7): will-bot69's
T6 Rank 2 to will-bot67 made yagami the reacter, and the card it named in her hand was
the r1 — which will-bot69 had thrown into the hole at T3 as `{r1,y1}`, still unnamed.
Had yagami discarded the r1, will-bot67 would still have played its r2 and will-bot69's
o10 would have settled to the r1.

## §2 Decision making — reactor0's, with the roles asked rather than assumed

**[reactor0's DECISION_MAKING.md](../reactor0/DECISION_MAKING.md) is the ruling
reference for how this convention decides what to do on its turn.** The General
Clue Evaluation List, the tier gate, Actionable Card Priority and the endgame
stall list are the same rungs, reached through the same
`uses_reactor0_decisions` predicate (true here since v16.6.0), and they are
shared rather than forked for the reason §1b gives: a copy drifts.

What the rungs are told differs, in four places, all of them inside `read_clue`
and its predicates:

| Question | reactor0 | here |
|---|---|---|
| which clue is reactive | positional: any clue not aimed at Bob | the reverse-reactive rule (§1c), through `dispatch_is_reactive` |
| which seat reacts | Bob | Cathy, through `dispatch_reacter` |
| which seat receives | Cathy | Bob — the clued seat |
| who acts first | the reacter, on the very next turn | the reacter too, on an ORDINARY reactive — but on a REVERSE one the RECEIVER, who plays the known play that made the clue reactive, so the reacter's card is judged after it |

The last is the one with teeth. A reverse-reactive finesse names the reacter a
card that is only playable once the receiver has played what he already knows
(§1c's delayed call), so judging it against the stacks as they stand reads it as
a strike — and the clue that gets two plays gets priced as a double discard.
`read_clue` judges the reacter's side against
`stacks_after_queued_plays(game, receiver)` instead
(`src/conventions/reactor0/decision.cpp:240-250`).

Each side of a reading also carries the seat that holds it
(`Designation::holder`), so the rungs that ask "can this hand afford the loss"
or "how good a ditch is this" ask it of the hand that will actually act. That is
v16.5.0's change, and reactor0's own behaviour is unchanged by it.

**Two rungs of this convention's own run ahead of reactor0's** (v16.25.0), both from
§1: the PASSBACK (§1j) before the pending reaction, since playing a card whose unnamed
copy is called elsewhere leaves that copy to strike; and the DISCHARGE (§1k) inside
the pending reaction itself, where the reaction card's play button becomes a discard.
Both log a DECIDE branch (`tiiah.dupe_passback`, `tiiah.discharge`).

**The endgame solver stands down here**, as §0 says: our own hidden plays leave
its card accounting inconsistent and it declines rather than solving wrongly, so
those turns are decided by the stall list and the ordinary rungs.

### §2a Prefer a play clue the receiver can NAME (v16.7.0), and the stable play hierarchy (v18.17.0)

**Among stable play clues, take one whose called card the receiver can read back
to a single identity.** In practice that is a colour clue: a rank clue names a
rank, and while several suits are waiting at that rank it names no card.

The reason is §1e. A play the receiver cannot name goes into the hole unnamed —
they learn nothing from it, so it stamps a superposition instead of advancing
`common_play_stacks`, and every clue after it is read against a staler shared
view. A named play advances the shared stacks for everybody.

Replay [2008145](https://hanab.live/shared-replay/2008145#1) T1 is what the rule
was written for. Stacks empty, Bob holding `g1 r1 y1` on slots 1-3, and rank 1
touches all three — so it wins the default tiebreak 5.97 to 1.99 and was given
twice in that game. It left Bob's called card inferred `{r1,y1,g1,b1,p1}`. Green,
red or yellow each name their card outright, and any of the three is better.

**The exception is not a special case.** A rank clue is a direct play call only
when every useful identity of that rank is playable (`playable_rank`,
`src/conventions/reactor0/interpret_clue.cpp`), so when one identity of the rank
is playable and the rest are trash, the call already means exactly one card. The
rule then separates nothing and the default tiebreak decides, as it does under
reactor0. A play reveal names its card by construction and is likewise never the
thing this demotes.

**The stable play hierarchy (v18.17.0) is the general form.** When several
stable play clues exist, they are tiebroken in this order. Each key is judged over
what the key above it left. All are read from the giver's private model of the
receiver once the clue has landed (`hypo.players[receiver]`):

1. **The fewest identities left on the called card** — the rule above, as a
   count.
2. **The most ancillary value.** It is 1.99 per newly touched good card besides
   the called one, less 1 per newly touched trash card the receiver cannot tell
   is trash. Known trash is judged on the stacks the called play leaves, so a card
   that can only be the called identity's dupe costs nothing.
3. **The smallest product of candidate counts over the receiver's other good
   cards**: the clue that leaves the rest of their hand best known.
4. **Colour over rank**, unless the colour could mistake a rainbow card for its
   suit.

The default tiebreak comes last.

The hierarchy only ranks clues that survive `misreads_its_called_card` (§1f, the
giver's half, v18.19.0). A colour clue that calls a card which really is rainbow
never reaches it, so a wrong §1f pin cannot pass for a name under criterion 1. Criterion 4 still demotes a colour clue whose touched cards are not
rainbow but could, to the receiver, be.

The keys are `ClueCandidate::target_inferences`, `ancillary_score`,
`others_product` and `colour_ok`, filled in `analyse_clues`. `settle_stable_play`
applies them (`src/conventions/reactor0/decision.cpp`). It is the tiebreak of
every stable-play rung: 3.1, 3.6b, 4.1, the refusal's stable play, and the endgame
stall list's rung 2. Outside this variant it is the default tiebreak alone, so no
other convention moves. `names_its_card` stays on the candidate as a record and is
no longer ranked; criterion 1 subsumes it.

Human diagnostic 2014561 (`v18_human_vs_bot_diagnostics/2014561.md`):
- **T50:** blue had three stable plays to black: Yellow (the y4), 3 and Purple
  (both the p3).
  - All three name their card, and none touches anything else of use. Purple's p1
    can only be the p1 or the p3 being called.
  - Purple also pins the clued o13 as the p4. Yellow and 3 leave it with three
    and four identities. The product (criterion 3) comes to 176 for Purple against
    540 and 680.
  - The default tiebreak had given the 3. It now gives Purple.
  - `tests/test_tiiah/test_decision_making/test_replay_2014561_stable_play_hierarchy_prefers_purple.cpp`,
    and one case per criterion in
    `tests/test_tiiah/test_decision_making/test_stable_play_hierarchy.cpp`.
- **T49:** stated criterion 1 (Green's `{g4}` over rank 4's `{r4,y4,g4}`). There,
  by ruling, the 4 stands: green could not know its own hole card had been the g3.

It is a TIEBREAK, not a veto: it orders the stable-play pool and never changes
which rung fires, so a clue that names its card cannot displace a better rung.

### §2b Among reactive plays, the fewest candidates for the receiver (v16.28.0)

**Priority 1's first tiebreak: of the reactive play clues, take the one that leaves
the RECEIVER the fewest identities for its called card.** It is §2a's argument asked
of a reactive: the receiver's reading is §1d's union of the bucket and the finesse,
intersected with what the clue leaves the card, and a clue whose touch — or miss —
rules out part of that union says more for the same two plays.

Replay [2011885](https://hanab.live/shared-replay/2011885#32) T32 (no longer under test: v18.0.0 reads that game's T8 clue differently): Red and Rank 1 to
will-bot69 both paired yagami's n2 with will-bot69's n3, and both are legal. Under
Rank 1 the bucket half adds the r5, so will-bot69 would read `{r5,n3}`; the Red did
not touch the n3, which rules the r5 out, and it reads `{n3}`. Every other term tied,
and the default tiebreak had taken the Rank 1.

The prediction is `tiiah::annotate_candidate`
(`src/conventions/tiiah/interpret_reactive.cpp:1289-1317`), which reads the reacter's
card by sight, the frame the giver shares with the receiver advanced by that card, and
the pair's worlds, through the same `receiver_reading` helper (`:588-634`) that
`narrow_receiver_call` uses — so the giver and the reader cannot disagree about what a
call says. It writes `ClueCandidate::receiver_reading_size`, which `rung_1`
(`src/conventions/reactor0/decision.cpp:1365-1387`) reads first. reactor0 cannot call
into this convention, so `reactor0::analyse_clues` takes an optional
`CandidateAnnotator` and the engine passes this one under TIIAH
(`candidate_annotator`, `src/basics/decide.cpp:49-52`); outside TIIAH the field stays
0 and the term separates nothing.

### §2c Never call a card we may already have played (v17.2.0), unless the call is named (v18.18.0)

Our stacks are the minimum across the worlds of our own hole cards. So a card that is
playable on them can still be a duplicate of one we threw in without naming it.
Calling it asks a partner to play into a strike in that world, and the partner who
watched our card go in cannot be told why, because the call reads as sound on the frame
we share. So a clue is not offered when it newly calls to play a card, whether the
stable target, the reacter's card or a reactive's receiver target, that is basic trash
in some strike-free world of our own hole cards. That is
`calls_a_card_we_may_have_played` (`reactor0/decision.cpp:738-777`), a candidate filter
in `analyse_clues` beside `calls_two_copies_to_play` (`:640`). Like that one, it reads
the giver's sight, so it changes which clues are given and never what a clue means.
Without it, a giver whose own `{g1,b1}` or `{r1,y1}` hole card was the very card it
called on a partner produced a strike. On seeds 1–100 it took the mean score from 14.75 to 17.53 and strikeouts from 69 to 47.

**Except a named stable call (v18.18.0).** A stable play clue is exempt when its
holder can name the called card exactly once the clue has landed (the hypo's model of
the receiver holds one identity). The holder watched our card go into the hole, so it
can check the call against it: if our card was that identity, it reads the call as
the dupe and throws the card (§1.3's receiver rule, `repin_own_call`). A call left
with several identities keeps the veto, since the holder cannot tell which we meant.
Since v20.4.0 our own hole cards no longer widen the stable reading (§1e), so a call
is named whenever the receiver's own hole cards leave it one identity. In replay
2018435 T11, will-bot69's Green on black's clued ra1 became `{ra1}` and is given. Its
Yellow on black's y1 stays `{y1,y2}` through black's own `{r1,y1,g2,g3}`, so it is
still vetoed.

Human diagnostic 2014561 T56:
- blue's unknown 4 had just gone into the hole, so the veto dropped Yellow (black's
  y4) and Purple (the clued p4);
- blue revealed a trash p1 instead;
- it now gives Yellow.

`tests/test_tiiah/test_decision_making/test_replay_2014561_named_stable_play_past_own_dupe_veto.cpp`.

**Except Bob's chop, when Bob is stuck with it (v18.7.0).** When §3's precondition
holds (`priority_3_applies`: Bob's chop is at risk or playable, and he has no safe
action), the filter spares Bob's chop (`analyse_clues`, `reactor0/decision.cpp:957-961`, applied at `:922`).
The choice there is between a possible duplicate and a certain loss, and a human
saves the card.

The motivating case is human diagnostic 2013726 T17
(`v18_human_vs_bot_diagnostics/2013726.md`). Blue's chop was a playable g1, and
blue had nothing to clue. Green's own `{r3,g1}` hole card vetoed every clue
calling it, so green played its b3 and blue threw the g1. v16.29.0, before the
filter, gave the 1. Tests:
`tests/test_tiiah/test_decision_making/test_own_dupe_filter.cpp` (the veto of an unnamed call, a named call given (v20.4.0), and the
chop spared) and
`tests/test_tiiah/test_decision_making/test_replay_2013726_save_playable_chop_despite_own_dupe.cpp`.

### §2d Never call a critical card to discard (v17.4.0)

A discard call throws the card away without anyone checking it is safe, and in
this variant a critical card lost is 25/25 lost. So a clue is not offered when it
newly calls a partner to discard a card we can see is critical
(`calls_a_critical_card_to_discard`, `reactor0/decision.cpp:791-805`, a candidate
filter in `analyse_clues` at `:860`). An inverted card is left out, because its
Discard button plays it. For example, a Rank 3 to Bob could read as a discard call on
the only y5. Across seeds 1–100, 13 of the 70 critical cards
thrown away had been called to discard. After the filter there were none.

### §2e An occupied Alice saves a stuck Bob's playable chop when Cathy's is safe (v18.13.0)

An OCCUPIED Alice, one holding a call she can action, gives only HIGH clues
(reactor0's tier gate, `clue_is_admissible`). A save of Bob's chop reaches HIGH only
through H1, which also asks that Bob could not have handled Cathy himself (H1c). So
when Bob is stuck on a playable chop at risk and could clue Cathy, Alice
played her call and Bob threw his chop. The reviewer: "Bob almost never stops to get
or save a playable card at risk on Cathy's chop."

So a clue to Bob that touches his chop, without calling it to discard, is flagged
`saves_stuck_bob_chop` (`reactor0/decision.cpp:1129-1132`) and exempt from the gate
(`:1157`) when all of the following hold:

- §3's precondition holds (`priority_3_applies`): Bob has no safe action.
- Bob's chop is a **playable card at risk** (`:894-911`):
  - it is playable (`has_playable_chop`);
  - it is at risk (`at_risk_chop`): no copy in a hand Alice can see, and none she
    can prove she holds;
  - it is not duplicated (`chop_is_duplicated`, `:851-865`): no copy in his own
    hand, and no called card anywhere, ours included, whose common reading is
    that identity.
- Cathy's chop is **safe to lose** (`cathy_chop_is_safe_to_lose`, `:867-884`):
  - it is not critical;
  - it is not playable, unless it is duplicated, either by another copy in
    Cathy's own hand or by a called card in anyone else's hand whose common
    reading is that one identity.

  A locked Cathy is safe.

Rung 3 then chooses among the admitted saves as usual.

Human diagnostic 2014076 T18 (`v18_human_vs_bot_diagnostics/2014076.md`): light's
Blue had called green's b2. Blue's chop was a playable g1, and light's chop an n3
with brown on 1. Green played the b2 (`tier_gate_rejected_all`), and blue threw the
g1. It now gives Green to blue. Tests:
`tests/test_tiiah/test_decision_making/test_replay_2014076_save_bobs_playable_chop_while_occupied.cpp`
and `tests/test_tiiah/test_decision_making/test_occupied_save_of_bobs_chop.cpp`:
- saved when Cathy's chop is a safe r3;
- saved when it is a playable b1 that Alice's own known call duplicates;
- not saved when it is a critical y5, or an unduplicated playable y1;
- not saved when Alice's own known call duplicates Bob's g1, or Cathy holds the
  other g1 in plain view.

The two Bob's-chop conditions came from self-play (v18.13.0, seeds 1–300). With only
§3's "at risk or playable", the rule also fired on a chop that was playable but not
at risk. At seed 105 T2, the other copy was in plain view in Cathy's hand. At seed
119 T17, our own call named it, so the save called a second y1 to play. Against
v18.12.0 that version lost 4 perfect games and gained 8 strikeouts. With both
conditions the difference is noise: 29 of 300 seeds move, 16 up and 13 down.

### §2f A locked Alice's stable play may go to Cathy (v18.15.0)

Section 4.1 of reactor0's list, a locked Alice's first choice, is "same as 3.1": a
stable play clue to Bob. Here role inversion (§1c) makes a clue to **Cathy** stable
too, whenever Bob holds a standing play and Cathy does not. That clue gets a card
played exactly as one to Bob does. So 4.1's pool takes a stable play clue to either
partner (`pool_stable_play_any_partner`, `reactor0/decision.cpp`). Rung 3.1 keeps
the Bob-only pool, since it exists for Bob's chop.

Human diagnostic 2014538 T23 (`v18_human_vs_bot_diagnostics/2014538.md`):
- blue was locked with 2 tokens, and green, blue's Bob, held a called n2;
- Green to black was a stable play on black's g3, which blue could see was playable;
- 4.1 never saw it, and 4.5 gave a stalling 3.

"A color stable clue ... should definitely outrank a rank 3 lock clue." It now
gives the Green.
`tests/test_tiiah/test_decision_making/test_replay_2014538_color_stable_play_over_rank_stall.cpp`.

### §2g A stuck Bob gets a play before a lock (reactor0 rung 3.6b, v18.16.0)

A change to reactor0's shared ladder, which this convention reaches through the
delegation above. When rung 3.7 would lock a stuck Bob, a stable play clue to him
takes the lock's place, provided there are fewer tokens than 3.1 needs and his
chop is not critical (reactor0 `DECISION_MAKING.md` priority 3, item 6b). It only
ever replaces a lock.

Human diagnostic 2014538 T24:
- green, on one token and holding a called n2, locked black with a 4 on a
  non-critical y4 at 3.7;
- that left black no safe action, when Green would have had him play the g3.

It now gives the Green.
`tests/test_tiiah/test_decision_making/test_replay_2014538_stable_play_over_lock_at_one_clue.cpp`.

### §2h No lock over a chop worse than one away (reactor0 rung 3.7, v20.2.0)

The user's ruling: the variant is hard, so rung 3.7 locks a stuck Bob only when his
chop is **critical, playable or one away from playable**, judged on Alice's own
stacks (`chop_worth_a_lock`, `src/conventions/reactor0/state_eval.cpp:167-179`, read
at `src/conventions/reactor0/decision.cpp:1801-1814`). Anything further away is not
worth committing his whole hand for, and Alice does something else instead, most
often her own standing play. 3.6b (§2g) goes with the lock it replaces. 3.6 and 3.10
already need a critical chop, and §4's lock, the forced branch, is unchanged. This
holds only under this variant; reactor0 keeps the lock.

Human diagnostic [2018365](../../../v18_human_vs_bot_diagnostics/2018365.md) T7:
- green, holding a called y1 at 7 tokens, gave black a 2 that touched the r2 on his
  lock slot, a lock;
- black's chop, the newest unclued card, was a y4 on empty yellow, three away.

Green now plays the y1.
`tests/test_tiiah/test_decision_making/test_replay_2018365_no_lock_over_far_chop.cpp`,
`tests/test_tiiah/test_decision_making/test_no_lock_over_far_chop.cpp`.

### §2i No forced clue at 8 tokens while Alice can play (reactor0 §4, v20.2.0)

A change to reactor0's shared ladder, for every convention that delegates to it. At 8
tokens reactor0 §4, the forced-clue list whose floor always returns a clue, now opens
only when Alice has no known play, or Bob is stuck on a hidden playable that a clue
moves now, or a clue gets two plays (4a-4c, the pace arm's own qualifiers;
`priority_4_applies`, `src/conventions/reactor0/decision.cpp:2018-2029`). A discard
is illegal at 8 tokens, but a play is not.

Human diagnostic [2018365](../../../v18_human_vs_bot_diagnostics/2018365.md) T10: at 8
tokens green, still holding the called y1, gave Blue on black's already-clued b3. The
clue saved nothing, and black's chop was a same-hand dupe. Green now plays the y1.
`tests/test_tiiah/test_decision_making/test_replay_2018365_play_over_forced_clue_at_eight.cpp`.

### §2j A spent reaction stops being urgent (reactor0 Precedence step 2, v20.7.0)

reactor0's rule since v9.3.0: a reaction stops being urgent once its target has left the
receiver's hand. Call invariant rule 0 (`relegate_spent_reactions`) clears `urgent`, so the
call is relegated to a receiver-CTP and no longer outranks every clue. The rule reads
`ConvData::react_target_order`, which only reactor0's walk wrote. This convention's walk
now writes it too, wherever the reacter's call is made: the walk and the §1k discharge
(`src/conventions/tiiah/interpret_reactive.cpp:757-768`), and the ASCR pairing (`:492-496`).

Human diagnostic [2018759](../../../v18_human_vs_bot_diagnostics/2018759.md) T34:
- black's T30 3 made will-bot67 the reacter on its m3, and it deferred;
- will-bot69 played the paired target at T32;
- at T34 the m3 was still urgent and played ahead of the §2e save of will-bot69's
  playable b2 chop (black's chop a trash m1).

It now gives Blue.
`tests/test_tiiah/test_decision_making/test_replay_2018759_spent_reaction_not_urgent_saves_bobs_chop.cpp`,
`tests/test_tiiah/test_spent_reaction_relegated.cpp`.

### §2k A call that plays in every world of our hole cards, or on the shared view, is a known play (v20.9.0, v20.11.0)

The user's ruling. A call on our own card can read as several identities, none of them
playable on our belief, and still play whichever world of our own hole cards we are
in. Some identity of it is playable in **each** strike-free world. Such a call is a
**known play**. That matters for:
- the loaded and locked tests (`thinks_loaded` / `thinks_locked`);
- *occupied*;
- reactor0 §4's 4a ("Alice has no known playable");
- the play phase.

All of them read the one per-card test `Player::order_playable`
(`src/basics/player_game.cpp:162-190`), which asks
`tiiah::plays_in_every_own_world` (`src/conventions/tiiah/superposition.cpp:1225-1234`).
It applies only to our own seat's cards, since we reason only over our own hole cards,
and only with at least two worlds.

**A standing call that plays on the SHARED view is a known play too, when only our
own hole cards hold our stacks back (v20.11.0, the user's ruling).** The shared view
is the frame every seat reads the call on. Which world of our own hole cards we are
in does not matter to a standing play call: it plays now
(`tiiah::plays_on_shared_view_past_own_hole`,
`src/conventions/tiiah/superposition.cpp:1236-1257`, read at `player_game.cpp:183-189`).
Each identity of the reading must:
- be playable on the shared view;
- not be trash on our own view;
- have every card between our own stack and it among the candidates of our own
  unnamed hole cards. The shared view counts that card as down, and we cannot name it.

When our view is behind the shared one for any other reason, our view knows better.
The first cut asked only the shared view, and in self-play 12 of the 23 plays it
added struck. Two shapes:
- **Our view ahead.** Seed 289 T5 played an `{r1}` call while the shared view had red
  on 0, though the seat knew red was on 1.
- **The shared view wrong.** Seed 21 T58 played an `{r2}` call with red on 1 shared,
  but on 0 for the seat and in truth, and no red among its own hole cards.

Replay [2018874](https://hanab.live/shared-replay/2018874) T47:
- will-bot69's o44 was stable-called `{b5}` at T46. Blue was on 4 in every shared view.
- Privately blue was on 3: will-bot69 had seen that yagami's hole card o0 was the g4,
  and its own hole card o29 `{g4,b4}` was unnamed. The missing b4 is a candidate of
  o29, so the arm applies.
- Before v20.11.0, `{b5}` was neither playable on its belief nor in every own world,
  so §4's 4a opened and it gave a 3 stall. It is not the endgame solver, which returned no move.

It now plays o44.
`tests/test_tiiah/test_decision_making/test_replay_2018874_standing_call_on_shared_view_plays_now.cpp`,
`tests/test_tiiah/test_standing_call_survives_and_plays.cpp`.

Replay [2018766](https://hanab.live/shared-replay/2018766) T23:
- will-bot69's o9 was called as `{r2,y2}` (§1d, v20.8.0). That is the r2 where its hole
  card o7 was the r1, and the y2 where o7 was the y1.
- No single identity played on its belief, so will-bot69 counted itself locked and
  unloaded, and §4 gave yagami a Red.

It now plays o9.
`tests/test_tiiah/test_decision_making/test_replay_2018766_call_in_every_world_is_a_play.cpp`,
`tests/test_tiiah/test_call_in_every_world_is_a_play.cpp`.

## Test coverage

| File | What it pins |
|---|---|
| `tests/test_tiiah/test_variant_flag.cpp` | the flag parses, agrees with the name on all 2426 variants, and lands on exactly 44 |
| `tests/test_tiiah/test_convention_predicates.cpp` | `is_reactor0_family` / `uses_reactor0_decisions`, and that both are an identity transform on reactor and reactor0 |
| `tests/test_tiiah/test_buckets.cpp` | §1a's four rows, the inverted re-indexing, and `bucket_of` |
| `tests/test_tiiah/test_engine_rules.cpp` | §1.1's table and §1.2 — a partner's hidden play advancing our stacks, our own leaving them alone, a hidden misplay striking, a hidden 5 paying nothing, and both sides of the orange mirror |
| `tests/test_tiiah/test_gate_and_clues.cpp` | §0 — `take_action` answers, and with a legal move; §1b read identically to reactor0 (a differential test); §1c refusing rather than guessing |
| `tests/test_net/test_tiiah_commands.cpp` | §0 — `/setall tiiah` refused, the variant selecting the convention, and the `/settings` line carrying the buckets and the dispatch |
| `tests/test_tiiah/test_decision_making/test_colour_preference.cpp` | §2a — replay 2008145 T1 giving a clue that names its card, the rank candidate naming none while all three colours do, the exception separating nothing, and reactor0 unmoved |
| `tests/test_tiiah/test_decision_making/test_stable_play_hierarchy.cpp` | §2a — the stable play hierarchy, one tie per criterion: fewest inferences, ancillary value (unknown trash costs), the product over the other good cards, colour, then the default tiebreak (v18.17.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2014561_stable_play_hierarchy_prefers_purple.cpp` | §2a — human diagnostic 2014561 T50: blue gives Purple to black (criterion 3), not the 3 (v18.17.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2014561_named_stable_play_past_own_dupe_veto.cpp` | §2c — human diagnostic 2014561 T56: blue gives Yellow to black though its unknown 4 may have been the y4, since black can name the call (v18.18.0) |
| `tests/test_tiiah/test_named_call_on_a_watched_dupe.cpp` | §1.3 — a named stable call on an identity the holder watched the giver play into the hole reads as the dupe: known trash, call withdrawn; the control keeps a live call (v18.18.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2014538_color_stable_play_over_rank_stall.cpp` | §2f — human diagnostic 2014538 T23: locked blue gives Green to black for the playable g3 (a role-inverted stable play in 4.1), not a stalling 3 (v18.15.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2014538_stable_play_over_lock_at_one_clue.cpp` | §2g — human diagnostic 2014538 T24: green on one token gives black Green for the g3 (reactor0 rung 3.6b), not a 4 locking a non-critical chop (v18.16.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2018365_no_lock_over_far_chop.cpp` | §2h — human diagnostic 2018365 T7: green, holding a called y1, plays it rather than lock black over a y4 chop three away (v20.2.0) |
| `tests/test_tiiah/test_decision_making/test_no_lock_over_far_chop.cpp` | §2h — `chop_worth_a_lock` reads critical, playable and one away as worth a lock, and two away or trash as not; a far chop gets neither the lock nor 3.6b's play, a one-away chop still reaches 3.7, and reactor0 keeps the rung (v20.2.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2018365_play_over_forced_clue_at_eight.cpp` | §2i — human diagnostic 2018365 T10: at 8 tokens green plays its called y1 rather than give a section-4 clue that saves nothing (v20.2.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2018759_spent_reaction_not_urgent_saves_bobs_chop.cpp` | §2j — human diagnostic 2018759 T34: will-bot67's deferred m3, whose target will-bot69 has played, is no longer urgent, and it gives Blue on will-bot69's playable b2 chop (v20.7.0) |
| `tests/test_tiiah/test_spent_reaction_relegated.cpp` | §2j — a deferred reaction whose target the receiver plays is relegated (still called, not urgent); while the target stays it stays urgent (v20.7.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2018766_call_in_every_world_is_a_play.cpp` | §2k — replay 2018766 T23: will-bot69's o9 `{r2,y2}`, the r2 or the y2 by the world of its own hole card, is a known play, so it plays it rather than counting itself locked (v20.9.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2018874_standing_call_on_shared_view_plays_now.cpp` | §2k — replay 2018874 T47: will-bot69 plays its standing `{b5}` call (blue on 4 in every shared view, on 3 privately) instead of a §4 stall (v20.11.0) |
| `tests/test_tiiah/test_replay_2018874_standing_call_survives_a_new_call_right.cpp` | §1c — replay 2018874 T49: yagami's reaction calls will-bot69's o23 (slot 5) and its standing `{b5}` call on o44 (slot 1) survives (v20.11.0) |
| `tests/test_tiiah/test_standing_call_survives_and_plays.cpp` | §1c and §2k — a newer call to the right leaves a standing call alone (No Variant still erases); our call playable on the shared view, with our stack behind only for our own hole card, is a known play; not one playable on no view, nor with no hole card of ours behind the gap, nor one our own view knows is trash (v20.11.0) |
| `tests/test_tiiah/test_replay_2019249_receiver_of_owed_reaction_waits.cpp` | §1c — replay 2019249 T4: will-bot69 waits on will-bot67's Yellow until yagami's owed T1 reaction (o7) plays, then reads the reverse reactive and plays its o15 `{g1,g2,b1,b2}` (v20.12.0) |
| `tests/test_tiiah/test_deferred_read.cpp` | §1c — the deferred read: we wait while Cathy owes us a reaction, read the reverse reactive once her owed card plays, and read no call when she discards (v20.12.0) |
| `tests/test_tiiah/test_dupe_ones_flip_the_position.cpp` | §1c — sure 1s make a clue to the giver's Cathy stable; a 1 possibly in the hole makes it a reactive (replay 2019249 T2, v20.12.0) |
| `tests/test_tiiah/test_fixed_reaction_card.cpp` | §1c and §1h — waiting on a clue to the reacter: his discard of the fixed reaction card reads it as the fix, voids the reaction and settles our hole card as the thrown b1; a clue from him turns the reaction off (self-play seed 259, v20.12.0) |
| `tests/test_tiiah/test_replay_2019249_rank_one_on_new_slot_one_is_rainbow.cpp` | §1f — replay 2019249 T14: yagami's 1 on will-bot69's new slot-1 card reads `{m1}` (v20.13.0) |
| `tests/test_tiiah/test_watched_one_floors_the_row.cpp` | §1.3 — a watched y1 floors the pair's row though our own stacks call it a strike, and two copies of a 1 leave the row on 1 (replay 2019249 T33, v20.14.0) |
| `tests/test_tiiah/test_watched_play_lands_on_a_joint_fact.cpp` | §1e rule 6 — a watched b2 lands when either of our two hole 1s could be the b1 (the joint fact), and a b3 lands in no world (v20.15.0) |
| `tests/test_tiiah/test_replay_2019249_watched_plays_land_reaction_reads_rainbow.cpp` | §1e rule 6 — replay 2019249 T20-T24: yagami's b2 and b3 land at will-bot69's seat, and o10 is called `{m2}` and played (v20.15.0) |
| `tests/test_tiiah/test_named_hole_card_above_the_shared_view.cpp` | §1e — a hole card named as the y2 above an unnamed gap settles the other hole card as the y1 and floors common yellow at 2; with no y1 among it, the gap card stays superposed (v20.16.0) |
| `tests/test_tiiah/test_receiver_standing_play_is_not_the_reaction.cpp` | §1d — on the reverse arm the receiver's standing play is not read as the reacter's card (self-play seed 270, v20.16.0) |
| `tests/test_tiiah/test_replay_2019408_named_hole_card_above_shared_view.cpp` | §1e — replay 2019408 T35: yagami's o3 stays `{r1,y3}`, and his T34 Red calls o36 `{r1}`, played (v20.16.0) |
| `tests/test_tiiah/test_decision_making/test_replay_9000259_fixed_reaction_card_is_thrown.cpp` | §1c and §1h — self-play seed 259 T5: Bob throws his fixed reaction card o7 at once (v20.12.0) |
| `tests/test_tiiah/test_call_in_every_world_is_a_play.cpp` | §2k — our call `{r2,y2}` over our own hole card `{r1,y1}` is a known play; with `{r1,g1}` a world plays neither and it is not (v20.9.0) |
| `tests/test_tiiah/test_replay_2018766_receiver_conditional_reading_keeps_the_call.cpp` | §1d and §1e — replay 2018766 T23: will-bot69's o9 keeps its reaction call as `{r2,y2}`, the r2 and the y2 each in the world of its own hole card, rather than being reset (v20.8.0) |
| `tests/test_tiiah/test_decision_making/test_occupied_save_of_bobs_chop.cpp` | §2e — an occupied Alice saves a stuck Bob's playable g1 when Cathy's chop is a safe r3 or a playable b1 her own known call duplicates, and plays her call when it is a critical y5 or an unduplicated y1, when her own known call duplicates Bob's g1, or when Cathy holds the other g1 in plain view (v18.13.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2014076_save_bobs_playable_chop_while_occupied.cpp` | §2e — human diagnostic 2014076 T18: occupied green gives blue Green (or 1) for the playable g1 on chop, not its own b2 play (v18.13.0) |
| `tests/test_tiiah/test_decision_making/test_clue_reading.cpp` | §2 — a clue to Bob read as reactive with Cathy reacting, the delayed connector read as a PLAY rather than a strike, a clue to Cathy read stable, and the double-play reactive chosen end to end |
| `tests/test_tiiah/test_receiver_bucket.cpp` | §1d's receiver half — a colour clue leaving the bucket below, the finesse continuation surviving when the clue does not rule it out, a rank clue reading the other way, and the reacter's own seat reading wider |
| `tests/test_tiiah/test_replay_2010329_receiver_reads_its_own_worlds.cpp` | §1d's receiver half over the worlds — the live game where the `r3` was playable in only one of them |
| `tests/test_tiiah/test_replay_2010329_a_withdrawn_call_keeps_its_inference.cpp` | §1i — a withdrawn call keeps its `{p1}`, which the reaction's negative used to widen away |
| `tests/test_tiiah/test_replay_2008217_receiver_misses_the_bucket.cpp` | the live game it was missing in, replayed |
| `tests/test_tiiah/test_replay_2008422_receiver_play_in_the_hole_is_not_a_discard.cpp` | §1d's negative half — our own hidden play read as a play rather than a discard, so a later referential discard on the same hand survives |
| `tests/test_tiiah/test_pairwise_stacks.cpp` | §1.3 — a partner's blind play reaching every row but theirs, our own reaching none, a known play reaching all, a play both seats of a pair watched raising their row as a floor over a card one of them cannot name (the prefix rule before v16.23.0), and the symmetry of `stacks_known_to_both` |
| `tests/test_tiiah/test_conditional_reading.cpp` | §1e's worlds — one world when nothing is in the hole, one per candidate when something is, whose seat they belong to, two cards multiplying and chaining in play order, and the cap reading it flat |
| `tests/test_tiiah/test_presumed_landing.cpp` | §1e rule 6 — the world that lets a partner's play land is the one we are in, a strike no world rescues still stands, and a play that already lands needs no rescuing |
| `tests/test_tiiah/test_own_plays_presumed_to_land.cpp` | §1e rule 6 on our OWN plays — our hole card cannot dupe a partner's play, nothing is refuted while every world lands, and `strike_free` standing everything down when every world strikes |
| `tests/test_tiiah/test_row_from_own_worlds.cpp` | §1.3 — two `{g1,b1}` holes raising their owner's row, one hole raising nothing for its owner but still reaching the seat that watched it, a lone hole raising the row when only one world lands, and our own hole card withholding that claim |
| `tests/test_tiiah/test_pair_visible_copies.cpp` | §1e rule 3's pair form — copies outside the pair settling for the pair (and not for the seat holding them), and a copy in the partner's own hand not counting |
| `tests/test_tiiah/test_replay_2010329_the_frame_is_the_pair_not_the_team.cpp` | §1d's frame — the live game the shared view cost: the pair's row, the call surviving Rule 5, the `{g2,b2}` reading, and the `b2` played |
| `tests/test_tiiah/test_shared_rule_six.cpp` | §1e rule 6 SHARED — a partner's rescued play moving the shared view and every row, our own collapse moving neither the shared view nor the row of the seat that cannot see its card, only the row of the seat that watched both (v18.1.0), and a frozen clue frame raised by the settle but never lowered |
| `tests/test_tiiah/test_replay_2011133_rule_six_is_shared.cpp` | the live game the private form lost: the collapse, blue on 1 in the shared view and every row, the promise surviving Rule 5 as `{b3}`, and the b3 played |
| `tests/test_tiiah/test_replay_2011319_discarded_playable_was_already_played.cpp` | §1e rule 7 — the live game the gentleman's discard cost: yagami's thrown `p1` settling our o5 (our Purple read `{p1,p2}` up to its worlds, v16.24.0), o9 left `{r2,p2}` so purple is on 1, order 16 left unpinned, and the `r2` played instead of the `b2` |
| `tests/test_tiiah/test_world_feasibility.cpp` | §1e — a finesse never called ahead of a direct playable, one card in the hand constraining nothing, every world infeasible standing the check down, worlds replayed in play order, and a floor replayed on itself staying put (the band) |
| `tests/test_tiiah/test_stable_clue_over_worlds.cpp` | §1e — a stable call read as the next card in each world of the pair's hole cards, the other half withdrawn when the hole card settles, and no widening with nothing in the hole |
| `tests/test_tiiah/test_replay_2011397_the_frame_is_the_minimum_across_worlds.cpp` | the live game: the T10 Blue read `{b2,b3}` with o9 left `{b2,p2}`, the T6 reaction settling both hole cards so every view is 10131 at T14, the p2 as the target and slot 1 answered, and the other bot reaching the same views |
| `tests/test_tiiah/test_dupes.cpp` | §1j and §1k — our copy of an unnamed called dupe thrown, no passback when the other reading is otherwise trash, the throw naming the other copy; the receiver still playing on a discharge and the giver's hole card settling, an ordinary discard when the giver's hole cannot be the card, and the reacter stamped to throw a card the giver already played; with wide empathy, a finesse through the giver's own hole play thrown on an ordinary and on a reverse reactive, and not thrown when no hole card of the giver's can be the connector (v20.1.0) |
| `tests/test_tiiah/test_replay_2011475_dupes_strikes_and_the_shared_collapse.cpp` | the live game: o4's call noted as the team reads it, o4 open until T24 and settled at T25, yagami's o21 read `{g1,b1,r4}` and will-bot67's r4 passed back at T20, and at T47 the strike's b3 on the stacks and Blue for the b4 |
| `tests/test_tiiah/test_receiver_reading_beats_the_stamp.cpp` | §1d — the receiver's bucket reading replacing a pair-frame stamp it is disjoint from, when the receiver's own stacks are ahead of the pair's |
| `tests/test_tiiah/test_replay_2011830_receiver_bucket_reading_not_the_stamp.cpp` | the live game: will-bot69's o15 read `{g4}` (not `{r2,y3,g3}`), green on 4 in its belief, and the called g5 played at T23 |
| `tests/test_tiiah/test_refusal_and_dupe_strike.cpp` | §1c and §1e rules 5 and 8 — a refusal given while occupied; several giver hole cards admitting the refused card leaving them open, a requirement kept and the shared view floored; a dupe strike floored when the striker watched the copy go in, and not when the copy was its own |
| `tests/test_tiiah/test_replay_2011854_refusal_and_the_dupe_strike.cpp` | the live game: at T27 will-bot67 refuses with Purple, o20 stays called `{g4}`, will-bot69's p2 is called and the shared view is on purple 1 with red still 2; at T42 the p1 strike is on the shared view and will-bot67 plays the b4 |
| `tests/test_tiiah/test_replay_2011887_undeferred_call_not_dropped.cpp` | §1d and §1e — the live game: will-bot67's b2 calls will-bot69's g3 and the call survives Rule 5, will-bot69 plays it at T20, and its row for will-bot67 no longer claims the r2 |
| `tests/test_tiiah/test_replay_2011885_named_card_not_absorbed.cpp` | §1e — the live game: o15 settles as the n1 once the g3 is named, brown is on 1 for everyone, and will-bot69's row for yagami no longer claims the r3. Its T32 §2b test was deleted in v18.0.0: v18 reads the T8 clue differently, and the reactive it chose between is no longer on offer |
| `tests/test_tiiah/test_world_replay_band.cpp` | §1e — the band absorbing an unnamed card but striking a named one, a row replaying our private settle with its shared set so that a card waiting on it lands, and leaving the settle out once the row counts it (v18.1.0) |
| `tests/test_tiiah/test_replay_2013616_reacts_after_the_pair_settles.cpp` | §1.3 and §1e — the live game: will-bot69's row for will-bot67 replays its privately settled o6 with `{r1,g1}`, reaches the same red 1 as will-bot67's copy after the pair watched a g1 land, and at T20 answers the Green by playing o24 (the r2) instead of reading it as a mistake |
| `tests/test_tiiah/test_known_play_standing_call.cpp` | §1c — a known play read from clue touches on the shared view: a standing call whose touches still allow an unplayable identity is not one, and a card touched down to playable 1s is (v18.0.0) |
| `tests/test_tiiah/test_known_play_floor.cpp` | §1.3 and §1e rule 6's shared-view form — a known play above every view reaching them all and settling the hole card under it, the same shared view from the seat that made the play, the rows taking the WATCHED card over a misread, and a strike raising nothing |
| `tests/test_tiiah/test_replay_2011327_a_known_play_is_in_every_view.cpp` | the live game the prefix rule cost: every one of will-bot67's views at `22121` on T22, and on T38 its belief at `43133` with the views it shares with yagami at the minimum of her two worlds, `43132` (v16.24.0), and no reactive Yellow pairing a trash p1 |
| `tests/test_tiiah/test_decision_making/test_replay_2011327_trash_reveal_over_lock.cpp` | reactor0 §3.3 read through TIIAH — Red revealing a clued r2 as trash is given instead of a rank 5 lock |
| `tests/test_tiiah/test_decision_making/test_replay_2011327_stall_drawn_card_not_blind_played.cpp` | the card drawn during a zero-clue stall is not blind-played; on the fixed views T32 is a reactive double play |
| `tests/test_tiiah/test_fix_clue.cpp` | §1h — a negative-touch fix and a positive-touch fix both read FIX and leave the card at the dead identity with its call gone; and three controls: a live identity, a call already named, and no call at all |
| `tests/test_tiiah/test_decision_making/test_fix_clue_priority.cpp` | §1h's priority — a fix given while Alice is OCCUPIED and every candidate is LOW (the tier-gate exemption), and the control where nothing needs fixing and she actions her own call |
| `tests/test_tiiah/test_reacter_play_read_by_the_receiver.cpp` | §1d's reacter half at the RECEIVER's seat — one playable in the bucket resolving the card and advancing the shared stacks, two narrowing without resolving, and the giver's seat keeping the reading it already had |
| `tests/test_tiiah/test_replay_2010512_receiver_reads_the_reacter_play.cpp` | the live game it cost: both of yagami's blind plays resolved, purple on 2 and (since v16.24.0) red and yellow on 1 in the shared view, the row at `[1,1,0,1,2]`, and the reactive answered on slot 4 instead of read as a MISTAKE |
| `tests/test_tiiah/test_replay_2010296_partner_play_is_presumed_to_land.cpp` | the live game the invented strike cost, replayed: the settle, the stacks, and the `{p1}` it unblocks |
| `tests/test_tiiah/test_replay_2009367_bucket_reading_depends_on_our_hole_card.cpp` | §1e — the live game it was narrow in, and the cascade that withdraws the conditional half |
| `tests/test_tiiah/test_replay_2009367_stable_clue_to_cathy_refuses_the_reactive.cpp` | §1c's refusal, read — the collapse it forces and that the clue is still read as the lock it is; and at T9, with yagami's called b1 a settled standing call (v18.10.0), we react with the b1 (order 16), as before v18.0.0 |
| `tests/test_tiiah/test_decision_making/test_refusal_clue.cpp` | §1c's refusal, GIVEN — outranking our own pending reaction when the named card is dead, and answering the reaction when it is not |
| `tests/test_tiiah/test_ordinary_reactive.cpp` | §1c's dispatch table — a clue to Cathy reactive with Bob reacting, the sum rule and bucket naming his slot 3 as `{r1, y1}`, the reacter playing it, and the reverse position keeping a clue to Cathy stable |
| `tests/test_tiiah/test_replay_2008177_ordinary_reactive_not_read.cpp` | the live game it was missing in, replayed |
| `tests/test_tiiah/test_decision_making/test_ordinary_reactive_reading.cpp` | §2 — the decision layer reading an ordinary reactive: REACTIVE_PLAY, Bob reacting, Cathy receiving, and the bot giving it |
| `tests/test_tiiah/test_reverse_reactive.cpp` | §1c — the target walk under stack simulation, a called card never retargeted, the dispatch reversing only when Bob has a known play and Cathy does not, and the sum rule picking the reacter's slot |
| `tests/test_tiiah/test_role_inversion.cpp` | §1c — role inversion: a colour-called card on Bob keeps a clue to Cathy stable though its touches allow unplayable identities, the same position dispatches a clue to Bob as a reverse reactive with Cathy reacting (v18.3.0), and a call on Cathy too leaves the ordinary reactive; the giver's `dispatch_is_reactive` agreeing |
| `tests/test_tiiah/test_all_targets_gotten.cpp` | §1c — every target gotten: the walk falls back to the leftmost called target (human diagnostic 2013726 T38), and an uncalled target still comes first (v18.5.0) |
| `tests/test_tiiah/test_fix_before_reverse_reactive.cpp` | §1c — a fix takes precedence over a reverse reactive: read at once from the giver's seat and by a Cathy who knows her blind play; a Cathy who cannot tell reads the reverse reactive, withdrawn when Bob plays an uncalled card or discards the dead one (v18.11.0); a clue that leaves the call good is the reverse reactive |
| `tests/test_tiiah/test_reverse_reactive_confirmation.cpp` | §1c — a reverse reactive stands when its receiver plays his standing play, and is withdrawn when he plays an uncalled card or discards (v18.11.0) |
| `tests/test_tiiah/test_reverse_reactive_standing_play.cpp` | §1c — a sure play: an uncalled clued 1 is no standing play while the hole may hold a 1, a known `{y1}` and an all-playable `{y1,g1}` are, a `{y1}` the hole may hold is not, a clued call is; a play of the card the reverse clue newly touched withdraws the reverse reactive, and the position's own standing play confirms it (v20.3.0) |
| `tests/test_tiiah/test_replay_2018428_reverse_reactive_needs_a_called_play.cpp` | §1c — replay 2018428 T18: black's 1 to will-bot69 re-touching an uncalled 1 is no reverse reactive, so will-bot67 does not blind-play its o11 (v20.3.0) |
| `tests/test_tiiah/test_deferred_collapse.cpp` | §1e — deferred collapse: a Red re-touching a clued r2 leaves the hole cards superposed, the holder's discard settles them at every seat (rule 7's shared form), and the same Red on an unclued r2 collapses at once (v18.12.0) |
| `tests/test_tiiah/test_replay_2014076_deferred_collapse_on_a_retouched_card.cpp` | §1e — human diagnostic 2014076 T14/T16: green waits on light's Red re-touching blue's r2, and blue's throw puts its stacks and the shared view on red 2 (v18.12.0) |
| `tests/test_tiiah/test_replay_2014402_finesse_read_before_dead_call_check.cpp` | §1d — replay 2014402 T27: black's i2 answered blue's 4, and green's o10 is called as the i3 (read before the call invariants, not erased as a dead i2) and played (v18.14.0) |
| `tests/test_tiiah/test_replay_2013963_reverse_reactive_finesse_on_a_settled_call.cpp` | §1c — replay 2013963 T12: will-bot67's settled unclued call puts the table in the reverse position, and will-bot69 reacts to yagami's 1 with its b2 (v18.10.0) |
| `tests/test_tiiah/test_replay_2013726_endgame_pairing_may_break_the_bucket.cpp` | §1d — human diagnostic 2013726 T27: at pace 1 blue gives 4 to green, a reactive whose pairing (black's r4, green's g1) breaks the bucket relation (v18.4.0) |
| `tests/test_tiiah/test_replay_2013726_reverse_reactive_finesse_on_a_called_play.cpp` | §1c — human diagnostic 2013726 T30: black's called r4 (touched r1–r5) puts the table in the reverse position, and blue gives Brown to black, a reverse-reactive finesse of green's n3 into black's n4 (v18.3.0) |
| `tests/test_tiiah/test_replay_2013645_role_inversion_keeps_the_clue_stable.cpp` | §1c — the live game at will-bot69's seat: yagami's T11 4 to will-bot67 is stable, nothing is called as its reaction, and at T12 will-bot69 plays its called b3 instead of blind-playing an r3 |
| `tests/test_tiiah/test_replay_2013645_stable_four_under_role_inversion.cpp` | §1c — the same game at will-bot67's seat: the 4 read as the stable clue, calling its chop to discard, with no reactive waiting |
| `tests/test_tiiah/test_bucket_encoding.cpp` | §1d — a rank clue naming the bucket below and a colour clue the bucket above, the spec's `{r4, y1}` worked example, a finesse naming its connector outright, and a pairing that breaks the relation going unread |
| `tests/test_tiiah/test_bucket_legality.cpp` | §1d — the giver may not reach past a bucket-illegal pairing to a legal one behind it, and a reader (who cannot see the reacter's card) takes the first pairing regardless |
| `tests/test_tiiah/test_replay_2010246_illegal_bucket_pairing_not_retargeted.cpp` | the live game the retargeting desynced, replayed |
| `tests/test_tiiah/test_reactions.cpp` | §1d — an inverted-only hand making the clue a double chuck, a double chuck over a critical card refused, and both parities resolving: the receiver is called to the button the reacter pressed |
| `tests/test_tiiah/test_rainbowy.cpp` | §1f — a colour clue naming its own suit rather than the rainbowy one, a rank clue on a new slot-1 card pinned to the rainbow (v20.13.0) and left alone off slot 1 or when the rainbow's next card is not of its rank, the re-pin when the own suit is finished, and a superpositioned giver's call read on its own stacks (`{p1}`, v20.4.0; `{p1,p2}` from v16.24.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2014884_rainbow_one_clued_by_rank.cpp` | §1f, the giver's half — replay 2014884 T4: Bob's unclued ra1 is clued with 1, not Yellow (read as the y1) or Purple (read as `{p2,ra1}`) (v18.19.0) |
| `tests/test_tiiah/test_decision_making/test_replay_2015013_colour_play_reveal_touching_rainbow.cpp` | §1b and §1f — replay 2015013 T37: will-bot67 gives Blue to yagami_black, a global play reveal of the clued b3 that also touches an unclued ra3, not a stalling 5 (v18.20.0) |
| `tests/test_tiiah/test_colour_reveal_frame.cpp` | §1b — a colour reveal only the pair's stacks make playable yields to the leftmost newly touched card; the same reveal on the common stacks outranks it (v18.20.0) |
| `tests/test_tiiah/test_pink_rank_clues.cpp` | §1b — a pinkish re-touch reads the global stacks: a 1 down only for the pair is a pink identity clue, not a pink trash clue (v19.0.0) |
| `tests/test_tiiah/test_replay_2015070_pink_identity_discard_collapses_superposition.cpp` | §1b and §1e rule 7 — replay 2015070 T20: yagami's T18 1 names barakeel's o3 the i1, barakeel's T19 discard settles will-bot67's hole i1, pink goes to 1 everywhere, and o22's call is the p1 (v19.0.0) |
| `tests/test_tiiah/test_replay_2015109_reactive_after_stale_common_naming.cpp` | §1e — replay 2015109 T48: barakeel's T29 y3 is booked by sight, so white stays on 3 and will-bot67's o17 is the reaction to barakeel's T47 1, `{b3}` (v19.1.0) |
| `tests/test_tiiah/test_hole_play_named_by_sight.cpp` | §1e — a played card our stale common copy reads `{y1}` is named the y2 we saw, and the shared view takes it (v19.1.0) |
| `tests/test_tiiah/test_replay_2017459_stable_call_rebases_on_bobs_played_call.cpp` | §1c — replay 2017459 T6: will-bot67's called p1 lands, so will-bot69's role-inverted Purple call on o13 is rebased to the p2, stands, and is played (v19.2.0) |
| `tests/test_tiiah/test_rebase_on_played_call.cpp` | §1c — an overlapping stable call becomes `{p2}` when the first call is played, and stays `{p1}` when that card is thrown or an unrelated card is played (v19.2.0) |
| `tests/test_tiiah/test_replay_2017491_reverse_reactive_world_fallback.cpp` | §1d — replay 2017491 T15: will-bot69's reverse reactive reads in the world where will-bot67's o18 was the b1; o12 is `{r3}`, blue is on 1, and will-bot67 plays o12 (v19.3.0) |
| `tests/test_tiiah/test_reactive_world_fallback.cpp` | §1d — a target one away on the frame but playable in a world reads there and collapses the worlds; with no such world the clue stays unreadable (v19.3.0) |
| `tests/test_tiiah/test_receiver_world_fallback.cpp` | §1d — the receiver world fallback: a bucket reading playable only in a hole world is called and the hole collapses, a one-away reading outside the bucket likewise, and with no hole card the bluff reading stands (v20.5.0); a reading conditional on every world still calls, and nothing collapses (v20.8.0) |
| `tests/test_tiiah/test_replay_2018517_receiver_world_fallback_collapses_hole.cpp` | §1d — replay 2018517 T21: will-bot67's o23 called as the r3 in the world where its hole card o21 was the r2, red on 2 for every seat, and it plays (v20.5.0) |
| `tests/test_tiiah/test_replay_2018535_receiver_finesse_through_a_hole_card.cpp` | §1d — replay 2018535 T8: will-bot67's o8 read as the m3 through the m2 its hole card o9 may have been, called, rainbow on 2 for every seat (v20.5.0) |
| `tests/test_tiiah/test_ascr_walk.cpp` | §1d and §1e (ASCR) — the walk's first target, whose reacter card plays only in a hole world, is taken and the hole collapses, ahead of a later target; with no hole card the walk goes on (v20.6.0) |
| `tests/test_tiiah/test_replay_2018541_ascr_reacter_card_plays_in_a_world.cpp` | §1d and §1e (ASCR) — human diagnostic 2018541 T26: will-bot67 reads the b4 pairing in the world where its o16 was the r3 and plays its r4, not o26 (v20.6.0) |
| `tests/test_tiiah/test_reacter_bucket_in_the_worlds.cpp` | §1d and §1e — the reacter's clued 2 reads the bucket-0 `{r2,y2}` of its own hole card's worlds, not the frame's `{g2,b2}`; with no hole card the frame's playables stand (v20.10.0) |
| `tests/test_tiiah/test_replay_2018857_reacter_reads_bucket_in_the_worlds.cpp` | §1d — replay 2018857 T17: will-bot67's o12, reacting to yagami's 5 onto the g2, reads `{r2}` (bucket 0, in the worlds where its o11 or o20 was the r1), not `{g2,b2}` (v20.10.0) |
| `tests/test_tiiah/test_replay_2017568_rank_four_on_lock_slot_is_a_ref_discard.cpp` | §1b (reactor0 §1c 5/6) — replay 2017568 T2: will-bot67's T1 4 on will-bot69's slots 2, 4, 5 calls slot 3 to discard; no lock (v20.0.0) |
| `tests/test_tiiah/test_replay_2018316_reverse_reactive_discharge.cpp` | §1k — replay 2018316 T8: yagami_black's 5 to yagami_blue is a reverse-reactive finesse naming yagami_green's o2 as the i1 black had thrown unknowingly at T2; green reads it `{i1}`, called to discard, and throws it rather than playing it into a strike |
| `tests/test_tiiah/test_rank_ref_discard_on_lock_slot.cpp` | §1b (reactor0 §1c 5/6) — the user's examples: a 4 on the lock slot alone is a lock, a 5 with a target calls slot 3, and makes no pink promise (v20.0.0) |
| `tests/test_tiiah/test_superposition.cpp` | §1e — a set recorded for our own and a partner's ambiguous play, a known play creating none and advancing both views, the two views diverging on a partner's play, a shared collapse, and the shared view staying absent outside the variant |
