// Hanab.live replay 1885749, variant "Orange (3 Suits)". Orig players:
// P0=will-bot69, P1=yagami_black, P2=will-bot67.
//
// At turn 21 (action[20]) will-bot67 (P2 = our ALICE) gave a colour-blue
// clue to yagami (P1). Under the reactor convention this was a reactive
// colour clue whose orange inversion forced yagami to PerformPlay an
// unclued card on turn 22 (action[21]) — the card was o5 (orig order 22),
// the critical orange-5. For an inverted suit on_play sends the card to
// the discard pile, so the o5 was lost and the game ended with
// endCondition=4 (no more progress possible).
//
// Root cause on the eval side: in orange variants the bot was scoring
// every chop/unknown discard at -1.5 / -0.5, so the colour-blue clue's
// positive eval (touched cards, fill, elim) beat the chop discard even
// though the clue would force a critical-orange misplay. The orange-
// aware bump in `eval_action` floors chop / possibly-orange unknowns
// at -0.25 and scores known-orange-playable discards at the play tier,
// so the bot now prefers a safe discard over the catastrophic clue.
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

// Deck order matches hanab.live export verbatim (1885749.json).
const std::vector<std::pair<int, int>> kDeck = {
    {1, 2}, {0, 5}, {2, 1}, {0, 1}, {0, 3},
    {0, 1}, {1, 5}, {1, 3}, {1, 1}, {0, 2},
    {2, 3}, {2, 4}, {1, 4}, {1, 1}, {2, 2},
    {1, 4}, {1, 2}, {0, 4}, {1, 1}, {0, 3},
    {0, 2}, {2, 1}, {2, 5}, {2, 3}, {1, 3},
    {2, 4}, {0, 1}, {2, 1}, {0, 4}, {2, 2},
};

// Actions 0..19 = turns 1..20. Action 20 (the bug turn) is NOT applied;
// the test inspects what take_action() returns at turn 21.
const std::vector<OrigAction> kOrigActions = {
    {3, 2, 1},   // T1  P0 rank-1 to P2
    {0, 5, 0},   // T2  P1 plays orig 5 (r1)
    {0, 13, 0},  // T3  P2 plays orig 13 (b1)
    {3, 2, 4},   // T4  P0 rank-4 to P2
    {0, 9, 0},   // T5  P1 plays orig 9 (r2)
    {3, 1, 4},   // T6  P2 rank-4 to P1
    {0, 4, 0},   // T7  P0 plays orig 4 (r3)
    {3, 2, 3},   // T8  P1 rank-3 to P2
    {0, 16, 0},  // T9  P2 plays orig 16 (b2)
    {0, 2, 0},   // T10 P0 plays orig 2 (o1) — orange inversion: PerformDiscard
    {0, 17, 0},  // T11 P1 plays orig 17 (r4)
    {3, 1, 1},   // T12 P2 rank-1 to P1
    {0, 1, 0},   // T13 P0 plays orig 1 (r5)
    {0, 7, 0},   // T14 P1 plays orig 7 (b3)
    {2, 1, 1},   // T15 P2 colour-blue to P1
    {1, 0, 0},   // T16 P0 discards orig 0 (b2)
    {3, 0, 2},   // T17 P1 rank-2 to P0
    {0, 14, 0},  // T18 P2 plays orig 14 (o2) — orange inversion
    {1, 24, 0},  // T19 P0 discards orig 24 (b3)
    {2, 2, 2},   // T20 P1 colour-orange to P2
};

// Build the game from will-bot67's perspective (orig P2 = ALICE).
// Player cycle: orig P0 -> P1 -> P2 -> P0. Observer = P2 (ALICE), so
// my BOB (next in cycle) = orig P0 (will-bot69) and my CATHY = orig P1
// (yagami). Original starting player = orig P0 = MY BOB.
Game build_from_bot67_perspective() {
  SetupOptions opts;
  opts.hands = {
      // ALICE = bot67 (observer, hidden hand).
      // Initial = orig orders 10..14 (= MY orders 0..4), newest-first.
      // 14=(2,2)=o2, 13=(1,1)=b1, 12=(1,4)=b4, 11=(2,4)=o4, 10=(2,3)=o3.
      {"xx", "xx", "xx", "xx", "xx"},
      // BOB = will-bot69. Initial = orig orders 0..4 (= MY orders 5..9),
      // newest-first.
      // 9=orig4=(0,3)=r3, 8=orig3=(0,1)=r1, 7=orig2=(2,1)=o1,
      // 6=orig1=(0,5)=r5, 5=orig0=(1,2)=b2.
      {"r3", "r1", "o1", "r5", "b2"},
      // CATHY = yagami. Initial = orig orders 5..9 (= MY orders 10..14),
      // newest-first.
      // 14=orig9=(0,2)=r2, 13=orig8=(1,1)=b1, 12=orig7=(1,3)=b3,
      // 11=orig6=(1,5)=b5, 10=orig5=(0,1)=r1.
      {"r2", "b1", "b3", "b5", "r1"},
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
  //  - orig 0..4   (orig P0)  -> my 5..9   (BOB)
  //  - orig 5..9   (orig P1)  -> my 10..14 (CATHY)
  //  - orig 10..14 (orig P2)  -> my 0..4   (ALICE)
  //  - orig 15+               -> identity
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

// Pin the post-fix behaviour at turn 21. The live bot picked
// PerformColour{target=yagami, value=blue}, which (under the orange
// inversion of the reactive interpretation) forced yagami to PerformPlay
// the o5 on turn 22 and ended the game. With the orange-aware discard
// eval bump, the bot must pick something else here — almost certainly
// a discard, but at minimum NOT the colour-blue-to-yagami clue.
TEST(EndgameReplay1885749, Turn21AvoidsBlueToYagamiClue) {
  Game g = build_from_bot67_perspective();
  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  // Sanity: stacks should be red 5, blue 3, orange 2 at turn 21.
  ASSERT_EQ(g.state.play_stacks[0], 5);
  ASSERT_EQ(g.state.play_stacks[1], 3);
  ASSERT_EQ(g.state.play_stacks[2], 2);

  PerformAction perform = g.take_action();
  if (std::holds_alternative<PerformColour>(perform)) {
    auto p = std::get<PerformColour>(perform);
    EXPECT_FALSE(p.target == static_cast<int>(TestPlayer::CATHY) && p.value == 1)
        << "regression: the colour-blue-to-yagami clue forces a critical "
           "o5 misplay on turn 22 — the orange-aware discard eval bump "
           "must rank a safe discard above this clue";
  }
}
