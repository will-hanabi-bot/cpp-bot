// The Actionable Superposition Collapse Rule in the reactive walk
// (tiiah/CONVENTION.md §1e, v20.6.0; human diagnostic 2018541 T26).
//
// Bob throws his r1 into the hole unnamed, so the frame Alice and Bob share keeps red
// on 0. Alice's 3 to Cathy walks Cathy's b2 first: the sum rule pairs it with Bob's
// slot 1, clued down to `{r2}`, which cannot play on that frame. Before walking on
// to Cathy's y1, the walk asks the worlds: where Bob's hole card was the r1, the r2
// plays. So the b2 pairing is the reading, Bob's slot 1 is called, and the hole
// collapses to the r1. With no hole card, the walk goes on to the y1 as before.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Blue on 1. Bob throws his slot 5 (the r1), into the hole or onto the discard pile,
// and draws the r2, clued red and 2. Cathy throws her r4 and draws a y4, leaving the
// b2 in her slot 2 and the y1 in her slot 3. Then Alice gives the 3 to Cathy.
Game play_out(bool into_the_hole) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g4", "b4", "y3", "p1", "r1"},
      {"b2", "y1", "g3", "p4", "r4"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 0, 1, 0};
  opts.clue_tokens = 5;
  opts.starting = TestPlayer::BOB;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/5, into_the_hole, "r2");
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"red", "2"});
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5,
                    /*reached_the_hole=*/false, "y4");
  return take_turn(std::move(g), "Alice clues 3 to Cathy");
}

}  // namespace

TEST(TiiahAscrWalk, AReacterCardPlayableInAWorldTakesTheFirstTarget) {
  Game g = play_out(/*into_the_hole=*/true);

  const int bobs_r2 = order_at(g, TestPlayer::BOB, 1);
  const int bobs_slot5 = order_at(g, TestPlayer::BOB, 5);
  EXPECT_EQ(g.meta[bobs_r2].status, CardStatus::CALLED_TO_PLAY)
      << "the b2 pairing, read in the world where Bob's hole card was the r1";
  EXPECT_EQ(g.common.thoughts[bobs_r2].possibilities(),
            IdentitySet::single(g.state.expand_short("r2")));
  EXPECT_NE(g.meta[bobs_slot5].status, CardStatus::CALLED_TO_PLAY)
      << "not the y1 pairing behind it";
  EXPECT_EQ(g.state.common_play_stacks[0], 1) << "the hole collapses to the r1";
}

// The control: Bob's 1 went to the discard pile, so no world makes the r2 play and the
// walk goes on to the y1, pairing Bob's slot 5 (a p1: purple is the bucket below red/yellow).
TEST(TiiahAscrWalk, WithNoWorldTheWalkGoesOnToTheNextTarget) {
  Game g = play_out(/*into_the_hole=*/false);

  const int bobs_r2 = order_at(g, TestPlayer::BOB, 1);
  const int bobs_slot5 = order_at(g, TestPlayer::BOB, 5);
  EXPECT_NE(g.meta[bobs_r2].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.meta[bobs_slot5].status, CardStatus::CALLED_TO_PLAY);
}
