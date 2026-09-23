// Which SEAT each side of a `ClueReading` belongs to
// (reactor0/DECISION_MAKING.md, "Whose hand a rung is talking about").
//
// Under reactor0 the reacter is always Bob and the receiver always Cathy, so a
// rung that wants "the hand doing the throwing" could read `bob_of(g)` and be
// right. It is right by coincidence: which seat plays which role is a property
// of the convention, not of the geometry, and Throw It in a Hole swaps the two.
//
// So `read_clue` fills each designation's `holder` from the waiting connection,
// and these tests pin that it agrees with the seat reactor0's rungs used to
// name — including in a target-parity variant, where a clue to BOB is reactive
// and Cathy still receives.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::ClueShape;

namespace {

Action make_clue(const Game& g, int giver, int target, ClueKind kind,
                 int value) {
  auto touched = g.state.clue_touched(g.state.hands[target], kind, value);
  return Action{
      ClueAction{giver, target, std::move(touched), BaseClue{kind, value}}};
}

hanabi::reactor0::ClueReading read(const Game& g, const Action& clue) {
  Game hypo = g.simulate(clue);
  return hanabi::reactor0::read_clue(g, hypo, std::get<ClueAction>(clue));
}

bool holds(const Game& g, int player, int order) {
  const auto& hand = g.state.hands[player];
  return std::find(hand.begin(), hand.end(), order) != hand.end();
}

}  // namespace

// The ordinary case: Alice clues Cathy, Bob reacts, Cathy receives, and each
// side's `holder` is the hand its order is actually in.
TEST(ReadingHolders, EachSideNamesTheHandItsCardIsIn) {
  SetupOptions opts;
  opts.hands = {
      {"g4", "g5", "y5", "g3", "y4"},
      {"b3", "p3", "b4", "p4", "r1"},
      {"b1", "r3", "y3", "p2", "g2"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto r = read(g, make_clue(g, 0, 2, ClueKind::RANK, 1));
  ASSERT_EQ(r.shape, ClueShape::REACTIVE_PLAY)
      << "the fixture did not produce a reactive reading at all";

  EXPECT_EQ(r.reacter_side.holder, 1) << "Bob reacts under reactor0";
  EXPECT_EQ(r.receiver_side.holder, 2) << "Cathy receives";
  EXPECT_TRUE(holds(g, r.reacter_side.holder, r.reacter_side.order));
  EXPECT_TRUE(holds(g, r.receiver_side.holder, r.receiver_side.order));
}

// A stable reading designates nobody, so neither holder is set — which is what
// keeps `discarded_sides` from crediting a discard to seat -1.
TEST(ReadingHolders, AStableReadingLeavesBothHoldersUnset) {
  SetupOptions opts;
  opts.hands = {
      {"g4", "g5", "y5", "g3", "y4"},
      {"r1", "p3", "b4", "p4", "b3"},
      {"b1", "r3", "y3", "p2", "g2"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  // A clue to BOB is stable under reactor0's positional dispatch.
  auto r = read(g, make_clue(g, 0, 1, ClueKind::COLOUR, 0));
  ASSERT_NE(r.shape, ClueShape::REACTIVE_PLAY);
  EXPECT_EQ(r.reacter_side.holder, -1);
  EXPECT_EQ(r.receiver_side.holder, -1);
}

// Target parity is the case where the two readings of "Bob" come apart in
// reactor0's own play: a clue to BOB is reactive there, and the seat it names a
// card for is still Cathy. The holders have to follow the roles, not the target.
TEST(ReadingHolders, UnderTargetParityTheClueGoesToBobAndCathyStillReceives) {
  SetupOptions opts;
  opts.hands = {
      {"g4", "g5", "y5", "g3", "y4"},
      {"b3", "p3", "b4", "p4", "r1"},
      {"b1", "r3", "y3", "p2", "g2"},
  };
  opts.variant_name = "Alternating Clues (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  for (int value : {1, 2, 3, 4, 5}) {
    auto r = read(g, make_clue(g, 0, /*target=*/1, ClueKind::RANK, value));
    if (r.reacter_side.holder < 0) continue;  // not a reactive reading
    EXPECT_EQ(r.reacter_side.holder, 1) << "rank " << value;
    EXPECT_EQ(r.receiver_side.holder, 2)
        << "rank " << value
        << ": the clue went to Bob, but Cathy is the seat it names a card for";
    EXPECT_TRUE(holds(g, r.receiver_side.holder, r.receiver_side.order));
  }
}
