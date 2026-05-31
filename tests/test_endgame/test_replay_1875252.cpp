// Port of python-bot/tests/test_endgame/test_replay_1875252.py.
//
// Endgame solver must pick a clue that communicates the must-play card
// (Light Pink 5 in yagami's hand). Verifies the per-player-knowledge fix
// of winnable_simpler.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/endgame/fraction.h"
#include "hanabi/endgame/solver.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::endgame;
using namespace hanabi::test;

namespace {

int yagami_i5_order(const Game& g) {
  const auto& yagami_hand = g.state.hands[static_cast<int>(TestPlayer::CATHY)];
  for (int o : yagami_hand) {
    auto id = g.state.deck[o].id();
    if (id && id->suit_index == 2 && id->rank == 5) return o;
  }
  throw std::runtime_error("Light Pink 5 not found in yagami's hand");
}

}  // namespace

TEST(EndgameReplay1875252, SolverPicksClueThatCommunicatesMustPlay) {
  SetupOptions opts;
  opts.hands = {
      // bot69 (P0, observer): hidden
      {"xx", "xx", "xx", "xx", "xx"},
      // bot67 (P1): all dead cards
      {"r2", "b3", "b4", "i1", "i3"},
      // yagami (P2): contains i5 at slot 1
      {"i5", "i4", "b2", "r3", "r4"},
  };
  opts.play_stacks = std::vector<int>{5, 5, 4};
  opts.discarded = {"r1"};
  opts.variant_name = "Omni-Ones & Light Pink (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.state.score(), 14);
  ASSERT_EQ(g.state.max_score(), 15);
  ASSERT_EQ(g.state.cards_left, 0);

  EndgameSolver solver(/*mc=*/true, /*timeout=*/10.0);
  SolveResult result = solver.solve(g);
  ASSERT_TRUE(result.ok()) << "solver should find a winning clue; got: " << result.error;
  EXPECT_EQ(result.winrate, Fraction(1));

  // The chosen action must be a clue.
  bool is_clue = std::holds_alternative<PerformColour>(result.action) ||
                  std::holds_alternative<PerformRank>(result.action);
  ASSERT_TRUE(is_clue) << "expected a clue, got " << hanabi::to_json(result.action, 0).dump();

  // Rule out the bug action (PerformRank target=1 value=3).
  if (std::holds_alternative<PerformRank>(result.action)) {
    const auto& pr = std::get<PerformRank>(result.action);
    EXPECT_FALSE(pr.target == static_cast<int>(TestPlayer::BOB) && pr.value == 3)
        << "solver picked the bug action PerformRank(target=Bob, value=3)";
  }

  // The clue must target yagami (the player holding i5).
  int target = std::holds_alternative<PerformColour>(result.action)
                    ? std::get<PerformColour>(result.action).target
                    : std::get<PerformRank>(result.action).target;
  EXPECT_EQ(target, static_cast<int>(TestPlayer::CATHY))
      << "clue should target yagami (player 2); got target=" << target;

  // The chosen clue must either mark i5 as CALLED_TO_PLAY directly (stable)
  // or set up a reactive waiting connection.
  int i5_order = yagami_i5_order(g);
  Action action = EndgameSolver::perform_to_action(result.action, g,
                                                      static_cast<int>(TestPlayer::ALICE));
  Game new_game = g.simulate_action(action);
  CardStatus i5_status = new_game.meta[i5_order].status;
  bool is_stable_mark = i5_status == CardStatus::CALLED_TO_PLAY;
  bool is_reactive_clue = !new_game.waiting.empty();
  EXPECT_TRUE(is_stable_mark || is_reactive_clue)
      << "clue neither marks i5 CALLED_TO_PLAY nor sets up a reactive WC";
}
