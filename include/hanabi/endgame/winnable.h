// Port of python-bot/src/hanabi_bot/endgame/winnable.py.
#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"
#include "hanabi/endgame/helper.h"

namespace hanabi {
class Game;
struct State;
struct Card;
}  // namespace hanabi

namespace hanabi::endgame {

enum class SimpleResult { ALWAYS_WINNABLE, UNWINNABLE };

struct WinnableWithDraws {
  std::vector<Identity> draws;
};

using SimpleResultT = std::variant<SimpleResult, WinnableWithDraws>;

bool past_deadline(std::optional<double> deadline);
bool is_dummy_clue(const PerformAction& perform);

State advance_state(State state, const PerformAction& perform, int player_index,
                     const std::optional<Card>& draw);

Game advance_game(const Game& game, const PerformAction& perform, int player_index,
                   std::optional<Identity> draw);

std::optional<PerformAction> clueless_winnable(State state, int player_turn,
                                                  std::optional<double> deadline, int depth);

bool winnable_simpler(const Game& game, int player_turn, const RemainingMap& remaining,
                      std::optional<double> deadline, int depth);

SimpleResultT winnable_if(const Game& game, int player_turn,
                            const PerformAction& perform,
                            const RemainingMap& remaining,
                            std::optional<double> deadline, int depth);

}  // namespace hanabi::endgame
