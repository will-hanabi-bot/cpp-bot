// Decision-quality tests for the v0.21 low-clue-count gate.
//
// Source: https://hanab.live/shared-replay/1889974 — 3-player Funnels &
// Dark Pink (6 Suits) game between will-bot69 (P0), yagami_black (P1),
// and will-bot67 (P2).
//
// Two cases where the live bot (pre-v0.21) burned a clue instead of
// playing a known-or-cued card under low clue tokens + high pace. The
// v0.21 gate in state_eval.cpp eval_action — when state.clue_tokens < 3
// and state.pace() >= 3, only "high value" clues survive scoring;
// otherwise the gate returns −1.0 so any known play (≥ 0.02) wins.
//
// These are decision-making tests (not correctness tests): a failure
// means the bot's choice deviates from the policy in CLAUDE.md, not that
// it played illegally. Per CLAUDE.md, breaking a test here needs review
// before the test is altered or removed.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// hanab.live deck for replay 1889974. Suit indices: 0=Red, 1=Yellow,
// 2=Green, 3=Blue, 4=Purple, 5=Dark Pink.
const std::vector<std::pair<int, int>> kDeck = {
    {4,4}, {2,1}, {0,3}, {1,1}, {1,4},
    {3,1}, {2,2}, {3,2}, {2,2}, {0,5},
    {2,5}, {0,2}, {2,4}, {2,3}, {5,4},
    {2,4}, {3,2}, {1,3}, {0,1}, {4,1},
    {1,4}, {4,3}, {2,3}, {4,3}, {0,4},
    {0,1}, {0,4}, {4,1}, {1,1}, {5,5},
    {4,5}, {4,2}, {3,1}, {3,3}, {5,2},
    {1,3}, {3,4}, {3,3}, {1,2}, {1,2},
    {3,1}, {5,1}, {1,1}, {0,1}, {5,3},
    {4,2}, {0,2}, {2,1}, {4,1}, {0,3},
    {4,4}, {3,5}, {1,5}, {2,1}, {3,4},
};

// All 43 actions; the tests below apply a prefix.
const std::vector<OrigAction> kOrigActions = {
    {2, 1,0}, // T1  P0 colour-r → P1
    {0, 5,0}, // T2  P1 plays order 5 (b1)
    {3, 1,3}, // T3  P2 rank-3 → P1
    {0, 1,0}, // T4  P0 plays order 1 (g1)
    {0, 7,0}, // T5  P1 plays order 7 (b2)
    {3, 1,2}, // T6  P2 rank-2 → P1
    {0, 3,0}, // T7  P0 plays order 3 (y1)
    {0, 8,0}, // T8  P1 plays order 8 (g2)
    {3, 1,2}, // T9  P2 rank-2 → P1
    {0,18,0}, // T10 P0 plays order 18 (r1)
    {3, 0,3}, // T11 P1 rank-3 → P0
    {0,11,0}, // T12 P2 plays order 11 (r2)
    {2, 1,2}, // T13 P0 colour-g → P1
    {0,19,0}, // T14 P1 plays order 19 (p1)
    {0,13,0}, // T15 P2 plays order 13 (g3)
    {0, 2,0}, // T16 P0 plays order 2 (r3)
    {0,15,0}, // T17 P1 plays order 15 (g4)
    {2, 0,1}, // T18 P2 colour-y → P0 — sets up T19
    {2, 2,4}, // T19 P0's live choice was colour-p → P2 (gated test asserts a play here)
    {1,22,0}, // T20
    {0,10,0}, // T21
    {0,24,0}, // T22
    {0, 9,0}, // T23
    {1,27,0}, // T24
    {1,28,0}, // T25
    {2, 0,3}, // T26
    {1,23,0}, // T27
    {0,31,0}, // T28
    {3, 0,4}, // T29
    {0,21,0}, // T30
    {0, 0,0}, // T31
    {2, 2,5}, // T32
    {0,30,0}, // T33
    {0,33,0}, // T34
    {2, 2,3}, // T35
    {0,36,0}, // T36
    {2, 2,3}, // T37
    {3, 0,3}, // T38
    {0,38,0}, // T39 P2 plays — sets up T40
    {2, 2,5}, // T40 P0's live choice was colour-I → P2 (gated test asserts a play here)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Identity mapping: orig P0/1/2 = test P0 (Alice) / P1 (Bob) / P2 (Cathy).
  ctx.orig_to_my_player = {0, 1, 2};
  const int N = static_cast<int>(kDeck.size());
  ctx.orig_to_my_order.resize(N);
  ctx.my_order_to_id.resize(N);
  for (int o = 0; o < N; ++o) {
    ctx.orig_to_my_order[o] = o;
    ctx.my_order_to_id[o] = kDeck[o];
  }
  return ctx;
}

// Build the starting Game. Hidden P0 (= our_player_index=0), visible
// P1/P2 with their orig dealt cards listed slot-1-first.
Game build_start() {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // P0 = will-bot69 (our POV).
      // P1 = yagami_black: orig orders [9,8,7,6,5] (slot 1 first).
      {"r5", "g2", "b2", "g2", "b1"},
      // P2 = will-bot67: orig orders [14,13,12,11,10] (slot 1 first).
      // Dark Pink short form is lowercase 'i' (catalog lowercases on load).
      {"i4", "g3", "g4", "r2", "g5"},
  };
  opts.variant_name = "Funnels & Dark Pink (6 Suits)";
  opts.starting = TestPlayer::ALICE;  // orig P0 starts.
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kOrigActions[i], ctx);
}

std::string describe(const PerformAction& a) {
  if (std::holds_alternative<PerformPlay>(a)) {
    return "PerformPlay{order=" + std::to_string(std::get<PerformPlay>(a).target) + "}";
  }
  if (std::holds_alternative<PerformDiscard>(a)) {
    return "PerformDiscard{order=" + std::to_string(std::get<PerformDiscard>(a).target) + "}";
  }
  if (std::holds_alternative<PerformColour>(a)) {
    const auto& c = std::get<PerformColour>(a);
    return "PerformColour{target=" + std::to_string(c.target) +
           ", value=" + std::to_string(c.value) + "}";
  }
  if (std::holds_alternative<PerformRank>(a)) {
    const auto& r = std::get<PerformRank>(a);
    return "PerformRank{target=" + std::to_string(r.target) +
           ", value=" + std::to_string(r.value) + "}";
  }
  return "PerformTerminate";
}

}  // namespace

// Turn 19: clue_tokens=1, pace=13 (high). Bob (yagami) chop = g2 (trash on
// G=4 stack). Cathy (will-bot67) chop = g5 (critical). Under the v0.21
// gate this rejected because Bob was safe (trash chop), Cathy's chop
// was good, BUT the clue didn't induce 2+ plays (p3 is delayed).
//
// v0.26 refined gate: condition (c) accepts the clue when Bob is safe
// AND Cathy has a non-trash chop AND the clue gets ≥ 1 play (not 2+).
// This case satisfies (c) — Cathy's g5 is non-trash, the purple clue
// CTPs p3 (≥ 1 play). So the new gate ALLOWS the purple clue. The test
// is disabled because it pins the OLD spec; the user has explicitly
// changed the spec for v0.26.
TEST(LowClueCountGate, DISABLED_Turn19PrefersPlayOverPurpleClue) {
  Game g = build_start();
  apply_prefix(g, 18);  // T1..T18 applied; T19 = bot's decision.

  ASSERT_EQ(g.state.current_player_index, 0)
      << "T19 should be will-bot69 (P0)'s turn";
  EXPECT_LT(g.state.clue_tokens, 3)
      << "guard: setup must reach low-clue-count state";
  EXPECT_GE(g.state.pace(), 3)
      << "guard: setup must reach high-pace state";

  PerformAction action = g.take_action();
  EXPECT_FALSE(is_clue(action))
      << "T19: bot should play, not burn a clue; got " << describe(action);
}

// Turn 40: clue_tokens=1, pace=9 (high). Bob chop = g2 (trash). Cathy chop
// = g4 (trash, green=5). Bob safe AND Cathy chop is *not* "good" → gate
// hard-rejects every clue, leaving plays/discards as the only options.
// The live bot clued Dark Pink to Cathy; we expect a play.
TEST(LowClueCountGate, Turn40PrefersPlayOverPinkClue) {
  Game g = build_start();
  apply_prefix(g, 39);  // T1..T39 applied; T40 = bot's decision.

  ASSERT_EQ(g.state.current_player_index, 0)
      << "T40 should be will-bot69 (P0)'s turn";
  EXPECT_LT(g.state.clue_tokens, 3);
  EXPECT_GE(g.state.pace(), 3);

  PerformAction action = g.take_action();
  EXPECT_FALSE(is_clue(action))
      << "T40: bot should play, not burn a clue; got " << describe(action);
}

// v0.26 regression for the refined spec: replay 1892397 T24.
// will-bot67 (giver) has 1 clue token, pace high. Stacks r=2 y=0 g=2
// b=3 p=4 i=0. Hands at T24:
//   Bob = will-bot69: r1 p3 y5 g5 b1 (slot 5 = b1 = trash).
//   Cathy = yagami:   p4 y1 i4 r4 y4 (slot 5 = y4 = non-trash).
// will-bot67 hand: y3 r5 y3 i2 b4.
//
// The blue clue to yagami touches NOTHING (no blue in yagami's hand).
// So it can't get 2+ new plays (b) and likely 0 plays anyway. Per the
// v0.26 spec:
//   (a) Bob trash chop b1 ⇒ Bob safe (not in danger). FAIL.
//   (b) 0 new plays. FAIL.
//   (c) Bob safe + Cathy non-trash chop. But 0 plays → FAIL.
// → Low value. The gate should reject; bot picks something else.
//
// Sourced from https://hanab.live/shared-replay/1892397#24.
TEST(LowClueCountGate, Turn24RejectsLowValueBlueClue) {
  // Setup directly via SetupOptions — simpler than full replay walking
  // since we just need the state at T24.
  SetupOptions opts;
  opts.hands = {
      // P0 = will-bot67 (our POV, the giver of T24). Hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // P1 = will-bot69 (Bob). Slot-1-first: r1, p3, y5, g5, b1.
      {"r1", "p3", "y5", "g5", "b1"},
      // P2 = yagami (Cathy). Slot-1-first: p4, y1, i4, r4, y4.
      {"p4", "y1", "i4", "r4", "y4"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{2, 0, 2, 3, 4, 0};
  opts.clue_tokens = 1;
  Game g = setup(std::move(opts));

  ASSERT_LT(g.state.clue_tokens, 3) << "guard: low clue count";
  ASSERT_GE(g.state.pace(), 3) << "guard: high pace";

  PerformAction action = g.take_action();
  // The bad clue would be PerformColour{target=2 (yagami), value=3 (blue)}.
  bool is_bad_blue = std::holds_alternative<PerformColour>(action) &&
                     std::get<PerformColour>(action).target == 2 &&
                     std::get<PerformColour>(action).value == 3;
  EXPECT_FALSE(is_bad_blue)
      << "T24: bot must not pick the low-value blue→yagami clue "
         "(touches nothing, no Cathy non-trash chop benefit). Got "
      << describe(action);
}
