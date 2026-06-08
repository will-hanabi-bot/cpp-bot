// Hanab.live replay 1883728. Variant "Orange (3 Suits)". Players:
// orig P0=yagami_black, P1=will-bot69, P2=will-bot67. Setup observer to
// will-bot67 (the receiver) so we can verify the convention chain reaches
// the right CTP/CTD on his b1.
//
// Action 0: yagami (P0) clues blue to will-bot67 (P2). The reactive
// play_target is b1 (P2's slot 1, leftmost playable touched), so
// target_discard hits will-bot69's slot 2 = o1.
// Action 1: will-bot69 PerformDiscards o1 (orange game-rule: with_play,
// orange stack 0→1). react_discard fires: COLOR + reacter-discard →
// target_i_play on the receiver's slot 1 (b1). b1 should be CTP with
// inferred = {(B,1)}.
//
// The user reports that b1 is being noted as "[kt]" (called-to-discard)
// instead of "[f] b1" (called-to-play with definite b1 inference) at
// turn 2 — this test pins down what the bot actually thinks.
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
    {2, 2}, {1, 4}, {0, 1}, {1, 3}, {0, 4},
    {0, 1}, {1, 1}, {2, 3}, {2, 1}, {1, 2},
    {0, 1}, {2, 4}, {1, 4}, {2, 1}, {1, 1},
    {0, 4}, {0, 2}, {1, 2}, {2, 4}, {0, 5},
    {0, 3}, {1, 1}, {2, 3}, {1, 5}, {0, 2},
    {2, 2}, {2, 5}, {1, 3}, {0, 3}, {2, 1},
};

const std::vector<OrigAction> kOrigActions = {
    {2, 2, 1},  // 0: P0 → P2 color 1 (Blue)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Observer = yagami_black = orig P0 → our ALICE (identity mapping).
  ctx.orig_to_my_player = {0, 1, 2};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int i = 0; i < 30; ++i) ctx.my_order_to_id[i] = kDeck[i];
  return ctx;
}

Game build_through_action_1() {
  SetupOptions opts;
  opts.hands = {
      // Alice = yagami_black (P0, observer + giver at action 0): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot69 (P1): orig [9,8,7,6,5] = [b2,o1,o3,b1,r1].
      {"b2", "o1", "o3", "b1", "r1"},
      // Cathy = will-bot67 (P2): orig [14,13,12,11,10] = [b1,o1,b4,o4,r1].
      {"b1", "o1", "b4", "o4", "r1"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

// Note: this test currently captures what the bot does — if the [kt]
// note is indeed coming out of the convention layer we want it to fail
// so we can see the fix needed.
TEST(EndgameReplay1883728, ReactiveBlueGivesCTPNotCTDOnB1) {
  Game g = build_through_action_1();

  // After action 0, the convention has run interpret_reactive_colour on
  // yagami's blue clue to will-bot67. Inspect meta on the relevant cards.
  // Bob (will-bot69) is the reacter. The reactive's play-target candidates
  // in Cathy (will-bot67)'s hand sorted leftmost-first are:
  //   b1 (slot 1, order 14), o1 (slot 2, order 13), r1 (slot 5, order 10).
  //
  // What we'd expect: the convention picks b1 (leftmost playable) as
  // the play_target → react_slot = calc_slot(focus=3, 1, 5) = 2 → Bob's
  // slot 2 = o1. Since b1 isn't on the inverted suit, no receiver-orange
  // swap. Standard COLOR + play-target → target_discard on Bob's o1. The
  // would_lose_inverted_reacter gate then checks: react_order o1 is
  // orange. final_is_target_play=false. is_playable(o1) = true → NOT lose.
  // So target_discard runs. Bob's o1 is CTD.
  int bob_o1 = g.state.hands[static_cast<int>(TestPlayer::BOB)][1];
  int cathy_b1 = g.state.hands[static_cast<int>(TestPlayer::CATHY)][0];
  int cathy_o1 = g.state.hands[static_cast<int>(TestPlayer::CATHY)][1];
  int cathy_r1 = g.state.hands[static_cast<int>(TestPlayer::CATHY)][4];
  EXPECT_EQ(bob_o1, 8);
  EXPECT_EQ(cathy_b1, 14);

  auto print_meta = [&](const char* label, int o) {
    std::cerr << "[probe] " << label << " (order " << o
              << ") status=" << static_cast<int>(g.meta[o].status)
              << " trash=" << g.meta[o].trash
              << " inferred={";
    bool first = true;
    for (Identity i : g.common.thoughts[o].inferred) {
      if (!first) std::cerr << ", ";
      std::cerr << "(" << static_cast<int>(i.suit_index) << ","
                << static_cast<int>(i.rank) << ")";
      first = false;
    }
    std::cerr << "}\n";
  };
  print_meta("Bob.slot2 = o1", bob_o1);
  print_meta("Cathy.slot1 = b1", cathy_b1);
  print_meta("Cathy.slot2 = o1", cathy_o1);
  print_meta("Cathy.slot5 = r1", cathy_r1);

  EXPECT_EQ(g.meta[bob_o1].status, CardStatus::CALLED_TO_DISCARD)
      << "Bob's o1 should be CTD via the convention chain";

  // Now apply Bob's PerformDiscard{o1} (his urgent CTD action) and verify
  // the convention chain resolves: orange stack advances (orange game-rule
  // inversion plays the orange via the physical discard), and Cathy's b1
  // is marked CTP via react_discard + COLOR → target_i_play.
  Game g_after = g;  // copy
  g_after.handle_action(DiscardAction{static_cast<int>(TestPlayer::BOB),
                                          bob_o1, /*suit=*/2, /*rank=*/1,
                                          /*failed=*/false});
  // Draw next card for Bob (order 15 = next_card_order).
  g_after.handle_action(DrawAction{static_cast<int>(TestPlayer::BOB),
                                       g_after.state.next_card_order,
                                       /*suit=*/0, /*rank=*/4});
  g_after.handle_action(TurnAction{
      g_after.state.turn_count,
      g_after.state.next_player_index(static_cast<int>(TestPlayer::BOB))});

  std::cerr << "[probe] after Bob discards o1: orange stack="
            << g_after.state.play_stacks[2] << "\n";
  std::cerr << "[probe] Cathy.b1 (post-discard) status="
            << static_cast<int>(g_after.meta[cathy_b1].status)
            << " inferred={";
  bool first = true;
  for (Identity i : g_after.common.thoughts[cathy_b1].inferred) {
    if (!first) std::cerr << ", ";
    std::cerr << "(" << static_cast<int>(i.suit_index) << ","
              << static_cast<int>(i.rank) << ")";
    first = false;
  }
  std::cerr << "}\n";

  EXPECT_EQ(g_after.state.play_stacks[2], 1)
      << "Orange stack should advance to 1 via the inverted discard";
  EXPECT_EQ(g_after.meta[cathy_b1].status, CardStatus::CALLED_TO_PLAY)
      << "Cathy's b1 should be CTP (note '[f] b1') after Bob's PerformDiscard";
  EXPECT_TRUE(g_after.common.thoughts[cathy_b1].inferred.contains(Identity{1, 1}))
      << "b1 inferred should include (B,1)";

  // Also verify will-bot69's own take_action would issue PerformDiscard
  // (not PerformPlay) for its CTD on o1 — that's the action that produces
  // the right downstream convention chain.
  Game g_bob_pov = g;
  g_bob_pov.state.our_player_index = static_cast<int>(TestPlayer::BOB);
  PerformAction perform_bob = g_bob_pov.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformDiscard>(perform_bob))
      << "Bob's take_action should issue PerformDiscard for the CTD on o1 "
         "(the orange game-rule then advances the orange stack via the "
         "physical discard)";
  EXPECT_EQ(std::get<PerformDiscard>(perform_bob).target, bob_o1);

  // And at the next turn, will-bot67 (Cathy) sees b1 CTP and plays it —
  // the "Turn 3: clue of 1" the user reported never happens because the
  // bot has a forced play, not a free turn.
  Game g_cathy_pov = g_after;
  g_cathy_pov.state.our_player_index = static_cast<int>(TestPlayer::CATHY);
  PerformAction perform_cathy = g_cathy_pov.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform_cathy))
      << "Cathy's take_action should issue PerformPlay for the CTP on b1";
  EXPECT_EQ(std::get<PerformPlay>(perform_cathy).target, cathy_b1);
}
