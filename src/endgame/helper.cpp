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

bool certainly_advances(const Game& game, int order, const PerformAction& how) {
  const State& state = game.state;
  const bool want_play = std::holds_alternative<PerformPlay>(how);
  if (!want_play && !std::holds_alternative<PerformDiscard>(how)) return false;
  // Pressing Discard is only legal below 8 tokens, so a chuck that cannot be
  // performed is not a certain play.
  if (!want_play && state.clue_tokens >= 8) return false;

  const IdentitySet live = game.me().thoughts[order].possibilities();
  if (live.is_empty()) return false;
  return live.forall([&](Identity i) {
    const bool inverted = state.variant->suits[i.suit_index].suit_type.inverted;
    // Play advances a plain card; Discard advances an inverted one. When the
    // reading set spans both, one half fails here -- correctly, since no single
    // button covers both.
    if (want_play == inverted) return false;
    return state.is_playable(i);
  });
}

std::vector<PerformAction> possible_call_actions(const Game& game) {
  const State& state = game.state;
  std::vector<PerformAction> out;
  for (int order : state.our_hand()) {
    const CardStatus st = game.meta[order].status;
    const bool ctp = st == CardStatus::CALLED_TO_PLAY;
    const bool ctd = st == CardStatus::CALLED_TO_DISCARD;
    if (!ctp && !ctd) continue;
    if (ctd && state.clue_tokens >= 8) continue;  // a discard is illegal here

    const IdentitySet live = game.me().thoughts[order].possibilities();
    if (live.is_empty()) continue;
    const bool could = live.exists([&](Identity i) {
      const bool inverted = state.variant->suits[i.suit_index].suit_type.inverted;
      // Play advances a plain card, Discard an inverted one -- the same pairing
      // `certainly_advances` uses, asked with `exists` instead of `forall`.
      if (ctp == inverted) return false;
      return state.is_playable(i);
    });
    if (!could) continue;
    out.push_back(ctp ? PerformAction{PerformPlay{order}}
                      : PerformAction{PerformDiscard{order}});
  }
  return out;
}

std::vector<PerformAction> certain_plays(const Game& game) {
  std::vector<PerformAction> out;
  for (int order : game.state.our_hand()) {
    PerformAction play{PerformPlay{order}};
    if (certainly_advances(game, order, play)) {
      out.push_back(play);
      continue;
    }
    PerformAction chuck{PerformDiscard{order}};
    if (certainly_advances(game, order, chuck)) out.push_back(chuck);
  }
  return out;
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

  auto inverted = [&](Identity id) {
    return state.variant->suits[id.suit_index].suit_type.inverted;
  };

  // The filler action, used when CP has no play of its own at `i == 0`. It has
  // to be certainly safe, because this function reports `Fraction(1)` — a
  // claimed certain win. Pressing Discard on an inverted (Orange / Dark Orange)
  // card is a play attempt that strikes unless it is currently playable, so a
  // KNOWN orange is pitched (press Play) instead, and a card that might be an
  // unplayable orange makes the promise unsound — see `filler_unsafe` below.
  // v6.1.0 routed the `i == 0` overwrite but left this default untouched.
  const int filler = state.hands[player_turn].front();
  const Thought& filler_thought = game.players[player_turn].thoughts[filler];
  auto filler_known = filler_thought.id(/*infer=*/true);
  bool filler_unsafe = false;
  PerformAction perform = PerformDiscard{filler};
  if (filler_known && inverted(*filler_known)) {
    perform = PerformPlay{filler};
  } else if (!filler_known) {
    filler_unsafe = filler_thought.possible.exists(
        [&](Identity i) { return inverted(i) && !state.is_playable(i); });
  }
  bool used_filler = true;

  std::vector<int> play_stacks = state.play_stacks;
  for (int i = 0; i < endgame_turns; ++i) {
    int pi = (player_turn + i) % state.num_players;
    auto playables = game.players[pi].obvious_playables(game, pi);
    if (playables.empty()) continue;
    int first = playables.front();
    auto id = state.deck[first].id();
    if (!id) continue;
    if (i == 0) {
      // The emitted action and the credited stack must agree. On an inverted
      // (Orange / Dark Orange) suit the button that advances the stack is
      // Discard — emitting PerformPlay pitched the card into the discard pile
      // while still crediting the rank below, so this claimed a certain win
      // for an action that could not achieve it.
      perform = inverted(*id) ? PerformAction{PerformDiscard{first}}
                              : PerformAction{PerformPlay{first}};
      used_filler = false;
    }
    play_stacks[id->suit_index] = id->rank;
  }
  int sum = 0;
  for (int v : play_stacks) sum += v;
  if (sum == state.max_score() && !(used_filler && filler_unsafe)) {
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
