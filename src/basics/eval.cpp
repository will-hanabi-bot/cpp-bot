#include "hanabi/basics/eval.h"

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"

namespace hanabi {

double force_clue(const Game& game, int giver,
                    const std::function<double(const Game&)>& advance,
                    std::optional<int> only,
                    const std::function<bool(const Clue&)>& clue_filter) {
  const State& state = game.state;
  if (!state.can_clue()) return -999.0;
  if (state.num_players == 2) {
    Game hypo = game;
    --hypo.state.clue_tokens;
    return advance(hypo);
  }

  std::vector<ClueAction> candidates;
  for (int i = 0; i < state.num_players; ++i) {
    if (i == giver || i == state.our_player_index) continue;
    if (only && *only != i) continue;
    for (const Clue& clue : state.all_valid_clues(i)) {
      if (!clue_filter(clue)) continue;
      auto list_ = state.clue_touched(state.hands[i], clue.kind, clue.value);
      candidates.emplace_back(giver, i, std::move(list_), clue.base());
    }
  }

  double best = -100.0;
  for (const auto& action : candidates) {
    Game hypo = game.simulate(action);
    if (hypo.last_move() &&
        std::holds_alternative<ClueInterp>(*hypo.last_move()) &&
        std::get<ClueInterp>(*hypo.last_move()) == ClueInterp::MISTAKE) {
      continue;
    }
    double v = advance(hypo);
    if (v > best) best = v;
  }

  if (best == -100.0) {
    Game hypo = game;
    --hypo.state.clue_tokens;
    return advance(hypo);
  }
  return best;
}

}  // namespace hanabi
