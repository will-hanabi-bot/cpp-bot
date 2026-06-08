// Hanab.live replay 1884410. Variant "Orange (3 Suits)". Players: orig
// P0=will-bot67 (observer / reacter), P1=will-bot69, P2=yagami_black.
//
// Two checks under the corrected outcome-oriented replay parser:
//
//   1. After action 5 (yagami physically discards orange-1 on a playable
//      orange stack), the orange stack must read 1 — the actual hanab.live
//      UI state. Under the old button-oriented parser the engine left it
//      at 0, which made every downstream `is_playable(orange)` query
//      wrong.
//
//   2. After yagami's red clue to will-bot69 at action 8, the convention
//      identifies will-bot69's slot 2 (= o2, currently playable on
//      orange=1) as the play_target, swaps the reacter call for the
//      inverted target, and stamps CTP+urgent on will-bot67's slot 3
//      (= r2). will-bot67's take_action must return PerformPlay on that
//      slot.
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

const std::vector<std::pair<int, int>> kDeck = {
    {1, 2}, {1, 3}, {0, 2}, {0, 5}, {1, 1},
    {1, 1}, {0, 1}, {1, 2}, {1, 4}, {1, 4},
    {2, 4}, {2, 4}, {1, 1}, {0, 1}, {0, 4},
    {2, 2}, {2, 1}, {2, 2}, {1, 5}, {2, 5},
    {0, 3}, {0, 2}, {0, 4}, {2, 3}, {2, 1},
    {1, 3}, {2, 1}, {2, 3}, {0, 3}, {0, 1},
};

const std::vector<OrigAction> kOrigActions = {
    {3, 2, 1},   // 0: P0 will-bot67 rank-1 to P2 yagami
    {0, 5, 0},   // 1: P1 will-bot69 plays order 5 (b1)
    {0, 13, 0},  // 2: P2 yagami plays order 13 (r1)
    {2, 2, 0},   // 3: P0 colour red to P2 yagami
    {1, 15, 0},  // 4: P1 will-bot69 discards order 15 (o2, MISPLAY: orange=0)
    {0, 16, 0},  // 5: P2 yagami plays order 16 (o1) — outcome=advance, orange 0→1
    {3, 1, 1},   // 6: P0 rank-1 to P1
    {0, 7, 0},   // 7: P1 plays order 7 (b2)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Identity mapping: orig P0 → ALICE (observer), P1 → BOB, P2 → CATHY.
  ctx.orig_to_my_player = {0, 1, 2};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int orig_o = 0; orig_o < 30; ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

Game build_through_action_7() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot67 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot69: orig [9,8,7,6,5] newest-first = [b4, b4, b2, r1, b1].
      {"b4", "b4", "b2", "r1", "b1"},
      // Cathy = yagami: orig [14,13,12,11,10] newest-first = [r4, r1, b1, o4, o4].
      {"r4", "r1", "b1", "o4", "o4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;  // orig P0 (will-bot67) starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

// Sanity gate on the outcome-oriented parser: after action 5 (yagami's
// physical Discard of o1 on a playable orange stack), the orange stack
// must read 1.
TEST(EndgameReplay1884410, OrangeStackAdvancesAfterAction5) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b4", "b4", "b2", "r1", "b1"},
      {"r4", "r1", "b1", "o4", "o4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (int i = 0; i <= 5; ++i) apply_orig_action(g, kOrigActions[i], ctx);

  // After action 5: red 1, blue 1, orange 1; strike 1 (from action 4).
  EXPECT_EQ(g.state.play_stacks[0], 1);  // red
  EXPECT_EQ(g.state.play_stacks[1], 1);  // blue
  EXPECT_EQ(g.state.play_stacks[2], 1)
      << "orange must advance via outcome-oriented parser";
  EXPECT_EQ(g.state.strikes, 1);
}

// Regression for the user's bug: at action 8 yagami clues red to
// will-bot69. will-bot67 (the reacter, our observer) should be marked
// CTP+urgent on slot 3 (= r2, currently playable on red 1) via the
// convention's inverted-target play_target swap (target = will-bot69's
// slot 2 = o2, playable on orange 1).
TEST(EndgameReplay1884410, WillBot67PlaysSlot3InResponseToRedClueToBob) {
  Game g = build_through_action_7();

  // It should be yagami's turn (CATHY), about to clue.
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::CATHY));
  ASSERT_EQ(g.state.play_stacks[0], 1);
  ASSERT_EQ(g.state.play_stacks[1], 2);  // b2 just played at action 7
  ASSERT_EQ(g.state.play_stacks[2], 1);

  ReplayContext ctx = make_ctx();
  apply_orig_action(g, OrigAction{2, 1, 0}, ctx);  // action 8: red → P1

  // Now it's Alice's (will-bot67's) turn — the reacter.
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));

  // Alice's slot 3 = my order 2 = r2. Must be CTP+urgent.
  int alice_slot_3 = g.state.hands[0][2];
  ASSERT_EQ(alice_slot_3, 2);
  EXPECT_EQ(g.meta[alice_slot_3].status, CardStatus::CALLED_TO_PLAY)
      << "convention should CTP slot 3 via inverted-target play_target swap";
  EXPECT_TRUE(g.meta[alice_slot_3].urgent);

  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "will-bot67 must respond with PerformPlay on slot 3 (r2), not clue";
  EXPECT_EQ(std::get<PerformPlay>(perform).target, alice_slot_3);
}
