// Regression test for replay 1916933 T31/T32 (Ambiguous & Dark Null
// (5 Suits), 3-player). Players: orig P0=will-bot69, P1=yagami_black,
// P2=will-bot67. Category: test_decision_making.
//
// wb69's red clue to wb67 reads (correctly, per the convention) as a
// reactive whose leftmost play target is wb67's slot-1 m4; the react
// mapping lands on yagami's slot 4 — the QUEUED, CALLED_TO_PLAY t3 —
// and a colour play-target calls the reacter to DISCARD it. The read
// itself is right (a loaded player is expected to play their queue, so
// a discard is the reaction signal); the bug was the GIVER's eval,
// which scored the clue like a normal 1-play reactive and never charged
// it for destroying the queued t3 (and the t4/t5 chain behind it).
// get_result now subtracts 10 per existing CTP flipped to CTD by the
// clue's reading (unless the card is giver-visibly trash), so wb69
// picks one of the sound alternatives instead: the stable blue push to
// wb67, or rank 4/5 to yagami stacking the slot-5 t4 on the queued t3.

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
// 1916933.json). Suits: Tomato(0) Mahogany(1) Berry(2) Navy(3) Dark Null(4).
const std::vector<std::pair<int, int>> kDeck = {
    {0,2}, {1,4}, {4,3}, {1,1}, {4,1},  // 0-4: t2 m4 u3 m1 u1
    {0,4}, {3,1}, {2,1}, {0,3}, {3,2},  // 5-9: t4 n1 b1 t3 n2
    {1,1}, {3,1}, {2,4}, {4,2}, {0,1},  // 10-14: m1 n1 b4 u2 t1
    {3,3}, {0,1}, {1,1}, {3,2}, {2,4},  // 15-19: n3 t1 m1 n2 b4
    {2,5}, {1,3}, {0,5}, {4,5}, {1,2},  // 20-24: b5 m3 t5 u5 m2
    {1,5}, {2,3}, {4,4}, {2,2}, {0,4},  // 25-29: m5 b3 u4 b2 t4
    {3,4}, {2,1}, {3,3}, {1,4}, {1,2},  // 30-34: n4 b1 n3 m4 m2
    {0,3}, {3,1}, {1,3}, {2,2}, {3,5},  // 35-39: t3 n1 m3 b2 n5
    {0,1}, {3,4}, {0,2}, {2,1}, {2,3},  // 40-44: t1 n4 t2 b1 b3
};

// Actions T1..T30 (T31 is the decision under test).
const std::vector<OrigAction> kActions = {
    {3,2,1}, {0,7,0},  {3,1,4}, {0,4,0},  {0,6,0},  {3,1,3},
    {0,16,0},{0,17,0}, {3,1,2}, {0,0,0},  {0,9,0},  {3,0,5},
    {3,1,3}, {1,19,0}, {2,1,1}, {1,1,0},  {3,0,5},  {0,13,0},
    {0,2,0}, {2,0,0},  {1,14,0},{3,1,5},  {0,15,0}, {0,24,0},
    {3,2,3}, {0,27,0}, {0,28,0},{1,18,0}, {0,21,0}, {1,11,0},
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // POV = will-bot69 (orig P0, the T31 decider) -> my P0; identity map.
  ctx.orig_to_my_player = {0, 1, 2};
  const int N = static_cast<int>(kDeck.size());
  ctx.orig_to_my_order.resize(N);
  ctx.my_order_to_id.resize(N);
  for (int o = 0; o < N; ++o) ctx.orig_to_my_order[o] = o;
  for (int o = 0; o < N; ++o) ctx.my_order_to_id[o] = kDeck[o];
  return ctx;
}

Game build_start() {
  SetupOptions opts;
  // my P0 = will-bot69 (POV, hidden). orig orders [4,3,2,1,0]:
  //   u1, m1, u3, m4, t2.
  // my P1 = yagami (orig P1). orig orders [9,8,7,6,5]:
  //   (3,2)=n2, (0,3)=t3, (2,1)=b1, (3,1)=n1, (0,4)=t4.
  // my P2 = will-bot67 (orig P2). orig orders [14,13,12,11,10]:
  //   (0,1)=t1, (4,2)=u2, (2,4)=b4, (3,1)=n1, (1,1)=m1.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"n2", "t3", "b1", "n1", "t4"},
      {"t1", "u2", "b4", "n1", "m1"},
  };
  opts.variant_name = "Ambiguous & Dark Null (5 Suits)";
  opts.starting = TestPlayer::ALICE;  // orig P0 (will-bot69) starts.
  return setup(std::move(opts));
}

}  // namespace

// Interp level: the red-to-wb67 READING is convention-correct and must
// stay — the leftmost play target (wb67 slot-1 m4) maps the reaction
// onto yagami's queued t3 (o8), which a colour play-target calls to
// DISCARD. The giver's job is to avoid the clue, not to reread it.
TEST(DecisionMaking1916933, RedReadingCallsQueuedT3ToDiscard) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < kActions.size(); ++i) apply_orig_action(g, kActions[i], ctx);

  ASSERT_EQ(g.meta[8].status, CardStatus::CALLED_TO_PLAY)
      << "test premise: yagami's t3 (o8) is queued pre-clue";

  apply_orig_action(g, OrigAction{2, 2, 0}, ctx);

  EXPECT_EQ(g.meta[8].status, CardStatus::CALLED_TO_DISCARD)
      << "the red reactive calls the reacter to discard the queued t3 — "
         "that is the convention's reading of this (bad) clue";
  EXPECT_EQ(g.meta[33].status, CardStatus::CALLED_TO_PLAY)
      << "wb67's slot-1 m4 is the reactive's play target";
}

// Decision level: the eval must charge the red clue for destroying the
// queued t3 and pick a sound alternative — the stable blue push to
// wb67, or rank 4/5 to yagami stacking the slot-5 t4 on the queued t3.
TEST(DecisionMaking1916933, T31AvoidsClueThatDiscardsQueuedPlayable) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < kActions.size(); ++i) apply_orig_action(g, kActions[i], ctx);

  PerformAction perform = g.take_action();

  bool is_buggy_red = std::holds_alternative<PerformColour>(perform) &&
                      std::get<PerformColour>(perform).target == 2 &&
                      std::get<PerformColour>(perform).value == 0;
  EXPECT_FALSE(is_buggy_red)
      << "red to wb67 trades away the queued unique t3 (and the t4/t5 "
         "chain) for a single CTP — the eval must reject it";

  bool is_blue_to_wb67 = std::holds_alternative<PerformColour>(perform) &&
                         std::get<PerformColour>(perform).target == 2 &&
                         std::get<PerformColour>(perform).value == 1;
  bool is_rank45_to_yagami = std::holds_alternative<PerformRank>(perform) &&
                             std::get<PerformRank>(perform).target == 1 &&
                             (std::get<PerformRank>(perform).value == 4 ||
                              std::get<PerformRank>(perform).value == 5);
  EXPECT_TRUE(is_blue_to_wb67 || is_rank45_to_yagami)
      << "expected blue -> wb67 (stable push) or rank 4/5 -> yagami "
         "(reverse reactive stacking t4 on the queued t3)";
}
