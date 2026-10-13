// THE REACTER READS ITS OWN CARD BY ELIMINATION (tiiah/CONVENTION.md §1d, v18.9.0;
// human diagnostic 2013726 T27).
//
// Through the bucket the team reads the reacter's card as one identity. When the
// reacter sees every copy of it, that reading is empty at the reacter's own seat, and
// core rule 2 makes the card any playable on its stacks, less the identity of the
// receiver's target (both would play it). Until v24.0.0 the motivating replay, a
// Brown (4 Suits) game, was the test; with one bucket per suit its pairing reads the
// reacter's r4 directly, so this fixture stands in for it.
#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

// Five suits (r y g b p, one bucket each), red on 4, blue and purple done. We are BOB,
// the reacter, and cannot see our hand. Alice's 1 to Cathy pairs our slot 5 with
// Cathy's slot 1, the y1 (bucket 1). A rank clue on turn 1 (an odd epoch) puts our
// card one bucket below, in red, whose one playable is the r5 -- and Alice holds the
// only r5. So for us the bucket reading is empty: our card is any playable left on
// our stacks, the y1 and the g1, less the y1 Cathy's target is.
TEST(TiiahReacterOwnCardByElimination, TheBucketReadingWeSeeElsewhereFallsToAnyPlayable) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.play_stacks = std::vector<int>{4, 0, 0, 5, 5};
  opts.hands = {{"r5", "g3", "y3", "y4", "g4"},
                {"xx", "xx", "xx", "xx", "xx"},
                {"y1", "r3", "g2", "p2", "b3"}};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 5;
  // Our seat is set here, with the convention: `use_tiiah` would replace `init`.
  opts.init = [](Game& g) {
    g.convention = Convention::TIIAH;
    g.state.our_player_index = 1;
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 1 to Cathy");

  const int ours = order_at(g, TestPlayer::BOB, 5);
  ASSERT_FALSE(g.waiting.empty()) << "guard: a reactive, us reacting";
  ASSERT_EQ(g.waiting.front().react_order, ours);
  ASSERT_EQ(g.meta[ours].status, CardStatus::CALLED_TO_PLAY);

  const Identity r5 = g.state.expand_short("r5");
  EXPECT_EQ(g.common.thoughts[ours].inferred, IdentitySet::single(r5))
      << "the team reads our card through the bucket";
  EXPECT_FALSE(g.players[1].thoughts[ours].possible.contains(r5))
      << "we see the only r5, in Alice's hand";
  EXPECT_EQ(g.players[1].thoughts[ours].inferred,
            IdentitySet::single(g.state.expand_short("g1")))
      << "any playable on our stacks, less the y1 Cathy's target is";

  // And we react with it.
  const PerformAction action = g.take_action();
  const auto* play = std::get_if<PerformPlay>(&action);
  ASSERT_NE(play, nullptr);
  EXPECT_EQ(play->target, ours);
}
