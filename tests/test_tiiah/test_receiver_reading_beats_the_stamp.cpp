// The receiver's bucket reading is judged against the card's PRE-stamp reading
// (tiiah/CONVENTION.md §1d, v16.26.0).
//
// reactor0's `stamp_receiver_call` makes the receiver's call first and narrows it
// to the playables of the frame the giver and the receiver share, knowing nothing
// about buckets. `narrow_receiver_call` then replaces that with §1d's reading on
// the receiver's own worlds. When the receiver knows more than the pair does --
// replay 2011830: will-bot69 had named its own hole card o7 as the g3 by seeing
// both y3s -- the two readings can be disjoint, and the guard that keeps a card
// from being emptied used to compare against the STAMP's set, so the bucket
// reading was thrown away and the bucket-blind one kept.
//
// Alice is us and the receiver, so the clue is built by hand: the harness derives
// which cards a clue touches from their identities, and ours have none.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

// Four suits, so the buckets are {R,Y}, {G}, {B}. Our belief has green on 1; the
// pair (Bob, us) and the team have nothing. Bob clues Blue to us and Cathy reacts
// with her b1 -- bucket 2, a colour clue -- so our called card is in bucket 1: on
// OUR stacks the g2, or the b2 the b1 continues into. Our untouched cards could
// only be the r1, the y1 or the g2, so the stamp's pair-frame reading is {r1,y1}
// and ours is {g2}. Blue anchors on 4 and Cathy reacts on slot 1, so the call is
// our slot 3.
TEST(TiiahReceiverReading, TheBucketReadingReplacesADisjointStamp) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us), the receiver
      {"r4", "y4", "g4", "b4", "g5"},  // Bob, the giver
      {"b1", "y3", "g3", "b3", "r3"},  // Cathy, the reacter
  };
  opts.play_stacks = std::vector<int>{0, 0, 1, 0};
  opts.starting = TestPlayer::BOB;
  opts.clue_tokens = 6;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  // What we know that the others do not: every shared view is a step behind.
  const std::vector<int> zero(4, 0);
  for (auto& row : g.state.pairwise_play_stacks) row = zero;
  for (auto& row : g.state.pairwise_evidence) row = zero;
  g.state.common_play_stacks = zero;
  g.state.common_evidence = zero;

  const std::vector<int> touched{order_at(g, TestPlayer::ALICE, 2)};
  g.catchup = true;
  g.handle_action(ClueAction{static_cast<int>(TestPlayer::BOB),
                             static_cast<int>(TestPlayer::ALICE), touched,
                             BaseClue{ClueKind::COLOUR, 3}});
  g.handle_action(TurnAction{g.state.turn_count,
                             static_cast<int>(TestPlayer::CATHY)});
  g.catchup = false;
  ASSERT_FALSE(g.waiting.empty()) << "guard: the clue read as a reactive";
  ASSERT_EQ(g.waiting.front().receiver, static_cast<int>(TestPlayer::ALICE));

  const IdentitySet untouched = IdentitySet::empty()
                                    .add(g.state.expand_short("r1"))
                                    .add(g.state.expand_short("y1"))
                                    .add(g.state.expand_short("g2"));
  for (int slot : {1, 3, 4, 5}) {
    g.with_thought(order_at(g, TestPlayer::ALICE, slot), [&](const Thought& t) {
      Thought out = t;
      out.inferred = untouched;
      out.possible = untouched;
      return out;
    });
  }

  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/1,
                    /*reached_the_hole=*/true, "y2");

  int called = -1;
  for (int o : g.state.hands[static_cast<int>(TestPlayer::ALICE)]) {
    if (g.meta[o].status == CardStatus::CALLED_TO_PLAY) called = o;
  }
  ASSERT_GE(called, 0) << "guard: the reaction called one of our cards";
  ASSERT_TRUE(g.common.thoughts[called].possible.contains(g.state.expand_short("g2")))
      << "guard: the fixture's call landed on an untouched card";
  EXPECT_EQ(g.common.thoughts[called].inferred,
            IdentitySet::single(g.state.expand_short("g2")))
      << "the stamp's {r1,y1} is bucket-blind; bucket 1 on our stacks is the g2";
}
