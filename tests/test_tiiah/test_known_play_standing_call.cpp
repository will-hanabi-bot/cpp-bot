// A STANDING call is a known play whatever our own stacks say (tiiah/CONVENTION.md
// §1c, v17.4.0).
//
// The dispatch keys on whether Bob and Cathy hold a known play, and every seat has
// to answer that alike. A called card qualifies while one good playable survives in
// its reading -- read on what the team shares, which for a standing call is always
// true: rule 3 of the call invariants withdraws a call no world of the team's
// leaves live. Until v17.4.0 the test was read on the seat's OWN stacks, so a seat
// one card ahead of the others (self-play 9000009 T28, blue 2 against blue 1)
// counted Bob's called b3 as a known play and the other two did not, and they read
// the same Rank 5 as a lock and as a reverse reactive.
#include <gtest/gtest.h>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/variants/hole.h"
#include "test_harness.h"
#include "test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

TEST(TiiahKnownPlay, AStandingCallIsAKnownPlayOnAnyStacks) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b3", "r4", "y4", "g4", "p4"},
      {"r5", "y5", "g5", "b5", "p5"},
  };
  use_tiiah(opts);
  Game g = setup(std::move(opts));

  // Bob's slot 1 is called to play as the b3, with blue still on 0 everywhere --
  // a call that is not playable on our stacks yet (a delayed one, or one resting
  // on a b1 and b2 we cannot name).
  const int o = order_at(g, TestPlayer::BOB, 1);
  g.meta[o].status = CardStatus::CALLED_TO_PLAY;
  g.common.thoughts[o].inferred = IdentitySet::single(Identity{3, 3});
  EXPECT_FALSE(g.state.is_playable(Identity{3, 3}));

  EXPECT_TRUE(reactor::variants::has_known_play(g, static_cast<int>(TestPlayer::BOB)))
      << "the call is the team's promise; our stacks do not get a vote";
  EXPECT_FALSE(reactor::variants::has_known_play(g, static_cast<int>(TestPlayer::CATHY)))
      << "Cathy holds nothing called and nothing entirely playable";
}
