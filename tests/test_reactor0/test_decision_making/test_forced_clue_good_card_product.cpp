// §4's pooled rung 4.2-4.5, ranked by the GOOD-CARD PRODUCT (reactor0
// DECISION_MAKING.md §4, v23.19.0, the user's ruling; replay 2026350 T52).
//
// A forced clue (8 tokens, no known play of Alice's). Bob already holds a known
// safe discard, so 4.2 used to be skipped outright, and 4.4's fill-in took the
// turn. Now the four rungs are one pool, and the clue that tells Bob most about
// his good cards wins; ties keep the old order.
#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

void known_as(Game& g, int order, const IdentitySet& set) {
  g.state.deck[order].clued = true;
  g.with_thought(order, [&set](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    out.possible = set;
    return out;
  });
}

int order_at(const Game& g, TestPlayer player, int slot) {
  return g.state.hands[static_cast<int>(player)][slot - 1];
}

// Red on 4, yellow and purple done, green on 3, blue on 2. Bob: an unclued b5
// (his only good card), a trash g1, a trash p3, a known `{r3,r4}` (trash: his safe
// discard) and a clued `{g1,g4}` that is the g1. Cathy has nothing to play.
Game position(bool b5_known) {
  SetupOptions opts;
  opts.play_stacks = std::vector<int>{4, 5, 3, 2, 5};
  opts.clue_tokens = 8;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},      // Alice (us)
      {"b5", "g1", "p3", "r3", "g1"},      // Bob
      {"g5", "b4", "p2", "b1", "g2"},      // Cathy
  };
  opts.starting = TestPlayer::ALICE;
  hanabi::test::reactor0::use_reactor0(opts);
  Game g = setup(std::move(opts));
  known_as(g, order_at(g, TestPlayer::BOB, 4),
           IdentitySet::empty().add(Identity{0, 3}).add(Identity{0, 4}));
  known_as(g, order_at(g, TestPlayer::BOB, 5),
           IdentitySet::empty().add(Identity{2, 1}).add(Identity{2, 4}));
  if (b5_known) {
    known_as(g, order_at(g, TestPlayer::BOB, 1), IdentitySet::single(Identity{3, 5}));
  }
  return g;
}

}  // namespace

// The 5 (a stable discard of the trash g1 that also names the b5) beats the 1
// (a fill-in of the clued g1, which says nothing about the b5).
TEST(Reactor0ForcedClueGoodCardProduct, TheFiveThatNamesTheGoodCardWins) {
  Game g = position(/*b5_known=*/false);
  const PerformAction action = g.take_action();
  const auto* rank = std::get_if<PerformRank>(&action);
  ASSERT_NE(rank, nullptr) << "a rank clue";
  EXPECT_EQ(rank->target, static_cast<int>(TestPlayer::BOB));
  EXPECT_EQ(rank->value, 5);
}

// With the b5 already known, no clue tells Bob more about a good card: the
// products tie, and the old order gives 4.4's fill-in, the 1.
TEST(Reactor0ForcedClueGoodCardProduct, ATieKeepsTheOldOrder) {
  Game g = position(/*b5_known=*/true);
  const PerformAction action = g.take_action();
  const auto* rank = std::get_if<PerformRank>(&action);
  ASSERT_NE(rank, nullptr) << "a rank clue";
  EXPECT_EQ(rank->target, static_cast<int>(TestPlayer::BOB));
  EXPECT_EQ(rank->value, 1);
}
