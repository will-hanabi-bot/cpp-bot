// v16.0.0 READS Throw It in a Hole but does not PLAY it.
//
// The convention's reactive half is unimplemented and the decision layer below
// is reactor0's, which prices clues by reactor0's meanings — so the bot refuses
// to act rather than act wrongly, which is what it did before this version.
#include <gtest/gtest.h>

#include <algorithm>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/convention.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Alice to move, nobody holding a known play, so the dispatcher's stable arm is
// the one under test.
SetupOptions stable_opts(const std::string& variant_name) {
  SetupOptions opts;
  opts.variant_name = variant_name;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y4", "g4", "b4", "p4"},
      {"y3", "g3", "b3", "p3", "r3"},
  };
  opts.starting = TestPlayer::ALICE;
  return opts;
}

std::optional<ClueInterp> interp_of(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* c = std::get_if<ClueInterp>(&g.move_history.back())) return *c;
  return std::nullopt;
}

}  // namespace

TEST(TiiahGate, TakeActionAnswers) {
  SetupOptions opts = stable_opts("Throw It in a Hole (5 Suits)");
  use_tiiah(opts);
  Game g = setup(std::move(opts));

  PerformAction a;
  ASSERT_NO_THROW(a = g.take_action())
      << "v16.6.0 lifted the refusal: the convention and the decision layer are "
         "both in place, so the engine has an answer";

  // Whatever it chose has to be a move this seat can legally make.
  const auto& hand = g.state.hands[g.state.our_player_index];
  const auto in_hand = [&](int order) {
    return std::find(hand.begin(), hand.end(), order) != hand.end();
  };
  if (auto* p = std::get_if<PerformPlay>(&a)) {
    EXPECT_TRUE(in_hand(p->target)) << "a play names a card in our own hand";
  } else if (auto* d = std::get_if<PerformDiscard>(&a)) {
    EXPECT_TRUE(in_hand(d->target)) << "a discard names a card in our own hand";
  } else {
    EXPECT_TRUE(hanabi::is_clue(a)) << "play, discard or clue — nothing else";
    const int target = std::visit([](const auto& v) { return v.target; }, a);
    EXPECT_NE(target, g.state.our_player_index) << "a clue names somebody else";
  }
}

// ...and the refusal is not over-broad: the same fixture under reactor0 still
// answers.
TEST(TiiahGate, OtherConventionsStillAct) {
  SetupOptions opts = stable_opts("No Variant");
  hanabi::test::reactor0::use_reactor0(opts);
  Game g = setup(std::move(opts));

  EXPECT_NO_THROW((void)g.take_action());
}

// A stable clue means under TIIAH exactly what it means under reactor0 — the
// user's spec says so, and the implementation delegates rather than forks. A
// DIFFERENTIAL test, so it cannot drift as reactor0's stable ladder evolves.
TEST(TiiahStableClues, ReadExactlyAsReactorZeroReadsThem) {
  auto play_clue = [](Convention convention) {
    SetupOptions opts = stable_opts("Throw It in a Hole (5 Suits)");
    opts.init = [convention](Game& g) { g.convention = convention; };
    Game g = setup(std::move(opts));
    return take_turn(std::move(g), "Alice clues 1 to Bob");
  };

  const Game r0 = play_clue(Convention::REACTOR0);
  const Game th = play_clue(Convention::TIIAH);

  ASSERT_TRUE(interp_of(r0).has_value());
  EXPECT_EQ(interp_of(th), interp_of(r0))
      << "the same clue must read the same way";
  for (size_t i = 0; i < r0.meta.size(); ++i) {
    EXPECT_EQ(th.meta[i].status, r0.meta[i].status) << "order " << i;
    EXPECT_EQ(th.meta[i].urgent, r0.meta[i].urgent) << "order " << i;
    EXPECT_EQ(th.common.thoughts[i].inferred, r0.common.thoughts[i].inferred)
        << "order " << i;
  }
}

// A reverse-reactive clue whose target walk finds nothing is not guessed at.
//
// Bob holds a known play, so the dispatch reverses (§1c) — but once his known
// r1 is assumed played his hand wants nothing that Cathy can bridge to, so
// there is no pairing to name. The clue reads as nothing and stamps nothing.
// An unread reactive does leave its waiting connection in place, with
// `react_order == -1`, exactly as reactor0's does.
TEST(TiiahStableClues, AReverseReactiveWithNoTargetStampsNothing) {
  SetupOptions opts = stable_opts("Throw It in a Hole (5 Suits)");
  // Bob: a known r1, and behind it nothing red or close to playable. Cathy
  // holds no bridge either.
  opts.hands[1] = {"r1", "y5", "g5", "b5", "p5"};
  opts.hands[2] = {"y4", "g4", "b4", "p4", "r4"};
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  const auto marks_before = hanabi::test::reactor0::hand_marks(g, TestPlayer::CATHY);
  g = take_turn(std::move(g), "Alice clues 5 to Bob");

  EXPECT_EQ(interp_of(g), ClueInterp::MISTAKE)
      << "no pairing to name, so no reading at all";
  EXPECT_EQ(hanabi::test::reactor0::hand_marks(g, TestPlayer::CATHY), marks_before)
      << "and nothing in the reacter's hand may be stamped on the strength of it";
  if (!g.waiting.empty()) {
    EXPECT_EQ(g.waiting.front().react_order, -1)
        << "an unread reactive names no reacter slot";
  }
}
