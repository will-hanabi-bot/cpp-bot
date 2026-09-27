// A play a pair can both name is in that pair's view AS A FLOOR, and a play the
// whole team can name raises the shared view -- settling the hole cards that must
// sit under it (tiiah/CONVENTION.md §1.3, §1e rule 6's shared-view form, v16.23.0).
//
// Replay 2011327 is the game: will-bot67 knew its r2 at T11 while its views had red
// on 0 behind will-bot69's unnamed r1, and every view it read a clue against
// stayed behind for the rest of the game.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

SetupOptions opts_for(std::vector<int> stacks = {0, 0, 0, 0, 0}) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y1", "g1", "b1", "p1"},
      {"r2", "y2", "g2", "b3", "p3"},
  };
  opts.play_stacks = stacks;
  opts.starting = TestPlayer::BOB;
  use_tiiah(opts);
  return opts;
}

int row(const Game& g, TestPlayer p, int suit) {
  return g.state.pairwise_play_stacks[static_cast<int>(p)][suit];
}

}  // namespace

// Bob throws his r1 into the hole blind; Cathy then plays an r2 she KNOWS. Bob and
// we watched it, Cathy named it: every view takes it. And since the team knows an
// r2 landed, the r1 under it must have been Bob's hole card.
TEST(TiiahKnownPlayFloor, AKnownPlayAboveEveryViewReachesThemAllAndSettlesTheHole) {
  Game g = setup(opts_for());
  const int bobs = order_at(g, TestPlayer::BOB, 1);
  g = hidden_action(std::move(g), TestPlayer::BOB, 1, /*reached_the_hole=*/true);
  ASSERT_TRUE(g.meta[bobs].superposed()) << "guard: Bob could not name his r1";
  ASSERT_EQ(g.state.common_play_stacks[0], 0);

  g = fully_known(std::move(g), TestPlayer::CATHY, 1, "r2");
  g = hidden_action(std::move(g), TestPlayer::CATHY, 1, /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.play_stacks[0], 2) << "we watched both";
  EXPECT_EQ(g.state.common_play_stacks[0], 2)
      << "Cathy's r2 was common knowledge, and it landed";
  EXPECT_EQ(row(g, TestPlayer::BOB, 0), 2) << "Bob watched it land";
  EXPECT_EQ(row(g, TestPlayer::CATHY, 0), 2) << "Cathy named it";
  EXPECT_FALSE(g.meta[bobs].superposed())
      << "only a world where Bob's card was the r1 lets the r2 land";
}

// The same deduction from the seat that MADE the known play: here the r2 the team
// names is our own, and we cannot see it. Every view still ends where the
// observer's did above, which is what makes the shared view shared.
TEST(TiiahKnownPlayFloor, TheSeatThatPlaysTheKnownCardReachesTheSameSharedView) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));
  const int bobs = order_at(g, TestPlayer::BOB, 1);
  g = hidden_action(std::move(g), TestPlayer::BOB, 1, /*reached_the_hole=*/true);
  // Cathy passes the turn with a discard she can see is safe to us.
  g = hidden_action(std::move(g), TestPlayer::CATHY, 5, /*reached_the_hole=*/false);

  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "r2");
  g = hidden_action(std::move(g), TestPlayer::ALICE, 1, /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.common_play_stacks[0], 2);
  EXPECT_EQ(row(g, TestPlayer::BOB, 0), 2);
  EXPECT_EQ(row(g, TestPlayer::CATHY, 0), 2);
  EXPECT_FALSE(g.meta[bobs].superposed());
}

// The rows take the card the watchers SAW. Our copy of what the player knew is a
// reading, and a reading made on a stale view can be wrong where our eyes are not.
// Replay 2011327 T19: will-bot67 read will-bot69's y2 as {y1}.
TEST(TiiahKnownPlayFloor, RowsTakeTheWatchedCardNotTheReading) {
  Game g = setup(opts_for({0, 1, 0, 0, 0}));
  const int y2 = order_at(g, TestPlayer::CATHY, 2);
  g.common.thoughts[y2].inferred = IdentitySet::single(Identity{1, 1});  // misread

  g = hidden_action(std::move(g), TestPlayer::BOB, 1, /*reached_the_hole=*/true);
  g = hidden_action(std::move(g), TestPlayer::CATHY, 2, /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.play_stacks[1], 2);
  EXPECT_EQ(row(g, TestPlayer::BOB, 1), 2)
      << "Bob and we both watched a y2 land, whatever the common copy said";
}

// A play our own belief resolves as a STRIKE is booked into nobody's view.
TEST(TiiahKnownPlayFloor, AStrikeRaisesNothing) {
  Game g = setup(opts_for());
  g = hidden_action(std::move(g), TestPlayer::BOB, 2, /*reached_the_hole=*/false);
  // Cathy blind-plays her b3 with blue on 0 and nothing of ours in the hole.
  g = hidden_action(std::move(g), TestPlayer::CATHY, 4, /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.play_stacks[3], 0);
  EXPECT_EQ(g.state.common_play_stacks[3], 0);
  EXPECT_EQ(row(g, TestPlayer::BOB, 3), 0) << "Bob watched a strike, not a b3";
}
