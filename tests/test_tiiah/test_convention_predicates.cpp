// The two convention predicates, and the TIIAH name round-trip.
//
// Nearly every shared fork in the engine used to be a NEGATIVE test against
// reactor0 (`convention != Convention::REACTOR0`), which silently drops a third
// convention onto REACTOR's side. v16.0.0 rewrote all of them as positive
// predicates, and this file is what makes that rewrite safe to trust: for the
// two pre-existing conventions each predicate is exactly the test it replaced,
// so the rewrite is an identity transform on every existing game, replay and
// fixture.
#include <gtest/gtest.h>

#include <optional>

#include <nlohmann/json.hpp>

#include "hanabi/basics/convention.h"
#include "hanabi/basics/game.h"
#include "hanabi/logging/state_snapshot.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

TEST(TiiahConventionPredicates, ReactorZeroFamily) {
  // The belief machinery: the no-widening clamp, the missed-call policy, the
  // no-reset-on-strike rule, call invariants, reaction resolution. TIIAH forks
  // from reactor0 and inherits all of it.
  EXPECT_FALSE(is_reactor0_family(Convention::REACTOR));
  EXPECT_TRUE(is_reactor0_family(Convention::REACTOR0));
  EXPECT_TRUE(is_reactor0_family(Convention::TIIAH));
}

TEST(TiiahConventionPredicates, ReactorZeroDecisions) {
  // True for TIIAH since v16.6.0. The layer no longer prices clues by
  // reactor0's MEANINGS: it asks `dispatch_is_reactive` which clue is
  // reactive and reads each side's seat off the reading, so the rungs are
  // shared rather than reactor0's alone.
  EXPECT_FALSE(uses_reactor0_decisions(Convention::REACTOR));
  EXPECT_TRUE(uses_reactor0_decisions(Convention::REACTOR0));
  EXPECT_TRUE(uses_reactor0_decisions(Convention::TIIAH));
}

TEST(TiiahConventionPredicates, TheRewriteIsAnIdentityOnExistingConventions) {
  for (Convention c : {Convention::REACTOR, Convention::REACTOR0}) {
    EXPECT_EQ(is_reactor0_family(c), c == Convention::REACTOR0);
    EXPECT_EQ(uses_reactor0_decisions(c), is_reactor0_family(c));
  }
}

// Without the `parse_convention` arm a TIIAH game would replay as REACTOR —
// silently, because the reader defaults rather than throwing, and reactor is
// what an unknown name becomes.
TEST(TiiahConventionPredicates, SnapshotsReplayUnderTiiah) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"r1", "g2", "b3", "y4", "p5"},
      {"r2", "g3", "b4", "y5", "p1"},
      {"r3", "g4", "b5", "y1", "p2"},
  };
  Game g = setup(std::move(opts));
  g.convention = Convention::TIIAH;

  nlohmann::json rec = logging::build_state_snapshot(g, /*turn=*/0);
  EXPECT_EQ(rec["replay"]["convention"], "tiiah");

  Game back = logging::apply_snapshot(rec);
  EXPECT_EQ(back.convention, Convention::TIIAH);
  EXPECT_TRUE(back.state.variant->throw_it_in_a_hole);
}

TEST(TiiahConventionPredicates, NameRoundTrip) {
  EXPECT_EQ(convention_name(Convention::TIIAH), "tiiah");
  EXPECT_EQ(parse_convention("tiiah"), Convention::TIIAH);
  EXPECT_EQ(parse_convention("TIIAH"), Convention::TIIAH);
  // Still not a name any foreign /setall grammar can hit by accident.
  EXPECT_EQ(parse_convention("hole"), std::nullopt);
  EXPECT_EQ(parse_convention("3"), std::nullopt);
}
