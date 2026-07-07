// Regression test for replay 1916813 T22 (Ambiguous & Dark Null (5 Suits),
// 3-player). Players: P0=will-bot67 (POV), P1=yagami_black, P2=will-bot69.
// Category: test_bad_reactive_target.
//
// Reactive colour dc-target rule (user-specified):
//   1.1 target the LEFTMOST CLUED trash card that is not already globally
//       known to be trash;
//   1.2 else the LEFTMOST UNCLUED (not-known) trash card;
//   2.1 else the LEFTMOST globally-known trash card;
//   2.2 else sacrifice.
//
// At T22, wb69 holds the clued-but-unknown trash n2 on slot 5 (o12, navy
// stack already 4; empathy {g2,g5,b2,b5} — not known trash) and the
// unclued trash m1 on slot 1 (o28). The pre-fix pool sorted UNCLUED trash
// first, so the bot's model paired the o28 target with yagami's playable
// u2 under BLUE (focus 3) and clued blue. The human reacter, following
// the clued-first convention, read blue as target slot 5 → react slot 3
// and blind-played the non-playable b1 — a strike caused by the wrong
// clue. Under the clued-first rule, RED (focus 2) is the clue that pairs
// the o12 target with yagami's playable u2 (react slot 2).

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
// 1916813.json). Suits: Tomato(0) Mahogany(1) Berry(2) Navy(3) Dark Null(4).
const std::vector<std::pair<int, int>> kDeck = {
    {0,1}, {0,4}, {2,1}, {1,2}, {1,5},  // 0-4: t1 t4 b1 m2 m5
    {0,2}, {3,1}, {2,1}, {3,2}, {0,3},  // 5-9: t2 n1 b1 n2 t3
    {3,1}, {4,1}, {3,2}, {1,4}, {1,1},  // 10-14: n1 u1 n2 m4 m1
    {0,1}, {2,2}, {4,5}, {4,4}, {3,4},  // 15-19: t1 b2 u5 u4 n4
    {2,1}, {2,4}, {0,2}, {3,3}, {1,1},  // 20-24: b1 b4 t2 n3 m1
    {4,2}, {0,5}, {4,3}, {1,1}, {2,3},  // 25-29: u2 t5 u3 m1 b3
    {1,4}, {2,5}, {0,1}, {1,2}, {1,3},  // 30-34: m4 b5 t1 m2 m3
    {0,3}, {3,4}, {2,3}, {3,1}, {0,4},  // 35-39: t3 n4 b3 n1 t4
    {2,2}, {3,5}, {1,3}, {2,4}, {3,3},  // 40-44: b2 n5 m3 b4 n3
};

// Actions T1..T21 (T22 is the decision under test).
const std::vector<OrigAction> kActions = {
    {3,2,1},   // T1  P0 wb67  rank-1 -> P2 wb69
    {0,6,0},   // T2  P1 yag   plays ord 6  = n1
    {3,1,1},   // T3  P2 wb69  rank-1 -> P1 yagami
    {0,2,0},   // T4  P0 wb67  plays ord 2  = b1
    {0,15,0},  // T5  P1 yag   plays ord 15 = t1
    {3,1,1},   // T6  P2 wb69  rank-1 -> P1 yagami
    {0,16,0},  // T7  P0 wb67  plays ord 16 = b2
    {3,0,4},   // T8  P1 yag   rank-4 -> P0 wb67
    {0,14,0},  // T9  P2 wb69  plays ord 14 = m1
    {3,1,1},   // T10 P0 wb67  rank-1 -> P1 yagami
    {0,8,0},   // T11 P1 yag   plays ord 8  = n2
    {0,11,0},  // T12 P2 wb69  plays ord 11 = u1
    {1,0,0},   // T13 P0 wb67  discards ord 0 = t1
    {0,5,0},   // T14 P1 yag   plays ord 5  = t2
    {1,10,0},  // T15 P2 wb69  discards ord 10 = n1
    {2,2,1},   // T16 P0 wb67  colour-1 (Blue) -> P2 wb69
    {0,23,0},  // T17 P1 yag   plays ord 23 = n3
    {1,24,0},  // T18 P2 wb69  discards ord 24 = m1
    {3,2,4},   // T19 P0 wb67  rank-4 -> P2 wb69
    {0,9,0},   // T20 P1 yag   plays ord 9  = t3
    {0,19,0},  // T21 P2 wb69  plays ord 19 = n4
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // POV = will-bot67 (orig P0) -> my P0; the turn cycle is already aligned
  // (orig P0 starts), so all player/order mappings are the identity.
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
  // my P0 = will-bot67 (POV, hidden). orig orders [4,3,2,1,0]:
  //   m5, m2, b1, t4, t1.
  // my P1 = yagami (orig P1). orig orders [9,8,7,6,5]:
  //   (0,3)=t3, (3,2)=n2, (2,1)=b1, (3,1)=n1, (0,2)=t2.
  // my P2 = will-bot69 (orig P2). orig orders [14,13,12,11,10]:
  //   (1,1)=m1, (1,4)=m4, (3,2)=n2, (4,1)=u1, (3,1)=n1.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"t3", "n2", "b1", "n1", "t2"},
      {"m1", "m4", "n2", "u1", "n1"},
  };
  opts.variant_name = "Ambiguous & Dark Null (5 Suits)";
  opts.starting = TestPlayer::ALICE;  // orig P0 (will-bot67) starts.
  return setup(std::move(opts));
}

}  // namespace

// Interp level: the RED reactive to wb69 (focus slot 2) must target the
// clued-unknown trash n2 (o12, wb69 slot 5), mapping react_slot 2 =
// yagami's playable u2 (o25). After yagami's u2 reaction, wb69's o12
// flips to CALLED_TO_DISCARD.
TEST(BadReactiveTarget1916813, RedReactiveTargetsCluedUnknownTrash) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < kActions.size(); ++i) apply_orig_action(g, kActions[i], ctx);

  // T22: apply the EXPECTED red clue (colour value 0) to wb69.
  apply_orig_action(g, OrigAction{2, 2, 0}, ctx);

  EXPECT_EQ(g.meta[25].status, CardStatus::CALLED_TO_PLAY)
      << "red (focus 2) + dc-target o12 (slot 5, clued-unknown trash) must "
         "call yagami's slot 2 (o25 = u2, playable) to react";

  // T23: yagami reacts by playing u2 (o25). The reaction decode marks the
  // receiver's dc-target — wb69's o12 — as CALLED_TO_DISCARD.
  apply_orig_action(g, OrigAction{0, 25, 0}, ctx);

  EXPECT_EQ(g.meta[12].status, CardStatus::CALLED_TO_DISCARD)
      << "after the u2 reaction, wb69's slot 5 (o12, clued n2 trash) must "
         "be the called discard target";
}

// Decision level: at T22 wb67 must give one of the two convention-correct
// clues — red to wb69 (reactive: u2 react + o12 discard) or blue to
// yagami (stable ref_play pushing u2) — and NOT the buggy blue to wb69
// (whose clued-first reading maps yagami's react slot 3 = b1, a strike).
TEST(BadReactiveTarget1916813, T22DoesNotClueBlueToWb69) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < kActions.size(); ++i) apply_orig_action(g, kActions[i], ctx);

  PerformAction perform = g.take_action();

  bool is_buggy_blue = std::holds_alternative<PerformColour>(perform) &&
                       std::get<PerformColour>(perform).target == 2 &&
                       std::get<PerformColour>(perform).value == 1;
  EXPECT_FALSE(is_buggy_blue)
      << "blue to wb69 reads (clued-first) as react slot 3 = b1, a strike "
         "— the eval must reject it";

  bool is_red_to_wb69 = std::holds_alternative<PerformColour>(perform) &&
                        std::get<PerformColour>(perform).target == 2 &&
                        std::get<PerformColour>(perform).value == 0;
  bool is_blue_to_yagami = std::holds_alternative<PerformColour>(perform) &&
                           std::get<PerformColour>(perform).target == 1 &&
                           std::get<PerformColour>(perform).value == 1;
  EXPECT_TRUE(is_red_to_wb69 || is_blue_to_yagami)
      << "expected red -> wb69 (reactive u2 + o12 discard) or blue -> "
         "yagami (stable ref_play pushing u2)";
}
