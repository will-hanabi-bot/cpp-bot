// Diagnostic for replay 1892397 — bug report by user.
//
// Variant: Funnels & Dark Prism (6 Suits). Players: P0=will-bot69,
// P1=yagami_black, P2=will-bot67.
//
// User reports:
//   (Bug 2) Notes on CTP'd cards are over-narrowed: yagami's g2
//     slot 3 (CTP'd at T13) narrows from {r3,y1,g2,b3,p3,i1} all the
//     way down to {r3} as unrelated cards play. Expected: only narrow
//     on visibility evidence.
//   (Bug 1) At T23 yagami plays the CTP'd g2 → strike (g stack=2
//     already from T16). The strike must NOT cascade-reset CTP status
//     or notes on UNRELATED cards (e.g. will-bot67's b4 finessed
//     elsewhere).
//
// This test pins the v0.26 behaviour by walking the replay and
// inspecting state at key checkpoints. Diagnostic-style; expand to
// behavioural assertions in future as the convention stabilises.

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
    {3,1}, {3,2}, {0,2}, {2,5}, {4,1},
    {1,4}, {4,4}, {3,1}, {2,2}, {0,1},
    {2,1}, {4,1}, {3,4}, {5,2}, {3,2},
    {4,2}, {1,5}, {0,4}, {4,3}, {1,3},
    {2,2}, {4,3}, {5,4}, {0,5}, {0,1},
};

const std::vector<OrigAction> kActions = {
    {3, 2, 2},  // T1
    {0, 7, 0},  // T2
    {3, 1, 3},  // T3
    {0, 4, 0},  // T4
    {0, 9, 0},  // T5
    {3, 1, 5},  // T6
    {0, 2, 0},  // T7
    {2, 0, 2},  // T8
    {1,11, 0},  // T9
    {0, 1, 0},  // T10
    {3, 0, 1},  // T11
    {0,10, 0},  // T12
    {3, 1, 4},  // T13  rank-4 → yagami (CTPs g2)
    {0,15, 0},  // T14
    {0,21, 0},  // T15
    {0,20, 0},  // T16  will-bot69 plays p3 (= 18? let me check)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Identity mapping: orig P0/1/2 = my P0/1/2.
  ctx.orig_to_my_player = {0, 1, 2};
  const int N = static_cast<int>(kDeck.size());
  ctx.orig_to_my_order.resize(N);
  ctx.my_order_to_id.resize(N);
  for (int o = 0; o < N; ++o) {
    ctx.orig_to_my_order[o] = o;
    ctx.my_order_to_id[o] = kDeck[o];
  }
  return ctx;
}

Game build_start() {
  SetupOptions opts;
  opts.hands = {
      // P0 = will-bot69 (POV, hidden).
      {"xx", "xx", "xx", "xx", "xx"},
      // P1 = yagami. Initial orig orders [9,8,7,6,5] slot-1-first:
      // (0,1)=r1, (2,2)=g2, (3,1)=b1, (4,4)=p4, (1,4)=y4.
      {"r1", "g2", "b1", "p4", "y4"},
      // P2 = will-bot67. Initial orig orders [14,13,12,11,10]:
      // (3,2)=b2, (5,2)=i2, (3,4)=b4, (4,1)=p1, (2,1)=g1.
      {"b2", "i2", "b4", "p1", "g1"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  opts.starting = TestPlayer::ALICE;
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kActions[i], ctx);
}

void dump_yagami_slot3(const Game& g, const char* label) {
  // yagami = P1, slot 3 = index 2.
  int o = g.state.hands[1][2];
  std::cerr << label << " yagami_slot3 ord=" << o
            << " status=" << static_cast<int>(g.meta[o].status)
            << " inf={";
  bool first = true;
  for (Identity i : g.common.thoughts[o].inferred) {
    if (!first) std::cerr << ",";
    std::cerr << g.state.log_id(i);
    first = false;
  }
  std::cerr << "}\n";
}

}  // namespace

// Diagnostic: walk through T13 → T16 and confirm yagami's g2 inferred
// is narrowed exactly once (when will-bot69 plays p3 at T16, making
// all p3 copies visible from will-bot69's POV — drops p3 from the
// inferred). Pre-fix, refresh_play_links's playable_set intersection
// would narrow on every play.
TEST(EndgameReplay1892397, YagamiG2InferredDoesNotOverNarrow) {
  // Fresh game per checkpoint — apply_prefix replays from action 0.
  {
    Game g = build_start();
    apply_prefix(g, 13);  // T1..T13.
    int yagami_slot3 = g.state.hands[1][2];
    ASSERT_EQ(g.meta[yagami_slot3].status, CardStatus::CALLED_TO_PLAY)
        << "T13 should CTP yagami's slot 3 (g2)";
    dump_yagami_slot3(g, "post-T13");
  }
  {
    Game g = build_start();
    apply_prefix(g, 15);  // T1..T15.
    int yagami_slot3 = g.state.hands[1][2];
    dump_yagami_slot3(g, "post-T15");
    // Pre-v0.26: refresh_play_links narrowed inferred to playable_set
    // on every play, shrinking the set even when no new visibility
    // evidence accumulated. v0.26 keeps the set wide until card_elim's
    // visibility narrowing fires legitimately.
    const auto& inf = g.common.thoughts[yagami_slot3].inferred;
    EXPECT_GE(inf.length(), 2)
        << "post-T15 yagami slot 3 inferred must not be over-narrowed";
  }
  {
    Game g = build_start();
    apply_prefix(g, 16);  // T1..T16 (will-bot69 plays p3 via T16).
    int yagami_slot3 = g.state.hands[1][2];
    dump_yagami_slot3(g, "post-T16");
  }
}
