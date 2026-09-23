# The Throw It in a Hole Convention

This is the **ruling reference** for how this bot interprets clues **when a game
runs the TIIAH convention**. Where this document and the code disagree, that is
a bug in one of them — every rule that is implemented cites the `file:line` that
implements it, and every rule that is not says so in the same breath.

Terminology is in [GLOSSARY.md](GLOSSARY.md); terms not defined there (chop,
pitch / chuck, CTP / CTD, critical, loaded, …) carry their
[reactor glossary](../reactor/GLOSSARY.md) meanings. Convention that is legal
but not yet implemented is tracked in [TODO.md](../../../TODO.md) §39–§41.

Reading conventions, as in the other two documents: **slot 1 is the leftmost,
newest card**; **Alice / Bob / Cathy** are positional — Alice is the clue giver,
Bob the next player, Cathy the one after.

## §0 Status

**v16.0.0 READS these games but does not PLAY them.** `Game::take_action` throws
for a TIIAH game (`src/basics/decide.cpp:909-913`) and the client says so once in
table chat and then sits still (`src/net/commands.cpp:646-666`). That is
deliberate: §1c below is specified and unimplemented, and the decision layer the
bot would otherwise use is reactor0's, which prices clues by *reactor0's*
meanings. Before v16.0.0 the flag was not read at all, so these tables were
played by reactor's rules — reactor's own §1b.8 said so — and stopping that is
what this version is for.

| Part | State |
|---|---|
| The engine rules (§1) | implemented |
| Buckets (§1a) | implemented |
| Stable clues (§1b) | implemented, by delegation to reactor0 |
| Reverse-reactive dispatch (§1c) | implemented (v16.2.0) |
| The bucket-encoded reactive (§1d) | **specified only** — TODO.md §40 |
| Superposition (§1e) | implemented (v16.1.0) |
| Rainbowy colour pinning (§1f) | **specified only** — TODO.md §42 |

Which convention a game runs is `Game::convention`, resolved at game init
(`src/net/commands.cpp:378-393`). TIIAH is resolved from the **variant** and
never from `/setall`: only these 44 variants can be played under it, and no
other convention can be played at them. `parse_convention` accepts the name
`"tiiah"` so a snapshot round-trips, but `/setall tiiah` is not a way to select
it.

The convention will be a **3-player** one, like reactor0 — the sum rule in §1d
takes its modulus from the hand size. Until it is finished, a TIIAH table of any
seat count is gated, rather than falling back to reactor.

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
`Game::convention`**, because a 4+ player TIIAH table resolves elsewhere and
still needs its stacks to be right.

### §1.1 We fill in what the server withholds

Each seat knows every card played **except its own**: we watched a partner's card
sit in their hand, so `state.deck[order]` still knows what it was even though the
action carries no identity.

`resolve_hidden_action` (`src/basics/action.cpp:82-130`) rebuilds the
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

It runs as a pre-step in `Game::handle_action` (`src/basics/game.cpp:556`), which
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
(`src/basics/game.cpp:547-566`) hands that identity to the resolution. What is
left when empathy cannot name it is §1e's superposition — the hand updates and
the stacks wait.

Because of all this, `State::play_stacks` in a TIIAH game is **what we believe**,
not what is. So are `score()`, `strikes` and therefore `State::ended()`. A wrong
belief is invisible until the game ends; that is the variant, not a defect.

### §1.3 Two views of the stacks

A seat knows every play but its own, so no two seats hold the same stacks — and
we know more about a partner's play than they do, having watched the card leave
their hand. A clue still has to mean ONE thing, so the engine keeps two views:

| | advances on |
|---|---|
| `State::play_stacks` | **our belief** — every play we can name, which is every partner's and our own identified ones |
| `State::common_play_stacks` (`state.h:59`) | **the shared view** — only plays whose identity was common knowledge, plus collapses on evidence every seat shares |

Anything deciding what a clue MEANS reads the shared view, through
`State::shared_score` / `shared_pace` (`state.h:121-124`) and
`Game::shared_in_endgame`. Outside TIIAH the second vector is empty and those
accessors are the ordinary ones, so no other variant pays for this. Anything
deciding what WE should do reads our belief, which is the better of the two.

Two rules already read the shared view: §1b's stall context
(`tiiah/interpret_clue.cpp:81-86`) and the stable orange ladder's pitch-vs-chuck
test (`reactor0/interpret_clue.cpp:519`), which TIIAH reaches by delegating.

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
(`src/conventions/tiiah/interpret_clue.cpp:36-86`) calls
`reactor0::stable_colour` / `reactor0::stable_rank`. Read
[reactor0's §1b and §1c](../reactor0/CONVENTION.md) for what they do.

The TIIAH dispatcher differs from reactor0's in two ways only: there is no
blind-family arm (no TIIAH variant is a Blind one — all 44 carry
`throwItInAHole` and no other behavioural flag) and no target-parity arm.

### §1c Reverse reactive

A **known play** is a card stamped `CALLED_TO_PLAY` whose inference still
contains at least one good playable identity, *or* a card whose global empathy is
entirely playable identities. Read from `common`, so every seat answers it the
same way (`has_known_play`, `src/conventions/tiiah/interpret_clue.cpp:20-32`).

When **Bob has a known play and Cathy does not**, dispatch reverses:

- a clue to **Bob** is REACTIVE, with **Cathy** reacting and **Bob** receiving;
- a clue to **Cathy** is STABLE.

Bob's target is the next playable in his hand under stack simulation, where every
known play in his hand is assumed already played. Worked example: Bob holds
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

`interpret_reactive` (`src/conventions/tiiah/interpret_reactive.cpp:90-182`)
installs the waiting connection, stamps the reacter's blind play and leaves the
receiver's own call for reaction time — the resolution machinery is reactor0's,
shared.

**The target walk** (`receiver_targets`, `:64-82`) runs over the stacks as they
will stand once the receiver's known plays are done (`simulate_known_plays`,
`:41-60`, a fixpoint so a chain advances in order). Two kinds of card qualify,
and they are walked in this order — reactor0's Phase A before Phase B, which is
the order every seat walks:

1. one that plays outright once those known plays are done;
2. one that is **one away**, which is a finesse: the reacter holds the bridge.
   §1c's own worked example is this case.

The giver walks the candidates and takes the first whose reacter side works. A
pairing refused on SHARED knowledge is walked past, because the reacter walks
past it too; one refused on what only the GIVER can see kills the clue (§1g),
because the reacter cannot see it and would act on that pairing anyway.

A clue whose walk finds nothing reads as a MISTAKE and stamps nothing. Guessing
would be worse than refusing: it would call a partner onto a card nobody named.

### §1d The reactive clue — SPECIFIED, NOT IMPLEMENTED (TODO.md §40)

All reactive clues are **even parity**: the two named cards are both pitched.
The two slots are picked by the sum rule as in reactor0 —
`react_slot + target_slot ≡ anchor (mod hand size)` — and **the anchor is
reactor0's**: the rank value for a rank clue, and the colour's value from the
fixed table (`include/hanabi/conventions/reactor0/colour_value.h`) for a colour
clue.

The clue KIND then carries what the two cards ARE, which is the part a hidden
stack cannot otherwise convey:

- a **rank** clue means a finesse, *or* the receiver's target sits one bucket
  **higher** than the reacter's card (wrapping);
- a **colour** clue means a finesse, *or* one bucket **lower** (wrapping).

**The finesse and the bucket relation are disjoint by definition**, so a clue is
never both and there is no precedence between them to settle.

Worked example, six suits, red/yellow/green/blue/purple/teal: a rank clue can get
a teal 1 to play into a teal 2 (a finesse), or a purple/teal card to play into a
yellow 1 — purple and teal are bucket 2, yellow is bucket 0, which is one higher
wrapping.

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

#### Inverted targets

**Inverted suits are skipped** as reactive targets, playables and finesses
alike, unless they are **the only playables left in the receiver's hand** — that
test is over the receiver's hand, not the whole table — in which case the clue
is a **double chuck** instead.

### §1e Superposition

A player who played a card without knowing its identity is **superpositioned**:
they keep a map of card order → the identities it could have been, and so does
everyone else on their behalf. A player may hold several at once.

A superpositioned reacter choosing a target picks the leftmost
playable/finessable **by stack simulation assuming that none of the superposed
cards were played**. Worked example: Bob played a card he knows is either a
purple 1 or a teal 1, nothing else has been played, and Cathy holds
`p2 p1 g1 r1 r5`. Assuming neither landed, Bob chooses her purple 1 on slot 2.

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
3. every copy of it is accounted for in the discard pile and the other hands.

Rules 1 and 2 say the same thing — that identity was still NEEDED, so the
superposed card was not it — and both are **shared**: every seat sees them and
narrows alike, so a collapse on either moves the shared stacks too. Only
evidence every seat holds counts, which is why a play that was itself a
superposition is not evidence: the seat that made it does not know what it was.

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

### §1f Rainbowy variants — SPECIFIED, NOT IMPLEMENTED (TODO.md §42)

In a rainbowy variant a non-orange **colour** stable clue pins the CTP to exactly
the next playable card of that colour's **own** suit — not a superposition of it
and the rainbowy suit. Clue red on turn 1 and the receiver writes red 1, and is
*not* superpositioned between red 1 and rainbow 1.

The exception is when that is immediately impossible: if red 5 is already played
and red is clued, the CTP is re-pinned to the rainbowy suit's next playable.

A stable colour clue given **by a superpositioned player** is read under §1e's
rule: if the giver is superpositioned between a purple 1 and a teal 1 and clues
purple, the receiver assumes the purple 1 was not played.

## Test coverage

| File | What it pins |
|---|---|
| `tests/test_tiiah/test_variant_flag.cpp` | the flag parses, agrees with the name on all 2426 variants, and lands on exactly 44 |
| `tests/test_tiiah/test_convention_predicates.cpp` | `is_reactor0_family` / `uses_reactor0_decisions`, and that both are an identity transform on reactor and reactor0 |
| `tests/test_tiiah/test_buckets.cpp` | §1a's four rows, the inverted re-indexing, and `bucket_of` |
| `tests/test_tiiah/test_engine_rules.cpp` | §1.1's table and §1.2 — a partner's hidden play advancing our stacks, our own leaving them alone, a hidden misplay striking, a hidden 5 paying nothing, and both sides of the orange mirror |
| `tests/test_tiiah/test_gate_and_clues.cpp` | §0's refusal, §1b read identically to reactor0 (a differential test), and §1c refusing rather than guessing |
| `tests/test_tiiah/test_reverse_reactive.cpp` | §1c — the target walk under stack simulation, a called card never retargeted, the dispatch reversing only when Bob has a known play and Cathy does not, and the sum rule picking the reacter's slot |
| `tests/test_tiiah/test_superposition.cpp` | §1e — a set recorded for our own and a partner's ambiguous play, a known play creating none and advancing both views, the two views diverging on a partner's play, a shared collapse, and the shared view staying absent outside the variant |
