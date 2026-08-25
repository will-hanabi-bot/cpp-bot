// A reaction's negative needs the alternative to have EXISTED.
//
// Every one of these negatives is an argument of the form "if that slot had
// been an X, the clue would have named it instead". It only holds if the clue
// COULD have named it: the pairing `react_slot + target_slot = anchor (mod 5)`
// means naming receiver slot S would have required the REACTER to action his
// slot `calc_slot(V, S, 5)`, on a card that could carry the reading. When his
// paired slot could not, no such clue was ever available and the negative is
// unfounded.
//
// `arm_reaction_elim` works this out per slot at capture time and stores the
// filtered sets on `Game::PendingReactionElim`. These tests read those sets
// directly, right after the reacter acts -- `test_deferred_elim.cpp` covers the
// other half, which slots each set is then applied to.
//
// Replay 1971882 is the case: an r4 was struck off the receiver's slot as "not
// one away", but naming it would have needed the reacter to blind-play an r3
// from a slot whose empathy was {g1,g3,g5,d3}.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Red on 1 and green on 1, so the playables are {r2,y1,g2,b1,p1} and the
// one-aways {r3,y2,g3,b2,p2}.
//
// Alice clues rank 2 to Cathy: anchor 2. Cathy's leftmost playable is the r2 at
// slot 1, so `calc_slot(2, 1, 5) = 1` sends Bob to his slot 1. Under a rank
// clue the parity is EVEN -- a double play -- so a finesse was on the table.
SetupOptions filter_opts() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.play_stacks = {1, 0, 1, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},   // Alice (giver, us)
      {"g2", "y4", "b4", "p4", "y3"},   // Bob (reacter): slot 1 is playable
      {"r2", "y5", "p5", "b5", "p4"},   // Cathy (receiver)
  };
  use_reactor0(opts);
  return opts;
}

// Drive the clue and the reaction, leaving the capture armed and unfired.
Game armed(SetupOptions opts) {
  Game g = setup(std::move(opts));
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");
  g = take_turn(std::move(g), "Bob plays g2 (slot 1)", "b1");
  return g;
}

const Game::PendingReactionElim& cap(const Game& g) {
  return g.pending_reaction_elim;
}

}  // namespace

// --- the capture is armed and filtered ------------------------------------

TEST(Reactor0ReactionElimFilter, TheCaptureIsArmedWithPerSlotSets) {
  Game g = armed(filter_opts());
  ASSERT_TRUE(cap(g).active) << "guard: the reaction armed the capture";
  ASSERT_EQ(cap(g).direct_elim.size(), 5u);
  ASSERT_EQ(cap(g).finesse_elim.size(), 5u);
  ASSERT_EQ(cap(g).trash_elim.size(), 5u);
}

// --- the finesse filter ---------------------------------------------------

// Anchor 2, so receiver slot 4 pairs with `calc_slot(2, 4, 5) = 3` -- Bob's
// slot 3. It is unclued, so his empathy there is wide and admits the b1 that a
// "receiver slot 4 is the b2" finesse would need. The negative is earned.
TEST(Reactor0ReactionElimFilter, AnAvailableConnectorEarnsTheNegative) {
  Game g = armed(filter_opts());
  const Identity b2{3, 2};  // one away: blue is on 0, so b1 connects
  EXPECT_TRUE(cap(g).finesse_elim[3].contains(b2))
      << "Bob's slot 3 could have been the b1, so the finesse existed";
}

// The same slot, with Bob's paired card clued to a suit that cannot be the
// connector. Now no clue could have named it, and the negative is unfounded --
// this is the 1971882 shape.
TEST(Reactor0ReactionElimFilter, AMissingConnectorBlocksTheNegative) {
  Game g = setup(filter_opts());
  // Narrow Bob's slot 3 to yellows, the way a yellow clue would have. Setting
  // the empathy directly keeps the fixture to one variable: everything else is
  // identical to `AnAvailableConnectorEarnsTheNegative`, so the only thing that
  // can explain a different answer is the missing connector.
  //
  // `common` is the set `effective_possible_for` reads.
  const int bob_slot3 = order_at(g, TestPlayer::BOB, 3);
  const IdentitySet yellows = IdentitySet::create(
      [](Identity i) { return i.suit_index == 1; },
      static_cast<int>(g.state.variant->suits.size()) * 5);
  g.with_thought(bob_slot3, [&yellows](const Thought& th) {
    Thought out = th;
    out.possible = th.possible.intersect(yellows);
    out.inferred = th.inferred.intersect(yellows);
    return out;
  });
  g.players[1] = g.common;

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");
  g = take_turn(std::move(g), "Bob plays g2 (slot 1)", "b1");

  ASSERT_TRUE(cap(g).active) << "guard: armed";
  const Identity b2{3, 2};
  EXPECT_FALSE(cap(g).finesse_elim[3].contains(b2))
      << "Bob's slot 3 can only be yellow, so it could never have been the b1 "
         "-- the finesse that would have named this slot did not exist";
}

// Each set holds only identities of its own kind, judged as of the CLUE, not as
// of whenever the receiver gets round to acting. Bob's play advances a stack
// between the two, so this has to compare against the pre-reaction position.
TEST(Reactor0ReactionElimFilter, EachSetHoldsOnlyItsOwnKindAsOfTheClue) {
  Game before = setup(filter_opts());
  before = take_turn(std::move(before), "Alice clues 2 to Cathy");
  Game g = before;
  g = take_turn(std::move(g), "Bob plays g2 (slot 1)", "b1");
  ASSERT_TRUE(cap(g).active) << "guard: armed";

  const State& s = before.state;
  for (int i = 0; i < 5; ++i) {
    for (Identity id : cap(g).direct_elim[i]) {
      EXPECT_TRUE(s.is_playable(id)) << "direct_elim holds only playables";
    }
    for (Identity id : cap(g).finesse_elim[i]) {
      EXPECT_EQ(s.playable_away(id), 1) << "finesse_elim holds only one-aways";
    }
    for (Identity id : cap(g).trash_elim[i]) {
      EXPECT_TRUE(s.is_basic_trash(id)) << "trash_elim holds only trash";
    }
  }
}

// --- parity ---------------------------------------------------------------

// A finesse is a double play, so it only exists in the EVEN bucket. A colour
// clue is the odd parity in a plain variant -- exactly one play -- so there was
// no finesse to prefer and `finesse_elim` stays empty. That is what lets
// `fire_reaction_elim` apply it without re-testing the parity.
TEST(Reactor0ReactionElimFilter, OddParityLeavesTheFinesseSetEmpty) {
  SetupOptions opts = filter_opts();
  Game g = setup(std::move(opts));
  // Red's colour value is 1: anchor 1, so Cathy's slot 1 (the playable r2)
  // pairs with `calc_slot(1, 1, 5) = 5` -- Bob's slot 5.
  g = take_turn(std::move(g), "Alice clues Red to Cathy");
  ASSERT_FALSE(g.waiting.empty()) << "guard: the clue read as reactive";
  g = take_turn(std::move(g), "Bob discards y3 (slot 5)", "b1");

  if (!cap(g).active) GTEST_SKIP() << "the reaction did not arm here";
  for (int i = 0; i < static_cast<int>(cap(g).finesse_elim.size()); ++i) {
    EXPECT_TRUE(cap(g).finesse_elim[i].is_empty())
        << "slot " << (i + 1) << ": no finesse exists in the odd bucket";
  }
}
