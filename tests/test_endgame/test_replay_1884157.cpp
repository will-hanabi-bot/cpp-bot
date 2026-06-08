// Hanab.live replay 1884157 turn 11 (action index 10). Variant
// "Null-Fives & Light Pink (3 Suits)". Players: orig P0=yagami_black,
// P1=will-bot67 (our bot POV), P2=will-bot69.
//
// At action 10 the bot was observed discarding order 8 (b1) even though
// it appears to have an empathy-known b3 at order 7 (per the in-game
// notes: "turn 7: [f] b3" on order 7). Blue stack is at 2 by that point,
// so b3 is currently playable. The user says: outside of endgame or a
// duplicate already CTP'd elsewhere, discarding when a known play is
// available should be heavily penalized.
//
// Player remap: ALICE = will-bot67 (orig P1, observer); BOB = will-bot69
// (orig P2); CATHY = yagami (orig P0). opts.starting = CATHY so the turn
// cycle CATHY → ALICE → BOB matches yagami → will-bot67 → will-bot69.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// hanab.live export deck. Suits: 0=Red, 1=Blue, 2=Light Pink. 30 cards.
const std::vector<std::pair<int, int>> kDeck = {
    {2, 1}, {2, 5}, {0, 4}, {1, 1}, {1, 2},
    {1, 4}, {2, 1}, {1, 3}, {1, 1}, {2, 2},
    {2, 3}, {2, 2}, {0, 3}, {1, 3}, {2, 3},
    {1, 4}, {0, 1}, {0, 4}, {1, 5}, {0, 1},
    {2, 4}, {2, 1}, {0, 3}, {1, 1}, {0, 2},
    {0, 1}, {0, 2}, {1, 2}, {2, 4}, {0, 5},
};

// Actions 0..9 — drives the game to just before will-bot67's discard of
// order 8 (b1) at action 10.
const std::vector<OrigAction> kOrigActions = {
    {3, 2, 3},   // 0: P0 → P2 rank-3
    {0, 6, 0},   // 1: P1 plays order 6 (i1)
    {3, 1, 4},   // 2: P2 → P1 rank-4
    {0, 3, 0},   // 3: P0 plays order 3 (b1)
    {0, 9, 0},   // 4: P1 plays order 9 (i2)
    {3, 1, 1},   // 5: P2 → P1 rank-1
    {0, 4, 0},   // 6: P0 plays order 4 (b2)
    {3, 0, 2},   // 7: P1 → P0 rank-2
    {0, 10, 0},  // 8: P2 plays order 10 (i3)
    {0, 16, 0},  // 9: P0 plays order 16 (r1)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // orig P0 (yagami) → CATHY, orig P1 (will-bot67) → ALICE,
  // orig P2 (will-bot69) → BOB.
  ctx.orig_to_my_player = {2, 0, 1};
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

Game build_through_action_9() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot67 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot69: orig [14,13,12,11,10] = [i3, b3, r3, i2, i3].
      {"i3", "b3", "r3", "i2", "i3"},
      // Cathy = yagami: orig [4,3,2,1,0] = [b2, b1, r4, i5, i1].
      {"b2", "b1", "r4", "i5", "i1"},
  };
  opts.variant_name = "Null-Fives & Light Pink (3 Suits)";
  opts.starting = TestPlayer::CATHY;  // orig P0 starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

TEST(EndgameReplay1884157, PlaysKnownB3InsteadOfDiscardingAtAction10) {
  Game g = build_through_action_9();

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE))
      << "should be will-bot67's turn at action 10";

  // Sanity-check the state we believe causes the bug. Order 2 is
  // will-bot67's empathy-known b3 (CALLED_TO_PLAY, inferred singleton
  // (1,3)); the blue stack is at 2 so b3 is currently playable.
  ASSERT_EQ(g.state.play_stacks[1], 2);
  ASSERT_EQ(g.meta[2].status, CardStatus::CALLED_TO_PLAY);
  auto inferred_b3 = g.me().thoughts[2].id(/*infer=*/true);
  ASSERT_TRUE(inferred_b3.has_value());
  EXPECT_EQ(inferred_b3->suit_index, 1);
  EXPECT_EQ(inferred_b3->rank, 3);

  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "bot must play the known-b3 (order 2) rather than discard";
  EXPECT_EQ(std::get<PerformPlay>(perform).target, 2);
}
