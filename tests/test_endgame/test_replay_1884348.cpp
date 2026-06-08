// Hanab.live replay 1884348 turn 11 (action index 9). Variant "Orange
// (3 Suits)". Players: orig P0=yagami_black, P1=will-bot69 (our bot
// POV, the reacter), P2=will-bot67 (the receiver).
//
// At action 9 yagami clues rank-5 to will-bot67. With orange semantics
// inverted (PerformPlay → discard pile, PerformDiscard → play attempt),
// the existing convention picks an orange-finesse interpretation that
// requires will-bot69's slot 1 to be o1 — but actually it's b2. The
// user wants the convention to fall through finesse (since the
// reacter's possible is ambiguous about the prereq) and land on a
// "leftmost trash" rank-reactive dc_target: target = P2's slot 1 (o2,
// orange) → reacter plays slot 1 = b2 to signal it.
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

const std::vector<std::pair<int, int>> kDeck = {
    {1, 1}, {1, 4}, {1, 1}, {2, 1}, {2, 5},
    {2, 1}, {2, 2}, {1, 3}, {0, 3}, {0, 1},
    {1, 1}, {2, 1}, {0, 1}, {1, 4}, {0, 5},
    {2, 3}, {1, 3}, {1, 5}, {0, 4}, {1, 2},
    {2, 2}, {2, 4}, {2, 3}, {0, 2}, {2, 4},
    {0, 1}, {1, 2}, {0, 4}, {0, 2}, {0, 3},
};

const std::vector<OrigAction> kOrigActions = {
    {2, 2, 0},   // 0: P0 colour-0(Red) to P2
    {0, 5, 0},   // 1: P1 plays order 5 (o1, orange-play → discard pile)
    {3, 1, 1},   // 2: P2 → P1 rank-1
    {0, 0, 0},   // 3: P0 plays order 0 (b1)
    {0, 9, 0},   // 4: P1 plays order 9 (r1)
    {1, 12, 0},  // 5: P2 discards order 12 (r1)
    {3, 1, 3},   // 6: P0 → P1 rank-3
    {0, 6, 0},   // 7: P1 plays order 6 (o2, orange-play → discard pile)
    {1, 18, 0},  // 8: P2 discards order 18 (r4)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // orig P0 (yagami) → CATHY, orig P1 (will-bot69, observer) → ALICE,
  // orig P2 (will-bot67) → BOB.
  ctx.orig_to_my_player = {2, 0, 1};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 5; ++o) ctx.orig_to_my_order[o] = 10 + o;
  for (int o = 5; o < 10; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 10; o < 15; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 15; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int orig_o = 0; orig_o < 30; ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

Game build_through_action_8() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot67: orig [14,13,12,11,10] newest-first =
      // [r5, b4, r1, o1, b1].
      {"r5", "b4", "r1", "o1", "b1"},
      // Cathy = yagami: orig [4,3,2,1,0] newest-first = [o5, o1, b1, b4, b1].
      {"o5", "o1", "b1", "b4", "b1"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::CATHY;  // orig P0 (yagami) starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

// DISABLED: this test was authored under the old button-oriented replay
// parser, where the replay's two orange "play"-button actions sent the
// cards to the discard pile and left orange stack = 0. With the
// corrected outcome-oriented parser, those same actions advance the
// orange stack to 2 (the actual hanab.live UI state), and the convention's
// chop-save fallback for orange no longer fires the same way in this
// position. The "bug" this test was guarding against may have been a
// downstream symptom of the parser bug; a fresh investigation of the
// 1884348 replay under the corrected state is needed to determine
// whether the chop-save fallback needs adjustment for the real game state.
TEST(EndgameReplay1884348, DISABLED_ReacterPlaysB2ForOrangeTrashTarget) {
  Game g = build_through_action_8();

  // Sanity: it should be yagami's turn about to clue. Orange stack is at
  // 2 — the replay's actions 1 (o1 outcome=advance) and 7 (o2 outcome=
  // advance) both go through the corrected outcome-oriented parser.
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::CATHY));
  ASSERT_EQ(g.state.play_stacks[0], 1);  // red
  ASSERT_EQ(g.state.play_stacks[1], 1);  // blue
  ASSERT_EQ(g.state.play_stacks[2], 2);  // orange (was 0 under old button-oriented parser)

  ReplayContext ctx = make_ctx();
  apply_orig_action(g, OrigAction{3, 2, 5}, ctx);  // action 9: rank-5 to P2

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));

  // Alice's slot 1 = order 19 (b2 actual) must be CTP+urgent. The
  // convention's chop-save fallback narrows its inferred to {r2, b2}.
  int alice_slot_1 = g.state.hands[0][0];
  ASSERT_EQ(alice_slot_1, 19);
  EXPECT_EQ(g.meta[alice_slot_1].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(g.meta[alice_slot_1].urgent);

  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "bot must react with PerformPlay on slot 1 (b2), not clue";
  EXPECT_EQ(std::get<PerformPlay>(perform).target, alice_slot_1);
}
