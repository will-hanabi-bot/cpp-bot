// An unreadable reactive falls back to the worlds (tiiah/CONVENTION.md §1d, the
// user's ruling; v19.3.0).
//
// Bob throws his b1 into the hole without knowing it, so the stacks Alice and Bob
// share still have blue on 0 -- the play was Bob's own, and he cannot name it.
// Alice's 4 to Cathy pairs Bob's slot 2 (an r1, clued red) with Cathy's slot 2 (a
// b2). On the shared frame the b2 is one away, and a finesse would need Bob's card
// to be the b1, which a red card cannot be: no pairing reads. In the world where
// Bob's hole card was the b1, the b2 plays outright, Bob's card is the playable of
// the bucket below (`{r1}`), and the worlds collapse to that one.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

SetupOptions fallback_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y3", "g4", "b4", "b1"},
      {"b2", "g3", "y4", "p4", "r3"},
  };
  opts.clue_tokens = 5;
  opts.starting = TestPlayer::BOB;
  use_tiiah(opts);
  return opts;
}

}  // namespace

TEST(TiiahReactiveWorldFallback, AOneAwayTargetPlayableInAWorldReadsThere) {
  Game g = setup(fallback_opts());
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"red"});
  const int bobs_r1 = order_at(g, TestPlayer::BOB, 1);

  // Bob's b1 goes in unnamed; Cathy throws her r3.
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/5,
                    /*reached_the_hole=*/true, "y4");
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5,
                    /*reached_the_hole=*/false, "r4");
  ASSERT_EQ(g.state.stacks_known_to_both(0, 1)[3], 0)
      << "the pair cannot name Bob's own play";

  g = take_turn(std::move(g), "Alice clues 4 to Cathy");

  EXPECT_EQ(g.meta[bobs_r1].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[bobs_r1].possibilities(),
            IdentitySet::single(g.state.expand_short("r1")));
  EXPECT_EQ(g.state.common_play_stacks[3], 1)
      << "the clue says which world we are in: Bob's hole card was the b1";
}

// The control: nothing in the hole can make the b2 playable, so the clue is as
// unreadable as before.
TEST(TiiahReactiveWorldFallback, WithNoWorldToExplainItTheClueStaysUnreadable) {
  Game g = setup(fallback_opts());
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"red"});
  const int bobs_r1 = order_at(g, TestPlayer::BOB, 1);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/5,
                    /*reached_the_hole=*/false, "y4");
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5,
                    /*reached_the_hole=*/false, "r4");

  g = take_turn(std::move(g), "Alice clues 4 to Cathy");

  EXPECT_NE(g.meta[bobs_r1].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.state.common_play_stacks[3], 0);
}
