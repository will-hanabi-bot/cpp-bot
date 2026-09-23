// Superposition (tiiah/CONVENTION.md §1e), and the two stack views it feeds.
//
// The seat that plays a card into the hole learns nothing about it, so what it
// keeps is a set of candidates — and everyone keeps that set on its behalf, from
// `common`, which is the only view all three compute alike.
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

// Three seats, nothing played, and Alice (us) holding cards we cannot see.
SetupOptions opts_for(const std::string& variant_name = "Throw It in a Hole (5 Suits)") {
  SetupOptions opts;
  opts.variant_name = variant_name;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y1", "g1", "b1", "p1"},
      {"r2", "y2", "g2", "b2", "p2"},
  };
  return opts;
}

IdentitySet superposition_at(const Game& g, TestPlayer p, int slot_order) {
  (void)p;
  return g.meta[slot_order].superposition;
}

}  // namespace

// Our own play with an ambiguous empathy: nothing advances, and the candidates
// are recorded. This is the gap v16.0.0 left open.
TEST(TiiahSuperposition, OurOwnAmbiguousPlayIsRecorded) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));
  const int order = order_at(g, TestPlayer::ALICE, 1);

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_TRUE(g.meta[order].superposed())
      << "we played a card we could not name, which is the definition";
  EXPECT_GT(g.meta[order].superposition.length(), 1);
  for (int stack : g.state.play_stacks) EXPECT_EQ(stack, 0);
  for (int stack : g.state.common_play_stacks) EXPECT_EQ(stack, 0);
}

// A card whose empathy pins one identity is not a superposition at all: the
// player knew, so every seat can follow it and BOTH views advance.
TEST(TiiahSuperposition, AKnownPlayAdvancesBothViews) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "r1");
  const int order = order_at(g, TestPlayer::ALICE, 1);

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_FALSE(g.meta[order].superposed());
  EXPECT_EQ(g.state.play_stacks[0], 1) << "we know what we played";
  EXPECT_EQ(g.state.common_play_stacks[0], 1) << "and so does everyone else";
}

// The two views are supposed to disagree. A partner's ambiguous play advances
// OUR stacks — we watched the card — and not the shared ones, because the
// partner who played it still does not know what it was.
TEST(TiiahSuperposition, APartnersAmbiguousPlayMovesOnlyOurView) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));
  const int order = order_at(g, TestPlayer::BOB, 1);  // Bob's r1

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "r3");

  EXPECT_EQ(g.state.play_stacks[0], 1) << "we saw the r1 go in";
  EXPECT_EQ(g.state.common_play_stacks[0], 0)
      << "but Bob does not know what he played, so the shared view waits";
  EXPECT_TRUE(g.meta[order].superposed())
      << "and we track the superposition on his behalf";
}

// Shared collapsing rule 2: a clue that calls a playable card of that identity
// to play says the identity was still needed, so the superposed card was not it.
TEST(TiiahSuperposition, ACalledPlayableCollapsesTheSet) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob plays his y1 blind. It is not red, so red stays on 0 in every view
      // — which is what lets the clue below name the r1 unambiguously.
      {"y1", "y4", "g4", "b4", "p4"},
      {"r1", "y3", "g3", "b3", "p3"},
  };
  opts.clue_tokens = 7;  // so Cathy may discard
  opts.starting = TestPlayer::BOB;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  const int order = order_at(g, TestPlayer::BOB, 1);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y2");
  const IdentitySet before = g.meta[order].superposition;
  ASSERT_TRUE(before.contains(Identity{0, 1})) << "guard: r1 is a candidate";
  ASSERT_GT(before.length(), 1);

  g = take_turn(std::move(g), "Cathy discards p3", "p2");
  // Red to Cathy calls her r1: an identity the team still needs, and one Bob
  // can see, so what Bob threw in was not the r1.
  g = take_turn(std::move(g), "Alice clues red to Cathy");

  EXPECT_FALSE(g.meta[order].superposition.contains(Identity{0, 1}))
      << "an identity the team still needs is not one that was already played";
  EXPECT_TRUE(g.meta[order].superposed())
      << "...and the rest of the set is untouched";
}

// The private rule narrows OUR belief and nothing else: the stored set is what
// partners predict from, and they cannot see what we can.
TEST(TiiahSuperposition, PrivateSightMovesOurStacksOnly) {
  SetupOptions opts = opts_for();
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));
  const int order = order_at(g, TestPlayer::ALICE, 1);

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);
  ASSERT_TRUE(g.meta[order].superposed()) << "guard";

  // Whatever narrowing our own eyes do, the shared set and the shared stacks
  // must be exactly where the table left them.
  EXPECT_EQ(g.state.common_play_stacks[0], 0);
  EXPECT_EQ(g.state.common_play_stacks[4], 0);
}

// Outside the variant none of this exists, and the shared accessors are the
// ordinary ones — which is what keeps the two belief reads free for every other
// convention.
TEST(TiiahSuperposition, TheSharedViewIsAbsentOutsideTheVariant) {
  SetupOptions opts = opts_for("No Variant");
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));

  EXPECT_TRUE(g.state.common_play_stacks.empty());
  EXPECT_EQ(g.state.shared_score(), g.state.score());
  EXPECT_EQ(g.state.shared_pace(), g.state.pace());
  EXPECT_EQ(g.shared_in_endgame(), g.in_endgame());

  g = take_turn(std::move(g), "Bob plays r1", "r3");
  EXPECT_EQ(g.state.shared_score(), g.state.score());
  EXPECT_TRUE(g.meta[order_at(g, TestPlayer::BOB, 1)].superposition.is_empty());
}
