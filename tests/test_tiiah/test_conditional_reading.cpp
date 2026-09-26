// Conditional readings (tiiah/CONVENTION.md §1e, v16.13.0).
//
// A seat that threw a card into the hole without knowing what it was does not
// know its own stacks. So a call on a LATER card means different identities in
// each world the earlier card leaves open, and the honest reading is their
// union — with a record of which world each candidate needed, so that settling
// the earlier card withdraws the ones that leaned on it.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/tiiah/superposition.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Alice is us and cannot see her own hand; Bob holds the 1s.
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

// The base a reading actually uses is the one that does NOT already count the
// holder's own hole plays -- their own belief, or the pairwise view of it. OUR
// belief counts a partner's blind play (we watched the card go in), so handing
// that in as the base would apply it twice.
State base_for(const Game& g, TestPlayer p) {
  return g.state.pairwise_view(static_cast<int>(p));
}

int worlds_for(const Game& g, TestPlayer p) {
  return static_cast<int>(
      hanabi::tiiah::open_worlds(g, base_for(g, p), static_cast<int>(p)).size());
}

}  // namespace

// Nothing in the hole: exactly one world, which is the state itself. This is
// what keeps every reading unchanged until somebody plays a card blind.
TEST(TiiahConditionalReading, NoHoleCardsMeansOneWorld) {
  SetupOptions opts = opts_for();
  Game g = setup(std::move(opts));

  const auto worlds = hanabi::tiiah::open_worlds(g, g.state, 0);
  ASSERT_EQ(worlds.size(), 1u);
  EXPECT_TRUE(worlds[0].assignment.empty());
  EXPECT_EQ(worlds[0].state.play_stacks, g.state.play_stacks);
}

// One card in the hole with two candidates: two worlds, each with that card
// assigned and its stacks advanced accordingly. Replay 2009367 T4's shape.
TEST(TiiahConditionalReading, OneHoleCardOpensAWorldPerCandidate) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));
  const int order = order_at(g, TestPlayer::BOB, 1);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);
  ASSERT_TRUE(g.meta[order].superposed());
  const int candidates = g.meta[order].superposition.length();
  ASSERT_GT(candidates, 1);

  const State base = base_for(g, TestPlayer::BOB);
  const auto worlds =
      hanabi::tiiah::open_worlds(g, base, static_cast<int>(TestPlayer::BOB));
  EXPECT_EQ(static_cast<int>(worlds.size()), candidates)
      << "one world per identity the card could have been";
  int advanced = 0;
  for (const auto& w : worlds) {
    ASSERT_EQ(w.assignment.size(), 1u);
    EXPECT_EQ(w.assignment[0].first, order);
    if (w.state.play_stacks != base.play_stacks) ++advanced;
  }
  EXPECT_GT(advanced, 0)
      << "a world in which the card was playable stands it up on the stacks";
}

// Our OWN hole card opens worlds for us, and a partner's opens none for us --
// the worlds belong to the seat that cannot name the card.
TEST(TiiahConditionalReading, TheWorldsBelongToTheSeatThatCannotName) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_GT(worlds_for(g, TestPlayer::BOB), 1)
      << "Bob threw it and does not know what it was";
  EXPECT_EQ(worlds_for(g, TestPlayer::CATHY), 1)
      << "Cathy watched it: nothing about it is open to her";
  EXPECT_EQ(worlds_for(g, TestPlayer::ALICE), 1);
}

// Two cards in the hole multiply, and the assignments are applied in PLAY ORDER
// so that a chain lands: if the first was the r1 and the second the r2, the
// world has red on 2.
TEST(TiiahConditionalReading, TwoHoleCardsMultiplyAndChain) {
  SetupOptions opts = opts_for();
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "r2", "g1", "b1", "p1"},
      {"y1", "y2", "g2", "b2", "p2"},
  };
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));
  // Bob's slot 2 (the r2) goes first, then his slot 1 (the r1) -- after the
  // first play the hand shifts, so slot 1 is the r1 both times is NOT true;
  // take the orders up front.
  const int older = order_at(g, TestPlayer::BOB, 2);
  const int newer = order_at(g, TestPlayer::BOB, 1);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/2,
                    /*reached_the_hole=*/true);
  // Slot 1 is still the r1: a play shifts the slots right of it, not left.
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);

  ASSERT_TRUE(g.meta[older].superposed());
  ASSERT_TRUE(g.meta[newer].superposed());
  // A cap generous enough to admit the product: with no clue information on
  // either card each superposition is all 25 identities, so the default cap of
  // 64 would (correctly) read this flat. The chain is what is under test here.
  const auto worlds =
      hanabi::tiiah::open_worlds(g, base_for(g, TestPlayer::BOB),
                                 static_cast<int>(TestPlayer::BOB), /*cap=*/2000);
  EXPECT_EQ(worlds.size(), static_cast<size_t>(g.meta[older].superposition.length() *
                                               g.meta[newer].superposition.length()));
  for (const auto& w : worlds) {
    ASSERT_EQ(w.assignment.size(), 2u);
    EXPECT_LT(w.assignment[0].first, w.assignment[1].first)
        << "oldest first, so a chain is replayed in the order it happened";
  }
}

// The cap. A partial enumeration would read as a conditional set missing worlds,
// which is worse than reading it flat, so the cap falls back to the one world we
// are standing in.
TEST(TiiahConditionalReading, TheCapReadsItFlat) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);

  const State base = base_for(g, TestPlayer::BOB);
  const auto capped = hanabi::tiiah::open_worlds(
      g, base, static_cast<int>(TestPlayer::BOB), /*cap=*/1);
  ASSERT_EQ(capped.size(), 1u);
  EXPECT_TRUE(capped[0].assignment.empty())
      << "flat, not a truncated list of worlds";
  EXPECT_EQ(capped[0].state.play_stacks, base.play_stacks);
}

// Outside the variant there are no hole cards and so never more than one world.
TEST(TiiahConditionalReading, OneWorldOutsideTheVariant) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.hands = {
      {"r1", "y1", "g1", "b1", "p1"},
      {"r2", "y2", "g2", "b2", "p2"},
      {"r3", "y3", "g3", "b3", "p3"},
  };
  Game g = setup(std::move(opts));
  EXPECT_EQ(worlds_for(g, TestPlayer::BOB), 1);
}
