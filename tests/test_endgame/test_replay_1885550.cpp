// Hanab.live replay 1885550, variant "Orange (3 Suits)". Orig players:
// P0=yagami_black, P1=will-bot69, P2=will-bot67.
//
// Background. At turn 6 (action[5]) the live will-bot67 (P2 = our
// ALICE) PerformDiscard'd order 16 — a rank-1-clued card. The user
// reported the bot did this "without knowing whether or not it is
// orange". Under the orange game-rule, PerformDiscard on a possibly-o1
// card with orange stack ≥ 1 risks a misplay strike.
//
// What current code does. In the latest build the bot's `inferred` for
// order 16 narrows to a singleton b1 (basic_trash on blue stack 1), so
// PerformDiscard{16} no longer carries an orange-strike risk from the
// bot's POV — the orange-trash safety filter added in this change is a
// no-op for this card. The actual card is r1 (also basic_trash on red
// stack 1) so the live discard was clean.
//
// This test pins the position at turn 6: stacks (1, 1, 1), ALICE's
// slot 1 = order 16, empathy-trash. It documents that the bot's chosen
// action — even if PerformDiscard{16} — is safe under the bot's narrow
// `possible` set, and (more importantly) that if a future change ever
// broadens `possible` to include o1 again, the orange-trash filter
// must keep PerformDiscard{16} off the table. The general orange-trash
// safety rule itself is covered by the synthetic unit test
// `Orange.AvoidPerformDiscardOnMultiIdOrangePossibleTrash` in
// tests/test_reactor/test_orange.cpp.
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

// Deck order matches hanab.live export verbatim (1885550.json).
const std::vector<std::pair<int, int>> kDeck = {
    {0, 2}, {0, 4}, {1, 5}, {2, 5}, {0, 1},
    {0, 1}, {1, 1}, {2, 1}, {2, 3}, {1, 4},
    {2, 4}, {2, 4}, {1, 1}, {1, 4}, {2, 2},
    {1, 3}, {0, 1}, {2, 1}, {2, 3}, {0, 3},
    {1, 2}, {2, 1}, {0, 2}, {1, 1}, {2, 2},
    {0, 4}, {1, 2}, {0, 3}, {1, 3}, {0, 5},
};

// Actions 0..4 = turns 1..5. Action 5 (the bug turn) is NOT applied; the
// test inspects what take_action() returns instead.
const std::vector<OrigAction> kOrigActions = {
    {3, 2, 1},  // T1  P0 (yagami)    -> P2 (bot67) rank-1
    {0, 5, 0},  // T2  P1 (bot69)     plays orig 5 (r1)
    {0, 12, 0}, // T3  P2 (bot67)     plays orig 12 (b1)
    {3, 2, 1},  // T4  P0 (yagami)    -> P2 (bot67) rank-1 (again)
    {0, 7, 0},  // T5  P1 (bot69)     plays orig 7 (o1) — orange inversion
                //                    advances orange stack 0->1
};

// Build the game from will-bot67's perspective (orig P2 = ALICE). Player
// cycle: orig P0 -> orig P1 -> orig P2 -> orig P0 ... In MY view, orig P2
// = ALICE, so my BOB (next in cycle) = orig P0 (yagami), my CATHY = orig
// P1 (bot69). Original starting player = orig P0 = MY BOB.
Game build_from_bot67_perspective() {
  SetupOptions opts;
  opts.hands = {
      // ALICE = bot67 (observer, hidden hand).
      // Initial = orig orders 10..14 (= MY orders 0..4), newest-first.
      // 14=(2,2)=o2, 13=(1,4)=b4, 12=(1,1)=b1, 11=(2,4)=o4, 10=(2,4)=o4.
      {"xx", "xx", "xx", "xx", "xx"},
      // BOB = yagami. Initial = orig orders 0..4 (= MY orders 5..9),
      // newest-first.
      // 9=(0,1)->r1, 8=(2,5)->o5, 7=(1,5)->b5, 6=(0,4)->r4, 5=(0,2)->r2.
      // (orig 0..4: (0,2)=r2, (0,4)=r4, (1,5)=b5, (2,5)=o5, (0,1)=r1.)
      {"r1", "o5", "b5", "r4", "r2"},
      // CATHY = bot69. Initial = orig orders 5..9 (= MY orders 10..14),
      // newest-first.
      // orig 5..9: (0,1)=r1, (0,1)=r1, (1,1)=b1, (2,1)=o1, (2,3)=o3.
      // MY 14=orig 9=o3, 13=orig 8=o1, 12=orig 7=b1, 11=orig 6=r1, 10=orig 5=r1.
      {"o3", "o1", "b1", "r1", "r1"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::BOB;
  return setup(std::move(opts));
}

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // orig P0 -> BOB, orig P1 -> CATHY, orig P2 -> ALICE.
  ctx.orig_to_my_player = {1, 2, 0};
  // Card-order remapping:
  //  - orig 0..4   (orig P0's hand)  -> my 5..9   (BOB's slots)
  //  - orig 5..9   (orig P1's hand)  -> my 10..14 (CATHY's slots)
  //  - orig 10..14 (orig P2's hand)  -> my 0..4   (ALICE's slots)
  //  - orig 15+    (deck draws)      -> identity
  ctx.orig_to_my_order.resize(kDeck.size());
  for (int o = 0; o <= 4; ++o) ctx.orig_to_my_order[o] = o + 5;
  for (int o = 5; o <= 9; ++o) ctx.orig_to_my_order[o] = o + 5;
  for (int o = 10; o <= 14; ++o) ctx.orig_to_my_order[o] = o - 10;
  for (int o = 15; o < static_cast<int>(kDeck.size()); ++o)
    ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(kDeck.size());
  for (size_t orig_o = 0; orig_o < kDeck.size(); ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

}  // namespace

// Pin the turn-6 position so future changes to convention elim or the
// take_action discard pipeline surface here. The invariant the orange-
// trash safety filter enforces: if ALICE's slot 1 (= order 16) ever
// regresses to a `possible` set that still includes orange while
// being empathy-trash, the bot must NOT dispatch PerformDiscard{16}.
TEST(EndgameReplay1885550, Turn6OrangeTrashFilterInvariant) {
  Game g = build_from_bot67_perspective();
  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  // Pin the position: it's ALICE's turn and stacks are (1, 1, 1).
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  ASSERT_EQ(g.state.play_stacks[0], 1);  // red
  ASSERT_EQ(g.state.play_stacks[1], 1);  // blue
  ASSERT_EQ(g.state.play_stacks[2], 1);  // orange

  // Slot 1 (newest) = my order 16 = the rank-1-clued r1 drawn after T3.
  int alice_slot1 = g.state.hands[static_cast<int>(TestPlayer::ALICE)][0];
  ASSERT_EQ(alice_slot1, 16);
  ASSERT_TRUE(g.me().order_trash(g, alice_slot1));

  bool has_orange_in_possible = false;
  for (Identity i : g.me().thoughts[alice_slot1].possible) {
    if (g.state.variant->suits[i.suit_index].suit_type.inverted) {
      has_orange_in_possible = true;
      break;
    }
  }
  bool is_ctd = g.meta[alice_slot1].status == CardStatus::CALLED_TO_DISCARD;
  bool has_singleton_inferred =
      g.me().thoughts[alice_slot1].id(/*infer=*/true).has_value();

  PerformAction perform = g.take_action();
  if (has_orange_in_possible && !is_ctd && !has_singleton_inferred) {
    // Filter precondition met: order 16 is empathy-trash AND its
    // possible includes orange AND no escape hatch (CTD / singleton).
    // The bot must avoid PerformDiscard{16} under the orange game-rule.
    if (std::holds_alternative<PerformDiscard>(perform)) {
      EXPECT_NE(std::get<PerformDiscard>(perform).target, alice_slot1)
          << "regression: orange-trash safety filter must keep order 16 "
             "off the PerformDiscard list when possible still includes o1";
    }
  }
  // Otherwise: the bot's view has narrowed past the filter precondition
  // (e.g. inferred singleton non-orange) — PerformDiscard{16} is safe
  // and the filter is correctly a no-op for this card.
}
