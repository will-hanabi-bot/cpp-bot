# The Throw It in a Hole Convention

This is the **ruling reference** for how this bot interprets clues **when a game
runs the TIIAH convention**. Where this document and the code disagree, that is
a bug in one of them — every rule that is implemented cites the `file:line` that
implements it, and every rule that is not says so in the same breath.

Terminology is in [GLOSSARY.md](GLOSSARY.md); terms not defined there (chop,
pitch / chuck, CTP / CTD, critical, loaded, …) carry their
[reactor glossary](../reactor/GLOSSARY.md) meanings. Convention that is legal
but not yet implemented is tracked in [TODO.md](../../../TODO.md), where this
convention's open entry is **44** — a suit whose low card went into the hole
unseen can read as unplayable to every seat at once (§1.3).

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
| The engine rules (§1) | implemented; the three stack views (v16.12.0) |
| Buckets (§1a) | implemented |
| Stable clues (§1b) | implemented, by delegation to reactor0 |
| The dispatch, both arms (§1c) | reverse implemented (v16.2.0), ordinary (v16.8.0); the refusal (v16.14.0) |
| The bucket-encoded reactive (§1d) | implemented (v16.3.0); the receiver's half (v16.9.0); its held negative (v16.11.0); the relation as a giver-side legality test (v16.15.0) |
| Superposition (§1e) | implemented (v16.1.0); the back-solve (v16.12.0); conditional readings (v16.13.0) |
| Rainbowy colour pinning (§1f) | implemented (v16.4.0) |
| Decision making (§2) | implemented (v16.6.0), by delegation to reactor0 |
| Naming the called card (§2a) | implemented (v16.7.0) |

What has **not** been decided is the endgame: the solver declines to solve a
TIIAH position rather than solving it wrongly, because our own hidden plays make
its card accounting inconsistent (`src/endgame/solver.cpp`). The stall list and
the ordinary rungs carry those turns. That is a deliberate hold, to be settled
after the convention has been played in anger.

Which convention a game runs is `Game::convention`, resolved at game init
(`src/net/commands.cpp:378-393`). TIIAH is resolved from the **variant** and
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
| it reached the hole | plain, dead | a failed Discard — a strike |
| it reached the hole | inverted | Discard (a chuck), failed if dead |
| it reached the pile | inverted | Play (a pitch) |
| it reached the pile | plain | Discard |

It runs as a pre-step in `Game::handle_action` (`src/basics/game.cpp:563`), which
is the only choke point live play, snapshot replay, `rewind` and `simulate` all
share — and it has to be there rather than inside `on_play`, because resolving a
hidden card can change the action's **type** and `on_play` cannot rewrite itself
into `on_discard`. `add_action` still records the RAW action, so the log says
what the wire said and the resolution stays a pure function of the action history
plus our own sight, which is what lets `rewind` and `apply_snapshot` reproduce
it without serialising anything.

**Our own play is the one the state cannot name**, because `deck[order].id()`
is `nullopt` for our seat. Our own EMPATHY often can, though, and a play we can
name is a play we can account for: `hidden_own_play_id`
(`src/basics/game.cpp:547-559`) hands that identity to the resolution. What is
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
  (`state.cpp:150-159`) is symmetric for that reason, and falls back to the shared
  view for an outsider — who recovers the rest by the back-solve (§1e).
- **It advances through the prefix, like any stack.** A row takes a card only when
  it is the next one for that row, so a play the row never saw blocks everything
  above it — which is exactly what the seat behind that row believes.
  `tests/test_tiiah/test_pairwise_stacks.cpp`. It is also the limit: a suit whose
  low card went into the hole unseen can be stuck for every reader at once
  (TODO.md 44, replay 2008489 T52).
- **Our own row is never consulted.** Against ourselves there is nothing we do not
  know, so `pairwise_view(us)` hands back our belief.

Anything deciding what a clue MEANS reads a shared view — `State::shared_view`
(`src/basics/state.cpp:143-156`) is that state, with `playable_set` and
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
| which slot a stable clue names (§1b) | the giver and the RECEIVER's pairwise view — `SharedStacks`, a scoped swap of `play_stacks` / `playable_set` / `trash_set` around the delegation (`tiiah/interpret_clue.cpp:29-57`, installed `:250-266`), plus a `prev` swapped to match |
| which slots a reactive clue pairs (§1c, §1d) | the giver and the REACTER's, since the reacter is the seat that must act — `stacks_known_to_both` into `reacter_faces` / `stacks_after_queued_plays` (`tiiah/interpret_reactive.cpp:231-240`, `variants/hole.cpp:10-22`) |
| what a call SAYS the card is | the HOLDER's own belief when the holder is us — `repin_own_call` for a stable call (`tiiah/interpret_clue.cpp:76-110`) and the reacter's own reading of the pairing (`tiiah/interpret_reactive.cpp:318-390`) |
| the §1f pin | `shared_view()` directly (`tiiah/interpret_clue.cpp:100`) |
| §1b's stall context | `Game::shared_in_endgame` (`tiiah/interpret_clue.cpp:245-251`) |
| the stable orange ladder's pitch-vs-chuck test | `State::shared_pace` (`reactor0/interpret_clue.cpp:519`), reached by delegating |
| the reaction's held negative (§1d) | asks the BUTTON, not the stacks — `Game::fire_reaction_elim` (`basics/decide.cpp:84-108`) |

The swap costs nothing until the views actually differ — `SharedStacks::needed`
— which is every game until somebody plays into the hole without knowing what
they played. What a seat DECIDES still reads its own belief, which is the best of
the three: the other two are for meanings only.

**The one sanctioned disagreement.** Which slot a clue names is the same at every
seat, because it rests on a view the giver and the actor both hold and the
receiver has no freedom left once the reacter has moved. What the called card IS
can differ: the holder reads it on their own stacks, which are at least as
advanced as anything the pair shares, and an observer reads it on the pair's. The
holder's is the one that governs, since they are the one who plays it.

The reaction's negative was the third entry in that table only as of v16.11.0,
and it was the one that cost a game: it asked "did a stack advance?" to tell a
receiver's play from their discard, which in this variant is the one question
the stacks cannot answer. Replay 2008422 is written up in §1d.

Three things still read our belief where they arguably should not, all inside
reactor0 and all out of reach of the swap: `common.hypo_stacks`, rebuilt from
`play_stacks` by the elim layer; `Player::hypo_stacks`; and
`reactor0::enforce_call_invariants` (`basics/decide.cpp:213-218`), which runs
*after* the swap has unwound and so judges rules 3 and 4 — whether a standing
call still has a button that works — against one seat's private stacks. Its own
comment says a call "has to die for every seat at the same moment"
(`reactor0/call_invariants.cpp:141-143`), so that one is a real gap rather than
a tolerable one. The first two matter for delayed-play chains rather than for
the call itself.

### §1.2 A 5 pays nothing

`State::with_play` skips the clue-token refund on the final rank
(`src/basics/state.cpp:121`). Gated there rather than inside `regain_clue`,
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

### §1b Stable clues — reactor0's, unchanged

A stable clue means exactly what it means under reactor0, and the code delegates
rather than forking a copy that would drift: `tiiah::interpret_clue`
(`src/conventions/tiiah/interpret_clue.cpp:148-216`) calls
`reactor0::stable_colour` / `reactor0::stable_rank`. Read
[reactor0's §1b and §1c](../reactor0/CONVENTION.md) for what they do.

The TIIAH dispatcher differs from reactor0's in two ways only: there is no
blind-family arm (no TIIAH variant is a Blind one — all 44 carry
`throwItInAHole` and no other behavioural flag) and no target-parity arm.

### §1c The dispatch: both reactives, and the position that switches them

A **known play** is a card stamped `CALLED_TO_PLAY` whose inference still
contains at least one good playable identity, *or* a card whose global empathy is
entirely playable identities. Read from `common`, so every seat answers it the
same way (`has_known_play`, `src/conventions/variants/hole.cpp:51-67`).

TIIAH runs **both** dispatches — reactor0's positional one and the reverse — and
the **position** decides which seat's clue carries the reaction. The position
holds when **Bob has a known play and Cathy does not**
(`reverse_reactive_position`, `src/conventions/variants/hole.cpp:69-76`):

| position | a clue to **Bob** | a clue to **Cathy** |
|---|---|---|
| holds | **REACTIVE** — Cathy reacts, Bob receives | STABLE |
| otherwise | STABLE | **REACTIVE** — Bob reacts, Cathy receives (reactor0's) |

The ordinary square is reactor0's rule unchanged, and it was **missing until
v16.8.0**: the dispatcher only ever added the reverse arm, so every clue to Cathy
fell through to the stable ladders. Replay
[2008177](https://hanab.live/shared-replay/2008177#4) T3 is what that cost — a
rank 2 that named a double play read as a lock, and the reacter discarded.
Whichever way the clue goes, it is read by the same §1d rules: even parity, the
sum rule, and the buckets.

`tiiah::interpret_clue` (`src/conventions/tiiah/interpret_clue.cpp:281-298`) is
the table, one `if` per row.

#### The refusal: Bob says the card is already played (v16.14.0)

Alice reads her own reactive against **her own** stacks, and those are stale in
exactly one way — she cannot see what she threw in the hole. So she can name a
card of Cathy's that is already down, and neither she nor Cathy can tell. Bob
can. The convention gives him a way to say so:

> **Alice gives Cathy an ordinary reactive, and Bob answers by giving Cathy any
> STABLE clue ⟹ the card Alice named is already played.** Alice collapses the
> superposition that must have been it (§1e rule 5).

Read at `read_refusal` (`interpret_clue.cpp:117-160`), ahead of the dispatch
table because it has to pre-empt it: Bob's clue to Cathy is `giver=bob,
target=cathy`, which the ordinary row would otherwise read as a fresh reactive.

Four things this rests on:

- **Stable is the discriminator.** A reacter who clues instead of reacting is
  normally *deferring*, and a deferral carries the reactive intent forward — so
  it is itself reactive. A refusal is stable. Without that test the two are the
  same event, since the engine already lets any clue by the reacter clear the
  waiting connection (`basics/decide.cpp:196-199`), which is also why the arm
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
easily. `clue_refuses_dead_target` (`reactor0/decision.cpp:688-718`) marks the
candidates, and they join **Precedence step 1** — `choose_very_high_clue` — rather
than a rung: refusing is done *instead of* reacting, and every rung sits below the
urgent return, which is the very thing being declined. The tier itself
(`clue_tier`) is left alone so that no non-hole game can see any of this.

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
reactor0 deliberately reversed and which TIIAH restores.

**The sum rule applies here exactly as to an ordinary reactive clue**
(§1d): `react_slot + target_slot ≡ anchor (mod hand size)`, with Cathy's slot as
the react slot and Bob's as the target. The walk above finds Bob's target; the
sum rule then fixes which of Cathy's cards is called.

Dispatch is decided by the position **before** the clue. Asking the post-clue
game instead makes every play clue to Bob answer "Bob has a known play", because
the clue itself just gave him one.

`interpret_reactive` (`src/conventions/tiiah/interpret_reactive.cpp:176-411`)
installs the waiting connection, stamps the reacter's blind play and leaves the
receiver's own call for reaction time — the resolution machinery is reactor0's,
shared.

**The target walk** (`receiver_targets`) runs over the **shared** stacks as
they will stand *when the reacter acts* (`reacter_faces`,
`src/conventions/tiiah/interpret_reactive.cpp:47-56`; the shared view is
§1.3's, and §1e says why the walk has to use it). Which stacks those are is
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
(`drop_dead_play_calls`, `src/conventions/reactor0/call_invariants.cpp:144-165`).
The call under test is **left out** of that simulation: counting it would spend
its own identity, and it would read dead exactly when it is most alive
(`stacks_after_queued_plays`'s `except_order`,
`src/conventions/variants/hole.cpp:9-42`). Nothing follows for the holder's turn
— the call is alive, not yet actionable.

### §1d The reactive clue

All reactive clues are **even parity**: whichever button the reacter presses, the
receiver is called to the same one (`wc.even_parity = true`,
`src/conventions/tiiah/interpret_reactive.cpp:197`). The two slots are picked by
the sum rule as in reactor0 — `react_slot + target_slot ≡ anchor (mod hand size)`
(`interpret_reactive.cpp:245-246`) — and **the anchor is reactor0's**: the rank value
for a rank clue, and the colour's value from the fixed table
(`include/hanabi/conventions/reactor0/colour_value.h`) for a colour clue
(`anchor_of`, `interpret_reactive.cpp:28-34`).

The clue KIND then carries what the two cards ARE, which is the part a hidden
stack cannot otherwise convey:

- a **rank** clue means a finesse, *or* the receiver's target sits one bucket
  **higher** than the reacter's card (wrapping);
- a **colour** clue means a finesse, *or* one bucket **lower** (wrapping).

**The finesse and the bucket relation are disjoint by definition**, so a clue is
never both and there is no precedence between them to settle. `required_target_bucket`
/ `bucket_relation_holds` (`src/conventions/tiiah/interpret_reactive.cpp:72-86`)
are the relation; the finesse is a target one away, whose *connector* is the one
card that bridges to it (`interpret_reactive.cpp:263-266`).

What the reacter writes down is therefore one of three things
(`interpret_reactive.cpp:318-390`):

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

#### The receiver reads it too

The relation has two ends, and the receiver reads theirs when the reaction
resolves — not at clue time, because until the reacter acts they do not know
which of their cards the sum rule names. Their card is the **union** of the same
two readings, intersected with what it could already be
(`narrow_receiver_call`, `src/conventions/tiiah/interpret_reactive.cpp:445-521`,
called from the engine seam at `src/basics/decide.cpp:582-589`):

- **the bucket** the reacter's sits one step from — one *lower* for a rank clue,
  one *higher* for a colour one, the relation above read backwards;
- **the continuation** of the card the reacter played, `id.next()`, which is what
  a finesse leaves behind.

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
the reacter's card was never narrowed at all.

#### What the receiver's OTHER slots learn, and whose eyes that is NOT

The reaction also lays a **negative** on the rest of the receiver's hand — *"if
that slot had been playable, the clue would have named it instead"* — held until
the receiver acts and then fired by `Game::fire_reaction_elim`
(`src/basics/decide.cpp:54-152`). reactor0 picks between three strengths of it by
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
| **legality** | the **giver**, alone | a pairing that is neither a finesse nor a bucket relation, and which the two players could not each name from their own empathy, makes the clue **illegal**. Alice may not give it (`interpret_reactive.cpp:288-311`) |
| **inference** | every reader | what the reacter's and the receiver's cards ARE, from the bucket the *receiver's target* sits in — which every seat can see (`:365-390`, `narrow_receiver_call`) |

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

#### Inverted targets, and the double chuck

**Inverted suits are skipped** as reactive targets, playables and finesses
alike (`receiver_targets`, `src/conventions/tiiah/interpret_reactive.cpp:135-166`),
unless they are **the only playables left in the receiver's hand** — that test is
over the receiver's hand, not the whole table — in which case the clue is a
**double chuck** instead: both players press **Discard**, which is the button
that stacks an inverted card.

A double chuck asks something different of the reacter, because they are not
playing. What they hold has to be **affordable to chuck**
(`safe_to_chuck`, `interpret_reactive.cpp:93-99`): either the button plays it —
an inverted card the stack is waiting for — or losing it costs the team nothing,
which is any card that is not critical (trash included, since trash is never
critical). Alice may not name a slot that fails this, and the refusal is a
giver-only one, so it kills the clue rather than moving to the next target.

Nothing else changes. The bucket relation does not apply — an inverted suit is in
no bucket — and does not need to, since no identity has to reach the reacter for
them to press Discard; a double chuck over a one-away target still names its
connector. The receiver's own call comes from the ordinary even-parity mirror at
resolution time (`receiver_button`, `src/conventions/reactor0/decision.cpp:98`),
which reads the button the reacter actually pressed
(`reacter_button_pressed`, `src/conventions/reactor0/interpret_reaction.cpp:514`)
rather than the hook that fired.

### §1e Superposition

A player who played a card without knowing its identity is **superpositioned**:
they keep a map of card order → the identities it could have been, and so does
everyone else on their behalf. A player may hold several at once.

A superpositioned reacter choosing a target picks the leftmost
playable/finessable **by stack simulation assuming that none of the superposed
cards were played**. Worked example: Bob played a card he knows is either a
purple 1 or a teal 1, nothing else has been played, and Cathy holds
`p2 p1 g1 r1 r5`. Assuming neither landed, Bob chooses her purple 1 on slot 2.

That rule and §1.3's shared view are **the same rule** (v16.4.0): a superposed
play never advanced `common_play_stacks`, so a walk that runs on the shared view
is already assuming none of them were played. `stacks_after_queued_plays` starts
there (`src/conventions/variants/hole.cpp:10-22`), so every seat walks the same
simulation however many cards have gone into the hole unnamed.

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
(`include/hanabi/basics/card.h:173-199`) beside the superposition itself, which
is what lets a later fact **withdraw** them: when the earlier card settles, the
worlds it contradicts die and the candidates with no world left go with them
(`refute_worlds`, `src/conventions/tiiah/superposition.cpp:19-87`, called from
the collapse below and from `settle` before it clears the set).

Two limits, both deliberate. The enumeration is **capped at 64 worlds**
(`open_worlds`, `:306-339`) and reads the call flat beyond that, because a
partial list of worlds would be a conditional set missing some of its own
conditions — worse than an unconditional one. And the collapse now runs to a
**fixpoint**, since settling one card can refute another's worlds, leave *it* a
singleton, and settle it in turn; a single pass over the orders only caught a
cascade that happened to run in increasing order.

The set is stamped on the card's `ConvData` (`include/hanabi/basics/card.h:169`)
at the moment of the play, by `note_hidden_action`
(`src/conventions/tiiah/superposition.cpp:121-148`), and is built from
**`common`** — the one view all three seats compute alike, which is what lets
everyone hold the same set on the player's behalf. A play whose common empathy
already names one identity is no superposition at all: the player knew, so the
SHARED stacks advance with it.

**Collapsing** (`collapse_superpositions`, `:150-196`). A candidate leaves a
superposition when:

1. another player plays a card of that identity;
2. another player's clue, stable or reactive, puts CTP on a playable card of
   that identity;
3. every copy of it is accounted for in the discard pile and the other hands;
4. **a clue between two OTHER seats calls a card we can see, and the card is
   further up its suit than our own stacks are** — the back-solve;
5. **a reactive we gave was REFUSED** (§1c): the card we named is already played,
   so a superposition of ours that admits it was it.

Rules 1 and 2 say the same thing — that identity was still NEEDED, so the
superposed card was not it — and both are **shared**: every seat sees them and
narrows alike, so a collapse on either moves the shared stacks too. Only
evidence every seat holds counts, which is why a play that was itself a
superposition is not evidence: the seat that made it does not know what it was.

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
(`src/conventions/tiiah/superposition.cpp:119-183`) keeps it narrow on purpose:
only a STABLE call, since a reactive one can name a card that is one away rather
than playable, and only a plain suit, since the arithmetic is a plain prefix.

**Rule 5 is the mirror of rule 4** (v16.14.0). The back-solve learns from a call
being HIGHER up its suit than we thought; the refusal learns from one being
BEHIND the stacks altogether. Both reduce to the same accounting: a seat's own
stacks can only be short by what that seat threw in the hole.
`collapse_refused_target` (`src/conventions/tiiah/superposition.cpp:341-354`),
driven from §1c's `read_refusal` rather than from the collapse pass, because the
evidence is the clue being given rather than anything about the cards. Shared,
like rules 1 and 2: every seat watches the refusal.

Rule 3 is **private** — it is `reactor0::sight_narrowed`'s shape, and it reads
our own eyes. It narrows what WE believe and never the set partners predict
from, so it may move `play_stacks` and never `common_play_stacks`, and the
stored set is left as it stands. That split is what keeps a reacter's target
choice predictable: it is made on the shared rule, not on what one seat can see.

When one candidate is left the card leaves the map and the stack it belongs to
advances (`settle`, `:99-117`). A card that did not land is gone all the same,
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

`pin_rainbowy_colour` (`src/conventions/tiiah/interpret_clue.cpp:103-146`) is a
post-step over reactor0's ladder rather than a fork of it: the ladder decides
WHICH card is called and this decides what that card is, by narrowing the new
call to one identity. It runs on the shared view (§1.3), which is what carries
the superpositioned-giver rule — there is no separate test for it, because "the
stacks a clue is read against" and "assume none of the superposed cards were
played" are the same sentence here.

Only a **new** call is pinned. An older one was pinned by its own clue, and §1i
forbids widening an inferred set back out.

The rainbowy suit is the one carrying `rainbowish` (Rainbow, Omni), `muddy`
(Muddy Rainbow, Cocoa Rainbow) or `prism` — 16 of the 44 variants have exactly
one. The non-orange proviso is defensive: no TIIAH variant pairs an inverted suit
with a rainbowy one, so the guard has nothing to exclude today.

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
(`src/conventions/reactor0/decision.cpp:221-231`).

Each side of a reading also carries the seat that holds it
(`Designation::holder`), so the rungs that ask "can this hand afford the loss"
or "how good a ditch is this" ask it of the hand that will actually act. That is
v16.5.0's change, and reactor0's own behaviour is unchanged by it.

**The endgame solver stands down here**, as §0 says: our own hidden plays leave
its card accounting inconsistent and it declines rather than solving wrongly, so
those turns are decided by the stall list and the ordinary rungs.

### §2a Prefer a play clue the receiver can NAME (v16.7.0)

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

Implemented as `ClueCandidate::names_its_card`, computed in `analyse_clues`
(`src/conventions/reactor0/decision.cpp`) as "the called card's shared
`possibilities()` hold exactly one identity" — the very set `note_hidden_action`
stamps a superposition from — and read by `stable_play_chain`, the only tiebreak
rungs 3.1 / 4.1 and the endgame stall list's rung 2 have. Outside this variant
the term is false of every candidate and `settle` skips it, so no other
convention moves.

It is a TIEBREAK, not a veto: it orders the stable-play pool and never changes
which rung fires, so a clue that names its card cannot displace a better rung.

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
| `tests/test_tiiah/test_decision_making/test_clue_reading.cpp` | §2 — a clue to Bob read as reactive with Cathy reacting, the delayed connector read as a PLAY rather than a strike, a clue to Cathy read stable, and the double-play reactive chosen end to end |
| `tests/test_tiiah/test_receiver_bucket.cpp` | §1d's receiver half — a colour clue leaving the bucket below, the finesse continuation surviving when the clue does not rule it out, a rank clue reading the other way, and the reacter's own seat reading wider |
| `tests/test_tiiah/test_replay_2008217_receiver_misses_the_bucket.cpp` | the live game it was missing in, replayed |
| `tests/test_tiiah/test_replay_2008422_receiver_play_in_the_hole_is_not_a_discard.cpp` | §1d's negative half — our own hidden play read as a play rather than a discard, so a later referential discard on the same hand survives |
| `tests/test_tiiah/test_pairwise_stacks.cpp` | §1.3 — a partner's blind play reaching every row but theirs, our own reaching none, a known play reaching all, the prefix rule blocking a row on the card it never saw, and the symmetry of `stacks_known_to_both` |
| `tests/test_tiiah/test_conditional_reading.cpp` | §1e's worlds — one world when nothing is in the hole, one per candidate when something is, whose seat they belong to, two cards multiplying and chaining in play order, and the cap reading it flat |
| `tests/test_tiiah/test_replay_2009367_bucket_reading_depends_on_our_hole_card.cpp` | §1e — the live game it was narrow in, and the cascade that withdraws the conditional half |
| `tests/test_tiiah/test_replay_2009367_stable_clue_to_cathy_refuses_the_reactive.cpp` | §1c's refusal, read — the collapse it forces, that the clue is still read as the lock it is, and the playable card it frees up |
| `tests/test_tiiah/test_decision_making/test_refusal_clue.cpp` | §1c's refusal, GIVEN — outranking our own pending reaction when the named card is dead, and answering the reaction when it is not |
| `tests/test_tiiah/test_replay_2008489_reacter_reads_its_own_stacks.cpp` | §1.3 + §1e — the reacter reading its own stacks rather than the giver's stale ones, and the back-solve recovering the blue it threw in the hole |
| `tests/test_tiiah/test_ordinary_reactive.cpp` | §1c's dispatch table — a clue to Cathy reactive with Bob reacting, the sum rule and bucket naming his slot 3 as `{r1, y1}`, the reacter playing it, and the reverse position keeping a clue to Cathy stable |
| `tests/test_tiiah/test_replay_2008177_ordinary_reactive_not_read.cpp` | the live game it was missing in, replayed |
| `tests/test_tiiah/test_decision_making/test_ordinary_reactive_reading.cpp` | §2 — the decision layer reading an ordinary reactive: REACTIVE_PLAY, Bob reacting, Cathy receiving, and the bot giving it |
| `tests/test_tiiah/test_reverse_reactive.cpp` | §1c — the target walk under stack simulation, a called card never retargeted, the dispatch reversing only when Bob has a known play and Cathy does not, and the sum rule picking the reacter's slot |
| `tests/test_tiiah/test_bucket_encoding.cpp` | §1d — a rank clue naming the bucket below and a colour clue the bucket above, the spec's `{r4, y1}` worked example, a finesse naming its connector outright, and a pairing that breaks the relation going unread |
| `tests/test_tiiah/test_bucket_legality.cpp` | §1d — the giver may not reach past a bucket-illegal pairing to a legal one behind it, and a reader (who cannot see the reacter's card) takes the first pairing regardless |
| `tests/test_tiiah/test_replay_2010246_illegal_bucket_pairing_not_retargeted.cpp` | the live game the retargeting desynced, replayed |
| `tests/test_tiiah/test_reactions.cpp` | §1d — an inverted-only hand making the clue a double chuck, a double chuck over a critical card refused, and both parities resolving: the receiver is called to the button the reacter pressed |
| `tests/test_tiiah/test_rainbowy.cpp` | §1f — a colour clue naming its own suit rather than the rainbowy one, a rank clue left alone, the re-pin when the own suit is finished, and a superpositioned giver read as though they had not played |
| `tests/test_tiiah/test_superposition.cpp` | §1e — a set recorded for our own and a partner's ambiguous play, a known play creating none and advancing both views, the two views diverging on a partner's play, a shared collapse, and the shared view staying absent outside the variant |
