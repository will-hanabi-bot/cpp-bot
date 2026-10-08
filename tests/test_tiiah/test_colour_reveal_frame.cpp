// A colour play reveal outranks the leftmost newly touched card when it is a
// reveal on the stacks the giver and the receiver share (tiiah/CONVENTION.md §1b,
// v22.5.0; v18.20.0 had required the stacks EVERY seat knows).
//
// The stable ladder reads a clue on the stacks its giver and receiver share
// (§1.3), and on those a previously-clued card may be revealed as playable when it
// is not on the common stacks: a play into the hole that its own player cannot
// name moves the pair's view and not the common one. Replay 2022792 T30 is the
// case (it reversed replay 2015013 T35's ruling). Here Cathy plays an unnamed p1;
// Alice and Bob both watched it, so their view has purple on 1, and nobody's
// common view does.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Bob's slot 5 is a p2 clued with 2, and Purple also newly touches the other p2 on
// his slot 1: both readings call a p2, and which card they call is the test.
SetupOptions reveal_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"p2", "r4", "g4", "b4", "p2"},
      {"y3", "g3", "b3", "r3", "p1"},
  };
  // Not 8, where a clue reads as a stall rather than a play call.
  opts.clue_tokens = 3;
  use_tiiah(opts);
  return opts;
}

}  // namespace

// Only the pair knows purple is on 1, and that is enough: Purple reveals the p2,
// and the leftmost newly touched card is not called.
TEST(TiiahColourRevealFrame, AWatchedRevealOutranksTheLeftmostNewCard) {
  SetupOptions opts = reveal_opts();
  opts.starting = TestPlayer::CATHY;
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 5, {"2"});

  // Unclued, so Cathy cannot name it: the play is superposed.
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5,
                    /*reached_the_hole=*/true, "y5");
  ASSERT_EQ(g.state.common_play_stacks[4], 0) << "nobody can name Cathy's play";
  ASSERT_EQ(g.state.stacks_known_to_both(0, 1)[4], 1)
      << "but Alice and Bob both watched it";

  g = take_turn(std::move(g), "Alice clues purple to Bob");
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_PLAY)
      << "the clued 2 is revealed on the pair's stacks";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY)
      << "the leftmost newly touched card is not called";
}

// The same clue with purple on 1 in full view: the reveal is global, and it
// outranks the leftmost newly touched card as well.
TEST(TiiahColourRevealFrame, AGlobalRevealOutranksTheLeftmostNewCard) {
  SetupOptions opts = reveal_opts();
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 1};
  opts.hands[2] = {"y3", "g3", "b3", "r3", "y4"};
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 5, {"2"});

  g = take_turn(std::move(g), "Alice clues purple to Bob");

  EXPECT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_PLAY)
      << "the clued 2 is revealed as the p2";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
}
