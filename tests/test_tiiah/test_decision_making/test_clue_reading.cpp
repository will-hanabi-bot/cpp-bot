// How the shared DECISION layer reads a Throw It in a Hole clue
// (reactor0/DECISION_MAKING.md, "Whose hand a rung is talking about";
// tiiah/CONVENTION.md §2).
//
// The rungs are reactor0's and the readings they price come from `read_clue`,
// which until v16.6.0 asked reactor0's positional dispatch. Under TIIAH that is
// backwards in both directions: the reactive clue is the one TO Bob, and the
// seat that reacts is Cathy. A clue read the wrong way round is worse than one
// not read at all — a reactive would have looked like a harmless stall to rung
// 4.5 — so these pin the reading itself rather than the action it leads to.
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

// Bob holds a known r1 with an r3 behind it, and Cathy the r2 that bridges
// them: the reverse-reactive shape of §1c.
SetupOptions reverse_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "r3", "y4", "g4", "b4"},
      {"r2", "y3", "g3", "b3", "p3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

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

// The clue goes to Bob and the decision layer reads it as a reactive — the
// opposite of reactor0, where a clue to Bob is stable by definition.
TEST(TiiahClueReading, AClueToBobReadsAsReactive) {
  Game g = setup(reverse_opts());
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  auto r = read(g, make_clue(g, 0, /*target=*/1, ClueKind::RANK, 3));

  ASSERT_NE(r.shape, ClueShape::OTHER)
      << "the reading fell through to OTHER, which is what a reactive looked "
         "like before the dispatch was shared";
  EXPECT_EQ(r.reacter_side.holder, 2) << "Cathy reacts";
  EXPECT_EQ(r.receiver_side.holder, 1) << "Bob receives, and he is the target";
}

// Both named cards play, so the shape is the one priority 1 is written over.
// The receiver's card is judged after the reacter's, which is what makes the
// delayed r3 count as a play rather than a strike.
TEST(TiiahClueReading, TheReverseReactiveIsAReactivePlay) {
  Game g = setup(reverse_opts());
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  auto r = read(g, make_clue(g, 0, /*target=*/1, ClueKind::RANK, 3));

  EXPECT_EQ(r.reacter_side.outcome, Outcome::PLAY) << "Cathy's r2";
  EXPECT_EQ(r.receiver_side.outcome, Outcome::PLAY) << "Bob's r3, after it";
  EXPECT_EQ(r.shape, ClueShape::REACTIVE_PLAY)
      << "got " << hanabi::reactor0::shape_name(r.shape);
}

// And the other direction: with the dispatch reversed, a clue to CATHY is the
// stable one, so it must not be read through a waiting connection.
TEST(TiiahClueReading, AClueToCathyIsReadStable) {
  Game g = setup(reverse_opts());
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  auto r = read(g, make_clue(g, 0, /*target=*/2, ClueKind::COLOUR, 0));

  EXPECT_NE(r.shape, ClueShape::REACTIVE_PLAY);
  EXPECT_NE(r.shape, ClueShape::REACTIVE_DISCARD);
  EXPECT_EQ(r.reacter_side.holder, -1)
      << "a stable reading designates no reacter at all";
}

// With no known play anywhere the dispatch does not reverse, so a clue to Bob
// is stable here too — the same clue, read differently because of what his hand
// holds rather than because of who was clued.
TEST(TiiahClueReading, WithoutAKnownPlayAClueToBobIsStable) {
  Game g = setup(reverse_opts());  // no `fully_known` this time

  auto r = read(g, make_clue(g, 0, /*target=*/1, ClueKind::RANK, 3));

  EXPECT_NE(r.shape, ClueShape::REACTIVE_PLAY);
  EXPECT_EQ(r.reacter_side.holder, -1);
}

// End to end: priority 1 is "a reactive clue where both named cards play", and
// that is exactly what this fixture offers. The bot has to find it through the
// shared rungs, which is the whole of what v16.6.0 turned on.
TEST(TiiahClueReading, TheDoublePlayReactiveIsWhatTheBotGives) {
  Game g = setup(reverse_opts());
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  PerformAction a;
  ASSERT_NO_THROW(a = g.take_action());
  ASSERT_TRUE(hanabi::is_clue(a))
      << "a clue that gets two plays outranks anything else here";
  const int target = std::visit([](const auto& v) { return v.target; }, a);
  EXPECT_EQ(target, 1) << "the reverse reactive is aimed at BOB";
}
