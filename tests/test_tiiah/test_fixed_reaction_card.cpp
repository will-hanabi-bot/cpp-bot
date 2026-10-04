// A reaction card that is FIXED is thrown at once, or the reacter clues -- and
// either way the reaction is off (tiiah/CONVENTION.md §1c, v20.12.0, the user's
// ruling; self-play seed 259).
//
// Seed 259's shape, seats rotated so that we are the seat that waits:
//   T1  Bob clues Blue to us, touching our slots 3 and 5: Cathy reacts, we receive.
//   T2  Cathy defers, cluing 3 to Bob.
//   T3  We play our slot 5 into the hole -- it was the b1.
//   T4  Bob clues Green to Cathy. To Bob it fixes Cathy's owed reaction card, her
//       b1, dead now that ours went in. We cannot name that card: Cathy owes us a
//       reaction, so we wait (the deferred read).
//   T5  Cathy throws the b1. The Green was the fix, the reaction is void, and the
//       b1 already played was our hole card.
// Or at T5 Cathy clues: no reverse reactive (its receiver would have played), and
// the reaction is off for good.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

struct Position {
  Game g;
  int our_hole_card = -1;
  int cathys_b1 = -1;
};

Position through_the_green() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g4", "b4", "y3", "p4", "r4"},
      {"g3", "y4", "b1", "p3", "r2"},
  };
  opts.clue_tokens = 7;
  opts.starting = TestPlayer::BOB;
  use_tiiah(opts);
  Position s;
  s.g = setup(std::move(opts));
  s.g = take_turn(std::move(s.g), "Bob clues blue to Alice (slots 3,5)");
  s.g = take_turn(std::move(s.g), "Cathy clues 3 to Bob");
  s.our_hole_card = order_at(s.g, TestPlayer::ALICE, 5);
  s.g = hidden_action(std::move(s.g), TestPlayer::ALICE, /*slot=*/5, /*reached_the_hole=*/true);
  s.cathys_b1 = order_at(s.g, TestPlayer::CATHY, 3);
  s.g = take_turn(std::move(s.g), "Bob clues green to Cathy");
  return s;
}

bool owed(const Game& g, int receiver) {
  return receiver < static_cast<int>(g.pending_reactions.size()) &&
         g.pending_reactions[receiver].has_value();
}

}  // namespace

TEST(TiiahFixedReaction, AThrownFixedCardMakesTheClueAFix) {
  Position s = through_the_green();
  ASSERT_EQ(s.g.deferred_reads.size(), 1u) << "guard: we wait on the Green";
  ASSERT_TRUE(owed(s.g, 0)) << "guard: Cathy owes us a reaction";

  s.g = hidden_action(std::move(s.g), TestPlayer::CATHY, /*slot=*/3,
                      /*reached_the_hole=*/false, "g2");

  EXPECT_EQ(s.g.deferred_reads[0].thrown, s.cathys_b1);
  EXPECT_FALSE(owed(s.g, 0)) << "the reaction it named was dead: void";
  EXPECT_GE(s.g.state.common_play_stacks[3], 1) << "the b1 is known to be down";
  for (int o : s.g.state.hands[0]) {
    EXPECT_NE(s.g.meta[o].status, CardStatus::CALLED_TO_DISCARD)
        << "no reaction calls any card of ours";
  }
}

TEST(TiiahFixedReaction, AClueTurnsTheReactionOff) {
  Position s = through_the_green();
  ASSERT_TRUE(owed(s.g, 0)) << "guard: Cathy owes us a reaction";

  s.g = take_turn(std::move(s.g), "Cathy clues 4 to Bob");

  EXPECT_EQ(s.g.deferred_reads[0].standing, -1) << "no standing play: not a reverse reactive";
  EXPECT_FALSE(owed(s.g, 0)) << "off for good";
}
