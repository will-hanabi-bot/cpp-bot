// Two cases where world replay used to strike a world it should not, or keep one
// it should not (tiiah/CONVENTION.md §1e, §1.3, v16.28.0).
//
//   * A view's BAND -- ranks it holds without naming the cards -- absorbs a world
//     card as one of those cards, but never an identity the team has already
//     NAMED in the hole: that is a duplicate, and the world strikes.
//   * A PAIRWISE ROW leaves out the hole cards we settled privately; a card too
//     high to land is then not a strike when one of those could be the card it
//     waits for -- unless the pair can rule that identity out by sight.
#include <gtest/gtest.h>

#include <vector>

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

constexpr Identity kG3{2, 3}, kG2{2, 2}, kY1{1, 1}, kR3{0, 3};

IdentitySet ids(std::initializer_list<Identity> list) {
  IdentitySet out = IdentitySet::empty();
  for (auto i : list) out = out.add(i);
  return out;
}

Game position() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"g3", "y1", "b3", "p3", "y4"},
                {"r4", "y5", "g5", "b5", "p5"}};
  opts.play_stacks = std::vector<int>{0, 0, 3, 0, 0};
  opts.starting = TestPlayer::BOB;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  return setup(std::move(opts));
}

bool struck_where(const std::vector<hanabi::tiiah::OpenWorld>& worlds, int order,
                  Identity id) {
  for (const hanabi::tiiah::OpenWorld& w : worlds) {
    for (const auto& [o, x] : w.assignment) {
      if (o == order && x == id) return w.struck;
    }
  }
  ADD_FAILURE() << "no world assigns that identity";
  return false;
}

}  // namespace

// Bob's {g3,y1} goes into the hole; the view holds green 3 over a band of 2-3.
// Unnamed, the g3 world is absorbed as the band's g3. Once some other hole card is
// NAMED the g3, the same world is a duplicate and strikes. Replay 2011885 T17.
TEST(TiiahWorldReplayBand, ANamedIdentityIsNotAbsorbedByTheBand) {
  Game g = position();
  const int bobs = order_at(g, TestPlayer::BOB, 1);
  g = hidden_action(std::move(g), TestPlayer::BOB, 1, /*reached_the_hole=*/true, "r5");
  g.with_meta(bobs, [](ConvData& m) { m.superposition = ids({kG3, kY1}); });
  const State base = g.state.with_stacks({0, 0, 3, 0, 0}).with_band({0, 0, 1, 0, 0});

  const auto unnamed = hanabi::tiiah::open_worlds(g, base, 1);
  EXPECT_FALSE(struck_where(unnamed, bobs, kG3)) << "the band's own unnamed g3";

  const int other = order_at(g, TestPlayer::CATHY, 2);  // stands in for the named g3
  g.with_meta(other, [](ConvData& m) { m.named_in_hole = IdentitySet::single(kG3); });
  const auto named = hanabi::tiiah::open_worlds(g, base, 1);
  EXPECT_TRUE(struck_where(named, bobs, kG3)) << "the g3 is named: this one is its dupe";
}

// Our own hole card, settled privately as the g1, is left out of a row's replay
// with its shared set {r3,g1}. Our other hole card, a {r2,g2}, is replayed on a
// row with green 0: its g2 world is waiting for a g1 the left-out card may be, so
// it is not a strike. Replay 2011887 T18, where striking it let the row claim r2.
TEST(TiiahWorldReplayBand, ARowDoesNotStrikeACardWaitingOnOurPrivateSettle) {
  Game g = position();
  const int settled = order_at(g, TestPlayer::ALICE, 1);
  g = hidden_action(std::move(g), TestPlayer::ALICE, 1, /*reached_the_hole=*/true);
  const int open = order_at(g, TestPlayer::ALICE, 1);
  g = hidden_action(std::move(g), TestPlayer::ALICE, 1, /*reached_the_hole=*/true);
  g.with_meta(settled, [](ConvData& m) {
    m.superposition = IdentitySet::empty();
    m.shared_left = ids({kR3, Identity{2, 1}});
  });
  g.with_meta(open, [](ConvData& m) { m.superposition = ids({Identity{0, 2}, kG2}); });
  const State base = g.state.with_stacks({1, 0, 0, 0, 0});

  const auto flat = hanabi::tiiah::open_worlds(g, base, std::vector<int>{0, 2});
  EXPECT_TRUE(struck_where(flat, open, kG2)) << "guard: read flat, the g2 strikes";

  const auto as_row = hanabi::tiiah::open_worlds(g, base, std::vector<int>{0, 2}, 64,
                                                 -1, /*shared=*/false, /*row=*/true);
  EXPECT_FALSE(struck_where(as_row, open, kG2))
      << "the g1 it waits for may be our privately settled card";
}
