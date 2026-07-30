// Reactor0 colour reactives (one play). Anchors come from the fixed
// colour-value table (red=1, yellow=2, green=3, blue=4, purple=5). Mode 1:
// receiver has a playable -> reacter discards, receiver plays. Mode 2: no
// playable -> reacter BLIND-PLAYS the react slot and the receiver
// discards the dc-target.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

TEST(Reactor0ReactiveColour, ReacterDiscardsReceiverPlays) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y2", "y3", "b3", "g4", "y4"},   // Bob
      {"r1", "y2", "g3", "b4", "p2"},   // Cathy: r1 playable at slot 1
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // Red to Cathy: anchor 1, target slot 1 -> react_slot = (1+5-1)%5 = 5.
  g = take_turn(std::move(g), "Alice clues red to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().focus_slot, 1) << "red = 1";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_DISCARD);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 5));
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 1), CardStatus::CALLED_TO_PLAY);
}

TEST(Reactor0ReactiveColour, KnownCriticalReactSlotAdvancesTarget) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y2", "y3", "b3", "g4", "p5"},   // Bob: slot 5 known 5 = critical
      {"r1", "y2", "g1", "b4", "p2"},   // Cathy: playables r1 (1), g1 (3)
  };
  use_reactor0(opts);
  Game g = setup(opts);
  // Bob's slot 5 known to be a 5 -> known critical, must not be discarded.
  g = pre_clue(std::move(g), TestPlayer::BOB, 5, {"5"});

  // Red to Cathy: r1 target would react at slot 5 (critical) -> advance to
  // g1 at slot 3: react_slot = (1+5-3)%5 = 3.
  g = take_turn(std::move(g), "Alice clues red to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::NONE)
      << "the known-critical react slot is skipped";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_DISCARD);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_PLAY);
}

TEST(Reactor0ReactiveColour, BlindPlayPointsAtTrash) {
  SetupOptions opts;
  opts.play_stacks = {{1, 0, 0, 0, 0}};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "y3", "b3", "g4", "y4"},   // Bob: slot 1 y1 is playable
      {"y4", "r1", "b4", "g4", "p4"},   // Cathy: r1 trash at slot 2
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // Green to Cathy (touches g4): anchor 3, dc-target slot 2 ->
  // react_slot = (3+5-2)%5 = 1. Bob blind-plays slot 1 (visibly y1,
  // playable, so the giver may give this).
  g = take_turn(std::move(g), "Alice clues green to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 1));
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_DISCARD));

  // Resolution: Bob plays y1 -> colour + reacter play = receiver discard.
  g = take_turn(std::move(g), "Bob plays y1 (slot 1)", "r3");
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::CALLED_TO_DISCARD);
}

TEST(Reactor0ReactiveColour, SameHandDupeIsADcTarget) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "y3", "b3", "g4", "y4"},
      {"y4", "b2", "g4", "b2", "p4"},   // Cathy: b2 duped at slots 2 and 4
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // No playable in Cathy's hand; leftmost trash-or-dupe is b2 at slot 2.
  // Green (anchor 3): react_slot = (3+5-2)%5 = 1; Bob's slot-1 y1 is the
  // visible blind play.
  g = take_turn(std::move(g), "Alice clues green to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.waiting.front().react_order, order_at(g, TestPlayer::BOB, 1));
}

TEST(Reactor0ReactiveColour, VisiblyUnplayableBlindSlotIsMistake) {
  SetupOptions opts;
  opts.play_stacks = {{1, 0, 0, 0, 0}};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "y2", "b3", "g4", "y4"},   // Bob: slot 1 y3 NOT playable
      {"y4", "r1", "b4", "g4", "p4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // Same shape as BlindPlayPointsAtTrash but the react slot is visibly
  // unplayable — every observer who can see Bob's card rejects the clue.
  g = take_turn(std::move(g), "Alice clues green to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::MISTAKE);
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
}
