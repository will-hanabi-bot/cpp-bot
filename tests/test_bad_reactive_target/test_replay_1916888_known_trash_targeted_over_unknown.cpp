// Regression test for replay 1916888 T30/T31 (Ambiguous & Dark Null
// (5 Suits), 3-player). Players: orig P0=yagami_black, P1=will-bot67,
// P2=will-bot69. Category: test_bad_reactive_target.
//
// dc-target rule: unknown trash has the highest priority (clued-unknown
// leftmost, then unclued leftmost); globally-known trash is targeted
// only when NO unknown trash exists. At T30 wb67 (Cathy) holds exactly
// one unknown trash — the unclued n2 on slot 2 (o31, navy stack 2) —
// and a globally-known trash 1 on slot 3 (o22, empathy {y1,g1,b1}, all
// trash). The pre-fix `pre_clued_trash` pool admitted o22 (clued +
// post-clue-known) even though it was ALREADY known pre-clue, so wb69's
// red-to-wb67 read targeted the known trash and paired it with yagami's
// playable n3 (react slot 1) — and wb69 gave that clue. Correct read:
// the lone unknown trash o31 is the target (react slot 2 = yagami's t4,
// a giver-visible non-play), so no reactive clue to wb67 works and the
// best clue is the stable red to yagami colour-pushing the n3.

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
// 1916888.json). Suits: Tomato(0) Mahogany(1) Berry(2) Navy(3) Dark Null(4).
const std::vector<std::pair<int, int>> kDeck = {
    {0,1}, {0,3}, {0,3}, {1,1}, {2,1},  // 0-4: t1 t3 t3 m1 b1
    {2,4}, {4,1}, {2,5}, {3,5}, {0,4},  // 5-9: b4 u1 b5 n5 t4
    {4,4}, {2,1}, {1,5}, {2,3}, {2,2},  // 10-14: u4 b1 m5 b3 b2
    {3,2}, {1,1}, {0,1}, {3,1}, {1,3},  // 15-19: n2 m1 t1 n1 m3
    {2,3}, {2,4}, {2,1}, {1,2}, {1,3},  // 20-24: b3 b4 b1 m2 m3
    {1,4}, {0,1}, {2,2}, {4,2}, {1,4},  // 25-29: m4 t1 b2 u2 m4
    {0,4}, {3,2}, {3,1}, {3,3}, {4,5},  // 30-34: t4 n2 n1 n3 u5
    {0,2}, {0,2}, {3,3}, {0,5}, {3,4},  // 35-39: t2 t2 n3 t5 n4
    {3,4}, {1,1}, {4,3}, {1,2}, {3,1},  // 40-44: n4 m1 u3 m2 n1
};

// Actions T1..T29 (T30 is the decision under test).
const std::vector<OrigAction> kActions = {
    {3,2,5},   // T1  P0 yag   rank-5 -> P2 wb69
    {0,6,0},   // T2  P1 wb67  plays ord 6  = u1
    {0,11,0},  // T3  P2 wb69  plays ord 11 = b1
    {3,1,4},   // T4  P0 yag   rank-4 -> P1 wb67
    {2,2,1},   // T5  P1 wb67  colour-1 (Blue) -> P2 wb69
    {0,16,0},  // T6  P2 wb69  plays ord 16 = m1
    {1,4,0},   // T7  P0 yag   discards ord 4 = b1
    {3,0,1},   // T8  P1 wb67  rank-1 -> P0 yagami
    {0,17,0},  // T9  P2 wb69  plays ord 17 = t1
    {0,18,0},  // T10 P0 yag   plays ord 18 = n1
    {3,2,2},   // T11 P1 wb67  rank-2 -> P2 wb69
    {0,14,0},  // T12 P2 wb69  plays ord 14 = b2
    {3,2,5},   // T13 P0 yag   rank-5 -> P2 wb69
    {0,15,0},  // T14 P1 wb67  plays ord 15 = n2
    {3,1,1},   // T15 P2 wb69  rank-1 -> P1 wb67
    {0,20,0},  // T16 P0 yag   plays ord 20 = b3
    {0,5,0},   // T17 P1 wb67  plays ord 5  = b4
    {3,1,3},   // T18 P2 wb69  rank-3 -> P1 wb67
    {0,23,0},  // T19 P0 yag   plays ord 23 = m2
    {0,24,0},  // T20 P1 wb67  plays ord 24 = m3
    {0,13,0},  // T21 P2 wb69  plays ord 13 = b3
    {1,3,0},   // T22 P0 yag   discards ord 3 = m1
    {0,7,0},   // T23 P1 wb67  plays ord 7  = b5
    {3,1,1},   // T24 P2 wb69  rank-1 -> P1 wb67
    {0,28,0},  // T25 P0 yag   plays ord 28 = u2
    {0,29,0},  // T26 P1 wb67  plays ord 29 = m4
    {1,27,0},  // T27 P2 wb69  discards ord 27 = b2
    {1,0,0},   // T28 P0 yag   discards ord 0  = t1
    {1,26,0},  // T29 P1 wb67  discards ord 26 = t1
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // POV = will-bot69 (orig P2, the T30 decider) -> my P0. Cycle:
  //   orig P2 (will-bot69) -> my P0  (POV)
  //   orig P0 (yagami)     -> my P1
  //   orig P1 (will-bot67) -> my P2
  ctx.orig_to_my_player = {1, 2, 0};
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
  // my P0 = will-bot69 (POV, hidden). orig orders [14,13,12,11,10]:
  //   b2, b3, m5, b1, u4.
  // my P1 = yagami (orig P0). orig orders [4,3,2,1,0]:
  //   (2,1)=b1, (1,1)=m1, (0,3)=t3, (0,3)=t3, (0,1)=t1.
  // my P2 = will-bot67 (orig P1). orig orders [9,8,7,6,5]:
  //   (0,4)=t4, (3,5)=n5, (2,5)=b5, (4,1)=u1, (2,4)=b4.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b1", "m1", "t3", "t3", "t1"},
      {"t4", "n5", "b5", "u1", "b4"},
  };
  opts.variant_name = "Ambiguous & Dark Null (5 Suits)";
  opts.starting = TestPlayer::BOB;  // orig P0 (yagami) -> my P1
  return setup(std::move(opts));
}

}  // namespace

// Interp level: the red reactive to wb67 must target the lone UNKNOWN
// trash (unclued n2, slot 2, my o31) — react slot 2 = yagami's o30 —
// and must NOT pair the globally-known trash 1 (slot 3, my o22) with
// yagami's n3 (o33, react slot 1), which is the pre-fix read.
TEST(BadReactiveTarget1916888, RedReactiveTargetsUnknownTrashNotKnown) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < kActions.size(); ++i) apply_orig_action(g, kActions[i], ctx);

  // T30: apply the red clue to wb67 (orig P1).
  apply_orig_action(g, OrigAction{2, 1, 0}, ctx);

  EXPECT_NE(g.meta[33].status, CardStatus::CALLED_TO_PLAY)
      << "yagami's n3 (o33) is the pre-fix react paired with the "
         "globally-known trash 1 — known trash must not outrank the "
         "unknown trash on slot 2";
  EXPECT_EQ(g.meta[30].status, CardStatus::CALLED_TO_PLAY)
      << "the lone unknown trash (unclued n2, slot 2) is the target → "
         "react slot 2 = yagami's o30";
}

// Decision level: the red-to-wb67 reactive reads as a giver-visible
// non-play react (t4), so no reactive clue to wb67 works; the expected
// clue is the stable red to yagami colour-pushing the playable n3.
TEST(BadReactiveTarget1916888, T30CluesRedToYagamiNotWb67) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < kActions.size(); ++i) apply_orig_action(g, kActions[i], ctx);

  PerformAction perform = g.take_action();

  bool is_buggy_red_wb67 = std::holds_alternative<PerformColour>(perform) &&
                           std::get<PerformColour>(perform).target == 2 &&
                           std::get<PerformColour>(perform).value == 0;
  EXPECT_FALSE(is_buggy_red_wb67)
      << "red to wb67 pairs yagami's n3 with the already-known trash 1 "
         "— the eval must reject it";

  bool is_red_to_yagami = std::holds_alternative<PerformColour>(perform) &&
                          std::get<PerformColour>(perform).target == 1 &&
                          std::get<PerformColour>(perform).value == 0;
  EXPECT_TRUE(is_red_to_yagami)
      << "expected red -> yagami (stable ref_play pushing the n3)";
}
