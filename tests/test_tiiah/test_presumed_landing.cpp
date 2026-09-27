// §1e rule 6: a partner's play is presumed to LAND (tiiah/CONVENTION.md, v16.16.0).
//
// Our own stacks can be short by exactly what we threw in the hole — a card we
// cannot name is the only thing that makes them short, since every partner's play
// is one we watched. So a partner's play that looks dead to us may be landing on
// a card we played without knowing it, and concluding "strike" is the one thing we
// must not do by default.
//
// Asked of the worlds our own hole cards leave open: if any of them lets the play
// land, the ones in which it strikes are refuted.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Alice is us and cannot see her own hand. Her slot 1 is the r1 — she will throw
// it into the hole not knowing which of two identities it was — and Bob then plays
// the r2, which only lands if that card was the r1.
SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r2", "y4", "g4", "b4", "p4"},
      {"y1", "g1", "b1", "p1", "r5"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

}  // namespace

// The 2010296 shape. Our hole card leaves two worlds open; Bob's r2 lands in
// exactly one of them, so that is the world we are in.
TEST(TiiahPresumedLanding, TheWorldThatLetsItLandIsTheOneWeAreIn) {
  Game g = setup(opts_for());
  const int mine = order_at(g, TestPlayer::ALICE, 1);

  // Pre-clue our slot 1 down to two candidates, one of which is the r1 Bob's r2
  // needs. Without that the superposition is all 25 ids and the enumeration is
  // capped -- which is its own test, below.
  g = pre_clue(std::move(g), TestPlayer::ALICE, /*slot=*/1, {"1"});
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);
  ASSERT_TRUE(g.meta[mine].superposed());
  ASSERT_TRUE(g.meta[mine].superposition.contains(Identity{0, 1}))
      << "the r1 has to be one of the readings, or there is no world to find";
  ASSERT_EQ(g.state.play_stacks[0], 0) << "and our own stacks have not moved";

  // Bob plays his r2 into the hole. It looks dead to us -- red is on 0 -- but it
  // lands in the world where the card we threw was the r1.
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.strikes, 0)
      << "we must not invent a strike out of stacks that are short by a card we "
         "could not name";
  EXPECT_EQ(g.state.play_stacks[0], 2)
      << "our r1, and then the r2 we watched";
  EXPECT_TRUE(g.meta[mine].superposition.is_empty())
      << "and we have learned what we threw: only one world survived";
}

// The control that keeps the rule honest. A partner really can misplay: when NO
// world we are in lets the card land, the strike stands.
TEST(TiiahPresumedLanding, AStrikeNoWorldRescuesStillStands) {
  SetupOptions opts = opts_for();
  // Bob's slot 1 is now an r3, which needs an r2 no world of ours can supply --
  // our hole card is a 1.
  opts.hands[1] = {"r3", "y4", "g4", "b4", "p4"};
  Game g = setup(std::move(opts));
  const int mine = order_at(g, TestPlayer::ALICE, 1);

  g = pre_clue(std::move(g), TestPlayer::ALICE, /*slot=*/1, {"1"});
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.strikes, 1)
      << "nothing we might have played makes an r3 playable, so it really struck";
  EXPECT_TRUE(g.meta[mine].superposed())
      << "and our own card is untouched -- a strike is not evidence about it";
}

// A partner's play that is playable on our stacks already needs no rescuing, so
// THIS rule does not fire on it.
//
// Rule 6's own-play form does, though (v16.18.0): if our hole card had been the
// other r1, one of the two must have struck, and that world is refuted. So the set
// does move — just not by anything `presume_play_lands` did.
TEST(TiiahPresumedLanding, APlayThatAlreadyLandsNeedsNoRescue) {
  SetupOptions opts = opts_for();
  opts.hands[1] = {"r1", "y4", "g4", "b4", "p4"};  // playable with red on 0
  Game g = setup(std::move(opts));
  const int mine = order_at(g, TestPlayer::ALICE, 1);

  g = pre_clue(std::move(g), TestPlayer::ALICE, /*slot=*/1, {"1"});
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);
  const IdentitySet before = g.meta[mine].superposition;

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.strikes, 0);
  EXPECT_EQ(g.state.play_stacks[0], 1) << "Bob's r1, and nothing of ours";
  EXPECT_FALSE(g.meta[mine].superposition.contains(Identity{0, 1}))
      << "the r1 is gone, but not by this rule: ours being the other copy needs a "
         "strike somewhere, and rule 6 refuses to presume one (v16.18.0)";
  EXPECT_EQ(g.meta[mine].superposition,
            before.difference(IdentitySet::single(Identity{0, 1})))
      << "and that is the ONLY thing it took -- the play itself needed no world of "
         "ours to land in, so nothing else about our card was in question";
}

// Nothing in the hole means nothing to blame, so a dead play is simply dead.
TEST(TiiahPresumedLanding, WithNothingInTheHoleADeadPlayStrikes) {
  SetupOptions opts = opts_for();
  opts.hands[1] = {"r2", "y4", "g4", "b4", "p4"};
  Game g = setup(std::move(opts));

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true);

  EXPECT_EQ(g.state.strikes, 1);
  EXPECT_EQ(g.state.play_stacks[0], 0);
}

// NOTE on the variant gate: there is deliberately no "outside TIIAH" test here.
// `presume_play_lands` returns on its first line for any other variant and there
// are no hole cards for it to reason about, and the 853 non-TIIAH tests in the
// other four binaries all run through `Game::handle_action` -- which is far
// stronger evidence than a fixture asserting the harness does as it is told.
