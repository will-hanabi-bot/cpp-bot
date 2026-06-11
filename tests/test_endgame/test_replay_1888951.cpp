// Hanab.live replay 1888951, variant "Scarce Ones & Gray (6 Suits)".
// Orig players: P0=yagami_black, P1=will-bot67, P2=will-bot69.
//
// At T29 the live will-bot67 (CP=P1=our ALICE) issued
// `PerformColour{target=yagami, value=yellow}`. That set up yagami's
// y4 for one stack advance but left a playable r3 stranded in
// will-bot69's hand; bot69 discarded the r3 on T30. The user's
// preferred move: `PerformRank{target=yagami, value=4}` — under the
// reactor convention this resolves as REACTIVE:
//   focus_slot = 2 (yagami's leftmost rank-4 = y4 at slot 2).
//   First play_target = yagami's a2 at slot 1 (playable_away=1 wait
//     actually currently playable since a-stack=1 and a2 is rank 2).
//   calc_slot(2, 1, 5) = 1 → react_order = bot69's slot 1 = r3.
// → bot69 plays r3 (reacter), yagami plays a2 (receiver). Two stack
// advances.
//
// Pre-fix the convention only stamped CTP on the *reacter's* slot, so
// `playables.size()` in get_result was 1, the +10 REACTIVE 2-play
// eval bonus didn't fire, and the rank-4 clue lost on tiebreakers.
// Post-fix the receiver's `target` also gets CTP'd; both cards land
// in hypo_plays and the bonus fires.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// Deck order matches hanab.live export verbatim (1888951.json). Suits:
// 0=Red, 1=Yellow, 2=Green, 3=Blue, 4=Purple, 5=Gray ("ash").
const std::vector<std::pair<int, int>> kDeck = {
    {2, 5}, {0, 4}, {0, 5}, {0, 4}, {3, 1},
    {4, 4}, {2, 2}, {0, 2}, {2, 2}, {1, 4},
    {2, 1}, {3, 5}, {1, 3}, {1, 5}, {3, 4},
    {0, 1}, {0, 1}, {0, 2}, {4, 5}, {2, 4},
    {5, 4}, {2, 3}, {1, 1}, {5, 3}, {4, 1},
    {4, 3}, {1, 3}, {5, 1}, {3, 1}, {2, 3},
    {1, 2}, {1, 4}, {0, 3}, {5, 2}, {1, 2},
    {2, 4}, {2, 1}, {4, 2}, {1, 1}, {0, 3},
    {3, 3}, {3, 2}, {3, 4}, {4, 3}, {3, 2},
    {4, 2}, {4, 4}, {5, 5}, {4, 1}, {3, 3},
};

// Actions 0..27 = turns 1..28. Action 28 (the bug turn) is NOT
// applied; the test inspects what take_action returns at T29.
const std::vector<OrigAction> kOrigActions = {
    {2, 2, 1},   // T1  P0 colour-1 (yellow) -> P2
    {1, 8, 0},   // T2  P1 discard orig 8 (g2)
    {0, 10, 0},  // T3  P2 play orig 10 (g1)
    {2, 2, 3},   // T4  P0 colour-3 (blue) -> P2
    {1, 15, 0},  // T5  P1 discard orig 15 (r1)
    {0, 16, 0},  // T6  P2 play orig 16 (r1)
    {2, 2, 4},   // T7  P0 colour-4 (purple) -> P2
    {0, 6, 0},   // T8  P1 play orig 6 (g2)
    {3, 1, 4},   // T9  P2 rank-4 -> P1
    {0, 4, 0},   // T10 P0 play orig 4 (b1)
    {0, 17, 0},  // T11 P1 play orig 17 (r2)
    {1, 14, 0},  // T12 P2 discard orig 14 (b4)
    {3, 2, 5},   // T13 P0 rank-5 -> P2
    {0, 21, 0},  // T14 P1 play orig 21 (g3)
    {0, 22, 0},  // T15 P2 play orig 22 (y1)
    {2, 2, 3},   // T16 P0 colour-3 (blue) -> P2
    {1, 7, 0},   // T17 P1 discard orig 7 (r2)
    {2, 1, 4},   // T18 P2 colour-4 (purple) -> P1
    {1, 3, 0},   // T19 P0 discard orig 3 (r4)
    {0, 19, 0},  // T20 P1 play orig 19 (g4)
    {3, 1, 1},   // T21 P2 rank-1 -> P1
    {0, 0, 0},   // T22 P0 play orig 0 (g5)
    {0, 27, 0},  // T23 P1 play orig 27 (a1)
    {0, 24, 0},  // T24 P2 play orig 24 (p1)
    {1, 28, 0},  // T25 P0 discard orig 28 (b1)
    {3, 0, 4},   // T26 P1 rank-4 -> P0
    {0, 30, 0},  // T27 P2 play orig 30 (y2)
    {0, 26, 0},  // T28 P0 play orig 26 (y3)
};

// Build from will-bot67's perspective (orig P1 = ALICE = observer).
// Player cycle: orig P0 -> P1 -> P2 -> P0. Observer is orig P1.
// In MY framework: ALICE = orig P1, BOB = orig P2 (next), CATHY =
// orig P0 (next after BOB). Starting player orig P0 = MY CATHY.
Game build_from_bot67_perspective() {
  SetupOptions opts;
  opts.hands = {
      // ALICE = bot67 (observer). orig 5..9 → MY 0..4, newest-first:
      // 9=(1,4)=y4, 8=(2,2)=g2, 7=(0,2)=r2, 6=(2,2)=g2, 5=(4,4)=p4.
      {"y4", "g2", "r2", "g2", "p4"},
      // BOB = will-bot69 (orig P2). orig 10..14 → MY 5..9.
      // 14=(3,4)=b4, 13=(1,5)=y5, 12=(1,3)=y3, 11=(3,5)=b5,
      // 10=(2,1)=g1.
      {"b4", "y5", "y3", "b5", "g1"},
      // CATHY = yagami (orig P0). orig 0..4 → MY 10..14.
      // 4=(3,1)=b1, 3=(0,4)=r4, 2=(0,5)=r5, 1=(0,4)=r4, 0=(2,5)=g5.
      {"b1", "r4", "r5", "r4", "g5"},
  };
  opts.variant_name = "Scarce Ones & Gray (6 Suits)";
  opts.starting = TestPlayer::CATHY;
  // ALICE's hidden hand is fully specified above (not "xx"), so the
  // card-count check passes.
  return setup(std::move(opts));
}

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // orig P0 -> CATHY, orig P1 -> ALICE, orig P2 -> BOB.
  ctx.orig_to_my_player = {2, 0, 1};
  // Card-order remapping:
  //  - orig 0..4   (orig P0)  -> MY 10..14 (CATHY)
  //  - orig 5..9   (orig P1)  -> MY 0..4   (ALICE)
  //  - orig 10..14 (orig P2)  -> MY 5..9   (BOB)
  //  - orig 15+               -> identity
  ctx.orig_to_my_order.resize(kDeck.size());
  for (int o = 0; o <= 4; ++o) ctx.orig_to_my_order[o] = o + 10;
  for (int o = 5; o <= 9; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 10; o <= 14; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 15; o < static_cast<int>(kDeck.size()); ++o)
    ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(kDeck.size());
  for (size_t orig_o = 0; orig_o < kDeck.size(); ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

}  // namespace

// At T29 (will-bot67's turn) the live bot picked colour-yellow → yagami
// (1 play). The fix should make the bot prefer rank-4 → yagami (2 plays
// via reactive interpretation). At minimum, the bot must NOT pick the
// 1-play colour-yellow clue that strands the r3 in bot69's hand.
TEST(EndgameReplay1888951, Turn29PrefersRank4OverColourYellow) {
  Game g = build_from_bot67_perspective();
  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  // Sanity-pin stacks at T29: r=2, y=3, g=5, b=1, p=1, a=1.
  ASSERT_EQ(g.state.play_stacks[0], 2);
  ASSERT_EQ(g.state.play_stacks[1], 3);
  ASSERT_EQ(g.state.play_stacks[2], 5);
  ASSERT_EQ(g.state.play_stacks[3], 1);
  ASSERT_EQ(g.state.play_stacks[4], 1);
  ASSERT_EQ(g.state.play_stacks[5], 1);

  PerformAction perform = g.take_action();
  if (std::holds_alternative<PerformColour>(perform)) {
    auto p = std::get<PerformColour>(perform);
    EXPECT_FALSE(p.target == static_cast<int>(TestPlayer::CATHY) && p.value == 1)
        << "regression: colour-yellow→yagami strands bot69's r3. "
           "The receiver-target CTP stamp + the v0.15 2-play bonus "
           "should make the bot prefer rank-4→yagami (2 plays).";
  }
}
