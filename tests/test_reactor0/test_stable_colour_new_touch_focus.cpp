// A stable colour clue's direct play focuses NEWLY touched cards first
// (reactor0/CONVENTION.md §1b priority 5, v20.4.0; replay 2018435 T11).
//
// The user's rule: unless it is a play reveal, the clue calls the leftmost newly
// touched card that could be playable. Only a clue that touches no new card falls
// back to the cards it re-touches. With new cards touched and none of them able to
// play, the clue calls nothing.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Bob's slot 1 is an r4 already clued red; his slot 2 is a new r1.
Game position(const char* bobs_second) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r4", bobs_second, "y2", "g2", "b2"},
      {"g1", "y4", "b3", "p2", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);
  return pre_clue(std::move(g), TestPlayer::BOB, 1, {"red"});
}

}  // namespace

// Red re-touches the clued slot 1, which could still be the r1, and newly touches
// slot 2. The new card is the focus.
TEST(Reactor0StableColourNewTouchFocus, TheNewCardOutranksARetouchedOneToItsLeft) {
  Game g = position("r1");

  g = take_turn(std::move(g), "Alice clues red to Bob");

  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY)
      << "the newly touched card is called";
  expect_infs(g, std::nullopt, TestPlayer::BOB, 2, {"r1"});
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY)
      << "not the re-touched one";
}

// With no new card touched, the re-touched card is called as before.
TEST(Reactor0StableColourNewTouchFocus, WithNoNewCardTheRetouchedOneIsCalled) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y3", "y2", "g2", "b2"},  // red touches only slot 1, clued red already
      {"g1", "y4", "b3", "p2", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"red"});

  g = take_turn(std::move(g), "Alice clues red to Bob");

  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
}

// New cards touched, none of which can play: the clue calls nothing, even though
// the re-touched slot 1 could still be the r1.
TEST(Reactor0StableColourNewTouchFocus, NewCardsThatCannotPlayMakeAStall) {
  Game g = position("r3");
  // Negative information on the unclued slot 2 -- an earlier 1 missed it -- so once
  // red lands it is `{r2..r5}`, none playable with red on 0.
  const int o = order_at(g, TestPlayer::BOB, 2);
  ASSERT_FALSE(g.state.deck[o].clued) << "guard: slot 2 is new to the red clue";
  g.with_thought(o, [](const Thought& t) {
    Thought out = t;
    out.possible = t.possible.filter([](Identity i) { return i.rank != 1; });
    out.inferred = t.inferred.filter([](Identity i) { return i.rank != 1; });
    return out;
  });

  g = take_turn(std::move(g), "Alice clues red to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::STALL);
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY))
      << "the re-touched slot 1 is not called in the new card's place";
}

// A play reveal still comes first: with red on 1, red fills a clued 2 in as the r2,
// though it also newly touches the r3 beside it.
TEST(Reactor0StableColourNewTouchFocus, APlayRevealStillComesFirst) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r2", "r3", "y2", "g2", "b2"},
      {"g1", "y4", "b3", "p2", "r4"},
  };
  opts.play_stacks = {{1, 0, 0, 0, 0}};
  use_reactor0(opts);
  Game g = setup(opts);
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"2"});

  g = take_turn(std::move(g), "Alice clues red to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REVEAL);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
}
