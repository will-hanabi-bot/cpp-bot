// The receiver's reading falls back to the worlds (tiiah/CONVENTION.md §1d, the
// user's ruling; v20.5.0; replay 2018517).
//
// Cathy throws a card into the hole without knowing it, so the frame Alice and
// Cathy share lags it. Alice's 2 to Cathy is a reactive: Bob's p3 into Cathy's
// slot 2. At the reaction the shared stamp reads Cathy's card on that frame, finds
// nothing playable there, and reads a bluff. In the world where Cathy's hole card
// was the right 1, her card plays: it is called, and the hole collapses.
//   1. The bucket rule first: a p reacts into the red/yellow bucket.
//   2. Otherwise any one-away identity that plays in some world.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Purple on 2. Cathy holds the target (slot 1, clued `colour`) and, in slot 5, the 1
// she is about to throw. Bob's slot 5 is the p3 that will react.
Game position(const char* target, const char* thrown, const char* colour) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g4", "b4", "y4", "p3"},
      {target, "g3", "y4", "b4", thrown},
  };
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 2};
  opts.clue_tokens = 5;
  opts.starting = TestPlayer::CATHY;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::CATHY, 1, {colour});
  return fully_known(std::move(g), TestPlayer::BOB, 5, "p3");
}

// Cathy throws her slot 5 (into the hole, or onto the discard pile), Alice gives
// the 2, and Bob reacts with his p3.
Game play_out(Game g, bool into_the_hole) {
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5, into_the_hole, "g4");
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");
  return hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/5,
                       /*reached_the_hole=*/true, "r4");
}

}  // namespace

// Step 1: the r2 is the bucket reading, playable where Cathy's hole card was the r1.
TEST(TiiahReceiverWorldFallback, TheBucketReadingPlaysInAWorldAndTheHoleCollapses) {
  Game g = position("r2", "r1", "red");
  const int target = order_at(g, TestPlayer::CATHY, 1);

  g = play_out(std::move(g), /*into_the_hole=*/true);

  EXPECT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[target].possibilities(),
            IdentitySet::single(g.state.expand_short("r2")));
  EXPECT_EQ(g.state.common_play_stacks[0], 1)
      << "Cathy's hole card was the r1, for every seat";
}

// Step 2: a known g2 is outside the bucket a p3 names, but it is one away and plays
// where Cathy's hole card was the g1.
TEST(TiiahReceiverWorldFallback, AOneAwayReadingPlaysInAWorldAndTheHoleCollapses) {
  Game g = position("g2", "g1", "green");
  const int target = order_at(g, TestPlayer::CATHY, 1);

  g = play_out(std::move(g), /*into_the_hole=*/true);

  EXPECT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[target].possibilities(),
            IdentitySet::single(g.state.expand_short("g2")));
  EXPECT_EQ(g.state.common_play_stacks[2], 1)
      << "Cathy's hole card was the g1, for every seat";
}

// The control: her 1 went to the discard pile, so no world explains the call and the
// stamp's bluff reading stands.
TEST(TiiahReceiverWorldFallback, WithNoHoleTheReadingIsUnchanged) {
  Game g = position("r2", "r1", "red");
  const int target = order_at(g, TestPlayer::CATHY, 1);

  g = play_out(std::move(g), /*into_the_hole=*/false);

  EXPECT_NE(g.meta[target].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.state.common_play_stacks[0], 0);
}
