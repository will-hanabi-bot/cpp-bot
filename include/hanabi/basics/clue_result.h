// Port of python-bot/src/hanabi_bot/basics/clue_result.py.
// Original Scala: scala-bot/src/scala_bot/basics/clueResult.scala.
//
// Statistics for a clue: empathy effects, bad-touch / trash, playables.
// Used by convention evaluators to score candidate clues.
#pragma once

#include <tuple>
#include <vector>

#include "hanabi/basics/identity.h"

namespace hanabi {

class Game;
struct ClueAction;

// Returns (new_touched, fill, elim) — empathy statistics for a clue.
std::tuple<std::vector<int>, std::vector<int>, std::vector<int>> elim_result(
    const Game& prev, const Game& game, const std::vector<int>& hand,
    const std::vector<int>& list_);

// Player(s) "responsible" for saving id — the ones with the fewest visible dupes.
std::vector<int> dupe_responsibility(const Game& game, Identity id, int except_);

// Returns (bad_touch, trash, avoidable_dupe).
std::tuple<std::vector<int>, std::vector<int>, int> bad_touch_result(
    const Game& prev, const Game& game, const ClueAction& action);

// Returns (blind_plays, playables) — newly playable orders.
std::pair<std::vector<int>, std::vector<int>> playables_result(const Game& prev,
                                                                  const Game& game);

}  // namespace hanabi
