// Port of python-bot/src/hanabi_bot/conventions/reactor/interpret_reactive.py.
// Reactive colour/rank clue dispatch.
#pragma once

#include <optional>

#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"

namespace hanabi::reactor {

// Narrow `thought.possible` by visibility from the HOLDER's POV: an
// identity stays possible only while the copies visible in every
// non-holder hand (plus base_count) don't exhaust its card count.
// See the definition for the POV-invariance rationale (v0.23).
IdentitySet effective_possible_for(const Game& game, int self_order);

std::optional<ClueInterp> interpret_reactive_colour(const Game& prev, Game& game,
                                                       const ClueAction& action,
                                                       int focus_slot, int reacter,
                                                       bool looks_stable);

std::optional<ClueInterp> interpret_reactive_rank(const Game& prev, Game& game,
                                                     const ClueAction& action,
                                                     int focus_slot, int reacter);

}  // namespace hanabi::reactor
