// Implementations that do not require Game's full type.
// Game-dependent methods (refer, chop_newest, is_duped, order_kt, …) live in
// src/basics/player_game.cpp.
#include "hanabi/basics/player.h"

#include <algorithm>

#include "hanabi/basics/state.h"

namespace hanabi {

// --- Link helpers ---------------------------------------------------------

const std::vector<int>& link_orders(const Link& link) {
  return std::visit([](const auto& l) -> const std::vector<int>& { return l.orders; }, link);
}

std::optional<Identity> link_promise(const Link& link) {
  return std::visit(
      [](const auto& l) -> std::optional<Identity> {
        if constexpr (std::is_same_v<std::decay_t<decltype(l)>, UnpromisedLink>) {
          return std::nullopt;
        } else {
          return l.promise();
        }
      },
      link);
}

// --- Player ---------------------------------------------------------------

Player Player::create(int player_index, std::string name, IdentitySet all_possible,
                       std::vector<int> hypo_stacks) {
  Player p;
  p.player_index = player_index;
  p.name = std::move(name);
  p.all_possible = all_possible;
  p.hypo_stacks = std::move(hypo_stacks);
  p.is_common = (player_index == -1);
  p.certain_map.assign(all_possible.length(), {});
  return p;
}

Player Player::with_thought(int order, const std::function<Thought(const Thought&)>& f) const {
  Player out = *this;
  out.thoughts[order] = f(thoughts[order]);
  out.dirty.insert(order);
  return out;
}

std::string Player::str_infs(const State& state, int order) const {
  std::vector<Identity> ids = thoughts[order].inferred.to_list();
  std::sort(ids.begin(), ids.end(), [](Identity a, Identity b) { return a.to_ord() < b.to_ord(); });
  std::string out;
  for (size_t i = 0; i < ids.size(); ++i) {
    if (i) out += ',';
    out += state.log_id(ids[i]);
  }
  return out;
}

std::string Player::str_poss(const State& state, int order) const {
  std::vector<Identity> ids = thoughts[order].possible.to_list();
  std::sort(ids.begin(), ids.end(), [](Identity a, Identity b) { return a.to_ord() < b.to_ord(); });
  std::string out;
  for (size_t i = 0; i < ids.size(); ++i) {
    if (i) out += ',';
    out += state.log_id(ids[i]);
  }
  return out;
}

int Player::unknown_ids(const State& state, Identity id) const {
  int visible = 0;
  for (const auto& hand : state.hands) {
    for (int o : hand) {
      if (thoughts[o].id() == id) ++visible;
    }
  }
  return state.card_count[id.to_ord()] - state.base_count[id.to_ord()] - visible;
}

std::vector<int> Player::linked_orders(const State& state) const {
  std::vector<int> out;
  for (const Link& link : links) {
    std::visit(
        [&](const auto& l) {
          using T = std::decay_t<decltype(l)>;
          if constexpr (std::is_same_v<T, PromisedLink> || std::is_same_v<T, SarcasticLink>) {
            if (static_cast<int>(l.orders.size()) > unknown_ids(state, l.id)) {
              out.insert(out.end(), l.orders.begin(), l.orders.end());
            }
          } else {
            int total = 0;
            for (Identity i : l.ids) total += unknown_ids(state, i);
            if (static_cast<int>(l.orders.size()) > total) {
              out.insert(out.end(), l.orders.begin(), l.orders.end());
            }
          }
        },
        link);
  }
  return out;
}

int Player::hypo_score() const {
  int total = 0;
  for (int v : hypo_stacks) total += v;
  return total + static_cast<int>(unknown_plays.size()) - linked_plays;
}

// --- Free functions -------------------------------------------------------

std::vector<int> visible_find(const State& state, const Player& player, Identity id,
                                int exclude_order) {
  std::vector<int> out;
  for (const auto& hand : state.hands) {
    for (int o : hand) {
      if (o == exclude_order) continue;
      if (!player.thoughts[o].matches(id, /*infer=*/true)) continue;
      if (!state.deck[o].matches(id, /*assume=*/true)) continue;
      out.push_back(o);
    }
  }
  return out;
}

std::vector<int> players_until(int num_players, int start, int target) {
  std::vector<int> result;
  int i = start;
  while (i != target) {
    result.push_back(i);
    i = (i + 1) % num_players;
  }
  return result;
}

std::pair<std::vector<Player>, Player> gen_players(const State& state) {
  std::vector<int> hypo_stacks(state.variant->suits.size(), 0);
  std::vector<Player> players;
  players.reserve(state.num_players);
  for (int i = 0; i < state.num_players; ++i) {
    players.push_back(Player::create(i, state.names[i], state.all_ids, hypo_stacks));
  }
  Player common = Player::create(-1, "common", state.all_ids, hypo_stacks);
  return {std::move(players), std::move(common)};
}

}  // namespace hanabi
