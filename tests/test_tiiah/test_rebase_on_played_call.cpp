// A stable call that overlaps a call standing in another hand builds on it once
// that call is played (tiiah/CONVENTION.md §1c, the user's ruling; v19.2.0).
//
// Seats here: Bob gives both clues; Cathy holds the first call (she is "Bob" to
// Bob, the first to act after him); we, Alice, hold the second (role inversion
// makes Bob's clue to us stable while Cathy holds her call). Our purple card is
// read `{p1}` when clued. What it IS depends on what Cathy does next:
//   - she plays her called p1: ours builds on it, `{p2}`;
//   - she throws hers, or plays something else: ours stays `{p1}`.
// Either way the call stands: we are to play the card just clued.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Bob calls Cathy's p1, the table goes round, and Bob calls our slot 1 purple.
Game two_overlapping_calls() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r4", "g4", "b4", "y4", "r5"},
      {"p1", "r3", "g3", "b3", "y3"},
  };
  opts.clue_tokens = 5;
  opts.starting = TestPlayer::BOB;
  use_tiiah(opts);
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Bob clues purple to Cathy");
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5,
                    /*reached_the_hole=*/false, "g5");
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/5,
                    /*reached_the_hole=*/false);
  g = take_turn(std::move(g), "Bob clues purple to Alice (slot 1)");
  return g;
}

IdentitySet one(const Game& g, const char* id) {
  return IdentitySet::single(g.state.expand_short(id));
}

}  // namespace

TEST(TiiahRebaseOnPlayedCall, PlayingTheOverlappingCallRebasesOurs) {
  Game g = two_overlapping_calls();
  const int ours = order_at(g, TestPlayer::ALICE, 1);
  ASSERT_EQ(g.meta[ours].status, CardStatus::CALLED_TO_PLAY);
  ASSERT_EQ(g.common.thoughts[ours].possibilities(), one(g, "p1"));
  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::CALLED_TO_PLAY)
      << "Cathy's p1, pushed to slot 2 by her draw";

  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/2,
                    /*reached_the_hole=*/true, "y5");

  EXPECT_EQ(g.meta[ours].status, CardStatus::CALLED_TO_PLAY) << "the call stands";
  EXPECT_EQ(g.common.thoughts[ours].possibilities(), one(g, "p2"))
      << "it builds on the p1 Cathy just played";
}

TEST(TiiahRebaseOnPlayedCall, ThrowingTheOverlappingCardLeavesOursAlone) {
  Game g = two_overlapping_calls();
  const int ours = order_at(g, TestPlayer::ALICE, 1);

  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/2,
                    /*reached_the_hole=*/false, "y5");

  EXPECT_EQ(g.meta[ours].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[ours].possibilities(), one(g, "p1"))
      << "Cathy threw her copy: ours is played on the stacks as they were";
}

TEST(TiiahRebaseOnPlayedCall, PlayingAnUnrelatedCardLeavesOursAlone) {
  Game g = two_overlapping_calls();
  const int ours = order_at(g, TestPlayer::ALICE, 1);

  // Her slot 1 is the g5 she drew: uncalled, and nothing to do with purple.
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/1,
                    /*reached_the_hole=*/true, "y5");

  EXPECT_EQ(g.common.thoughts[ours].possibilities(), one(g, "p1"));
}
