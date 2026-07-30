// Reactor0's positional dispatcher: a clue to Bob is ALWAYS stable — even
// if Bob is loaded — and a clue to Cathy is ALWAYS reactive, even in stall
// contexts. Both cases diverge from reactor's heuristic dispatcher.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

TEST(Reactor0Dispatch, ClueToLoadedBobIsStillStable) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},   // Alice (us)
      {"r1", "y3", "g4", "b2", "p4"},   // Bob
      {"g1", "y2", "b3", "p2", "r4"},   // Cathy
  };
  use_reactor0(opts);
  Game g = setup(opts);
  // Load Bob: slot 1 clued "1" => an obvious playable.
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"1"});

  g = take_turn(std::move(g), "Alice clues red to Bob");

  // Reactor's dispatcher would consider a reactive reading for a clue that
  // touches a loaded Bob; reactor0 never does.
  EXPECT_TRUE(g.waiting.empty()) << "stable clues install no WC in reactor0";
  EXPECT_NE(last_clue_interp(g), ClueInterp::REACTIVE);
}

TEST(Reactor0Dispatch, ClueToCathyAtEightCluesIsStillReactive) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y3", "g4", "b2", "p4"},
      {"g1", "y2", "b3", "p2", "r4"},
  };
  opts.clue_tokens = 8;  // stall context in reactor
  use_reactor0(opts);
  Game g = setup(opts);
  // Load Bob so reactor's stall branch would have routed this stable.
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"1"});

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().reacter, static_cast<int>(TestPlayer::BOB));
  EXPECT_EQ(g.waiting.front().focus_slot, 2) << "the anchor is the clue value";
}

TEST(Reactor0Dispatch, ClueToCathyFromLockedGiverIsStillReactive) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y3", "g4", "b2", "p4"},
      {"g1", "y2", "b3", "p2", "r4"},
  };
  opts.clue_tokens = 4;
  use_reactor0(opts);
  Game g = setup(opts);
  // Lock Alice (the giver): every card chop-moved.
  for (int o : g.state.hands[0]) {
    g.with_meta(o, [](ConvData& m) { m.status = CardStatus::CHOP_MOVED; });
  }

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
}
