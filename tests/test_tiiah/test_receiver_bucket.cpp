// The RECEIVER's half of the bucket relation (tiiah/CONVENTION.md §1d).
//
// The reacter's card is narrowed when the clue is read. The receiver's call is
// not made until the reacter acts, and reactor0's shared `stamp_receiver_call`
// makes it — knowing nothing about buckets. So until v16.9.0 the receiver kept
// the generic reading: every playable the stacks and their own negative
// information still allowed.
//
// What they may write is ONE of §1d's two readings, intersected with what the card
// could already be: the bucket one step along from the reacter's, or -- only when
// the finesse is provable -- the continuation of the card the reacter played
// (v22.0.0; until then the union of the two).
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

// Replay 2008217's shape, moved to four suits (buckets {R,Y}, {G}, {B}: the
// three-bucket rule). Alice clues Blue to Cathy — an ordinary reactive (§1c), Bob
// reacting. Anchor 4, Bob answers on his slot 4, so Cathy's slot is (4 + 5) mod 5
// = 5.
//
// Bob's slot 4 is what the clue names; the fixture varies it per test, since it
// is the card whose bucket the receiver reads.
SetupOptions replay_opts(const std::string& bob_slot4) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y5", "r4", "y3", bob_slot4, "g4"},
      {"r2", "g3", "b4", "y4", "r1"},
  };
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 7;
  use_tiiah(opts);
  return opts;
}

// Alice clues, Bob answers on slot 4, and we read what Cathy was left with.
Game clue_and_react(SetupOptions opts, const std::string& clue,
                    const std::string& bob_draw) {
  Game g = setup(std::move(opts));
  g = take_turn(std::move(g), clue);
  EXPECT_FALSE(g.waiting.empty()) << "the fixture did not produce a reactive";
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/4,
                    /*reached_the_hole=*/true, bob_draw);
  return g;
}

}  // namespace

// The replay's shape. Bob plays a b1 — bucket 2 — and the clue is a COLOUR one,
// so Cathy is one bucket lower: green. The finesse branch would add the b2 that
// the b1 continues into, and a card the Blue clue did not touch cannot be blue,
// so it drops.
TEST(TiiahReceiverBucket, AColourClueLeavesTheBucketBelow) {
  SetupOptions opts = replay_opts("b1");
  opts.hands[2] = {"r2", "g3", "b4", "y4", "g1"};  // her slot 5 is a g1
  Game g = clue_and_react(std::move(opts), "Alice clues blue to Cathy", "b5");

  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 5), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/5, {"g1"});
}

// The same clue with the reacter holding a g1 instead. Green is bucket 1, so the
// bucket half is the bucket below it: red and yellow. The continuation is the g2,
// which the Blue clue says nothing against -- but the card could be a bucket card
// too, so the finesse is not provable and the bucket alone stands (v22.0.0; until
// then the g2 survived).
TEST(TiiahReceiverBucket, AnUnprovableContinuationDrops) {
  Game g = clue_and_react(replay_opts("g1"), "Alice clues blue to Cathy", "b5");

  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 5), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/5, {"r1", "y1"});
}

// A RANK clue reads the relation the other way: the receiver is one bucket
// HIGHER than the reacter. Rank 4 anchors on 4, so the slots pair the same way.
// Bob's b1 is bucket 2, so Cathy is bucket 0, wrapping — red and yellow. The b2
// the b1 continues into is an unprovable finesse, so it is not read (v22.0.0).
TEST(TiiahReceiverBucket, ARankClueLeavesTheBucketAbove) {
  Game g = clue_and_react(replay_opts("b1"), "Alice clues 4 to Cathy", "b5");

  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 5), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/5, {"r1", "y1"});
}
