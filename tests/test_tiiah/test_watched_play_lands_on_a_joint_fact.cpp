// A partner's watched play that lands in some world of our hole cards collapses
// the worlds to those, even when no single card is named (tiiah/CONVENTION.md
// §1e, v20.15.0, the user's ruling; replay 2019249 T20-T23).
//
// We threw two 1s into the hole, each of which could be any 1. Bob's b2 goes in.
// If neither of ours was the b1 it struck -- but there is a world in which one of
// them was, so it landed, and our stacks reach blue 2. Until v20.15.0 the worlds
// were pruned card by card: each card could still be every 1, our stacks (the
// minimum over every assignment) kept blue on 0, and the b2 was booked as a strike.
// The control: a b3 lands in no world of two 1s, and is a strike.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

constexpr int kBlue = 3;

Game two_ones_in_the_hole(const char* bobs_card) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {bobs_card, "g4", "y3", "p4", "r4"},
      {"g3", "b4", "p3", "r3", "y4"},
  };
  opts.clue_tokens = 6;
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::ALICE, 4, {"1"});
  g = pre_clue(std::move(g), TestPlayer::ALICE, 5, {"1"});
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/5, /*reached_the_hole=*/true);
  // Discards between our two plays: a clue here could read into our cards.
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/5, /*reached_the_hole=*/false,
                    "g5");
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5, /*reached_the_hole=*/false,
                    "r5");
  // Our other 1 has moved to slot 5 with the new card's draw.
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/5, /*reached_the_hole=*/true);
  // Bob's card is in his slot 2 now, behind the g5 he drew.
  return hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/2, /*reached_the_hole=*/true,
                       "g2");
}

}  // namespace

TEST(TiiahJointFact, AWatchedB2LandsOnOneOfOurOnes) {
  Game g = two_ones_in_the_hole("b2");
  EXPECT_EQ(g.state.play_stacks[kBlue], 2) << "one of our 1s was the b1, and the b2 landed";
}

TEST(TiiahJointFact, AWatchedB3LandsInNoWorld) {
  Game g = two_ones_in_the_hole("b3");
  EXPECT_EQ(g.state.play_stacks[kBlue], 0) << "two 1s cannot be the b1 and the b2";
}
