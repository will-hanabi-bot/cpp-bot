// Giver-side safety: reactor0 clues whose interpretation is a MISTAKE
// (e.g. a blind react slot that is visibly unplayable) must not survive
// find_all_clues — the same mechanism reactor uses (MISTAKE results are
// dropped at decide.cpp's find_all_clues).
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

TEST(Reactor0GiverFilters, VisiblyUnplayableBlindSlotIsNeverClued) {
  SetupOptions opts;
  opts.play_stacks = {{1, 0, 0, 0, 0}};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},   // Alice (giver, us)
      {"y3", "y2", "b3", "g4", "y4"},   // Bob: slot 1 y3 NOT playable
      {"y4", "r1", "b4", "g4", "p4"},   // Cathy: r1 trash at slot 2
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // A green clue to Cathy would be mode-2 blind play on Bob's slot 1
  // (react_slot = (3+5-2)%5 = 1), which is visibly y3 — unplayable. The
  // giver must not offer it.
  auto clues = g.find_all_clues(static_cast<int>(TestPlayer::ALICE));
  const Variant& v = *g.state.variant;
  int green_value = -1;
  for (size_t i = 0; i < v.clue_colour_names.size(); ++i) {
    if (v.clue_colour_names[i] == "Green") green_value = static_cast<int>(i);
  }
  ASSERT_GE(green_value, 0);
  bool found_green_to_cathy = false;
  for (const auto& perform : clues) {
    if (auto* pc = std::get_if<PerformColour>(&perform)) {
      if (pc->target == static_cast<int>(TestPlayer::CATHY) &&
          pc->value == green_value) {
        found_green_to_cathy = true;
      }
    }
  }
  EXPECT_FALSE(found_green_to_cathy)
      << "a MISTAKE-reading clue must be filtered by find_all_clues";
}
