// Hanab.live replay 1890204 — regression for v0.22 Stage A
// (receiver-target inferred narrowing) + Stage B (chain-consistency
// guard in target_play's connector narrowing).
//
// Variant: Funnels & Dark Pink (6 Suits). Players: P0=will-bot67,
// P1=yagami_black, P2=will-bot69. Our POV = will-bot67 (P0).
//
// Pre-v0.22 desync: at T7 will-bot67 clues rank-4 to yagami. The
// convention picked yagami's slot 2 (b1) as the reactive play target
// and mapped via calc_slot to will-bot69's slot 2 (b2). target_play's
// connector narrowing locked yagami's slot 1 to {b1} as a chain prereq,
// even though the giver could see yagami's slot 1 is actually y2 (not
// b1). The bad chain committed; at T9 will-bot69 played slot 2
// expecting one of {r2,r3,y2,y3,g2,b2,p2,i2} to be currently playable
// but b2 wasn't (yagami at T8 played y2 not b1), → strike.
//
// Post-v0.22: target_play bails when conn_order's visible deck id
// contradicts the chain. T7 then doesn't commit the bad CTP on
// will-bot69's slot 2.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// hanab.live deck for replay 1890204.
const std::vector<std::pair<int, int>> kDeck = {
    {5,4}, {3,4}, {1,3}, {0,1}, {0,3},
    {4,1}, {1,2}, {3,2}, {1,1}, {3,1},
    {0,4}, {2,4}, {5,3}, {0,1}, {3,2},
    {1,2}, {5,1}, {3,1},
};

const std::vector<OrigAction> kActions = {
    {3, 2, 4},  // T1 P0 rank-4 → P2 (will-bot69)
    {0, 8, 0},  // T2 P1 plays order 8 (y1)
    {3, 1, 3},  // T3 P2 rank-3 → P1
    {0, 3, 0},  // T4 P0 plays order 3 (r1)
    {2, 0, 1},  // T5 P1 colour-y → P0
    {1,13, 0},  // T6 P2 discards order 13 (r1)
    {3, 1, 4},  // T7 P0 rank-4 → P1 (the suspect clue)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
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
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      // P1 = yagami_black: orig orders [9,8,7,6,5] slot-1-first.
      {"b1", "y1", "b2", "y2", "p1"},
      // P2 = will-bot69: orig orders [14,13,12,11,10] slot-1-first.
      {"b2", "r1", "i3", "g4", "r4"},
  };
  opts.variant_name = "Funnels & Dark Pink (6 Suits)";
  opts.starting = TestPlayer::ALICE;
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kActions[i], ctx);
}

}  // namespace

// Stage B regression: the convention at T7 must NOT commit a CTP on
// will-bot69's slot 2 (order 14, actual b2). The chain-consistency
// guard rejects the bad reactive interp because yagami's slot 1 (the
// would-be connector) is y2 (visible), not b1 (what the chain needed).
TEST(EndgameReplay1890204, T7DoesNotCTPWillbot69Slot2) {
  Game g = build_start();
  apply_prefix(g, 7);  // T1..T7 applied.

  // Will-bot69's slot 2 = order 14 = b2 (actual).
  int wb69_slot2 = g.state.hands[2][1];
  ASSERT_EQ(wb69_slot2, 14);
  EXPECT_NE(g.meta[wb69_slot2].status, CardStatus::CALLED_TO_PLAY)
      << "Stage B: target_play's chain-consistency guard must reject the "
         "bad reactive interp at T7. Pre-fix, will-bot69's slot 2 (b2) "
         "got CTP'd via an inconsistent chain (yagami slot 1 narrowed to "
         "{b1} but actually y2), leading to the T9 strike.";
}

// Stage A regression: when the reactive interp DOES commit a CTP on a
// receiver target, the target's inferred is narrowed (no longer the
// wide post-basic-clue-elim empathy). This is the same y2 narrowing
// the user surfaced for T3 — pinned here via the post-T3 state.
TEST(EndgameReplay1890204, T3ReceiverTargetInferredIsNarrow) {
  Game g = build_start();
  apply_prefix(g, 3);  // T1..T3 applied.

  // Yagami's slot 1 = order 15 = y2 (CTP'd at T3 as reactive receiver).
  int yagami_slot1 = g.state.hands[1][0];
  ASSERT_EQ(yagami_slot1, 15);
  ASSERT_EQ(g.meta[yagami_slot1].status, CardStatus::CALLED_TO_PLAY)
      << "T3 should CTP yagami's slot 1 via reactive";

  const auto& inferred = g.common.thoughts[yagami_slot1].inferred;
  // Pre-Stage A: inferred had ~18 ids (post-basic-clue narrow by rank
  // ≤ 3). Post-Stage A: narrowed to (playable_set ∪ next-ranks-of-
  // reacter-inferred). Rank-3+ ids of all-zero stacks must not survive.
  // Stacks at end of T3: r=0, y=1, g=0, b=0, p=0, i=0 — playable_set =
  // {r1,y2,g1,b1,p1,i1}, max reachable via one reaction = rank 3.
  for (int suit = 0; suit < 6; ++suit) {
    for (int rank = 4; rank <= 5; ++rank) {
      EXPECT_FALSE(inferred.contains(Identity{suit, rank}))
          << "narrowed inferred must not contain rank-" << rank
          << " ids; suit=" << suit;
    }
  }
  // The actual id (y2) must still be in the narrowed set.
  EXPECT_TRUE(inferred.contains(Identity{1, 2}))
      << "narrowed inferred must keep the actual identity y2";
}
