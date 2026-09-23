// Shared helpers for the Throw It in a Hole test suite.
//
// Deliberately a NEW header rather than a parameter on
// `tests/test_reactor0/test_reactor0_helpers.h`: that one sets
// `Convention::REACTOR0` through `SetupOptions::init`, and the two conventions
// must not share a switch nobody re-reads.
#pragma once

#include <string>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/convention.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"

namespace hanabi::test::tiiah {

// Opt a SetupOptions into the TIIAH convention. The variant still has to be a
// TIIAH one — the ENGINE rules key on `Variant::throw_it_in_a_hole`, not on the
// convention, because a 4+ player TIIAH table runs reactor and needs them too.
inline void use_tiiah(SetupOptions& opts) {
  opts.init = [](Game& g) { g.convention = Convention::TIIAH; };
}

inline int order_at(const Game& g, TestPlayer player, int slot) {
  return g.state.hands[static_cast<int>(player)][slot - 1];
}

inline CardStatus status_at(const Game& g, TestPlayer player, int slot) {
  return g.meta[order_at(g, player, slot)].status;
}

// Press a button on `slot` WITHOUT telling anyone what the card was — the shape
// every play arrives in under this variant. `take_turn` cannot express it: it
// builds actions from an identity string, and the whole point here is that the
// wire carries none.
//
// `reached_the_hole` picks which outcome the server reported: true for the hole
// (hidden), false for the discard pile (visible, but we still exercise the
// identity-free wire form). `draw` is the card the actor draws, as a short
// identity, or "" for our own seat.
inline Game hidden_action(Game game, TestPlayer actor, int slot,
                          bool reached_the_hole, const std::string& draw = "") {
  const int pi = static_cast<int>(actor);
  const int order = order_at(game, actor, slot);
  const int next_order = game.state.next_card_order;
  game.catchup = true;
  if (reached_the_hole) {
    game.handle_action(PlayAction{pi, order, -1, -1});
  } else {
    game.handle_action(DiscardAction{pi, order, -1, -1, /*failed=*/false});
  }
  if (!draw.empty()) {
    Identity id = game.state.expand_short(draw);
    game.handle_action(DrawAction{pi, next_order, id.suit_index, id.rank});
  } else if (pi == game.state.our_player_index) {
    game.handle_action(DrawAction{pi, next_order, -1, -1});
  }
  game.handle_action(
      TurnAction{game.state.turn_count, game.state.next_player_index(pi)});
  game.catchup = false;
  return game;
}

}  // namespace hanabi::test::tiiah
