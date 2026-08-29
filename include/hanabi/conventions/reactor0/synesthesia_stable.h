// Synesthesia's stable clue convention. See src/conventions/reactor0/CONVENTION.md §1f.
//
// Synesthesia can never give a rank clue -- it carries `clueRanks: []` -- so the
// ordinary `stable_colour` / `stable_rank` ladders have nothing to work with.
// Once target parity stands down (the score reaches 50% of the variant maximum,
// or the team is at 8 clues -- `bob_clue_is_reactive`) a clue to Bob is stable
// again, and there its meaning comes from a FIXED TABLE that names an action
// outright: each clue colour is one button and one slot in Bob's hand.
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
//   Red 1 pitch | Yellow 2 pitch | Green 3 chuck | Blue 2 chuck
//   Purple 5 pitch | Orange 1 chuck | anything else 4 pitch
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
