// Hanab.live replay 1889209, variant "Funnels & Cocoa Rainbow (6 Suits)".
// Orig players: P0=will-bot67, P1=yagami_black, P2=will-bot69.
//
// T4 (P0=bot67): colour-red → P2=bot69. Under the reactor convention
// this is REACTIVE with reacter = yagami (the middle player). Yagami
// is expected to react (discard slot 1).
//
// T5 (P1=yagami): instead of reacting, yagami DEFERS by giving rank-3
// → P0=bot67. Pre-fix the dispatch's standard heuristic landed on
// `*reacter == action.target` and ran `interpret_stable`. Per the
// user's rule, when the previous reactive didn't resolve (because the
// reacter deferred), the new clue must be REACTIVE — the chain carries
// forward.
//
// The fix in `Game::interpret_clue` captures a `was_deferring` flag
// before clearing `waiting` and adds an early dispatch branch that
// forces `interpret_reactive(reacter = next-after-giver)`.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/interp.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// Deck order matches hanab.live export verbatim (1889209.json).
// Suits: 0=Red, 1=Yellow, 2=Green, 3=Blue, 4=Purple, 5=Cocoa Rainbow.
const std::vector<std::pair<int, int>> kDeck = {
    {3, 4}, {5, 4}, {1, 1}, {0, 1}, {2, 4},
    {2, 2}, {4, 4}, {1, 4}, {3, 1}, {1, 3},
    {0, 2}, {2, 2}, {0, 5}, {0, 1}, {2, 3},
    {2, 4}, {1, 5}, {3, 1}, {0, 1}, {5, 3},
    {5, 5}, {4, 4}, {3, 3}, {5, 1}, {3, 4},
};

// Actions 0..3 = turns 1..4. Action 4 (T5 = the deferral) is applied
// inside the test so we can inspect `last_move()`.
const std::vector<OrigAction> kOrigActions0To3 = {
    {3, 2, 4},   // T1 P0 rank-4 → P2
    {0, 8, 0},   // T2 P1 play orig 8 (b1)
    {0, 13, 0},  // T3 P2 play orig 13 (r1)
    {2, 2, 0},   // T4 P0 colour-red → P2 (the reactive)
};

// Build from will-bot67's perspective (orig P0 = ALICE = observer).
// Player cycle is identity: orig P0→ALICE, P1→BOB, P2→CATHY. Starting
// = orig P0 = MY ALICE.
Game build_from_bot67_perspective() {
  SetupOptions opts;
  opts.hands = {
      // ALICE = bot67 (observer). orig 0..4 → MY 0..4, newest-first.
      // Substitute non-Cocoa-Rainbow cards for ALICE's hidden hand so
      // the short_form parser doesn't trip on Cocoa Rainbow's 'M'.
      // ALICE's actual identities don't affect the test outcome (her
      // hand is hidden from her POV anyway), only card-count totals.
      {"g4", "r1", "y1", "p2", "b4"},
      // BOB = yagami. orig 5..9 → MY 5..9 newest-first:
      // 9=(1,3)=y3, 8=(3,1)=b1, 7=(1,4)=y4, 6=(4,4)=p4, 5=(2,2)=g2.
      {"y3", "b1", "y4", "p4", "g2"},
      // CATHY = bot69. orig 10..14 → MY 10..14 newest-first:
      // 14=(2,3)=g3, 13=(0,1)=r1, 12=(0,5)=r5, 11=(2,2)=g2, 10=(0,2)=r2.
      {"g3", "r1", "r5", "g2", "r2"},
  };
  opts.variant_name = "Funnels & Cocoa Rainbow (6 Suits)";
  opts.starting = TestPlayer::ALICE;
  return setup(std::move(opts));
}

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  ctx.orig_to_my_player = {0, 1, 2};
  ctx.orig_to_my_order.resize(kDeck.size());
  for (int o = 0; o < static_cast<int>(kDeck.size()); ++o)
    ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(kDeck.size());
  for (size_t orig_o = 0; orig_o < kDeck.size(); ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

}  // namespace

TEST(EndgameReplay1889209, Turn5DeferredClueIsReactive) {
  Game g = build_from_bot67_perspective();
  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions0To3) apply_orig_action(g, a, ctx);

  // After T4 the reactive clue should have pushed a ReactorWC with
  // reacter = MY BOB (= yagami). Sanity-pin the deferral precondition.
  ASSERT_FALSE(g.waiting.empty())
      << "T4 colour-red must interpret as REACTIVE and push a ReactorWC";
  EXPECT_EQ(g.waiting.front().reacter, static_cast<int>(TestPlayer::BOB))
      << "T4's reacter is yagami (= MY BOB)";

  // Apply T5: yagami clues rank-3 to bot67 (= MY ALICE). Pre-fix this
  // interprets as STABLE; post-fix as REACTIVE.
  OrigAction t5{3, 0, 3};
  apply_orig_action(g, t5, ctx);

  ASSERT_FALSE(g.move_history.empty());
  ASSERT_TRUE(std::holds_alternative<ClueInterp>(g.move_history.back()));
  // ClueInterp::REACTIVE == 1 per include/hanabi/basics/interp.h.
  EXPECT_EQ(static_cast<int>(std::get<ClueInterp>(g.move_history.back())), 1)
      << "T5 must interpret as REACTIVE: yagami (= prev reacter) deferred "
         "by giving a clue, and the reactive chain continues into the new "
         "clue per the deferral rule.";
}
