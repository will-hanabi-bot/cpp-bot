// starting_required_efficiency and the allow_reactive_locks default
// (efficiency.cpp). Values hand-computed from the formula
// max_score / (8 + starting_pace + num_suits), Clue Starved halving the
// regain pool; cross-checked against hanab.live's variant table.
//
// The default is the formula PLUS four unconditional family vetoes (v13.5.0).
// Each veto case below pins both halves -- that the formula alone would have
// said ON, and that the veto overrides it -- because a veto that merely agreed
// with the formula would pass while testing nothing.
#include <gtest/gtest.h>

#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/efficiency.h"

using hanabi::get_variant;
using hanabi::reactor0::default_allow_reactive_locks;
using hanabi::reactor0::starting_required_efficiency;

TEST(Reactor0Efficiency, NoVariantThreePlayer) {
  const auto& v = get_variant("No Variant");
  // pace = 50 - 15 + 3 - 25 = 13; clues = 8 + 13 + 5 = 26.
  EXPECT_NEAR(starting_required_efficiency(v, 3), 25.0 / 26.0, 1e-9);
  EXPECT_TRUE(default_allow_reactive_locks(v, 3));
}

TEST(Reactor0Efficiency, NoVariantFivePlayer) {
  const auto& v = get_variant("No Variant");
  // pace = 50 - 20 + 5 - 25 = 10; clues = 8 + 10 + 5 = 23.
  EXPECT_NEAR(starting_required_efficiency(v, 5), 25.0 / 23.0, 1e-9);
  EXPECT_TRUE(default_allow_reactive_locks(v, 5));
}

TEST(Reactor0Efficiency, ClueStarvedIsHard) {
  const auto& v = get_variant("Clue Starved (5 Suits)");
  // regains = 13 + 5 = 18, halved = 9; clues = 17.
  EXPECT_NEAR(starting_required_efficiency(v, 3), 25.0 / 17.0, 1e-9);
  EXPECT_FALSE(default_allow_reactive_locks(v, 3))
      << "1.47 > 1.42: reactive locks default off in Clue Starved";
}

TEST(Reactor0Efficiency, SixSuitsThreePlayer) {
  const auto& v = get_variant("6 Suits");
  // pace = 60 - 15 + 3 - 30 = 18; clues = 8 + 18 + 6 = 32.
  EXPECT_NEAR(starting_required_efficiency(v, 3), 30.0 / 32.0, 1e-9);
  EXPECT_TRUE(default_allow_reactive_locks(v, 3));
}

// --- the four family vetoes (v13.5.0) -------------------------------------

TEST(Reactor0Efficiency, ThreeSuitsVetoesLocks) {
  const auto& v = get_variant("Rainbow (3 Suits)");
  // pace = 30 - 15 + 3 - 15 = 3; clues = 8 + 3 + 3 = 14.
  EXPECT_NEAR(starting_required_efficiency(v, 3), 15.0 / 14.0, 1e-9);
  EXPECT_LE(starting_required_efficiency(v, 3), 1.42)
      << "guard: the formula alone would allow locks here";
  EXPECT_FALSE(default_allow_reactive_locks(v, 3))
      << "a lock commits five cards out of a fifteen-card score";
}

TEST(Reactor0Efficiency, OddsAndEvensVetoesLocks) {
  const auto& v = get_variant("Odds and Evens (5 Suits)");
  EXPECT_NEAR(starting_required_efficiency(v, 3), 25.0 / 26.0, 1e-9);
  EXPECT_LE(starting_required_efficiency(v, 3), 1.42)
      << "guard: the formula alone would allow locks here";
  EXPECT_FALSE(default_allow_reactive_locks(v, 3));
}

TEST(Reactor0Efficiency, AlternatingCluesVetoesLocks) {
  const auto& v = get_variant("Alternating Clues (5 Suits)");
  EXPECT_NEAR(starting_required_efficiency(v, 3), 25.0 / 26.0, 1e-9);
  EXPECT_LE(starting_required_efficiency(v, 3), 1.42)
      << "guard: the formula alone would allow locks here";
  EXPECT_FALSE(default_allow_reactive_locks(v, 3))
      << "no stable clues at all, so a lock costs the only channel there is";
}

TEST(Reactor0Efficiency, SynesthesiaVetoesLocks) {
  // The variant from the live report that prompted the change.
  const auto& v = get_variant("Synesthesia & Black (6 Suits)");
  // Black is one-of-each: 5 normal suits (50) + 5 = 55 cards.
  // pace = 55 - 15 + 3 - 30 = 13; clues = 8 + 13 + 6 = 27.
  EXPECT_NEAR(starting_required_efficiency(v, 3), 30.0 / 27.0, 1e-9);
  EXPECT_LE(starting_required_efficiency(v, 3), 1.42)
      << "guard: the formula alone would allow locks here";
  EXPECT_FALSE(default_allow_reactive_locks(v, 3));

  const auto& plain = get_variant("Synesthesia (5 Suits)");
  EXPECT_FALSE(default_allow_reactive_locks(plain, 3))
      << "the veto is the family, not the Black suit";
}

// The vetoes are exactly four families -- a 5-suit variant outside them keeps
// the formula. `NoVariantThreePlayer` above is the other half of this guard.
TEST(Reactor0Efficiency, AnOrdinaryFiveSuitVariantStillAllowsLocks) {
  EXPECT_TRUE(default_allow_reactive_locks(get_variant("Rainbow (5 Suits)"), 3));
  EXPECT_TRUE(default_allow_reactive_locks(get_variant("Black (5 Suits)"), 3));
}
