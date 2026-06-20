// Regression test for the v0.37 known_plays tightening.
//
// Replay: https://hanab.live/shared-replay/1899567 — 3-player
// Gray Reversed (6 Suits) game between yagami_black (orig P0),
// will-bot69 (orig P1), and will-bot67 (orig P2).
//
// At T21 will-bot67 (giver) clues rank-4 to will-bot69. The clue
// touches wb69's slot 1 (g4) and slot 3 (b4); focus = slot 3 via
// `reactive_focus` (newest demoted, max touched order). r stack=4
// after T20's r4 play makes wb69's slot 2 (r5) currently playable;
// the human convention reads this as a "double play reactive" on
// r5 — receiver wb69 plays r5, reacter yagami plays slot 1 blindly
// (calc_slot(focus=3, target=2, 5) = 1).
//
// Pre-v0.37: the bot's `reactive_context.known_plays` included r5
// because the v0.19 stable interp at T19 narrowed wb69 slot 2 to a
// good-touch-inferred "must be r5" (`possible = {r1..r5}` after the
// red clue, with r1/r2/r3 basic-trash post-T20 → `obvious_playables`
// returned r5 from order_kp's loose "possible has a playable" check).
// With r5 in known_plays, the play-target loop filtered it out, the
// pool became empty, and the convention fell through to a b4 finesse
// (react=5 → yagami's b3 prereq → wb69 b4).
//
// Post-v0.37: `known_plays` requires a strict singleton
// `common.thoughts[o].id()`. wb69 slot 2 has `possible = {r1, r2, r3,
// r5}` (length 4) after T21's untouched-narrowing — `id()` returns
// nullopt — so r5 stays in `all_playable`, the play-target loop
// picks r5, and `target_play(yagami_slot_1)` succeeds (slot 1's
// effective_possible includes y1 ∈ playable_set; non-singleton
// inferred passes the chain-consistency guard).
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// Hanab.live deck for replay 1899567. Suit indices: 0=Red, 1=Yellow,
// 2=Green, 3=Blue, 4=Purple, 5=Gray Reversed.
const std::vector<std::pair<int, int>> kDeck = {
    {3,3}, {2,1}, {5,5}, {0,2}, {0,3},
    {5,1}, {3,2}, {3,5}, {2,4}, {3,4},
    {0,1}, {4,2}, {4,3}, {1,3}, {3,1},
    {2,2}, {4,3}, {1,3}, {4,4}, {0,3},
    {0,2}, {1,2}, {0,5}, {4,1}, {0,4},
    {2,2}, {2,4}, {5,3}, {1,3}, {2,1},
    {1,1}, {5,2}, {2,1}, {3,1}, {2,3},
    {4,4}, {0,4}, {3,2}, {4,1}, {3,1},
    {4,5}, {0,1}, {4,2}, {3,3}, {1,5},
    {1,4}, {1,4}, {4,1}, {2,5}, {3,4},
    {1,1}, {5,4}, {2,3}, {0,1}, {1,1},
};

// First 21 actions, T1..T21. T21 is the rank-4 clue being tested.
const std::vector<OrigAction> kActions = {
    {3,1,1},  // T1  yagami rank-1 → wb69
    {3,0,1},  // T2  wb69 rank-1 → yagami
    {0,14,0}, // T3  wb67 plays card 14 (b1)
    {0,2,0},  // T4  yagami plays card 2 (gr5)
    {3,0,1},  // T5  wb69 rank-1 → yagami
    {0,10,0}, // T6  wb67 plays card 10 (r1)
    {0,1,0},  // T7  yagami plays card 1 (g1)
    {3,0,4},  // T8  wb69 rank-4 → yagami
    {0,15,0}, // T9  wb67 plays card 15 (g2)
    {0,3,0},  // T10 yagami plays card 3 (r2)
    {2,2,1},  // T11 wb69 colour-yellow → wb67
    {3,1,5},  // T12 wb67 rank-5 → wb69
    {0,4,0},  // T13 yagami plays card 4 (r3)
    {0,6,0},  // T14 wb69 plays card 6 (b2)
    {0,19,0}, // T15 wb67 plays card 19 (r3) — STRIKE (r stack already 3)
    {2,2,4},  // T16 yagami colour-purple → wb67
    {1,8,0},  // T17 wb69 discards card 8 (g4)
    {0,23,0}, // T18 wb67 plays card 23 (p1)
    {2,1,0},  // T19 yagami colour-red → wb69 (clues both r4 and r5)
    {0,24,0}, // T20 wb69 plays card 24 (r4)
    {3,1,4},  // T21 wb67 rank-4 → wb69 (the suspect clue)
};

// Rotation: POV = will-bot67 (orig P2 → my P0). Sequence preservation
// puts orig P0 → my P1, orig P1 → my P2.
ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  ctx.orig_to_my_player = {1, 2, 0};  // orig P0→1, P1→2, P2→0.
  const int N = static_cast<int>(kDeck.size());
  ctx.orig_to_my_order.resize(N);
  ctx.my_order_to_id.resize(N);
  // Initial 15-card deal: orig slot-order N within player P maps to my
  // slot-order N within (orig_to_my_player[P]).
  for (int orig_p = 0; orig_p < 3; ++orig_p) {
    int my_p = ctx.orig_to_my_player[orig_p];
    for (int i = 0; i < 5; ++i) {
      ctx.orig_to_my_order[orig_p * 5 + i] = my_p * 5 + i;
    }
  }
  // Post-deal draws keep their orig order.
  for (int o = 15; o < N; ++o) ctx.orig_to_my_order[o] = o;
  for (int orig_o = 0; orig_o < N; ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

Game build_start() {
  SetupOptions opts;
  opts.hands = {
      // my P0 = wb67 (POV, hidden).
      {"xx", "xx", "xx", "xx", "xx"},
      // my P1 = yagami (orig P0). Orig orders 0-4 → my orders 5-9.
      // Initial slots (slot 1 = newest): r3, r2, gr5, g1, b3.
      // "Gray Reversed" is not in suits.json; pick_short falls through
      // 'g','r' (both already used by Green and Red) and lands on 'a'.
      {"r3", "r2", "a5", "g1", "b3"},
      // my P2 = wb69 (orig P1). Orig orders 5-9 → my orders 10-14.
      // Initial slots: b4, g4, b5, b2, gr1 → a1.
      {"b4", "g4", "b5", "b2", "a1"},
  };
  opts.variant_name = "Gray Reversed (6 Suits)";
  // orig P0 = yagami → my P1 starts.
  opts.starting = TestPlayer::BOB;
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kActions[i], ctx);
}

}  // namespace

// At T21 wb67's rank-4 clue to wb69 must be interpreted as a reactive
// double play on wb69's r5 (currently playable on r stack=4), NOT as a
// finesse on b4 via yagami's b3. The reactive interp stamps:
//   - reacter yagami's slot 1 (= my order 21 = y2 actual) as
//     CALLED_TO_PLAY (the convention asks yagami to blindly play
//     slot 1 in support of wb69's r5).
//   - receiver wb69's slot 2 (= my order 22 = r5) as CALLED_TO_PLAY
//     (the play target).
// Crucially, yagami's slot 5 (= my order 5 = b3) must NOT be CTP'd —
// that would be the rejected finesse interpretation.
TEST(EndgameReplay1899567, T21InterpretsAsR5ReactiveNotB4Finesse) {
  Game g = build_start();
  apply_prefix(g, 21);  // T1..T21 applied.

  // Sanity: confirm we placed the right orders at the slots we'll
  // assert on.
  int yagami_slot1 = g.state.hands[1][0];
  int yagami_slot5 = g.state.hands[1][4];
  int wb69_slot2 = g.state.hands[2][1];
  ASSERT_EQ(yagami_slot1, 21)
      << "test setup: yagami slot 1 must be the y2 drawn at T13 "
         "(my order 21)";
  ASSERT_EQ(yagami_slot5, 5)
      << "test setup: yagami slot 5 must be the original b3 (my order 5)";
  ASSERT_EQ(wb69_slot2, 22)
      << "test setup: wb69 slot 2 must be the r5 drawn at T17 "
         "(my order 22)";

  EXPECT_EQ(g.meta[yagami_slot1].status, CardStatus::CALLED_TO_PLAY)
      << "v0.37: the reactive interp must CTP yagami's slot 1 as the "
         "reacter for wb69's r5 target (calc_slot(focus=3, target=2, "
         "5) = 1). Pre-v0.37 the bot skipped r5 from play_targets "
         "(known_plays trusted the good-touch narrowing) and fell to "
         "a b4 finesse instead.";
  EXPECT_EQ(g.meta[wb69_slot2].status, CardStatus::CALLED_TO_PLAY)
      << "the reactive interp must stamp wb69 slot 2 (r5) as the "
         "receiver play target.";
  EXPECT_NE(g.meta[yagami_slot5].status, CardStatus::CALLED_TO_PLAY)
      << "yagami slot 5 (b3) must NOT be CTP'd — that would be the "
         "pre-v0.37 b4 finesse interpretation the fix is meant to "
         "replace.";
}
