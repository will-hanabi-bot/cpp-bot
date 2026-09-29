// A KNOWN PLAY is read from what every seat computes alike: the card's clue-touch
// empathy on the shared view (tiiah/CONVENTION.md §1c, v18.0.0).
//
// The dispatch keys on whether Bob and Cathy hold a known play, and every seat has
// to answer that alike. v17.4.0 counted any standing call, but call statuses,
// inferences and one seat's own stacks can all differ between seats. Over 500
// self-play games about one clue in ten was read as different kinds, and four in ten
// of those disagreements involved a seat in the reverse position. So a known play
// is now a card whose clue touches (`possible`) allow only identities playable on
// the shared view, and a standing call does not count on its own.
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

TEST(TiiahKnownPlay, AKnownPlayIsReadFromTheTouchesOnTheSharedView) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b3", "r4", "y4", "g4", "p4"},
      {"r1", "y5", "g5", "b5", "p5"},
  };
  use_tiiah(opts);
  Game g = setup(std::move(opts));

  // Bob's slot 1 is called to play as the b3, but its touches still allow the b3
  // and the b4, neither playable with blue on 0: a standing call alone is not a
  // known play any more.
  const int called = order_at(g, TestPlayer::BOB, 1);
  g.meta[called].status = CardStatus::CALLED_TO_PLAY;
  g.common.thoughts[called].inferred = IdentitySet::single(Identity{3, 3});
  g.common.thoughts[called].possible =
      IdentitySet::single(Identity{3, 3}).add(Identity{3, 4});
  EXPECT_FALSE(reactor::variants::has_known_play(g, static_cast<int>(TestPlayer::BOB)))
      << "the call is a promise, but not one every seat reads alike";

  // Cathy's slot 1 has been touched down to the 1s, and every 1 is playable on the
  // shared view: that is a known play, called or not.
  const int ones = order_at(g, TestPlayer::CATHY, 1);
  IdentitySet all_ones = IdentitySet::empty();
  for (int suit = 0; suit < 5; ++suit) all_ones = all_ones.add(Identity{suit, 1});
  g.common.thoughts[ones].possible = all_ones;
  g.common.thoughts[ones].inferred = all_ones;
  EXPECT_TRUE(reactor::variants::has_known_play(g, static_cast<int>(TestPlayer::CATHY)))
      << "every identity its touches allow is playable on the shared view";
}
