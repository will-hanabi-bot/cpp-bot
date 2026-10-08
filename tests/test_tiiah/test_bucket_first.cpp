// THE BUCKET FIRST (tiiah/CONVENTION.md §1d, v22.0.0, the user's ruling).
//
// A reactive's receiver reads its called card as the BUCKET half alone -- the
// playables of the bucket one step from the card the reacter played -- unless the
// card cannot be any of them and could be the card after the reacter's: then it is
// a finesse, provably, and reads as that card. Until v22.0.0 it read the union of
// the two. A finesse the receiver cannot prove may not be given.
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

std::optional<ClueInterp> interp_of(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* c = std::get_if<ClueInterp>(&g.move_history.back())) return *c;
  return std::nullopt;
}

// Six suits: red, yellow (bucket 0), green, blue (1), purple, teal (2). Nothing
// played. Bob's slot 4 is the card the clue names for him.
SetupOptions six_suits(const std::string& bob_slot4,
                       std::vector<std::string> cathy) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (6 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r4", "y4", "b4", bob_slot4, "g4"},
      std::move(cathy),
  };
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 7;
  use_tiiah(opts);
  return opts;
}

Game clue_and_react(SetupOptions opts, const std::string& clue) {
  Game g = setup(std::move(opts));
  g = take_turn(std::move(g), clue);
  EXPECT_FALSE(g.waiting.empty()) << "the fixture did not produce a reactive";
  return hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/4,
                       /*reached_the_hole=*/true, "t5");
}

// Four suits: red, yellow (bucket 0), green (1), blue (2) -- the three-bucket
// rule. Nothing played. Bob's slot 4 is the card the clue names for him.
SetupOptions four_suits(const std::string& bob_slot4,
                        std::vector<std::string> cathy) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r4", "y4", "g4", bob_slot4, "g3"},
      std::move(cathy),
  };
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 7;
  use_tiiah(opts);
  return opts;
}

Game clue_and_react_4_suits(SetupOptions opts, const std::string& clue) {
  Game g = setup(std::move(opts));
  g = take_turn(std::move(g), clue);
  EXPECT_FALSE(g.waiting.empty()) << "the fixture did not produce a reactive";
  return hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/4,
                       /*reached_the_hole=*/true, "b5");
}

}  // namespace

// The user's example, in four suits. Alice's 5 to Cathy (anchor 5) pairs Bob's
// slot 4 with Cathy's slot 1. Bob plays the b1, bucket 2; a rank clue names the
// bucket above, wrapping to red and yellow. Cathy's slot 1 was not touched, so it
// could also be the b2 the b1 continues into -- not provably a finesse, so the
// bucket alone: `{r1,y1}`.
TEST(TiiahBucketFirst, AnUnprovableFinesseReadsTheBucketAlone) {
  Game g = clue_and_react_4_suits(four_suits("b1", {"r1", "b2", "r5", "g5", "y5"}),
                                  "Alice clues 5 to Cathy");
  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 1), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/1, {"r1", "y1"});
}

// Alice's Yellow to Cathy (anchor 2) pairs Bob's slot 4 with Cathy's slot 3. Bob
// plays the y1, bucket 0; a colour clue names the bucket below, purple and teal.
// Cathy's slot 3 is yellow, so it cannot be either: the finesse, provably -- `{y2}`.
TEST(TiiahBucketFirst, AProvableFinesseReadsTheContinuation) {
  Game g = clue_and_react(six_suits("y1", {"r3", "g3", "y2", "b3", "p3"}),
                          "Alice clues yellow to Cathy");
  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/3, {"y2"});
}

// The giver's half. The same 5 to Cathy with her slot 1 the g2: the pairing is a
// finesse on Bob's g1, and Cathy could not prove it -- she would read `{p1,t1}`.
// So Alice may not give it: at her seat the clue is no reactive.
TEST(TiiahBucketFirst, TheGiverMayNotGiveAnUnprovableFinesse) {
  Game g = setup(six_suits("g1", {"g2", "r3", "p5", "b5", "y5"}));
  g = take_turn(std::move(g), "Alice clues 5 to Cathy");
  EXPECT_NE(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_NE(status_at(g, TestPlayer::CATHY, 1), CardStatus::CALLED_TO_PLAY);
}
