// Port of python-bot/tests/test_endgame/test_replay_1875305.py.
//
// hanab.live replay 1875305 turn 22. The solver should find a winning clue
// (not the discard the bot picked in the actual replay).
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/endgame/fraction.h"
#include "hanabi/endgame/solver.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::endgame;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// Hanab.live export deck. Suit indices: 0=Red, 1=Green, 2=Blue.
const std::vector<std::pair<int, int>> kDeck = {
    {1, 3}, {2, 5}, {1, 4}, {2, 2}, {0, 5},
    {2, 3}, {0, 1}, {1, 3}, {1, 4}, {1, 1},
    {2, 4}, {2, 3}, {1, 2}, {2, 1}, {1, 5},
    {0, 1}, {1, 2}, {2, 1}, {0, 4}, {0, 3},
    {2, 2}, {0, 2}, {1, 1}, {2, 1}, {2, 4},
    {0, 2}, {0, 1}, {0, 4}, {1, 1}, {0, 3},
};

const std::vector<OrigAction> kOrigActions = {
    {3, 2, 2}, {0, 9, 0}, {0, 13, 0}, {3, 2, 3}, {0, 15, 0},
    {2, 0, 1}, {0, 3, 0}, {3, 0, 3}, {0, 12, 0}, {0, 0, 0},
    {3, 0, 5}, {0, 11, 0}, {0, 2, 0}, {2, 0, 0}, {0, 10, 0},
    {0, 1, 0}, {2, 0, 0}, {0, 14, 0}, {1, 24, 0}, {2, 0, 2},
    {0, 21, 0},
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

TEST(EndgameReplay1875305, SolverFindsWinningClue) {
  SetupOptions opts;
  opts.hands = {
      // Alice = bot67 (observer): hidden
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = bot69
      {"g1", "g4", "g3", "r1", "b3"},
      // Cathy = yagami
      {"g5", "b1", "g2", "b3", "b4"},
  };
  opts.variant_name = "Muddy-Rainbow-Ones (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  ASSERT_EQ(g.state.score(), 12);
  ASSERT_EQ(g.state.cards_left, 2);
  EXPECT_EQ(g.state.play_stacks, (std::vector<int>{2, 5, 5}));

  EndgameSolver solver(/*mc=*/true, /*timeout=*/30.0);
  SolveResult result = solver.solve(g);
  ASSERT_TRUE(result.ok()) << "solver should find a winning action; got: " << result.error;
  EXPECT_EQ(result.winrate, Fraction(1));

  bool is_clue = std::holds_alternative<PerformColour>(result.action) ||
                  std::holds_alternative<PerformRank>(result.action);
  EXPECT_TRUE(is_clue) << "expected a clue, got " << hanabi::to_json(result.action, 0).dump();
}
