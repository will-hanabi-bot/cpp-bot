// A card the team names later is evidence about a reactive's target walk
// (tiiah/CONVENTION.md §1e "World feasibility", v20.21.0, the user's ruling;
// replay 2019598).
//
// The receiver held slot 1 and slot 5 when a reactive called slot 1. Slot 1 went
// into the hole as {y1, b2}: the y1 direct, the b2 a finesse. Slot 5 later goes into
// the hole NAMED a g1 (`named_in_hole`), a direct playable. The walk takes a direct
// playable before a finesse, so in the world where slot 1 is the b2 the walk would
// have called slot 5 -- that world is refuted. That is the only thing a named card
// is evidence for.

#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/tiiah/superposition.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;
using hanabi::tiiah::OpenWorld;

namespace {

constexpr Identity kY1{1, 1}, kG1{2, 1}, kB2{3, 2};

Game named_card_game() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"y1", "g4", "g3", "b4", "g1"},
                {"r4", "y4", "g5", "b5", "p5"}};
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return setup(std::move(opts));
}

// The team names the card in the hole (a shared settle).
void name_as(Game& g, int order, Identity id) {
  g.meta[order].named_in_hole = IdentitySet::single(id);
}

// A card still in hand whose team reading is one identity: not a name yet.
void read_as(Game& g, int order, Identity id) {
  g.with_thought(order, [id](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(id);
    return out;
  });
}

OpenWorld slot1_is(int order, Identity id) {
  OpenWorld w;
  w.assignment = {{order, id}};
  return w;
}

}  // namespace

TEST(TiiahNamedCardFeasibility, ANamedDirectPlayableRefutesTheFinesseWorld) {
  Game g = named_card_game();
  const std::vector<int> hand = g.state.hands[1];
  g.reaction_records.push_back(
      ReactionRecord{1, /*receiver=*/1, hand, {}, {0, 0, 0, 0, 0}, hand[0]});
  name_as(g, hand[4], kG1);

  EXPECT_FALSE(hanabi::tiiah::world_feasible(g, slot1_is(hand[0], kB2), true))
      << "the b2 is a finesse, and the named g1 a direct playable in the same hand";
  EXPECT_TRUE(hanabi::tiiah::world_feasible(g, slot1_is(hand[0], kY1), true))
      << "the y1 is direct and leftmost";
  EXPECT_TRUE(hanabi::tiiah::world_feasible(g, slot1_is(hand[0], kB2), false))
      << "only the settle-time prune counts named cards";
}

// Control: a card nobody can name constrains nothing.
TEST(TiiahNamedCardFeasibility, AnUnnamedCardConstrainsNothing) {
  Game g = named_card_game();
  const std::vector<int> hand = g.state.hands[1];
  g.reaction_records.push_back(
      ReactionRecord{1, 1, hand, {}, {0, 0, 0, 0, 0}, hand[0]});
  EXPECT_TRUE(hanabi::tiiah::world_feasible(g, slot1_is(hand[0], kB2), true));
}

// Control: a card already clued when the reactive was given may have been passed
// over as a play the receiver already knew (replay 2011327 T28).
TEST(TiiahNamedCardFeasibility, ACardCluedBeforeTheReactiveConstrainsNothing) {
  Game g = named_card_game();
  const std::vector<int> hand = g.state.hands[1];
  ReactionRecord r{1, 1, hand, {}, {0, 0, 0, 0, 0}, hand[0]};
  r.clued = {hand[4]};
  g.reaction_records.push_back(r);
  name_as(g, hand[4], kG1);
  EXPECT_TRUE(hanabi::tiiah::world_feasible(g, slot1_is(hand[0], kB2), true));
}

// Control: a reading of one identity on a card still in hand is not a name.
TEST(TiiahNamedCardFeasibility, AHandCardReadAsOneIdentityIsNotAName) {
  Game g = named_card_game();
  const std::vector<int> hand = g.state.hands[1];
  g.reaction_records.push_back(
      ReactionRecord{1, 1, hand, {}, {0, 0, 0, 0, 0}, hand[0]});
  read_as(g, hand[4], kG1);
  EXPECT_TRUE(hanabi::tiiah::world_feasible(g, slot1_is(hand[0], kB2), true));
}

// Control: a named card does not open the rules on the world's own cards to a
// lone world card. The target alone, read as neither direct nor one away, is not
// judged -- its reading already came from that very reaction.
TEST(TiiahNamedCardFeasibility, ANamedCardDoesNotJudgeALoneWorldCard) {
  Game g = named_card_game();
  const std::vector<int> hand = g.state.hands[1];
  g.reaction_records.push_back(
      ReactionRecord{1, 1, hand, {}, {0, 0, 0, 0, 0}, hand[0]});
  name_as(g, hand[4], kG1);
  constexpr Identity kB4{3, 4};
  EXPECT_TRUE(hanabi::tiiah::world_feasible(g, slot1_is(hand[0], kB4), true));
}
