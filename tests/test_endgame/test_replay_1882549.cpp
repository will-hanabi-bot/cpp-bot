// Hanab.live replay 1882549 turn 22 (action index 20). At table 392 the
// bot died inside take_action with "trying to add move to full move_history"
// after yagami played brown 4. This test replays the actions through that
// point from will-bot69's POV and asserts that take_action() returns
// without throwing.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// hanab.live export deck. Suits: 0=Red, 1=Blue, 2=Brown. 30 cards.
const std::vector<std::pair<int, int>> kDeck = {
    {0, 1}, {0, 3}, {2, 5}, {0, 2}, {1, 3},
    {1, 1}, {2, 2}, {2, 3}, {1, 2}, {2, 2},
    {2, 4}, {1, 4}, {0, 2}, {1, 5}, {2, 4},
    {0, 1}, {2, 1}, {0, 4}, {1, 1}, {2, 1},
    {0, 1}, {2, 3}, {0, 5}, {1, 2}, {2, 1},
    {0, 4}, {1, 4}, {1, 3}, {0, 3}, {1, 1},
};

// Actions 0..20 of the export — through yagami's brown-4 play that
// pre-fix triggered the crash on the next take_action call.
const std::vector<OrigAction> kOrigActions = {
    {3, 1, 2},  // 0: P0 → P1 rank-2
    {2, 0, 1},  // 1: P1 → P0 colour blue
    {3, 1, 5},  // 2: P2 → P1 rank-5
    {0, 0, 0},  // 3: P0 plays order 0
    {0, 5, 0},  // 4: P1 plays order 5
    {2, 1, 2},  // 5: P2 → P1 colour brown
    {1, 15, 0}, // 6: P0 discards order 15
    {0, 8, 0},  // 7: P1 plays order 8
    {2, 0, 2},  // 8: P2 → P0 colour brown
    {0, 3, 0},  // 9: P0 plays order 3
    {0, 16, 0}, // 10: P1 plays order 16
    {3, 1, 3},  // 11: P2 → P1 rank-3
    {0, 1, 0},  // 12: P0 plays order 1
    {0, 9, 0},  // 13: P1 plays order 9
    {3, 1, 2},  // 14: P2 → P1 rank-2
    {0, 17, 0}, // 15: P0 plays order 17
    {0, 7, 0},  // 16: P1 plays order 7
    {3, 0, 3},  // 17: P2 → P0 rank-3
    {0, 4, 0},  // 18: P0 plays order 4
    {2, 2, 2},  // 19: P1 → P2 colour brown
    {0, 14, 0}, // 20: P2 plays order 14 (brown 4)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  ctx.orig_to_my_player = {0, 1, 2};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int i = 0; i < 30; ++i) ctx.my_order_to_id[i] = kDeck[i];
  return ctx;
}

}  // namespace

TEST(EndgameReplay1882549, TakeActionAfterYagamiBrownFourDoesNotThrow) {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot67 (P1): orig [9,8,7,6,5] = [n2, b2, n3, n2, b1].
      {"n2", "b2", "n3", "n2", "b1"},
      // Cathy = yagami_black (P2): orig [14,13,12,11,10] = [n4, b5, r2, b4, n4].
      {"n4", "b5", "r2", "b4", "n4"},
  };
  opts.variant_name = "Light-Pink-Ones & Brown (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  // At this point it's Alice's (will-bot69's) turn. The bug pre-fix was that
  // take_action threw on the move_history invariant, leaving the table stuck.
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  EXPECT_NO_THROW({
    PerformAction perform = g.take_action();
    (void)perform;
  });
}
