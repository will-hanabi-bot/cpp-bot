// A pinkish re-touch -- pink tempo, pink trash, pink identity -- rests on the
// stacks EVERY seat knows (tiiah/CONVENTION.md §1b, v19.0.0), not on the stacks
// the giver and receiver share, which the rest of the stable ladder reads.
//
// Cathy plays an unnamed i1 into the hole. Alice and Bob both watched it, so
// their pair view has pink on 1; nobody's common view does. A 1 re-touching
// Bob's known pink card is therefore a pink IDENTITY clue (1 is not down
// globally), not a pink trash clue (it is down for the pair).
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

TEST(TiiahPinkRankClues, APinkReTouchReadsTheGlobalStacks) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole & Pink (6 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g4", "b4", "i1", "r3", "y3"},
      {"y2", "g2", "b2", "r2", "i1"},
  };
  // Not 8, where a clue reads as a stall rather than a play call.
  opts.clue_tokens = 3;
  opts.starting = TestPlayer::CATHY;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"pink"});

  // Unclued, so Cathy cannot name it: the play is superposed.
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5,
                    /*reached_the_hole=*/true, "y5");
  ASSERT_EQ(g.state.common_play_stacks[5], 0) << "nobody can name Cathy's play";
  ASSERT_EQ(g.state.stacks_known_to_both(0, 1)[5], 1)
      << "but Alice and Bob both watched it";

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  const int o = order_at(g, TestPlayer::BOB, 3);
  EXPECT_FALSE(g.meta[o].trash) << "not a pink trash clue: 1 is not down globally";
  EXPECT_EQ(g.common.thoughts[o].possibilities(),
            IdentitySet::single(g.state.expand_short("i1")))
      << "a pink identity clue: the card is a 1";
}
