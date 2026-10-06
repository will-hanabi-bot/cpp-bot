// A stable call is read on the GIVER's frame in each world of the receiver's own
// hole cards (tiiah/CONVENTION.md §1e, v21.0.0, the user's ruling; replay 2020406).
//
// Cathy throws a `{p1,y1}` into the hole -- we watch it land as the p1, she cannot
// name it. We then throw our own `{r2,p2}`, which Cathy can see. Where ours was the
// p2, it lands only if her card was the p1, so she knows purple is on 2, and her
// Purple to us calls the p3. Where ours was the r2, she cannot know, her frame has
// purple on 0, and Purple calls the p1. The call reads `{p1,p3}`, and since a world
// exists in which it is not the dupe of her p1 we watched, it stands.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

constexpr Identity kR2{0, 2}, kY1{1, 1}, kY2{1, 2}, kP1{4, 1}, kP2{4, 2}, kP3{4, 3};

void read_as(Game& g, int order, const IdentitySet& set) {
  g.state.deck[order].clued = true;
  g.with_thought(order, [&set](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    out.possible = set;
    return out;
  });
}

// Red on 1, purple on 0. `ours` is what our own hole card could be.
Game position(IdentitySet ours) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.play_stacks = std::vector<int>{1, 0, 0, 0, 0};
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"r4", "g4", "b4", "y4", "r3"},
                {"p1", "g3", "b3", "y3", "r5"}};
  opts.starting = TestPlayer::CATHY;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  read_as(g, order_at(g, TestPlayer::CATHY, 1), IdentitySet::empty().add(kP1).add(kY1));
  g = hidden_action(std::move(g), TestPlayer::CATHY, 1, /*reached_the_hole=*/true, "b2");
  read_as(g, order_at(g, TestPlayer::ALICE, 5), ours);
  g = hidden_action(std::move(g), TestPlayer::ALICE, 5, /*reached_the_hole=*/true);
  g = hidden_action(std::move(g), TestPlayer::BOB, 5, /*reached_the_hole=*/false, "b1");
  return take_turn(std::move(g), "Cathy clues purple to Alice (slot 2)");
}

}  // namespace

TEST(TiiahGiverFramePerWorld, ThePurpleIsTheP1OrTheP3) {
  Game g = position(IdentitySet::empty().add(kR2).add(kP2));
  const int called = order_at(g, TestPlayer::ALICE, 2);
  EXPECT_EQ(g.meta[called].status, CardStatus::CALLED_TO_PLAY)
      << "a world exists in which the call is not the watched dupe";
  EXPECT_EQ(g.common.thoughts[called].inferred, IdentitySet::empty().add(kP1).add(kP3));
}

// Control: our hole card cannot be a purple card, so no world tells Cathy her own
// card. Her frame has purple on 0 throughout, the call is the p1, and we watched her
// p1 go in: the named dupe (v18.18.0) -- withdrawn.
TEST(TiiahGiverFramePerWorld, NothingToDeduceLeavesTheNamedDupe) {
  Game g = position(IdentitySet::empty().add(kR2).add(kY2));
  const int called = order_at(g, TestPlayer::ALICE, 2);
  EXPECT_NE(g.meta[called].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[called].inferred, IdentitySet::single(kP1));
}
