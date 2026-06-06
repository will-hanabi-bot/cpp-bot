// Hanab.live replay 1882573 turn 18. At turn 18 (action index 18) P0
// (will-bot67) gives a rank-5 clue to P1 (yagami_black). From P2
// (will-bot69)'s POV, this should NOT mark the bot's own slot-5 null-5
// (my-order 4) as CALLED_TO_PLAY only to immediately reset to NONE —
// the user saw "turn 18: [f] u5 | turn 18: [reset]" in the shared replay's
// note for that card.
//
// This test reproduces the turn 18 gameAction stream and traces order 4's
// CardStatus across the discrete handle_action calls. We expect EITHER
// the status to stay NONE throughout (no spurious mark) OR to settle on
// CALLED_TO_PLAY and stay there — never a 0→2→0 oscillation inside the
// same turn.
#include <gtest/gtest.h>

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/net/notes.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// hanab.live export deck for game 1882573.
// Suits: Red(0), Blue(1), Null(2).
const std::vector<std::pair<int, int>> kDeck = {
    {2, 2}, {0, 4}, {2, 1}, {0, 3}, {1, 2},
    {2, 3}, {1, 4}, {0, 1}, {0, 1}, {2, 2},
    {1, 1}, {0, 3}, {0, 2}, {2, 1}, {2, 5},
    {1, 3}, {0, 4}, {0, 1}, {0, 5}, {2, 4},
    {2, 3}, {1, 3}, {1, 1}, {0, 2}, {2, 4},
    {1, 5}, {1, 2}, {2, 1}, {1, 4}, {1, 1},
};

const std::vector<OrigAction> kOrigActions = {
    {3, 2, 4},  // 0:  P0 → P2 rank-4
    {0, 8, 0},  // 1:  P1 plays order 8 (r1)
    {3, 1, 5},  // 2:  P2 → P1 rank-5
    {0, 2, 0},  // 3:  P0 plays order 2 (u1)
    {0, 9, 0},  // 4:  P1 plays order 9 (u2)
    {0, 13, 0}, // 5:  P2 plays order 13 (u1 — misplay/strike)
    {3, 2, 3},  // 6:  P0 → P2 rank-3
    {0, 5, 0},  // 7:  P1 plays order 5 (u3)
    {0, 10, 0}, // 8:  P2 plays order 10 (b1)
    {2, 2, 0},  // 9:  P0 → P2 colour red
    {1, 7, 0},  // 10: P1 discards order 7
    {3, 1, 4},  // 11: P2 → P1 rank-4
    {0, 4, 0},  // 12: P0 plays order 4 (b2)
    {0, 19, 0}, // 13: P1 plays order 19 (u4)
    {0, 12, 0}, // 14: P2 plays order 12 (r2)
    {3, 2, 3},  // 15: P0 → P2 rank-3
    {0, 15, 0}, // 16: P1 plays order 15 (b3)
    {0, 11, 0}, // 17: P2 plays order 11 (r3)
    {3, 1, 5},  // 18: P0 → P1 rank-5  ← THE CLUE THAT CAUSED THE BUG
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Player mapping: orig P0 (will-bot67) → my Bob (P1),
  //                 orig P1 (yagami_black) → my Cathy (P2),
  //                 orig P2 (will-bot69) → my Alice (P0, observer).
  ctx.orig_to_my_player = {1, 2, 0};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 5; ++o) ctx.orig_to_my_order[o] = o + 5;     // Bob
  for (int o = 5; o < 10; ++o) ctx.orig_to_my_order[o] = o + 5;    // Cathy
  for (int o = 10; o < 15; ++o) ctx.orig_to_my_order[o] = o - 10;  // Alice
  for (int o = 15; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int i = 0; i < 5; ++i) ctx.my_order_to_id[i] = kDeck[10 + i];
  for (int i = 0; i < 5; ++i) ctx.my_order_to_id[5 + i] = kDeck[i];
  for (int i = 0; i < 5; ++i) ctx.my_order_to_id[10 + i] = kDeck[5 + i];
  for (int i = 15; i < 30; ++i) ctx.my_order_to_id[i] = kDeck[i];
  return ctx;
}

}  // namespace

TEST(EndgameReplay1882573, Turn18NoteDoesNotResetU5) {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot67 (P0 orig): orig orders [4,3,2,1,0] = [b2,r3,u1,r4,u2].
      {"b2", "r3", "u1", "r4", "u2"},
      // Cathy = yagami_black (P1 orig): orig [9,8,7,6,5] = [u2,r1,r1,b4,u3].
      {"u2", "r1", "r1", "b4", "u3"},
  };
  opts.variant_name = "Light-Pink-Ones & Null (3 Suits)";
  opts.starting = TestPlayer::BOB;  // orig P0 (will-bot67) starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  // Reproduce apply_action's behavior step-by-step: between each individual
  // handle_action call we call compute_note_segments and dump any segment
  // emitted for order 4. That mirrors the bot's net/commands.cpp::apply_action
  // hook where notes are diffed per gameAction received.
  std::vector<std::string> segments_for_order_4;
  auto step = [&](const Action& a) {
    Game prev_snapshot = g;
    g.handle_action(a);
    auto segs = hanabi::net::compute_note_segments(prev_snapshot, g);
    for (const auto& [order, seg] : segs) {
      if (order == 4) segments_for_order_4.push_back(seg);
    }
  };

  auto apply_step_by_step = [&](const OrigAction& a) {
    int pi = g.state.current_player_index;
    g.catchup = true;
    if (a.type == 0 || a.type == 1) {
      int orig_order = a.target;
      int my_order = ctx.orig_to_my_order[orig_order];
      auto [suit, rank] = ctx.deck[orig_order];
      if (a.type == 0) {
        bool playable = g.state.play_stacks[suit] + 1 == rank;
        if (playable) {
          step(Action{PlayAction{pi, my_order, suit, rank}});
        } else {
          step(Action{DiscardAction{pi, my_order, suit, rank, /*failed=*/true}});
        }
      } else {
        step(Action{DiscardAction{pi, my_order, suit, rank, false}});
      }
      int new_my_order = g.state.next_card_order;
      if (new_my_order < static_cast<int>(ctx.deck.size())) {
        auto [d_suit, d_rank] = ctx.deck[new_my_order];
        if (pi == g.state.our_player_index) {
          step(Action{DrawAction{pi, new_my_order, -1, -1}});
        } else {
          step(Action{DrawAction{pi, new_my_order, d_suit, d_rank}});
        }
      }
    } else if (a.type == 2 || a.type == 3) {
      ClueKind kind = a.type == 2 ? ClueKind::COLOUR : ClueKind::RANK;
      int target = ctx.orig_to_my_player[a.target];
      auto touched = touched_orders(*g.state.variant, g.state.hands[target], ctx,
                                       kind, a.value);
      step(Action{ClueAction{pi, target, std::move(touched), BaseClue(kind, a.value)}});
    }
    step(Action{TurnAction{g.state.turn_count, g.state.next_player_index(pi)}});
    g.catchup = false;
  };

  for (const auto& a : kOrigActions) apply_step_by_step(a);

  // The bug: across the turn-17 / turn-18 transition (during action 16's
  // PlayAction → TurnAction chain) the bot marked u5 CALLED_TO_PLAY only to
  // immediately reset it. The user observed this as an unexpected
  // "[f] u5 | [reset]" pair on the same turn. Verify neither segment was
  // emitted at turn 17 nor turn 18.
  for (const auto& seg : segments_for_order_4) {
    EXPECT_FALSE(seg.rfind("turn 17:", 0) == 0)
        << "spurious turn-17 note on u5: " << seg;
    EXPECT_FALSE(seg.rfind("turn 18:", 0) == 0)
        << "spurious turn-18 note on u5: " << seg;
  }
}
