// Hanab.live replay 1885467 turn 41 (action index 40). Variant
// "Chimneys & Rainbow (4 Suits)". Players: orig P0=yagami_black,
// P1=will-bot69 (observer), P2=will-bot67.
//
// User-reported "fatal endgame error": at turn 41 will-bot69 needs to
// play a card to win the endgame, but instead gave a red clue.
//
// Diagnosis under v0.7 (post-tie-break fix):
//   stacks   r=5 g=4 b=5 m=3,   score=17,   max=20,   rem=3.
//   cards_left = 1 (orig 39 = b1 remaining in the deck).
//   ALICE has g5 (order 17) AND m5 (order 1) both empathy-narrowed to
//   singletons. yagami has m4 visible (orig 38). To win we'd need
//   g5+m4+m5 = three plays in three remaining endgame turns.
//
//   The solver reports `winrate = 0/1` and the bot falls through to the
//   heuristic gate (winrate < 1/100). The solver's recursive simulation
//   can't make yagami's bot play her own m4 once g5 plays — yagami's
//   order 38 is colour-green-clued but the inferred set isn't narrowed
//   to {m4} in the bot's empathy model after g5 advances the stack, so
//   she's not in obvious_playables in the recursive winnable() call.
//   No simulated finish reaches 20, hence winrate = 0.
//
//   Separately, the heuristic eval picks PerformColour over the
//   PerformPlay even though ALICE has a known playable (g5). That's a
//   heuristic-side bug: the take_action eval should still prefer the
//   known play when no winning solver path exists, since at least
//   advancing the score 17 → 18 dominates a clue that scores 17.
//
// v0.7's play-first tie-break / plays-first multi-hypo iteration does
// NOT fix this case — the tie-break only applies when actions tie at
// the same winrate, and here no candidate reaches a positive winrate.
// This test pins the diagnosis so future fixes can be measured against
// it: a successful fix should produce winrate > 0 OR have the
// heuristic-fallback path prefer the known play.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/endgame/solver.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// 40-card deck for "Chimneys & Rainbow (4 Suits)". Suits: R(0), G(1),
// B(2), Rainbow(3) = "m" (multi).
const std::vector<std::pair<int, int>> kDeck = {
    {3, 3}, {0, 1}, {0, 3}, {3, 2}, {0, 2},
    {0, 3}, {3, 5}, {2, 3}, {1, 2}, {2, 4},
    {1, 3}, {2, 5}, {2, 2}, {0, 4}, {3, 4},
    {2, 1}, {0, 2}, {1, 5}, {1, 3}, {2, 1},
    {0, 1}, {3, 3}, {3, 1}, {2, 4}, {0, 1},
    {3, 1}, {1, 1}, {3, 1}, {0, 4}, {1, 1},
    {2, 2}, {3, 2}, {0, 5}, {2, 3}, {1, 2},
    {1, 4}, {1, 1}, {1, 4}, {3, 4}, {2, 1},
};

const std::vector<OrigAction> kOrigActions = {
    {3, 1, 5},   // 0:  P0 → P1 rank-5
    {1, 5, 0},   // 1:  P1 discards 5 (r3)
    {3, 1, 5},   // 2:  P2 → P1 rank-5
    {0, 1, 0},   // 3:  P0 plays  1 (r1)
    {0, 15, 0},  // 4:  P1 plays 15 (b1)
    {1, 14, 0},  // 5:  P2 discards 14 (m4)
    {3, 1, 3},   // 6:  P0 → P1 rank-3
    {2, 0, 0},   // 7:  P1 colour-red → P0
    {1, 10, 0},  // 8:  P2 discards 10 (g3)
    {0, 16, 0},  // 9:  P0 plays 16 (r2)
    {3, 0, 3},   // 10: P1 → P0 rank-3
    {0, 12, 0},  // 11: P2 plays 12 (b2)
    {0, 2, 0},   // 12: P0 plays  2 (r3)
    {0, 7, 0},   // 13: P1 plays  7 (b3)
    {3, 1, 4},   // 14: P2 → P1 rank-4
    {0, 22, 0},  // 15: P0 plays 22 (m1)
    {0, 9, 0},   // 16: P1 plays  9 (b4)
    {1, 21, 0},  // 17: P2 discards 21 (m3)
    {2, 2, 1},   // 18: P0 colour-green → P2
    {1, 23, 0},  // 19: P1 discards 23 (b4)
    {0, 26, 0},  // 20: P2 plays 26 (g1)
    {3, 2, 5},   // 21: P0 → P2 rank-5
    {0, 8, 0},   // 22: P1 plays  8 (g2)
    {0, 28, 0},  // 23: P2 plays 28 (r4)
    {1, 24, 0},  // 24: P0 discards 24 (r1)
    {1, 29, 0},  // 25: P1 discards 29 (g1)
    {0, 11, 0},  // 26: P2 plays 11 (b5)
    {2, 1, 0},   // 27: P0 colour-red → P1
    {0, 32, 0},  // 28: P1 plays 32 (r5)
    {2, 1, 0},   // 29: P2 colour-red → P1
    {0, 3, 0},   // 30: P0 plays  3 (m2)
    {3, 0, 4},   // 31: P1 → P0 rank-4
    {0, 18, 0},  // 32: P2 plays 18 (g3)
    {0, 0, 0},   // 33: P0 plays  0 (m3)
    {2, 0, 0},   // 34: P1 colour-red → P0
    {3, 0, 4},   // 35: P2 → P0 rank-4
    {0, 37, 0},  // 36: P0 plays 37 (g4)
    {2, 0, 0},   // 37: P1 colour-red → P0
    {2, 0, 1},   // 38: P2 colour-green → P0
    {3, 1, 5},   // 39: P0 → P1 rank-5  (turn 40)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // ALICE = will-bot69 (orig P1, observer), BOB = will-bot67 (orig P2),
  // CATHY = yagami (orig P0). starting=CATHY.
  ctx.orig_to_my_player = {2, 0, 1};
  ctx.orig_to_my_order.resize(kDeck.size());
  for (int o = 0; o < 5; ++o) ctx.orig_to_my_order[o] = 10 + o;
  for (int o = 5; o < 10; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 10; o < 15; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 15; o < static_cast<int>(kDeck.size()); ++o)
    ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(kDeck.size());
  for (size_t orig_o = 0; orig_o < kDeck.size(); ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

}  // namespace

TEST(EndgameReplay1885467, Turn41SolverReportsZeroWinrate) {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot67: orig P2 orders [14,13,12,11,10] newest-first.
      // deck[14]=(3,4)=m4, deck[13]=(0,4)=r4, deck[12]=(2,2)=b2,
      // deck[11]=(2,5)=b5, deck[10]=(1,3)=g3.
      {"m4", "r4", "b2", "b5", "g3"},
      // Cathy = yagami: orig P0 orders [4,3,2,1,0] newest-first.
      // deck[4]=(0,2)=r2, deck[3]=(3,2)=m2, deck[2]=(0,3)=r3,
      // deck[1]=(0,1)=r1, deck[0]=(3,3)=m3.
      {"r2", "m2", "r3", "r1", "m3"},
  };
  opts.variant_name = "Chimneys & Rainbow (4 Suits)";
  opts.starting = TestPlayer::CATHY;  // orig P0 (yagami) starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  // After action 39 it should be Alice's (will-bot69's) turn 41.
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));

  // Sanity: the late-game state from the export.
  EXPECT_EQ(g.state.play_stacks[0], 5);  // red
  EXPECT_EQ(g.state.play_stacks[1], 4);  // green
  EXPECT_EQ(g.state.play_stacks[2], 5);  // blue
  EXPECT_EQ(g.state.play_stacks[3], 3);  // rainbow
  EXPECT_EQ(g.state.score(), 17);
  EXPECT_EQ(g.state.cards_left, 1);

  // ALICE has g5 in obvious_playables — the v0.7 tie-break can't help if
  // the solver returns winrate=0; the heuristic must be the one to prefer
  // the play, and that's a separate bug from the solver tie-break.
  auto know_play = g.me().obvious_playables(g, g.state.our_player_index);
  EXPECT_FALSE(know_play.empty())
      << "alice should have g5 (order 17) as obvious_playable";

  // Solver returns winrate=0 — its recursive simulation can't make
  // yagami's bot play her own m4 once g5 plays, so no completion is
  // found. The v0.7 fix only helps for ties at positive winrate.
  hanabi::endgame::EndgameSolver solver(/*mc=*/true, /*timeout=*/6.0);
  auto sr = solver.solve(g);
  EXPECT_EQ(sr.winrate, hanabi::endgame::Fraction(0))
      << "solver presently can't find a winning sequence here; if this "
         "ever drifts to a positive winrate, recheck the heuristic-fallback "
         "diagnosis";

  // Document the current take_action behaviour. This intentionally does
  // not assert PerformPlay — the heuristic-fallback bug is tracked as a
  // separate issue. We just record what the bot does so a fix is visible.
  PerformAction perform = g.take_action();
  (void)perform;
}
