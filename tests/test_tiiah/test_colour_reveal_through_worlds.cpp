// A COLOUR REVEAL IN SOME WORLDS (tiiah/CONVENTION.md §1b, v22.1.0, the user's
// ruling; replay 2021427 T13).
//
// A colour play reveal is judged on the pair's stacks (v22.5.0). When it fails there
// only because a hole card is unnamed, and some world of the hole cards makes it a
// reveal, that world is assumed: the call stands and the hole collapses for every
// seat. Unless the called card is trash in another world -- then the clue may be
// asking for a dupe to be thrown, and the collapse waits (v18.12.0).
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

void read_as(Game& g, int order, const IdentitySet& set) {
  g.state.deck[order].clued = true;
  g.with_thought(order, [&set](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    out.possible = set;
    return out;
  });
}

IdentitySet ids(std::initializer_list<const char*> shorts, const Game& g) {
  IdentitySet out = IdentitySet::empty();
  for (const char* s : shorts) out = out.add(g.state.expand_short(s));
  return out;
}

// Cathy's slot 2 is a g2 she knows is a 2; her slot 1 is a g4, not yet clued, that
// she knows is a 3 or higher. We
// throw `ours` into the hole -- each a set we cannot name -- and Bob clues Green to
// Cathy: it re-touches her 2 and newly touches the g4.
Game position(const std::vector<std::vector<const char*>>& ours) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"r4", "y4", "b4", "p4", "r3"},
                {"g4", "g2", "y3", "b3", "p3"}};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  read_as(g, order_at(g, TestPlayer::CATHY, 2), ids({"r2", "y2", "g2", "b2", "p2"}, g));
  // Her g4 has been told it is no 1 or 2 (as 2021427's o7 had), so the Green cannot
  // make it a direct play.
  {
    const int g4 = order_at(g, TestPlayer::CATHY, 1);
    const IdentitySet high = IdentitySet::create(
        [](Identity i) { return i.rank >= 3; },
        static_cast<int>(g.state.variant->suits.size()) * 5);
    g.with_thought(g4, [&high](const Thought& t) {
      Thought out = t;
      out.inferred = t.inferred.intersect(high);
      out.possible = t.possible.intersect(high);
      return out;
    });
  }
  for (std::size_t i = 0; i < ours.size(); ++i) {
    const int slot = 5 - static_cast<int>(i);
    IdentitySet set = IdentitySet::empty();
    for (const char* s : ours[i]) set = set.add(g.state.expand_short(s));
    read_as(g, order_at(g, TestPlayer::ALICE, slot), set);
    g = hidden_action(std::move(g), TestPlayer::ALICE, slot, /*reached_the_hole=*/true);
    if (i + 1 < ours.size()) {
      g = hidden_action(std::move(g), TestPlayer::BOB, 5, /*reached_the_hole=*/false, "r5");
      g = hidden_action(std::move(g), TestPlayer::CATHY, 5, /*reached_the_hole=*/false,
                        "y5");
    }
  }
  return take_turn(std::move(g), "Bob clues green to Cathy");
}

// Cathy's g2, wherever her hand has moved it.
int cathys_g2(const Game& g) {
  for (int o : g.state.hands[static_cast<int>(TestPlayer::CATHY)]) {
    if (g.state.deck[o].id() == g.state.expand_short("g2")) return o;
  }
  return -1;
}

}  // namespace

// Our `{g1,b1}` went in. On the stacks we can name green is on 0, so the Green
// is no reveal there, and the g4 cannot be a direct play. Where our card was the g1
// it reveals the g2: that world is assumed, our card is the g1 for every seat, and
// the g2 is called.
TEST(TiiahColourRevealThroughWorlds, TheRevealCallsAndTheHoleCollapses) {
  Game g = position({{"g1", "b1"}});
  const int g2 = cathys_g2(g);
  ASSERT_GE(g2, 0);
  EXPECT_EQ(g.meta[g2].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[g2].inferred, ids({"g2"}, g));
  EXPECT_EQ(g.state.common_play_stacks[2], 1) << "green on 1 for every seat";
}

// Control (2014076's shape): our `{g1,b1}` and `{g2,r1}` went in. Where they were the
// g1 and the r1 the Green reveals the g2; where they were the g1 and the g2, Cathy's
// g2 is trash, and the clue may be asking for it to be thrown. The call stands, as
// §1e's (v22.5.0: the reveal holds on that world's pair frame), but nothing
// collapses: that waits for the holder (v18.12.0).
TEST(TiiahColourRevealThroughWorlds, TrashInAnotherWorldDefers) {
  Game g = position({{"g1", "b1"}, {"g2", "r1"}});
  const int g2 = cathys_g2(g);
  ASSERT_GE(g2, 0);
  EXPECT_EQ(g.meta[g2].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.state.common_play_stacks[2], 0) << "nothing collapsed";
}
