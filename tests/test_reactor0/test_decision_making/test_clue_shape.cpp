// Reactor0 clue-shape classification (src/conventions/reactor0/decision.cpp,
// DECISION_MAKING.md "General Clue Evaluation List").
//
// The priority list is written over SHAPES — reactive play clue, reactive
// discard clue, double discard, stable play, stable discard, trash reveal,
// lock — so getting the classifier right is the whole of phase 1's correctness.
// These tests pin it directly, the way test_clue_tier.cpp pins `clue_tier`,
// rather than through `take_action`, whose answer also depends on the priority
// walk and the tier gate.
//
// Two properties are load-bearing and each has its own test below:
//
//   * classification is RESULT-oriented. `CALLED_TO_PLAY` names the Play
//     button, not the outcome; on an inverted suit pressing Play is a pitch
//     (a discard) and pressing Discard is a chuck (a play, or a strike).
//   * Bob acts FIRST. The receiver's card is judged against the stacks Bob
//     leaves behind, which is what makes a finesse classify as two plays rather
//     than a play and a strike.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::ClueShape;
using hanabi::reactor0::Outcome;

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

}  // namespace

// --- outcome_of: the result-orientation primitive -------------------------

// On a plain suit the buttons mean what they say: Play plays (or strikes), and
// Discard discards.
TEST(ClueShape, OutcomeOnAPlainSuitFollowsTheButton) {
  SetupOptions opts;
  opts.hands = {
      {"g1", "g2", "y1", "y2", "y3"},
      {"r1", "r3", "b4", "p4", "b5"},
      {"r2", "b2", "p3", "g3", "y4"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  const State& s = g.state;

  const int bob_r1 = s.hands[1][0];  // playable
  const int bob_r3 = s.hands[1][1];  // two away
  EXPECT_EQ(hanabi::reactor0::outcome_of(s, bob_r1, CardStatus::CALLED_TO_PLAY),
            Outcome::PLAY);
  EXPECT_EQ(hanabi::reactor0::outcome_of(s, bob_r3, CardStatus::CALLED_TO_PLAY),
            Outcome::STRIKE)
      << "pressing Play on a card that is not playable is a misplay";
  EXPECT_EQ(
      hanabi::reactor0::outcome_of(s, bob_r1, CardStatus::CALLED_TO_DISCARD),
      Outcome::DISCARD);
  EXPECT_EQ(
      hanabi::reactor0::outcome_of(s, bob_r3, CardStatus::CALLED_TO_DISCARD),
      Outcome::DISCARD);
}

// On an inverted suit the buttons swap, and the asymmetry matters: a pitch can
// never strike, a chuck can.
TEST(ClueShape, OutcomeOnAnInvertedSuitSwapsTheButtons) {
  SetupOptions opts;
  opts.hands = {
      {"r1", "r2", "b1", "b2", "b3"},
      {"o1", "o3", "r3", "b4", "r4"},
      {"o2", "r2", "b2", "r3", "b3"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0};
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  const State& s = g.state;
  ASSERT_TRUE(s.variant->suits[2].suit_type.inverted);

  const int bob_o1 = s.hands[1][0];  // playable orange
  const int bob_o3 = s.hands[1][1];  // two away
  EXPECT_EQ(hanabi::reactor0::outcome_of(s, bob_o1, CardStatus::CALLED_TO_PLAY),
            Outcome::DISCARD)
      << "pressing Play on an orange card is a PITCH — it reaches the discard "
         "pile and regains a clue, and can never strike";
  EXPECT_EQ(hanabi::reactor0::outcome_of(s, bob_o3, CardStatus::CALLED_TO_PLAY),
            Outcome::DISCARD)
      << "a pitch cannot strike whatever the identity";
  EXPECT_EQ(
      hanabi::reactor0::outcome_of(s, bob_o1, CardStatus::CALLED_TO_DISCARD),
      Outcome::PLAY)
      << "pressing Discard on a playable orange is a CHUCK that advances it";
  EXPECT_EQ(
      hanabi::reactor0::outcome_of(s, bob_o3, CardStatus::CALLED_TO_DISCARD),
      Outcome::STRIKE)
      << "chucking an orange that is not playable is a misplay";
}

// --- read_clue on the stable side -----------------------------------------

// A stable colour clue that names a playable card in Bob's hand is a stable
// play clue — priority 3.1's subject.
TEST(ClueShape, StableColourNamingAPlayableReadsAsStablePlay) {
  SetupOptions opts;
  opts.hands = {
      {"g3", "g4", "y4", "g5", "y5"},
      {"r1", "b2", "b3", "p3", "b4"},
      {"y2", "b2", "p4", "g2", "y3"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto r = read(g, make_clue(g, 0, 1, ClueKind::COLOUR, /*Red=*/0));
  EXPECT_EQ(r.shape, ClueShape::STABLE_PLAY)
      << "got " << hanabi::reactor0::shape_name(r.shape);
  EXPECT_EQ(r.stable_subject, g.state.hands[1][0])
      << "the named card is Bob's r1 on slot 1";
}

// --- read_clue on the reactive side ---------------------------------------

// Rank Phase A: both designated cards are playable now, so the clue is a
// reactive play clue — priority 1.
TEST(ClueShape, ReactiveRankDoublePlayReadsAsReactivePlay) {
  SetupOptions opts;
  opts.hands = {
      {"g4", "g5", "y5", "g3", "y4"},
      // The rank-1 clue's anchor is its value, 1. Cathy's leftmost playable is
      // her slot 1, and calc_slot(anchor=1, target=1, hand=5) = 5 — so Phase A
      // needs Bob's SLOT 5 to be playable, not his slot 1.
      {"b3", "p3", "b4", "p4", "r1"},
      {"b1", "r3", "y3", "p2", "g2"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  // Alice clues Cathy, so dispatch is reactive with Bob as reacter (§1a).
  auto r = read(g, make_clue(g, 0, 2, ClueKind::RANK, 1));
  ASSERT_NE(r.shape, ClueShape::OTHER)
      << "the fixture did not produce a reactive reading at all";
  EXPECT_EQ(r.reacter_side.outcome, Outcome::PLAY);
  EXPECT_EQ(r.receiver_side.outcome, Outcome::PLAY);
  EXPECT_EQ(r.shape, ClueShape::REACTIVE_PLAY)
      << "got " << hanabi::reactor0::shape_name(r.shape);
}

// A stable clue must never be read through the waiting connection, even when a
// stale one from an earlier turn is still sitting in the game. The freshness
// guard in `wc_is_fresh` is what stops that, and this pins it.
TEST(ClueShape, StaleWaitingConnectionDoesNotLeakIntoAStableReading) {
  SetupOptions opts;
  opts.hands = {
      {"g3", "g4", "y4", "g5", "y5"},
      {"r1", "b2", "b3", "p3", "b4"},
      {"y2", "b2", "p4", "g2", "y3"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const Action clue = make_clue(g, 0, 1, ClueKind::COLOUR, /*Red=*/0);
  const auto clean = read(g, clue);

  // Inject a stale connection: a different giver, a different turn.
  Game dirty = g;
  ReactorWC stale;
  stale.giver = 2;
  stale.reacter = 0;
  stale.receiver = 1;
  stale.receiver_hand = dirty.state.hands[1];
  stale.focus_slot = 1;
  stale.turn = dirty.state.turn_count - 1;
  stale.react_order = dirty.state.hands[0][0];
  dirty.waiting.push_back(stale);

  const auto dirtied = read(dirty, clue);
  EXPECT_EQ(dirtied.shape, clean.shape)
      << "a stale waiting connection changed a STABLE clue's reading; the "
         "freshness guard is not holding";
  EXPECT_EQ(dirtied.stable_subject, clean.stable_subject);
}
