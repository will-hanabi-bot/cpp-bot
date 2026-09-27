// A clue that shows an ALREADY-clued card is trash is a trash reveal
// (src/conventions/reactor0/decision.cpp `read_stable`, DECISION_MAKING.md 3.3 /
// 4.2, v16.23.0) -- in an ordinary variant as in Throw It in a Hole.
//
// The priority list puts a trash reveal (3.3, 4.2) above a lock (3.6/3.7, 4.6),
// and the rungs are walked in that order. But the classifier only called a clue
// TRASH_REVEAL when it newly set `meta.trash`, which one rank branch of
// interpretation does (a rank clue touching only trash AND a new card). A colour
// clue narrowing a clued card down to trash reads FIX, and came out OTHER -- a
// shape no rung selects, so the lock won. Replay 2011327 T22 is the TIIAH case
// (tests/test_tiiah/test_decision_making/test_replay_2011327_trash_reveal_over_lock.cpp).
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

namespace {

Action make_clue(const Game& g, int giver, int target, ClueKind kind, int value) {
  auto touched = g.state.clue_touched(g.state.hands[target], kind, value);
  return Action{ClueAction{giver, target, std::move(touched), BaseClue{kind, value}}};
}

}  // namespace

TEST(Reactor0TrashReveal, RedOnAClueDTwoWithRedOnTwoIsATrashReveal) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g4", "b4", "r2", "y4", "p4"},
      {"y3", "b3", "g3", "p3", "g2"},
  };
  opts.play_stacks = {2, 2, 1, 2, 1};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 3;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"2"});
  const int r2 = order_at(g, TestPlayer::BOB, 3);
  ASSERT_GT(g.common.thoughts[r2].possibilities().length(), 1)
      << "guard: a clued 2 of unknown colour";

  Action red = make_clue(g, 0, 1, ClueKind::COLOUR, /*Red=*/0);
  ASSERT_EQ(std::get<ClueAction>(red).list_.size(), 1u) << "guard: Red touches only the r2";
  Game hypo = g.simulate(red);
  auto r = hanabi::reactor0::read_clue(g, hypo, std::get<ClueAction>(red));
  EXPECT_EQ(r.shape, ClueShape::TRASH_REVEAL)
      << "got " << hanabi::reactor0::shape_name(r.shape);
  EXPECT_EQ(r.stable_subject, r2);
}
