// §1.3: a pairwise row takes what holds in every world that survives for the seat
// behind it (tiiah/CONVENTION.md, v16.18.0).
//
// A row is what we know seat `p` knows, and it leaves out `p`'s own hidden plays
// because `p` cannot name them. But `p` can still REASON about them: across the
// worlds they leave open, rule 6 refutes the ones in which a play struck, and a
// height reached in all the survivors is one `p` holds.
//
// Replay 2010329 is the case. yagami's two hole cards each read {g1,b1}, so
// (g1,g1) and (b1,b1) each strike and green and blue are on 1 whichever way round
// the other two were — which is what made its rank 2 at T14 readable at all.
//
// The enumeration takes OUR hole cards as well as `p`'s, and that is what keeps the
// two seats saying the same thing: `p` computing the same pair's row runs over the
// same two seats' cards from the same base, so the two rows agree. It is also what
// stops the row claiming more than `p` believes — the third test.
//
// No reversed-suit test: `data/variants.json` has 44 Throw It in a Hole variants
// and not one of them reverses a suit, so the reversed arm of the floor is written
// for the idiom's sake (`st.reversed ? prev() : next()` everywhere in this tree)
// and cannot be reached from a real table.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us)
      {"g1", "b1", "y4", "g4", "b4"},  // Bob: two cards for the hole
      {"r4", "y4", "g5", "b5", "p5"},
  };
  opts.starting = TestPlayer::BOB;
  use_tiiah(opts);
  return opts;
}

// A reading written onto both halves of the empathy, so `Game::elim` has nothing to
// widen back. The real identity has to be in the set or the fixture is lying to the
// model about the deck.
void read_as(Game& g, int order, const IdentitySet& set) {
  g.state.deck[order].clued = true;
  g.with_thought(order, [&set](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    out.possible = set;
    return out;
  });
}

IdentitySet two_of(Identity a, Identity b) {
  return IdentitySet::empty().add(a).add(b);
}

const std::vector<int>& row_of(const Game& g, TestPlayer p) {
  return g.state.pairwise_play_stacks[static_cast<int>(p)];
}

}  // namespace

// Bob throws two {g1,b1} cards in the hole. Neither play tells his own row
// anything on its own; the PAIR of them tells it both suits are on 1.
//
// Bob plays twice running, with no turn taken in between. The engine replays
// whatever the wire said and nothing on this path reads `current_player_index`; a
// fixture that spent two more turns getting the turn order right would be testing
// the harness rather than the rule.
TEST(TiiahRowFromOwnWorlds, TwoHolesOverTwoSuitsRaiseTheRow) {
  Game g = setup(opts_for());
  const int first = order_at(g, TestPlayer::BOB, 1);
  const int second = order_at(g, TestPlayer::BOB, 2);
  const IdentitySet g1b1 = two_of(Identity{2, 1}, Identity{3, 1});
  read_as(g, first, g1b1);
  read_as(g, second, g1b1);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y3");
  const std::vector<int> flat{0, 0, 0, 0, 0};
  ASSERT_EQ(row_of(g, TestPlayer::BOB), flat)
      << "one card leaves two worlds that disagree, so his row learns nothing";
  ASSERT_EQ(g.state.play_stacks[2], 1) << "though we watched it, so we know";
  ASSERT_EQ(order_at(g, TestPlayer::BOB, 2), second)
      << "guard: the draw went to slot 1 and left the second card in slot 2";

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/2,
                    /*reached_the_hole=*/true, "p3");

  const std::vector<int> both{0, 0, 1, 1, 0};
  EXPECT_EQ(row_of(g, TestPlayer::BOB), both)
      << "(g1,g1) and (b1,b1) each strike, so every surviving world has green and "
         "blue on 1 -- and Bob can see that as well as we can";
  EXPECT_EQ(g.state.play_stacks[2], 1);
  EXPECT_EQ(g.state.play_stacks[3], 1);
  EXPECT_EQ(g.state.strikes, 0) << "and nobody was presumed to have struck";
}

// The control: ONE hole card over two suits leaves two strike-free worlds that
// disagree, and a row may only take what holds in both. Cathy's row does move --
// she WATCHED the card, which is the ordinary rule and not this one.
TEST(TiiahRowFromOwnWorlds, OneHoleRaisesNothingForItsOwner) {
  Game g = setup(opts_for());
  read_as(g, order_at(g, TestPlayer::BOB, 1),
          two_of(Identity{2, 1}, Identity{3, 1}));

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y3");

  const std::vector<int> flat{0, 0, 0, 0, 0};
  const std::vector<int> watched{0, 0, 1, 0, 0};
  EXPECT_EQ(row_of(g, TestPlayer::BOB), flat)
      << "his own play is the one thing his row cannot have";
  EXPECT_EQ(row_of(g, TestPlayer::CATHY), watched)
      << "Cathy saw the card leave his hand, so she knows the g1 landed";
}

// A lone hole card CAN raise its owner's row, when the worlds it leaves do not
// disagree: a {g2,b1} read against an empty green stack can only have been the b1,
// because the g2 would have struck.
TEST(TiiahRowFromOwnWorlds, ALoneHoleCardRaisesTheRowWhenOnlyOneWorldLands) {
  SetupOptions opts = opts_for();
  opts.hands[1] = {"b1", "y4", "g4", "b4", "p4"};
  Game g = setup(std::move(opts));
  read_as(g, order_at(g, TestPlayer::BOB, 1),
          two_of(Identity{2, 2}, Identity{3, 1}));

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y3");

  const std::vector<int> blue{0, 0, 0, 1, 0};
  EXPECT_EQ(row_of(g, TestPlayer::BOB), blue)
      << "green is on 0, so a g2 could not have landed -- Bob knows it was the b1";
}

// OUR hole card is in the enumeration too, and here it withholds the claim above.
// Ours reads {g1,y1}: if it was the g1 then Bob's card could have been the g2 and
// landed, so Bob can no longer conclude his was the b1. He reasons the same way --
// he watched our card go in and cannot tell which of the two it was.
TEST(TiiahRowFromOwnWorlds, OurOwnHoleCardCanWithholdTheClaim) {
  SetupOptions opts = opts_for();
  opts.hands[1] = {"b1", "y4", "g4", "b4", "p4"};
  Game g = setup(std::move(opts));
  const int mine = order_at(g, TestPlayer::ALICE, 1);
  read_as(g, mine, two_of(Identity{2, 1}, Identity{1, 1}));
  read_as(g, order_at(g, TestPlayer::BOB, 1),
          two_of(Identity{2, 2}, Identity{3, 1}));

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);
  ASSERT_TRUE(g.meta[mine].superposed())
      << "guard: our card lands whichever it was, so it is still unsettled";

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y3");

  const std::vector<int> flat{0, 0, 0, 0, 0};
  EXPECT_EQ(row_of(g, TestPlayer::BOB), flat)
      << "with a g1 of ours possibly in the hole ahead of his card, the g2 world "
         "survives too -- and the two worlds disagree about blue";
}
