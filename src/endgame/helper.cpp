#include "hanabi/endgame/helper.h"

#include <algorithm>
#include <stdexcept>

#include "hanabi/basics/game.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

namespace hanabi::endgame {

RemainingMap remaining_remove(const RemainingMap& remaining, Identity id) {
  RemainingMap out = remaining;
  auto it = out.find(id.to_ord());
  if (it == out.end()) {
    throw std::invalid_argument("remaining_remove: id not in map");
  }
  if (it->second == 1) {
    out.erase(it);
  } else {
    --it->second;
  }
  return out;
}

int remaining_total(const RemainingMap& remaining) {
  int total = 0;
  for (const auto& [_, v] : remaining) total += v;
  return total;
}

std::vector<Identity> find_must_plays(const State& state, const std::vector<int>& hand) {
  std::vector<std::optional<Identity>> ids;
  ids.reserve(hand.size());
  for (int o : hand) ids.push_back(state.deck[o].id());

  std::vector<Identity> ret;
  for (size_t i = 0; i < hand.size(); ++i) {
    if (!ids[i]) continue;
    Identity id = *ids[i];
    if (!state.is_useful(id)) continue;
    int matches = 1;
    for (size_t j = i + 1; j < hand.size(); ++j) {
      if (ids[j] && ids[j]->to_ord() == id.to_ord()) ++matches;
    }
    if (matches == state.card_count[id.to_ord()] - state.base_count[id.to_ord()]) {
      ret.insert(ret.begin(), id);
    }
  }
  return ret;
}

bool unwinnable_state(const State& state, int player_turn, int /*depth*/) {
  if (state.ended() || state.pace() < 0) return true;

  std::vector<bool> is_void(state.num_players, false);
  std::vector<int> must_plays(state.num_players, 0);
  std::vector<int> must_start_endgame;

  for (int i = state.num_players - 1; i >= 0; --i) {
    const auto& hand = state.hands[i];
    bool void_p = true;
    for (int o : hand) {
      auto id = state.deck[o].id();
      if (id && !state.is_basic_trash(*id)) {
        void_p = false;
        break;
      }
    }
    if (void_p) is_void[i] = true;
    auto plays = find_must_plays(state, hand);
    must_plays[i] += static_cast<int>(plays.size());
    if (plays.size() > 1) must_start_endgame.insert(must_start_endgame.begin(), i);
  }

  if (state.endgame_turns) {
    int possible_players = 0;
    int double_play = -1;
    for (int i = 0; i < *state.endgame_turns; ++i) {
      int pi = (player_turn + i) % state.num_players;
      if (!is_void[pi]) {
        ++possible_players;
        if (must_plays[pi] > 1) double_play = i;
      }
    }
    if (possible_players + state.score() < state.max_score()) return true;
    if (double_play != -1) return true;
  }

  if (state.cards_left == 1) {
    if (must_start_endgame.size() > 1) return true;
    if (must_start_endgame.size() == 1) {
      int target = must_start_endgame[0];
      if (player_turn != target) {
        int hops = 0;
        int i = player_turn;
        while (i != target) {
          ++hops;
          i = (i + 1) % state.num_players;
        }
        if (hops > state.clue_tokens) return true;
      }
    }
  } else if (!state.endgame_turns) {
    int void_count = 0;
    for (bool v : is_void) if (v) ++void_count;
    if (void_count > state.pace()) return true;
  }

  return false;
}

TriviallyResult trivially_winnable(const Game& game, int player_turn) {
  const State& state = game.state;
  TriviallyResult r;
  if (!state.endgame_turns) return r;
  int endgame_turns = *state.endgame_turns;
  if (state.rem_score() > endgame_turns) return r;

  PerformAction perform = PerformDiscard{state.hands[player_turn].front()};
  std::vector<int> play_stacks = state.play_stacks;
  for (int i = 0; i < endgame_turns; ++i) {
    int pi = (player_turn + i) % state.num_players;
    auto playables = game.players[pi].obvious_playables(game, pi);
    if (playables.empty()) continue;
    int first = playables.front();
    auto id = state.deck[first].id();
    if (!id) continue;
    if (i == 0) perform = PerformPlay{first};
    play_stacks[id->suit_index] = id->rank;
  }
  int sum = 0;
  for (int v : play_stacks) sum += v;
  if (sum == state.max_score()) {
    r.found = true;
    r.actions = {perform};
    r.winrate = Fraction(1);
  }
  return r;
}

std::pair<std::vector<GameArr>, std::vector<GameArr>> gen_arrs(const Game& game,
                                                                  const RemainingMap& remaining,
                                                                  bool clue_only) {
  const State& state = game.state;
  GameArr undrawn{Fraction(1), remaining, std::nullopt};

  int rem_total = remaining_total(remaining);
  if (rem_total != state.cards_left) {
    throw std::logic_error("gen_arrs: remaining_total does not match cards_left");
  }

  std::vector<GameArr> drawn;
  if (clue_only) {
    // empty
  } else {
    bool all_trash = !remaining.empty();
    for (const auto& [ord, _] : remaining) {
      Identity id = Identity::from_ord(ord);
      if (!state.is_basic_trash(id)) {
        all_trash = false;
        break;
      }
    }
    if (all_trash) {
      // Short-circuit.
      Identity id = Identity::from_ord(remaining.begin()->first);
      drawn.push_back(GameArr{Fraction(1), remaining_remove(remaining, id), id});
    } else {
      std::vector<GameArr> useful_arrs;
      Fraction trash_prob = Fraction(0);
      RemainingMap trash_remaining = remaining;
      std::optional<Identity> trash_drew;
      for (const auto& [ord, missing] : remaining) {
        Identity id = Identity::from_ord(ord);
        Fraction prob = Fraction(missing, state.cards_left);
        RemainingMap new_remaining = remaining_remove(remaining, id);
        if (state.is_basic_trash(id)) {
          trash_prob += prob;
          trash_remaining = new_remaining;
          trash_drew = id;
        } else {
          useful_arrs.insert(useful_arrs.begin(),
                             GameArr{prob, std::move(new_remaining), id});
        }
      }
      if (trash_prob > Fraction(0)) {
        drawn = std::move(useful_arrs);
        drawn.push_back(GameArr{trash_prob, std::move(trash_remaining), trash_drew});
      } else {
        drawn = std::move(useful_arrs);
      }
      if (drawn.empty()) drawn = {undrawn};
    }
  }

  return {{undrawn}, std::move(drawn)};
}

}  // namespace hanabi::endgame
