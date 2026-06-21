// Regression for replay 1900437 T30. Variant: Ambiguous & Gray Pink
// (5 Suits) — pinkish-rank focus rule applies, so `reactive_focus`
// returns `clue.value = 4` (not the newest-touched slot 2). With
// focus=4, the rank-4 clue from yagami to bot69 maps target=slot 4
// (b4, currently playable) to react_slot = calc_slot(4,4,5) = 5 →
// bot67's slot 5 (m4 actual, also currently playable). The bot must
// interpret this as a b4 reactive double-play: bot69 plays b4, bot67
// plays m4.
//
// Pre-v0.37 the convention's good-touch auto-advance through the
// receiver's apparent natural plays would have made b4's playability
// gate evaluate to false in `ctx.hypo_state`; with the v0.37 strict
// self_plays fix this no longer happens, and the convention picks
// the correct b4 reactive instead of falling to a misplay-inducing
// m5 or t4 finesse.
//
// Players: P0=will-bot67 (POV), P1=will-bot69, P2=yagami_black.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// hanab.live deck for replay 1900437.
const std::vector<std::pair<int, int>> kDeck = {
    {1,4}, {1,1}, {3,3}, {3,2}, {3,2}, {0,4}, {4,1}, {1,2}, {0,5}, {0,2},
    {2,3}, {3,1}, {3,3}, {2,1}, {0,1}, {1,1}, {3,1}, {2,4}, {2,1}, {3,4},
    {2,3}, {2,5}, {2,2}, {0,2}, {3,1}, {4,3}, {2,1}, {4,2}, {0,1}, {1,5},
    {1,3}, {0,4}, {1,3}, {2,2}, {4,5}, {0,3}, {3,4}, {0,1}, {4,4}, {2,4},
    {0,3}, {1,2}, {3,5}, {1,1}, {1,4},
};

// First 29 actions (T1..T29). T30 is the clue under investigation;
// applied separately so we can inspect both pre- and post-clue state.
const std::vector<OrigAction> kPrefix = {
    {2,2,0},  // T1  P0 colour-0 → P2
    {1,5,0},  // T2  P1 discards 5
    {0,14,0}, // T3  P2 plays 14
    {3,2,3},  // T4  P0 rank-3 → P2
    {0,15,0}, // T5  P1 plays 15
    {0,13,0}, // T6  P2 plays 13
    {3,2,3},  // T7  P0 rank-3 → P2
    {0,7,0},  // T8  P1 plays 7
    {0,11,0}, // T9  P2 plays 11
    {1,4,0},  // T10 P0 discards 4
    {2,0,1},  // T11 P1 colour-1 → P0
    {3,1,5},  // T12 P2 rank-5 → P1
    {0,3,0},  // T13 P0 plays 3
    {3,0,5},  // T14 P1 rank-5 → P0
    {0,12,0}, // T15 P2 plays 12
    {0,22,0}, // T16 P0 plays 22
    {3,2,2},  // T17 P1 rank-2 → P2
    {0,10,0}, // T18 P2 plays 10
    {1,24,0}, // T19 P0 discards 24
    {0,9,0},  // T20 P1 plays 9
    {1,23,0}, // T21 P2 discards 23
    {2,2,0},  // T22 P0 colour-0 → P2
    {0,6,0},  // T23 P1 plays 6
    {2,1,0},  // T24 P2 colour-0 → P1
    {1,2,0},  // T25 P0 discards 2
    {0,27,0}, // T26 P1 plays 27
    {3,1,4},  // T27 P2 rank-4 → P1 (first 4-clue)
    {0,30,0}, // T28 P0 plays 30
    {0,19,0}, // T29 P1 plays 19
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Our POV is P0 = will-bot67 (matches orig P0). No rotation needed.
  ctx.orig_to_my_player = {0, 1, 2};
  const int N = static_cast<int>(kDeck.size());
  ctx.orig_to_my_order.resize(N);
  ctx.my_order_to_id.resize(N);
  for (int o = 0; o < N; ++o) {
    ctx.orig_to_my_order[o] = o;
    ctx.my_order_to_id[o] = kDeck[o];
  }
  return ctx;
}

Game build_start() {
  SetupOptions opts;
  // Initial deal: orders 0-4 → P0 (slot 5..1), 5-9 → P1, 10-14 → P2.
  opts.hands = {
      // P0 = will-bot67 (POV, hidden).
      {"xx", "xx", "xx", "xx", "xx"},
      // P1 = will-bot69. orig orders [9,8,7,6,5] slot 1..5.
      // (0,2)=t2, (0,5)=t5, (1,2)=m2, (4,1)=i1, (0,4)=t4.
      {"t2", "t5", "m2", "i1", "t4"},
      // P2 = yagami_black. orig orders [14,13,12,11,10] slot 1..5.
      // (0,1)=t1, (2,1)=b1, (3,3)=n3, (3,1)=n1, (2,3)=b3.
      {"t1", "b1", "n3", "n1", "b3"},
  };
  opts.variant_name = "Ambiguous & Gray Pink (5 Suits)";
  opts.starting = TestPlayer::ALICE;  // orig P0 starts.
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kPrefix[i], ctx);
}

}  // namespace

TEST(EndgameReplay1900437, T30CTPsBot69Slot4AndBot67Slot5) {
  Game g = build_start();
  apply_prefix(g, 29);
  ReplayContext ctx = make_ctx();
  apply_orig_action(g, /*T30*/ {3,1,4}, ctx);

  int bot67_slot5 = g.state.hands[0][4];
  int bot69_slot4 = g.state.hands[1][3];
  int bot69_slot3 = g.state.hands[1][2];

  ASSERT_EQ(bot67_slot5, 0)
      << "bot67's slot 5 must be the original m4 (order 0)";
  ASSERT_EQ(bot69_slot4, 17)
      << "bot69's slot 4 must be the b4 drawn at T5 (order 17)";

  EXPECT_EQ(g.meta[bot67_slot5].status, CardStatus::CALLED_TO_PLAY)
      << "T30's b4 reactive must CTP bot67's slot 5 (m4) as the reacter "
         "via calc_slot(focus=4, target=4, 5) = 5";
  EXPECT_TRUE(g.meta[bot67_slot5].urgent)
      << "the reacter slot must be urgent";
  EXPECT_EQ(g.meta[bot69_slot4].status, CardStatus::CALLED_TO_PLAY)
      << "T30's b4 reactive must CTP bot69's slot 4 (b4) as the "
         "receiver target — this is the user-expected direct-playable "
         "double-play, not an m5 finesse";
  EXPECT_NE(g.meta[bot69_slot3].status, CardStatus::CALLED_TO_PLAY)
      << "bot69's slot 3 (m5) must NOT be CTP'd — that would be the "
         "rejected m5 finesse interpretation";
}
