// Reactor0 stable rank clues: the six-priority ladder — direct play,
// trash reveal (terminal), fix/reveal, lock, referential discard, stall.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

TEST(Reactor0StableRank, DirectPlayOnAllPlayableRank) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "g2", "r3", "b4", "p2"},
      {"g1", "y4", "b3", "p3", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_DISCARD))
      << "a direct play clue is not also a referential discard";
}

TEST(Reactor0StableRank, DirectPlayWithNoNewCardsPromisesLeftmostTouched) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "g2", "r3", "b4", "p2"},
      {"g1", "y4", "b3", "p3", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"1"});

  // Re-cluing 1 touches nothing new; the leftmost touched card that could
  // be playable is promised playable.
  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
}

TEST(Reactor0StableRank, TrashRevealIsTerminal) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g2", "y1", "b4", "p2"},
      {"g2", "y4", "b3", "p3", "r4"},
  };
  opts.play_stacks = {{1, 1, 1, 1, 1}};  // every 1 is trash
  use_reactor0(opts);
  Game g = setup(opts);

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REVEAL);
  EXPECT_TRUE(g.meta[order_at(g, TestPlayer::BOB, 3)].trash);
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_DISCARD))
      << "a trash reveal never falls through to a referential discard";
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CHOP_MOVED));
}

TEST(Reactor0StableRank, TrashRankWithNoNewCardsIsStall) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g2", "y1", "b4", "p2"},
      {"g2", "y4", "b3", "p3", "r4"},
  };
  opts.play_stacks = {{1, 1, 1, 1, 1}};
  use_reactor0(opts);
  Game g = setup(opts);
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"1"});

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::STALL);
}

TEST(Reactor0StableRank, LockSlotTouchLocksTheHand) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r2", "y3", "g4", "b2", "p3"},   // p3 at slot 5 = the lock slot
      {"g1", "y4", "b3", "p2", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::LOCK);
  for (int slot = 1; slot <= 5; ++slot) {
    EXPECT_EQ(status_at(g, TestPlayer::BOB, slot), CardStatus::CHOP_MOVED)
        << "slot " << slot;
  }
}

TEST(Reactor0StableRank, ReferentialDiscardMatchesReactor) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r2", "y3", "g1", "b3", "p4"},
      {"g2", "y4", "b4", "p2", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // Rank 2 touches slot 1 only (newly); not all-playable (r2 needs r1),
  // not trash, not the lock slot -> referential discard: first unclued
  // slot right of the focus.
  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::DISCARD);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_DISCARD);
}

TEST(Reactor0StableRank, NoNewCardsNothingElseIsStall) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r2", "y3", "g1", "b3", "p4"},
      {"g2", "y4", "b4", "p2", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"2"});

  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::STALL);
}
