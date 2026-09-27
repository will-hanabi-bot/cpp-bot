// §1e rule 3 has a PAIR form (tiiah/CONVENTION.md, v16.18.0).
//
// Rule 3 rules an identity out of our own hole card when every copy of it is
// accounted for somewhere we can see. It is private, because our own eyes are not
// a partner's — so when it settles a card, our belief moves and no row does.
//
// But the eyes are sometimes shared. A copy in a hand NEITHER of us holds is one we
// both see, and each of us can see that the other sees it, so the deduction is the
// pair's and their row may take it. A copy in either of OUR hands is not: the other
// of us cannot see it.
//
// Replay 2010329: our order 3 read {r4,y1} and both r4 copies sat in will-bot69's
// hand, so yagami ruled the r4 out exactly as we did and held yellow on 1 — which
// is the frame its rank 2 at T14 had to be read in.
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

namespace {

// Our slot 1 reads {r4,y1}: the 2010329 shape, with red already on 3 so the r4 is
// the other thing the reading could be. Alice's hand carries no identities — ours
// never does, and naming them would let the engine resolve our own play.
SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.play_stacks = {3, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us)
      {"g4", "b4", "p4", "g3", "b3"},  // Bob: no r4 of his own
      {"r4", "r4", "y4", "g5", "b5"},  // Cathy: BOTH copies, where both of us see
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

void read_as_r4_or_y1(Game& g, int order) {
  const IdentitySet two =
      IdentitySet::empty().add(Identity{0, 4}).add(Identity{1, 1});
  g.state.deck[order].clued = true;
  g.with_thought(order, [&two](const Thought& t) {
    Thought out = t;
    out.inferred = two;
    out.possible = two;
    return out;
  });
}

const std::vector<int>& row_of(const Game& g, TestPlayer p) {
  return g.state.pairwise_play_stacks[static_cast<int>(p)];
}

}  // namespace

// Both copies sit in the third seat's hand, so Bob accounts for them as we do and
// HIS row learns the yellow. Cathy's does not: she is holding them.
TEST(TiiahPairVisibleCopies, CopiesOutsideThePairSettleForThePair) {
  Game g = setup(opts_for());
  const int mine = order_at(g, TestPlayer::ALICE, 1);
  read_as_r4_or_y1(g, mine);

  ASSERT_TRUE(hanabi::tiiah::all_copies_visible_to_pair(
      g, mine, Identity{0, 4}, static_cast<int>(TestPlayer::BOB)))
      << "both r4s are in Cathy's hand, which neither of us holds";
  ASSERT_FALSE(hanabi::tiiah::all_copies_visible_to_pair(
      g, mine, Identity{0, 4}, static_cast<int>(TestPlayer::CATHY)))
      << "and Cathy cannot see her own hand, so she cannot follow it";

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.play_stacks[1], 1)
      << "our own belief names the card: rule 3's private form still fires";
  const std::vector<int> learned{3, 1, 0, 0, 0};
  const std::vector<int> unmoved{3, 0, 0, 0, 0};
  EXPECT_EQ(row_of(g, TestPlayer::BOB), learned)
      << "Bob rules the r4 out exactly as we do, so what we know he knows moves "
         "with us";
  EXPECT_EQ(row_of(g, TestPlayer::CATHY), unmoved)
      << "Cathy holds both copies and cannot see them, so her row may not take it";
  EXPECT_EQ(g.state.common_play_stacks[1], 0)
      << "and it is not common knowledge either -- one seat cannot follow it";
}

// The control. Move one copy into Bob's OWN hand: we can still see both, so our
// belief names the card, but Bob cannot account for the one he is holding and his
// row must stay where it was.
TEST(TiiahPairVisibleCopies, ACopyInThePartnersOwnHandDoesNotCount) {
  SetupOptions opts = opts_for();
  opts.hands[1] = {"r4", "b4", "p4", "g3", "b3"};
  opts.hands[2] = {"r4", "y4", "g5", "b5", "p4"};
  Game g = setup(std::move(opts));
  const int mine = order_at(g, TestPlayer::ALICE, 1);
  read_as_r4_or_y1(g, mine);

  EXPECT_FALSE(hanabi::tiiah::all_copies_visible_to_pair(
      g, mine, Identity{0, 4}, static_cast<int>(TestPlayer::BOB)))
      << "one of the two is in Bob's hand, and Bob cannot see his own hand";

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.play_stacks[1], 1)
      << "we see both copies, so the private form of rule 3 is unaffected";
  const std::vector<int> unmoved{3, 0, 0, 0, 0};
  EXPECT_EQ(row_of(g, TestPlayer::BOB), unmoved)
      << "but a row may only hold what the seat behind it can work out";
}
