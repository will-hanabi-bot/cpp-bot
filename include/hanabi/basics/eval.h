// Port of python-bot/src/hanabi_bot/basics/eval.py.
// Original Scala: scala-bot/src/scala_bot/basics/eval.scala.
//
// Force-clue evaluator: assume `giver` will spend a clue, then score the
// resulting state via the `advance` callable.
#pragma once

#include <functional>
#include <optional>

#include "hanabi/basics/clue.h"

namespace hanabi {

class Game;

double force_clue(
    const Game& game, int giver,
    const std::function<double(const Game&)>& advance,
    std::optional<int> only = std::nullopt,
    const std::function<bool(const Clue&)>& clue_filter =
        [](const Clue&) { return true; });

}  // namespace hanabi
