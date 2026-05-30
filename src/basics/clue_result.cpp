#include "hanabi/basics/clue_result.h"

#include <algorithm>
#include <unordered_set>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"

namespace hanabi {

std::tuple<std::vector<int>, std::vector<int>, std::vector<int>> elim_result(
    const Game& prev, const Game& game, const std::vector<int>& hand,
    const std::vector<int>& list_) {
  const State& state = game.state;
  std::vector<int> new_touched;
  std::vector<int> fill;
  std::vector<int> elim;

  std::unordered_set<int> list_set(list_.begin(), list_.end());

  // Walk hand in reverse (matches Python's `reversed(hand)` with `insert(0, …)`
  // - both end up newest-first).
  for (auto it = hand.rbegin(); it != hand.rend(); ++it) {
    int order = *it;
    const Thought& prev_thought = prev.common.thoughts[order];
    const Thought& thought = game.common.thoughts[order];
    const Card& card = state.deck[order];
    CardStatus status = game.meta[order].status;

    if (!(card.clued && status != CardStatus::CALLED_TO_DISCARD &&
          thought.possible.length() < prev_thought.possible.length())) {
      continue;
    }

    auto card_id = card.id();
    if (game.common.order_kt(game, order) ||
        (card_id && state.is_basic_trash(*card_id))) {
      continue;
    }

    if (!prev.state.deck[order].clued && !prev.is_blind_playing(order)) {
      new_touched.insert(new_touched.begin(), order);
    } else if (list_set.count(order) && state.has_consistent_infs(thought) &&
                status != CardStatus::CALLED_TO_PLAY) {
      fill.insert(fill.begin(), order);
    } else if (state.has_consistent_infs(thought)) {
      elim.insert(elim.begin(), order);
    }
  }

  return {std::move(new_touched), std::move(fill), std::move(elim)};
}

std::vector<int> dupe_responsibility(const Game& game, Identity id, int except_) {
  const State& state = game.state;

  auto potential_dupes = [&](int player_index) {
    int total = 0;
    for (int o : state.hands[player_index]) {
      if (state.deck[o].clued && game.common.thoughts[o].inferred.contains(id)) ++total;
    }
    return total;
  };

  std::vector<std::pair<int, int>> dupes;
  for (int i = 0; i < state.num_players; ++i) {
    if (i == except_) continue;
    dupes.emplace_back(potential_dupes(i), i);
  }
  if (dupes.empty()) return {};
  int min_dupe = dupes.front().first;
  for (const auto& [cnt, _] : dupes) min_dupe = std::min(min_dupe, cnt);
  std::vector<int> result;
  for (const auto& [cnt, i] : dupes) {
    if (cnt == min_dupe) result.push_back(i);
  }
  return result;
}

std::tuple<std::vector<int>, std::vector<int>, int> bad_touch_result(
    const Game& prev, const Game& game, const ClueAction& action) {
  const State& state = game.state;
  const int giver = action.giver;
  const int target = action.target;

  std::vector<int> dupe_scores;
  dupe_scores.reserve(prev.players.size());
  for (size_t i = 0; i < prev.players.size(); ++i) {
    if (static_cast<int>(i) == target) {
      dupe_scores.push_back(99);
      continue;
    }
    const Player& player = prev.players[i];
    int total = 0;
    for (int order : state.hands[target]) {
      const Card& card = state.deck[order];
      if (prev.state.deck[order].clued || !card.clued) continue;
      auto card_id = card.id();
      if (!card_id || state.is_basic_trash(*card_id)) continue;
      for (int o : state.hands[i]) {
        const Thought& t = player.thoughts[o];
        if (state.deck[o].clued && t.inferred.length() > 1 && t.inferred.contains(*card_id)) {
          ++total;
        }
      }
    }
    dupe_scores.push_back(total);
  }

  int min_score = dupe_scores.front();
  for (int s : dupe_scores) min_score = std::min(min_score, s);
  int avoidable_dupe = dupe_scores[giver] - min_score;

  std::vector<int> bad_touch;
  std::vector<int> trash;

  auto are_dupes = [&](int o1, int o2) {
    if (o1 == o2) return false;
    if (!(state.deck[o1].clued && state.deck[o2].clued)) return false;
    if (!game.me().thoughts[o1].matches(state.deck[o2])) return false;
    if (std::find(bad_touch.begin(), bad_touch.end(), o2) != bad_touch.end()) return false;
    if (std::find(trash.begin(), trash.end(), o2) != trash.end()) return false;
    return !game.common.thoughts[o1].id().has_value() ||
            !game.common.thoughts[o2].id().has_value();
  };

  // First pass: detect new-clue trash + dupes within target's hand.
  for (auto it = state.hands[target].rbegin(); it != state.hands[target].rend(); ++it) {
    int order = *it;
    if (prev.state.deck[order].clued || !state.deck[order].clued) continue;
    if (game.common.order_trash(game, order)) {
      trash.insert(trash.begin(), order);
      continue;
    }
    auto order_id = state.deck[order].id();
    bool is_dup = false;
    if (order_id) {
      if (state.is_basic_trash(*order_id)) {
        is_dup = true;
      } else {
        for (int o : state.hands[target]) {
          if (are_dupes(order, o)) {
            is_dup = true;
            break;
          }
        }
      }
    }
    if (is_dup) bad_touch.insert(bad_touch.begin(), order);
  }

  // Second pass: detect duplicates across all hands.
  for (auto it = state.hands[target].rbegin(); it != state.hands[target].rend(); ++it) {
    int order = *it;
    if (std::find(bad_touch.begin(), bad_touch.end(), order) != bad_touch.end()) continue;
    if (std::find(trash.begin(), trash.end(), order) != trash.end()) continue;
    if (!(!prev.state.deck[order].clued && state.deck[order].clued)) continue;

    bool duplicated = false;
    for (const auto& hand : state.hands) {
      for (int o : hand) {
        if ((prev.is_touched(o) || game.is_touched(o)) && are_dupes(order, o)) {
          duplicated = true;
          break;
        }
      }
      if (duplicated) break;
    }
    if (duplicated) bad_touch.insert(bad_touch.begin(), order);
  }

  return {std::move(bad_touch), std::move(trash), avoidable_dupe};
}

std::pair<std::vector<int>, std::vector<int>> playables_result(const Game& prev,
                                                                  const Game& game) {
  std::vector<int> blind_plays;
  std::vector<int> playables;

  std::vector<int> sorted_hypo(game.me().hypo_plays.begin(),
                                 game.me().hypo_plays.end());
  std::sort(sorted_hypo.begin(), sorted_hypo.end());

  for (int order : sorted_hypo) {
    if (prev.me().hypo_plays.count(order)) continue;
    auto id = game.me().thoughts[order].id(/*infer=*/true);
    bool bad = false;
    if (id) {
      if (prev.me().hypo_stacks[id->suit_index] >= id->rank) {
        bad = true;
      } else {
        for (int o : prev.me().hypo_plays) {
          if (game.me().thoughts[o].matches(*id, /*infer=*/true)) {
            bad = true;
            break;
          }
        }
      }
    }
    if (bad) continue;

    if (game.is_blind_playing(order) && !prev.is_blind_playing(order)) {
      blind_plays.push_back(order);
      playables.push_back(order);
    } else {
      playables.push_back(order);
    }
  }
  return {std::move(blind_plays), std::move(playables)};
}

}  // namespace hanabi
