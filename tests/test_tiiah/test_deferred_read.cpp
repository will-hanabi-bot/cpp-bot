// A clue we cannot read until its receiver settles the reaction it owes us
// (tiiah/CONVENTION.md §1c, v20.12.0, the user's ruling; replay 2019249 T4).
//
// The shape of replay 2019249, seats rotated so that we are will-bot69:
//   T1  Bob clues 1 to us, touching our slots 3 and 4: Cathy reacts, we receive.
//   T2  Cathy defers, cluing 4 to Bob (stable: our 1s are sure plays).
//   T3  We play our slot 3 into the hole. Our other 1 is no longer a sure play.
//   T4  Bob clues Yellow to Cathy. Were Cathy holding a standing play, it would be a
//       reverse reactive with us reacting -- and she owes us a reaction whose card
//       we cannot name. So we wait.
//   T5  Cathy plays her owed card, the r1 in her slot 3. Counted as her standing
//       call, the Yellow re-reads: Cathy's r2 in slot 1 is the leftmost playable once
//       the r1 has played, and Yellow (anchor 2) pairs it with our slot 1.
// The control: Cathy discards instead, so she held no standing play; the Yellow
// re-reads as the stable clue it is in that position, and calls nothing of ours.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

Game through_the_yellow() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g3", "y4", "p3", "g4", "p2"},
      {"r2", "y3", "r1", "b3", "b1"},
  };
  opts.clue_tokens = 7;
  opts.starting = TestPlayer::BOB;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = take_turn(std::move(g), "Bob clues 1 to Alice (slots 3,4)");
  g = take_turn(std::move(g), "Cathy clues 4 to Bob");
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/3, /*reached_the_hole=*/true);
  return take_turn(std::move(g), "Bob clues yellow to Cathy");
}

}  // namespace

TEST(TiiahDeferredRead, WeWaitThenReadTheReverseReactive) {
  Game g = through_the_yellow();
  ASSERT_EQ(g.deferred_reads.size(), 1u) << "guard: the Yellow waits";
  EXPECT_EQ(g.deferred_reads[0].standing, -2);
  EXPECT_NE(status_at(g, TestPlayer::ALICE, 1), CardStatus::CALLED_TO_PLAY)
      << "nothing is read yet";

  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/3, /*reached_the_hole=*/true,
                    "g2");
  EXPECT_GE(g.deferred_reads[0].standing, 0) << "resolved by Cathy's play";
  EXPECT_EQ(status_at(g, TestPlayer::ALICE, 1), CardStatus::CALLED_TO_PLAY)
      << "our slot 1 answers Cathy's r2";
}

TEST(TiiahDeferredRead, ADiscardLeavesNoStandingPlay) {
  Game g = through_the_yellow();
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5, /*reached_the_hole=*/false,
                    "g2");
  ASSERT_EQ(g.deferred_reads.size(), 1u);
  EXPECT_EQ(g.deferred_reads[0].standing, -1);
  EXPECT_NE(status_at(g, TestPlayer::ALICE, 1), CardStatus::CALLED_TO_PLAY);
}
