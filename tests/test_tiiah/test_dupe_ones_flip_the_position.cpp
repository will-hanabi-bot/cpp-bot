// A clue to the giver's Cathy is stable while the giver's Bob holds a SURE play, and
// an ordinary reactive once the hole could hold a dupe of it (tiiah/CONVENTION.md
// §1c, v20.3.0; confirmed for replay 2019249 T2 in v20.12.0, the user's ruling).
//
// Replay 2019249 T2: will-bot67's 1 had touched two of will-bot69's cards, and with
// nothing in the hole every 1 was a sure play. So yagami's 4 to will-bot67 (his
// Cathy) read as a stable discard call -- will-bot69 was loaded with a safe action.
// Had a 1 already gone into the hole, those 1s would no longer be safe plays, being
// possible dupes, and the same 4 would be a reactive.
#include <gtest/gtest.h>

#include <optional>
#include <string>

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

// Cathy holds two cards clued 1. With `one_in_the_hole`, we first throw an unknown
// card into the hole, which could be a 1; then Bob clues 4 to us, his Cathy.
Game four_to_us(bool one_in_the_hole) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g4", "b4", "y3", "p4", "r4"},
      {"g3", "b3", "p3", "r1", "y1"},
  };
  opts.clue_tokens = 6;
  opts.starting = one_in_the_hole ? TestPlayer::ALICE : TestPlayer::BOB;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::CATHY, 4, {"1"});
  g = pre_clue(std::move(g), TestPlayer::CATHY, 5, {"1"});
  if (one_in_the_hole) {
    g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/5, /*reached_the_hole=*/true);
  }
  return take_turn(std::move(g), "Bob clues 4 to Alice (slot 2)");
}

}  // namespace

TEST(TiiahDupeOnes, SureOnesMakeTheClueToCathyStable) {
  Game g = four_to_us(/*one_in_the_hole=*/false);
  EXPECT_NE(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_TRUE(g.waiting.empty()) << "no reaction is owed";
}

TEST(TiiahDupeOnes, AOneInTheHoleMakesItAReactive) {
  Game g = four_to_us(/*one_in_the_hole=*/true);
  EXPECT_EQ(interp_of(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().reacter, static_cast<int>(TestPlayer::CATHY));
  EXPECT_EQ(g.waiting.front().receiver, static_cast<int>(TestPlayer::ALICE));
}
