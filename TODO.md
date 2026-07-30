# TODO — convention that is legal but not implemented

[CONVENTION.md](CONVENTION.md) describes the bot **as it behaves**. This file is
the other half: rules that are part of the convention and legal to play, but that
the current build does not produce or decode. Anything here is a known gap, not a
disagreement about what the convention says.

Delete an entry when it ships, and update the corresponding
`CONVENTION.md` / [GLOSSARY.md](GLOSSARY.md) wording in the same commit.

---

## 1. Trash push should read as a referential play

**Convention.** A rank clue whose every touchable identity is basic trash is
interpreted **exactly like a referential play clue**, as if the trash cards had
been touched by a colour clue: it calls the card one slot to the **left** (newer)
of a newly-touched card to play.

**Today.** `try_stable`'s branch 2 (`src/conventions/reactor/interpret_clue.cpp:447-468`)
only intersects the focus's `inferred` with the trash set and sets `meta.trash`.
No status is stamped and **no play is called** — the clue conveys "this is
garbage" and nothing more.

**Touchpoints.**
- The `try_stable` branch order (`interpret_clue.cpp:424-618`): trash push is
  branch 2, `ref_play` is branch 8. Making branch 2 defer to `ref_play` is the
  shape of the fix.
- `variant->touch_possibilities(kind, value)` (`:450-458`) — the loop that keeps
  Pink-Fives from being misread as a trash push. Any rewrite must preserve it.
- `bool trash_push` (`:448`) is the local flag; the name stays, since the term is
  reactor convention.

---

## 2. Bluffs are legal but never played or read

**Convention.** A rare reactive move in which the reacter *believes* they are
playing a card that connects with a one-away-from-playable card in the receiver's
hand, but plays a different card. It is legal so long as the receiver can tell
**after** the reaction that their target is not actually playable — they then mark
it one-away-from-playable and chuck their chop as normal.

**Today.** The interpreter neither initiates nor decodes one. The POV-invariant
abort in the finesse phase
(`src/conventions/reactor/interpret_reactive.cpp:824-838`) returns `nullopt`
whenever the observer can see that the reacter's card is *not* the required
`prev_id` — which is exactly the bluff case — and there is deliberately no "try
the next slot" retry (CONVENTION.md §1a.5).

**Touchpoints.**
- `CardStatus::BLUFFED`, `MAYBE_BLUFFED`, `F_MAYBE_BLUFFED`
  (`include/hanabi/basics/card.h:30-32`) and `FinesseKind::BLUFF`
  (`include/hanabi/basics/connection.h:20-26`) already exist but are never set.
- Decoding requires the receiver to re-derive their target's playability *after*
  the reaction — i.e. work in `interpret_reaction.cpp`, not just a relaxed abort.

---

## 3. Chop should pick the most recent CTD by `signal_turn`

**Convention.** When a hand holds several cards called to discard, the chop is
the one signalled **most recently by `signal_turn`** — not the newest by hand
position. An earlier CTD may be a sacrifice while a later one is not, and never
the reverse, so preferring the newest signal keeps good cards around longer.

**Today.** `Game::chop`'s first pass (`src/basics/decide.cpp:404-417`) scans the
hand slot 1 → oldest and returns the first `CALLED_TO_DISCARD` it finds, i.e. the
newest CTD *in the hand*. These usually coincide but need not.

**Touchpoints.**
- `find_all_discards` (`decide.cpp:902-926`, v0.30) already filters the candidate
  pool by largest `signal_turn`, so the two mechanisms currently disagree; this
  change makes `chop()` match the filter rather than inventing a new rule.
- Every `chop()` consumer inherits the new choice: `has_ptd()`
  (`decide.cpp:419-458`), the giver-side safety checks (`:622-625`, `:877-878`,
  `:974-978`), and eval (`state_eval.cpp:44`, `:82-103`, `:118-120`, `:323`,
  `:438-440`, `:521-524`).
- `Player::chop_newest` (`player_game.cpp:38-45`, used only by `is_sieved`) has no
  CTD pass at all and is out of scope.
