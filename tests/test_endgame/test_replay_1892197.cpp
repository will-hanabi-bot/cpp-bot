// Diagnostic for replay 1892197 T9 — bug report by user.
//
// Variant: Funnels & Dark Prism (6 Suits). Players: P0=yagami_black,
// P1=will-bot67, P2=will-bot69. T9 = P2 (will-bot69) rank-4 to P0
// (yagami).
//
// User's diagnosis: at T9, yagami has p2 CTP'd (from T5 colour-purple
// clue) — still pending to play. The convention should simulate all
// receiver pending plays before deciding play_targets. After p2 plays,
// p stack = 2 → p3 becomes playable. The "leftmost playable" in
// yagami's hand under the POST-PENDING-PLAYS state is p3 at slot 2
// (not g1 at slot 4 which is currently playable but not the intended
// target). calc_slot(focus, target=2, hs=5) → react_slot=2 = will-
// bot67's slot 2 = i2.
//
// will-bot69 (giver) intended r1 (will-bot67 slot 5) → g1 (yagami
// slot 4), but the convention's actual interp uses post-pending-plays
// hypo and picks p3 → i2.
//
// This test dumps post-T9 state for diagnosis.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

const std::vector<std::pair<int, int>> kDeck = {
    {4,1}, {3,2}, {2,1}, {4,2}, {4,3},
    {3,1}, {0,1}, {3,2}, {0,4}, {0,3},
    {2,2}, {1,2}, {1,2}, {2,2}, {3,3},
    {5,2}, {5,5}, {3,4}, {1,4},
};

const std::vector<OrigAction> kActions = {
    {2, 2, 2},  // T1 P0 colour-g → P2
    {0, 5, 0},  // T2 P1 plays order 5 (b1)
    {3, 1, 4},  // T3 P2 rank-4 → P1
    {0, 0, 0},  // T4 P0 plays order 0 (p1)
    {2, 0, 4},  // T5 P1 colour-p → P0  (sets up p2/p3/i5 CTP chain)
    {1,11, 0},  // T6 P2 discards order 11 (y2)
    {2, 2, 1},  // T7 P0 colour-y → P2
    {0, 7, 0},  // T8 P1 plays order 7 (b2)
    {3, 0, 4},  // T9 P2 rank-4 → P0 (the suspect)
};

// Rotation: our POV = P2 (will-bot69, the giver of T9). orig P2 → my P0.
ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  ctx.orig_to_my_player = {1, 2, 0};  // orig P0→1, P1→2, P2→0.
  const int N = static_cast<int>(kDeck.size());
  ctx.orig_to_my_order.resize(N);
  ctx.my_order_to_id.resize(N);
  for (int orig_p = 0; orig_p < 3; ++orig_p) {
    int my_p = ctx.orig_to_my_player[orig_p];
    for (int i = 0; i < 5; ++i) {
      ctx.orig_to_my_order[orig_p * 5 + i] = my_p * 5 + i;
    }
  }
  for (int o = 15; o < N; ++o) ctx.orig_to_my_order[o] = o;
  for (int orig_o = 0; orig_o < N; ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

Game build_start() {
  SetupOptions opts;
  opts.hands = {
      // my P0 = will-bot69 (POV, hidden).
      {"xx", "xx", "xx", "xx", "xx"},
      // my P1 = yagami (orig P0). Initial slot-1-first orig=[4,3,2,1,0]:
      // (4,3)=p3, (4,2)=p2, (2,1)=g1, (3,2)=b2, (4,1)=p1.
      {"p3", "p2", "g1", "b2", "p1"},
      // my P2 = will-bot67 (orig P1). Initial orig=[9,8,7,6,5]:
      // (0,3)=r3, (0,4)=r4, (3,2)=b2, (0,1)=r1, (3,1)=b1.
      {"r3", "r4", "b2", "r1", "b1"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  // orig P0 (yagami) → my P1 (BOB). So starting = BOB.
  opts.starting = TestPlayer::BOB;
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kActions[i], ctx);
}

}  // namespace

// Regression: T9 giver-eval rejects the rank-4 → yagami clue.
//
// Under v0.24's POV-invariant has_consistent_infs (for the reactive
// path), the convention picks target_slot=2 (p3, becomes playable
// once yagami's pending p2 plays) with react_slot=2 (= will-bot67's
// slot 2 = i2 actual, NOT playable on i stack=0). The reacter would
// misplay. The giver's eval simulation should anticipate this strike
// and rate the clue low → pick a different action.
TEST(EndgameReplay1892197, T9GiverEvalRejectsBadClue) {
  Game g = build_start();
  apply_prefix(g, 8);  // T1..T8 — STOP before the suspect T9 clue.

  ASSERT_EQ(g.state.current_player_index, 0)
      << "POV bot will-bot69 must be on turn at T9";

  PerformAction action = g.take_action();
  // The suspect clue is rank-4 to yagami (= my P1 in the rotated test).
  bool is_the_bad_clue = std::holds_alternative<PerformRank>(action) &&
                          std::get<PerformRank>(action).target ==
                              static_cast<int>(TestPlayer::BOB) &&
                          std::get<PerformRank>(action).value == 4;
  EXPECT_FALSE(is_the_bad_clue)
      << "will-bot69's eval at T9 must reject the rank-4 → yagami clue "
         "— v0.24's POV-invariant convention now picks slot 2 (p3) as "
         "receiver target, mapping to will-bot67's slot 2 = i2 (not "
         "playable). advance() should catch this strike.";
}

TEST(EndgameReplay1892197, DumpPreT9) {
  Game g = build_start();
  apply_prefix(g, 8);
  std::cerr << "=== PRE-T9 ===\n";
  std::vector<const char*> names = {"will-bot69 (POV)", "yagami (BOB)",
                                     "will-bot67 (CATHY)"};
  for (int pi = 0; pi < 3; ++pi) {
    std::cerr << "P" << pi << " " << names[pi] << ":\n";
    for (int slot = 0; slot < (int)g.state.hands[pi].size(); ++slot) {
      int o = g.state.hands[pi][slot];
      auto id = g.state.deck[o].id();
      std::cerr << "  s" << (slot+1) << " ord=" << o << " act=";
      if (id) std::cerr << g.state.log_id(*id); else std::cerr << "??";
      std::cerr << " stat=" << (int)g.meta[o].status
                << " urg=" << g.meta[o].urgent
                << " clued=" << g.state.deck[o].clued
                << " inf=" << g.common.thoughts[o].inferred.length();
      if (g.common.thoughts[o].inferred.length() <= 10) {
        std::cerr << "{";
        bool first = true;
        for (Identity i : g.common.thoughts[o].inferred) {
          if (!first) std::cerr << ",";
          std::cerr << g.state.log_id(i);
          first = false;
        }
        std::cerr << "}";
      }
      std::cerr << "\n";
    }
  }
  std::cerr << "stacks:";
  for (int s : g.state.play_stacks) std::cerr << " " << s;
  std::cerr << "\ncurrent_player_index=" << g.state.current_player_index << "\n";
  SUCCEED();
}

TEST(EndgameReplay1892197, DumpPostT9) {
  Game g = build_start();
  apply_prefix(g, 9);

  std::cerr << "=== POST-T9 ===\n";
  std::vector<const char*> names = {"will-bot69 (POV)", "yagami (BOB)",
                                     "will-bot67 (CATHY)"};
  for (int pi = 0; pi < 3; ++pi) {
    std::cerr << "P" << pi << " " << names[pi] << ":\n";
    for (int slot = 0; slot < (int)g.state.hands[pi].size(); ++slot) {
      int o = g.state.hands[pi][slot];
      auto id = g.state.deck[o].id();
      std::cerr << "  s" << (slot+1) << " ord=" << o << " act=";
      if (id) std::cerr << g.state.log_id(*id); else std::cerr << "??";
      std::cerr << " stat=" << (int)g.meta[o].status
                << " urg=" << g.meta[o].urgent
                << " clued=" << g.state.deck[o].clued
                << " inf=" << g.common.thoughts[o].inferred.length();
      if (g.common.thoughts[o].inferred.length() <= 10) {
        std::cerr << "{";
        bool first = true;
        for (Identity i : g.common.thoughts[o].inferred) {
          if (!first) std::cerr << ",";
          std::cerr << g.state.log_id(i);
          first = false;
        }
        std::cerr << "}";
      }
      std::cerr << "\n";
    }
  }
  std::cerr << "stacks:";
  for (int s : g.state.play_stacks) std::cerr << " " << s;
  std::cerr << "\n";
  for (size_t i = 0; i < g.move_history.size(); ++i) {
    if (std::holds_alternative<ClueInterp>(g.move_history[i])) {
      std::cerr << "move[" << i << "] ClueInterp="
                << (int)std::get<ClueInterp>(g.move_history[i]) << "\n";
    }
  }
  SUCCEED();
}
