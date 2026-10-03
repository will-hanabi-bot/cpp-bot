// Rank referential discards on the lock slot, in Throw It in a Hole (reactor0
// CONVENTION.md §1c priorities 5/6; the user's convention, v20.0.0).
//
// The user's examples, on yagami_black's hand in replay 2017568 -- here Bob's:
// `g5 r5 g2 p1 i4`, pink on 0. A 4 touches only the i4 on the lock slot: no
// target, a lock. A 5 touches slots 1, 2 and 5: slot 3 is called to discard.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

SetupOptions pink(std::vector<std::string> bob) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole & Pink (6 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      std::move(bob),
      {"r4", "y4", "g4", "b4", "p4"},
  };
  opts.clue_tokens = 5;
  use_tiiah(opts);
  return opts;
}

CardStatus bob_status(const Game& g, int slot) {
  return g.meta[order_at(g, TestPlayer::BOB, slot)].status;
}

}  // namespace

TEST(TiiahRankRefDiscardOnLockSlot, AFourOnlyOnTheLockSlotIsALock) {
  Game g = setup(pink({"g5", "r5", "g2", "p1", "i4"}));

  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  for (int slot = 1; slot <= 5; ++slot) {
    EXPECT_EQ(bob_status(g, slot), CardStatus::CHOP_MOVED) << "slot " << slot;
  }
}

TEST(TiiahRankRefDiscardOnLockSlot, AFiveWithATargetCallsSlotThreeToDiscard) {
  Game g = setup(pink({"g5", "r5", "g2", "p1", "i4"}));

  g = take_turn(std::move(g), "Alice clues 5 to Bob");

  EXPECT_EQ(bob_status(g, 3), CardStatus::CALLED_TO_DISCARD);
  EXPECT_NE(bob_status(g, 1), CardStatus::CHOP_MOVED) << "not a lock";
}

// The pink promise belongs to a lock. A 5 on a hand whose lock slot is an i3 used to
// be read as a broken promise; with a referential target it is a discard, which
// promises nothing.
TEST(TiiahRankRefDiscardOnLockSlot, ARefDiscardOnTheLockSlotMakesNoPinkPromise) {
  Game g = setup(pink({"g5", "r5", "g2", "p1", "i3"}));

  g = take_turn(std::move(g), "Alice clues 5 to Bob");

  EXPECT_EQ(bob_status(g, 3), CardStatus::CALLED_TO_DISCARD);
}
