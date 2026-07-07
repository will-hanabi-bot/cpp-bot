// Regression test for replay 1916791 (Ambiguous & Dark Null (5 Suits),
// 3-player). Players: orig P0=will-bot69, P1=will-bot67, P2=yagami_black.
//
// T27-T29 issue: at T26 will-bot67 gives a reactive blue clue (reacter =
// will-bot69, receiver = yagami), calling wb69 to act on a specific card.
// At T27 yagami gives rank-4 to will-bot67 (touching only the CTD'd,
// unclued slot-2 n4). The stable read of that clue is a ref_discard that
// CTDs wb67's slot 3 — the dark null 5, giver-visibly CRITICAL — so the
// giver and wb69 both flip to reactive via bad_stable; wb69 drops the T26
// WC, adopts the T27 one, and blind-plays t4 from slot 5 at T28 (its
// reaction: focus 2 + react slot 5 → receiver slot 2). will-bot67 cannot
// see its own slot 3, keeps the stable read AND its own T26 WC, and
// consumed wb69's T28 play as the T26 reaction — no rewind, slot 2 stayed
// CTD, and wb67 discarded the critical dark null 5 at T29.
//
// Fix: the reactive WC records the order the reacter was called to act
// on (react_order). When the reacter acts on a DIFFERENT order and a
// newer clue intervened after the WC was set up, the reaction is
// attributed to that newer clue: rewind it as REACTIVE. The replay then
// decodes wb69's slot-5 play against the T27 WC — target_i_play flips
// wb67's slot 2 from CTD to CTP promising exactly navy 4.

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

// 45-card deck from the hanab.live export. Suits: Tomato(0) Mahogany(1)
// Berry(2) Navy(3) Dark Null(4).
const std::vector<std::pair<int, int>> kDeck = {
    {1,3}, {0,4}, {0,1}, {4,3}, {1,1},  // 0-4: m3 t4 t1 u3 m1
    {4,1}, {1,1}, {1,5}, {2,2}, {4,5},  // 5-9: u1 m1 m5 b2 u5
    {3,2}, {0,1}, {0,2}, {3,1}, {3,3},  // 10-14: n2 t1 t2 n1 n3
    {2,2}, {1,2}, {3,1}, {4,2}, {2,1},  // 15-19: b2 m2 n1 u2 b1
    {3,3}, {0,3}, {3,1}, {0,1}, {2,1},  // 20-24: n3 t3 n1 t1 b1
    {3,4}, {0,4}, {1,2}, {1,4}, {2,5},  // 25-29: n4 t4 m2 m4 b5
    {1,1}, {3,2}, {2,4}, {0,3}, {1,4},  // 30-34: m1 n2 b4 t3 m4
    {3,5}, {2,4}, {2,3}, {0,5}, {1,3},  // 35-39: n5 b4 b3 t5 m3
    {3,4}, {4,4}, {2,3}, {2,1}, {0,2},  // 40-44: n4 u4 b3 b1 t2
};

// Actions T1..T28 (wb67's T29 decision is taken via take_action).
const std::vector<OrigAction> kActions = {
    {3,2,3},   // T1  P0 wb69  rank-3 -> P2 yagami
    {0,6,0},   // T2  P1 wb67  plays ord 6  = m1
    {0,13,0},  // T3  P2 yag   plays ord 13 = n1
    {1,4,0},   // T4  P0 wb69  discards ord 4 = m1
    {3,0,4},   // T5  P1 wb67  rank-4 -> P0 wb69
    {0,16,0},  // T6  P2 yag   plays ord 16 = m2
    {3,2,1},   // T7  P0 wb69  rank-1 -> P2 yagami
    {0,5,0},   // T8  P1 wb67  plays ord 5  = u1
    {0,11,0},  // T9  P2 yag   plays ord 11 = t1
    {3,2,3},   // T10 P0 wb69  rank-3 -> P2 yagami
    {0,19,0},  // T11 P1 wb67  plays ord 19 = b1
    {0,18,0},  // T12 P2 yag   plays ord 18 = u2
    {3,2,1},   // T13 P0 wb69  rank-1 -> P2 yagami
    {0,15,0},  // T14 P1 wb67  plays ord 15 = b2
    {0,12,0},  // T15 P2 yag   plays ord 12 = t2
    {3,2,1},   // T16 P0 wb69  rank-1 -> P2 yagami
    {0,21,0},  // T17 P1 wb67  plays ord 21 = t3
    {2,1,0},   // T18 P2 yag   colour-0 (Red) -> P1 wb67
    {0,0,0},   // T19 P0 wb69  plays ord 0  = m3
    {1,23,0},  // T20 P1 wb67  discards ord 23 = t1
    {0,10,0},  // T21 P2 yag   plays ord 10 = n2
    {2,2,1},   // T22 P0 wb69  colour-1 (Blue) -> P2 yagami
    {1,27,0},  // T23 P1 wb67  discards ord 27 = m2
    {0,20,0},  // T24 P2 yag   plays ord 20 = n3
    {3,1,5},   // T25 P0 wb69  rank-5 -> P1 wb67
    {2,2,1},   // T26 P1 wb67  colour-1 (Blue) -> P2 yagami (reactive)
    {3,1,4},   // T27 P2 yag   rank-4 -> P1 wb67 (the suspect clue)
    {0,1,0},   // T28 P0 wb69  blind-plays ord 1 = t4 (the T27 reaction)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // POV = will-bot67 (orig P1) -> my P0. To preserve turn cycle:
  //   orig P1 (will-bot67) -> my P0  (POV)
  //   orig P2 (yagami)     -> my P1
  //   orig P0 (will-bot69) -> my P2
  ctx.orig_to_my_player = {2, 0, 1};
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
  // my P0 = will-bot67 (POV, hidden). orig orders [9,8,7,6,5]:
  //   u5, b2, m5, m1, u1.
  // my P1 = yagami (orig P2). orig orders [14,13,12,11,10]:
  //   (3,3)=n3, (3,1)=n1, (0,2)=t2, (0,1)=t1, (3,2)=n2.
  // my P2 = will-bot69 (orig P0). orig orders [4,3,2,1,0]:
  //   (1,1)=m1, (4,3)=u3, (0,1)=t1, (0,4)=t4, (1,3)=m3.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"n3", "n1", "t2", "t1", "n2"},
      {"m1", "u3", "t1", "t4", "m3"},
  };
  opts.variant_name = "Ambiguous & Dark Null (5 Suits)";
  opts.starting = TestPlayer::CATHY;  // orig P0 (will-bot69) -> my P2
  return setup(std::move(opts));
}

}  // namespace

// After wb69's off-script T28 play, wb67 must re-attribute it to the T27
// clue (rewound as reactive): slot 2 (orig o25 -> my o25) transitions
// CTD -> CTP promising exactly navy 4, and the ref-discard CTD that the
// abandoned stable read stamped on slot 3 (the critical dark null 5,
// orig o9 -> my o4) must be gone.
TEST(EndgameReplay1916791, T28ReactionFlipsSlot2ToNavy4CTP) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < kActions.size(); ++i) apply_orig_action(g, kActions[i], ctx);

  EXPECT_EQ(g.meta[25].status, CardStatus::CALLED_TO_PLAY)
      << "slot 2 (n4) must flip CTD -> CTP once wb69's t4 reaction "
         "identifies the reactive read of T27";
  EXPECT_EQ(g.common.thoughts[25].inferred, IdentitySet::single(Identity(3, 4)))
      << "the reaction promises exactly navy 4";
  EXPECT_NE(g.meta[4].status, CardStatus::CALLED_TO_DISCARD)
      << "the stable ref_discard CTD on the critical dark null 5 must be "
         "reverted by the rewind";
}

// T29: wb67 must play the promised navy 4 from slot 2, not discard the
// critical dark null 5 (the buggy line).
TEST(EndgameReplay1916791, T29PlaysPromisedNavy4) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < kActions.size(); ++i) apply_orig_action(g, kActions[i], ctx);

  PerformAction perform = g.take_action();

  bool is_buggy_discard = std::holds_alternative<PerformDiscard>(perform) &&
                          std::get<PerformDiscard>(perform).target == 4;
  EXPECT_FALSE(is_buggy_discard)
      << "bot discarded the critical dark null 5 (my order 4) — the T28 "
         "reaction was not re-attributed to the T27 clue";

  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "wb67 must PLAY at T29 (the promised n4)";
  EXPECT_EQ(std::get<PerformPlay>(perform).target, 25)
      << "wb67 must play the promised navy 4 (my order 25, slot 2)";
}
