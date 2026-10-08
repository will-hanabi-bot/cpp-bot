// The double chuck, and the resolution side of a reverse reactive
// (tiiah/CONVENTION.md §1d).
//
// Every reactive clue here is even parity, so the receiver presses whichever
// button the reacter pressed. That is what makes the double chuck work: when
// the receiver's only playables are on inverted suits, both players press
// Discard, and Discard is the button that stacks an inverted card.
//
// The resolution itself is reactor0's — `wc.even_parity` is bound at clue time
// and `reacter_button_pressed` backs the button out of what landed on the table
// — so these tests are here to prove the wiring holds under a hidden stack,
// where neither player is told what reached it.
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

std::optional<ClueInterp> interp_of(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* c = std::get_if<ClueInterp>(&g.move_history.back())) return *c;
  return std::nullopt;
}

// Five suits plus Orange, which is the inverted one. The non-inverted four are
// what the buckets are built from, and Orange sits in none of them.
SetupOptions orange_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole & Orange (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob: a known r1, and behind it nothing playable but the o1.
      {"r1", "o1", "y4", "g4", "b4"},
      // Cathy: slot 2 is a y3, a card the team can afford to lose.
      {"g3", "y3", "b3", "r3", "y2"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

}  // namespace

// Bob's only playable is the o1, so the walk — which skips inverted cards while
// anything else is available — comes back with it anyway, and the clue is a
// double chuck. Rank 4 anchors on 4, the target is his slot 2, so the reaction
// is slot 2 of Cathy's hand: she is called to DISCARD, not to play.
TEST(TiiahReactions, AnInvertedOnlyHandMakesTheClueADoubleChuck) {
  Game g = setup(orange_opts());
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::CALLED_TO_DISCARD)
      << "both players press Discard, which is what stacks the o1";
  EXPECT_TRUE(g.meta[order_at(g, TestPlayer::CATHY, 2)].urgent);
}

// ...and what the reacter is asked to chuck has to be affordable. The same
// pairing over a card the team cannot spare is refused outright rather than
// retargeted: the reacter cannot see their own hand, so they would chuck it
// whatever the walk decided next (§1g).
TEST(TiiahReactions, ADoubleChuckOverACriticalCardIsRefused) {
  SetupOptions opts = orange_opts();
  opts.hands[2] = {"g3", "y5", "b3", "r3", "y2"};  // slot 2 is the only y5
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  EXPECT_NE(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::NONE);
}

// The resolution. Bob goes first — that is the shape of the reverse reactive —
// then Cathy chucks the slot she was called on, and Bob's o1 is waiting for him
// with the same button on it.
TEST(TiiahReactions, TheReceiverIsCalledToTheButtonTheReacterPressed) {
  Game g = setup(orange_opts());
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");
  g = take_turn(std::move(g), "Alice clues 4 to Bob");
  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);

  // Bob plays the r1 he already knew about; it goes in the hole, unseen.
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "b2");
  // Cathy answers with the Discard button. Her y3 is plain, so it lands in the
  // pile where everyone can see it.
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/2,
                    /*reached_the_hole=*/false, "g2");

  EXPECT_EQ(status_at(g, TestPlayer::BOB, /*slot=*/2),
            CardStatus::CALLED_TO_DISCARD)
      << "his o1 is chucked, which is how an inverted card is played";
}

// The ordinary case of the same wiring: a double PITCH, where the reacter
// presses Play and the receiver is called to play in turn. Four suits, so the
// buckets are {R,Y}, {G}, {B}: Bob's g1 is bucket 1, a colour clue names the
// bucket above for the reacter, and Cathy's b1 (Blue anchors on 4, target slot 2,
// so slot 2) is the pitch.
TEST(TiiahReactions, ADoublePitchCallsTheReceiverToPlay) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "g1", "y4", "b4", "r4"},
      {"y3", "b1", "g3", "b3", "r3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  g = take_turn(std::move(g), "Alice clues blue to Bob");
  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::CALLED_TO_PLAY);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "b2");
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/2,
                    /*reached_the_hole=*/true, "y2");

  EXPECT_EQ(status_at(g, TestPlayer::BOB, /*slot=*/2), CardStatus::CALLED_TO_PLAY)
      << "the g1 the clue was for";
}
