// A reaction whose target has left the receiver's hand is no longer urgent (reactor0
// DECISION_MAKING.md Precedence step 2, v9.3.0; restored for Throw It in a Hole in
// v20.7.0; human diagnostic 2018759 T34).
//
// Alice's 3 to Cathy calls Bob's p1 for Cathy's y1. Bob defers, cluing instead. When
// Cathy then plays the y1, the target Bob's slot was paired with is gone: his call
// stands, but it is no longer urgent, so it no longer outranks every clue. The TIIAH
// walk did not record the pairing until v20.7.0, so the call stayed urgent.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Blue on 1. Bob throws his r1 onto the discard pile and draws a clued r2; Cathy
// throws her r4 and draws a y4, leaving the b2 in her slot 2 and the y1 in her slot
// 3. Alice's 3 to Cathy then pairs the y1 with Bob's slot 5, the p1 (the b2's
// pairing, Bob's `{r2}`, plays in no world). Bob defers with a clue to Alice.
Game deferred() {
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
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/5, /*reached_the_hole=*/false,
                    "r2");
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"red", "2"});
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5,
                    /*reached_the_hole=*/false, "y4");
  g = take_turn(std::move(g), "Alice clues 3 to Cathy");
  return take_turn(std::move(g), "Bob clues 5 to Alice (slot 1)");
}

}  // namespace

TEST(TiiahSpentReaction, TheTargetPlayedRelegatesTheReaction) {
  Game g = deferred();
  const int bobs_p1 = order_at(g, TestPlayer::BOB, 5);
  ASSERT_EQ(g.meta[bobs_p1].status, CardStatus::CALLED_TO_PLAY);
  ASSERT_TRUE(g.meta[bobs_p1].urgent) << "guard: a pending reaction";
  ASSERT_EQ(g.meta[bobs_p1].react_target_order, order_at(g, TestPlayer::CATHY, 3))
      << "guard: paired with Cathy's y1";

  // Cathy plays the y1 her call was for.
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/3,
                    /*reached_the_hole=*/true, "g4");

  EXPECT_EQ(g.meta[bobs_p1].status, CardStatus::CALLED_TO_PLAY) << "the call stands";
  EXPECT_FALSE(g.meta[bobs_p1].urgent) << "its target has left Cathy's hand";
}

// The control: Cathy throws some other card, the target is still there, and the
// reaction stays urgent.
TEST(TiiahSpentReaction, WhileTheTargetStaysTheReactionStaysUrgent) {
  Game g = deferred();
  const int bobs_p1 = order_at(g, TestPlayer::BOB, 5);
  ASSERT_TRUE(g.meta[bobs_p1].urgent) << "guard: a pending reaction";

  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5,
                    /*reached_the_hole=*/false, "g4");

  EXPECT_TRUE(g.meta[bobs_p1].urgent);
}
