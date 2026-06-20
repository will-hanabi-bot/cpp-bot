// Shared helper for the hanab.live replay tests. Replays apply a sequence of
// game actions captured from a real hanab.live export, with order/player
// remappings to fit the test harness's convention of our_player_index=0.
#pragma once

#include <array>
#include <utility>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/variant.h"

namespace hanabi::test::replay {

// One server-side action: hanab.live export format.
//   type: 0=Play, 1=Discard, 2=Color clue, 3=Rank clue
//   target: card order (for play/discard) or player index (for clue)
//   value: clue value (color index or rank); 0 for play/discard
struct OrigAction {
  int type;
  int target;
  int value;
};

// All the per-replay mappings + the deck.
struct ReplayContext {
  std::vector<std::pair<int, int>> deck;             // (suit, rank) by orig order
  std::vector<int> orig_to_my_order;
  std::vector<int> orig_to_my_player;
  std::vector<std::pair<int, int>> my_order_to_id;   // (suit, rank) by my order
};

// Compute touched orders using ground-truth identities (needed because the
// observer's own cards appear as (-1, -1) in state.deck so state.clue_touched
// won't pick them up).
inline std::vector<int> touched_orders(const Variant& variant,
                                          const std::vector<int>& hand_orders,
                                          const ReplayContext& ctx,
                                          ClueKind kind, int value) {
  std::vector<int> out;
  for (int my_order : hand_orders) {
    auto [suit, rank] = ctx.my_order_to_id[my_order];
    if (variant.id_touched(Identity(suit, rank), kind, value)) {
      out.push_back(my_order);
    }
  }
  return out;
}

// Apply one replay action to the game (in catchup mode), then issue the
// next-card draw + advance to the next player's turn.
inline void apply_orig_action(Game& g, const OrigAction& action,
                                 const ReplayContext& ctx) {
  int pi = g.state.current_player_index;
  g.catchup = true;

  if (action.type == 0 || action.type == 1) {
    int orig_order = action.target;
    int my_order = ctx.orig_to_my_order[orig_order];
    auto [suit, rank] = ctx.deck[orig_order];
    const bool inverted = g.state.variant->suits[suit].suit_type.inverted;
    // Use the State's variant-aware playable check (handles `reversed`
    // suits, where play_stacks starts at 6 and counts down 5->1).
    const bool playable = g.state.is_playable(Identity(suit, rank));
    // Hanab.live's export is OUTCOME-oriented: type=0 = card landed on a
    // play stack, type=1 = card landed in the discard pile. We build the
    // outcome-shaped action, then `orient_action_for_engine` flips it
    // into the engine's button-oriented shape for inverted (Orange /
    // Dark Orange) suits (the engine's `on_play`/`on_discard` invert the
    // effect for inverted-suit cards, so we have to compensate). The
    // failed flag for type=1 follows: misplay = card was non-playable AND
    // the action's button caused a misplay outcome — i.e., DISCARD on a
    // non-playable inverted-suit card (PLAY on non-playable inverted
    // wouldn't strike, it'd just go to the discard pile). For type=1 +
    // non-inverted + non-playable we conservatively treat as a clean
    // discard (the common case — chop-discarding a trash card).
    Action act;
    if (action.type == 0) {
      if (!playable && !inverted) {
        // Non-inverted Play of a non-playable card: misplay. (Inverted
        // type=0 always implies playable, so this branch only catches
        // non-inverted edge cases.)
        act = DiscardAction{pi, my_order, suit, rank, /*failed=*/true};
      } else {
        act = PlayAction{pi, my_order, suit, rank};
      }
    } else {
      act = DiscardAction{pi, my_order, suit, rank,
                            /*failed=*/inverted && !playable};
    }
    act = orient_action_for_engine(std::move(act), *g.state.variant);
    g.handle_action(act);
    int new_my_order = g.state.next_card_order;
    if (new_my_order < static_cast<int>(ctx.deck.size())) {
      auto [d_suit, d_rank] = ctx.deck[new_my_order];
      if (pi == g.state.our_player_index) {
        g.handle_action(DrawAction{pi, new_my_order, -1, -1});
      } else {
        g.handle_action(DrawAction{pi, new_my_order, d_suit, d_rank});
      }
    }
  } else if (action.type == 2 || action.type == 3) {
    ClueKind kind = action.type == 2 ? ClueKind::COLOUR : ClueKind::RANK;
    int target = ctx.orig_to_my_player[action.target];
    int value = action.value;
    auto touched = touched_orders(*g.state.variant, g.state.hands[target], ctx,
                                     kind, value);
    g.handle_action(ClueAction{pi, target, std::move(touched), BaseClue(kind, value)});
  } else {
    throw std::invalid_argument("Unknown action type");
  }

  g.handle_action(TurnAction{g.state.turn_count, g.state.next_player_index(pi)});
  g.catchup = false;
}

}  // namespace hanabi::test::replay
