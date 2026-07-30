// Reactor0 rank reactives (even plays). The anchor is the RANK itself:
// react_slot + target_slot ≡ rank (mod 5). Phase A double play (leftmost
// playable, INCLUDING already-CTP'd targets), Phase B finesse (leftmost
// one-away), Phase C double discard.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

SetupOptions base_opts() {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},   // Alice (giver, us)
      {"g1", "y3", "b3", "g4", "y4"},   // Bob (reacter)
      {"r1", "y2", "g3", "b4", "p5"},   // Cathy (receiver)
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

TEST(Reactor0ReactiveRank, DoublePlayLeftmostPlayable) {
  Game g = setup(base_opts());

  // Rank 2 to Cathy: anchor 2. Cathy's leftmost playable is r1 at slot 1,
  // so react_slot = (2 + 5 - 1) % 5 = 1 -> Bob's slot 1 (g1, playable).
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().focus_slot, 2) << "anchor = clue value";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 1));
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.waiting.front().react_order, order_at(g, TestPlayer::BOB, 1));
}

TEST(Reactor0ReactiveRank, AlreadyCtpTargetIsStillTheTarget) {
  Game g = setup(base_opts());
  // Pre-mark Cathy's slot-1 r1 as already CALLED_TO_PLAY — reactor skips
  // such targets, reactor0 deliberately re-targets them.
  g.with_meta(order_at(g, TestPlayer::CATHY, 1),
              [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 1));
  EXPECT_EQ(g.waiting.front().react_order, order_at(g, TestPlayer::BOB, 1));
}

TEST(Reactor0ReactiveRank, VettingWalksToNextLeftmostPlayable) {
  SetupOptions opts = base_opts();
  // Two playables in Cathy's hand: r1 (slot 1) and g1 (slot 3). Bob's
  // computed react slot for the r1 target is slot 1; make that slot
  // visibly unworkable (a known 4 can be neither playable nor a
  // connector), so the convention advances to the g1 target instead:
  // react_slot = (2 + 5 - 3) % 5 = 4.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y4", "y3", "b3", "g1", "b4"},
      {"r1", "y2", "g1", "b4", "p5"},
  };
  Game g = setup(opts);
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"4"});

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::NONE)
      << "the known-4 react slot is skipped by advancing the target";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 4));
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_PLAY);
}

TEST(Reactor0ReactiveRank, FinesseLeftmostOneAway) {
  SetupOptions opts = base_opts();
  // Cathy has no playable; her leftmost one-away is r2 at slot 2. Rank 3
  // to Cathy: anchor 3, react_slot = (3 + 5 - 2) % 5 = 1. Bob's slot 1
  // holds the r1 connector.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y4", "b4", "g4", "y3"},
      {"y3", "r2", "g3", "b3", "p4"},
  };
  Game g = setup(opts);

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 1));
  // The finesse pins the connector.
  expect_infs(g, std::nullopt, TestPlayer::BOB, 1, {"r1"});
}

TEST(Reactor0ReactiveRank, DoubleDiscardWhenNoPlayOrFinesse) {
  SetupOptions opts = base_opts();
  // Stacks r=1: Cathy's r1 (slot 3) is trash; nothing playable, and no
  // one-away in her hand. Rank 4 to Cathy: anchor 4, dc-target slot 3 ->
  // react_slot = (4 + 5 - 3) % 5 = 1. Bob discards slot 1.
  opts.play_stacks = {{1, 0, 0, 0, 0}};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y2", "y3", "b3", "g4", "y4"},
      {"y4", "b4", "r1", "g4", "p4"},
  };
  Game g = setup(opts);

  g = take_turn(std::move(g), "Alice clues 4 to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_DISCARD);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 1));
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY))
      << "double discard promises zero plays";
}

TEST(Reactor0ReactiveRank, ResolutionMarksReceiverTargetOnDiscard) {
  // Receiver-POV decode: Bob clues Cathy... here OUR bot is Alice and the
  // reaction is Bob's discard; verify react_discard applies the dc-target
  // to Cathy (elim_dc_dc path).
  SetupOptions opts = base_opts();
  opts.play_stacks = {{1, 0, 0, 0, 0}};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y2", "y3", "b3", "g4", "y4"},
      {"y4", "b4", "r1", "g4", "p4"},
  };
  Game g = setup(opts);
  g = take_turn(std::move(g), "Alice clues 4 to Cathy");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_DISCARD);

  g = take_turn(std::move(g), "Bob discards y2 (slot 1)", "r3");

  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_DISCARD)
      << "rank + reacter discard resolves the receiver's dc-target";
}
