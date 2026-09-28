// The self-play detectors (v17_self_play_diagnostics/diagnostics.h) fire on a
// fault injected into one seat's view. Three turns of a seeded TIIAH game.

#include <gtest/gtest.h>

#include <algorithm>

#include "diagnostics.h"
#include "hanabi/basics/identity_set.h"
#include "sim.h"

using namespace hanabi::selfplay;

namespace {

SimConfig short_game() {
  SimConfig cfg;
  cfg.seed = 5;
  cfg.game_id = 9000005;
  cfg.log_dir = "";  // no logs
  cfg.max_turns = 3;
  return cfg;
}

}  // namespace

TEST(SelfPlayDiagnostics, AWrongInferenceIsReported) {
  Sim sim(short_game());
  Diagnostics diag(sim);
  SimHooks inner = diag.hooks();
  int victim = -1;
  SimHooks hooks;
  hooks.before_action = inner.before_action;
  hooks.after_action = [&](const Sim& s, const Outcome& o) {
    if (o.turn == 0) {
      // Seat 1 comes to believe seat 2's newest card is something it is not.
      victim = s.truth().hands[2].front();
      const hanabi::Identity truth = s.truth().deck[victim];
      const hanabi::Identity wrong{truth.suit_index, truth.rank == 5 ? 4 : truth.rank + 1};
      sim.seat_for_test(1).common.thoughts[victim].inferred =
          hanabi::IdentitySet::single(wrong);
    }
    inner.after_action(s, o);
  };
  const SimResult r = sim.run(hooks);
  diag.finish(sim, r);
  const auto& issues = diag.issues();
  EXPECT_TRUE(std::any_of(issues.begin(), issues.end(), [&](const Issue& i) {
    return i.cls == "1" && i.kind == "common_inferred" && i.seat == 1 && i.order == victim;
  }));
  // The turn limit is the harness's own stop, reported as such.
  ASSERT_TRUE(r.error.has_value());
  EXPECT_EQ(r.error->kind, "harness_error");
}

TEST(SelfPlayDiagnostics, ACleanShortGameReportsNoWrongInference) {
  Sim sim(short_game());
  Diagnostics diag(sim);
  const SimResult r = sim.run(diag.hooks());
  diag.finish(sim, r);
  for (const Issue& i : diag.issues()) {
    EXPECT_NE(i.cls, "1") << i.kind << " seat " << i.seat << " order " << i.order;
    EXPECT_NE(i.cls, "crash") << i.detail.dump();
  }
  EXPECT_EQ(r.turns, 3);
}
