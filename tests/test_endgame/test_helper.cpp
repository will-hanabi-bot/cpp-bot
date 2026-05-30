// Port of python-bot/tests/test_endgame/test_helper.py.
#include <algorithm>

#include <gtest/gtest.h>

#include "hanabi/basics/identity.h"
#include "hanabi/endgame/helper.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::endgame;
using namespace hanabi::test;

namespace {

// Setup matching the Python tests' standard 3-player No Variant frame.
SetupOptions three_p() {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "r5", "g3", "b4", "p2"},
      {"y4", "y5", "g4", "b5", "p5"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 0};
  return opts;
}

}  // namespace

TEST(EndgameHelper, FindMustPlaysReturnsOnlyUsefulLoneCopies) {
  // r5 is critical (1 copy in deck) AND Bob holds the only one → must-play.
  Game g = setup(three_p());
  const auto& bob_hand = g.state.hands[static_cast<int>(TestPlayer::BOB)];
  auto must = find_must_plays(g.state, bob_hand);
  bool found_r5 = false;
  for (Identity id : must) {
    if (id.suit_index == 0 && id.rank == 5) {
      found_r5 = true;
      break;
    }
  }
  EXPECT_TRUE(found_r5);
}

TEST(EndgameHelper, UnwinnableStatePaceNegative) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y2", "g3", "b4", "p5"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 0};
  Game g = setup(std::move(opts));
  // Force a negative-pace state: cards_left=0, max_ranks lowered to 2.
  g.state.cards_left = 0;
  g.state.max_ranks = {2, 2, 2, 2, 2};
  g.state.play_stacks = {0, 0, 0, 0, 0};
  EXPECT_TRUE(unwinnable_state(g.state, 0));
}

TEST(EndgameHelper, UnwinnableStateNormalMidGameIsFalse) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y2", "g3", "b4", "p5"},
  };
  Game g = setup(std::move(opts));
  EXPECT_FALSE(unwinnable_state(g.state, 0));
}

TEST(EndgameHelper, TriviallyWinnableReturnsErrorOutsideEndgame) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y2", "g3", "b4", "p5"},
  };
  Game g = setup(std::move(opts));
  // endgame_turns is nullopt → trivially_winnable returns found=false.
  auto r = trivially_winnable(g, 0);
  EXPECT_FALSE(r.found);
}
