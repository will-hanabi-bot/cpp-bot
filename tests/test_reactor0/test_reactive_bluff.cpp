// When the reacter plays something the pairing did not predict.
//
// The receiver's call is built from what the button they were handed would
// advance, read against the stacks the reacter leaves behind. If that comes out
// EMPTY and the reaction stacked a plain card, the reacter did not play what the
// pairing expected -- and there are exactly two accounts left:
//
//   BLUFF      the receiver's card is one away from playable on the stacks as
//              they were BEFORE the reaction. It lands later, when the card
//              ahead of it does.
//   DUPE BLUFF not even that survives, so the receiver must be holding the
//              other copy of the very card the reacter just played.
//
// In both the CALL IS DROPPED and only the inference is kept: neither card is
// playable now, so pressing the button would strike or throw a card away. The
// ordinary machinery collects each later -- the bluff target when its connector
// lands, the dupe via the chuck list.
//
// Plays only. An empty receiver-CTD set has no such account and goes straight to
// the escalation ladder's `[?]`.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Hand-build the waiting connection the reactive clue left behind. The bluff is
// by definition a reaction the clue-time pairing did not predict, so driving it
// through selection would be building the fixture around the wrong turn: what
// is under test is what the RECEIVER concludes once the reacter has surprised
// them.
void arm_wc(Game& g, int anchor, ClueKind kind, int value, bool even) {
  ReactorWC wc{static_cast<int>(TestPlayer::ALICE),
               static_cast<int>(TestPlayer::BOB),
               static_cast<int>(TestPlayer::CATHY),
               g.state.hands[static_cast<int>(TestPlayer::CATHY)],
               Clue{kind, value, static_cast<int>(TestPlayer::CATHY)},
               anchor,
               /*inverted=*/false,
               g.state.turn_count,
               /*all_plays=*/false};
  wc.even_parity = even;
  g.waiting.clear();
  g.waiting.push_back(std::move(wc));
}

}  // namespace

// The ruling's first worked example. Red on 1, No Variant. Cathy's slot 2 is
// clued as a 3; Bob plays a b1 from slot 1, and `calc_slot(3, 1, 5) == 2` maps
// that to exactly her slot 2.
//
// After the b1 the playables are {r2, y1, g1, b2, p1} -- no 3 among them, so the
// ordinary reading is empty. Unwinding to the stacks as they were, the one-aways
// are {r3, y2, g2, b2, p2}, and only the r3 is a 3.
TEST(Reactor0ReactiveBluff, OneAwayReadingSurvivesAndTheCallIsDropped) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::BOB;
  opts.play_stacks = {1, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us)
      {"b1", "y4", "g4", "p4", "y5"},  // Bob -- slot 1 is the surprise
      {"y3", "r3", "g5", "b5", "p5"},  // Cathy -- slot 2 is the target
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::CATHY, 2, {"3"});
  const int target = order_at(g, TestPlayer::CATHY, 2);
  arm_wc(g, /*anchor=*/3, ClueKind::RANK, 3, /*even=*/true);

  g = take_turn(std::move(g), "Bob plays b1 (slot 1)", "g2");

  expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"r3"});
  EXPECT_EQ(g.meta[target].status, CardStatus::NONE)
      << "the r3 is not playable yet -- pressing Play on it would strike, so "
         "the call is withdrawn and only the inference kept";
  EXPECT_EQ(g.meta[target].note_mark, NoteMark::RESET)
      << "and the withdrawal is noted; without a stamp there is no CTP -> NONE "
         "transition for notes.cpp to catch on its own";
}

// The ruling's second worked example. Stacks r1 y1 g1 b3 p1. Cathy's slot 3 is
// clued as a 4; Bob blind-plays the b4 from slot 1, and `calc_slot(4, 1, 5) == 3`
// maps that to her slot 3.
//
// No 4 is playable afterwards, and the one-aways on the old stacks are
// {r3, y3, g3, b5, p3} -- no 4 there either. The only account left is that
// Cathy holds the other b4.
TEST(Reactor0ReactiveBluff, DupeBluffPinsTheOtherCopy) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::BOB;
  opts.play_stacks = {1, 1, 1, 3, 1};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us)
      {"b4", "y5", "g5", "p5", "r5"},  // Bob -- the blind play
      {"y2", "g2", "b4", "r2", "p2"},  // Cathy -- slot 3 is the other b4
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::CATHY, 3, {"4"});
  const int target = order_at(g, TestPlayer::CATHY, 3);
  arm_wc(g, /*anchor=*/4, ClueKind::RANK, 4, /*even=*/true);

  g = take_turn(std::move(g), "Bob plays b4 (slot 1)", "g3");

  expect_infs(g, std::nullopt, TestPlayer::CATHY, 3, {"b4"});
  EXPECT_EQ(g.meta[target].status, CardStatus::NONE)
      << "the blue stack is on 4 now, so this copy is trash -- the chuck list "
         "collects it by the ordinary rules";
}

// The control. Same shape, but a 3 IS playable after the reaction, so the
// ordinary reading is non-empty and no bluff is read at all: the call stands.
TEST(Reactor0ReactiveBluff, AnOrdinaryReadingIsNotABluff) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::BOB;
  // Green on 2, so g3 is playable throughout and the CTP set is never empty.
  opts.play_stacks = {1, 0, 2, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us)
      {"b1", "y4", "g4", "p4", "y5"},  // Bob
      {"y3", "g3", "r4", "b5", "p5"},  // Cathy -- slot 2 is a playable g3
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::CATHY, 2, {"3"});
  const int target = order_at(g, TestPlayer::CATHY, 2);
  arm_wc(g, /*anchor=*/3, ClueKind::RANK, 3, /*even=*/true);

  g = take_turn(std::move(g), "Bob plays b1 (slot 1)", "g2");

  EXPECT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY)
      << "a playable reading exists, so this is an ordinary double play";
  expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"g3"});
}
