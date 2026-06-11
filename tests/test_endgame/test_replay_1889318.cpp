// Hanab.live replay 1889318, variant "Funnels & Dark Pink (6 Suits)".
// Orig players: P0=will-bot67, P1=will-bot69, P2=yagami_black.
//
// T58 (P0=bot67): colour-red → P1=bot69. Via interpret_stable's ref_play
// this calls bot69's slot 1 (= y3#51, unclued, actually playable on y
// stack=2) to play.
//
// T59 (P1=bot69's turn): bot69 has a CTP'd pending play AND no clue
// would create a play in another player's hand AND yagami's chop (=
// y1, trash) is safe to discard. Per the user's rule, bot69 must PLAY
// rather than CLUE.
//
// Live behaviour (pre-fix): bot69 picked rank-1 → bot67 instead of
// playing the y3. Post-fix the take_action force-play override
// returns PerformPlay{y3#51}.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

const std::vector<std::pair<int, int>> kDeck = []() {
  // Parsed straight from 1889318.json's deck array.
  return std::vector<std::pair<int, int>>{
      {1, 1}, {3, 1}, {0, 3}, {2, 1}, {3, 1}, {0, 1}, {4, 1}, {3, 3}, {4, 4}, {4, 2},
      {3, 4}, {0, 5}, {3, 5}, {4, 5}, {4, 4}, {3, 4}, {1, 3}, {1, 5}, {1, 4}, {2, 4},
      {2, 4}, {3, 3}, {4, 2}, {2, 2}, {2, 2}, {1, 2}, {4, 3}, {2, 3}, {1, 4}, {2, 5},
      {0, 1}, {1, 1}, {0, 2}, {2, 2}, {0, 4}, {2, 3}, {2, 1}, {0, 4}, {0, 2}, {3, 2},
      {3, 2}, {1, 5}, {0, 2}, {3, 4}, {4, 3}, {0, 4}, {0, 1}, {1, 3}, {1, 1}, {2, 1},
      {1, 3}, {1, 4}, {5, 5}, {3, 5}, {2, 1},
  };
}();

const std::vector<OrigAction> kOrigActions = {
    {2, 1, 0}, {0, 6, 0}, {3, 1, 3}, {0, 4, 0}, {0, 9, 0},
    {3, 1, 1}, {0, 16, 0}, {0, 5, 0}, {3, 1, 1}, {0, 0, 0},
    {0, 19, 0}, {2, 1, 4}, {1, 18, 0}, {2, 0, 4}, {1, 10, 0},
    {0, 22, 0}, {0, 8, 0}, {2, 1, 4}, {1, 3, 0}, {2, 0, 2},
    {0, 13, 0}, {2, 2, 1}, {0, 25, 0}, {0, 27, 0}, {1, 1, 0},
    {3, 0, 3}, {0, 29, 0}, {0, 26, 0}, {2, 0, 3}, {0, 11, 0},
    {1, 30, 0}, {3, 0, 2}, {0, 33, 0}, {2, 2, 1}, {0, 7, 0},
    {1, 23, 0}, {0, 24, 0}, {2, 0, 0}, {1, 37, 0}, {2, 2, 2},
    {1, 21, 0}, {0, 12, 0}, {0, 34, 0}, {2, 0, 2}, {1, 35, 0},
    {0, 42, 0}, {1, 15, 0}, {0, 31, 0}, {2, 2, 4}, {0, 28, 0},
    {1, 46, 0}, {0, 20, 0}, {1, 47, 0}, {3, 0, 4}, {2, 2, 3},
    {0, 50, 0}, {2, 0, 2}, {2, 1, 0},
};

// Build from will-bot69's perspective (orig P1 = ALICE = observer).
// Player cycle: orig P0 → P1 → P2 → P0. observer = orig P1. MY BOB =
// next in cycle = orig P2 (yagami). MY CATHY = orig P0 (bot67).
// Starting = orig P0 = MY CATHY.
Game build_from_bot69_perspective() {
  SetupOptions opts;
  opts.hands = {
      // ALICE = bot69. orig 5..9 → MY 0..4, newest-first.
      // 9=(4,2)=p2, 8=(4,4)=p4, 7=(3,3)=b3, 6=(4,1)=p1, 5=(0,1)=r1.
      {"p2", "p4", "b3", "p1", "r1"},
      // BOB = yagami. orig 10..14 → MY 5..9, newest-first.
      // 14=(4,4)=p4, 13=(4,5)=p5, 12=(3,5)=b5, 11=(0,5)=r5, 10=(3,4)=b4.
      {"p4", "p5", "b5", "r5", "b4"},
      // CATHY = bot67. orig 0..4 → MY 10..14, newest-first.
      // 4=(3,1)=b1, 3=(2,1)=g1, 2=(0,3)=r3, 1=(3,1)=b1, 0=(1,1)=y1.
      {"b1", "g1", "r3", "b1", "y1"},
  };
  opts.variant_name = "Funnels & Dark Pink (6 Suits)";
  opts.starting = TestPlayer::CATHY;
  return setup(std::move(opts));
}

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  ctx.orig_to_my_player = {2, 0, 1};
  ctx.orig_to_my_order.resize(kDeck.size());
  for (int o = 0; o <= 4; ++o) ctx.orig_to_my_order[o] = o + 10;
  for (int o = 5; o <= 9; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 10; o <= 14; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 15; o < static_cast<int>(kDeck.size()); ++o)
    ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(kDeck.size());
  for (size_t orig_o = 0; orig_o < kDeck.size(); ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

}  // namespace

TEST(EndgameReplay1889318, Turn59PlaysY3InsteadOfCluing) {
  Game g = build_from_bot69_perspective();
  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  // y3 = orig 51 (drawn by bot69 mid-game). orig 15+ maps identity, so
  // MY order 51 = y3.
  const int y3_order = 51;

  PerformAction perform = g.take_action();
  EXPECT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "force-play override must select the pending play y3 instead "
         "of any clue (no clue creates a play here; yagami's chop is "
         "safe trash)";
  if (std::holds_alternative<PerformPlay>(perform)) {
    EXPECT_EQ(std::get<PerformPlay>(perform).target, y3_order)
        << "expected PerformPlay{y3 = MY order 51}";
  }
}
