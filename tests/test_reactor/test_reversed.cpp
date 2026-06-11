// Behavioural tests for "Reversed" suits (stack plays 5 → 4 → 3 → 2 → 1).
//
// Reversed is orthogonal to the existing `inverted` flag (Orange's
// action-button swap). With reversed support, `State::is_playable`,
// `is_useful`, `is_basic_trash`, `playable_away`, and the score/max_score
// arithmetic all become direction-aware. The convention layer reads
// those helpers exclusively, so reactive/stable interpretations
// automatically respect the reversed direction.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

// User-supplied example: in a 5-suit game with Purple Reversed and
// stacks empty, Cathy holds `r3 p5 b1 b2 y1`. Alice clues rank-2 to
// Cathy → focus_slot=4 (b2). The convention's play-target loop picks
// the leftmost playable (= the reversed p5 at slot 2 — playable
// because the reversed stack starts "above 5"). `calc_slot(4, 2, 5) =
// 2` → Bob's slot 2 = react_order. Bob is called to play.
TEST(Reversed, RankClueReactiveFocusesPlayableReversed5) {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer + giver): filler that doesn't collide with
      // anyone else's cards.
      {"r1", "y2", "g1", "g2", "b3"},
      // Bob (reacter): the play-target loop's `target_play` requires
      // Bob's react_order's actual id to be in the playable_set
      // (`has_consistent_infs` check). Slot 2 must therefore be an
      // actually-playable card on the current stacks. We use "y1"
      // (yellow stack is at 0 → y1 playable).
      {"r4", "y1", "g4", "y3", "g3"},
      // Cathy (receiver): the user's exact example, with Purple
      // Reversed in place of "white reversed".
      // Slot 1=r3, 2=p5, 3=b1, 4=b2, 5=y1.
      {"r3", "p5", "b1", "b2", "y1"},
  };
  opts.variant_name = "Reversed (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  // Sanity: p5 must be playable on the initial reversed stack.
  ASSERT_TRUE(g.state.is_playable(Identity{4, 5}))
      << "Purple Reversed 5 must be playable when the reversed stack "
         "is empty (stack initialised to 6 in `State::create`)";

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  ASSERT_FALSE(g.move_history.empty());
  ASSERT_TRUE(std::holds_alternative<ClueInterp>(g.move_history.back()));
  // ClueInterp::REACTIVE == 1.
  EXPECT_EQ(static_cast<int>(std::get<ClueInterp>(g.move_history.back())), 1)
      << "rank-2 to Cathy must interpret as REACTIVE";

  int bob_slot2 = g.state.hands[static_cast<int>(TestPlayer::BOB)][1];
  int cathy_slot2 = g.state.hands[static_cast<int>(TestPlayer::CATHY)][1];
  EXPECT_EQ(g.meta[bob_slot2].status, CardStatus::CALLED_TO_PLAY)
      << "Bob's slot 2 (= reacter's react_order via calc_slot) must "
         "be CTP'd by the reactive interp";
  EXPECT_EQ(g.meta[cathy_slot2].status, CardStatus::CALLED_TO_PLAY)
      << "Cathy's slot 2 (= the reversed p5, receiver's target) must "
         "also be CTP'd via the v0.16 receiver-target stamp";
}

// Sanity: rank-1 on a reversed suit is the FINAL rank, not currently
// playable on the initial reversed stack. Just confirm the state-
// level semantic: is_playable and is_basic_trash answer correctly,
// AND clue_touched still works in a Funnels/Cocoa-free variant
// (i.e., the convention layer reads the right helpers).
TEST(Reversed, ReversedRank1IsFinalNotInitiallyPlayable) {
  // Construct directly to bypass the test_harness `play_stacks`
  // pre-seed loop, which assumes normal-direction stacks and tries
  // to base_count r=1..stack — broken for reversed stacks.
  const Variant& v = get_variant("Reversed (5 Suits)");
  TableOptions topts;
  topts.num_players = 3;
  topts.variant_name = "Reversed (5 Suits)";
  State s = State::create({"Alice", "Bob", "Cathy"}, /*our_player_index=*/0, v,
                            std::move(topts));

  // suit 4 = Purple Reversed. Initial stack = 6 (sentinel "nothing
  // played"); next playable = 5.
  EXPECT_TRUE(s.is_playable(Identity{4, 5}));
  EXPECT_FALSE(s.is_playable(Identity{4, 1}));
  // p1 is NOT basic trash either — it's the final rank still
  // reachable on the reversed stack (rank ≥ max_ranks=1).
  EXPECT_FALSE(s.is_basic_trash(Identity{4, 1}));
  // After playing p5..p2, p1 becomes the playable.
  s = s.with_play(Identity{4, 5});
  s = s.with_play(Identity{4, 4});
  s = s.with_play(Identity{4, 3});
  s = s.with_play(Identity{4, 2});
  EXPECT_TRUE(s.is_playable(Identity{4, 1}))
      << "after playing p5..p2 on the reversed stack, p1 becomes the next play";
  // Once p1 plays, the stack is complete; p1 is basic trash and the
  // suit contributes 5 to the score.
  s = s.with_play(Identity{4, 1});
  EXPECT_TRUE(s.is_basic_trash(Identity{4, 1}));
  EXPECT_EQ(s.played_count(4), 5);
}
