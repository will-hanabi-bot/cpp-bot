// Regression for replay 1888970, variant "Funnels & Black (5 Suits)".
// Orig players: P0=yagami_black, P1=will-bot69, P2=will-bot67.
//
// At T48 the live will-bot67 (v0.15) DISCARDED g5#28 — a critical 5 —
// and the team lost the game. Per the user, will-bot67 should have
// PLAYED slot 2 (= g4#39, inferred as r5/g4 from an earlier finesse).
// At cards_left=1, playing is the only path to a score of 25.
//
// Driving the replay through T47 and running take_action under the
// current code (post-v0.16), the bot now picks PerformPlay(g4) — the
// correct move. This test pins that behaviour so the regression is
// caught if a future change reverts it.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// Deck order matches hanab.live export verbatim (1888970.json).
const std::vector<std::pair<int, int>> kDeck = {
    {3, 2}, {3, 1}, {0, 2}, {3, 2}, {4, 1},
    {0, 1}, {0, 4}, {1, 2}, {1, 2}, {3, 5},
    {1, 1}, {4, 4}, {2, 1}, {1, 4}, {0, 2},
    {0, 3}, {3, 3}, {0, 1}, {2, 3}, {0, 1},
    {4, 2}, {3, 1}, {3, 4}, {3, 1}, {4, 5},
    {2, 3}, {2, 1}, {3, 4}, {2, 5}, {3, 3},
    {1, 1}, {1, 5}, {1, 1}, {2, 2}, {2, 2},
    {2, 1}, {1, 3}, {4, 3}, {0, 4}, {2, 4},
    {0, 3}, {1, 3}, {2, 4}, {0, 5}, {1, 4},
};

// Actions 0..46 = turns 1..47. Action 47 (T48 = the bug turn) NOT
// applied.
const std::vector<OrigAction> kOrigActions = {
    {3, 2, 3},   {0, 5, 0},   {2, 1, 3},   {0, 1, 0},   {3, 0, 3},
    {0, 14, 0},  {0, 4, 0},   {3, 0, 5},   {0, 10, 0},  {0, 0, 0},
    {2, 0, 4},   {0, 12, 0},  {0, 20, 0},  {1, 8, 0},   {3, 1, 5},
    {0, 16, 0},  {0, 15, 0},  {3, 1, 1},   {0, 22, 0},  {0, 7, 0},
    {1, 21, 0},  {2, 2, 2},   {0, 9, 0},   {1, 19, 0},  {1, 26, 0},
    {1, 23, 0},  {2, 1, 0},   {1, 2, 0},   {0, 6, 0},   {2, 0, 1},
    {0, 33, 0},  {1, 34, 0},  {3, 0, 2},   {3, 2, 5},   {0, 36, 0},
    {2, 1, 4},   {3, 2, 1},   {0, 37, 0},  {0, 13, 0},  {3, 2, 1},
    {0, 25, 0},  {0, 11, 0},  {0, 31, 0},  {2, 0, 2},   {3, 0, 4},
    {0, 24, 0},  {3, 0, 1},
};

// Build from will-bot67's perspective (orig P2 = ALICE = observer).
// Player cycle: orig P0 -> P1 -> P2 -> P0. ALICE = orig P2, BOB = orig
// P0 (next), CATHY = orig P1. Starting = orig P0 = MY BOB.
Game build_from_bot67_perspective() {
  SetupOptions opts;
  opts.hands = {
      // ALICE = bot67. orig 10..14 -> MY 0..4 newest-first:
      // 14=(0,2)=r2, 13=(1,4)=y4, 12=(2,1)=g1, 11=(4,4)=k4, 10=(1,1)=y1.
      {"xx", "xx", "xx", "xx", "xx"},
      // BOB = yagami. orig 0..4 -> MY 5..9 newest-first:
      // 4=(4,1)=k1, 3=(3,2)=b2, 2=(0,2)=r2, 1=(3,1)=b1, 0=(3,2)=b2.
      {"k1", "b2", "r2", "b1", "b2"},
      // CATHY = bot69. orig 5..9 -> MY 10..14 newest-first:
      // 9=(3,5)=b5, 8=(1,2)=y2, 7=(1,2)=y2, 6=(0,4)=r4, 5=(0,1)=r1.
      {"b5", "y2", "y2", "r4", "r1"},
  };
  opts.variant_name = "Funnels & Black (5 Suits)";
  opts.starting = TestPlayer::BOB;
  return setup(std::move(opts));
}

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // orig P0 -> BOB, orig P1 -> CATHY, orig P2 -> ALICE.
  ctx.orig_to_my_player = {1, 2, 0};
  ctx.orig_to_my_order.resize(kDeck.size());
  for (int o = 0; o <= 4; ++o) ctx.orig_to_my_order[o] = o + 5;
  for (int o = 5; o <= 9; ++o) ctx.orig_to_my_order[o] = o + 5;
  for (int o = 10; o <= 14; ++o) ctx.orig_to_my_order[o] = o - 10;
  for (int o = 15; o < static_cast<int>(kDeck.size()); ++o)
    ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(kDeck.size());
  for (size_t orig_o = 0; orig_o < kDeck.size(); ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

}  // namespace

// Diagnostic — observes the bot's choice at T48 and prints state.
TEST(EndgameReplay1888970, Turn48DoesNotDiscardCritical) {
  Game g = build_from_bot67_perspective();
  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  ASSERT_EQ(g.state.cards_left, 1);

  PerformAction perform = g.take_action();
  // Primary regression: must not discard g5#28 (the critical that
  // sealed the live game's loss at v0.15).
  if (std::holds_alternative<PerformDiscard>(perform)) {
    EXPECT_NE(std::get<PerformDiscard>(perform).target, 28)
        << "regression: discarding g5 at cards_left=1 loses the critical "
           "5 and drops max_score from 25 → 24.";
  }
  // Tighter pin: under current code the bot picks PerformPlay(g4#39).
  // Document this as the post-fix behaviour. If a future change moves
  // the choice to a different non-disastrous action, the test should
  // be re-tuned rather than relaxed.
  EXPECT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "expected PerformPlay (slot 2 = g4#39) at T48 — this is the "
         "only path to score 25 from this position";
  if (std::holds_alternative<PerformPlay>(perform)) {
    EXPECT_EQ(std::get<PerformPlay>(perform).target, 39);
  }
}
