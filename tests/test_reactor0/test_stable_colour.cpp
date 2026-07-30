// Reactor0 stable colour clues are DIRECT play clues — no referential
// play. Priority: play reveal > leftmost touched that could be playable >
// stall.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

TEST(Reactor0StableColour, PlayRevealFillsInPreviouslyCluedCard) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y2", "r3", "g4", "b2", "p4"},   // Bob: r3 at slot 2, the only red
      {"g2", "y4", "b3", "p2", "r4"},
  };
  opts.play_stacks = {{2, 0, 0, 0, 0}};  // r3 is playable
  use_reactor0(opts);
  Game g = setup(opts);
  // Slot 2 pre-clued "3": possible = all 3s, not an obvious playable yet.
  g = pre_clue(std::move(g), TestPlayer::BOB, 2, {"3"});

  g = take_turn(std::move(g), "Alice clues red to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REVEAL);
  // The reveal needs no stamp — empathy now identifies r3 as playable.
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
  auto obvious = g.common.obvious_playables(g, static_cast<int>(TestPlayer::BOB));
  EXPECT_TRUE(std::find(obvious.begin(), obvious.end(),
                        order_at(g, TestPlayer::BOB, 2)) != obvious.end());
}

TEST(Reactor0StableColour, LeftmostTouchedCouldBePlayableIsCalled) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "r3", "y2", "g2", "b2"},   // red touches slots 1 and 2
      {"g1", "y4", "b3", "p2", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  g = take_turn(std::move(g), "Alice clues red to Bob");

  // The TOUCHED slot-1 card itself is called — not a referential neighbour.
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::BOB, 1, {"r1"});
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::NONE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::NONE)
      << "no referential target left of the touched cards";
}

TEST(Reactor0StableColour, KnownUnplayableColourIsStall) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "y2", "g4", "b2", "p4"},   // r3 is Bob's only red
      {"g1", "y4", "b3", "p2", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);
  // Slot 1 known to be a 3 => with stacks at 0 it cannot be playable.
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"3"});

  g = take_turn(std::move(g), "Alice clues red to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::STALL);
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_DISCARD));
}
