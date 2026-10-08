// The reacter's own card reads the bucket the relation names first, in the worlds
// of its own hole cards (tiiah/CONVENTION.md §1d, §1e; v20.10.0; replay 2018857).
//
// Four suits, so the buckets are {R,Y}, {G}, {B}. Green and blue are on 1. We
// (Alice) throw an unclued card into the hole, so to us red and yellow could each be
// on 1. Our slot-2 card is clued 2. Cathy's 4 to Bob is a reactive with us reacting:
// Bob's g2 (bucket 1) pairs with our 2, so a rank clue names bucket 0 for it -- the
// r2 or the y2, each playable only where our hole card was the r1 or the y1. On the
// frame only the g2 and b2 play, and those are what the stamp writes; the bucket
// reading must replace them, not be tested against them.
//
// The control below still runs on the five-suit fixture.
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

// We throw our slot 5 (into the hole, or onto the discard pile), Bob throws his r4,
// and Cathy gives Bob a 4. Our clued 2 sits in slot 2 when the clue lands.
Game play_out(bool into_the_hole) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g2", "b4", "y4", "g3", "r4"},
      {"y3", "r3", "b3", "p3", "g4"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 1, 1, 0};
  opts.clue_tokens = 5;
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::ALICE, 1, {"2"});
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/5, into_the_hole);
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/5, /*reached_the_hole=*/false,
                    "p4");
  return take_turn(std::move(g), "Cathy clues 4 to Bob");
}

// The same position in four suits (the three-bucket rule).
Game play_out_4_suits(bool into_the_hole) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g2", "b4", "y4", "g3", "r4"},
      {"y3", "r3", "b3", "y5", "g4"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 1, 1};
  opts.clue_tokens = 5;
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::ALICE, 1, {"2"});
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/5, into_the_hole);
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/5, /*reached_the_hole=*/false,
                    "r4");
  return take_turn(std::move(g), "Cathy clues 4 to Bob");
}

IdentitySet set_of(const Game& g, std::initializer_list<const char*> ids) {
  IdentitySet out = IdentitySet::empty();
  for (const char* i : ids) out = out.add(g.state.expand_short(i));
  return out;
}

}  // namespace

TEST(TiiahReacterBucketInTheWorlds, TheBucketReadingInTheWorldsReplacesTheFramesPlayables) {
  Game g = play_out_4_suits(/*into_the_hole=*/true);
  const int ours = order_at(g, TestPlayer::ALICE, 2);

  ASSERT_EQ(g.meta[ours].status, CardStatus::CALLED_TO_PLAY) << "guard: we react";
  EXPECT_EQ(g.common.thoughts[ours].inferred, set_of(g, {"r2", "y2"}))
      << "bucket 0 in the worlds of our hole card, not the frame's g2/b2";
}

// The control: no hole card, so no world plays a bucket-0 2. Our 2 could answer
// Bob's g2 only with the frame's g2 or b2, out of the bucket, and Bob -- reading
// bucket 2 from either -- would find the p1 among his card's possibilities. The
// violation is known and not one the receiver could see through, so since v22.4.0
// the pairing is no pairing, and our 2 is not called (until then it read {g2,b2}).
TEST(TiiahReacterBucketInTheWorlds, WithNoHoleTheOutOfBucketPairingIsNoPairing) {
  Game g = play_out(/*into_the_hole=*/false);
  const int ours = order_at(g, TestPlayer::ALICE, 2);

  EXPECT_NE(g.meta[ours].status, CardStatus::CALLED_TO_PLAY);
}
