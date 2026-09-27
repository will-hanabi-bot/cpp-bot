// Which worlds of hole cards are open (tiiah/CONVENTION.md §1e, v16.24.0):
//
//   * a world must be one the TARGETING RULES allow -- a recorded reaction whose
//     receiver held two or more of the world's cards rules out any world in which
//     one of them would have out-ranked the card the reacter called;
//   * a world is replayed in PLAY order, not card order;
//   * a view's BAND -- cards it counts without being able to name them -- absorbs a
//     world's card rather than striking it, so a floor replayed on itself stays put.

#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/action.h"
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

SetupOptions opts_for(std::vector<std::vector<std::string>> hands,
                      std::vector<int> stacks, TestPlayer starting) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = std::move(hands);
  opts.play_stacks = std::move(stacks);
  opts.starting = starting;
  use_tiiah(opts);
  return opts;
}

void read_as(Game& g, int order, const IdentitySet& set) {
  g.state.deck[order].clued = true;
  g.with_thought(order, [&set](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    out.possible = set;
    return out;
  });
}

IdentitySet two_of(Identity a, Identity b) { return IdentitySet::empty().add(a).add(b); }

constexpr Identity kB2{3, 2}, kB3{3, 3}, kB4{3, 4}, kP2{4, 2}, kP3{4, 3};

}  // namespace

// --- feasibility ------------------------------------------------------------

// Replay 2011397 T6, reduced: the reacter's answer called the receiver's slot 2.
// With a direct playable on slot 3 in some world, slot 2 could only have been
// called if it was a direct playable too.
TEST(TiiahWorldFeasibility, AFinesseIsNeverCalledAheadOfADirectPlayable) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"g4", "b2", "b3", "g3", "g4"},
                           {"r4", "y4", "g5", "b5", "p5"}},
                          {0, 0, 1, 1, 0}, TestPlayer::ALICE));
  const std::vector<int> hand = g.state.hands[1];
  const int slot2 = hand[1];
  const int slot3 = hand[2];
  g.reaction_records.push_back(
      ReactionRecord{6, /*receiver=*/1, hand, {}, {0, 0, 1, 1, 0}, slot2});

  OpenWorld real;
  real.assignment = {{slot2, kB2}, {slot3, kB3}};
  EXPECT_TRUE(hanabi::tiiah::world_feasible(g, real)) << "b2 direct on slot 2, b3 a finesse after it";

  OpenWorld impossible;
  impossible.assignment = {{slot2, kP2}, {slot3, kB2}};
  EXPECT_FALSE(hanabi::tiiah::world_feasible(g, impossible))
      << "p2 is a finesse and b2 a direct playable to its right: slot 3 would be called";

  OpenWorld dead;
  dead.assignment = {{slot2, kB4}, {slot3, kB3}};
  EXPECT_FALSE(hanabi::tiiah::world_feasible(g, dead))
      << "the called card must be direct or one away in the world";
}

// One of the world's cards in that hand is not enough: its reading already came
// from that very reaction.
TEST(TiiahWorldFeasibility, OneCardInTheHandConstrainsNothing) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"g4", "b2", "b3", "g3", "g4"},
                           {"r4", "y4", "g5", "b5", "p5"}},
                          {0, 0, 1, 1, 0}, TestPlayer::ALICE));
  const std::vector<int> hand = g.state.hands[1];
  g.reaction_records.push_back(ReactionRecord{6, 1, hand, {}, {0, 0, 1, 1, 0}, hand[1]});
  OpenWorld w;
  w.assignment = {{hand[1], kB4}};
  EXPECT_TRUE(hanabi::tiiah::world_feasible(g, w));
}

// When the evidence rules out every world it is contradicting itself, and
// `open_worlds` keeps them all rather than assert that.
TEST(TiiahWorldFeasibility, EveryWorldInfeasibleStandsTheCheckDown) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"g4", "p2", "b2", "g3", "g4"},
                           {"r4", "y4", "g5", "b5", "p5"}},
                          {0, 0, 1, 1, 0}, TestPlayer::ALICE));
  const std::vector<int> hand = g.state.hands[1];
  g.reaction_records.push_back(ReactionRecord{6, 1, hand, {}, {0, 0, 1, 1, 0}, hand[1]});
  g.meta[hand[1]].superposition = IdentitySet::single(kP2);
  g.meta[hand[2]].superposition = IdentitySet::single(kB2);
  const auto worlds = hanabi::tiiah::open_worlds(g, g.state, 1);
  ASSERT_EQ(worlds.size(), 1u);
  EXPECT_EQ(worlds.front().assignment.size(), 2u) << "the one world, kept";
}

// --- play order -------------------------------------------------------------

// The card with the HIGHER order goes into the hole first. Replayed in card order
// the b2 lands on nothing and every world strikes; in play order the (b1, b2)
// world is the only strike-free one, and Bob's row and the shared view reach blue 2.
TEST(TiiahWorldFeasibility, WorldsAreReplayedInPlayOrder) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"b1", "b2", "y4", "g4", "p4"},
                           {"r4", "y4", "g5", "b5", "p5"}},
                          {0, 0, 0, 0, 0}, TestPlayer::BOB));
  const int b1 = order_at(g, TestPlayer::BOB, 1);
  const int b2 = order_at(g, TestPlayer::BOB, 2);
  ASSERT_GT(b1, b2) << "guard: the card played first has the higher order";
  read_as(g, b1, two_of(Identity{3, 1}, Identity{0, 3}));
  read_as(g, b2, two_of(kB2, Identity{0, 4}));

  g = hidden_action(std::move(g), TestPlayer::BOB, 1, /*reached_the_hole=*/true, "y5");
  g = hidden_action(std::move(g), TestPlayer::CATHY, 5, /*reached_the_hole=*/false, "g1");
  g = hidden_action(std::move(g), TestPlayer::ALICE, 5, /*reached_the_hole=*/false);
  ASSERT_EQ(order_at(g, TestPlayer::BOB, 2), b2) << "guard: the b2 moved to slot 2";
  g = hidden_action(std::move(g), TestPlayer::BOB, 2, /*reached_the_hole=*/true, "y5");

  EXPECT_EQ(g.state.common_play_stacks[3], 2) << "the only strike-free world is (b1, b2)";
  EXPECT_EQ(g.state.pairwise_play_stacks[1][3], 2) << "Bob's row, likewise";
}

// --- the band ---------------------------------------------------------------

// Replay 2011327 T36, reduced. Cathy's first hole card is {b3,p3} (the p3), her
// second {b3,b4} (the b3). The worlds (b3,b4) and (p3,b3) leave blue on 3 in both,
// purple on 2 in one -- so her row floors at blue 3, purple 2. Replaying the same
// worlds on that row once it is floored must not strike the (p3,b3) world and lift
// blue to 4: the b3 is one of the cards the floor already counts.
TEST(TiiahWorldFeasibility, AFloorReplayedOnItselfStaysPut) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"r4", "y4", "g4", "g5", "p5"},
                           {"p3", "b3", "y3", "g3", "r3"}},
                          {0, 0, 0, 2, 2}, TestPlayer::CATHY));
  const int first = order_at(g, TestPlayer::CATHY, 1);
  const int second = order_at(g, TestPlayer::CATHY, 2);
  read_as(g, first, two_of(kB3, kP3));
  read_as(g, second, two_of(kB3, kB4));

  g = hidden_action(std::move(g), TestPlayer::CATHY, 1, /*reached_the_hole=*/true, "y5");
  g = hidden_action(std::move(g), TestPlayer::ALICE, 5, /*reached_the_hole=*/false);
  g = hidden_action(std::move(g), TestPlayer::BOB, 5, /*reached_the_hole=*/false, "y5");
  ASSERT_EQ(order_at(g, TestPlayer::CATHY, 2), second);
  g = hidden_action(std::move(g), TestPlayer::CATHY, 2, /*reached_the_hole=*/true, "g1");
  const std::vector<int> floor{0, 0, 0, 3, 2};
  EXPECT_EQ(g.state.pairwise_play_stacks[2], floor);

  // Two more rounds of collapsing over the same worlds.
  g = hidden_action(std::move(g), TestPlayer::ALICE, 5, /*reached_the_hole=*/false);
  g = hidden_action(std::move(g), TestPlayer::BOB, 5, /*reached_the_hole=*/false, "y5");
  EXPECT_EQ(g.state.pairwise_play_stacks[2], floor) << "blue went to 4 here before";
  EXPECT_EQ(g.state.common_play_stacks, floor) << "and the shared view is the same floor";
}
