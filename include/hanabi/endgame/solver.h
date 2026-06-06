// Port of python-bot/src/hanabi_bot/endgame/solver.py.
#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"
#include "hanabi/endgame/fraction.h"
#include "hanabi/endgame/helper.h"

namespace hanabi {
class Game;
}

namespace hanabi::endgame {

struct Arrangement {
  std::vector<Identity> ids;
  Fraction prob;
  RemainingMap remaining;
};

// Result of `winnable`: either (actions, winrate) or an error string.
struct WinnableResult {
  std::vector<PerformAction> actions;
  Fraction winrate;
  std::string error;
  bool ok() const { return error.empty(); }
};

// Result of `solve`: either (action, winrate) or an error string.
struct SolveResult {
  PerformAction action;
  Fraction winrate;
  std::string error;
  bool ok() const { return error.empty(); }
};

class EndgameSolver {
 public:
  bool monte_carlo = true;
  double timeout = 30.0;  // seconds

  EndgameSolver() = default;
  EndgameSolver(bool mc, double t) : monte_carlo(mc), timeout(t) {}

  SolveResult solve(const Game& game,
                      std::optional<PerformAction> only_action = std::nullopt);

  // Recursive winnability check.
  WinnableResult winnable(const Game& game, int player_turn, const RemainingMap& remaining,
                            std::optional<double> deadline, int depth = 0);

 private:
  using ActionEntry = std::pair<PerformAction, std::vector<Identity>>;

  // success_rate[depth][perform] = (avg_winrate, samples)
  std::unordered_map<int, std::vector<std::tuple<PerformAction, Fraction, int>>>
      success_rate_;

  std::vector<ActionEntry> possible_actions(const Game& game, int player_turn,
                                              const RemainingMap& remaining,
                                              std::optional<double> deadline, int depth,
                                              bool infer = false);
  Fraction action_winrate(const Game& game, const std::vector<GameArr>& arrs,
                            const ActionEntry& action, int player_turn,
                            std::optional<double> deadline);
  std::vector<std::pair<PerformAction, Fraction>> optimize_full(
      const Game& game, const std::pair<std::vector<GameArr>, std::vector<GameArr>>& arrs,
      const std::vector<ActionEntry>& actions, int player_turn,
      std::optional<double> deadline, bool single_hypo);
  WinnableResult optimize(const Game& game,
                            const std::pair<std::vector<GameArr>, std::vector<GameArr>>& arrs,
                            const std::vector<ActionEntry>& actions, int player_turn,
                            std::optional<double> deadline, int depth);

  void cache_result(int depth, const PerformAction& perform, Fraction winrate);
  std::optional<Fraction> cached_winrate(int depth, const PerformAction& perform) const;

 public:
  // Public for use by advance_game in winnable.cpp.
  static Action perform_to_action(const PerformAction& perform, const Game& game,
                                    int player_index);
};

}  // namespace hanabi::endgame
