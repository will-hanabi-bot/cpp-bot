#include "hanabi/endgame/solver.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/endgame/winnable.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::endgame {

namespace {

double monotonic_seconds() {
  using clock = std::chrono::steady_clock;
  return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

bool past_deadline_local(std::optional<double> deadline) {
  return deadline && monotonic_seconds() > *deadline;
}

bool perform_eq(const PerformAction& a, const PerformAction& b) { return a == b; }

// True iff this PerformAction is a play (as opposed to a clue or discard).
// Used to break winrate ties — when several candidates produce the same
// winrate, the user prefers the one that plays a card.
bool is_play_perform(const PerformAction& p) {
  return std::holds_alternative<PerformPlay>(p);
}

}  // namespace

// --- perform_to_action (public, used by winnable.cpp's advance_game) ----

Action EndgameSolver::perform_to_action(const PerformAction& perform, const Game& game,
                                          int player_index) {
  const State& state = game.state;
  return std::visit(
      [&](const auto& p) -> Action {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, PerformPlay>) {
          if (p.target < static_cast<int>(state.deck.size())) {
            auto id = state.deck[p.target].id();
            if (id) return PlayAction{player_index, p.target, id->suit_index, id->rank};
          }
          return PlayAction{player_index, p.target, -1, -1};
        } else if constexpr (std::is_same_v<T, PerformDiscard>) {
          if (p.target < static_cast<int>(state.deck.size())) {
            auto id = state.deck[p.target].id();
            if (id) return DiscardAction{player_index, p.target, id->suit_index, id->rank, false};
          }
          return DiscardAction{player_index, p.target, -1, -1, false};
        } else if constexpr (std::is_same_v<T, PerformColour>) {
          BaseClue clue{ClueKind::COLOUR, p.value};
          auto touched = state.clue_touched(state.hands[p.target], ClueKind::COLOUR, p.value);
          return ClueAction{player_index, p.target, std::move(touched), clue};
        } else if constexpr (std::is_same_v<T, PerformRank>) {
          BaseClue clue{ClueKind::RANK, p.value};
          auto touched = state.clue_touched(state.hands[p.target], ClueKind::RANK, p.value);
          return ClueAction{player_index, p.target, std::move(touched), clue};
        } else {
          throw std::invalid_argument("Cannot convert PerformTerminate to Action");
        }
      },
      perform);
}

// --- cache helpers ------------------------------------------------------

void EndgameSolver::cache_result(int depth, const PerformAction& perform, Fraction winrate) {
  auto& bucket = success_rate_[depth];
  for (auto& [p, f, count] : bucket) {
    if (perform_eq(p, perform)) {
      f = (f * Fraction(count) + winrate) / Fraction(count + 1);
      ++count;
      return;
    }
  }
  bucket.emplace_back(perform, winrate, 1);
}

std::optional<Fraction> EndgameSolver::cached_winrate(int depth,
                                                         const PerformAction& perform) const {
  auto it = success_rate_.find(depth);
  if (it == success_rate_.end()) return std::nullopt;
  for (const auto& [p, f, _] : it->second) {
    if (perform_eq(p, perform)) return f;
  }
  return std::nullopt;
}

// --- possible_actions ---------------------------------------------------

std::vector<EndgameSolver::ActionEntry> EndgameSolver::possible_actions(
    const Game& game, int player_turn, const RemainingMap& remaining,
    std::optional<double> deadline, int depth, bool infer) {
  const State& state = game.state;

  auto try_action = [&](const PerformAction& perform) -> std::optional<ActionEntry> {
    auto res = winnable_if(game, player_turn, perform, remaining, deadline, depth);
    if (std::holds_alternative<SimpleResult>(res)) {
      if (std::get<SimpleResult>(res) == SimpleResult::UNWINNABLE) return std::nullopt;
      return ActionEntry{perform, {}};
    }
    return ActionEntry{perform, std::get<WinnableWithDraws>(res).draws};
  };

  // Urgent first.
  std::optional<PerformAction> urgent_perform;
  for (int urgent : state.hands[player_turn]) {
    if (game.meta[urgent].urgent) {
      CardStatus status = game.meta[urgent].status;
      urgent_perform = (status == CardStatus::CALLED_TO_PLAY)
                            ? PerformAction{PerformPlay{urgent}}
                            : PerformAction{PerformDiscard{urgent}};
      break;
    }
  }
  if (urgent_perform) {
    auto r = try_action(*urgent_perform);
    if (r) return {*r};
  }

  std::vector<int> playables;
  if (infer || game.good_touch || state.endgame_turns) {
    playables = game.players[player_turn].thinks_playables(game, player_turn,
                                                              /*exclude_trash=*/true);
  } else {
    playables = game.players[player_turn].obvious_playables(game, player_turn);
  }

  std::vector<ActionEntry> play_actions;
  for (int order : playables) {
    if (past_deadline_local(deadline)) return {};
    if (!state.deck[order].id()) continue;
    auto r = try_action(PerformPlay{order});
    if (r) play_actions.push_back(*r);
  }

  // Cap consecutive clues.
  bool too_many_clues = false;
  if (state.num_players > 2) {
    int count = 0;
    bool stop = false;
    for (auto it = state.action_list.rbegin(); it != state.action_list.rend() && !stop;
          ++it) {
      for (auto jt = it->rbegin(); jt != it->rend(); ++jt) {
        if (hanabi::requires_draw(*jt)) {
          stop = true;
          break;
        }
        if (std::holds_alternative<ClueAction>(*jt)) ++count;
      }
    }
    if (count > state.num_players + 1) too_many_clues = true;
  }

  PerformAction default_clue = PerformRank{0, 0};
  bool clue_winnable = false;
  if (state.can_clue() && !too_many_clues) {
    auto res = winnable_if(game, player_turn, default_clue, remaining, deadline, depth);
    if (std::holds_alternative<SimpleResult>(res) &&
        std::get<SimpleResult>(res) == SimpleResult::ALWAYS_WINNABLE) {
      clue_winnable = true;
    }
  }

  std::vector<ActionEntry> clue_actions;
  bool should_enumerate_clues = state.can_clue() && !too_many_clues &&
                                  (clue_winnable || depth <= 1);
  if (should_enumerate_clues) {
    bool fully_known = (remaining.empty() ||
                          (remaining.size() == 1 &&
                            state.is_basic_trash(Identity::from_ord(remaining.begin()->first))));
    if (fully_known) {
      for (const auto& hand : state.hands) {
        for (int o : hand) {
          auto id = state.deck[o].id();
          if (!id) continue;
          if (state.is_basic_trash(*id)) continue;
          if (!game.common.thoughts[o].matches(*id, /*infer=*/true)) {
            fully_known = false;
            break;
          }
        }
        if (!fully_known) break;
      }
    }
    auto all_clues = game.find_all_clues(player_turn);
    if (fully_known && !all_clues.empty()) {
      clue_actions.push_back({all_clues.front(), {}});
    } else {
      for (const auto& c : all_clues) clue_actions.push_back({c, {}});
    }
  }

  if (past_deadline_local(deadline)) return {};

  // Discard gate.
  bool ignore_dc = state.pace() == 0 || state.clue_tokens == 8;
  if (!ignore_dc) {
    for (int p : playables) {
      auto id = game.players[player_turn].thoughts[p].id(/*infer=*/true);
      if (!id) continue;
      if (id->rank == 5) {
        ignore_dc = true;
        break;
      }
      bool other_crit = false;
      for (int o : state.hands[player_turn]) {
        if (o == p) continue;
        if (state.deck[o].clued &&
            game.common.thoughts[o].possible.forall(
                [&](Identity i) { return state.is_critical(i); })) {
          other_crit = true;
          break;
        }
      }
      if (other_crit) {
        bool crit_or_match = state.is_critical(*id);
        if (!crit_or_match) {
          for (int o : state.hands[player_turn]) {
            if (o == p) continue;
            if (game.players[player_turn].thoughts[o].matches(*id, /*infer=*/true)) {
              crit_or_match = true;
              break;
            }
          }
        }
        if (crit_or_match) {
          ignore_dc = true;
          break;
        }
      }
    }
  }

  std::vector<ActionEntry> dc_actions;
  if (!ignore_dc) {
    auto discard_candidates = game.find_all_discards(player_turn);
    for (const auto& d : discard_candidates) {
      auto r = try_action(d);
      if (r) dc_actions.push_back(*r);
    }
  }

  // Order.
  bool prefer_dc = true;
  for (int i = 0; i < state.num_players; ++i) {
    if (i == player_turn) continue;
    for (int o : state.hands[i]) {
      auto deck_id = state.deck[o].id();
      if (deck_id && state.is_playable(*deck_id)) {
        prefer_dc = false;
        break;
      }
    }
    if (!prefer_dc) break;
  }

  std::vector<ActionEntry> out;
  if (prefer_dc) {
    for (auto& a : play_actions) out.push_back(std::move(a));
    for (auto& a : dc_actions) out.push_back(std::move(a));
    for (auto& a : clue_actions) out.push_back(std::move(a));
  } else {
    for (auto& a : play_actions) out.push_back(std::move(a));
    for (auto& a : clue_actions) out.push_back(std::move(a));
    for (auto& a : dc_actions) out.push_back(std::move(a));
  }
  return out;
}

// --- action_winrate -----------------------------------------------------

Fraction EndgameSolver::action_winrate(const Game& game, const std::vector<GameArr>& arrs,
                                          const ActionEntry& action, int player_turn,
                                          std::optional<double> deadline) {
  if (past_deadline_local(deadline)) return Fraction(0);
  const auto& [perform, winnable_draws] = action;
  int next_player = game.state.next_player_index(player_turn);
  Fraction total(0);
  for (const auto& arr : arrs) {
    if (past_deadline_local(deadline)) break;
    if (arr.drew && std::find(winnable_draws.begin(), winnable_draws.end(), *arr.drew) ==
                         winnable_draws.end() &&
        !winnable_draws.empty()) {
      continue;
    }
    Action game_action = perform_to_action(perform, game, player_turn);
    Game new_game = game.simulate_action(game_action, arr.drew);
    if (new_game.state.max_score() < game.state.max_score()) continue;
    auto res = winnable(new_game, next_player, arr.remaining, deadline, 1);
    if (!res.ok()) continue;
    total = total + arr.prob * res.winrate;
  }
  return total;
}

// --- optimize_full ------------------------------------------------------

std::vector<std::pair<PerformAction, Fraction>> EndgameSolver::optimize_full(
    const Game& game, const std::pair<std::vector<GameArr>, std::vector<GameArr>>& arrs,
    const std::vector<ActionEntry>& actions, int player_turn,
    std::optional<double> deadline, bool single_hypo) {
  const auto& [undrawn, drawn] = arrs;
  std::vector<std::pair<PerformAction, Fraction>> result;
  bool stop = false;
  for (const auto& act : actions) {
    if (stop || past_deadline_local(deadline)) {
      // Fill remaining actions with 0 winrate so the caller still gets
      // a valid (sorted) list rather than hanging.
      result.emplace_back(act.first, Fraction(0));
      continue;
    }
    const auto& arr_list = is_clue(act.first) ? undrawn : drawn;
    Fraction wr = action_winrate(game, arr_list, act, player_turn, deadline);
    result.emplace_back(act.first, wr);
    // When only one arrangement-hypo is in play, an action that already wins
    // 100% of the time is optimal — remaining actions can at best tie. Skip
    // their evaluation (each call into action_winrate recurses through
    // winnable() over all draws, so this typically saves seconds).
    if (single_hypo && wr == Fraction(1)) stop = true;
  }
  std::sort(result.begin(), result.end(),
             [](const auto& a, const auto& b) {
               if (a.second != b.second) return a.second > b.second;
               // Tie-break: prefer a play over a non-play at the same winrate.
               return is_play_perform(a.first) && !is_play_perform(b.first);
             });
  return result;
}

// --- optimize -----------------------------------------------------------

WinnableResult EndgameSolver::optimize(
    const Game& game, const std::pair<std::vector<GameArr>, std::vector<GameArr>>& arrs,
    const std::vector<ActionEntry>& actions_in, int player_turn,
    std::optional<double> deadline, int depth) {
  const auto& [undrawn, drawn] = arrs;
  int next_player = game.state.next_player_index(player_turn);

  // Sort by cached success rate.
  std::vector<ActionEntry> actions = actions_in;
  if (success_rate_.count(depth)) {
    std::sort(actions.begin(), actions.end(),
               [&](const ActionEntry& a, const ActionEntry& b) {
                 auto wa = cached_winrate(depth, a.first);
                 auto wb = cached_winrate(depth, b.first);
                 Fraction fa = wa.value_or(Fraction(-1));
                 Fraction fb = wb.value_or(Fraction(-1));
                 if (fa != fb) return fa > fb;
                 // Tie-break: a play comes before a non-play. Ensures the
                 // 100%-winrate early-exits surface a play first when one
                 // is available.
                 return is_play_perform(a.first) && !is_play_perform(b.first);
               });
  }

  std::vector<PerformAction> best_actions;
  Fraction best_winrate(0);

  for (const auto& [perform, winnable_draws] : actions) {
    if (past_deadline_local(deadline)) {
      if (!best_actions.empty()) {
        return WinnableResult{std::move(best_actions), best_winrate, ""};
      }
      return WinnableResult{{}, Fraction(0), "timeout"};
    }

    const auto& arr_list = is_clue(perform) ? undrawn : drawn;
    Fraction winrate(0);
    Fraction rem_prob(1);
    for (const auto& arr : arr_list) {
      if (past_deadline_local(deadline)) break;  // check inside arr loop too
      if (winrate + rem_prob < best_winrate) break;
      rem_prob = rem_prob - arr.prob;
      if (arr.drew && std::find(winnable_draws.begin(), winnable_draws.end(), *arr.drew) ==
                          winnable_draws.end() &&
          !winnable_draws.empty()) {
        continue;
      }
      Action game_action = perform_to_action(perform, game, player_turn);
      Game new_game = game.simulate_action(game_action, arr.drew);
      if (new_game.state.max_score() < game.state.max_score()) continue;
      auto res = winnable(new_game, next_player, arr.remaining, deadline, depth + 1);
      if (!res.ok()) continue;
      winrate = winrate + arr.prob * res.winrate;
    }

    cache_result(depth, perform, winrate);

    // Early-exit.
    int cards_left = game.state.cards_left;
    int bdr_count = 0;
    for (Identity id : game.state.all_ids) {
      if (game.state.trash_set.contains(id)) continue;
      if (!game.state.is_useful(id)) continue;
      if (!game.state.is_critical(id)) continue;
      if (id.rank == 5) continue;
      ++bdr_count;
    }
    if (winrate == Fraction(1) ||
        (bdr_count == 1 && cards_left > 1 &&
          winrate == Fraction(cards_left - 1, cards_left))) {
      return WinnableResult{{perform}, winrate, ""};
    }

    if (winrate > best_winrate) {
      best_actions = {perform};
      best_winrate = winrate;
    } else if (winrate > Fraction(0) && winrate == best_winrate) {
      best_actions.push_back(perform);
    }
  }

  if (best_actions.empty()) return WinnableResult{{}, Fraction(0), "no winning actions"};
  // Bubble any play to the front so callers that take actions.front() get
  // the play when the best winrate ties between a play and a non-play.
  std::stable_partition(best_actions.begin(), best_actions.end(),
                         [](const PerformAction& p) { return is_play_perform(p); });
  return WinnableResult{std::move(best_actions), best_winrate, ""};
}

// --- winnable -----------------------------------------------------------

WinnableResult EndgameSolver::winnable(const Game& game, int player_turn,
                                          const RemainingMap& remaining,
                                          std::optional<double> deadline, int depth) {
  const State& state = game.state;
  if (past_deadline_local(deadline)) return WinnableResult{{}, Fraction(0), "timeout"};
  // Safety: cap recursion depth. Without this cap a pathological branch
  // can outrun the deadline check between simulate_actions (each of which
  // can take 10-50ms due to the convention pipeline + elim() chain).
  if (depth > 20) return WinnableResult{{}, Fraction(0), "depth limit"};

  auto trivial = trivially_winnable(game, player_turn);
  if (trivial.found) {
    return WinnableResult{std::move(trivial.actions), trivial.winrate, ""};
  }

  // Viable-clueless check.
  bool viable_clueless = true;
  for (size_t suit_index = 0; suit_index < state.variant->suits.size() && viable_clueless;
        ++suit_index) {
    for (int rank = state.play_stacks[suit_index] + 1;
          rank <= state.max_ranks[suit_index] && viable_clueless; ++rank) {
      Identity id(static_cast<int>(suit_index), rank);
      bool found = false;
      for (const auto& hand : state.hands) {
        for (int o : hand) {
          if (game.common.thoughts[o].matches(id, /*infer=*/true)) {
            auto deck_id = state.deck[o].id();
            if (!deck_id || *deck_id == id) {
              found = true;
              break;
            }
          }
        }
        if (found) break;
      }
      if (!found) viable_clueless = false;
    }
  }

  if (viable_clueless) {
    State clueless_state = state;
    for (const auto& hand : state.hands) {
      for (int order : hand) {
        auto common_id = game.common.thoughts[order].id();
        if (common_id) {
          clueless_state.deck[order].suit_index = common_id->suit_index;
          clueless_state.deck[order].rank = common_id->rank;
        }
      }
    }
    auto win = clueless_winnable(std::move(clueless_state), player_turn, deadline, depth);
    if (win) {
      // Replace dummy clue with a real clue from find_all_clues.
      if (is_dummy_clue(*win)) {
        auto real_clues = game.find_all_clues(player_turn);
        if (!real_clues.empty()) {
          return WinnableResult{{real_clues.front()}, Fraction(1), ""};
        }
      }
      return WinnableResult{{*win}, Fraction(1), ""};
    }
  }

  bool bottom_decked = !remaining.empty();
  for (const auto& [ord, _] : remaining) {
    Identity id = Identity::from_ord(ord);
    if (!state.is_critical(id) || id.rank == 5) {
      bottom_decked = false;
      break;
    }
  }
  if (bottom_decked || unwinnable_state(state, player_turn, depth)) {
    return WinnableResult{{}, Fraction(0), "bottom-decked or unwinnable"};
  }

  auto performs = possible_actions(game, player_turn, remaining, deadline, depth);
  if (performs.empty()) return WinnableResult{{}, Fraction(0), "no actions"};

  // Direct win check.
  if (state.score() + 1 == state.max_score()) {
    for (const auto& [p, _] : performs) {
      if (std::holds_alternative<PerformPlay>(p)) {
        int target = std::get<PerformPlay>(p).target;
        auto deck_id = state.deck[target].id();
        if (deck_id && state.is_playable(*deck_id)) {
          return WinnableResult{{p}, Fraction(1), ""};
        }
      }
    }
  }

  // One-BDR-left special case.
  int suits_at_max = 0;
  for (size_t i = 0; i < state.variant->suits.size(); ++i) {
    if (state.play_stacks[i] == state.max_ranks[i]) ++suits_at_max;
  }
  bool one_bdr_left = state.score() == state.max_score() - 2 &&
                       suits_at_max == static_cast<int>(state.variant->suits.size()) - 1 &&
                       state.playable_set.length() > 0 &&
                       state.is_critical(state.playable_set.head());
  if (one_bdr_left) {
    Identity bdr_id = state.playable_set.head();
    bool unseen = visible_find(state, game.me(), bdr_id).empty();
    auto next_id = bdr_id.next();
    bool known_5 = false;
    if (next_id) {
      for (int o : visible_find(state, game.me(), *next_id)) {
        if (game.players[state.holder_of(o)].thoughts[o].matches(*next_id)) {
          known_5 = true;
          break;
        }
      }
    }
    bool navigable =
        state.clue_tokens + state.pace() > state.num_players && state.cards_left > 2;
    if (unseen && known_5 && navigable) {
      return WinnableResult{{performs.back().first},
                             Fraction(state.cards_left - 1, state.cards_left), ""};
    }
  }

  auto arrs = gen_arrs(game, remaining, false);
  return optimize(game, arrs, performs, player_turn, deadline, depth);
}

// --- solve ---------------------------------------------------------------

SolveResult EndgameSolver::solve(const Game& game,
                                   std::optional<PerformAction> only_action) {
  hanabi::instr::ScopedTimer st("endgame.solve");
  hanabi::logging::LogScope ls(
      "endgame.solve",
      {{"cards_left", game.state.cards_left}, {"clue_tokens", game.state.clue_tokens}});
  const State& state = game.state;

  // Trivial: one play wins.
  if (state.score() + 1 == state.max_score()) {
    for (int o : state.our_hand()) {
      auto id = game.me().thoughts[o].id(/*infer=*/true);
      if (id && state.is_playable(*id)) {
        return SolveResult{PerformPlay{o}, Fraction(1), ""};
      }
    }
  }

  double start = monotonic_seconds();
  double deadline = start + timeout;

  // Compute remaining_ids + own_ids.
  std::map<int, int> seen_ids;
  std::vector<std::pair<int, std::optional<Identity>>> own_ids;
  for (int i = 0; i < state.num_players; ++i) {
    for (int order : state.hands[i]) {
      auto id = game.me().thoughts[order].id();
      if (id) {
        ++seen_ids[id->to_ord()];
        if (i == state.our_player_index) own_ids.insert(own_ids.begin(), {order, *id});
      } else if (i == state.our_player_index) {
        own_ids.insert(own_ids.begin(), {order, std::nullopt});
      }
    }
  }
  RemainingMap remaining_ids;
  for (Identity id : state.variant->all_ids()) {
    int missing = state.card_count[id.to_ord()] - state.base_count[id.to_ord()] -
                   seen_ids[id.to_ord()];
    if (missing > 0) remaining_ids[id.to_ord()] = missing;
  }

  int useful_unseen = 0;
  for (const auto& [ord, v] : remaining_ids) {
    Identity id = Identity::from_ord(ord);
    if (state.is_useful(id) && v == state.card_count[id.to_ord()]) ++useful_unseen;
  }
  if (useful_unseen > 3) {
    return SolveResult{PerformPlay{0}, Fraction(0), "too many missing useful identities"};
  }

  // Pre-populate assumed_game.
  Game assumed_game = game;
  for (const auto& [order, id] : own_ids) {
    if (id) assumed_game.with_id(order, *id);
  }

  auto linked_set = std::vector<int>{};
  {
    auto linked = game.me().linked_orders(state);
    linked_set = std::move(linked);
  }
  std::vector<int> unknown_own;
  for (const auto& [order, id] : own_ids) {
    if (!id) unknown_own.push_back(order);
  }

  int total_unknown = state.cards_left + static_cast<int>(unknown_own.size());
  if (total_unknown == 0) {
    auto result = winnable(assumed_game, state.our_player_index, remaining_ids, deadline);
    if (!result.ok()) return SolveResult{PerformPlay{0}, Fraction(0), "no winning strategy"};
    return SolveResult{result.actions.front(), result.winrate, ""};
  }

  // --- Arrangement generation ---

  auto impossible_arr = [&](const std::vector<Identity>& ids, Identity id, int order,
                              bool try_filter) {
    const Thought& thought = game.me().thoughts[order];
    auto deck_id = state.deck[order].id();
    if (deck_id && *deck_id != id) return true;
    if (!thought.possible.contains(id)) return true;
    if (try_filter && !game.valid_arr(id, order)) return true;
    if (state.is_basic_trash(id) &&
        std::find(linked_set.begin(), linked_set.end(), order) != linked_set.end()) {
      for (const Link& link : game.me().links) {
        const auto& orders = link_orders(link);
        auto promise = link_promise(link);
        if (promise && state.is_useful(*promise) &&
            std::find(orders.begin(), orders.end(), order) != orders.end()) {
          bool all_trash = true;
          for (int o : orders) {
            if (o == order) continue;
            auto it = std::find(unknown_own.begin(), unknown_own.end(), o);
            if (it == unknown_own.end()) {
              all_trash = false;
              break;
            }
            int idx = static_cast<int>(it - unknown_own.begin());
            if (idx >= static_cast<int>(ids.size()) || !state.is_basic_trash(ids[idx])) {
              all_trash = false;
              break;
            }
          }
          if (all_trash) return true;
        }
      }
    }
    return false;
  };

  auto expand_arr = [&](const Arrangement& arrangement, bool try_filter) {
    std::vector<Arrangement> result;
    if (past_deadline_local(deadline)) {
      result.push_back(arrangement);
      return result;
    }
    int total_cards = remaining_total(arrangement.remaining);
    if (total_cards == 0) return result;
    int order_for_next = unknown_own[arrangement.ids.size()];
    for (const auto& [ord, missing] : arrangement.remaining) {
      Identity id = Identity::from_ord(ord);
      if (impossible_arr(arrangement.ids, id, order_for_next, try_filter)) continue;
      RemainingMap new_remaining = remaining_remove(arrangement.remaining, id);
      std::vector<Identity> new_ids = arrangement.ids;
      new_ids.push_back(id);
      Fraction new_prob = arrangement.prob * Fraction(missing, total_cards);
      result.push_back(Arrangement{std::move(new_ids), new_prob, std::move(new_remaining)});
    }
    return result;
  };

  Arrangement initial_arr{{}, Fraction(1), remaining_ids};
  std::vector<Arrangement> arrs_iter = {initial_arr};
  for (size_t i = 0; i < unknown_own.size(); ++i) {
    std::vector<Arrangement> next_arrs;
    for (const auto& arr : arrs_iter) {
      auto expanded = expand_arr(arr, /*try_filter=*/true);
      for (auto& a : expanded) next_arrs.push_back(std::move(a));
    }
    arrs_iter = std::move(next_arrs);
  }
  if (arrs_iter.empty()) {
    arrs_iter = {initial_arr};
    for (size_t i = 0; i < unknown_own.size(); ++i) {
      std::vector<Arrangement> next_arrs;
      for (const auto& arr : arrs_iter) {
        auto expanded = expand_arr(arr, /*try_filter=*/false);
        for (auto& a : expanded) next_arrs.push_back(std::move(a));
      }
      arrs_iter = std::move(next_arrs);
    }
  }

  if (past_deadline_local(deadline)) {
    return SolveResult{PerformPlay{0}, Fraction(0), "timeout"};
  }

  // Monte Carlo grouping.
  std::vector<Arrangement> arrs;
  if (monte_carlo) {
    Fraction sum_prob(0);
    std::vector<std::pair<std::string, Arrangement>> grouped;
    for (const auto& arr : arrs_iter) {
      std::string key;
      for (Identity id : arr.ids) {
        if (!key.empty()) key += ",";
        key += state.is_basic_trash(id) ? "_" : state.log_id(id);
      }
      bool found_existing = false;
      for (auto& [k, existing] : grouped) {
        if (k == key) {
          existing.prob = existing.prob + arr.prob;
          found_existing = true;
          break;
        }
      }
      if (!found_existing) grouped.emplace_back(key, arr);
      sum_prob = sum_prob + arr.prob;
    }
    if (sum_prob > Fraction(0)) {
      for (auto& [_, a] : grouped) {
        a.prob = a.prob / sum_prob;
        arrs.push_back(std::move(a));
      }
    } else {
      for (auto& [_, a] : grouped) arrs.push_back(std::move(a));
    }
  } else {
    arrs = std::move(arrs_iter);
  }

  std::sort(arrs.begin(), arrs.end(),
             [](const Arrangement& a, const Arrangement& b) { return a.prob > b.prob; });
  if (arrs.empty()) arrs = {{{}, Fraction(1), remaining_ids}};

  // Build hypos.
  struct Hypo {
    Game game;
    std::vector<ActionEntry> actions;
    std::pair<std::vector<GameArr>, std::vector<GameArr>> game_arrs;
    Fraction prob;
  };
  std::vector<Hypo> hypos;
  for (const auto& arr : arrs) {
    if (past_deadline_local(deadline)) break;
    Game hypo_game = assumed_game;
    for (size_t i = 0; i < arr.ids.size(); ++i) {
      hypo_game.with_id(unknown_own[i], arr.ids[i]);
    }
    auto actions = possible_actions(hypo_game, state.our_player_index, arr.remaining,
                                       deadline, 0);
    if (actions.empty()) {
      actions = possible_actions(hypo_game, state.our_player_index, arr.remaining,
                                    deadline, 0, /*infer=*/true);
    }
    bool clue_only = !actions.empty();
    for (const auto& [p, _] : actions) {
      if (!hanabi::is_clue(p)) {
        clue_only = false;
        break;
      }
    }
    auto game_arrs = gen_arrs(hypo_game, arr.remaining, clue_only);
    hypos.push_back({std::move(hypo_game), std::move(actions), std::move(game_arrs), arr.prob});
  }

  if (only_action) {
    Fraction winrate(0);
    for (const auto& h : hypos) {
      std::optional<ActionEntry> match;
      for (const auto& a : h.actions) {
        if (perform_eq(a.first, *only_action)) {
          match = a;
          break;
        }
      }
      if (!match) continue;
      const auto& arr_list = hanabi::is_clue(match->first) ? h.game_arrs.first : h.game_arrs.second;
      winrate = winrate + h.prob *
                            action_winrate(h.game, arr_list, *match, state.our_player_index,
                                             deadline);
    }
    return SolveResult{*only_action, winrate, ""};
  }

  std::vector<PerformAction> all_actions;
  for (const auto& h : hypos) {
    for (const auto& [perform, _] : h.actions) {
      bool found = false;
      for (const auto& a : all_actions) {
        if (perform_eq(a, perform)) {
          found = true;
          break;
        }
      }
      if (!found) all_actions.push_back(perform);
    }
  }

  if (hypos.empty()) return SolveResult{PerformPlay{0}, Fraction(0), "no hypotheses"};
  const auto& first = hypos.front();
  // With a single hypo, optimize_full can short-circuit once it sees a
  // winrate-1 action — the iterative cross-hypo refinement below is a no-op.
  bool single_hypo = hypos.size() == 1;
  auto initial = optimize_full(first.game, first.game_arrs, first.actions,
                                  state.our_player_index, deadline, single_hypo);
  for (auto& [_, w] : initial) w = w * first.prob;

  // Append missing actions.
  for (const auto& a : all_actions) {
    bool found = false;
    for (const auto& [p, _] : initial) {
      if (perform_eq(p, a)) {
        found = true;
        break;
      }
    }
    if (!found) initial.emplace_back(a, Fraction(0));
  }

  if (initial.empty()) {
    return SolveResult{PerformPlay{0}, Fraction(0), "no winning actions"};
  }

  if (hypos.size() > 1) {
    // Per the user's directive: evaluate plays first so the multi-hypo
    // early-exit on a 100% candidate fires on a play if any play wins
    // 100%. With pure winrate-desc ordering, a non-play that wins 100%
    // in the first hypo would short-circuit the loop before any play is
    // tried even if that play also wins 100% across all hypos.
    std::stable_partition(initial.begin(), initial.end(),
                            [](const auto& kv) {
                              return is_play_perform(kv.first);
                            });
    auto best = initial.front();
    for (auto& [action, winrate] : initial) {
      if (past_deadline_local(deadline)) break;
      Fraction cur = winrate;
      Fraction rem_prob = Fraction(1) - winrate;
      for (size_t i = 1; i < hypos.size(); ++i) {
        if (cur + rem_prob < best.second) break;
        const auto& h = hypos[i];
        std::optional<ActionEntry> match;
        for (const auto& a : h.actions) {
          if (perform_eq(a.first, action)) {
            match = a;
            break;
          }
        }
        if (!match) {
          rem_prob = rem_prob - h.prob;
          continue;
        }
        const auto& arr_list = hanabi::is_clue(match->first) ? h.game_arrs.first
                                                                : h.game_arrs.second;
        Fraction hypo_wr = h.prob *
                            action_winrate(h.game, arr_list, *match, state.our_player_index,
                                              deadline);
        cur = cur + hypo_wr;
        rem_prob = rem_prob - h.prob;
      }
      if (cur == Fraction(1)) {
        best = {action, cur};
        break;
      }
      bool better = cur > best.second ||
                    (cur == best.second && is_play_perform(action) &&
                      !is_play_perform(best.first));
      if (better) best = {action, cur};
    }
    if (best.second == Fraction(0)) {
      return SolveResult{best.first, Fraction(0), "no winning actions"};
    }
    return SolveResult{best.first, best.second, ""};
  }

  auto [best_action, best_winrate] = initial.front();
  return SolveResult{best_action, best_winrate, ""};
}

}  // namespace hanabi::endgame
