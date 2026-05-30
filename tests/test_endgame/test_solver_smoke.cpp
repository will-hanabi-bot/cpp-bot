// Smoke test for the endgame solver: confirms construction + a simple solve
// case completes without crashing. Behavioral parity vs. Python lands once
// the reactor convention port is complete (Phase 4 tail).
#include <gtest/gtest.h>

#include "hanabi/endgame/fraction.h"
#include "hanabi/endgame/helper.h"
#include "hanabi/endgame/solver.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::endgame;
using namespace hanabi::test;

TEST(EndgameSmoke, FractionArithmetic) {
  Fraction half(1, 2);
  Fraction third(1, 3);
  EXPECT_EQ(half + third, Fraction(5, 6));
  EXPECT_EQ(half * third, Fraction(1, 6));
  EXPECT_TRUE(half > third);
  EXPECT_FALSE(half < third);
  EXPECT_EQ(Fraction(2, 4), Fraction(1, 2));  // normalized on construction
}

TEST(EndgameSmoke, RemainingMapOps) {
  RemainingMap m;
  m[Identity(0, 1).to_ord()] = 3;
  m[Identity(0, 2).to_ord()] = 2;
  EXPECT_EQ(remaining_total(m), 5);

  RemainingMap m2 = remaining_remove(m, Identity(0, 1));
  EXPECT_EQ(remaining_total(m2), 4);
  EXPECT_EQ(m2[Identity(0, 1).to_ord()], 2);

  RemainingMap m3 = remaining_remove(m2, Identity(0, 1));
  EXPECT_EQ(m3.count(Identity(0, 1).to_ord()), 1u);
  EXPECT_EQ(m3[Identity(0, 1).to_ord()], 1);

  RemainingMap m4 = remaining_remove(m3, Identity(0, 1));
  // Last copy removed → key deleted.
  EXPECT_EQ(m4.count(Identity(0, 1).to_ord()), 0u);
}

TEST(EndgameSmoke, SolverConstructsAndAttemptsTrivialWin) {
  // 2-player game with play stacks at (4, 5, 5, 5, 5) - score 24, one r5 wins.
  // fully_known pins Alice's slot 1 to r5; we run elim() so the per-player
  // Thought picks up the pinned inference and solver.solve's trivial-win
  // early-exit can fire.
  SetupOptions opts;
  opts.hands = {
      {"r5", "xx", "xx", "xx", "xx"},
      {"xx", "xx", "xx", "xx", "xx"},
  };
  opts.play_stacks = std::vector<int>{4, 5, 5, 5, 5};
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "r5");
  // Mark Alice's r5 dirty in common so elim propagates to per-player thoughts.
  g.common.dirty.insert(g.state.hands[0][0]);
  g.elim();

  EndgameSolver solver{/*mc=*/true, /*timeout=*/10.0};
  SolveResult r = solver.solve(g);
  ASSERT_TRUE(r.ok()) << r.error;
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(r.action));
  EXPECT_EQ(r.winrate, Fraction(1));
}
