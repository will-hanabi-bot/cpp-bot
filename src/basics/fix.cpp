#include "hanabi/basics/fix.h"

#include <algorithm>
#include <unordered_set>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"

namespace hanabi {

FixResult check_fix(const Game& prev, const Game& game, const ClueAction& action) {
  const auto& list_ = action.list_;
  std::vector<int> clued_resets;
  std::vector<int> duplicate_reveals;

  for (auto it = list_.rbegin(); it != list_.rend(); ++it) {
    int order = *it;
    auto thought_id = game.common.thoughts[order].id();
    auto prev_thought_id = prev.common.thoughts[order].id();
    const bool prev_clued = prev.state.deck[order].clued;

    bool duplicated = false;
    if (prev_clued && thought_id && !prev_thought_id) {
      for (int o : list_) {
        if (o == order) continue;
        if (!prev.state.deck[o].clued) continue;
        if (game.state.deck[order].matches(game.state.deck[o])) {
          duplicated = true;
          break;
        }
      }
    }

    if (prev.common.order_kt(game, order)) continue;

    bool clued_reset_branch =
        (prev.meta[order].status == CardStatus::CALLED_TO_PLAY &&
         prev.is_blind_playing(order) &&
         game.common.thoughts[order].info_lock &&
         game.common.thoughts[order].info_lock->forall(
             [&](Identity i) { return game.state.is_basic_trash(i); })) ||
        (prev.state.deck[order].clued && !prev.common.thoughts[order].reset &&
         game.common.order_kt(game, order));

    if (clued_reset_branch) {
      clued_resets.insert(clued_resets.begin(), order);
    } else if (duplicated) {
      duplicate_reveals.insert(duplicate_reveals.begin(), order);
    }
  }

  if (!clued_resets.empty() || !duplicate_reveals.empty()) {
    return FixResultNormal{std::move(clued_resets), std::move(duplicate_reveals)};
  }
  return FixResultNone{};
}

std::optional<IdentitySet> distribution_clue(const Game& prev, const Game& game,
                                                const ClueAction& action, int focus) {
  const State& state = game.state;
  const Thought& thought = game.common.thoughts[focus];

  bool all_prev_clued = true;
  for (int o : action.list_) {
    if (!prev.state.deck[o].clued) {
      all_prev_clued = false;
      break;
    }
  }
  if (all_prev_clued) return std::nullopt;

  if (!game.in_endgame() &&
      state.rem_score() > static_cast<int>(state.variant->suits.size())) {
    return std::nullopt;
  }
  auto focus_id = state.deck[focus].id();
  if (focus_id && state.is_basic_trash(*focus_id)) return std::nullopt;

  IdentitySet poss;
  if (action.clue.kind == ClueKind::COLOUR) {
    poss = thought.possible;
  } else {
    int r = action.clue.value;
    poss = thought.possible.filter([&](Identity i) { return i.rank == r; });
  }

  IdentitySet useful;
  for (Identity id : poss) {
    if (state.is_basic_trash(id)) continue;
    bool duplicated = false;
    for (int i = 0; i < state.num_players; ++i) {
      if (i == action.target) continue;
      for (int o : state.hands[i]) {
        if (game.is_touched(o) && game.order_matches(o, id, /*infer=*/true)) {
          duplicated = true;
          break;
        }
      }
      if (duplicated) break;
    }
    if (duplicated) {
      useful = useful.add(id);
    } else {
      return std::nullopt;
    }
  }
  return useful.non_empty() ? std::optional<IdentitySet>{useful} : std::nullopt;
}

bool rainbow_mismatch(const Game& game, const ClueAction& action, Identity id,
                       int prompt) {
  const State& state = game.state;
  const int target = action.target;
  const auto& list_ = action.list_;
  const BaseClue clue = action.clue;

  if (clue.kind != ClueKind::COLOUR) return false;
  if (!state.variant->suits[id.suit_index].suit_type.rainbowish) return false;
  if (game.known_as(prompt, "Rainbow") || game.known_as(prompt, "Omni")) return false;
  for (const auto& c : state.deck[prompt].clues) {
    if (c.kind == clue.kind && c.value == clue.value) return false;
  }

  bool all_rainbow = true;
  if (target == state.our_player_index) {
    for (int o : list_) {
      bool ok = game.me().thoughts[o].possible.forall([&](Identity c2) {
        return state.variant->suits[c2.suit_index].suit_type.rainbowish;
      });
      if (!ok) {
        all_rainbow = false;
        break;
      }
    }
  } else {
    for (int o : list_) {
      if (!state.variant->suits[state.deck[o].suit_index].suit_type.rainbowish) {
        all_rainbow = false;
        break;
      }
    }
  }
  if (!all_rainbow) return false;

  for (const auto& c : state.deck[prompt].clues) {
    auto touched = state.clue_touched(state.hands[target], c.kind, c.value);
    auto sorted_a = touched;
    auto sorted_b = list_;
    std::sort(sorted_a.begin(), sorted_a.end());
    std::sort(sorted_b.begin(), sorted_b.end());
    if (sorted_a == sorted_b) return true;
  }
  return false;
}

}  // namespace hanabi
