// A call on our own card that plays in every world of our own hole cards is a known
// play (tiiah/CONVENTION.md §2k, v20.9.0, the user's ruling; replay 2018766 T23).
//
// We threw a 1 into the hole without naming it. Our called card reads `{r2,y2}`:
// no single identity plays on our belief (red and yellow on 0). Where our hole card
// was the r1 the r2 plays, and where it was the y1 the y2 does, so whichever world we
// are in, the card plays. If the hole card could also have been a g1, there is a
// world in which neither plays, and it is no known play.
#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Alice (us) throws her slot 5 into the hole; the team reads it as `hole`. Then her
// slot 2 is called to play as `{r2,y2}`.
Game position(IdentitySet hole) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g4", "b4", "y4", "p3"},
      {"b2", "g3", "y4", "p4", "r4"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  const int thrown = order_at(g, TestPlayer::ALICE, 5);
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/5, /*reached_the_hole=*/true);
  g.with_meta(thrown, [&hole](ConvData& m) {
    m.superposition = hole;
    m.shared_left = IdentitySet::empty();
  });
  const int called = order_at(g, TestPlayer::ALICE, 2);
  const IdentitySet reading =
      IdentitySet::single(g.state.expand_short("r2")).add(g.state.expand_short("y2"));
  for (Player* p : {&g.common, &g.players[0]}) {
    p->thoughts[called].inferred = reading;
    p->thoughts[called].possible = reading;
  }
  g.with_meta(called, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
  return g;
}

bool known_play(const Game& g, int order) {
  const auto plays = g.me().thinks_playables(g, static_cast<int>(TestPlayer::ALICE));
  return std::find(plays.begin(), plays.end(), order) != plays.end();
}

}  // namespace

TEST(TiiahCallInEveryWorld, ACallThatPlaysInEveryOwnWorldIsAKnownPlay) {
  Game g = position(IdentitySet::single(Identity{0, 1}).add(Identity{1, 1}));
  EXPECT_TRUE(known_play(g, order_at(g, TestPlayer::ALICE, 2)))
      << "the r2 where the hole card was the r1, the y2 where it was the y1";
}

TEST(TiiahCallInEveryWorld, AWorldWhereNeitherPlaysLeavesItUnknown) {
  Game g = position(IdentitySet::single(Identity{0, 1}).add(Identity{2, 1}));
  EXPECT_FALSE(known_play(g, order_at(g, TestPlayer::ALICE, 2)))
      << "where the hole card was the g1, neither the r2 nor the y2 plays";
}
