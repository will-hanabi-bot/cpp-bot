#include "hanabi/conventions/reactor/interpret_reactive.h"

#include <algorithm>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor/interpret_clue.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"

namespace hanabi::reactor {

namespace {

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

struct ReactiveContext {
  std::vector<std::pair<int, Identity>> possible_conns;
  std::vector<int> known_plays;
  State hypo_state;
};

// Compute (possible_conns, known_plays, hypo_state) for the reactive
// interpretation. Port of _reactive_context.
ReactiveContext reactive_context(const Game& prev, const Game& game,
                                    const ClueAction& action, int reacter) {
  int giver = action.giver;
  int receiver = action.target;
  const State& state = game.state;

  ReactiveContext ctx;
  ctx.possible_conns = delayed_plays(game, giver, receiver, /*stable=*/false);
  auto old_playables = prev.common.obvious_playables(prev, receiver);
  auto new_playables = game.common.obvious_playables(game, receiver);
  for (int o : old_playables) {
    if (contains(new_playables, o)) ctx.known_plays.push_back(o);
  }

  State after_others = prev.state;
  for (int i : players_until(state.num_players, state.next_player_index(giver),
                                reacter)) {
    Game hypo_prev = prev;
    hypo_prev.state = after_others;
    auto ps = prev.common.obvious_playables(hypo_prev, i);
    std::optional<int> playable_o;
    for (int o : ps) {
      if (prev.meta[o].urgent) {
        playable_o = o;
        break;
      }
    }
    if (!playable_o && !ps.empty()) playable_o = ps.front();
    if (!playable_o) continue;
    auto id = state.deck[*playable_o].id();
    if (id) after_others = after_others.try_play(*id);
  }

  ctx.hypo_state = after_others;
  Game hypo_prev = prev;
  hypo_prev.state = after_others;
  auto self_plays = prev.common.obvious_playables(hypo_prev, receiver);
  for (int o : self_plays) {
    auto id = state.deck[o].id();
    if (id) ctx.hypo_state = ctx.hypo_state.try_play(*id);
  }
  return ctx;
}

}  // namespace

// --- interpret_reactive_colour ------------------------------------------

std::optional<ClueInterp> interpret_reactive_colour(const Game& prev, Game& game,
                                                       const ClueAction& action,
                                                       int focus_slot, int reacter,
                                                       bool looks_stable) {
  const State& state = game.state;
  int receiver = action.target;
  ReactiveContext ctx = reactive_context(prev, game, action, reacter);

  // Find play targets in receiver's hand.
  std::vector<std::pair<int, int>> play_targets;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (game.meta[o].status == CardStatus::CALLED_TO_DISCARD) continue;
    if (contains(ctx.known_plays, o)) continue;
    auto id = state.deck[o].id();
    if (!id || !ctx.hypo_state.is_playable(*id)) continue;
    play_targets.emplace_back(o, static_cast<int>(i));
  }

  // Sort: unclued dupe → 99 (last); else by slot index.
  auto sort_key = [&](const std::pair<int, int>& t) {
    int o = t.first;
    bool unclued_dupe = !prev.state.deck[o].clued;
    if (unclued_dupe) {
      for (int o2 : state.hands[receiver]) {
        if (o2 < o && prev.state.deck[o2].clued &&
            state.deck[o].matches(state.deck[o2])) {
          return 99;
        }
      }
    }
    return t.second;
  };
  std::sort(play_targets.begin(), play_targets.end(),
             [&](const auto& a, const auto& b) { return sort_key(a) < sort_key(b); });

  int hand_size = kHandSize[state.num_players];
  for (const auto& [_target, index] : play_targets) {
    int target_slot = index + 1;
    int react_slot = calc_slot(focus_slot, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    if (looks_stable) {
      auto rkt = prev.common.thinks_trash(prev, reacter);
      auto rop = prev.common.obvious_playables(prev, reacter);
      if (contains(rkt, react_order) && rop.empty()) continue;
    }
    if (game.common.thoughts[react_order].possible.forall(
            [&](Identity i) { return state.is_critical(i); })) {
      continue;
    }
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    auto interp = target_discard(game, action, react_order, /*urgent=*/true);
    if (!interp) return std::nullopt;
    return ClueInterp::REACTIVE;
  }

  // Discard targets.
  auto prev_kt = prev.common.thinks_trash(prev, receiver);
  std::vector<std::pair<int, int>> unknown_trash;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (contains(prev_kt, o)) continue;
    auto deck_id = state.deck[o].id();
    if (!deck_id) continue;
    bool is_trash = state.is_basic_trash(*deck_id);
    if (!is_trash) {
      for (int o2 : state.hands[receiver]) {
        if (o2 < o && state.deck[o].matches(state.deck[o2])) {
          is_trash = true;
          break;
        }
      }
    }
    if (is_trash) unknown_trash.emplace_back(o, static_cast<int>(i));
  }
  std::sort(unknown_trash.begin(), unknown_trash.end(),
             [&](const auto& a, const auto& b) {
               int ka = prev.state.deck[a.first].clued ? -1 : 1;
               int kb = prev.state.deck[b.first].clued ? -1 : 1;
               return ka < kb;
             });

  std::vector<std::pair<int, int>> unknown_dupes;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (contains(prev_kt, o)) continue;
    bool dup = false;
    for (const auto& hand : state.hands) {
      for (int o2 : hand) {
        if (o2 != o && game.common.thoughts[o2].matches(state.deck[o], /*infer=*/true)) {
          dup = true;
          break;
        }
      }
      if (dup) break;
    }
    if (dup) unknown_dupes.emplace_back(o, static_cast<int>(i));
  }

  std::vector<std::pair<int, int>> known_trash;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    auto id = state.deck[o].id();
    if (id && state.is_basic_trash(*id)) {
      known_trash.emplace_back(o, static_cast<int>(i));
    }
  }

  std::vector<std::pair<int, int>> sacrifices;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (contains(prev_kt, o)) continue;
    auto id = state.deck[o].id();
    if (id && !state.is_critical(*id)) sacrifices.emplace_back(o, static_cast<int>(i));
  }
  std::sort(sacrifices.begin(), sacrifices.end(),
             [&](const auto& a, const auto& b) {
               auto ai = state.deck[a.first].id();
               auto bi = state.deck[b.first].id();
               int ka = -game.common.playable_away(*ai) * 10 + (5 - ai->rank);
               int kb = -game.common.playable_away(*bi) * 10 + (5 - bi->rank);
               return ka < kb;
             });

  std::vector<std::pair<int, int>> dc_targets;
  if (!unknown_trash.empty()) dc_targets = unknown_trash;
  else if (!known_trash.empty()) dc_targets = known_trash;
  else if (!unknown_dupes.empty()) dc_targets = unknown_dupes;
  else dc_targets = sacrifices;

  if (dc_targets.empty()) return std::nullopt;

  for (const auto& [target, index] : dc_targets) {
    if (state.next_player_index(action.giver) != reacter &&
        game.meta[target].status == CardStatus::CALLED_TO_PLAY) {
      continue;
    }
    int target_slot = index + 1;
    int react_slot = calc_slot(focus_slot, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    auto prev_plays = prev.common.obvious_playables(prev, reacter);
    if (contains(prev_plays, react_order)) continue;
    bool ok = game.common.thoughts[react_order].possible.exists([&](Identity i) {
      if (state.playable_set.contains(i)) return true;
      for (const auto& [_, c] : ctx.possible_conns) {
        if (c == i) return true;
      }
      return false;
    });
    if (!ok) continue;
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    auto interp = target_play(game, action, react_order, /*urgent=*/true, /*stable=*/false);
    if (!interp) return std::nullopt;
    return ClueInterp::REACTIVE;
  }
  return std::nullopt;
}

// --- interpret_reactive_rank --------------------------------------------

std::optional<ClueInterp> interpret_reactive_rank(const Game& prev, Game& game,
                                                     const ClueAction& action,
                                                     int focus_slot, int reacter) {
  const State& state = game.state;
  int receiver = action.target;
  ReactiveContext ctx = reactive_context(prev, game, action, reacter);

  std::vector<std::pair<int, int>> play_targets;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (game.meta[o].status == CardStatus::CALLED_TO_DISCARD) continue;
    if (contains(ctx.known_plays, o)) continue;
    auto id = state.deck[o].id();
    if (!id || !ctx.hypo_state.is_playable(*id)) continue;
    play_targets.emplace_back(o, static_cast<int>(i));
  }
  auto sort_key = [&](const std::pair<int, int>& t) {
    int o = t.first;
    if (!prev.state.deck[o].clued) {
      for (int o2 : state.hands[receiver]) {
        if (o2 != o && prev.state.deck[o2].clued &&
            state.deck[o].matches(state.deck[o2])) {
          return 99;
        }
      }
    }
    return t.second;
  };
  std::sort(play_targets.begin(), play_targets.end(),
             [&](const auto& a, const auto& b) { return sort_key(a) < sort_key(b); });

  int hand_size = kHandSize[state.num_players];
  for (const auto& [target, index] : play_targets) {
    int target_slot = index + 1;
    int react_slot = calc_slot(focus_slot, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    auto prev_plays = prev.common.obvious_playables(prev, reacter);
    if (contains(prev_plays, react_order)) continue;
    bool ok = game.common.thoughts[react_order].possible.exists([&](Identity i) {
      if (state.playable_set.contains(i)) return true;
      for (const auto& [_, c] : ctx.possible_conns) {
        if (c == i) return true;
      }
      return false;
    });
    if (!ok) continue;
    auto interp = target_play(game, action, react_order, /*urgent=*/true, /*stable=*/false);
    if (!interp) return std::nullopt;
    auto target_id = state.deck[target].id();
    if (target_id) {
      Identity ti = *target_id;
      game.with_thought(react_order, [ti](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.difference(ti);
        return out;
      });
    }
    return ClueInterp::REACTIVE;
  }

  // Finesse fallback.
  std::vector<std::pair<int, int>> finesse_targets;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    auto id = state.deck[o].id();
    if (id && state.playable_away(*id) == 1) {
      finesse_targets.emplace_back(o, static_cast<int>(i));
    }
  }
  if (finesse_targets.empty()) return std::nullopt;

  for (int react_slot : {1, 5, 4, 3, 2}) {
    int target_slot = calc_slot(focus_slot, react_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    std::optional<std::pair<int, int>> receive;
    for (const auto& [o, i] : finesse_targets) {
      if (i + 1 == target_slot) {
        receive = std::make_pair(o, i);
        break;
      }
    }
    if (!receive) continue;
    int receive_order = receive->first;
    auto prev_plays = prev.common.obvious_playables(prev, reacter);
    if (contains(prev_plays, react_order)) continue;
    auto deck_id = state.deck[receive_order].id();
    if (!deck_id) continue;
    auto prev_id = deck_id->prev();
    if (!prev_id) continue;
    bool ok = game.common.thoughts[react_order].possible.exists([&](Identity i) {
      if (state.playable_set.contains(i)) return true;
      for (const auto& [_, c] : ctx.possible_conns) {
        if (c == i) return true;
      }
      return false;
    });
    if (!ok) continue;
    if (!game.common.thoughts[react_order].possible.contains(*prev_id)) continue;
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    auto interp = target_play(game, action, react_order, /*urgent=*/true, /*stable=*/false);
    if (!interp) return std::nullopt;
    Identity pi = *prev_id;
    game.with_thought(react_order, [pi](const Thought& t) {
      Thought out = t;
      out.inferred = IdentitySet::single(pi);
      return out;
    });
    return ClueInterp::REACTIVE;
  }

  return std::nullopt;
}

}  // namespace hanabi::reactor
