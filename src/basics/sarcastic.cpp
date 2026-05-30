#include "hanabi/basics/sarcastic.h"

#include <algorithm>
#include <unordered_set>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"

namespace hanabi {

bool valid_transfer(const Game& game, Identity id, int order) {
  const Thought& thought = game.common.thoughts[order];
  if (!thought.possible.contains(id)) return false;
  auto looked = thought.id(/*infer=*/true, /*symmetric=*/true);
  if (looked && looked->rank < id.rank) return false;
  return !thought.info_lock || thought.info_lock->contains(id);
}

namespace {

// Recursive search for a gentleman's-discard chain in `holder`'s hand.
// Returns the chain (newest-to-oldest order list) on success, nullopt otherwise.
std::optional<std::vector<int>> find_gd(const Game& game, Identity id, int holder,
                                          State hypo_state,
                                          std::unordered_set<int> connected) {
  const auto& hand = game.state.hands[holder];
  // Scan rightmost first ("findLast" in Scala).
  for (auto it = hand.rbegin(); it != hand.rend(); ++it) {
    int f = *it;
    if (connected.count(f)) continue;
    if (!game.common.thoughts[f].possible.contains(id)) continue;

    std::optional<Identity> finesse_id;
    if (f < static_cast<int>(game.future.size()) && game.future[f].length() == 1) {
      finesse_id = game.future[f].head();
    } else {
      finesse_id = game.me().thoughts[f].id();
    }

    if (!finesse_id) return std::vector<int>{f};
    if (*finesse_id == id) return std::vector<int>{f};
    if (hypo_state.is_playable(*finesse_id)) {
      auto next_connected = connected;
      next_connected.insert(f);
      auto rest = find_gd(game, id, holder, hypo_state.with_play(*finesse_id),
                           std::move(next_connected));
      if (!rest) return std::nullopt;
      std::vector<int> chain{f};
      chain.insert(chain.end(), rest->begin(), rest->end());
      return chain;
    }
    return std::nullopt;
  }
  return std::nullopt;
}

DiscardResult try_finding(const Game& game, const DiscardAction& action,
                            std::unordered_set<int> excluding) {
  const State& state = game.state;
  const Identity id{action.suit_index, action.rank};
  const bool gd = state.is_playable(id);

  while (true) {
    std::optional<int> dupe;
    for (const auto& hand : state.hands) {
      for (int o : hand) {
        if (excluding.count(o)) continue;
        if (game.order_matches(o, id)) {
          dupe = o;
          break;
        }
      }
      if (dupe) break;
    }

    if (dupe) {
      int holder = state.holder_of(*dupe);
      if (holder == action.player_index_v) {
        if (state.card_count[id.to_ord()] - state.base_count[id.to_ord()] > 1) {
          excluding.insert(*dupe);
          continue;
        }
        return DiscardResultNone{};
      }
      if (gd) {
        auto gd_chain = find_gd(game, id, holder, state, {});
        if (!gd_chain) {
          if (state.card_count[id.to_ord()] - state.base_count[id.to_ord()] > 1) {
            excluding.insert(*dupe);
            continue;
          }
          return DiscardResultMistake{};
        }
        return DiscardResultGentlemansDiscard{std::move(*gd_chain)};
      }
      std::vector<int> orders;
      for (int o : state.hands[holder]) {
        if (valid_transfer(game, id, o)) orders.push_back(o);
      }
      return DiscardResultSarcastic{std::move(orders)};
    }

    // No visible dupe.
    if (action.player_index_v == state.our_player_index) {
      if (game.meta[action.order].status == CardStatus::CALLED_TO_DISCARD) {
        return DiscardResultNone{};
      }
      return DiscardResultMistake{};
    }

    if (gd) {
      auto gd_chain = find_gd(game, id, state.our_player_index, state, {});
      if (!gd_chain) return DiscardResultMistake{};
      bool linked = gd_chain->size() == 1 &&
                     state.deck[gd_chain->front()].clued &&
                     game.common.order_playable(game, gd_chain->front());
      if (linked) {
        std::vector<int> matching;
        for (int o : state.hands[state.our_player_index]) {
          if (state.deck[o].clued && game.common.thoughts[o].possible.contains(id)) {
            matching.push_back(o);
          }
        }
        return DiscardResultSarcastic{std::move(matching)};
      }
      return DiscardResultGentlemansDiscard{std::move(*gd_chain)};
    }
    std::vector<int> orders;
    for (int o : state.our_hand()) {
      if (valid_transfer(game, id, o)) orders.push_back(o);
    }
    return DiscardResultSarcastic{std::move(orders)};
  }
}

}  // namespace

DiscardResult interpret_useful_dc(const Game& game, const DiscardAction& action) {
  return try_finding(game, action, {});
}

}  // namespace hanabi
