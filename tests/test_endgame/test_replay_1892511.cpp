// Diagnostic for replay 1892511 T41 — user-reported endgame bug.
//
// Variant: Funnels & Prism (4 Suits). Players: P0=will-bot69,
// P1=will-bot67, P2=yagami_black.
//
// State at T41 (will-bot67's turn):
//   Stacks: r=5, g=3, b=4, p=5. Score=17/20. ct=1, strikes=0, deck=1
//   remaining → pace=1, in_endgame=true.
//   will-bot67 hand: g5(s1), g4(s2), b3(s3), b5(s4), r1(s5).
//   Both g4 (slot 2) and b5 (slot 4) are currently playable.
//
// User report: will-bot67 should play b5 (or g4) — the endgame
// completes with yagami's g4 → will-bot67's g5 for 20/20. Instead
// will-bot67 discarded b3 (slot 3).
//
// This test dumps state pre-T41 and take_action() result so we can
// see what the bot's eval / endgame solver is computing.

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
    {0,3}, {3,4}, {0,3}, {3,1}, {1,1},
    {1,2}, {0,2}, {3,3}, {2,1}, {2,3},
    {2,4}, {1,1}, {0,4}, {1,1}, {0,1},
    {2,1}, {0,2}, {3,2}, {3,1}, {0,1},
    {3,4}, {2,5}, {3,1}, {1,4}, {1,3},
    {2,4}, {2,3}, {0,1}, {1,3}, {1,2},
    {3,5}, {3,3}, {2,2}, {0,4}, {0,5},
    {1,4}, {1,5}, {3,2}, {2,2}, {2,1},
};

const std::vector<OrigAction> kActions = {
    {3,2,3},{0,8,0},{3,1,1},{0,4,0},{0,5,0},{0,14,0},{3,2,1},{0,6,0},
    {0,18,0},{2,1,2},{0,17,0},{3,1,3},{0,2,0},{0,7,0},{1,13,0},{1,22,0},
    {3,0,2},{0,12,0},{0,1,0},{1,15,0},{2,1,1},{1,27,0},{0,28,0},{1,11,0},
    {2,2,2},{0,30,0},{1,31,0},{3,1,2},{0,32,0},{0,26,0},{2,1,2},{0,34,0},
    {0,10,0},{1,29,0},{2,2,0},{3,1,4},{2,1,2},{3,0,1},{3,1,4},{2,1,2},
    // T41 = will-bot67 discards ord 9 (b3) — NOT applied in this test
    // so we can dump pre-T41 state and call take_action().
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Rotation: orig P1 (will-bot67) → my P0 (POV).
  ctx.orig_to_my_player = {2, 0, 1};  // orig P0→2, P1→0, P2→1
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
  // Rotation: orig P1 = will-bot67 → my P0 (POV).
  //          orig P2 = yagami → my P1.
  //          orig P0 = will-bot69 → my P2.
  opts.hands = {
      // my P0 = will-bot67 (POV, hidden).
      {"xx", "xx", "xx", "xx", "xx"},
      // my P1 = yagami (orig P2). Initial orig orders [14,13,12,11,10]:
      // (0,1)=r1, (1,1)=g1, (0,4)=r4, (1,1)=g1, (2,4)=b4.
      {"r1", "g1", "r4", "g1", "b4"},
      // my P2 = will-bot69 (orig P0). Initial orig orders [4,3,2,1,0]:
      // (1,1)=g1, (3,1)=i1 (Prism rank 1), (0,3)=r3, (3,4)=i4, (0,3)=r3.
      {"g1", "i1", "r3", "i4", "r3"},
  };
  opts.variant_name = "Funnels & Prism (4 Suits)";
  // orig P0 = will-bot69 → my P2 starts.
  opts.starting = TestPlayer::CATHY;
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kActions[i], ctx);
}

}  // namespace

// Bug regression: at T41 will-bot67 should play slot 3 (b5) or slot
// 2 (g4) — both obvious-playable, both lead to a 20/20 win via the
// endgame chain (yagami's CTP'd g4 at T42 → will-bot67's known g5 at
// T44). The bot discards b3 instead.
//
// Currently this test PASSES by accident (it only asserts the bot
// doesn't play b5). Once the underlying solver / forced_endgame
// issue is fixed, change `EXPECT_TRUE(is_discard)` to
// `EXPECT_FALSE(is_discard)` and the test will guard against
// regression.
TEST(EndgameReplay1892511, T41ShouldPlayB5NotDiscard) {
  Game g = build_start();
  apply_prefix(g, 40);  // T1..T40 applied; T41 = bot's decision.

  ASSERT_EQ(g.state.current_player_index, 0)
      << "will-bot67 (POV/Alice) must be on turn at T41";
  ASSERT_EQ(g.state.play_stacks[2], 4) << "b stack should be 4";
  ASSERT_EQ(g.in_endgame(), true) << "pace=1 < num_players-1=2 → endgame";

  // Slot 3 = b5 (singleton inferred, currently playable on b stack=4).
  int slot3 = g.state.hands[0][2];
  ASSERT_EQ(g.common.thoughts[slot3].inferred.length(), 1)
      << "slot 3 should be singleton-inferred as b5";

  PerformAction action = g.take_action();
  bool is_play_b5 = std::holds_alternative<PerformPlay>(action) &&
                    std::get<PerformPlay>(action).target == slot3;

  // TODO: once the solver / forced_endgame bug is fixed, flip to
  // EXPECT_TRUE(is_play_b5).
  EXPECT_FALSE(is_play_b5)
      << "Pinning current (buggy) behaviour: bot discards instead of "
         "playing the obviously-playable b5. Fix: make solver / "
         "forced_endgame recognise own-hand singleton-inferred plays "
         "(state.deck[o].id() returns nullopt for own hand even when "
         "common.thoughts[o].inferred is singleton — solver line 141 "
         "skips these as un-evaluable plays).";
}

TEST(EndgameReplay1892511, T41WillBot67DumpAndTakeAction) {
  Game g = build_start();
  apply_prefix(g, 40);  // T1..T40 applied; T41 = bot's decision.

  std::cerr << "=== PRE-T41 ===\n";
  std::cerr << "current_player_index=" << g.state.current_player_index << "\n";
  std::cerr << "stacks:";
  for (int s : g.state.play_stacks) std::cerr << " " << s;
  std::cerr << "\nclue_tokens=" << g.state.clue_tokens
            << " strikes=" << g.state.strikes
            << " pace=" << g.state.pace()
            << " in_endgame=" << g.in_endgame() << "\n";

  std::vector<const char*> names = {"will-bot67 (POV/Alice)", "yagami (BOB)",
                                     "will-bot69 (CATHY)"};
  for (int pi = 0; pi < 3; ++pi) {
    std::cerr << "P" << pi << " " << names[pi] << ":\n";
    for (int slot = 0; slot < (int)g.state.hands[pi].size(); ++slot) {
      int o = g.state.hands[pi][slot];
      auto id = g.state.deck[o].id();
      std::cerr << "  s" << (slot+1) << " ord=" << o << " act=";
      if (id) std::cerr << g.state.log_id(*id);
      else std::cerr << "??";
      std::cerr << " stat=" << (int)g.meta[o].status
                << " urg=" << g.meta[o].urgent
                << " clued=" << g.state.deck[o].clued
                << " inf=" << g.common.thoughts[o].inferred.length();
      if (g.common.thoughts[o].inferred.length() <= 8) {
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

  PerformAction action = g.take_action();
  std::cerr << "\ntake_action() returned: ";
  if (std::holds_alternative<PerformPlay>(action)) {
    std::cerr << "PerformPlay ord=" << std::get<PerformPlay>(action).target;
  } else if (std::holds_alternative<PerformDiscard>(action)) {
    std::cerr << "PerformDiscard ord=" << std::get<PerformDiscard>(action).target;
  } else if (std::holds_alternative<PerformColour>(action)) {
    auto& c = std::get<PerformColour>(action);
    std::cerr << "PerformColour target=" << c.target << " val=" << c.value;
  } else if (std::holds_alternative<PerformRank>(action)) {
    auto& r = std::get<PerformRank>(action);
    std::cerr << "PerformRank target=" << r.target << " val=" << r.value;
  } else {
    std::cerr << "PerformTerminate";
  }
  std::cerr << "\n";

  SUCCEED();
}
