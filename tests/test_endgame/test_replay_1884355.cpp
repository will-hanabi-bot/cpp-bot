// Hanab.live replay 1884355 action 11. Variant
// "Light-Pink-Fives & Omni (3 Suits)". orig players: P0=yagami_black,
// P1=will-bot67, P2=will-bot69 (the POV at action 11).
//
// Action 9: P0 clues rank-2 to P2 (newly touches orig orders 11, 20).
// Action 10: P1 plays orig 15 (b5). That play is unexpected under the
// stable LOCK reading of action 9, so react_play rewinds turn 9 and
// re-interprets as REACTIVE; target_i_play marks P2's slot 5 (= orig
// order 10 = r3) CALLED_TO_PLAY with info_lock {r3}. At action 11 the
// bot must play that r3 rather than discarding the empathy-known b2
// on slot 4 — the same discard-with-known-play class as 1884157.
//
// Player remap: orig P0 → BOB, P1 → CATHY, P2 → ALICE. This preserves
// the orig cycle P0→P1→P2 as the test cycle ALICE→BOB→CATHY (forward
// in indices) with starting=BOB.
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

// hanab.live export deck (1884355). Suits: 0=Red, 1=Blue, 2=Omni.
const std::vector<std::pair<int, int>> kDeck = {
    {1, 3}, {1, 2}, {1, 1}, {0, 4}, {2, 1},
    {1, 1}, {0, 2}, {1, 3}, {0, 3}, {2, 3},
    {0, 3}, {1, 2}, {0, 4}, {1, 4}, {0, 1},
    {1, 5}, {1, 1}, {0, 2}, {2, 2}, {2, 4},
    {2, 4}, {2, 1}, {1, 4}, {2, 3}, {0, 1},
    {0, 1}, {2, 1}, {0, 5}, {2, 2}, {2, 5},
};

// Actions 0..10 — drives the game to just before will-bot69's discard
// of orig order 11 (b2) at action 11.
const std::vector<OrigAction> kOrigActions = {
    {3, 2, 1},   // 0: P0 → P2 rank-1
    {0, 5, 0},   // 1: P1 plays orig 5 (b1)
    {3, 1, 3},   // 2: P2 → P1 rank-3
    {0, 1, 0},   // 3: P0 plays orig 1 (b2)
    {0, 7, 0},   // 4: P1 plays orig 7 (b3)
    {0, 14, 0},  // 5: P2 plays orig 14 (r1)
    {3, 2, 3},   // 6: P0 → P2 rank-3
    {0, 17, 0},  // 7: P1 plays orig 17 (r2)
    {0, 13, 0},  // 8: P2 plays orig 13 (b4)
    {3, 2, 2},   // 9: P0 → P2 rank-2  (the reactive clue)
    {0, 15, 0},  // 10: P1 plays orig 15 (b5)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // orig P0 → BOB=1, orig P1 → CATHY=2, orig P2 → ALICE=0.
  ctx.orig_to_my_player = {1, 2, 0};
  ctx.orig_to_my_order.resize(30);
  // Initial deal lands my orders 0..4 in ALICE's hand (= orig P2),
  // 5..9 in BOB (= orig P0), 10..14 in CATHY (= orig P1).
  for (int o = 10; o < 15; ++o) ctx.orig_to_my_order[o] = o - 10;
  for (int o = 0; o < 5; ++o)   ctx.orig_to_my_order[o] = o + 5;
  for (int o = 5; o < 10; ++o)  ctx.orig_to_my_order[o] = o + 5;
  for (int o = 15; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int orig_o = 0; orig_o < 30; ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

Game build_through_action_10() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = yagami (orig P0): orig [4,3,2,1,0] = [o1, r4, b1, b2, b3].
      {"o1", "r4", "b1", "b2", "b3"},
      // Cathy = will-bot67 (orig P1): orig [9,8,7,6,5] = [o3, r3, b3, r2, b1].
      {"o3", "r3", "b3", "r2", "b1"},
  };
  opts.variant_name = "Light-Pink-Fives & Omni (3 Suits)";
  opts.starting = TestPlayer::BOB;  // orig P0 starts → BOB.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

TEST(EndgameReplay1884355, PlaysKnownR3InsteadOfDiscardingAtAction11) {
  Game g = build_through_action_10();

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE))
      << "should be will-bot69's turn at action 11";

  // Sanity-check the state we believe causes the bug. Slot 5 of
  // will-bot69 (oldest = back of hand) is the reactive-pushed r3:
  // CALLED_TO_PLAY, inferred singleton (0,3), info_lock {r3}, with
  // the red stack at 2 so r3 is currently playable. Slot 4 is the
  // empathy-known b2 (basic trash with blue stack at 5).
  const auto& alice_hand = g.state.hands[0];
  int r3_order = alice_hand.back();
  int b2_order = alice_hand[alice_hand.size() - 2];

  ASSERT_EQ(g.state.play_stacks[0], 2);
  ASSERT_EQ(g.state.play_stacks[1], 5);
  ASSERT_EQ(g.meta[r3_order].status, CardStatus::CALLED_TO_PLAY);
  auto inferred_r3 = g.me().thoughts[r3_order].id(/*infer=*/true);
  ASSERT_TRUE(inferred_r3.has_value());
  EXPECT_EQ(inferred_r3->suit_index, 0);
  EXPECT_EQ(inferred_r3->rank, 3);
  auto inferred_b2 = g.me().thoughts[b2_order].id(/*infer=*/true);
  ASSERT_TRUE(inferred_b2.has_value());
  EXPECT_EQ(inferred_b2->suit_index, 1);
  EXPECT_EQ(inferred_b2->rank, 2);

  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "bot must play the known-r3 (slot 5) rather than discard the b2";
  EXPECT_EQ(std::get<PerformPlay>(perform).target, r3_order);
}
