// Port of python-bot/src/hanabi_bot/conventions/reactor/interpret_reactive.py.
// Reactive colour/rank clue dispatch.
#pragma once

#include <optional>

#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"

namespace hanabi::reactor {

std::optional<ClueInterp> interpret_reactive_colour(const Game& prev, Game& game,
                                                       const ClueAction& action,
                                                       int focus_slot, int reacter,
                                                       bool looks_stable);

std::optional<ClueInterp> interpret_reactive_rank(const Game& prev, Game& game,
                                                     const ClueAction& action,
                                                     int focus_slot, int reacter);

}  // namespace hanabi::reactor
