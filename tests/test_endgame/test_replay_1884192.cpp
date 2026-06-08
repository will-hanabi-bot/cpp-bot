// Hanab.live replay 1884192 turn 20 (action index 19). Variant
// "Light-Pink-Fives & Prism (3 Suits)". Players: orig P0=yagami_black,
// P1=will-bot69 (the reacter / our bot POV), P2=will-bot67.
//
// At action 18 yagami clues rank-3 to will-bot67 (P2). This is a reactive
// rank clue: the reacter is will-bot69 (next after yagami). The
// convention iterates receiver play_targets and derives the reacter slot
// via calc_slot(focus_slot, target_slot, hand_size). For
// Light-Pink-Fives & Prism the rank-clue focus_slot is the clue value
// (3) — so:
//   target_slot=1 (r4) → react_slot=2 (i2 actual)
//   target_slot=2 (i5) → react_slot=1 (r4 actual)
//   target_slot=3 (b4) → react_slot=5 (i1 actual)
// The bug: the bot committed to (react_slot=2) and misplayed i2.
//
// Player remap: ALICE = will-bot69 (orig P1, observer); BOB = will-bot67
// (orig P2); CATHY = yagami (orig P0). opts.starting = CATHY (yagami
// starts) so the turn cycle CATHY → ALICE → BOB matches the orig.
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
    {2, 4}, {0, 1}, {2, 1}, {2, 3}, {2, 2},
    {0, 3}, {2, 1}, {0, 2}, {0, 2}, {2, 1},
    {2, 3}, {1, 3}, {1, 1}, {1, 1}, {2, 4},
    {0, 1}, {1, 1}, {1, 4}, {1, 3}, {2, 2},
    {1, 4}, {1, 5}, {1, 2}, {2, 5}, {1, 2},
    {0, 4}, {0, 4}, {0, 1}, {0, 3}, {0, 5},
};

// Actions 0..17 — drives the game to just before yagami's pivotal
// rank-3 clue (action 18) which is the reactive clue we want to probe.
const std::vector<OrigAction> kOrigActions = {
    {3, 2, 3},   // 0:  P0 → P2 rank-3
    {0, 9, 0},   // 1:  P1 plays order 9 (i1)
    {0, 13, 0},  // 2:  P2 plays order 13 (b1)
    {2, 2, 1},   // 3:  P0 colour 1 (Blue) → P2
    {0, 15, 0},  // 4:  P1 plays order 15 (r1)
    {3, 1, 3},   // 5:  P2 → P1 rank-3
    {0, 4, 0},   // 6:  P0 plays order 4 (i2)
    {0, 8, 0},   // 7:  P1 plays order 8 (r2)
    {1, 16, 0},  // 8:  P2 discards order 16 (b1)
    {1, 18, 0},  // 9:  P0 discards order 18 (b3)
    {3, 2, 1},   // 10: P0 → P2 rank-1
    {3, 1, 2},   // 11: P0 → P1 rank-2
    {0, 3, 0},   // 12: P0 plays order 3 (i3)
    {3, 0, 3},   // 13: P2 → P0 rank-3
    {0, 14, 0},  // 14: P2 plays order 14 (i4)
    {0, 22, 0},  // 15: P0 plays order 22 (b2)
    {0, 5, 0},   // 16: P1 plays order 5 (r3)
    {0, 11, 0},  // 17: P2 plays order 11 (b3)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // orig P0 (yagami) → CATHY, orig P1 (will-bot69, observer) → ALICE,
  // orig P2 (will-bot67) → BOB.
  ctx.orig_to_my_player = {2, 0, 1};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 5; ++o) ctx.orig_to_my_order[o] = 10 + o;   // P0 → CATHY
  for (int o = 5; o < 10; ++o) ctx.orig_to_my_order[o] = o - 5;   // P1 → ALICE
  for (int o = 10; o < 15; ++o) ctx.orig_to_my_order[o] = o - 5;  // P2 → BOB
  for (int o = 15; o < 30; ++o) ctx.orig_to_my_order[o] = o;      // draws
  ctx.my_order_to_id.resize(30);
  for (int orig_o = 0; orig_o < 30; ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

Game build_through_action_17() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot67: orig P2 orders [14,13,12,11,10] newest-first.
      // deck[14]=(2,4)=i4, deck[13]=(1,1)=b1, deck[12]=(1,1)=b1,
      // deck[11]=(1,3)=b3, deck[10]=(2,3)=i3.
      {"i4", "b1", "b1", "b3", "i3"},
      // Cathy = yagami: orig P0 orders [4,3,2,1,0].
      // deck[4]=(2,2)=i2, deck[3]=(2,3)=i3, deck[2]=(2,1)=i1,
      // deck[1]=(0,1)=r1, deck[0]=(2,4)=i4.
      {"i2", "i3", "i1", "r1", "i4"},
  };
  opts.variant_name = "Light-Pink-Fives & Prism (3 Suits)";
  opts.starting = TestPlayer::CATHY;  // orig P0 (yagami) starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

// Yagami's rank-3 clue to will-bot67 is reactive. The convention's
// first-iteration mapping (target_slot=1 → react_slot=2) lands on Alice's
// order 19, which is actually i2 (trash) — but Alice's possible at
// convention time still contained i5 (BOB's visible copy, not yet
// elim'd), so the `ok` check passed. With the effective-possible
// (visibility-narrowed) check we now reject that mapping and fall
// through to the next play_target (slot 2 = i5), which yields react_slot
// 1 (Alice's r4). Alice should play that.
TEST(EndgameReplay1884192, ReacterFallsThroughToPlayableReactSlot) {
  Game g = build_through_action_17();

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::CATHY));
  ASSERT_EQ(g.state.play_stacks[0], 3);  // red
  ASSERT_EQ(g.state.play_stacks[1], 3);  // blue
  ASSERT_EQ(g.state.play_stacks[2], 4);  // prism

  // Apply yagami's rank-3 clue to will-bot67.
  ReplayContext ctx = make_ctx();
  apply_orig_action(g, OrigAction{3, 2, 3}, ctx);

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));

  // Order 25 = Alice's slot 1 = r4 (currently playable). Convention
  // should CTP it as the react_slot for target_slot=2 (i5 in Bob).
  int alice_slot_1 = g.state.hands[0][0];
  ASSERT_EQ(alice_slot_1, 25);
  EXPECT_EQ(g.meta[alice_slot_1].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(g.meta[alice_slot_1].urgent);

  // Order 19 = Alice's slot 2 = i2 (trash). Must NOT be CTP'd.
  int alice_slot_2 = g.state.hands[0][1];
  ASSERT_EQ(alice_slot_2, 19);
  EXPECT_NE(g.meta[alice_slot_2].status, CardStatus::CALLED_TO_PLAY);

  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform));
  EXPECT_EQ(std::get<PerformPlay>(perform).target, alice_slot_1)
      << "bot must play r4 (slot 1) not i2 (slot 2)";
}
