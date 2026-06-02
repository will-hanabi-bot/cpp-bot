// Hanab.live replay 1876746 turn 27. yagami clued rank-4 to will-bot69 on
// turn 19 (action 18); will-bot67 reacted by playing slot 1 (action 19),
// marking will-bot69's slot 4 as CALLED_TO_PLAY. That slot is order 14 in
// the orig export = white-5. After the reactive play, will-bot69 sees both
// blue-3 copies elsewhere in play/discard, so the inferred set should
// narrow from {b3, w5} to {w5} and will-bot69 plays w5 on her next turn.
//
// In the actual replay the bot discarded order 13 (a red-4) instead of
// playing order 14 — the empathy narrowing didn't happen. This test
// reproduces the exact position and asserts take_action() returns
// PerformPlay(slot-4) = PerformPlay(my-order 4).
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

// hanab.live export deck (https://hanab.live/export/1876746). 30 cards.
// Suit indices: 0=Red, 1=Blue, 2=White.
const std::vector<std::pair<int, int>> kDeck = {
    {2, 2}, {1, 3}, {0, 1}, {2, 1}, {1, 2},
    {2, 4}, {0, 2}, {1, 3}, {1, 4}, {2, 2},
    {1, 2}, {2, 3}, {0, 4}, {0, 4}, {2, 5},
    {0, 1}, {1, 4}, {2, 3}, {0, 2}, {0, 3},
    {1, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 5},
    {0, 3}, {1, 5}, {2, 1}, {2, 4}, {2, 1},
};

const std::vector<OrigAction> kOrigActions = {
    {3, 1, 2},  {1, 8, 0},  {3, 1, 3},  {0, 3, 0},  {0, 15, 0},
    {3, 0, 2},  {0, 0, 0},  {3, 0, 3},  {0, 11, 0}, {0, 18, 0},
    {3, 0, 4},  {0, 19, 0}, {0, 20, 0}, {3, 0, 4},  {0, 12, 0},
    {3, 2, 2},  {0, 5, 0},  {0, 10, 0}, {3, 2, 4},  {0, 24, 0},
    {2, 1, 1},  {0, 1, 0},  {1, 7, 0},  {2, 0, 1},  {0, 16, 0},
    {0, 26, 0},
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Player mapping: orig yagami(0)→Bob(1), will-bot67(1)→Cathy(2),
  // will-bot69(2)→Alice(0). Bot's POV is will-bot69.
  ctx.orig_to_my_player = {1, 2, 0};
  ctx.orig_to_my_order.resize(30);
  // Alice (orig P2 will-bot69) initial hand orig 10-14 → my 0-4 (oldest first).
  for (int o = 10; o < 15; ++o) ctx.orig_to_my_order[o] = o - 10;
  // Bob (orig P0 yagami) initial hand orig 0-4 → my 5-9.
  for (int o = 0; o < 5; ++o) ctx.orig_to_my_order[o] = o + 5;
  // Cathy (orig P1 will-bot67) initial hand orig 5-9 → my 10-14.
  for (int o = 5; o < 10; ++o) ctx.orig_to_my_order[o] = o + 5;
  // Later draws happen in the same order in our simulation as in the orig.
  for (int o = 15; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  // my-orders 0..4: Alice's orig 10..14
  for (int i = 0; i < 5; ++i) ctx.my_order_to_id[i] = kDeck[10 + i];
  // my-orders 5..9: Bob's orig 0..4
  for (int i = 0; i < 5; ++i) ctx.my_order_to_id[5 + i] = kDeck[i];
  // my-orders 10..14: Cathy's orig 5..9
  for (int i = 0; i < 5; ++i) ctx.my_order_to_id[10 + i] = kDeck[5 + i];
  // Later draws keep their orig order.
  for (int i = 15; i < 30; ++i) ctx.my_order_to_id[i] = kDeck[i];
  return ctx;
}

}  // namespace

TEST(EndgameReplay1876746, ReactivePlayMarksWhiteFive) {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = yagami: hand[slot1..slot5] = [orig 4, 3, 2, 1, 0] = [b2, w1, r1, b3, w2].
      {"b2", "w1", "r1", "b3", "w2"},
      // Cathy = will-bot67: [orig 9, 8, 7, 6, 5] = [w2, b4, b3, r2, w4].
      {"w2", "b4", "b3", "r2", "w4"},
  };
  opts.variant_name = "Deceptive-Ones & White (3 Suits)";
  opts.starting = TestPlayer::BOB;  // orig P0 (yagami) starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  // After action 25 (P1's play of order 26 in orig coords), the next-up player
  // should be P2 (will-bot69) = Alice in our coords. Turn 27 in UI.
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE))
      << "expected Alice's turn (will-bot69)";

  // Slot 4 of Alice's hand should hold orig 14 = w5, marked CALLED_TO_PLAY.
  int w5_my_order = ctx.orig_to_my_order[14];
  ASSERT_EQ(w5_my_order, 4);
  EXPECT_EQ(g.meta[w5_my_order].status, CardStatus::CALLED_TO_PLAY)
      << "white-5 slot should be CALLED_TO_PLAY after the reactive sequence";

  // The inferred set should narrow to {w5}: Alice can see both b3s
  // (in Bob's slot 4 = my-order 6, and Cathy's slot 3 = my-order 12), so
  // any inference of b3 on her CALLED_TO_PLAY slot should be eliminated.
  IdentitySet inferred = g.players[0].thoughts[w5_my_order].inferred;
  Identity w5(2, 5);
  EXPECT_TRUE(inferred.contains(w5))
      << "inferred for CALLED_TO_PLAY slot must include w5";
  // Stricter assertion: by this point inferred should be exactly {w5}.
  EXPECT_EQ(inferred.length(), 1) << "inferred should have narrowed to {w5}";

  // take_action() must play the white-5.
  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "expected a Play action, got " << hanabi::to_json(perform, 0).dump();
  EXPECT_EQ(std::get<PerformPlay>(perform).target, w5_my_order)
      << "bot should play white-5 (my-order " << w5_my_order << "), got "
      << hanabi::to_json(perform, 0).dump();
}
