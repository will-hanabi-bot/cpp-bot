// starting_required_efficiency and the allow_reactive_locks default
// (efficiency.cpp). Values hand-computed from the formula
// max_score / (8 + starting_pace + num_suits), Clue Starved halving the
// regain pool; cross-checked against hanab.live's variant table.
#include <gtest/gtest.h>

#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/efficiency.h"

using hanabi::get_variant;
using hanabi::reactor0::default_allow_reactive_locks;
using hanabi::reactor0::starting_required_efficiency;

TEST(Efficiency, NoVariantThreePlayer) {
  const auto& v = get_variant("No Variant");
  // pace = 50 - 15 + 3 - 25 = 13; clues = 8 + 13 + 5 = 26.
  EXPECT_NEAR(starting_required_efficiency(v, 3), 25.0 / 26.0, 1e-9);
  EXPECT_TRUE(default_allow_reactive_locks(v, 3));
}

TEST(Efficiency, NoVariantFivePlayer) {
  const auto& v = get_variant("No Variant");
  // pace = 50 - 20 + 5 - 25 = 10; clues = 8 + 10 + 5 = 23.
  EXPECT_NEAR(starting_required_efficiency(v, 5), 25.0 / 23.0, 1e-9);
  EXPECT_TRUE(default_allow_reactive_locks(v, 5));
}

TEST(Efficiency, ClueStarvedIsHard) {
  const auto& v = get_variant("Clue Starved (5 Suits)");
  // regains = 13 + 5 = 18, halved = 9; clues = 17.
  EXPECT_NEAR(starting_required_efficiency(v, 3), 25.0 / 17.0, 1e-9);
  EXPECT_FALSE(default_allow_reactive_locks(v, 3))
      << "1.47 > 1.42: reactive locks default off in Clue Starved";
}

TEST(Efficiency, SixSuitsThreePlayer) {
  const auto& v = get_variant("6 Suits");
  // pace = 60 - 15 + 3 - 30 = 18; clues = 8 + 18 + 6 = 32.
  EXPECT_NEAR(starting_required_efficiency(v, 3), 30.0 / 32.0, 1e-9);
  EXPECT_TRUE(default_allow_reactive_locks(v, 3));
}
