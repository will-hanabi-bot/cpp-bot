// Hanab.live replay 1892259 turn 45 — regression for the user-reported
// "wrong reactive clue targeting" bug. In the original game (run by an
// older bot version, pre-v0.24/v0.25) will-bot67 cluing rank-2 →
// will-bot69 was issued; the reactor's calc_slot rule mapped the
// resulting reacter slot to yagami's slot 1 = y1 (basic trash since
// y_stack=4). yagami subsequently played y1 and struck.
//
// Variant: Funnels & Dark Prism (6 Suits). Players: orig P0=yagami,
// P1=will-bot69, P2=will-bot67. POV = will-bot67 (the T45 giver) →
// my P0.

#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

const std::vector<std::pair<int, int>> kDeck = {
    {2,5}, {1,2}, {0,1}, {2,3}, {0,4}, {4,5}, {3,1}, {4,3}, {3,2}, {5,2},
    {1,5}, {5,3}, {0,1}, {2,1}, {3,4}, {3,2}, {2,4}, {4,4}, {3,5}, {4,2},
    {3,3}, {1,1}, {1,3}, {2,2}, {1,1}, {4,2}, {5,4}, {0,4}, {0,5}, {3,3},
    {3,4}, {4,3}, {1,3}, {3,1}, {5,1}, {3,1}, {2,2}, {1,4}, {0,3}, {2,1},
    {0,2}, {2,3}, {2,4}, {1,1}, {4,1}, {0,1}, {1,2}, {1,4}, {4,1}, {4,4},
    {0,3}, {0,2}, {2,1}, {4,1}, {5,5},
};

// T1..T44. We deliberately stop one short of T45 so we can probe
// `take_action` on the wb67 turn the user flagged.
const std::vector<OrigAction> kActions = {
    {3,2,1}, {0,6,0}, {3,1,4}, {0,2,0}, {0,15,0}, {0,13,0}, {1,16,0}, {3,2,5}, {3,0,2}, {1,4,0},
    {2,0,1}, {1,12,0}, {0,20,0}, {3,0,5}, {0,21,0}, {0,1,0}, {3,0,3}, {0,23,0}, {0,22,0}, {3,0,2},
    {0,14,0}, {0,3,0}, {2,0,0}, {0,18,0}, {1,24,0}, {1,17,0}, {1,29,0}, {1,19,0}, {1,31,0}, {2,1,1},
    {1,33,0}, {0,34,0}, {2,1,4}, {1,30,0}, {0,9,0}, {2,1,4}, {0,37,0}, {1,8,0}, {2,1,0}, {1,39,0},
    {0,40,0}, {2,1,0}, {1,41,0}, {0,42,0},
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // orig P0 (yagami) → my P1 (BOB). orig P1 (wb69) → my P2 (CATHY).
  // orig P2 (wb67, the T45 giver) → my P0 (POV).
  ctx.orig_to_my_player = {1, 2, 0};
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
      // my P0 = will-bot67 (POV, hidden).
      {"xx", "xx", "xx", "xx", "xx"},
      // my P1 = yagami_black (orig P0). orig[0..4] = g5, y2, r1, g3, r4.
      // slot-1-first (newest = orig 4 = r4): [r4, g3, r1, y2, g5].
      {"r4", "g3", "r1", "y2", "g5"},
      // my P2 = will-bot69 (orig P1). orig[5..9] = p5, b1, p3, b2, i2.
      // slot-1-first: [i2, b2, p3, b1, p5].
      {"i2", "b2", "p3", "b1", "p5"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  // orig P0 (yagami) starts → my P1 = BOB.
  opts.starting = TestPlayer::BOB;
  return setup(std::move(opts));
}

}  // namespace

// Regression: at T45 in this game state, the bot must not issue
// rank-2 → will-bot69. That clue's leftmost-playable target is p1
// (wb69 slot 1) which via `calc_slot(focus_slot=2, target_slot=1,
// hand_size=5)` maps the reacter slot to yagami's slot 1 = y1 — basic
// trash on a y_stack=4 board, so yagami would strike when executing
// the convention's play.
//
// The fix is upstream of this test: with v0.24 (advance hypo_state
// through good-touch self-plays) and v0.25 (POV-invariant
// consistency in reactive target_play), the bot's eval correctly
// prices this clue at -100 (`get_result` rejects it via the
// `hypo_plays.count(o) > 0` check on the would-strike reacter CTP).
// The user-visible result at this turn is a different clue (rank-1
// → wb69) that does not strike.
TEST(EndgameReplay1892259, T45DoesNotIssueStrikingRankClue) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < 44; ++i) apply_orig_action(g, kActions[i], ctx);
  ASSERT_EQ(g.state.current_player_index, 0)
      << "T45 should be will-bot67's turn (= my P0).";
  ASSERT_EQ(g.state.score(), 17);

  PerformAction perform = g.take_action();
  if (auto* r = std::get_if<PerformRank>(&perform)) {
    bool bad = r->target == static_cast<int>(TestPlayer::CATHY) &&
               r->value == 2;
    EXPECT_FALSE(bad)
        << "rank-2 → will-bot69 maps the reactive reacter slot to "
           "yagami's slot 1 = y1 (basic trash, y_stack=4). Issuing "
           "this clue makes yagami strike on the next turn. The "
           "reactor's eval pipeline must filter it out.";
  }
}
