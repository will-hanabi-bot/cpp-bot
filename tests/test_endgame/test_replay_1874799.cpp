// Port of python-bot/tests/test_endgame/test_replay_1874799.py.
//
// hanab.live replay 1874799 turn 23. The bot must stall (clue), not play
// its null-5, otherwise the team ends one point short of max.
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

// Hanab.live export deck (https://hanab.live/export/1874799).
// Suit indices: 0=Red, 1=Blue, 2=Null. 30 cards.
const std::vector<std::pair<int, int>> kDeck = {
    {2, 4}, {1, 3}, {0, 4}, {2, 3}, {0, 4},
    {2, 5}, {0, 3}, {1, 1}, {1, 3}, {1, 1},
    {0, 2}, {0, 5}, {2, 2}, {0, 1}, {2, 1},
    {1, 2}, {2, 3}, {0, 1}, {1, 5}, {0, 2},
    {1, 4}, {0, 3}, {1, 2}, {1, 1}, {2, 1},
    {0, 1}, {2, 1}, {1, 4}, {2, 2}, {2, 4},
};

const std::vector<OrigAction> kOrigActions = {
    {3, 2, 2}, {0, 9, 0}, {0, 14, 0}, {3, 2, 3}, {0, 15, 0},
    {0, 13, 0}, {3, 2, 5}, {0, 8, 0}, {0, 12, 0}, {2, 2, 1},
    {1, 19, 0}, {0, 10, 0}, {3, 2, 2}, {0, 6, 0}, {2, 0, 0},
    {0, 3, 0}, {2, 2, 1}, {0, 20, 0}, {0, 2, 0}, {1, 23, 0},
    {3, 1, 5}, {0, 0, 0},
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 5; ++o) ctx.orig_to_my_order[o] = o + 10;       // bot67
  for (int o = 5; o < 10; ++o) ctx.orig_to_my_order[o] = o - 5;       // bot69
  for (int o = 10; o < 15; ++o) ctx.orig_to_my_order[o] = o - 5;      // yagami
  for (int o = 15; o < 30; ++o) ctx.orig_to_my_order[o] = o;          // later draws
  ctx.orig_to_my_player = {2, 0, 1};                                    // orig P0->P2, P1->P0, P2->P1
  ctx.my_order_to_id.resize(30);
  for (int i = 0; i < 5; ++i) {
    ctx.my_order_to_id[i] = kDeck[5 + i];           // alice (= bot69)
    ctx.my_order_to_id[5 + i] = kDeck[10 + i];       // bob (= yagami)
    ctx.my_order_to_id[10 + i] = kDeck[i];           // cathy (= bot67)
  }
  for (int i = 15; i < 30; ++i) ctx.my_order_to_id[i] = kDeck[i];
  return ctx;
}

}  // namespace

TEST(EndgameReplay1874799, Turn23StallOverPlay) {
  SetupOptions opts;
  opts.hands = {
      // Alice = bot69 (observer): hidden
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = yagami: orig orders [14,13,12,11,10] = (null-1, r1, null-2, r5, r2)
      {"u1", "r1", "u2", "r5", "r2"},
      // Cathy = bot67: orig orders [4,3,2,1,0] = (r4, null-3, r4, b3, null-4)
      {"r4", "u3", "r4", "b3", "u4"},
  };
  opts.variant_name = "Pink-Ones & Null (3 Suits)";
  opts.starting = TestPlayer::CATHY;
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  // Should now be at start of turn 23 (Alice's turn).
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  ASSERT_EQ(g.state.score(), 12);
  ASSERT_EQ(g.state.cards_left, 1);
  EXPECT_EQ(g.state.play_stacks, (std::vector<int>{4, 4, 4}));

  // Sanity: bot69's null-5 is at my-order 0, inferred + CALLED_TO_PLAY.
  int null_5_my_order = ctx.orig_to_my_order[5];  // orig 5 = null-5
  ASSERT_EQ(null_5_my_order, 0);
  auto inferred = g.players[0].thoughts[null_5_my_order].id(/*infer=*/true);
  ASSERT_TRUE(inferred.has_value()) << "bot69 should have inferred slot 5";
  EXPECT_EQ(*inferred, Identity(2, 5));
  EXPECT_EQ(g.meta[null_5_my_order].status, CardStatus::CALLED_TO_PLAY);

  PerformAction perform = g.take_action();

  // Bug action: play null-5. Should NOT pick this.
  bool is_buggy = std::holds_alternative<PerformPlay>(perform) &&
                   std::get<PerformPlay>(perform).target == null_5_my_order;
  EXPECT_FALSE(is_buggy) << "bot picked the buggy line (play null-5 from order "
                          << null_5_my_order << "); expected a stall clue";

  // Winning line: any clue.
  bool is_clue = std::holds_alternative<PerformColour>(perform) ||
                  std::holds_alternative<PerformRank>(perform);
  EXPECT_TRUE(is_clue) << "expected a stall clue, got "
                        << hanabi::to_json(perform, 0).dump();
}
