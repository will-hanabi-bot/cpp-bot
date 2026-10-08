// The shared DECISION layer reading a TIIAH ORDINARY reactive
// (tiiah/CONVENTION.md §1c's dispatch table; reactor0/DECISION_MAKING.md,
// "Whose hand a rung is talking about").
//
// The mirror of `test_clue_reading.cpp`, which pins the REVERSE direction. Both
// halves matter and they fail differently: with the dispatch one-armed, a clue
// to Cathy was classified stable, so no rung could price the double play it
// actually was — and our bots could not give one either.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;
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

// Replay 2008177 T3's shape, moved to four suits (buckets {R,Y}, {G}, {B}: the
// three-bucket rule). Nothing played; Cathy's only playable is the g1 on slot 4,
// and Bob's slot 3 is the r1 the sum rule names.
SetupOptions replay_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b4", "b5", "r1", "y1", "b3"},
      {"y4", "b3", "y5", "g1", "y2"},
  };
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 0, 0};
  opts.clue_tokens = 6;
  use_tiiah(opts);
  return opts;
}

}  // namespace

// Both named cards play, so the rungs see the shape priority 1 is written over.
TEST(TiiahOrdinaryReading, AClueToCathyReadsAsAReactivePlay) {
  Game g = setup(replay_opts());

  auto r = read(g, make_clue(g, 0, /*target=*/2, ClueKind::RANK, 2));

  ASSERT_NE(r.shape, ClueShape::OTHER)
      << "the reading fell through, which is what a stable classification of a "
         "reactive clue looks like from here";
  EXPECT_EQ(r.reacter_side.outcome, Outcome::PLAY) << "Bob's r1";
  EXPECT_EQ(r.receiver_side.outcome, Outcome::PLAY) << "Cathy's g1, after it";
  EXPECT_EQ(r.shape, ClueShape::REACTIVE_PLAY)
      << "got " << hanabi::reactor0::shape_name(r.shape);
}

// ...and the seats are the ordinary way round, which is the half v16.6.0's
// `dispatch_reacter` got wrong for this variant: it handed back Cathy whatever
// the position.
TEST(TiiahOrdinaryReading, BobReactsAndCathyReceives) {
  Game g = setup(replay_opts());

  auto r = read(g, make_clue(g, 0, /*target=*/2, ClueKind::RANK, 2));

  EXPECT_EQ(r.reacter_side.holder, 1) << "Bob reacts";
  EXPECT_EQ(r.receiver_side.holder, 2) << "Cathy receives, and is the target";
  EXPECT_EQ(r.reacter_side.order, order_at(g, TestPlayer::BOB, 3))
      << "the sum rule: anchor 2, target slot 4, so slot 3";
}

// End to end: a clue that gets two cards playing is priority 1, so it is what
// the bot gives from this position.
TEST(TiiahOrdinaryReading, TheDoublePlayIsWhatTheBotGives) {
  Game g = setup(replay_opts());

  PerformAction a;
  ASSERT_NO_THROW(a = g.take_action());
  ASSERT_TRUE(hanabi::is_clue(a)) << "two plays outrank anything else here";
  EXPECT_EQ(std::visit([](const auto& v) { return v.target; }, a), 2)
      << "aimed at Cathy, which is what makes it reactive";
}
