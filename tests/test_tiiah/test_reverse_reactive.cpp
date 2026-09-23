// The reverse reactive (tiiah/CONVENTION.md §1c).
//
// Reactor0 dispatches positionally — a clue to Cathy is reactive. TIIAH
// reverses it when Bob holds a known play and Cathy does not: a clue to BOB is
// reactive, with Cathy reacting and Bob receiving. Bob plays what he already
// knows, Cathy answers, and Bob's target is waiting when he comes round again.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/tiiah/interpret_reactive.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

std::optional<ClueInterp> interp_of(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* c = std::get_if<ClueInterp>(&g.move_history.back())) return *c;
  return std::nullopt;
}

// Alice clues, Bob receives, Cathy reacts. Bob's r1 is made a known play by the
// fixture, which is what turns a clue to him reactive.
SetupOptions reverse_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "r3", "y4", "g4", "b4"},   // Bob: known r1, then r3 behind it
      {"r2", "y3", "g3", "b3", "p3"},   // Cathy: the r2 that bridges them
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

}  // namespace

// The target walk: once Bob's known r1 is assumed played, red is on 1, so his
// r3 is not yet playable — but his own r1 is no longer waiting either. With
// Cathy holding the r2, the walk names Bob's r3 as what the clue is for.
TEST(TiiahReverseReactive, TheTargetIsFoundUnderStackSimulation) {
  Game g = setup(reverse_opts());
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  auto target = hanabi::tiiah::receiver_target(g, /*receiver=*/1);
  ASSERT_TRUE(target.has_value());
  EXPECT_EQ(*target, order_at(g, TestPlayer::BOB, 2))
      << "the r1 is assumed played, so the r3 is the next thing his hand wants";
}

// ...and a card already called to play is never the target: it is one of the
// plays the simulation just assumed. Reactor's rule, which reactor0 reversed
// and this convention restores.
TEST(TiiahReverseReactive, ACalledCardIsNeverRetargeted) {
  SetupOptions opts = reverse_opts();
  opts.hands[1] = {"r1", "r2", "y4", "g4", "b4"};  // r2 directly behind the r1
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  const int called = order_at(g, TestPlayer::BOB, 1);
  auto target = hanabi::tiiah::receiver_target(g, /*receiver=*/1);
  ASSERT_TRUE(target.has_value());
  EXPECT_NE(*target, called);
  EXPECT_EQ(*target, order_at(g, TestPlayer::BOB, 2))
      << "his r2 is playable once the r1 goes, and it is what the clue means";
}

// The dispatch itself: with a known play on Bob and none on Cathy, a clue to
// Bob reads REACTIVE and installs a connection with Cathy reacting.
TEST(TiiahReverseReactive, AClueToBobIsReactiveWithCathyReacting) {
  Game g = setup(reverse_opts());
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().reacter, 2) << "Cathy reacts";
  EXPECT_EQ(g.waiting.front().receiver, 1) << "Bob receives";
  EXPECT_TRUE(g.waiting.front().even_parity.value_or(false))
      << "every reactive in this convention is even parity";
}

// The sum rule picks Cathy's slot, exactly as in an ordinary reactive:
// react_slot + target_slot = anchor (mod hand size).
TEST(TiiahReverseReactive, TheSumRulePicksTheReactersSlot) {
  Game g = setup(reverse_opts());
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  // Rank 3 -> anchor 3. Bob's target is his slot 2, so Cathy's slot is
  // (3 + 5 - 2) % 5 = 1 -- her r2, which is what bridges the r1 to the r3.
  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().react_order, order_at(g, TestPlayer::CATHY, 1));
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(g.meta[order_at(g, TestPlayer::CATHY, 1)].urgent)
      << "a reaction is answered on the reacter's very next turn";
}

// The dispatch is a two-sided test. Cathy holding a known play too means the
// clue is nothing of the kind, and the ordinary stable reading applies.
TEST(TiiahReverseReactive, AKnownPlayOnCathyKeepsTheClueStable) {
  SetupOptions opts = reverse_opts();
  // A known play has to be PLAYABLE to count as one, so Cathy gets a y1 —
  // her r2 would not do it, with red still on 0.
  opts.hands[2] = {"r2", "y1", "g3", "b3", "p3"};
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");
  g = fully_known(std::move(g), TestPlayer::CATHY, /*slot=*/2, "y1");

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_NE(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_TRUE(g.waiting.empty())
      << "no reaction may be installed when the dispatch did not reverse";
}

// And with no known play anywhere, a clue to Bob is the plain stable clue it is
// under reactor0.
TEST(TiiahReverseReactive, WithoutAKnownPlayTheClueIsStable) {
  Game g = setup(reverse_opts());

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_NE(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_TRUE(g.waiting.empty());
}
