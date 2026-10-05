// A re-touch in a Muddy suit (tiiah/CONVENTION.md §1f, the user's ruling, pinned in
// v20.24.0). A colour clue that touches only cards already clued calls the leftmost as
// the colour's own next card when it can be that, and otherwise as the muddy suit's
// next playable. No rank touches a Muddy or Cocoa Rainbow card, so this is how one is
// called after the colour that first touched it.
//
// No code was needed: reactor0's ladder already reads a re-touch this way. The colour
// pin names the colour's own next card when the card can be it, and after a second
// colour only the muddy suit fits the card. An explicit pin was tried and dropped: on
// the shared stacks it read two own-suit cards as muddy in 300 self-play games.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

Game muddy(std::vector<std::string> bob, std::vector<int> stacks) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole & Muddy Rainbow (6 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      std::move(bob),
      {"y3", "g3", "b3", "r3", "y2"},
  };
  opts.play_stacks = std::move(stacks);
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return setup(std::move(opts));
}

}  // namespace

// Bob's slot-2 m2 was clued Red earlier; Blue touches only it. It cannot be blue's
// next card, so it is the muddy suit's: `{m2}`.
TEST(TiiahMuddyRetouch, ARetouchCallsTheMuddyNextCard) {
  Game g = muddy({"y3", "m2", "g4", "y4", "r4"}, {0, 0, 0, 0, 0, 1});
  g = pre_clue(std::move(g), TestPlayer::BOB, 2, {"red"});
  g = take_turn(std::move(g), "Alice clues blue to Bob");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::BOB, /*slot=*/2, {"m2"});
}

// Control: the colour's own next card wins when the re-touched card can be it.
// Bob's b1 was clued Blue earlier; Blue again touches only it: `{b1}`.
TEST(TiiahMuddyRetouch, TheOwnSuitWinsWhenTheCardCanBeIt) {
  Game g = muddy({"y3", "b1", "g4", "y4", "r4"}, {0, 0, 0, 0, 0, 1});
  g = pre_clue(std::move(g), TestPlayer::BOB, 2, {"blue"});
  g = take_turn(std::move(g), "Alice clues blue to Bob");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::BOB, /*slot=*/2, {"b1"});
}
