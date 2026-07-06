// Regression test for replay 1915981 (Ambiguous & Null (5 Suits), 3-player).
//
// T12 issue: yagami gives colour-Blue to will-bot69 at T11 (target=cathy
// from yagami's POV; will-bot67 is bob). will-bot67 is LOADED — its slot 2
// b1 was CTP'd by the T1 reactive — and the blue clue is a good stable
// clue: the ref_play reading colour-pushes wb69's slot 1 (t2, directly
// playable on tomato stack = 1). Per the convention, a clue to Cathy while
// Bob is loaded reads STABLE unless the stable interpretation is bad, so
// wb67 should simply play its CTP'd b1 at T12.
//
// The pre-fix dispatcher instead routed the clue to interpret_reactive:
// the reacter-search loop skipped wb67 (loaded, plays kept), suppressed
// the vacuous reacter==target pick for wb69 (v0.39 guard, replay 1899623
// T16), and the `!reacter` fallback went reactive UNCONDITIONALLY. wb67
// then "reacted" by discarding its slot 1 (m3). Fix: the `!reacter`
// fallback routes to interpret_stable, whose bad_stable check falls back
// to reactive only when the stable reading is actually bad (e.g. 1899623
// T16, where the stable rank clue CTD'd a giver-visibly useful card).

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

// 50-card deck from the hanab.live export. Suits: Tomato(0) Mahogany(1)
// Berry(2) Navy(3) Null(4).
const std::vector<std::pair<int, int>> kDeck = {
    {4,1}, {1,2}, {1,5}, {4,4}, {1,1},  // 0-4: u1 m2 m5 u4 m1
    {2,5}, {4,1}, {4,2}, {2,3}, {4,3},  // 5-9: b5 u1 u2 b3 u3
    {0,1}, {1,1}, {2,2}, {2,1}, {1,3},  // 10-14: t1 m1 b2 b1 m3
    {1,4}, {3,4}, {3,3}, {0,1}, {1,4},  // 15-19: m4 n4 n3 t1 m4
    {0,2}, {0,1}, {4,5}, {0,4}, {3,3},  // 20-24: t2 t1 u5 t4 n3
    {2,1}, {2,4}, {4,3}, {4,1}, {3,5},  // 25-29: b1 b4 u3 u1 n5
    {3,1}, {0,2}, {2,1}, {1,2}, {2,4},  // 30-34: n1 t2 b1 m2 b4
    {4,4}, {3,2}, {3,2}, {2,2}, {0,3},  // 35-39: u4 n2 n2 b2 t3
    {3,4}, {1,1}, {1,3}, {2,3}, {3,1},  // 40-44: n4 m1 m3 b3 n1
    {0,5}, {0,3}, {0,4}, {3,1}, {4,2},  // 45-49: t5 t3 t4 n1 u2
};

// Actions T1..T11 (we take T12 ourselves via take_action).
const std::vector<OrigAction> kActions = {
    {3,2,3},   // T1  P0 wb69  rank-3 -> P2 wb67 (reactive: CTPs wb67 slot 2 b1)
    {0,6,0},   // T2  P1 yag   blind-plays ord 6 = u1 (the T1 reaction)
    {3,1,5},   // T3  P2 wb67  rank-5 -> P1 yagami
    {0,4,0},   // T4  P0 wb69  plays ord 4  = m1
    {0,7,0},   // T5  P1 yag   plays ord 7  = u2
    {3,1,4},   // T6  P2 wb67  rank-4 -> P1 yagami
    {0,1,0},   // T7  P0 wb69  plays ord 1  = m2
    {0,9,0},   // T8  P1 yag   plays ord 9  = u3
    {2,1,1},   // T9  P2 wb67  colour-1 (Blue) -> P1 yagami
    {0,18,0},  // T10 P0 wb69  plays ord 18 = t1
    {2,0,1},   // T11 P1 yag   colour-1 (Blue) -> P0 wb69 (= the suspect clue)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // POV = will-bot67 (orig P2) -> my P0. To preserve turn cycle:
  //   orig P2 (will-bot67) -> my P0  (POV)
  //   orig P0 (will-bot69) -> my P1  (one step forward in cycle from POV)
  //   orig P1 (yagami)     -> my P2  (two steps)
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
  // my P0 = will-bot67 (POV, hidden). orig orders [14,13,12,11,10]:
  //   m3, b1, b2, m1, t1.
  // my P1 = will-bot69 (orig P0). orig orders [4,3,2,1,0]:
  //   (1,1)=m1, (4,4)=u4, (1,5)=m5, (1,2)=m2, (4,1)=u1.
  // my P2 = yagami (orig P1). orig orders [9,8,7,6,5]:
  //   (4,3)=u3, (2,3)=b3, (4,2)=u2, (4,1)=u1, (2,5)=b5.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"m1", "u4", "m5", "m2", "u1"},
      {"u3", "b3", "u2", "u1", "b5"},
  };
  opts.variant_name = "Ambiguous & Null (5 Suits)";
  opts.starting = TestPlayer::BOB;  // orig P0 (will-bot69) -> my P1
  return setup(std::move(opts));
}

}  // namespace

// T11 must read STABLE: bob (wb67, us) is loaded via the T1-CTP'd slot 2
// b1, and the blue clue's ref_play colour-pushes wb69's slot 1 (t2,
// playable) — a good stable clue.
TEST(EndgameReplay1915981, T11BlueToCathyWithLoadedBobIsStable) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (int i = 0; i < 11; ++i) apply_orig_action(g, kActions[i], ctx);

  // Sanity: our slot 2 b1 (orig 13 -> my 3) was CTP'd by the T1 reactive
  // and must still be CTP'd after T11.
  EXPECT_EQ(g.meta[3].status, CardStatus::CALLED_TO_PLAY)
      << "wb67 slot 2 (b1) must still be CALLED_TO_PLAY after T11";

  // Stable ref_play: wb69's slot 1 (orig o20 = t2, playable) is the
  // referred play target of the blue clue.
  EXPECT_EQ(g.meta[ctx.orig_to_my_order[20]].status, CardStatus::CALLED_TO_PLAY)
      << "T11 blue must stable-CTP wb69's slot 1 (t2) via ref_play";

  // The waiting connection must be the stable response-inversion shape
  // (inverted=true), not a reactive WC. Pre-fix the dispatcher's
  // `!reacter` fallback produced a reactive WC (inverted=false) with
  // wb67 as reacter.
  ASSERT_FALSE(g.waiting.empty()) << "T11 stable interp must leave a WC";
  EXPECT_TRUE(g.waiting.front().inverted)
      << "T11 WC must be stable/response-inversion shaped (inverted=true). "
         "inverted=false means the dispatcher routed the clue to reactive "
         "— i.e. the bug.";
}

// T12: wb67 must play its CTP'd b1 (my order 3), not react. The buggy
// build reacted to the "reactive" T11 clue by discarding slot 1 (m3).
TEST(EndgameReplay1915981, T12PlaysCalledB1InsteadOfReacting) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (int i = 0; i < 11; ++i) apply_orig_action(g, kActions[i], ctx);

  PerformAction perform = g.take_action();

  bool is_buggy_discard = std::holds_alternative<PerformDiscard>(perform) &&
                          std::get<PerformDiscard>(perform).target == 4;
  EXPECT_FALSE(is_buggy_discard)
      << "bot reacted by discarding slot 1 (m3, my order 4) — the T11 "
         "clue was misread as reactive";

  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "wb67 must PLAY at T12 (its CTP'd b1)";
  EXPECT_EQ(std::get<PerformPlay>(perform).target, 3)
      << "wb67 must play the CTP'd b1 (my order 3, slot 2)";
}
