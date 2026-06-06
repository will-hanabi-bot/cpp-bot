// Hanab.live replay 1883535 turn 16 (action index 15). Variant
// "White-Fives & Null (3 Suits)". Players: orig P0=will-bot67 (our bot POV),
// P1=will-bot69, P2=yagami_black.
//
// At action 11 yagami_black gives rank-5 to will-bot67. This is a reverse
// reactive: will-bot67 already has a pending b2 (order 3) in slot 4 from
// the earlier rank-2 + blue clues, and the reactive's intent is to push
// will-bot67's slot 1 onto the play queue *after* will-bot69 reacts by
// playing null-2 from slot 4.
//
// Action 12 (will-bot67 plays order 0 = r5): ancillary play, doesn't
// touch the queue.
// Action 13 (will-bot69 plays order 9 = n2): reactive resolution, slot 1
// (order 19, actually b3) gets called-to-play in will-bot67's hand.
// Action 14 (yagami discards order 12).
// Action 15 (will-bot67's turn): MUST play b2 (order 3) before b3
// (order 19). Pre-fix the bot played b3 immediately, misplaying it onto
// a blue stack still at 1.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// hanab.live export deck. Suits: 0=Red, 1=Blue, 2=Null. 30 cards.
const std::vector<std::pair<int, int>> kDeck = {
    {0, 5}, {1, 1}, {0, 3}, {1, 2}, {1, 4},
    {2, 2}, {0, 1}, {0, 4}, {0, 2}, {2, 2},
    {2, 5}, {2, 3}, {0, 1}, {1, 5}, {1, 4},
    {0, 3}, {1, 1}, {2, 1}, {0, 1}, {1, 3},
    {2, 1}, {2, 4}, {0, 4}, {2, 4}, {2, 3},
    {1, 3}, {1, 2}, {0, 2}, {1, 1}, {2, 1},
};

// Actions 0..14 — drives the game to just before will-bot67's misplay at
// action 15.
const std::vector<OrigAction> kOrigActions = {
    {3, 1, 1},   // 0:  P0 → P1 rank-1
    {0, 6, 0},   // 1:  P1 plays order 6 (r1)
    {3, 0, 2},   // 2:  P2 → P0 rank-2 (touches order 3 = b2)
    {2, 2, 1},   // 3:  P0 → P2 colour blue
    {0, 8, 0},   // 4:  P1 plays order 8 (r2)
    {3, 1, 4},   // 5:  P2 → P1 rank-4
    {0, 2, 0},   // 6:  P0 plays order 2 (r3)
    {0, 7, 0},   // 7:  P1 plays order 7 (r4)
    {2, 0, 1},   // 8:  P2 → P0 colour blue (b2 locks to (B,2))
    {0, 17, 0},  // 9:  P0 plays order 17 (n1)
    {0, 16, 0},  // 10: P1 plays order 16 (b1)
    {3, 0, 5},   // 11: P2 → P0 rank-5  ← reverse reactive
    {0, 0, 0},   // 12: P0 plays order 0 (r5) ancillary
    {0, 9, 0},   // 13: P1 plays order 9 (n2) reactive
    {1, 12, 0},  // 14: P2 discards order 12 (r1)
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

Game build_game_through_action_14() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot67 (P0, observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot69 (P1): orig [9,8,7,6,5] = [u2, r2, r4, r1, b4].
      {"u2", "r2", "r4", "r1", "b4"},
      // Cathy = yagami_black (P2): orig [14,13,12,11,10] = [b4, b5, r1, u3, u5].
      {"b4", "b5", "r1", "u3", "u5"},
  };
  opts.variant_name = "White-Fives & Null (3 Suits)";
  opts.starting = TestPlayer::ALICE;  // orig P0 starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

TEST(EndgameReplay1883535, PlaysHonorQueueOrderBlueTwoBeforeSlotOne) {
  Game g = build_game_through_action_14();

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE))
      << "should be will-bot67's turn at action 15";

  // Sanity: both b2 (order 3, empathy-locked) and the reactive-pushed
  // slot 1 (order 19, CTP with ambiguous inferred {b2, n3}) are in
  // common's obvious_playables. The convention says the empathy-known
  // play must resolve first.
  const Player& m = g.me();
  auto known_p = m.obvious_playables(g, 0);
  ASSERT_NE(std::find(known_p.begin(), known_p.end(), 3), known_p.end());
  ASSERT_NE(std::find(known_p.begin(), known_p.end(), 19), known_p.end());
  EXPECT_EQ(g.meta[19].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_FALSE(m.thoughts[3].id(/*infer=*/true).has_value() == false);
  EXPECT_FALSE(m.thoughts[19].id(/*infer=*/true).has_value());

  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform));
  EXPECT_EQ(std::get<PerformPlay>(perform).target, 3)
      << "bot should play b2 (order 3) before slot-1 (order 19 = b3)";
}
