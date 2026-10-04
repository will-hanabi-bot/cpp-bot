// A watched card one above a pair's row floors that row, struck or not
// (tiiah/CONVENTION.md §1.3, v20.14.0, the user's ruling; replay 2019249 T33).
//
// If both seats of a pair watched a 1 go into the hole, their row for that suit is
// at least 1: either it landed, or it struck as a duplicate of a 1 already down.
// Our OWN stacks are not asked. Here they already have yellow on 1 (as will-bot69's
// did, having privately settled its own hole card as the y1) while the rows have
// it on 0, so our stacks call Bob's y1 a strike -- and until v20.14.0 no row took
// it.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

constexpr int kYellow = 1;

Game ours_ahead_of_the_rows() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "g4", "b3", "p4", "r4"},
      {"y1", "g3", "b4", "p3", "r3"},
  };
  opts.clue_tokens = 6;
  opts.starting = TestPlayer::BOB;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g.with_state([](State& st) {
    st = st.with_stacks({0, 1, 0, 0, 0});  // our own belief: yellow already on 1
  });
  return g;
}

}  // namespace

TEST(TiiahWatchedOne, FloorsTheRowEvenWhenOurStacksCallItAStrike) {
  Game g = ours_ahead_of_the_rows();
  const int cathy = static_cast<int>(TestPlayer::CATHY);
  ASSERT_EQ(g.state.pairwise_play_stacks[cathy][kYellow], 0) << "guard: the row lags";
  ASSERT_FALSE(g.state.is_playable(g.state.expand_short("y1"))) << "guard: we call it dead";

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1, /*reached_the_hole=*/true,
                    "g2");

  EXPECT_GE(g.state.pairwise_play_stacks[cathy][kYellow], 1)
      << "Cathy and we both watched a 1 go in";
}

// Two copies of the same 1 still leave the stack at least 1 in every row both
// watchers share -- never higher than 1 on their account.
TEST(TiiahWatchedOne, TwoCopiesOfAOneLeaveTheRowOnOne) {
  Game g = ours_ahead_of_the_rows();
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1, /*reached_the_hole=*/true,
                    "g2");
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/1, /*reached_the_hole=*/true,
                    "b2");
  const int bob = static_cast<int>(TestPlayer::BOB);
  const int cathy = static_cast<int>(TestPlayer::CATHY);
  EXPECT_EQ(g.state.pairwise_play_stacks[cathy][kYellow], 1);
  EXPECT_EQ(g.state.pairwise_play_stacks[bob][kYellow], 1);
}
