// An expendable card on an INVERTED suit is shed with the PLAY button -- a
// pitch -- and can therefore be named as a reactive target.
//
// Reactor0 reads a reactive clue target-first: the receiver's playable cards,
// then (even bucket only) a finesse, then TRASH, each leftmost-first, walking on
// when the reacter's own reaction does not work. Whichever button the receiver
// needs, the parity decides the reacter's: even matches it, odd opposes it.
//
// The trash step used to exclude inverted cards outright, on the grounds that a
// CTD on orange is a chuck-as-play-attempt that strikes on trash. That is true
// of the DISCARD button and blind to the other one. `receiver_ctp_set` has
// always admitted a spare orange on Play (`!playable && !critical`), and
// `slot_elims` -- the deferred negatives -- has always computed this exact
// reading for the reacter's side. Only the SELECTION was missing, so the whole
// clue read as a MISTAKE whenever every expendable card the receiver held was
// orange (replay 1974257 T30).
//
// The four tests below are one 2x2. Only ONE thing differs between the arms:
// whether the receiver's leftmost trash card is on the inverted suit.
//
//                          | receiver sheds it by | so the reacter presses
//   -----------------------+----------------------+------------------------
//   plain trash, odd       | Discard              | Play      (opposite)
//   plain trash, even      | Discard              | Discard   (same)
//   inverted trash, odd    | Play (a pitch)       | Discard   (opposite)
//   inverted trash, even   | Play (a pitch)       | Play      (same)
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// "Orange (5 Suits)" -- r/y/g/b/o, orange inverted. Stacks r1 y0 g0 b0 o1, so:
//   * r1 and o1 are both basic TRASH, one plain and one inverted -- the pair the
//     2x2 turns on;
//   * every card in Bob's hand is genuinely playable, so a call to blind-play
//     lands whatever slot the anchor picks (the giver can see Bob's hand and
//     rejects a clue whose react slot cannot play);
//   * Cathy holds nothing playable and nothing one away, so the reading reaches
//     the trash step in both buckets.
SetupOptions base(std::string cathy_slot1) {
  SetupOptions opts;
  opts.variant_name = "Orange (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {1, 0, 0, 0, 1};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},                          // Alice (us)
      {"r2", "y1", "g1", "b1", "y1"},                          // Bob -- all playable
      {std::move(cathy_slot1), "y4", "g4", "b4", "y3"},        // Cathy
  };
  use_reactor0(opts);
  return opts;
}

const int kBob = static_cast<int>(TestPlayer::BOB);

// The anchor the clue carried, read off the connection rather than recomputed:
// a colour's reactive value comes from a per-variant table, so hardcoding it
// here would be asserting the table rather than the reading.
int anchor_of(const Game& g) {
  return g.waiting.front().focus_slot;
}

// Which of Bob's slots the reading called, and which of Cathy's it paired with.
int called_slot(const Game& g) {
  const auto& hand = g.state.hands[kBob];
  for (size_t i = 0; i < hand.size(); ++i) {
    if (g.meta[hand[i]].status != CardStatus::NONE) return static_cast<int>(i) + 1;
  }
  return -1;
}

// Assert the clue was READ (not a MISTAKE) and landed on the expected pairing.
void expect_target_is_cathys_slot_1(const Game& g) {
  ASSERT_FALSE(g.waiting.empty()) << "the reactive connection must survive";
  const int slot = called_slot(g);
  ASSERT_GT(slot, 0) << "the clue read as a MISTAKE -- no call was stamped on "
                        "the reacter at all";
  EXPECT_EQ(hanabi::reactor::calc_slot(anchor_of(g), slot, 5), 1)
      << "the called slot must pair with Cathy's slot 1, her leftmost trash";
}

}  // namespace

// --- inverted trash: the case the reading could not see -------------------

// Replay 1974257's shape. Cathy sheds her orange 1 by PITCHING it (Play), and
// odd parity opposes, so Bob is called to press Discard.
TEST(Reactor0InvertedTrashTarget, OddParityCallsTheReacterToDiscard) {
  Game g = setup(base("o1"));
  ASSERT_TRUE(g.state.is_basic_trash(Identity{4, 1})) << "guard: orange on 1";

  g = take_turn(std::move(g), "Alice clues green to Cathy");

  expect_target_is_cathys_slot_1(g);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, called_slot(g)),
            CardStatus::CALLED_TO_DISCARD)
      << "a pitch is the Play button, and odd parity opposes it";
}

// The same target in the EVEN bucket: the buttons match, so Bob plays.
TEST(Reactor0InvertedTrashTarget, EvenParityCallsTheReacterToPlay) {
  Game g = setup(base("o1"));

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  expect_target_is_cathys_slot_1(g);
  ASSERT_EQ(anchor_of(g), 3) << "guard: a rank clue's anchor is its rank";
  EXPECT_EQ(called_slot(g), 2) << "calc_slot(3, 1, 5) = 2";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY)
      << "even parity matches the receiver's Play button";
}

// --- plain trash: the controls, and the whole point of the 2x2 ------------

// Swap the o1 for an r1 -- equally trash, equally leftmost, plain -- and both
// buttons flip. If these two moved with the pair above, the tests would be
// measuring "is it trash" rather than "is it inverted".
TEST(Reactor0InvertedTrashTarget, PlainTrashOddParityCallsTheReacterToPlay) {
  Game g = setup(base("r1"));
  ASSERT_TRUE(g.state.is_basic_trash(Identity{0, 1})) << "guard: red on 1";

  g = take_turn(std::move(g), "Alice clues green to Cathy");

  expect_target_is_cathys_slot_1(g);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, called_slot(g)),
            CardStatus::CALLED_TO_PLAY)
      << "a plain card is shed with Discard, and odd parity opposes it";
}

TEST(Reactor0InvertedTrashTarget, PlainTrashEvenParityCallsTheReacterToDiscard) {
  Game g = setup(base("r1"));

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  expect_target_is_cathys_slot_1(g);
  EXPECT_EQ(called_slot(g), 2) << "calc_slot(3, 1, 5) = 2";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_DISCARD)
      << "even parity matches the receiver's Discard button";
}

// --- the even bucket now walks --------------------------------------------

// The even bucket's trash step used to ask for the leftmost candidate ONLY and
// give up if the reacter's side did not work there. It now walks, as the odd
// bucket always has.
//
// Cathy holds two plain trash r1s, at slots 1 and 2. With anchor 3 the leftmost
// pairs onto Bob's slot 2 -- which is a known Orange 5, the single copy, so
// every reading of it is critical and it cannot be thrown. The reading must
// move on to Cathy's slot 2, which pairs onto Bob's slot 1.
TEST(Reactor0InvertedTrashTarget, TheEvenBucketWalksPastAnUnusableReactSlot) {
  SetupOptions opts = base("r1");
  opts.hands[1] = {"y4", "o5", "g1", "b1", "y1"};  // Bob: slot 2 IS the o5
  opts.hands[2] = {"r1", "r1", "y4", "g4", "y3"};  // Cathy: two trash r1s
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, 2, "o5");
  ASSERT_TRUE(g.state.is_critical(Identity{4, 5}))
      << "guard: Orange 5 is the only copy, so Bob cannot spare it";

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  ASSERT_FALSE(g.waiting.empty());
  ASSERT_EQ(called_slot(g), 1)
      << "slot 2 is unthrowable, so the reading walks to Cathy's next trash "
         "card -- before v10.6.0 the even bucket gave up here and the clue "
         "read as a MISTAKE";
  EXPECT_EQ(hanabi::reactor::calc_slot(3, 1, 5), 2);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_DISCARD)
      << "a plain trash target in the even bucket: both press Discard";
}
