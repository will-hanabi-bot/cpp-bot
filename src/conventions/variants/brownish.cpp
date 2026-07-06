#include "hanabi/conventions/variants/brownish.h"

#include <algorithm>

#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/variants/predicates.h"

namespace hanabi::reactor::variants {

namespace {

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

}  // namespace

bool brownish_tcm_applies(const Game& prev, const Game& game,
                          const ClueAction& action,
                          const std::vector<int>& newly_touched) {
  const State& state = game.state;
  int target = action.target;
  if (!includes_brownish(state) || action.clue.kind != ClueKind::RANK ||
      prev.common.obvious_loaded(game, target)) {
    return false;
  }
  bool no_newest_in_touched =
      !state.hands[target].empty() &&
      !contains(newly_touched, state.hands[target][0]);
  if (!no_newest_in_touched) return false;
  for (size_t i = 0; i < state.variant->suits.size(); ++i) {
    const auto& s = state.variant->suits[i];
    bool brown = s.suit_type.brownish;
    if (brown && state.play_stacks[i] + 1 < state.max_ranks[i]) {
      return true;
    }
  }
  return false;
}

}  // namespace hanabi::reactor::variants
