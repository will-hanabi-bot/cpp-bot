#include "hanabi/conventions/variants/inverted.h"

#include <algorithm>

#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor/interpret_clue.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "hanabi/conventions/reactor/interpret_reactive.h"

namespace hanabi::reactor::variants {

namespace {

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

}  // namespace

bool is_inverted_id(const State& state, Identity id) {
  return state.variant->suits[id.suit_index].suit_type.inverted;
}

bool target_is_inverted(const State& state, int target_order) {
  int suit_index = state.deck[target_order].suit_index;
  if (suit_index < 0) return false;
  return state.variant->suits[suit_index].suit_type.inverted;
}

bool would_lose_inverted_reacter(const State& state, int react_order,
                                 bool receiver_target_inverted,
                                 bool standard_is_target_play) {
  int suit_index = state.deck[react_order].suit_index;
  if (suit_index < 0) return false;
  if (!state.variant->suits[suit_index].suit_type.inverted) return false;
  // The receiver-orange swap toggles play↔discard. After the swap, we end
  // up calling target_play iff (standard_is_target_play XOR
  // receiver_target_inverted) is false.
  const bool final_is_target_play = standard_is_target_play
                                        ? !receiver_target_inverted
                                        : receiver_target_inverted;
  if (final_is_target_play) return true;
  // target_discard on an orange reacter card: physical discard maps to a
  // play attempt under the game-rule inversion. Safe only when the orange
  // is currently playable on the orange stack — otherwise it is a misplay
  // strike, so reject.
  auto id = state.deck[react_order].id();
  if (!id) return false;
  return !state.is_playable(*id);
}

CardStatus called_focus_status(const State& state,
                               const IdentitySet& new_inferred) {
  for (Identity i : new_inferred) {
    if (state.variant->suits[i.suit_index].suit_type.inverted) {
      return CardStatus::CALLED_TO_DISCARD;
    }
  }
  return CardStatus::CALLED_TO_PLAY;
}

DiscardAction make_discard_for_simulation(const State& state, int player_index,
                                          int order) {
  auto id = state.deck[order].id();
  if (!id) return DiscardAction{player_index, order, -1, -1, /*failed=*/false};
  bool inverted = state.variant->suits[id->suit_index].suit_type.inverted;
  bool playable = state.is_playable(*id);
  bool failed = inverted && !playable;
  return DiscardAction{player_index, order, id->suit_index, id->rank, failed};
}

bool discard_advances_stack(const State& state,
                            const std::optional<Identity>& id) {
  return id && is_inverted_id(state, *id) && state.is_playable(*id);
}

bool possible_has_inverted(const State& state, const IdentitySet& possible) {
  for (Identity i : possible) {
    if (state.variant->suits[i.suit_index].suit_type.inverted) return true;
  }
  return false;
}

std::optional<ClueInterp> orange_chop_save(
    const Game& prev, Game& game, const ClueAction& action, int focus_slot,
    int reacter, const std::vector<std::pair<int, Identity>>& possible_conns) {
  const State& state = game.state;
  int receiver = action.target;
  std::optional<int> receiver_chop;
  for (int o : state.hands[receiver]) {
    if (!state.deck[o].clued && game.meta[o].status == CardStatus::NONE) {
      receiver_chop = o;
      break;
    }
  }
  if (!receiver_chop) return std::nullopt;
  auto chop_id = state.deck[*receiver_chop].id();
  if (!chop_id || !is_inverted_id(state, *chop_id)) {
    // Only the orange (inverted) case is encoded as a chop-save here.
    // Non-orange chops would require a target_discard on the reacter,
    // which is unsafe without a critical-check from the giver's POV
    // that the observer can't perform on their own card. Bail.
    return std::nullopt;
  }
  int chop_index = -1;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    if (state.hands[receiver][i] == *receiver_chop) {
      chop_index = static_cast<int>(i);
      break;
    }
  }
  if (chop_index < 0) return std::nullopt;
  int hand_size = kHandSize[state.num_players];
  int chop_target_slot = chop_index + 1;
  int chop_react_slot = calc_slot(focus_slot, chop_target_slot, hand_size);
  if (chop_react_slot < 1 ||
      chop_react_slot > static_cast<int>(state.hands[reacter].size())) {
    return std::nullopt;
  }
  int chop_react_order = state.hands[reacter][chop_react_slot - 1];
  auto chop_prev_plays = prev.common.obvious_playables(prev, reacter);
  if (contains(chop_prev_plays, chop_react_order)) return std::nullopt;
  IdentitySet chop_effective = effective_possible_for(game, chop_react_order);
  bool chop_has_playable = chop_effective.exists([&](Identity i) {
    if (state.playable_set.contains(i)) return true;
    for (const auto& [_, c] : possible_conns) {
      if (c == i) return true;
    }
    return false;
  });
  if (!chop_has_playable) return std::nullopt;
  // Don't choose a reacter whose own physical play would lose an
  // inverted card (orange PerformPlay = discard pile).
  if (would_lose_inverted_reacter(state, chop_react_order,
                                  /*receiver_target_inverted=*/true,
                                  /*standard_is_target_play=*/false)) {
    return std::nullopt;
  }
  auto chop_interp =
      target_play(game, action, chop_react_order, /*urgent=*/true,
                  /*stable=*/false);
  if (!chop_interp) return std::nullopt;
  return ClueInterp::REACTIVE;
}

}  // namespace hanabi::reactor::variants
