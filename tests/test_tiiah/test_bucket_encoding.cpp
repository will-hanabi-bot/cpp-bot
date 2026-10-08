// What a reactive clue SAYS about the reacter's own card (tiiah/CONVENTION.md
// §1d).
//
// The card is going into the hole, so nothing can name it directly. The clue
// KIND carries the relation instead: under a rank clue the receiver's target
// sits one bucket HIGHER than the reacter's card, under a colour clue one
// bucket LOWER. The reacter runs that backwards and writes down the playables
// of the named bucket — "playable" judged after the receiver's queued plays, so
// a card that only comes live once they play what they already know counts.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
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

}  // namespace

// The worked example from the spec, in four suits, so the buckets are {R,Y},
// {G}, {B}. Red is on 2 and Bob knows his r3, so a clue to him reverses.
// Once that r3 is assumed played his leftmost want is the g1 — a bucket 1
// target — and rank 5 anchors the reaction on Cathy's slot 4.
//
// Bucket 1 under a RANK clue means the reacter sits in bucket 0, and the
// playables of bucket 0 after the queued r3 are r4 and y1. Cathy cannot tell
// which she holds, and does not need to: both are plays.
TEST(TiiahBucketEncoding, ARankClueNamesTheBucketBelowTheTarget) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.play_stacks = std::vector<int>{2, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g1", "r3", "y5", "g5", "b5"},   // the known r3, and a g1 in front of it
      {"b4", "b3", "y4", "r4", "g4"},   // slot 4 is the reaction
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/2, "r3");

  g = take_turn(std::move(g), "Alice clues 5 to Bob");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 4), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/4, {"r4", "y1"});
}

// The other direction. A COLOUR clue puts the target one bucket lower, so the
// reacter reads one bucket higher: a bucket 1 target (green) names bucket 2,
// which in a four-suit game is blue alone.
TEST(TiiahBucketEncoding, AColourClueNamesTheBucketAboveTheTarget) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "g1", "y4", "b4", "r4"},  // known r1, then the g1 the clue is for
      {"y3", "b1", "g3", "b3", "r3"},  // slot 2 is the reaction
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  // Blue anchors on 4, the target is his slot 2, so the reaction is
  // (4 - 2) = slot 2.
  g = take_turn(std::move(g), "Alice clues blue to Bob");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/2, {"b1"});
}

// A finesse says the identity by itself and the buckets never come into it:
// there is exactly one card that bridges to a one-away target, so the reacter
// writes that card down and nothing else.
TEST(TiiahBucketEncoding, AFinesseNamesTheConnectorOutright) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "r3", "y4", "g4", "b4"},  // the r3 is one away behind the r1
      {"r2", "y3", "g3", "b3", "p3"},  // and Cathy holds the bridge
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/1, {"r2"});
}

// And the relation is a rule, not a decoration. The same colour clue over a
// reacter card that is playable but sits in the WRONG bucket says nothing
// either player could act on, so the pairing is refused rather than stamped —
// and with no other target in Bob's hand the clue reads as nothing at all.
TEST(TiiahBucketEncoding, APairingThatBreaksTheBucketRelationIsNotRead) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "g1", "y4", "b4", "r4"},
      {"y3", "y1", "g3", "b3", "r3"},  // y1 plays, but yellow is bucket 0
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  g = take_turn(std::move(g), "Alice clues blue to Bob");

  EXPECT_NE(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::NONE)
      << "a pairing the relation refuses may not be stamped anyway";
}
