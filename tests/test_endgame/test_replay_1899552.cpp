// Diagnostic + regression test for replay 1899552 T9 — user-reported
// strike-resets-CTP bug.
//
// Variant: Prism Reversed (3 Suits). Suits r, b, p (Prism Reversed -> p).
// Players (orig order): P0=yagami, P1=will-bot67 (POV), P2=will-bot69.
//
// At T9, will-bot69 misplays b1 (a duplicate of the b1 already played
// by yagami at T4). The bombed-discard handler in interpret_discard
// (`src/basics/game.cpp:205-220`) unconditionally clears all conv info
// (`m = m.cleared()`) on every card in every hand -- including
// will-bot67's slot 2 which was CTP'd via the rank-4 clue at T6.
//
// User expectation: cards that are CTP'd should keep their CTP across
// a strike. The v0.26 fix (game.cpp:801-822) preserves CTP through the
// elim-driven reset for dupe-strike scenarios, but that runs *after*
// the bombed-discard handler has already wiped meta. The fix below
// applies the same preservation in the bombed-discard handler.

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

// 30-card deck from the hanab.live export.
const std::vector<std::pair<int, int>> kDeck = {
    {1,3}, {2,5}, {2,1}, {0,3}, {1,1},
    {2,5}, {2,3}, {2,3}, {1,2}, {2,5},
    {2,2}, {0,2}, {0,4}, {1,5}, {1,1},
    {1,4}, {2,4}, {1,1}, {1,3}, {0,1},
    {0,4}, {0,1}, {0,1}, {0,2}, {0,5},
    {2,2}, {1,2}, {0,3}, {2,4}, {1,4},
};

// Actions T1..T9. T9 = will-bot69 misplays b1 (the strike).
const std::vector<OrigAction> kActions = {
    {3,2,5},  // T1 P0 yagami rank-5 -> P2 wb69
    {0,9,0},  // T2 P1 wb67 plays ord 9 = p5
    {3,1,3},  // T3 P2 wb69 rank-3 -> P1 wb67
    {0,4,0},  // T4 P0 yagami plays ord 4 = b1
    {0,8,0},  // T5 P1 wb67 plays ord 8 = b2
    {3,1,4},  // T6 P2 wb69 rank-4 -> P1 wb67  <-- CTPs (or touches) wb67's slot 2
    {0,0,0},  // T7 P0 yagami plays ord 0 = b3
    {2,2,1},  // T8 P1 wb67 colour-1 (blue) -> P2 wb69
    {0,14,0}, // T9 P2 wb69 plays ord 14 = b1  <-- DUPE-STRIKE (b1 already played at T4)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // POV = will-bot67 (orig P1) -> my P0. The remapping must preserve
  // the turn cycle (engine cycles 0->1->2->0; orig cycled 0->1->2),
  // so orig P0 must land at my P_{(POV+n-1) % n} = my P2.
  //   orig P0 (yagami)     -> my P2
  //   orig P1 (will-bot67) -> my P0  (POV)
  //   orig P2 (will-bot69) -> my P1
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
  // my P0 = will-bot67 (POV, hidden).
  // my P1 = will-bot69 (orig P2). Initial orig orders [14,13,12,11,10]:
  //   (1,1)=b1, (1,5)=b5, (0,4)=r4, (0,2)=r2, (2,2)=p2.
  // my P2 = yagami (orig P0). Initial orig orders [4,3,2,1,0]:
  //   (1,1)=b1, (0,3)=r3, (2,1)=p1, (2,5)=p5, (1,3)=b3.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b1", "b5", "r4", "r2", "p2"},
      {"b1", "r3", "p1", "p5", "b3"},
  };
  opts.variant_name = "Prism Reversed (3 Suits)";
  // orig P0 (yagami -> my P2) starts.
  opts.starting = TestPlayer::CATHY;
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kActions[i], ctx);
}

}  // namespace

// Verifies that wb67's slot 2 (b4) keeps its CTP-derived state across
// the T9 dupe-strike. Sequence:
//   T6: wb69 clues rank-4 to wb67 -- narrows wb67's slot 2 inferred
//       to {r4, b4} (p4 visible-pinned to yagami via reasoning).
//   T7: yagami plays b3, advancing b stack to 3.
//   T8: wb67 clues blue to wb69 (unrelated).
//   ---- between T6 and T8 the convention re-evaluates: with b stack
//        now at 3, b4 is the leftmost playable rank-4 and the
//        narrowing pins slot 2 to {b4} singleton with status CTP. ----
//   T9: wb69 plays b1 -- a DUPE of the b1 yagami played at T4. Strike.
//
// Pre-fix the bombed-discard handler in `Game::interpret_discard`
// (game.cpp:205-220) called `m = m.cleared()` on every card in every
// hand, which set wb67's slot 2 status back to NONE -- silently
// dropping the CTP. The fix skips cards whose status is
// CALLED_TO_PLAY, mirroring the v0.26 elim-driven CTP preservation
// that runs only afterwards.
TEST(EndgameReplay1899552, T9DupeStrikePreservesSlot2CTP) {
  Game g = build_start();
  apply_prefix(g, 8);  // T1..T8 -- through the blue clue, just before the strike.

  int wb67_slot2 = g.state.hands[0][1];
  ASSERT_EQ(g.meta[wb67_slot2].status, CardStatus::CALLED_TO_PLAY)
      << "Sequence setup: by T8 the rank-4 clue from T6 + b3 play at T7 "
         "must have narrowed wb67's slot 2 to a singleton CTP b4. If "
         "this assertion fails the test's pre-strike assumption is "
         "stale -- the convention path no longer produces a CTP here.";

  // T9: wb69 dupe-strikes b1.
  ReplayContext ctx = make_ctx();
  apply_orig_action(g, kActions[8], ctx);

  EXPECT_EQ(g.state.strikes, 1) << "T9 must increment strikes";
  EXPECT_EQ(g.meta[wb67_slot2].status, CardStatus::CALLED_TO_PLAY)
      << "wb67's slot 2 CTP must survive the T9 dupe-strike. Pre-fix "
         "the bombed-discard handler in interpret_discard wiped it via "
         "`m.cleared()`. The fix preserves CTP cards across strikes "
         "(consistent with the v0.26 elim-driven CTP preservation).";
}
