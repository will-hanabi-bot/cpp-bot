// Diagnostic + regression test for replay 1899044 T17 — user-reported
// stable-clue convention bug.
//
// Variant: Scarce Ones & Dark Null (6 Suits). Players (orig order):
// P0=will-bot69, P1=will-bot67, P2=yagami_black. Suit short forms:
// r y g b p u (Dark Null = u).
//
// State at T16 (will-bot69's turn, about to give the bug clue):
//   Stacks: r=1, y=2, g=0, b=3, p=0, u=0. ct=6 (clue-tokens before
//   the rank-3 clue), strikes=0, score=6.
//   will-bot67 hand (slot 1=newest): y2(s1), g3(s2), p2(s3, clued
//   suit-4 at T10), b1(s4), r3(s5, clued suit-0 at T4).
//
// T16 action: will-bot69 clues RANK 3 → will-bot67. The clue newly
// touches slot 2 (g3, ord 20 in orig numbering) and re-touches the
// already-clued slot 5 (r3, ord 6).
//
// Per Reactor's stable-rank convention this is a referential discard
// on the first unclued slot after the focus. focus = slot 2 (the
// newly-touched newest), so target_index loop skips slot 3 (clued
// suit-4) and lands on slot 4 (b1, unclued). will-bot67 should
// therefore discard slot 4 at T17.
//
// User report: will-bot67 unexpectedly discards slot 1 (y2) at T17.
//
// `T17DumpAndTakeAction` is the diagnostic: it prints the post-T16
// state of every order in will-bot67's hand (status, urgent, clued
// flag, inferred set) and the take_action() return, so the bot's
// actual interpretation can be compared against the hand-trace in
// the plan file.
//
// `T17ShouldDiscardSlot4` is the regression guard. Currently
// **pinned to the buggy behaviour** — EXPECT_TRUE(is_discard_slot_1)
// — so the suite stays green until the convention fix lands. Flip
// it to EXPECT_TRUE(is_discard_slot_4) once the fix is in.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// Deck from hanab.live export (50 cards). (suit_index, rank).
const std::vector<std::pair<int, int>> kDeck = {
    {5,2}, {4,5}, {1,4}, {4,4}, {0,1},
    {3,3}, {0,3}, {0,3}, {3,2}, {1,1},
    {1,3}, {5,5}, {2,3}, {3,5}, {3,1},
    {3,1}, {2,5}, {4,2}, {0,1}, {1,3},
    {2,3}, {1,2}, {2,2}, {1,1}, {0,4},
    {1,5}, {4,4}, {3,4}, {2,2}, {4,3},
    {2,1}, {4,1}, {4,2}, {5,4}, {4,1},
    {5,1}, {0,2}, {2,1}, {3,3}, {0,5},
    {5,3}, {0,2}, {4,3}, {2,4}, {1,4},
    {1,2}, {0,4}, {2,4}, {3,4}, {3,2},
};

// Actions T1..T16 in hanab.live shape:
//   type 0=Play, 1=Discard, 2=ColourClue, 3=RankClue
//   target = card order (play/discard) or player index (clue)
//   value  = clue value (colour index / rank); 0 for play/discard
// We apply T1..T16 in the test prefix; T17 = will-bot67's decision.
const std::vector<OrigAction> kActions = {
    {3,2,5},  // T1 : P0 rank-5 → P2
    {0,9,0},  // T2 : P1 plays ord 9 = (1,1)
    {0,14,0}, // T3 : P2 plays ord 14 = (3,1)
    {2,1,0},  // T4 : P0 colour-0 (Red) → P1 — touches orig orders 7,6
    {0,8,0},  // T5 : P1 plays ord 8 = (3,2)
    {2,0,4},  // T6 : P2 colour-4 (Purple) → P0
    {0,4,0},  // T7 : P0 plays ord 4 = (0,1)
    {3,2,3},  // T8 : P1 rank-3 → P2
    {1,12,0}, // T9 : P2 discards ord 12 = (2,3)
    {2,1,4},  // T10: P0 colour-4 (Purple) → P1
    {0,5,0},  // T11: P1 plays ord 5 = (3,3)
    {1,19,0}, // T12: P2 discards ord 19 = (1,3) newly drawn
    {2,2,1},  // T13: P0 colour-1 (Yellow) → P2
    {1,7,0},  // T14: P1 discards ord 7 = (0,3)
    {0,21,0}, // T15: P2 plays ord 21 = (1,2)
    {3,1,3},  // T16: P0 rank-3 → P1  ← the bug clue
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Player rotation. Hanab.live indexes orig P0=will-bot69, P1=will-bot67,
  // P2=yagami_black. We rotate so the POV (will-bot67) is my P0.
  //   orig P0 (will-bot69) → my P2
  //   orig P1 (will-bot67) → my P0  (POV)
  //   orig P2 (yagami)     → my P1
  ctx.orig_to_my_player = {2, 0, 1};
  const int N = static_cast<int>(kDeck.size());
  ctx.orig_to_my_order.resize(N);
  ctx.my_order_to_id.resize(N);
  for (int orig_p = 0; orig_p < 3; ++orig_p) {
    int my_p = ctx.orig_to_my_player[orig_p];
    for (int i = 0; i < 5; ++i) {
      ctx.orig_to_my_order[orig_p * 5 + i] = my_p * 5 + i;
    }
  }
  for (int o = 15; o < N; ++o) ctx.orig_to_my_order[o] = o;
  for (int orig_o = 0; orig_o < N; ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

Game build_start() {
  SetupOptions opts;
  // my P0 = will-bot67 (POV, own hand hidden).
  // my P1 = yagami (orig P2). Initial orig orders [14,13,12,11,10] =
  //   (3,1)=b1, (3,5)=b5, (2,3)=g3, (5,5)=u5 (Dark Null 5), (1,3)=y3.
  // my P2 = will-bot69 (orig P0). Initial orig orders [4,3,2,1,0] =
  //   (0,1)=r1, (4,4)=p4, (1,4)=y4, (4,5)=p5, (5,2)=u2.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b1", "b5", "g3", "u5", "y3"},
      {"r1", "p4", "y4", "p5", "u2"},
  };
  opts.variant_name = "Scarce Ones & Dark Null (6 Suits)";
  // orig P0 (will-bot69 → my P2) acts first.
  opts.starting = TestPlayer::CATHY;
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kActions[i], ctx);
}

}  // namespace

// Diagnostic dump. Run via `ctest -V -R EndgameReplay1899044.T17Dump`
// to inspect the per-order status / urgent / clued / inferred fields
// of will-bot67's hand right after the T16 rank-3 clue, and see what
// `take_action()` chose. This tells us which leg of the plan-file
// diagnosis (a/b/c) is firing.
TEST(EndgameReplay1899044, T17DumpAndTakeAction) {
  Game g = build_start();
  apply_prefix(g, 16);  // T1..T16 applied; T17 = bot's decision.

  std::cerr << "=== PRE-T17 (post-T16 clue) ===\n";
  std::cerr << "current_player_index=" << g.state.current_player_index << "\n";
  std::cerr << "stacks:";
  for (int s : g.state.play_stacks) std::cerr << " " << s;
  std::cerr << "\nclue_tokens=" << g.state.clue_tokens
            << " strikes=" << g.state.strikes
            << " pace=" << g.state.pace()
            << " in_endgame=" << g.in_endgame() << "\n";
  std::cerr << "waiting size=" << g.waiting.size();
  if (!g.waiting.empty()) {
    std::cerr << " front{reacter=" << g.waiting.front().reacter
              << ", receiver=" << g.waiting.front().receiver
              << ", inverted=" << g.waiting.front().inverted << "}";
  }
  std::cerr << "\n";

  const char* names[] = {"will-bot67 (POV/Alice)", "yagami (BOB)",
                          "will-bot69 (CATHY)"};
  for (int pi = 0; pi < 3; ++pi) {
    std::cerr << "P" << pi << " " << names[pi] << ":\n";
    for (int slot = 0; slot < (int)g.state.hands[pi].size(); ++slot) {
      int o = g.state.hands[pi][slot];
      auto id = g.state.deck[o].id();
      std::cerr << "  s" << (slot+1) << " ord=" << o << " act=";
      if (id) std::cerr << g.state.log_id(*id);
      else std::cerr << "??";
      std::cerr << " stat=" << (int)g.meta[o].status
                << " urg=" << g.meta[o].urgent
                << " focused=" << g.meta[o].focused
                << " trash=" << g.meta[o].trash
                << " clued=" << g.state.deck[o].clued
                << " inf=" << g.common.thoughts[o].inferred.length();
      if (g.common.thoughts[o].inferred.length() <= 8) {
        std::cerr << "{";
        bool first = true;
        for (Identity i : g.common.thoughts[o].inferred) {
          if (!first) std::cerr << ",";
          std::cerr << g.state.log_id(i);
          first = false;
        }
        std::cerr << "}";
      }
      std::cerr << "\n";
    }
  }

  PerformAction action = g.take_action();
  std::cerr << "\ntake_action() returned: ";
  if (std::holds_alternative<PerformPlay>(action)) {
    std::cerr << "PerformPlay ord=" << std::get<PerformPlay>(action).target;
  } else if (std::holds_alternative<PerformDiscard>(action)) {
    std::cerr << "PerformDiscard ord=" << std::get<PerformDiscard>(action).target;
  } else if (std::holds_alternative<PerformColour>(action)) {
    auto& c = std::get<PerformColour>(action);
    std::cerr << "PerformColour target=" << c.target << " val=" << c.value;
  } else if (std::holds_alternative<PerformRank>(action)) {
    auto& r = std::get<PerformRank>(action);
    std::cerr << "PerformRank target=" << r.target << " val=" << r.value;
  } else {
    std::cerr << "PerformTerminate";
  }
  std::cerr << "\n";

  SUCCEED();
}

// Regression guard for the v0.30 ref_discard / elim interaction fix.
//
// Root cause traced via T17DumpAndTakeAction:
//   T9 yagami's discard of g3 created a SarcasticLink whose viable
//   candidates narrowed over time to a single order = will-bot67's
//   ord 15 (= slot 4 at T16). The link's `refresh_links` pass then
//   stamped ord 15 with status=SARCASTIC and inferred={g3}.
//   T16 rank-3 clue: ord 15 is untouched, so on_clue applied
//   `inferred.difference(rank-3 set)` -> inferred became empty.
//   ref_discard correctly chose ord 15 as the discard target and
//   stamped CALLED_TO_DISCARD. But elim()'s Step 1 then fired its
//   "inferred empty -> reset" rule and wiped the CTD status back to
//   NONE, leaving will-bot67's chop at slot 1.
// Fix: ref_discard calls reset_inferences on the target if its
// inferred is empty, before stamping CTD. (Identical pattern to the
// fix already in target_play / target_discard.)
TEST(EndgameReplay1899044, T17ShouldDiscardSlot4) {
  Game g = build_start();
  apply_prefix(g, 16);

  ASSERT_EQ(g.state.current_player_index, 0)
      << "will-bot67 (POV/Alice) must be on turn at T17";

  // Sanity-check the hand layout matches the diagnosis.
  // Slot 4 (1-indexed) = index 3. Slot 1 (newest) = index 0.
  int slot1 = g.state.hands[0][0];
  int slot4 = g.state.hands[0][3];
  ASSERT_FALSE(g.state.deck[slot4].clued)
      << "slot 4 must be unclued for ref_discard to target it";
  ASSERT_FALSE(g.state.deck[slot1].clued)
      << "slot 1 must be unclued (rank-3 didn't touch it)";

  // Convention check: slot 4 must be stamped CTD by the rank-3 clue
  // (focus = slot 2, target_index loop skips clued slot 3, lands on
  // unclued slot 4).
  EXPECT_EQ(g.meta[slot4].status, CardStatus::CALLED_TO_DISCARD)
      << "stable rank-3 ref_discard must mark slot 4 CTD";

  PerformAction action = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformDiscard>(action))
      << "with no immediate plays, will-bot67 should discard";
  int discard_target = std::get<PerformDiscard>(action).target;

  EXPECT_EQ(discard_target, slot4)
      << "will-bot67 should discard the convention-marked CTD slot 4 "
         "(b1), not chop slot 1 (y2). Regression for replay 1899044 "
         "T17.";
  EXPECT_NE(discard_target, slot1)
      << "slot 1 (y2) is chop only if CTD-on-slot-4 was lost - the "
         "v0.30 bug.";
}
