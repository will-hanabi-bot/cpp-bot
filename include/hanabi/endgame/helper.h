// Port of python-bot/src/hanabi_bot/endgame/helper.py.
#pragma once

#include <map>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"
#include "hanabi/endgame/fraction.h"

namespace hanabi {
class Game;
struct State;
}  // namespace hanabi

namespace hanabi::endgame {

// RemainingMap: multiset of unseen identities, keyed by Identity ord. We use
// std::map (ordered) for deterministic iteration order matching Python.
using RemainingMap = std::map<int, int>;

RemainingMap remaining_remove(const RemainingMap& remaining, Identity id);
int remaining_total(const RemainingMap& remaining);

std::vector<Identity> find_must_plays(const State& state, const std::vector<int>& hand);

bool unwinnable_state(const State& state, int player_turn, int depth = 0);

// One winning line for the trivial case. Empty/"" returned via empty vector +
// false bool.
struct TriviallyResult {
  std::vector<PerformAction> actions;
  Fraction winrate;
  bool found = false;
};
TriviallyResult trivially_winnable(const Game& game, int player_turn);

struct GameArr {
  Fraction prob;
  RemainingMap remaining;
  std::optional<Identity> drew;
};

// Returns (undrawn, drawn). Faithful to Python's gen_arrs semantics.
std::pair<std::vector<GameArr>, std::vector<GameArr>> gen_arrs(
    const Game& game, const RemainingMap& remaining, bool clue_only);

}  // namespace hanabi::endgame
