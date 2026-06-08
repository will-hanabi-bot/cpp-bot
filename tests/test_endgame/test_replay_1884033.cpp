// Hanab.live replay 1884033 turn 20 (action index 19). Variant
// "Null-Fives & Brown (3 Suits)". Players: orig P0=will-bot69,
// P1=will-bot67 (our bot POV), P2=yagami_black.
//
// By the time it's will-bot67's turn at action 19, two of his cards are
// CALLED_TO_PLAY at the same time:
//   * red-4 (orig order 18), CTP from yagami's first rank-4 clue on
//     action 14.
//   * red-5 (orig order 24), CTP from the reactive resolution after
//     yagami's second rank-4 at action 17 + will-bot69's slot-2 b5 play
//     at action 18.
//
// Convention: plays must be resolved in queue order — r4 must play
// before r5. Pre-fix the bot picked r5 first and misplayed it onto a red
// stack still at 3.
//
// To keep the observer = will-bot67 (so we can call take_action directly
// from his POV without remapping our_player_index after the fact) we
// remap: orig P0 (will-bot69) → CATHY, orig P1 (will-bot67) → ALICE,
// orig P2 (yagami) → BOB. orig P0 starts, so opts.starting = CATHY and
// the turn order CATHY → ALICE → BOB matches will-bot69 →
// will-bot67 → yagami_black.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// hanab.live export deck (orig orders 0..29). Suits: 0=Red, 1=Blue,
// 2=Brown ("n" in the test-harness short form).
const std::vector<std::pair<int, int>> kDeck = {
    {2, 1}, {1, 4}, {0, 3}, {0, 3}, {0, 2},
    {1, 3}, {2, 3}, {1, 2}, {1, 1}, {0, 2},
    {2, 2}, {1, 1}, {0, 4}, {2, 1}, {1, 1},
    {1, 4}, {0, 1}, {0, 1}, {0, 4}, {2, 4},
    {2, 2}, {1, 5}, {1, 3}, {2, 4}, {0, 5},
    {0, 1}, {2, 3}, {2, 5}, {1, 2}, {2, 1},
};

// Actions 0..18 — through will-bot69's slot-2 b5 play that completes
// the second reactive resolution and marks will-bot67's slot-1 (= orig
// order 24 = r5) as CTP.
const std::vector<OrigAction> kOrigActions = {
    {3, 1, 1},   // 0:  P0 → P1 rank-1
    {3, 0, 2},   // 1:  P1 → P0 rank-2
    {0, 14, 0},  // 2:  P2 plays order 14 (b1)
    {2, 1, 1},   // 3:  P0 → P1 colour blue
    {1, 8, 0},   // 4:  P1 discards order 8 (b1)
    {2, 0, 0},   // 5:  P2 → P0 colour red
    {0, 0, 0},   // 6:  P0 plays order 0 (n1)
    {0, 16, 0},  // 7:  P1 plays order 16 (r1)
    {3, 1, 2},   // 8:  P2 → P1 rank-2
    {0, 4, 0},   // 9:  P0 plays order 4 (r2)
    {0, 7, 0},   // 10: P1 plays order 7 (b2)
    {3, 1, 3},   // 11: P2 → P1 rank-3
    {0, 2, 0},   // 12: P0 plays order 2 (r3)
    {0, 5, 0},   // 13: P1 plays order 5 (b3)
    {3, 1, 4},   // 14: P2 → P1 rank-4  ← marks r4 (order 18) CTP'ish
    {0, 1, 0},   // 15: P0 plays order 1 (b4)
    {0, 20, 0},  // 16: P1 plays order 20 (n2)
    {3, 1, 4},   // 17: P2 → P1 rank-4  ← reclue, reactive setup
    {0, 21, 0},  // 18: P0 plays order 21 (b5) — reactive resolution
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Player remap:
  //   orig P0 (will-bot69) → CATHY (our P2)
  //   orig P1 (will-bot67) → ALICE (our P0, observer)
  //   orig P2 (yagami)     → BOB   (our P1)
  ctx.orig_to_my_player = {2, 0, 1};

  // Order remap. Initial deal:
  //   orig 0..4   (will-bot69's hand) → our 10..14 (CATHY's hand)
  //   orig 5..9   (will-bot67's hand) → our 0..4   (ALICE's hand)
  //   orig 10..14 (yagami's hand)     → our 5..9   (BOB's hand)
  // Subsequent draws happen in the same player order as orig because
  // CATHY → ALICE → BOB matches orig P0 → orig P1 → orig P2, so orig
  // orders 15+ map to identical our orders.
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 5; ++o) ctx.orig_to_my_order[o] = 10 + o;
  for (int o = 5; o < 10; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 10; o < 15; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 15; o < 30; ++o) ctx.orig_to_my_order[o] = o;

  ctx.my_order_to_id.resize(30);
  for (int orig_o = 0; orig_o < 30; ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

Game build_through_action_18() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot67 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = yagami_black: orig [14,13,12,11,10] = [b1, n1, r4, b1, n2].
      {"b1", "n1", "r4", "b1", "n2"},
      // Cathy = will-bot69: orig [4, 3, 2, 1, 0] = [r2, r3, r3, b4, n1].
      {"r2", "r3", "r3", "b4", "n1"},
  };
  opts.variant_name = "Null-Fives & Brown (3 Suits)";
  // orig P0 (will-bot69) starts. In our remap that's CATHY.
  opts.starting = TestPlayer::CATHY;
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

TEST(EndgameReplay1884033, PlayQueueOrderHonoursSignalTurnForMultiIdCTPs) {
  Game g = build_through_action_18();

  // After action 18 it's will-bot67's turn.
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));

  // Our-order remap: orig order 18 (r4) → our order 18 (>=15 identity).
  // orig order 24 (r5) → our order 24.
  int r4_order = 18;
  int r5_order = 24;
  ASSERT_NE(std::find(g.state.hands[0].begin(), g.state.hands[0].end(),
                       r4_order),
            g.state.hands[0].end());
  ASSERT_NE(std::find(g.state.hands[0].begin(), g.state.hands[0].end(),
                       r5_order),
            g.state.hands[0].end());

  // Diagnostic dump of both candidates.
  auto dump = [&](const char* label, int o) {
    std::cerr << "[probe] " << label << " (order " << o << ") status="
              << static_cast<int>(g.meta[o].status)
              << " signal_turn=" << g.meta[o].signal_turn.value_or(-1)
              << " inferred={";
    bool first = true;
    for (Identity i : g.common.thoughts[o].inferred) {
      if (!first) std::cerr << ", ";
      std::cerr << "(" << static_cast<int>(i.suit_index) << ","
                << static_cast<int>(i.rank) << ")";
      first = false;
    }
    std::cerr << "}\n";
  };
  dump("r4", r4_order);
  dump("r5", r5_order);
  std::cerr << "[probe] stacks: r=" << g.state.play_stacks[0]
            << " b=" << g.state.play_stacks[1]
            << " n=" << g.state.play_stacks[2] << "\n";

  // The fix's premise: both must be CALLED_TO_PLAY with r4's signal_turn
  // strictly less than r5's signal_turn so the new signal-order rule
  // restricts playable_orders to r4.
  EXPECT_EQ(g.meta[r4_order].status, CardStatus::CALLED_TO_PLAY)
      << "r4 should be CTP by action 14 by the time we hit action 19";
  EXPECT_EQ(g.meta[r5_order].status, CardStatus::CALLED_TO_PLAY)
      << "r5 should be CTP by action 18's reactive resolution";
  ASSERT_TRUE(g.meta[r4_order].signal_turn.has_value());
  ASSERT_TRUE(g.meta[r5_order].signal_turn.has_value());
  EXPECT_LT(g.meta[r4_order].signal_turn.value(),
            g.meta[r5_order].signal_turn.value())
      << "r4 was queued before r5 — its signal_turn must be smaller";

  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "expected a play action at this state";
  EXPECT_EQ(std::get<PerformPlay>(perform).target, r4_order)
      << "bot should play r4 first (earlier signal_turn) before r5";
}
