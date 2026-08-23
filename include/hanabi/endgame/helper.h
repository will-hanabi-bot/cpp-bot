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

// Does pressing this action's button on this card advance a stack for EVERY
// reading its holder still has?
//
// Inverted suits make the BUTTON part of the question: Play advances a plain
// card and PITCHES an inverted one, Discard the reverse. A reading set spanning
// both kinds is therefore never certain, even when every identity in it is
// playable, because the two halves need opposite buttons.
//
// Deliberately built on `possibilities()` rather than on `obvious_playables`
// (clue-derived only) or `Thought::id(infer=true)` (needs a pinned singleton).
// A card read as {a5, d5} with both stacks on 4 scores whichever it is, and
// neither of those two notions can see that -- which is exactly how replay
// 1969779 T68 came to gamble a trash card on the final turn while holding one.
bool certainly_advances(const Game& game, int order, const PerformAction& how);

// Every action on our own cards that `certainly_advances`, in hand order.
std::vector<PerformAction> certain_plays(const Game& game);

struct GameArr {
  Fraction prob;
  RemainingMap remaining;
  std::optional<Identity> drew;
};

// Returns (undrawn, drawn). Faithful to Python's gen_arrs semantics.
std::pair<std::vector<GameArr>, std::vector<GameArr>> gen_arrs(
    const Game& game, const RemainingMap& remaining, bool clue_only);

}  // namespace hanabi::endgame
