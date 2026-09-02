// Synesthesia's stable clue convention. See src/conventions/reactor0/CONVENTION.md §1f.
//
// Synesthesia can never give a rank clue -- it carries `clueRanks: []` -- so the
// ordinary `stable_colour` / `stable_rank` ladders have nothing to work with.
// Once target parity stands down (the score reaches 50% of the variant maximum,
// or the team is at 8 clues -- `bob_clue_is_reactive`) a clue to Bob is stable
// again, and there its meaning comes from a FIXED TABLE that names an action
// outright: each clue colour is one button and one slot in Bob's hand.
//
// UNREACHABLE IN THREE VARIANTS (v13.0.0). Where the variant offers only two
// clue colours a COLOUR clue is never stable -- `colour_is_never_stable`,
// `interpret_reactive.h` -- and Synesthesia has no other kind, so in
// `Synesthesia & Rainbow / White / Null (3 Suits)` nothing ever reaches this
// table. It still governs the other 33 Synesthesia variants; the three are
// purely reactive by design, not by omission.
#pragma once

#include <optional>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/variant.h"

namespace hanabi::reactor0 {

// What a Synesthesia colour clue names.
struct SynesthesiaCall {
  // CALLED_TO_PLAY is a *pitch* (press Play), CALLED_TO_DISCARD a *chuck*
  // (press Discard) -- the glossary's names for the two buttons, which on an
  // inverted suit swap their ordinary effects.
  CardStatus button = CardStatus::CALLED_TO_PLAY;
  int slot = 1;  // 1-based, in the CLUED seat's hand
};

// The table, keyed on the clue colour's NAME in `Variant::clue_colour_names` --
// the same key `colour_clue_value` uses, though deliberately NOT the same table
// (that one gives Blue=4 and lets Orange take the first untaken value).
//
//   Red 1 pitch | Yellow 2 pitch | Green 3 pitch | Blue 4 pitch
//   Purple 5 pitch | Orange 1 chuck | anything else 4 chuck
//
// The five named colours are `pitch` at their own *colour value* (Red=1 ...
// Purple=5), which as of v14.0.0 agrees with `colour_clue_value` on those five.
// That is a coincidence and not a licence to delete this table: the two still
// disagree on Orange and on the catch-all, and `colour_clue_value` has no notion
// of a button at all.
//
// COLLISIONS ARE IMPOSSIBLE, which is what makes a fixed table safe here rather
// than the "first untaken" dance `colour_clue_value` needs. Across all 36
// Synesthesia variants (`data/variants.json`) the suits are a subset of
// {Red, Yellow, Green, Blue, Purple} plus AT MOST ONE other, and of the others
// only Orange, Brown, Black and Teal are clueable at all: Rainbow and Dark
// Rainbow are `allClueColors` and add no colour of their own, while White, Gray,
// Null and Dark Null are `noClueColors`. So at most one clue colour ever reaches
// the catch-all.
//
// Per `data/suits.json`, **Dark Orange clues as "Orange"** and so takes the
// Orange row -- which is exactly where chuck and pitch carry their literal
// inverted meanings -- and Dark Brown clues as "Brown", falling to the catch-all
// alongside Black and Teal.
//
// A 3-suit variant offers only three colours, so some slots are simply
// unreachable there. That is a limit of the variant, not of the table.
SynesthesiaCall synesthesia_call(const Variant& variant, int colour_index);

// Does every PITCH row of the table read as a CHUCK instead (v14.0.0)? True when
// the only playable cards Bob could be holding are on INVERTED suits.
//
// Pressing Play on an inverted card throws it away and pressing Discard stacks
// it, so where every reachable play is inverted, "play this" can only be spelled
// with the Discard button -- an unflipped pitch call would be unobeyable. When it
// fires, Red reads d1, Yellow d2, Green d3, Blue d4, Purple d5; Orange and the
// catch-all are already chucks and do not move.
//
// NON-VACUOUS: at least one playable reading must exist AND all of them must be
// inverted. Thirty of the thirty-six Synesthesia variants have no inverted suit,
// so a vacuously-true rule would fire there whenever Bob's empathy admitted
// nothing playable and rewrite a table the flip was never meant to reach. Where
// Bob has no playable reading at all the clue simply degrades to a STALL, which
// `slot_is_pitchable` already delivers with no help from this.
//
// While it fires, Red (d1) carries the same call as Orange (d1). That collision
// is deliberate and harmless: nothing is ambiguous for Bob, the giver merely has
// one fewer distinct call for as long as the flip holds. `synesthesia_call`
// itself stays a pure function of the variant, so the UNFLIPPED table keeps its
// no-collision guarantee above.
//
// Read off `effective_possible_for` -- see the definition for what that does and
// does not guarantee across seats.
bool synesthesia_pitch_flips(const Game& game, int bob);

// Read a stable Synesthesia clue. `action.target` is Bob, since a stable clue
// under target parity is only ever one to Bob.
//
// Returns `STALL` when the named action is bad by COMMON knowledge -- the slot
// does not exist, or the button is one Bob cannot press there. Nothing is
// stamped, and giver and receiver agree on that because the test is common.
//
// Returns nullopt (a MISTAKE, so the giver never offers the clue) when only the
// SEEING seats can tell the action is bad. §1g: shared knowledge retargets,
// giver-only knowledge rejects. The alternative -- letting the clue quietly mean
// something else -- is undecodable by the one player who cannot see the card.
std::optional<ClueInterp> synesthesia_stable(Game& game,
                                             const ClueAction& action);

}  // namespace hanabi::reactor0
