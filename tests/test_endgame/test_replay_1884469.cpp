// Hanab.live replay 1884469 turn 5 (action index 4). Variant "Orange
// (3 Suits)". Players: orig P0=yagami_black, P1=will-bot69 (observer),
// P2=will-bot67.
//
// At turn 5 the user expects will-bot69 to clue orange (colour value 2)
// to yagami_black — a clean reactive that focuses yagami's slot 2 (o2,
// currently playable on orange=1) and pairs it with reacter
// will-bot67's slot 5 (b2, currently playable on blue=1). The bot
// actually chose rank-5 → will-bot67. This probe inspects what
// take_action returns at that turn so we can see why the orange clue
// isn't picked.
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
    {0, 3}, {1, 1}, {2, 1}, {2, 2}, {2, 3},
    {0, 2}, {1, 1}, {1, 5}, {2, 1}, {1, 4},
    {2, 1}, {1, 2}, {0, 3}, {0, 5}, {2, 5},
    {1, 3}, {0, 4}, {0, 1}, {0, 1}, {0, 4},
    {1, 4}, {0, 1}, {2, 4}, {2, 4}, {0, 2},
    {2, 3}, {1, 1}, {1, 2}, {1, 3}, {2, 2},
};

const std::vector<OrigAction> kOrigActions = {
    {2, 2, 1},   // 0: P0 yagami colour-1 (blue) to P2 will-bot67
    {0, 6, 0},   // 1: P1 will-bot69 plays order 6 (b1)
    {0, 10, 0},  // 2: P2 will-bot67 plays order 10 (o1; outcome=advance)
    {3, 1, 2},   // 3: P0 yagami rank-2 to P1 will-bot69
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // ALICE = will-bot69 (orig P1, observer), BOB = will-bot67 (orig P2),
  // CATHY = yagami (orig P0). starting=CATHY.
  ctx.orig_to_my_player = {2, 0, 1};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 5; ++o) ctx.orig_to_my_order[o] = 10 + o;   // P0 → CATHY
  for (int o = 5; o < 10; ++o) ctx.orig_to_my_order[o] = o - 5;   // P1 → ALICE
  for (int o = 10; o < 15; ++o) ctx.orig_to_my_order[o] = o - 5;  // P2 → BOB
  for (int o = 15; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int orig_o = 0; orig_o < 30; ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

Game build_through_action_3() {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice = will-bot69 (observer)
      // Bob = will-bot67: orig [14,13,12,11,10] = [o5, r5, r3, b2, o1].
      {"o5", "r5", "r3", "b2", "o1"},
      // Cathy = yagami: orig [4,3,2,1,0] = [o3, o2, o1, b1, r3].
      {"o3", "o2", "o1", "b1", "r3"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::CATHY;  // orig P0 starts
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

// At turn 5 will-bot69 should pick the clean reactive: colour orange
// (value 2) to yagami_black, which CTPs will-bot67's slot 5 (b2,
// currently playable on blue=1) and queues yagami to play the focused
// orange-2 by PerformDiscard (orange game-rule = stack advance) on the
// receiver-side resolution.
//
// Before the advance() failed-flag fix, advance() simulated CATHY's
// fallback "discard chop" with failed=false even when the chop was a
// non-playable orange (o5). The engine's `on_discard(inverted,
// failed=false)` then called `with_play` and skipped the orange stack
// straight to 5 — every rank-clue's advance() got that artificial
// bonus, so rank-5 to will-bot67 outscored the real reactive.
TEST(EndgameReplay1884469, WillBot69PicksReactiveOrangeClueToYagami) {
  Game g = build_through_action_3();

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  ASSERT_EQ(g.state.play_stacks[0], 0);
  ASSERT_EQ(g.state.play_stacks[1], 1);
  ASSERT_EQ(g.state.play_stacks[2], 1);

  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformColour>(perform))
      << "expected colour clue, got something else";
  const auto& cl = std::get<PerformColour>(perform);
  EXPECT_EQ(cl.value, 2)
      << "expected orange (colour value 2) clue, got colour " << cl.value;
  EXPECT_EQ(cl.target, static_cast<int>(TestPlayer::CATHY))
      << "expected target = yagami (CATHY), got P" << cl.target;
}
