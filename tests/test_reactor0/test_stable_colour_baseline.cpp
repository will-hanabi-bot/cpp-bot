// Defect E (finding #12). stable_colour picks its target with
// `leftmost_could_be_playable`, which falls back to `possible` when
// `inferred` is empty (interpret_clue.cpp:98) -- then hands that target to
// reactor::target_play, which filters `inferred` ONLY
// (reactor/interpret_clue.cpp:139). Empty in, empty out: target_play resets
// the inferences and returns nullopt, and the clue is stamped MISTAKE.
//
// The fix is not in either of those, but upstream and convention-agnostic:
// an emptied `inferred` means an earlier inference chain has been
// contradicted, so the card is reset to its global empathy IMMEDIATELY, in
// Game::on_clue, before any convention interprets the clue. The clue's own
// empathy is already folded into `possible` at that point, so the reset
// lands on exactly "global empathy, then this clue".
//
// With that, no layer ever sees an empty `inferred`, and the position below
// reads as what it plainly is: an ordinary stable play clue on b1.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

TEST(Reactor0StableColourBaseline, EmptyInferredTargetStallsNotMistakes) {
  SetupOptions opts;
  // Bob's slot 3 is b1 -- actually playable. Everything else in Bob's hand
  // is non-blue so the clue's touch set is just that card.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g2", "b1", "r4", "y4"},   // Bob: slot 3 = b1
      {"g1", "y2", "r3", "p2", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // Simulate an earlier convention chain having narrowed slot 3's common
  // inferred to a green singleton. The incoming blue clue intersects that
  // with the blue touch set in on_clue, emptying `inferred` -- while
  // `possible` still admits the playable b1.
  int bob_s3 = order_at(g, TestPlayer::BOB, 3);
  Identity g3{2, 3};
  g.with_thought(bob_s3, [g3](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(g3);
    return out;
  });
  ASSERT_EQ(g.common.thoughts[bob_s3].inferred.length(), 1u)
      << "fixture must leave slot 3 narrowed to a non-blue singleton";

  g = take_turn(std::move(g), "Alice clues blue to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY)
      << "the contradicted inference is discarded and the clue read afresh: "
         "blue on a hand whose only blue is the playable b1 is an ordinary "
         "stable play clue";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_PLAY)
      << "b1 is the play target";
  EXPECT_FALSE(g.common.thoughts[bob_s3].inferred.is_empty())
      << "no layer may be handed a card with an empty inferred set";
}

TEST(Reactor0StableColourBaseline, UntouchedContradictionAlsoResets) {
  SetupOptions opts;
  // Bob's slot 3 is b1. A GREEN clue does not touch it, and differencing the
  // green set out of a green-singleton inference empties it just the same.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g2", "b1", "r4", "y4"},   // Bob: slot 2 green, slot 3 = b1
      {"g1", "y2", "r3", "p2", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  int bob_s3 = order_at(g, TestPlayer::BOB, 3);
  Identity g3{2, 3};
  g.with_thought(bob_s3, [g3](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(g3);
    return out;
  });
  ASSERT_EQ(g.common.thoughts[bob_s3].inferred.length(), 1u);

  g = take_turn(std::move(g), "Alice clues green to Bob");

  EXPECT_FALSE(g.common.thoughts[bob_s3].inferred.is_empty())
      << "being left untouched contradicts an inference just as much as "
         "being touched; the reset must apply to both branches of on_clue";
}
