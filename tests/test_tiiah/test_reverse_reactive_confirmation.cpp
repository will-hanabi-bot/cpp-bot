// Reverse-reactive confirmation (tiiah/CONVENTION.md §1c, v18.11.0).
//
// The reviewer's rule: a reverse reactive stands only if its RECEIVER actually
// plays one of his standing plays -- the known play or standing call that put the
// table in the reverse position. If his next non-clue action is a play of some
// other card, or a discard, the clue was something else (a fix only he and the
// giver could see, say) and the reverse reactive is off.
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
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

// Alice clues, Bob receives, Cathy reacts. Bob's r1 is a known play, which is what
// makes a clue to him reactive; a 3 then calls Cathy's r2 (her slot 1) onto his r3.
Game reversed() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "r3", "y4", "g4", "b4"},
      {"r2", "y3", "g3", "b3", "p3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");
  g = take_turn(std::move(g), "Alice clues 3 to Bob");
  return g;
}

}  // namespace

// Bob plays the known r1 that made the position: the reverse reactive stands, and
// Cathy's r2 is still called.
TEST(TiiahReverseReactiveConfirmation, PlayingTheStandingPlayConfirmsIt) {
  Game g = reversed();
  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  const int cathys = order_at(g, TestPlayer::CATHY, 1);
  ASSERT_EQ(g.meta[cathys].status, CardStatus::CALLED_TO_PLAY);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1, /*reached_the_hole=*/true,
                    "p4");

  EXPECT_FALSE(g.waiting.empty()) << "the reverse reactive stands";
  EXPECT_EQ(g.meta[cathys].status, CardStatus::CALLED_TO_PLAY);
}

// Bob plays some other card -- his slot 2, never called: the reverse reactive is
// off, and Cathy's r2 is no longer called.
TEST(TiiahReverseReactiveConfirmation, PlayingAnUncalledCardWithdrawsIt) {
  Game g = reversed();
  const int cathys = order_at(g, TestPlayer::CATHY, 1);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/2, /*reached_the_hole=*/true,
                    "p4");

  EXPECT_TRUE(g.waiting.empty()) << "the reverse reactive is off";
  EXPECT_NE(g.meta[cathys].status, CardStatus::CALLED_TO_PLAY);
}

// Bob discards: off too.
TEST(TiiahReverseReactiveConfirmation, ADiscardWithdrawsIt) {
  Game g = reversed();
  const int cathys = order_at(g, TestPlayer::CATHY, 1);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/5, /*reached_the_hole=*/false,
                    "p4");

  EXPECT_TRUE(g.waiting.empty());
  EXPECT_NE(g.meta[cathys].status, CardStatus::CALLED_TO_PLAY);
}
