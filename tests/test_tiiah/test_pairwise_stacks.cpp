// The PAIRWISE stack views (tiiah/CONVENTION.md §1.3, v16.12.0).
//
// A play goes into the hole, so it is known to every seat EXCEPT the one who
// made it. `common_play_stacks` — what ALL of them know — therefore advances
// only for a play its own player could name, and one seat's ignorance holds the
// whole team's reading back. A clue only has to mean one thing to the two seats
// it is between, so what it is read against is `pairwise_play_stacks[p]`: the
// plays we know seat p knows.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Three seats. Alice is us and cannot see her own hand; Bob holds the 1s.
SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y1", "g1", "b1", "p1"},
      {"r2", "y2", "g2", "b2", "p2"},
  };
  return opts;
}

int red_row(const Game& g, TestPlayer p) {
  return g.state.pairwise_play_stacks[static_cast<int>(p)][0];
}

}  // namespace

// A PARTNER's hidden play, which they could not name. We watched the card leave
// their hand, so we know it — and so does everybody else at the table except
// them. Their own row stays put; every other row advances.
TEST(TiiahPairwiseStacks, APartnersBlindPlayAdvancesEveryRowButTheirs) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.play_stacks[0], 1) << "we watched it: our belief advances";
  EXPECT_EQ(g.state.common_play_stacks[0], 0)
      << "Bob could not name it, so it is not common knowledge";
  EXPECT_EQ(red_row(g, TestPlayer::BOB), 0)
      << "Bob threw it in the hole and never learned what it was";
  EXPECT_EQ(red_row(g, TestPlayer::CATHY), 1)
      << "Cathy watched it, and so did we — the two of us share it";
}

// Our OWN hidden play is the one card at the table we cannot name. Every
// partner watched it, but we cannot write down what we do not know, so no row
// moves — the rows hold what WE know a seat knows.
TEST(TiiahPairwiseStacks, OurOwnBlindPlayAdvancesNothing) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.play_stacks[0], 0);
  EXPECT_EQ(g.state.common_play_stacks[0], 0);
  EXPECT_EQ(red_row(g, TestPlayer::BOB), 0);
  EXPECT_EQ(red_row(g, TestPlayer::CATHY), 0);
}

// A play its own player COULD name is common knowledge, so it reaches every
// row — including the player's own.
TEST(TiiahPairwiseStacks, AKnownPlayReachesEveryRow) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.common_play_stacks[0], 1) << "everyone can follow it";
  EXPECT_EQ(red_row(g, TestPlayer::BOB), 1);
  EXPECT_EQ(red_row(g, TestPlayer::CATHY), 1);
}

// The PREFIX rule, and the reason a row is not simply "our belief minus one
// seat". A row can only take a card its own stack is waiting for, so a play the
// row never saw blocks everything above it — which is exactly what the seat
// behind that row believes.
TEST(TiiahPairwiseStacks, ARowCannotSkipTheCardItNeverSaw) {
  SetupOptions opts = opts_for();
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y1", "g1", "b1", "p1"},
      {"r2", "y2", "g2", "b2", "p2"},
  };
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));

  // Bob blind-plays the r1: his own row stays on 0, Cathy's goes to 1.
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);
  // Cathy blind-plays the r2: Bob's row is still on 0 and cannot take an r2,
  // so it stays there, while our belief has both.
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.play_stacks[0], 2) << "we watched both";
  EXPECT_EQ(red_row(g, TestPlayer::BOB), 0)
      << "Bob never saw the r1 he played, so the r2 he watched cannot land "
         "either -- which is precisely what Bob himself believes";
  EXPECT_EQ(red_row(g, TestPlayer::CATHY), 1)
      << "Cathy saw the r1 and played the r2 blind, so she is on 1";
}

// `stacks_known_to_both` is symmetric, and answers for OUR seat by handing back
// our own belief -- against ourselves there is nothing we do not know.
TEST(TiiahPairwiseStacks, KnownToBothIsSymmetricAndOursIsOurBelief) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);

  const int me = static_cast<int>(TestPlayer::ALICE);
  const int bob = static_cast<int>(TestPlayer::BOB);
  EXPECT_EQ(g.state.stacks_known_to_both(me, bob),
            g.state.stacks_known_to_both(bob, me));
  EXPECT_EQ(g.state.stacks_known_to_both(me, bob),
            g.state.pairwise_play_stacks[bob]);
  EXPECT_EQ(g.state.pairwise_view(me).play_stacks, g.state.play_stacks)
      << "our own row is never consulted: our belief IS what we know we know";
}

// Outside the variant none of this exists, and every accessor falls through to
// the ordinary single view.
TEST(TiiahPairwiseStacks, AbsentOutsideTheVariant) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.hands = {
      {"r1", "y1", "g1", "b1", "p1"},
      {"r2", "y2", "g2", "b2", "p2"},
      {"r3", "y3", "g3", "b3", "p3"},
  };
  Game g = setup(std::move(opts));

  EXPECT_TRUE(g.state.pairwise_play_stacks.empty());
  EXPECT_EQ(g.state.stacks_known_to_both(0, 1), g.state.play_stacks);
  EXPECT_EQ(g.state.pairwise_view(1).play_stacks, g.state.play_stacks);
}
