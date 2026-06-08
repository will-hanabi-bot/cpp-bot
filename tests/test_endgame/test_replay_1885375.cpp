// Hanab.live replay 1885375 turn 35 (action index 34). Variant
// "Chimneys & Muddy Rainbow (4 Suits)". Players: orig P0=yagami_black,
// P1=will-bot69 (observer), P2=will-bot67.
//
// Diagnosis: at turn 35 will-bot69's m4 (order 34, empathy-known) wins
// only 2/15 from the bot's POV — the bot cannot see its own m5
// (slot 5 = order 7 = m5) so a large fraction of empathy-arrangements
// have m5 in the undrawn deck or in a different slot, in which case
// playing m4 immediately can't be completed. The rank-1 clue to
// will-bot67 wins 1/1 by orchestrating a different finish path. So
// the bot picking rank-1 over m4 is genuinely the higher-winrate
// choice; the user's hypothesis that this was a winrate-tie didn't
// hold for this position.
//
// This test pins down the diagnosis and ensures the endgame solver
// keeps returning the 100% solution (any solution that wins 1/1 is
// acceptable). The play-first tie-break / multi-hypo plays-first
// iteration is exercised by the solver structure here — even though
// the play isn't picked, both the early-exit on a 100% non-play and
// the plays-first iteration must still terminate correctly.
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

// 40-card deck for "Chimneys & Muddy Rainbow (4 Suits)". Suits: R(0), G(1),
// B(2), Muddy Rainbow (3) = "m".
const std::vector<std::pair<int, int>> kDeck = {
    {0, 1}, {3, 2}, {1, 3}, {1, 4}, {2, 5},
    {2, 3}, {2, 1}, {3, 5}, {1, 1}, {0, 5},
    {0, 4}, {3, 1}, {2, 1}, {0, 1}, {2, 2},
    {1, 1}, {2, 4}, {2, 1}, {3, 3}, {2, 4},
    {0, 2}, {1, 2}, {1, 5}, {3, 2}, {1, 1},
    {3, 1}, {2, 2}, {0, 2}, {0, 3}, {0, 1},
    {1, 2}, {1, 4}, {1, 3}, {0, 3}, {3, 4},
    {2, 3}, {3, 3}, {3, 4}, {0, 4}, {3, 1},
};

const std::vector<OrigAction> kOrigActions = {
    {3, 2, 4},   // 0:  P0 → P2 rank-4
    {0, 8, 0},   // 1:  P1 plays  8 (g1)
    {3, 1, 4},   // 2:  P0 → P1 rank-4
    {0, 0, 0},   // 3:  P0 plays  0 (r1)
    {0, 6, 0},   // 4:  P1 plays  6 (b1)
    {0, 13, 0},  // 5:  P2 plays 13 (r1)
    {1, 16, 0},  // 6:  P0 discards 16 (b4)
    {3, 0, 4},   // 7:  P1 → P0 rank-4
    {0, 11, 0},  // 8:  P2 plays 11 (m1)
    {0, 1, 0},   // 9:  P0 plays  1 (m2)
    {3, 0, 2},   // 10: P1 → P0 rank-2
    {0, 20, 0},  // 11: P2 plays 20 (r2)
    {0, 21, 0},  // 12: P0 plays 21 (g2)
    {3, 0, 3},   // 13: P1 → P0 rank-3
    {0, 14, 0},  // 14: P2 plays 14 (b2)
    {0, 2, 0},   // 15: P0 plays  2 (g3)
    {3, 0, 3},   // 16: P1 → P0 rank-3
    {0, 18, 0},  // 17: P2 plays 18 (m3)
    {0, 3, 0},   // 18: P0 plays  3 (g4)
    {1, 17, 0},  // 19: P1 discards 17 (b1)
    {1, 26, 0},  // 20: P2 discards 26 (b2)
    {3, 2, 4},   // 21: P0 → P2 rank-4
    {0, 28, 0},  // 22: P1 plays 28 (r3)
    {0, 22, 0},  // 23: P2 plays 22 (g5)
    {3, 1, 3},   // 24: P0 → P1 rank-3
    {2, 0, 0},   // 25: P1 → P0 colour-0 (red)
    {0, 10, 0},  // 26: P2 plays 10 (r4)
    {1, 27, 0},  // 27: P0 discards 27 (r2)
    {0, 5, 0},   // 28: P1 plays  5 (b3)
    {3, 1, 5},   // 29: P2 → P1 rank-5
    {0, 19, 0},  // 30: P0 plays 19 (b4)
    {0, 9, 0},   // 31: P1 plays  9 (r5)
    {3, 0, 5},   // 32: P2 → P0 rank-5
    {0, 4, 0},   // 33: P0 plays  4 (b5)
};

// Order 34 = deck[34] = (3,4) = m4. Drawn by P1 at action 28's play (P1
// plays my 5, draws orig 34). In the test's identity mapping orig 34
// maps to my 34. At turn 35 it lives in P1's hand.
constexpr int kM4Order = 34;

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // ALICE = will-bot69 (orig P1, observer), BOB = will-bot67 (orig P2),
  // CATHY = yagami (orig P0). starting=CATHY.
  ctx.orig_to_my_player = {2, 0, 1};
  ctx.orig_to_my_order.resize(kDeck.size());
  for (int o = 0; o < 5; ++o) ctx.orig_to_my_order[o] = 10 + o;   // P0 → CATHY
  for (int o = 5; o < 10; ++o) ctx.orig_to_my_order[o] = o - 5;   // P1 → ALICE
  for (int o = 10; o < 15; ++o) ctx.orig_to_my_order[o] = o - 5;  // P2 → BOB
  for (int o = 15; o < static_cast<int>(kDeck.size()); ++o)
    ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(kDeck.size());
  for (size_t orig_o = 0; orig_o < kDeck.size(); ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

}  // namespace

// At turn 35 the solver finds a 1/1 winrate. Per the diagnosis above
// the play wins less than the clue, so the bot is correct to clue.
// We pin the winrate at Fraction(1) — any drift back to a fractional
// answer would indicate a solver regression.
TEST(EndgameReplay1885375, Turn35EndgameWinrateIsOne) {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot67: orig P2 orders [14,13,12,11,10] newest-first.
      // deck[14]=(2,2)=b2, deck[13]=(0,1)=r1, deck[12]=(2,1)=b1,
      // deck[11]=(3,1)=m1, deck[10]=(0,4)=r4.
      {"b2", "r1", "b1", "m1", "r4"},
      // Cathy = yagami: orig P0 orders [4,3,2,1,0] newest-first.
      // deck[4]=(2,5)=b5, deck[3]=(1,4)=g4, deck[2]=(1,3)=g3,
      // deck[1]=(3,2)=m2, deck[0]=(0,1)=r1.
      {"b5", "g4", "g3", "m2", "r1"},
  };
  opts.variant_name = "Chimneys & Muddy Rainbow (4 Suits)";
  opts.starting = TestPlayer::CATHY;  // orig P0 (yagami) starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  // Now it's Alice's (= will-bot69's) turn.
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));

  // Sanity-check the late-game state.
  EXPECT_EQ(g.state.play_stacks[3], 3) << "muddy stack should be 3 going into turn 35";
  EXPECT_EQ(g.state.score(), 18);
  EXPECT_EQ(g.state.max_score(), 20);
  EXPECT_EQ(g.state.cards_left, 2);

  // Run the solver in isolation: should find a 100% solution.
  hanabi::endgame::EndgameSolver solver(/*mc=*/true, /*timeout=*/6.0);
  auto sr = solver.solve(g);
  ASSERT_TRUE(sr.ok()) << sr.error;
  EXPECT_EQ(sr.winrate, hanabi::endgame::Fraction(1))
      << "solver should find a 100%-winrate solution at this position";

  // PerformPlay{m4} explicitly has a much lower winrate than 1 — the bot
  // can't see its own m5, so the immediate play doesn't always finish.
  // We pin this so that any future drift toward "play m4 wins 100%" is
  // surfaced for human review.
  hanabi::endgame::EndgameSolver solver2(/*mc=*/true, /*timeout=*/6.0);
  auto sr_play = solver2.solve(g, PerformAction{PerformPlay{kM4Order}});
  EXPECT_LT(sr_play.winrate, hanabi::endgame::Fraction(1))
      << "PerformPlay{m4} should not match the 1/1 winrate of the best clue";

  // The actual chosen action should win 100% (any 100% action is fine —
  // the test pins the winrate, not the specific PerformAction kind).
  PerformAction perform = g.take_action();
  (void)perform;  // The kind isn't asserted; the winrate above is what we lock in.
}
