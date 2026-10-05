// A hole card we settle frees our other card's play (tiiah/CONVENTION.md §2k,
// v20.23.0; replay 2019676).
//
// Settling our own hidden play books its copy as played (`with_play`, into
// `base_count`), but nothing reveals the card, so elimination never removes that
// identity from our other cards' readings. When judging whether OUR card plays, an
// identity with no copy left by our own accounting is no candidate.
//
// Here our hole card was `{r5,y5}` and another card of ours reads `{y5,p5}`, with
// yellow and purple on 4. Settled as the y5, yellow is on 5 and the y5 is spent, so
// the other card is a play -- the p5.

#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/tiiah/superposition.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

constexpr Identity kR5{0, 5}, kY5{1, 5}, kP5{4, 5};

void read_as(Game& g, int order, const IdentitySet& set) {
  g.state.deck[order].clued = true;
  g.with_thought(order, [&set](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    out.possible = set;
    return out;
  });
}

// Alice's `{r5,y5}` goes into the hole; her `{y5,p5}` stays.
Game after_the_hidden_play(std::vector<int> stacks, int* hole, int* other) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g2", "b2", "r2", "g3", "b3"},
      {"r3", "y3", "g3", "b3", "p3"},
  };
  opts.play_stacks = std::move(stacks);
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  *hole = order_at(g, TestPlayer::ALICE, 1);
  *other = order_at(g, TestPlayer::ALICE, 2);
  read_as(g, *hole, IdentitySet::empty().add(kR5).add(kY5));
  read_as(g, *other, IdentitySet::empty().add(kY5).add(kP5));
  return hidden_action(std::move(g), TestPlayer::ALICE, 1, /*reached_the_hole=*/true);
}

}  // namespace

TEST(TiiahSettledHoleCardPlay, TheSpentCopyLeavesOurOtherCardAPlay) {
  int hole = -1, other = -1;
  Game g = after_the_hidden_play({4, 4, 0, 0, 4}, &hole, &other);
  ASSERT_TRUE(g.meta[hole].superposed()) << "guard: the hole card is unnamed";

  ASSERT_TRUE(hanabi::tiiah::narrow_superposition(g, hole, IdentitySet::single(kY5)));
  g.elim();

  ASSERT_EQ(g.state.play_stacks[1], 5) << "guard: our yellow is on 5";
  ASSERT_TRUE(g.me().thoughts[other].possibilities().contains(kY5))
      << "guard: the reading itself keeps the y5 -- nothing is eliminated";
  EXPECT_TRUE(g.me().order_playable(g, other)) << "the y5 is spent: it is the p5";
}

// Control: an identity with a copy still unaccounted for stays a candidate. Our
// hole card `{r4,y4}` is settled as a y4, but the other y4 could be our `{y4,p5}`,
// which is then no known play.
TEST(TiiahSettledHoleCardPlay, AnotherCopyLeftKeepsTheIdentity) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g2", "b2", "r2", "g3", "b3"},
      {"r3", "y2", "g3", "b3", "p3"},
  };
  opts.play_stacks = std::vector<int>{3, 3, 0, 0, 4};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  constexpr Identity kR4{0, 4}, kY4{1, 4};
  const int hole = order_at(g, TestPlayer::ALICE, 1);
  const int other = order_at(g, TestPlayer::ALICE, 2);
  read_as(g, hole, IdentitySet::empty().add(kR4).add(kY4));
  read_as(g, other, IdentitySet::empty().add(kY4).add(kP5));
  g = hidden_action(std::move(g), TestPlayer::ALICE, 1, /*reached_the_hole=*/true);
  ASSERT_TRUE(hanabi::tiiah::narrow_superposition(g, hole, IdentitySet::single(kY4)));
  g.elim();
  ASSERT_EQ(g.state.play_stacks[1], 4) << "guard: our yellow is on 4";
  EXPECT_FALSE(g.me().order_playable(g, other)) << "the second y4 may be this card";
}
