#include "hanabi/endgame/winnable.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/instrumentation/timer.h"

namespace hanabi::endgame {

namespace {

double monotonic_seconds() {
  using clock = std::chrono::steady_clock;
  return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

}  // namespace

bool past_deadline(std::optional<double> deadline) {
  return deadline && monotonic_seconds() > *deadline;
}

bool is_dummy_clue(const PerformAction& perform) {
  if (!std::holds_alternative<PerformRank>(perform)) return false;
  const auto& pr = std::get<PerformRank>(perform);
  return pr.target == 0 && pr.value == 0;
}

// --- advance_state -------------------------------------------------------

namespace {

State remove_and_draw(State s, int player_index, int order,
                       const std::optional<Card>& draw) {
  int new_order = s.next_card_order;
  auto& hand = s.hands[player_index];
  std::vector<int> new_hand;
  new_hand.reserve(hand.size());
  new_hand.push_back(new_order);
  for (int o : hand) {
    if (o != order) new_hand.push_back(o);
  }
  s.hands[player_index] = std::move(new_hand);

  int new_cards_left = std::max(0, s.cards_left - 1);
  int new_next_order = new_order + (s.cards_left > 0 ? 1 : 0);
  std::optional<int> new_endgame_turns;
  if (s.endgame_turns) {
    new_endgame_turns = *s.endgame_turns - 1;
  } else if (s.cards_left == 1) {
    new_endgame_turns = s.num_players;
  }
  s.cards_left = new_cards_left;
  s.next_card_order = new_next_order;
  s.endgame_turns = new_endgame_turns;

  if (new_order < static_cast<int>(s.deck.size()) && s.deck[new_order].id()) {
    return s;
  }
  Card new_card;
  if (draw) {
    new_card = *draw;
  } else {
    new_card.suit_index = -1;
    new_card.rank = -1;
    new_card.order = new_order;
    new_card.turn_drawn = s.turn_count;
  }
  if (static_cast<int>(s.deck.size()) == new_order) {
    s.deck.push_back(std::move(new_card));
  } else {
    s.deck[new_order] = std::move(new_card);
  }
  return s;
}

}  // namespace

State advance_state(State state, const PerformAction& perform, int player_index,
                     const std::optional<Card>& draw) {
  if (std::holds_alternative<PerformPlay>(perform)) {
    int target = std::get<PerformPlay>(perform).target;
    auto played_id = state.deck[target].id();
    State after;
    if (!played_id) {
      after = state;
      ++after.strikes;
    } else if (state.is_playable(*played_id)) {
      after = state.with_play(*played_id);
    } else {
      after = state;
      ++after.strikes;
      after = after.with_discard(*played_id, target);
    }
    return remove_and_draw(std::move(after), player_index, target, draw);
  }
  if (std::holds_alternative<PerformDiscard>(perform)) {
    int target = std::get<PerformDiscard>(perform).target;
    auto d_id = state.deck[target].id();
    State after = state;
    if (d_id) after = after.with_discard(*d_id, target);
    after = after.regain_clue();
    return remove_and_draw(std::move(after), player_index, target, draw);
  }
  // Clue (real or dummy).
  State after = state;
  --after.clue_tokens;
  if (after.endgame_turns) --(*after.endgame_turns);
  return after;
}

// --- advance_game --------------------------------------------------------

namespace {

// Convert PerformAction to Action for game.simulate_action.
Action perform_to_action(const PerformAction& perform, const Game& game,
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

}  // namespace

Game advance_game(const Game& game, const PerformAction& perform, int player_index,
                   std::optional<Identity> draw) {
  if (is_dummy_clue(perform)) {
    Game out = game;
    out.state = advance_state(game.state, perform, player_index, std::nullopt);
    return out;
  }
  Action action = perform_to_action(perform, game, player_index);
  return game.simulate_action(action, draw);
}

// --- clueless_winnable ---------------------------------------------------

std::optional<PerformAction> clueless_winnable(State state, int player_turn,
                                                  std::optional<double> deadline, int depth) {
  if (state.score() == state.max_score()) return PerformPlay{99};
  if (past_deadline(deadline)) return std::nullopt;
  if (unwinnable_state(state, player_turn, depth)) return std::nullopt;

  auto action_winnable = [&](const PerformAction& perform) {
    State new_state = advance_state(state, perform, player_turn, std::nullopt);
    return clueless_winnable(std::move(new_state), state.next_player_index(player_turn),
                               deadline, depth + 1)
        .has_value();
  };

  for (int order : state.hands[player_turn]) {
    auto id = state.deck[order].id();
    if (id && state.is_playable(*id)) {
      PerformAction perform = PerformPlay{order};
      if (action_winnable(perform)) return perform;
    }
  }

  if (state.can_clue()) {
    PerformAction default_clue = PerformRank{0, 0};
    if (action_winnable(default_clue)) return default_clue;
  }

  for (int order : state.hands[player_turn]) {
    if (!state.deck[order].id()) {
      PerformAction perform = PerformDiscard{order};
      if (action_winnable(perform)) return perform;
    }
  }

  return std::nullopt;
}

// --- winnable_simpler / winnable_if --------------------------------------

namespace {

std::vector<int> player_known_plays(const Game& game, int player_turn) {
  const State& state = game.state;
  std::vector<int> plays;
  std::vector<bool> seen(state.deck.size(), false);
  for (int o : game.players[player_turn].thinks_playables(game, player_turn,
                                                              /*exclude_trash=*/true)) {
    if (o < static_cast<int>(seen.size()) && !seen[o]) {
      plays.push_back(o);
      seen[o] = true;
    }
  }
  for (auto it = state.hands[player_turn].rbegin(); it != state.hands[player_turn].rend();
        ++it) {
    int o = *it;
    if (o < static_cast<int>(seen.size()) && seen[o]) continue;
    auto inferred = game.players[player_turn].thoughts[o].id(/*infer=*/true);
    if (inferred && state.is_playable(*inferred)) {
      plays.push_back(o);
      if (o < static_cast<int>(seen.size())) seen[o] = true;
    }
  }
  // Reactive play+play newest unclued candidate.
  // Reactor data lives on Game directly (see Phase 2 design notes).
  // waiting is std::vector<ReactorWC>; treat as "active" if non-empty.
  if (!game.waiting.empty()) {
    const auto& wc = game.waiting.front();
    if (wc.reacter == player_turn && !wc.inverted &&
        wc.clue.kind == ClueKind::RANK) {
      for (int o : state.hands[player_turn]) {
        if (o < static_cast<int>(seen.size()) && seen[o]) continue;
        if (state.deck[o].clued) continue;
        plays.push_back(o);
        if (o < static_cast<int>(seen.size())) seen[o] = true;
        break;
      }
    }
  }
  return plays;
}

}  // namespace

bool winnable_simpler(const Game& game, int player_turn, const RemainingMap& remaining,
                       std::optional<double> deadline, int depth) {
  hanabi::instr::ScopedTimer st("endgame.winnable_simpler");
  const State& state = game.state;
  if (state.score() == state.max_score()) return true;
  if (unwinnable_state(state, player_turn, depth)) return false;

  std::vector<PerformAction> plays;
  std::vector<PerformAction> discards;
  bool found_dc = false;

  if (state.can_clue()) {
    discards.emplace_back(PerformRank{0, 0});
    found_dc = true;
  }

  for (int order : player_known_plays(game, player_turn)) {
    plays.insert(plays.begin(), PerformPlay{order});
  }

  for (auto it = state.hands[player_turn].rbegin(); it != state.hands[player_turn].rend();
        ++it) {
    if (found_dc) break;
    auto inferred = game.players[player_turn].thoughts[*it].id(/*infer=*/true);
    if (!inferred || state.is_basic_trash(*inferred)) {
      discards.insert(discards.begin(), PerformDiscard{*it});
      found_dc = true;
    }
  }

  std::vector<PerformAction> actions = plays;
  for (auto& d : discards) actions.push_back(d);

  for (const auto& action : actions) {
    auto res = winnable_if(game, player_turn, action, remaining, deadline, depth);
    if (std::holds_alternative<SimpleResult>(res)) {
      if (std::get<SimpleResult>(res) == SimpleResult::UNWINNABLE) continue;
      return true;
    }
    return true;
  }
  return false;
}

SimpleResultT winnable_if(const Game& game, int player_turn,
                            const PerformAction& perform, const RemainingMap& remaining,
                            std::optional<double> deadline, int depth) {
  if (past_deadline(deadline)) return SimpleResult::UNWINNABLE;
  const State& state = game.state;
  bool is_clue = std::holds_alternative<PerformColour>(perform) ||
                  std::holds_alternative<PerformRank>(perform);

  if (state.cards_left == 0 || is_clue) {
    Game new_game = advance_game(game, perform, player_turn, std::nullopt);
    if (winnable_simpler(new_game, state.next_player_index(player_turn), remaining,
                          deadline, depth + 1)) {
      return SimpleResult::ALWAYS_WINNABLE;
    }
    return SimpleResult::UNWINNABLE;
  }

  std::vector<Identity> trash_ids;
  std::vector<Identity> other_ids;
  for (const auto& [ord, _] : remaining) {
    Identity id = Identity::from_ord(ord);
    if (state.is_basic_trash(id)) trash_ids.push_back(id);
    else other_ids.push_back(id);
  }

  auto is_winnable = [&](Identity draw_id) {
    Game new_game = advance_game(game, perform, player_turn, draw_id);
    RemainingMap new_remaining = remaining_remove(remaining, draw_id);
    return winnable_simpler(new_game, state.next_player_index(player_turn), new_remaining,
                              deadline, depth + 1);
  };

  bool trash_winnable = trash_ids.empty() || is_winnable(trash_ids.front());
  std::vector<Identity> other_winnable;
  for (Identity i : other_ids) {
    if (is_winnable(i)) other_winnable.push_back(i);
  }

  if (!trash_winnable && other_winnable.empty()) return SimpleResult::UNWINNABLE;
  if (trash_winnable) {
    std::vector<Identity> draws = trash_ids;
    for (Identity i : other_winnable) draws.push_back(i);
    return WinnableWithDraws{std::move(draws)};
  }
  return WinnableWithDraws{std::move(other_winnable)};
}

}  // namespace hanabi::endgame
