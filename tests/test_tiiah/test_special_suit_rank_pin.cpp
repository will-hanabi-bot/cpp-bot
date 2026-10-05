// A rank play clue on a new slot-1 card names the special suit's playable, for a
// white-ish special suit too (tiiah/CONVENTION.md §1f, v20.20.0, the user's
// ruling). Since v20.13.0 this held for a rainbowy suit (`test_rainbowy.cpp`). It
// now holds for any suit no colour touches but rank still does: White, Gray, Light
// Pink and Gray Pink. Null is white-ish but touched by no rank either, so it is
// left alone.
//
// The clue still has to be a rank stable play clue, so the special suit's next card
// must be of the clued rank.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Bob's slot 1 is `slot1`; nobody holds a known play, so a clue to Bob is stable
// (§1c's dispatch does not reverse).
SetupOptions opts_for(const std::string& variant, const std::string& slot1) {
  SetupOptions opts;
  opts.variant_name = variant;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {slot1, "y4", "g4", "b4", "r4"},
      {"y3", "g3", "b3", "r3", "y2"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

int bob_slot_reading_size(const Game& g, int slot) {
  return g.common.thoughts[order_at(g, TestPlayer::BOB, slot)].inferred.length();
}

}  // namespace

TEST(TiiahSpecialRankPin, WhiteOneOnANewSlotOneCard) {
  Game g = setup(opts_for("Throw It in a Hole & White (5 Suits)", "w1"));
  g = take_turn(std::move(g), "Alice clues 1 to Bob");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::BOB, /*slot=*/1, {"w1"});
}

TEST(TiiahSpecialRankPin, GrayOneOnANewSlotOneCard) {
  Game g = setup(opts_for("Throw It in a Hole & Gray (6 Suits)", "a1"));
  g = take_turn(std::move(g), "Alice clues 1 to Bob");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::BOB, /*slot=*/1, {"a1"});
}

TEST(TiiahSpecialRankPin, LightPinkOneOnANewSlotOneCard) {
  Game g = setup(opts_for("Throw It in a Hole & Light Pink (5 Suits)", "i1"));
  g = take_turn(std::move(g), "Alice clues 1 to Bob");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::BOB, /*slot=*/1, {"i1"});
}

TEST(TiiahSpecialRankPin, GrayPinkOneOnANewSlotOneCard) {
  Game g = setup(opts_for("Throw It in a Hole & Gray Pink (6 Suits)", "i1"));
  g = take_turn(std::move(g), "Alice clues 1 to Bob");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::BOB, /*slot=*/1, {"i1"});
}

// Controls. White is already on 1, so its next card is no 1: the 1 is a playable 1
// of no particular suit.
TEST(TiiahSpecialRankPin, NotPinnedWhenTheSpecialSuitsNextCardIsAnotherRank) {
  SetupOptions opts = opts_for("Throw It in a Hole & White (5 Suits)", "r1");
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 1};
  Game g = setup(std::move(opts));
  g = take_turn(std::move(g), "Alice clues 1 to Bob");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_GT(bob_slot_reading_size(g, 1), 1);
}

// ...the 1 calls Bob's slot 2, not his slot 1.
TEST(TiiahSpecialRankPin, NotPinnedOffSlotOne) {
  SetupOptions opts = opts_for("Throw It in a Hole & White (5 Suits)", "y4");
  opts.hands[1] = {"y4", "w1", "g4", "b4", "r4"};
  Game g = setup(std::move(opts));
  g = take_turn(std::move(g), "Alice clues 1 to Bob");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY);
  EXPECT_GT(bob_slot_reading_size(g, 2), 1);
}

// ...and Null, which no rank touches, is not a special suit for this rule.
TEST(TiiahSpecialRankPin, NullIsNotPinned) {
  Game g = setup(opts_for("Throw It in a Hole & Null (5 Suits)", "r1"));
  g = take_turn(std::move(g), "Alice clues 1 to Bob");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_GT(bob_slot_reading_size(g, 1), 1);
}
