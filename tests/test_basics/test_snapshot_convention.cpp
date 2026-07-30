// Convention/rlocks fields must round-trip through the state snapshot, and
// — critically — snapshots written before the fields existed must replay
// under REACTOR: every historical replay test and old-log rerun depends on
// that default.
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "hanabi/basics/convention.h"
#include "hanabi/basics/game.h"
#include "hanabi/logging/state_snapshot.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using nlohmann::json;

namespace {

Game make_game() {
  SetupOptions opts;
  opts.hands = {
      {"r1", "g2", "b3", "y4", "p5"},
      {"r2", "g3", "b4", "y5", "p1"},
      {"r3", "g4", "b5", "y1", "p2"},
  };
  return setup(opts);
}

}  // namespace

TEST(SnapshotConvention, RoundTripsConventionAndRlocks) {
  Game g = make_game();
  g.convention = Convention::REACTOR0;
  g.allow_reactive_locks = false;

  json rec = logging::build_state_snapshot(g, /*turn=*/0);
  EXPECT_EQ(rec["replay"]["convention"], "reactor0");
  EXPECT_EQ(rec["replay"]["rlocks"], false);

  Game back = logging::apply_snapshot(rec);
  EXPECT_EQ(back.convention, Convention::REACTOR0);
  EXPECT_FALSE(back.allow_reactive_locks);
}

TEST(SnapshotConvention, MissingKeysDefaultToReactor) {
  Game g = make_game();
  json rec = logging::build_state_snapshot(g, /*turn=*/0);
  // Simulate a pre-v1.12.1 snapshot.
  rec["replay"].erase("convention");
  rec["replay"].erase("rlocks");

  Game back = logging::apply_snapshot(rec);
  EXPECT_EQ(back.convention, Convention::REACTOR)
      << "historical snapshots must replay under the convention they were "
         "played with";
  EXPECT_TRUE(back.allow_reactive_locks);
}

TEST(SnapshotConvention, ParseConventionNames) {
  EXPECT_EQ(parse_convention("reactor"), Convention::REACTOR);
  EXPECT_EQ(parse_convention("Reactor1"), Convention::REACTOR);
  EXPECT_EQ(parse_convention("reactor0"), Convention::REACTOR0);
  EXPECT_EQ(parse_convention("Reactor0"), Convention::REACTOR0);
  EXPECT_EQ(parse_convention("3"), std::nullopt)
      << "foreign /setall grammars (will-bot2's '/setall 3') must not parse";
  EXPECT_EQ(convention_name(Convention::REACTOR), "reactor");
  EXPECT_EQ(convention_name(Convention::REACTOR0), "reactor0");
}
