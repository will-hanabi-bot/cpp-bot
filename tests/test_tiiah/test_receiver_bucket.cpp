// The RECEIVER's half of the bucket relation (tiiah/CONVENTION.md §1d).
//
// The reacter's card is narrowed when the clue is read. The receiver's call is
// not made until the reacter acts, and reactor0's shared `stamp_receiver_call`
// makes it — knowing nothing about buckets. So until v16.9.0 the receiver kept
// the generic reading: every playable the stacks and their own negative
// information still allowed.
//
// What they may write is the union of §1d's two readings, intersected with what
// the card could already be: the bucket one step along from the reacter's, and
// the continuation of the card the reacter played.
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

// Replay 2008217, to the card. Alice clues Blue to Cathy — an ordinary reactive
// (§1c), Bob reacting. Anchor 4, Bob answers on his slot 4, so Cathy's slot is
// (4 + 5) mod 5 = 5.
//
// Bob's slot 4 is what the clue names; the fixture varies it per test, since it
// is the card whose bucket the receiver reads.
SetupOptions replay_opts(const std::string& bob_slot4) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y5", "p4", "y3", bob_slot4, "g4"},
      {"p2", "g3", "b4", "y4", "r1"},
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

// The replay. Bob plays a b1 — bucket 1 — and the clue is a COLOUR one, so
// Cathy is one bucket lower: red and yellow. The finesse branch would add the
// b2 that the b1 continues into, and a card the Blue clue did not touch cannot
// be blue, so it drops.
TEST(TiiahReceiverBucket, AColourClueLeavesTheBucketBelow) {
  Game g = clue_and_react(replay_opts("b1"), "Alice clues blue to Cathy", "p5");

  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 5), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/5, {"r1", "y1"});
}

// The same clue with the reacter holding a g1 instead. Green is bucket 1 too,
// so the bucket half is unchanged — but now the continuation is the g2, which
// the Blue clue says nothing against, so it survives.
TEST(TiiahReceiverBucket, TheFinesseContinuationSurvivesWhenItCan) {
  Game g = clue_and_react(replay_opts("g1"), "Alice clues blue to Cathy", "p5");

  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 5), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/5,
              {"r1", "y1", "g2"});
}

// A RANK clue reads the relation the other way: the receiver is one bucket
// HIGHER than the reacter. Rank 4 anchors on 4, so the slots pair the same way.
// Bob's r1 is bucket 0, so Cathy is bucket 1 — green and blue.
TEST(TiiahReceiverBucket, ARankClueLeavesTheBucketAbove) {
  SetupOptions opts = replay_opts("r1");
  opts.hands[2] = {"p2", "g3", "b4", "y4", "b1"};  // her slot 5 is a b1
  Game g = clue_and_react(std::move(opts), "Alice clues 4 to Cathy", "p5");

  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 5), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/5,
              {"g1", "b1", "r2"});
}

// The POV difference, pinned so it is deliberate. The REACTER cannot name what
// they played, so they read the bucket off the inference the clue left them and
// carry every continuation it allows — a wider set than the seats that watched
// the card. Seats differing on what they can see is this variant (§1.1).
TEST(TiiahReceiverBucket, TheReacterReadsAWiderSetThanTheSeatsThatSaw) {
  SetupOptions opts = replay_opts("g1");
  opts.init = [](Game& g) {
    g.convention = Convention::TIIAH;
    g.state.our_player_index = 1;  // we are Bob, the reacter
  };
  Game g = clue_and_react(std::move(opts), "Alice clues blue to Cathy", "");

  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 5), CardStatus::CALLED_TO_PLAY);
  const IdentitySet infs =
      g.common.thoughts[order_at(g, TestPlayer::CATHY, 5)].inferred;
  EXPECT_TRUE(infs.contains(g.state.expand_short("g2")))
      << "from our own seat the card we played could have been the b1 or the "
         "g1, so both continuations stand";
  EXPECT_GT(infs.length(), 2);
}
