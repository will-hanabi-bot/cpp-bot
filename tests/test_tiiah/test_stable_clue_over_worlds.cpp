// A STABLE call is read in every world of the hole cards of the two seats it is
// between (tiiah/CONVENTION.md §1e, v16.24.0): the reading is the union, and the
// half that rests on one world is withdrawn when that world dies.
//
// Replay 2011397 T10 is the case: yagami's Blue named will-bot69's b3 while its
// earlier hole card was {b2,p2}. Read on blue 1 alone it was {b2}, and rule 2 then
// told every seat the hole card was the p2.

#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/tiiah/superposition.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;
using hanabi::tiiah::OpenWorld;

namespace {

constexpr Identity kB2{3, 2}, kB3{3, 3}, kP2{4, 2};

SetupOptions opts_for(std::vector<int> stacks) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b2", "b3", "y4", "g4", "r4"},
      {"r3", "y3", "g3", "y5", "p5"},
  };
  opts.play_stacks = std::move(stacks);
  opts.starting = TestPlayer::BOB;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  return opts;
}

// Bob throws his b2 into the hole reading {b2,p2}; Cathy passes; Alice clues Blue on
// his b3.
Game after_the_clue(int* hole) {
  Game g = setup(opts_for({0, 0, 0, 1, 1}));
  *hole = order_at(g, TestPlayer::BOB, 1);
  g.state.deck[*hole].clued = true;
  g.with_thought(*hole, [](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::empty().add(kB2).add(kP2);
    out.possible = out.inferred;
    return out;
  });
  g = hidden_action(std::move(g), TestPlayer::BOB, 1, /*reached_the_hole=*/true, "y5");
  g = hidden_action(std::move(g), TestPlayer::CATHY, 5, /*reached_the_hole=*/false, "g1");
  return take_turn(std::move(g), "Alice clues blue to Bob");
}

}  // namespace

TEST(TiiahStableClueOverWorlds, TheCallIsTheNextCardInEachWorld) {
  int hole = -1;
  Game g = after_the_clue(&hole);
  const int b3 = order_at(g, TestPlayer::BOB, 2);
  ASSERT_EQ(g.meta[b3].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[b3].inferred, IdentitySet::empty().add(kB2).add(kB3))
      << "the b2 if the hole card was the p2, the b3 if it was the b2";
  EXPECT_TRUE(g.meta[hole].superposed()) << "and no rule collapsed the hole card on it";
  EXPECT_TRUE(g.meta[b3].conditional.has_value()) << "each half remembers its world";
}

TEST(TiiahStableClueOverWorlds, SettlingTheHoleCardWithdrawsTheOtherHalf) {
  int hole = -1;
  Game g = after_the_clue(&hole);
  const int b3 = order_at(g, TestPlayer::BOB, 2);
  ASSERT_TRUE(hanabi::tiiah::narrow_superposition(g, hole, IdentitySet::single(kB2)));
  EXPECT_EQ(g.common.thoughts[b3].inferred, IdentitySet::single(kB3))
      << "the hole card was the b2, so the call is the b3";
}

// The control: with nothing in the hole there is one world, and the call is
// exactly what the ladder read.
TEST(TiiahStableClueOverWorlds, NoHoleCardsNoWidening) {
  Game g = setup(opts_for({0, 0, 0, 2, 1}));
  // Bob throws away his (trash) b2, so Blue touches the b3 alone.
  g = hidden_action(std::move(g), TestPlayer::BOB, 1, /*reached_the_hole=*/false, "y5");
  g = hidden_action(std::move(g), TestPlayer::CATHY, 5, /*reached_the_hole=*/false, "g1");
  g = take_turn(std::move(g), "Alice clues blue to Bob");
  const int b3 = order_at(g, TestPlayer::BOB, 2);
  ASSERT_EQ(g.meta[b3].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[b3].inferred, IdentitySet::single(kB3));
}
