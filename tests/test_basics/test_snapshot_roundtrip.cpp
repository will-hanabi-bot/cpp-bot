// Round-trip test for the per-game snapshot writer/reader.
//
// The crux of the iteration-tooling overhaul is `replay_log`: a logged STATE
// snapshot must be reconstructible into an exact Game so the user can re-run
// take_action() under current code. This test exercises the writer + reader
// pair on small synthetic games (via test_harness setup()) and asserts that
// the reconstructed Game has the same public state as the original.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/logging/state_snapshot.h"
#include "test_harness.h"

namespace {

using namespace hanabi;
using namespace hanabi::test;

// Compare the structural state of two Games: enough to confirm replay_log
// will produce identical take_action input. Doesn't compare every internal
// field — just the ones a logged bug-report-replay relies on.
void expect_same_state(const Game& a, const Game& b) {
  EXPECT_EQ(a.state.turn_count, b.state.turn_count);
  EXPECT_EQ(a.state.current_player_index, b.state.current_player_index);
  EXPECT_EQ(a.state.clue_tokens, b.state.clue_tokens);
  EXPECT_EQ(a.state.strikes, b.state.strikes);
  EXPECT_EQ(a.state.cards_left, b.state.cards_left);
  EXPECT_EQ(a.state.play_stacks, b.state.play_stacks);
  EXPECT_EQ(a.state.our_player_index, b.state.our_player_index);
  EXPECT_EQ(a.state.num_players, b.state.num_players);
  EXPECT_EQ(a.state.names, b.state.names);
  // Hands by player.
  ASSERT_EQ(a.state.hands.size(), b.state.hands.size());
  for (size_t p = 0; p < a.state.hands.size(); ++p) {
    EXPECT_EQ(a.state.hands[p], b.state.hands[p]) << "player " << p;
  }
  // Per-card empathy (common-perspective).
  ASSERT_EQ(a.common.thoughts.size(), b.common.thoughts.size());
  for (size_t o = 0; o < a.common.thoughts.size(); ++o) {
    EXPECT_EQ(a.common.thoughts[o].possible, b.common.thoughts[o].possible)
        << "order " << o;
    EXPECT_EQ(a.common.thoughts[o].inferred, b.common.thoughts[o].inferred)
        << "order " << o;
  }
  // Meta (status, focused, urgent).
  ASSERT_EQ(a.meta.size(), b.meta.size());
  for (size_t o = 0; o < a.meta.size(); ++o) {
    EXPECT_EQ(a.meta[o].status, b.meta[o].status) << "order " << o;
    EXPECT_EQ(a.meta[o].focused, b.meta[o].focused) << "order " << o;
    EXPECT_EQ(a.meta[o].urgent, b.meta[o].urgent) << "order " << o;
  }
  // Waiting connections.
  EXPECT_EQ(a.waiting.size(), b.waiting.size());
}

TEST(SnapshotRoundTrip, FreshDeal) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "g2", "b1", "y4", "p5"},
      {"y2", "r1", "g3", "b4", "p1"},
  };
  Game original = setup(std::move(opts));
  auto snap = hanabi::logging::build_state_snapshot(original, original.state.turn_count);
  Game reconstructed = hanabi::logging::apply_snapshot(snap);
  expect_same_state(original, reconstructed);
}

TEST(SnapshotRoundTrip, AfterClueAndPlay) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "g2", "b1", "y4", "p5"},
      {"y2", "r1", "g3", "b4", "p1"},
  };
  Game game = setup(std::move(opts));
  game = take_turn(game, "Alice clues 1 to Bob (slot 1,3)");
  game = take_turn(game, "Bob plays r1 (slot 1)", "y3");

  auto snap = hanabi::logging::build_state_snapshot(game, game.state.turn_count);
  Game reconstructed = hanabi::logging::apply_snapshot(snap);
  expect_same_state(game, reconstructed);
  // take_action must produce the same answer (no endgame, so MC isn't in play).
  EXPECT_EQ(game.take_action(), reconstructed.take_action());
}

TEST(SnapshotRoundTrip, JsonSurvivesStringRoundTrip) {
  // Confirm the JSONL line we'd write to disk round-trips through serialize
  // → parse → apply.
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r2", "g3", "b4", "y5", "p1"},
      {"y1", "r1", "g2", "b3", "p4"},
  };
  Game original = setup(std::move(opts));
  auto snap = hanabi::logging::build_state_snapshot(original, original.state.turn_count);
  std::string s = snap.dump();
  auto reparsed = nlohmann::json::parse(s);
  Game reconstructed = hanabi::logging::apply_snapshot(reparsed);
  expect_same_state(original, reconstructed);
}

}  // namespace
