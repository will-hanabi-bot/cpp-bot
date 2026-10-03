// Rank referential discards on the lock slot (CONVENTION.md §1c priorities 5/6;
// the user's convention, v20.0.0).
//
// In Clue Starved (and Throw It in a Hole), a stable rank clue that touches the
// lock slot is an ordinary referential discard whenever it has a target -- the
// first unclued card right of the leftmost newly touched one -- and a lock only
// when it has none. Every other variant keeps the lock
// (`Reactor0StableRank.LockSlotTouchLocksTheHand` is that control).
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

SetupOptions starved(std::vector<std::string> bob) {
  SetupOptions opts;
  opts.variant_name = "Clue Starved (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      std::move(bob),
      {"g1", "y4", "b3", "p2", "r4"},
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

// A 3 touches slot 2 and slot 5 (the lock slot). Slot 3 is the first unclued card
// right of the leftmost new touch, so it is called to discard: no lock.
TEST(Reactor0RankRefDiscardOnLockSlot, ALockSlotClueWithATargetIsARefDiscard) {
  Game g = setup(starved({"r2", "y3", "g4", "b2", "p3"}));

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::DISCARD);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_DISCARD);
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CHOP_MOVED)
      << "the hand is not locked";
}

// A 3 touches only the lock slot: nothing lies to its right, so there is no target,
// and it is a lock as before.
TEST(Reactor0RankRefDiscardOnLockSlot, ALockSlotClueWithNoTargetIsStillALock) {
  Game g = setup(starved({"r2", "y4", "g4", "b2", "p3"}));

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::LOCK);
  for (int slot = 1; slot <= 5; ++slot) {
    EXPECT_EQ(status_at(g, TestPlayer::BOB, slot), CardStatus::CHOP_MOVED) << "slot " << slot;
  }
}
