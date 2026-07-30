// The reactive lock (rlocks): a reactive dc-target on the receiver's
// OLDEST slot reads as a whole-hand lock — uniformly, in both the colour
// (reacter plays) and rank double-discard (reacter discards) modes, and
// conservatively even when the oldest slot actually holds trash.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Cathy: all good, unique, unplayable — no trash, no dupes, no one-aways.
SetupOptions lock_opts(bool rlocks) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g4", "r1", "b3", "p4"},   // Bob: slot 3 r1 visibly playable
      {"y4", "b4", "g3", "p3", "r3"},   // Cathy
  };
  use_reactor0(opts, rlocks);
  return opts;
}

}  // namespace

TEST(Reactor0ReactiveLock, ColourLockTargetsOldestSlot) {
  Game g = setup(lock_opts(/*rlocks=*/true));

  // Blue to Cathy: anchor 4. No playable, no trash/dupe -> the lock
  // candidate is slot 5: react_slot = (4+5-5)%5 = 4... which is b3, not
  // playable. Use green (anchor 3): react_slot = (3+5-5)%5 = 3 = Bob's r1.
  g = take_turn(std::move(g), "Alice clues green to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 3));

  // Resolution: Bob blind-plays r1; target slot 5 + rlocks -> Cathy locks.
  g = take_turn(std::move(g), "Bob plays r1 (slot 3)", "y1");
  for (int slot = 1; slot <= 5; ++slot) {
    EXPECT_EQ(status_at(g, TestPlayer::CATHY, slot), CardStatus::CHOP_MOVED)
        << "slot " << slot;
  }
}

TEST(Reactor0ReactiveLock, RlocksOffFallsBackToSacrifice) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g4", "r1", "b3", "p4"},   // Bob: slot 3 r1 visibly playable
      {"p4", "b3", "g3", "p3", "r3"},   // Cathy: p4 uniquely deepest away
  };
  use_reactor0(opts, /*rlocks=*/false);
  Game g = setup(opts);

  // Sacrifice ordering: p4 (playable_away 3) uniquely sorts first, so the
  // dc-target is slot 1. Blue (anchor 4): react_slot = (4+5-1)%5 = 3 =
  // Bob's r1, visibly playable.
  g = take_turn(std::move(g), "Alice clues blue to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_PLAY);

  // Resolution: Bob plays r1; target slot 1 is a genuine discard, and no
  // lock fires with rlocks off.
  g = take_turn(std::move(g), "Bob plays r1 (slot 3)", "y1");
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 1), CardStatus::CALLED_TO_DISCARD);
  EXPECT_FALSE(any_status(g, TestPlayer::CATHY, CardStatus::CHOP_MOVED));
}

TEST(Reactor0ReactiveLock, TrashOnOldestSlotStillLocks) {
  SetupOptions opts;
  opts.play_stacks = {{1, 0, 0, 0, 0}};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g4", "y1", "b3", "p4"},   // Bob: slot 3 y1 visibly playable
      {"y4", "b4", "g3", "p3", "r1"},   // Cathy: r1 TRASH on slot 5
  };
  use_reactor0(opts, /*rlocks=*/true);
  Game g = setup(opts);

  // Cathy's only trash sits on the oldest slot. Green (anchor 3):
  // react_slot = (3+5-5)%5 = 3. The receiver cannot tell trash-on-slot-5
  // apart from the lock signal, so the conservative lock reading applies.
  g = take_turn(std::move(g), "Alice clues green to Cathy");
  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);

  g = take_turn(std::move(g), "Bob plays y1 (slot 3)", "y2");
  for (int slot = 1; slot <= 5; ++slot) {
    EXPECT_EQ(status_at(g, TestPlayer::CATHY, slot), CardStatus::CHOP_MOVED)
        << "slot " << slot;
  }
}

TEST(Reactor0ReactiveLock, RankDoubleDiscardLockAlsoLocks) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g4", "y2", "b3", "p4"},   // Bob
      {"y4", "b4", "g3", "p3", "r3"},   // Cathy: all good/unique/unplayable
  };
  use_reactor0(opts, /*rlocks=*/true);
  Game g = setup(opts);

  // Rank 3 to Cathy (touches g3, p3, r3): anchor 3. No play target, no
  // finesse target (everything >= 2 away), no trash/dupe -> the dc phase's
  // lock candidate at slot 5: react_slot = (3+5-5)%5 = 3. Bob DISCARDS
  // slot 3.
  g = take_turn(std::move(g), "Alice clues 3 to Cathy");
  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_DISCARD);

  g = take_turn(std::move(g), "Bob discards y2 (slot 3)", "y1");
  for (int slot = 1; slot <= 5; ++slot) {
    EXPECT_EQ(status_at(g, TestPlayer::CATHY, slot), CardStatus::CHOP_MOVED)
        << "slot " << slot;
  }
}
