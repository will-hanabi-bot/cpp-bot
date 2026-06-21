// Regression tests for replay 1899623 (Ambiguous (6 Suits), 3-player).
//
// Two user-reported bugs from this replay, both addressed in v0.39:
//
// T7 issue (n2 bomb at T9): will-bot67 gives rank-3 to yagami
// (target=bob from wb67's POV). Pre-v0.39 the dispatcher's reacter-
// search loop vacuously picked wb69 (cathy — no obvious_playables →
// empty old_play → vacuous "shrunk" match) as reacter, routing the
// clue to interpret_reactive_rank. The reactive play_targets loop then
// picked yagami's slot 1 (n1) as the chain destination and CTP'd
// wb69's slot 1 (n2 actual) as the filler. But yagami's queue already
// had m2 (CTP'd from earlier) ahead of n1 — she played m2 at T8, the
// chain prereq never fired, and wb69 played n2 at T9 against navy
// stack 0 → bomb. v0.39 fix: interpret_reactive_rank's play_targets
// loop now skips targets blocked by older CTP'd cards in the
// receiver's hand, falling through to the finesse fallback (whose
// POV-invariant guard validates the chain). The finesse fallback
// resolves to a correct t2/t3 chain in this replay.
//
// T16 issue (spurious `[kt]` on wb69 slot 2): will-bot67 gives rank-4
// to will-bot69 (target=cathy from wb67's POV). Pre-v0.39 the
// dispatcher's reacter-search loop vacuously picked wb69 as reacter
// (cathy=target), satisfied reacter==target, and routed to
// interpret_stable. try_stable's ref_discard stamped CTD on wb69 slot
// 2 (n3) — visible to observers as a `[kt]` note. Yagami at T17 then
// played an unexpected card, triggering the response-inversion rewind
// that reverted the bad stable to reactive (`[reset]`). v0.39 fix:
// the dispatcher's vacuous-truth guard blocks the reacter==target
// pick when target != bob; the clue routes directly to
// interpret_reactive with reacter=bob, and no spurious CTD is
// stamped.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// 60-card deck from hanab.live export.
const std::vector<std::pair<int, int>> kDeck = {
    {2,2}, {2,4}, {3,3}, {4,4}, {0,3},
    {0,1}, {1,2}, {0,3}, {5,4}, {2,3},
    {1,2}, {1,5}, {1,1}, {0,2}, {3,1},
    {5,2}, {5,1}, {5,2}, {2,1}, {1,1},
    {0,1}, {5,3}, {4,2}, {0,4}, {4,4},
    {2,4}, {5,1}, {4,1}, {5,1}, {3,1},
    {3,5}, {2,1}, {4,1}, {0,1}, {1,3},
    {4,3}, {0,2}, {2,2}, {3,4}, {4,1},
    {3,3}, {3,1}, {4,3}, {0,5}, {5,3},
    {1,1}, {1,4}, {2,5}, {0,4}, {2,3},
    {1,4}, {5,4}, {4,2}, {5,5}, {4,5},
    {3,2}, {3,2}, {1,3}, {3,4}, {2,1},
};

// Actions T1..T16. T7 = rank-3 clue (issue 1 trigger), T16 = rank-4
// clue (issue 2 trigger).
const std::vector<OrigAction> kActions = {
    {2,1,1},  // T1  P0 wb67  colour-1 (Green) -> P1 yagami
    {2,0,1},  // T2  P1 yag   colour-1 (Green) -> P0 wb67
    {0,14,0}, // T3  P2 wb69  plays ord 14 = o1
    {3,1,4},  // T4  P0 wb67  rank-4 -> P1 yagami
    {0,5,0},  // T5  P1 yag   plays ord 5  = t1
    {0,12,0}, // T6  P2 wb69  plays ord 12 = m1
    {3,1,3},  // T7  P0 wb67  rank-3 -> P1 yagami  (= issue-1 clue)
    {0,6,0},  // T8  P1 yag   plays ord 6  = m2
    {0,17,0}, // T9  P2 wb69  plays ord 17 = n2    (PRE-v0.39 BOMB)
    {3,2,5},  // T10 P0 wb67  rank-5 -> P2 wb69
    {0,18,0}, // T11 P1 yag   plays ord 18 = e1
    {0,13,0}, // T12 P2 wb69  plays ord 13 = t2
    {1,4,0},  // T13 P0 wb67  discards ord 4 = t3
    {2,0,2},  // T14 P1 yag   colour-2 (Blue) -> P0 wb67
    {1,19,0}, // T15 P2 wb69  discards ord 19 = m1
    {3,2,4},  // T16 P0 wb67  rank-4 -> P2 wb69    (= issue-2 clue)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // POV = will-bot69 (orig P2) -> my P0. To preserve turn cycle:
  //   orig P2 (will-bot69) -> my P0  (POV)
  //   orig P0 (will-bot67) -> my P1  (one step forward in cycle from POV)
  //   orig P1 (yagami)     -> my P2  (two steps)
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
  // my P0 = will-bot69 (POV, hidden).
  // my P1 = will-bot67 (orig P0). orig orders [4,3,2,1,0]:
  //   (0,3)=t3, (4,4)=b4, (3,3)=o3, (2,4)=e4, (2,2)=e2.
  // my P2 = yagami (orig P1). orig orders [9,8,7,6,5]:
  //   (2,3)=e3, (5,4)=n4, (0,3)=t3, (1,2)=m2, (0,1)=t1.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"t3", "b4", "o3", "e4", "e2"},
      {"e3", "n4", "t3", "m2", "t1"},
  };
  opts.variant_name = "Ambiguous (6 Suits)";
  opts.starting = TestPlayer::BOB;  // orig P0 (will-bot67) -> my P1
  return setup(std::move(opts));
}

}  // namespace

// T7 regression. Stepping past the suspect rank-3 clue from will-bot67
// to yagami should not commit a CTP on will-bot69's slot 1 (the n2
// actual that bombed at T9 pre-fix). With v0.39's interpret_reactive_
// rank older-CTP guard the play_targets loop skips yagami's slot 1
// (n1) because yagami's slot 5 m2 is already CTP'd from earlier and
// would fire first; the convention falls through to the finesse
// fallback, which validly CTPs wb69's slot 3 (t2 actual) as a finesse
// prereq for yagami's slot 4 t3.
TEST(EndgameReplay1899623, T7DoesNotCTPNavy2InWillbot69) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (int i = 0; i < 7; ++i) apply_orig_action(g, kActions[i], ctx);

  // wb69 slot 1 (= my ord 17, n2 actual) is the card that bombed at
  // T9 pre-fix. It MUST NOT be CTP'd by T7's clue.
  // (Strikes-from-the-replay aren't asserted here because the replay's
  // T9 action is fixed in stone — `apply_orig_action` will replay
  // wb69's PerformPlay{17} regardless of what the bot's interp said.
  // The user-facing fix is "the convention no longer signals the
  // bomb", which is what this assertion captures.)
  int wb69_slot1 = g.state.hands[0][0];
  EXPECT_NE(g.meta[wb69_slot1].status, CardStatus::CALLED_TO_PLAY)
      << "T7 rank-3 → yagami must not CTP wb69 slot 1 (n2). Pre-v0.39 "
         "the reactive play_targets loop picked yagami slot 1 (n1) as "
         "the chain destination — but yagami's older CTP (m2 at slot "
         "5) fires first, breaking the prereq and the n2 strikes.";
}

// T16 regression. The rank-4 clue from will-bot67 to will-bot69 must
// not mark wb69's slot 2 (n3) as CALLED_TO_DISCARD via the spurious
// stable ref_discard. With v0.39's dispatcher vacuous-truth guard the
// clue routes directly to interpret_reactive with reacter=bob; the
// reactive interp leaves slot 2 untouched and the waiting connection
// is reactive-shaped (inverted=false, reacter=yagami).
TEST(EndgameReplay1899623, T16DoesNotCTDNavy3OnWillbot69Slot2) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (int i = 0; i < 16; ++i) apply_orig_action(g, kActions[i], ctx);

  // wb69 slot 2 must not be CTD'd. Pre-v0.39 dispatcher routed T16 to
  // stable via vacuous reacter==target (= wb69), and ref_discard
  // stamped CTD on the first unclued post-focus slot.
  int wb69_slot2 = g.state.hands[0][1];
  EXPECT_NE(g.meta[wb69_slot2].status, CardStatus::CALLED_TO_DISCARD)
      << "T16 rank-4 → wb69 must not mark slot 2 (n3) as CTD via the "
         "buggy stable ref_discard path.";

  // The T16 WC must be reactive-shaped (inverted=false), not the
  // stable+response-inversion shape (inverted=true) the buggy
  // dispatcher used to produce.
  ASSERT_FALSE(g.waiting.empty())
      << "T16 reactive interp must leave a waiting connection";
  EXPECT_FALSE(g.waiting.front().inverted)
      << "T16 WC must be reactive (inverted=false). inverted=true "
         "means try_stable set it up via response-inversion — i.e. "
         "the bug.";
  EXPECT_EQ(g.waiting.front().reacter, 2)
      << "Reactive reacter at T16 must be my P2 (yagami / bob). "
         "Pre-fix reacter was the vacuous match P0 (wb69 / target).";
}
