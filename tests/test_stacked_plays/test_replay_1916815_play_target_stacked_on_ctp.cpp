// Regression test for replay 1916815 T14/T15 (Ambiguous & Dark Null
// (5 Suits), 3-player). Players: orig P0=will-bot67, P1=will-bot69,
// P2=yagami_black. Category: test_stacked_plays.
//
// A reactive play target must never STACK on an already-CTP'd card: the
// receiver's pending CTPs are simulated first (they will play on their
// own), and the convention targets the NEXT playable under the
// post-simulation stacks.
//
// Here wb67's slot 2 (o20 = m1) was CTP'd by the T11 blue reactive
// (yagami's T12 discard reaction), and the newly drawn m2 sits on wb67's
// slot 1 (o22). At T14 wb69 clued rank-2 to wb67 intending to "re-get"
// the already-called m1 (its model mapped target slot 2 → react slot 1 =
// yagami's playable t1). The human reacter read the clue per the
// convention — pending m1 simulated → target = m2 on slot 1 → react
// slot 2 — and bombed m3 at T15. With the pending-CTP simulation the
// model reads the clue like the human (react slot 2, giver-visibly a
// bomb), the eval rejects rank-2, and wb69 clues rank-1 to yagami
// instead (playable-rank CTP on the t1).

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

// 45-card deck from the hanab.live export (tests/test_endgame/replays/
// 1916815.json). Suits: Tomato(0) Mahogany(1) Berry(2) Navy(3) Dark Null(4).
const std::vector<std::pair<int, int>> kDeck = {
    {3,3}, {0,3}, {3,4}, {3,1}, {2,2},  // 0-4: n3 t3 n4 n1 b2
    {0,4}, {0,3}, {2,1}, {2,3}, {3,1},  // 5-9: t4 t3 b1 b3 n1
    {2,5}, {2,4}, {4,4}, {2,1}, {1,3},  // 10-14: b5 b4 u4 b1 m3
    {2,2}, {0,2}, {2,4}, {3,2}, {1,4},  // 15-19: b2 t2 b4 n2 m4
    {1,1}, {0,1}, {1,2}, {1,1}, {3,4},  // 20-24: m1 t1 m2 m1 n4
    {1,3}, {4,3}, {4,2}, {0,1}, {3,1},  // 25-29: m3 u3 u2 t1 n1
    {3,3}, {0,1}, {0,4}, {2,3}, {1,2},  // 30-34: n3 t1 t4 b3 m2
    {4,5}, {3,2}, {3,5}, {0,2}, {2,1},  // 35-39: u5 n2 n5 t2 b1
    {1,4}, {1,5}, {4,1}, {1,1}, {0,5},  // 40-44: m4 m5 u1 m1 t5
};

// Actions T1..T13 (T14 is the decision under test).
const std::vector<OrigAction> kActions = {
    {2,1,1},   // T1  P0 wb67  colour-1 (Blue) -> P1 wb69
    {3,0,3},   // T2  P1 wb69  rank-3 -> P0 wb67
    {0,13,0},  // T3  P2 yag   plays ord 13 = b1
    {0,3,0},   // T4  P0 wb67  plays ord 3  = n1
    {2,2,0},   // T5  P1 wb69  colour-0 (Red) -> P2 yagami
    {0,15,0},  // T6  P2 yag   plays ord 15 = b2
    {1,16,0},  // T7  P0 wb67  discards ord 16 = t2
    {2,0,1},   // T8  P1 wb69  colour-1 (Blue) -> P0 wb67
    {1,17,0},  // T9  P2 yag   discards ord 17 = b4
    {0,18,0},  // T10 P0 wb67  plays ord 18 = n2
    {2,0,1},   // T11 P1 wb69  colour-1 (Blue) -> P0 wb67 (reactive)
    {1,19,0},  // T12 P2 yag   discards ord 19 = m4 (reaction: CTPs wb67 m1)
    {0,0,0},   // T13 P0 wb67  plays ord 0  = n3
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // POV = will-bot69 (orig P1, the T14 decider) -> my P0. Cycle:
  //   orig P1 (will-bot69) -> my P0  (POV)
  //   orig P2 (yagami)     -> my P1
  //   orig P0 (will-bot67) -> my P2
  ctx.orig_to_my_player = {2, 0, 1};
  const int N = static_cast<int>(kDeck.size());
  ctx.orig_to_my_order.resize(N);
  ctx.my_order_to_id.resize(N);
  for (int orig_p = 0; orig_p < 3; ++orig_p) {
    int my_p = ctx.orig_to_my_player[orig_p];
    for (int i = 0; i < 5; ++i) {
      ctx.orig_to_my_order[orig_p * 5 + i] = my_p * 5 + i;
    }
  }
  for (int o = 15; o < N; ++o) ctx.orig_to_my_order[o] = o;
  for (int orig_o = 0; orig_o < N; ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

Game build_start() {
  SetupOptions opts;
  // my P0 = will-bot69 (POV, hidden). orig orders [9,8,7,6,5]:
  //   n1, b3, b1, t3, t4.
  // my P1 = yagami (orig P2). orig orders [14,13,12,11,10]:
  //   (1,3)=m3, (2,1)=b1, (4,4)=u4, (2,4)=b4, (2,5)=b5.
  // my P2 = will-bot67 (orig P0). orig orders [4,3,2,1,0]:
  //   (2,2)=b2, (3,1)=n1, (3,4)=n4, (0,3)=t3, (3,3)=n3.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"m3", "b1", "u4", "b4", "b5"},
      {"b2", "n1", "n4", "t3", "n3"},
  };
  opts.variant_name = "Ambiguous & Dark Null (5 Suits)";
  opts.starting = TestPlayer::CATHY;  // orig P0 (will-bot67) -> my P2
  return setup(std::move(opts));
}

}  // namespace

// Interp level: the rank-2 to wb67 must be read like the human reacter —
// pending m1 (my o20, slot 2, already CTP) simulated, target = the newly
// drawn m2 (slot 1) → react slot 2 = yagami's o9 (m3). The pre-fix model
// stacked the target on the CTP'd m1 and called yagami's slot 1 (o21)
// instead.
TEST(StackedPlays1916815, Rank2TargetsNextPlayableNotStackedCTP) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < kActions.size(); ++i) apply_orig_action(g, kActions[i], ctx);

  // Sanity: wb67's slot 2 m1 is CTP'd by the T11/T12 reactive.
  ASSERT_EQ(g.meta[20].status, CardStatus::CALLED_TO_PLAY)
      << "test premise: wb67's m1 (my o20) must already be CTP'd";

  // T14: apply the rank-2 clue to wb67 (my P2).
  apply_orig_action(g, OrigAction{3, 0, 2}, ctx);

  EXPECT_EQ(g.meta[9].status, CardStatus::CALLED_TO_PLAY)
      << "rank-2 (focus 3) must target the post-simulation next playable "
         "m2 (slot 1) → react slot 2 = yagami's o9";
  EXPECT_NE(g.meta[21].status, CardStatus::CALLED_TO_PLAY)
      << "yagami's slot 1 (o21) is the pre-fix 're-get m1' react — the "
         "target must not stack on the CTP'd m1";
  EXPECT_EQ(g.meta[20].status, CardStatus::CALLED_TO_PLAY)
      << "the already-called m1 keeps its CTP";
}

// Decision level: wb69 must not give the rank-2 (its convention read is
// a giver-visible bomb on yagami's slot 2); the expected clue is rank-1
// to yagami, a playable-rank CTP on the t1 in slot 1.
TEST(StackedPlays1916815, T14CluesRank1InsteadOfRank2) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < kActions.size(); ++i) apply_orig_action(g, kActions[i], ctx);

  PerformAction perform = g.take_action();

  bool is_buggy_rank2 = std::holds_alternative<PerformRank>(perform) &&
                        std::get<PerformRank>(perform).target == 2 &&
                        std::get<PerformRank>(perform).value == 2;
  EXPECT_FALSE(is_buggy_rank2)
      << "rank-2 to wb67 reads as react slot 2 (yagami's m3) — a bomb "
         "the eval must reject";

  bool is_rank1_to_yagami = std::holds_alternative<PerformRank>(perform) &&
                            std::get<PerformRank>(perform).target == 1 &&
                            std::get<PerformRank>(perform).value == 1;
  EXPECT_TRUE(is_rank1_to_yagami)
      << "expected rank-1 -> yagami (playable-rank CTP on the slot-1 t1)";
}
