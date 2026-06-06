// Hanab.live replay 1883183 turn 27. Variant "Pink-Fives & Omni (3 Suits)".
// Players: P0=will-bot67, P1=yagami_black, P2=will-bot69.
//
// At action index 26 will-bot69 (P2) clues rank-2 to will-bot67 (P0). Pre-fix
// the stable interpreter looped only over Identity(s, clue.value) for
// trash_push / playable_rank, so it never noticed that the rank-2 clue also
// touches (R,5) via Pink-Fives. Since (R,2),(B,2),(K,2) are all basic-trash
// in this state, the interpreter collapsed to trash_push and marked
// order 29 (red-5, just drawn) as trash. Downstream good-touch on order 25
// (omni-3) then left (R,5) as its only non-trash inference, so will-bot67
// played omni-3 thinking it was red-5.
//
// Post-fix the interpreter iterates the variant's full touch_possibilities;
// (R,5) is the only non-trash touched id and is_playable, so playable_rank
// triggers and order 29 is CALLED_TO_PLAY with red-5 in its inferred set.
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

// hanab.live export deck. Suits: 0=Red, 1=Blue, 2=Omni. 30 cards.
const std::vector<std::pair<int, int>> kDeck = {
    {1, 1}, {1, 4}, {1, 4}, {2, 2}, {0, 1},
    {0, 1}, {2, 1}, {0, 4}, {0, 2}, {1, 2},
    {0, 3}, {2, 2}, {2, 3}, {0, 4}, {1, 3},
    {2, 1}, {2, 4}, {1, 3}, {2, 1}, {0, 3},
    {1, 1}, {1, 5}, {2, 5}, {1, 1}, {0, 2},
    {2, 3}, {1, 2}, {0, 1}, {2, 4}, {0, 5},
};

// All 27 actions up to and including the rank-2 clue under test (index 26).
const std::vector<OrigAction> kOrigActions = {
    {3, 2, 3},   // 0:  P0 → P2 rank-3
    {0, 6, 0},   // 1:  P1 plays order 6 (o1)
    {0, 11, 0},  // 2:  P2 plays order 11 (o2)
    {3, 2, 4},   // 3:  P0 → P2 rank-4
    {0, 5, 0},   // 4:  P1 plays order 5 (r1)
    {3, 1, 4},   // 5:  P2 → P1 rank-4
    {0, 0, 0},   // 6:  P0 plays order 0 (b1)
    {0, 8, 0},   // 7:  P1 plays order 8 (r2)
    {0, 12, 0},  // 8:  P2 plays order 12 (o3)
    {2, 1, 0},   // 9:  P0 → P1 colour red
    {0, 9, 0},   // 10: P1 plays order 9 (b2)
    {0, 14, 0},  // 11: P2 plays order 14 (b3)
    {1, 18, 0},  // 12: P0 discards order 18 (o1)
    {3, 0, 2},   // 13: P1 → P0 rank-2
    {0, 16, 0},  // 14: P2 plays order 16 (o4)
    {0, 2, 0},   // 15: P0 plays order 2 (b4)
    {2, 0, 0},   // 16: P1 → P0 colour red
    {0, 22, 0},  // 17: P2 plays order 22 (o5)
    {2, 1, 1},   // 18: P0 → P1 colour blue
    {0, 21, 0},  // 19: P1 plays order 21 (b5)
    {3, 0, 1},   // 20: P2 → P0 rank-1
    {3, 1, 1},   // 21: P0 → P1 rank-1
    {0, 19, 0},  // 22: P1 plays order 19 (r3)
    {3, 0, 1},   // 23: P2 → P0 rank-1
    {1, 23, 0},  // 24: P0 discards order 23 (b1)
    {0, 7, 0},   // 25: P1 plays order 7 (r4)
    {3, 0, 2},   // 26: P2 → P0 rank-2  — the bug under test
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Identity mapping: orig P0/1/2 == our P0/1/2 (will-bot67 is the observer
  // and original P0, so no remap needed).
  ctx.orig_to_my_player = {0, 1, 2};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int i = 0; i < 30; ++i) ctx.my_order_to_id[i] = kDeck[i];
  return ctx;
}

}  // namespace

TEST(EndgameReplay1883183, PinkFivesRankTwoCallsRedFiveNotTrashPush) {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot67 (P0, observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = yagami_black (P1): orig orders [9,8,7,6,5] = [b2, r2, r4, o1, r1].
      {"b2", "r2", "r4", "o1", "r1"},
      // Cathy = will-bot69 (P2): orig orders [14,13,12,11,10] = [b3, r4, o3, o2, r3].
      {"b3", "r4", "o3", "o2", "r3"},
  };
  opts.variant_name = "Pink-Fives & Omni (3 Suits)";
  opts.starting = TestPlayer::ALICE;  // orig P0 starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();

  // Apply actions 0..25 (everything before the bad rank-2 clue at index 26).
  for (size_t i = 0; i < 26; ++i) apply_orig_action(g, kOrigActions[i], ctx);

  // Sanity: order 29 (red-5) is the newest card in will-bot67's hand,
  // drawn at action 24 after the b1 discard.
  ASSERT_FALSE(g.state.hands[0].empty());
  EXPECT_EQ(g.state.hands[0].front(), 29)
      << "Expected order 29 (red-5) to be slot 1 in will-bot67's hand";

  // Apply action 26 — the rank-2 clue from will-bot69 to will-bot67.
  apply_orig_action(g, kOrigActions[26], ctx);

  // Post-fix expectations. Pre-fix: trash_push wrongly fires (only rank-2
  // ids were considered), order 29 was marked trash, and downstream
  // good-touch promoted order 25 (omni-3) to a known red-5 misplay.
  EXPECT_FALSE(g.meta[29].trash)
      << "Order 29 (red-5) must not be marked as trash";
  EXPECT_EQ(g.meta[29].status, CardStatus::CALLED_TO_PLAY)
      << "Order 29 (red-5) should be called to play via playable_rank";
  EXPECT_TRUE(g.common.thoughts[29].inferred.contains(Identity{0, 5}))
      << "Order 29's inferred should still include red-5";

  // And take_action should play the just-clued red-5, not the omni-3.
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "Expected a play action on will-bot67's turn after the rank-2 clue";
  EXPECT_EQ(std::get<PerformPlay>(perform).target, 29)
      << "Bot should play order 29 (red-5), not order 25 (omni-3)";
}
