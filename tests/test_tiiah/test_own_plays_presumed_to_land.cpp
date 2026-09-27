// §1e rule 6 turned on OUR OWN plays (tiiah/CONVENTION.md, v16.18.0).
//
// v16.16.0 said never presume a PARTNER's play struck. The same argument applies
// to our own: a card we threw in the hole either landed or struck and nobody told
// us which, so the default has to be the same default — it landed. Every world in
// which one of our own hole cards failed is refuted, as long as some world has
// none failing at all.
//
// It reaches cases v16.16.0's form cannot, because that one only ever asked how to
// rescue a play that looked dead. Here the partner's play is perfectly healthy and
// it is OUR card the argument lands on: if ours had been the same identity, one of
// the two must have struck.
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

// Alice is us, so her hand carries no identities at all — that is the whole point
// of the variant, and a fixture that named them would let the engine resolve our
// own play and never build a superposition in the first place.
SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us)
      {"g1", "y4", "g4", "b4", "p4"},  // Bob holds the g1 he will play
      {"r4", "y4", "g5", "b5", "p5"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

// Read the card as {g1,b1} — what a rank 1 plus two colour negatives would leave.
// Both halves of the empathy, so `Game::elim` has nothing to widen back.
void read_as_g1_or_b1(Game& g, int order) {
  const IdentitySet two =
      IdentitySet::empty().add(Identity{2, 1}).add(Identity{3, 1});
  g.state.deck[order].clued = true;
  g.with_thought(order, [&two](const Thought& t) {
    Thought out = t;
    out.inferred = two;
    out.possible = two;
    return out;
  });
}

}  // namespace

// Our hole card reads {g1,b1}; Bob then plays the g1. His play lands either way,
// so v16.16.0's form has nothing to rescue and never looks — but if ours had been
// the g1 too, one of the two struck, and under rule 6 that world is refuted.
TEST(TiiahOwnPlaysLand, OurCardCannotDupeAPartnersPlay) {
  Game g = setup(opts_for());
  const int mine = order_at(g, TestPlayer::ALICE, 1);
  read_as_g1_or_b1(g, mine);

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);
  ASSERT_TRUE(g.meta[mine].superposed())
      << "guard: our card went in unnamed, over two identities";
  ASSERT_EQ(g.state.play_stacks[2], 0) << "and nothing has been played yet";

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y3");

  EXPECT_EQ(g.state.strikes, 0) << "and we do not invent one to explain it";
  EXPECT_TRUE(g.meta[mine].superposition.is_empty())
      << "the g1 world is refuted, which leaves one -- so we know what we threw";
  EXPECT_EQ(g.state.play_stacks[2], 1) << "Bob's g1, which we watched";
  EXPECT_EQ(g.state.play_stacks[3], 1) << "and our own b1, which we have just named";
}

// The control. With nothing to dupe, both worlds land and neither is refuted.
TEST(TiiahOwnPlaysLand, NothingIsRefutedWhileEveryWorldLands) {
  SetupOptions opts = opts_for();
  opts.hands[1] = {"y4", "g4", "b4", "p4", "r4"};  // Bob has no rank 1 to play
  Game g = setup(std::move(opts));
  const int mine = order_at(g, TestPlayer::ALICE, 1);
  read_as_g1_or_b1(g, mine);

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_TRUE(g.meta[mine].superposed())
      << "a card that lands whatever it was tells us nothing about which it was";
  EXPECT_EQ(g.state.play_stacks[2], 0);
  EXPECT_EQ(g.state.play_stacks[3], 0);
}

// `strike_free` is the rule in one function, and its escape hatch is what keeps it
// honest: when EVERY world has a strike in it, the strike is not an assumption
// anybody made and there is nothing to refute.
TEST(TiiahOwnPlaysLand, EveryWorldStrikingLeavesThemAllStanding) {
  Game g = setup(opts_for());

  std::vector<hanabi::tiiah::OpenWorld> worlds(
      3, hanabi::tiiah::OpenWorld{{}, g.state});
  for (auto& w : worlds) w.struck = true;
  EXPECT_EQ(hanabi::tiiah::strike_free(worlds).size(), 3u)
      << "all of them, so a caller comparing sizes refutes nothing";

  worlds[1].struck = false;
  const auto live = hanabi::tiiah::strike_free(worlds);
  ASSERT_EQ(live.size(), 1u) << "one world without a strike refutes the other two";
  EXPECT_EQ(live.front(), &worlds[1]);
}
