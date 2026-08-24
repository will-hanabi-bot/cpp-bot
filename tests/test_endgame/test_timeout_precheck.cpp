// A timed-out search is a lower bound, not a verdict — and the caller has to be
// able to tell.
//
// Every deadline check in the tree makes the position look WORSE, never better:
// `winnable_if` reports UNWINNABLE, `action_winrate` returns 0, `optimize_full`
// zero-fills the tail, and the two "timeout" results are swallowed by `continue`
// in their callers. So a truncated search still answers, its answer looks
// exactly like a completed one, and its winrate is a floor.
//
// Two consequences this file pins:
//   * `SolveResult::timed_out` distinguishes the two, which `decide.cpp`'s
//     endgame fork needs in order to run its pre-check;
//   * `possible_call_actions` is the weaker "could advance" notion that
//     pre-check's tier 2 uses -- the `exists` mirror of `certainly_advances`.
//
// This is also the only coverage anywhere of a solver timeout: every other
// solver test sets a generous budget and asserts success.
#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/endgame/helper.h"
#include "hanabi/endgame/solver.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using hanabi::endgame::EndgameSolver;
using hanabi::endgame::Fraction;
using hanabi::endgame::possible_call_actions;
using hanabi::endgame::SolveResult;

namespace {

// A real endgame with genuine uncertainty, so the search has work to do and
// cannot exit through the "trivial one play wins" shortcut -- that shortcut runs
// BEFORE the deadline exists and can never report a timeout. Two points are
// missing, so `score() + 1 != max_score()`.
SetupOptions searchable_endgame() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {4, 4, 5, 5, 5};
  opts.clue_tokens = 2;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r5", "y1", "g1", "b1", "p1"},
      {"y5", "g2", "b2", "p2", "y2"},
  };
  return opts;
}

}  // namespace

// --- the flag -------------------------------------------------------------

TEST(EndgameTimeout, ZeroBudgetReportsTimedOut) {
  Game g = setup(searchable_endgame());
  ASSERT_NE(g.state.score() + 1, g.state.max_score())
      << "guard: must not exit through the trivial-win shortcut, which runs "
         "before the deadline exists";

  EndgameSolver solver{/*mc=*/true, /*timeout=*/0.0};
  SolveResult r = solver.solve(g);
  EXPECT_TRUE(r.timed_out)
      << "a zero budget cannot have searched anything; the caller must be able "
         "to see that";
}

TEST(EndgameTimeout, AGenerousBudgetDoesNot) {
  Game g = setup(searchable_endgame());
  EndgameSolver solver{/*mc=*/true, /*timeout=*/30.0};
  SolveResult r = solver.solve(g);
  EXPECT_FALSE(r.timed_out) << "this position is small enough to finish";
}

// --- tier 2: a call that COULD advance ------------------------------------

namespace {

// Red on 4. Our slot 1 carries a CTP; the test sets its reading.
Game called_position(IdentitySet reading, CardStatus status,
                     int clue_tokens = 4) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {4, 0, 0, 0, 0};
  opts.clue_tokens = clue_tokens;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "g1", "b1", "p1", "y2"},
      {"y3", "g3", "b3", "p3", "g2"},
  };
  Game g = setup(std::move(opts));
  const int o = g.state.hands[0][0];
  g.with_thought(o, [&](const Thought& t) {
    Thought out = t;
    out.inferred = reading;
    return out;
  });
  g.with_meta(o, [status](ConvData& m) { m.status = status; });
  g.players[0] = g.common;
  return g;
}

}  // namespace

TEST(EndgameTimeout, ACallWithOnePlayableReadingCouldAdvance) {
  // {r5, r1}: r5 plays with red on 4, r1 is long gone. Not CERTAIN -- but it
  // might score, which is what tier 2 is for.
  Game g = called_position(IdentitySet{}.add(Identity{0, 5}).add(Identity{0, 1}),
                           CardStatus::CALLED_TO_PLAY);
  const int o = g.state.hands[0][0];

  EXPECT_FALSE(hanabi::endgame::certainly_advances(
      g, o, PerformAction{PerformPlay{o}}))
      << "guard: one reading is trash, so this is not a certain play";

  auto calls = possible_call_actions(g);
  ASSERT_EQ(calls.size(), 1u);
  EXPECT_EQ(std::get<PerformPlay>(calls.front()).target, o);
}

TEST(EndgameTimeout, ACallNoReadingOfWhichCanAdvanceIsSkipped) {
  // Every reading is trash: red is on 4, so r1 and r2 are both long gone.
  Game g = called_position(IdentitySet{}.add(Identity{0, 1}).add(Identity{0, 2}),
                           CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(possible_call_actions(g).empty())
      << "a call that cannot possibly score is not worth the turn";
}

TEST(EndgameTimeout, AnUncalledPlayableIsNotATierTwoCandidate) {
  // Same reading, no stamp. Tier 2 is about STANDING CALLS; an uncalled card
  // that merely might be playable is a guess, and tier 1 already covers the
  // ones that are certain.
  Game g = called_position(IdentitySet{}.add(Identity{0, 5}).add(Identity{0, 1}),
                           CardStatus::NONE);
  EXPECT_TRUE(possible_call_actions(g).empty());
}

TEST(EndgameTimeout, AChuckIsSkippedAtEightTokens) {
  // A CTD presses Discard, which is illegal at 8 tokens -- so it is not an
  // available action however good it looks.
  Game g = called_position(IdentitySet{}.add(Identity{0, 5}),
                           CardStatus::CALLED_TO_DISCARD, /*clue_tokens=*/8);
  EXPECT_TRUE(possible_call_actions(g).empty());

  Game ok = called_position(IdentitySet{}.add(Identity{0, 5}),
                            CardStatus::CALLED_TO_DISCARD, /*clue_tokens=*/4);
  // Red is a plain suit, so pressing Discard on it advances nothing either way.
  EXPECT_TRUE(possible_call_actions(ok).empty())
      << "Discard only advances a stack on an inverted suit";
}

namespace {

// Red on 4 and yellow on 4: two points missing, so the points half of the fork
// is open and the trivial-win shortcut cannot fire.
SetupOptions precheck_position() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {4, 4, 5, 5, 5};
  opts.clue_tokens = 2;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y5", "g1", "b1", "p1", "g2"},
      {"g3", "b3", "p3", "g4", "b4"},
  };
  return opts;
}

// Slot 3 carries a CTP reading {r5, r1}: r5 scores, r1 is long gone. Not
// certain, so tier 1 declines and tier 2 is the layer under test. Slot 3 rather
// than slot 1 so a positional fallback cannot pick it by accident.
int stamp_precheck_call(Game& g) {
  const int called = g.state.hands[0][2];
  g.with_thought(called, [](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet{}.add(Identity{0, 5}).add(Identity{0, 1});
    return out;
  });
  g.with_meta(called, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
  g.players[0] = g.common;
  return called;
}

}  // namespace

// --- the pace gate --------------------------------------------------------

// The fork has two halves and the solver needs BOTH.
//
// `rem_score() <= num_suits + 1` asks how many points are missing, not how
// close the deck is to empty, so on a 6-suit variant it opens around the
// halfway mark and stays open. Across the log corpus 303 turns sat inside the
// points half with `pace() > num_players`, each one paying for a full search of
// a deck with 8-16 cards still in it.
//
// The threshold is `num_players` rather than a literal 3 because pace already
// carries the seat count: `pace() == cards_left + num_players - rem_score()`,
// so a fixed 3 would mean `rem_score() >= num_players - 2` at one card left --
// free at 3 seats, but at 5 it would exclude exactly the near-max endgames the
// solver is best at. At 3 seats the two forms are the same.
//
// This is the same fixture as the pre-check test below, minus its `cards_left`
// correction: the harness derives `cards_left` as `total - score - discarded`
// and never subtracts the dealt cards, which leaves this position at pace 28.
TEST(EndgameTimeout, ThePaceGateKeepsTheSolverOutOfMidGame) {
  Game g = setup(precheck_position());
  const int called = stamp_precheck_call(g);

  ASSERT_LE(g.state.rem_score(),
            static_cast<int>(g.state.variant->suits.size()) + 1)
      << "guard: the points half of the fork is open";
  ASSERT_GT(g.state.pace(), g.state.num_players)
      << "guard: but the pace half is not, which is the whole test";

  g.endgame_timeout = 0.0;  // if the solver ran, tier 2 would take the call
  hanabi::PerformAction action = g.take_action();

  const bool took_the_call = std::holds_alternative<PerformPlay>(action) &&
                             std::get<PerformPlay>(action).target == called;
  EXPECT_FALSE(took_the_call)
      << "the solver and its timeout pre-check must not run this far from the "
         "end of the deck; the ordinary ladder owns this turn";
}

// --- the pre-check, end to end --------------------------------------------

// With the solver given no time at all, the fork must action the standing call
// that could advance rather than whatever the truncated search preferred.
//
// `Game::endgame_timeout` exists for exactly this: waiting six seconds for a
// real timeout would make the test slow and its outcome machine-dependent.
TEST(EndgameTimeout, PreCheckActionsACallTheTruncatedSearchDidNotPick) {
  Game g = setup(precheck_position());
  // The harness derives `cards_left` as `total - score - discarded` and never
  // subtracts the 15 dealt cards, so a synthetic fixture lands ~15 too high --
  // which puts this position at pace 28. Two cards left is the real count here
  // (50 - 23 on stacks - 15 in hands - 10 discarded) and is what the fork's
  // pace half is about.
  g.with_state([](State& s) { s.cards_left = 2; });
  ASSERT_LE(g.state.rem_score(), static_cast<int>(g.state.variant->suits.size()) + 1)
      << "guard: the endgame fork is open here (points half)";
  ASSERT_LE(g.state.pace(), g.state.num_players)
      << "guard: and its pace half, or the solver never runs and the pre-check "
         "this test is about cannot fire";

  const int called = stamp_precheck_call(g);

  ASSERT_TRUE(hanabi::endgame::certain_plays(g).empty())
      << "guard: nothing here is a CERTAIN play, so tier 1 stands down";
  ASSERT_EQ(possible_call_actions(g).size(), 1u) << "guard: tier 2 has one";

  g.endgame_timeout = 0.0;  // the solver gets no time at all
  hanabi::PerformAction action = g.take_action();

  ASSERT_TRUE(std::holds_alternative<PerformPlay>(action))
      << "the standing call presses Play";
  EXPECT_EQ(std::get<PerformPlay>(action).target, called)
      << "a call that could advance beats a truncated search's preference";
}
