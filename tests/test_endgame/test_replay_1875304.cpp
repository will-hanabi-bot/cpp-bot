// Port of python-bot/tests/test_endgame/test_replay_1875304.py.
//
// hanab.live replay 1875304 turn 22. The bot's urgent g4 play must NOT
// shortcut past the endgame solver - the winning line is a stall clue
// rank-4 to yagami.
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

// Hanab.live export deck. Suit indices: 0=Red, 1=Green, 2=Blue.
const std::vector<std::pair<int, int>> kDeck = {
    {2, 4}, {0, 2}, {2, 3}, {1, 4}, {2, 5},
    {0, 3}, {2, 1}, {1, 1}, {1, 3}, {1, 2},
    {0, 4}, {2, 4}, {0, 5}, {2, 2}, {0, 3},
    {1, 5}, {1, 3}, {1, 2}, {1, 4}, {1, 1},
    {0, 1}, {0, 1}, {2, 1}, {1, 1}, {2, 2},
    {0, 2}, {0, 4}, {0, 1}, {2, 1}, {2, 3},
};

const std::vector<OrigAction> kOrigActions = {
    {3, 2, 3}, {0, 6, 0}, {0, 13, 0}, {2, 2, 2}, {0, 7, 0},
    {3, 1, 3}, {0, 2, 0}, {0, 17, 0}, {0, 11, 0}, {3, 2, 4},
    {0, 8, 0}, {3, 1, 5}, {0, 4, 0}, {0, 21, 0}, {1, 20, 0},
    {1, 22, 0}, {1, 23, 0}, {2, 1, 0}, {0, 1, 0}, {0, 5, 0},
    {3, 1, 2},
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.orig_to_my_player = {0, 1, 2};
  ctx.my_order_to_id = kDeck;
  return ctx;
}

}  // namespace

TEST(EndgameReplay1875304, SolverFindsWinningClue) {
  SetupOptions opts;
  opts.hands = {
      // Alice = bot69 (observer): hidden
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = yagami
      {"g2", "g3", "g1", "b1", "r3"},
      // Cathy = bot67
      {"r3", "b2", "r5", "b4", "r4"},
  };
  opts.variant_name = "Muddy-Rainbow-Ones (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  ASSERT_EQ(g.state.score(), 11);
  ASSERT_EQ(g.state.cards_left, 1);
  EXPECT_EQ(g.state.play_stacks, (std::vector<int>{3, 3, 5}));

  // Sanity: bot69's slot 3 is the called-to-play g4 (= order 18).
  int g4_my_order = ctx.orig_to_my_order[18];
  ASSERT_EQ(g4_my_order, 18);
  int alice_slot3 = g.state.hands[static_cast<int>(TestPlayer::ALICE)][2];
  ASSERT_EQ(alice_slot3, g4_my_order);
  EXPECT_EQ(g.meta[g4_my_order].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(g.meta[g4_my_order].urgent);

  PerformAction perform = g.take_action();

  bool is_buggy = std::holds_alternative<PerformPlay>(perform) &&
                   std::get<PerformPlay>(perform).target == g4_my_order;
  EXPECT_FALSE(is_buggy) << "bot picked the buggy line (play g4); expected a clue";

  bool is_clue = std::holds_alternative<PerformColour>(perform) ||
                  std::holds_alternative<PerformRank>(perform);
  EXPECT_TRUE(is_clue) << "expected a clue, got " << hanabi::to_json(perform, 0).dump();
  EXPECT_FALSE(std::holds_alternative<PerformDiscard>(perform));
}
