// Diagnostic + regression test for replay 1899527 T47 — user-reported
// forced-endgame bug.
//
// Variant: Black Reversed (5 Suits). Players (orig order):
// P0=will-bot67, P1=will-bot69 (POV), P2=yagami_black.
// Suit shorts: r, y, g, b, l (Black Reversed; "k" reserved for plain
// Black via pick_short's special case, but this variant's suit isn't
// the exact string "Black" so pick_short defaults to first-free
// letter after 'b' collision -> 'l').
//
// State at T47 (will-bot69's turn):
//   Stacks: r=4, y=3, g=5, b=5, l(=k_reversed) complete (stack at 1).
//   Score = 4+3+5+5+5 = 22. Cards left in deck = 1. Clue tokens = 1.
//   will-bot69 hand (slot 1=newest): r5(s1, clued rank-5 at T45),
//   b3(s2, clued rank-3 at T43 + T46 -- but b stack=5 so trash),
//   g1(s3, unclued), y4(s4, clued yellow at T37), r4(s5, unclued --
//   r4 already played at T28 so trash).
//
// Two critical singleton-inferred cards in P1's hand:
//   * ord 42 = r5 (clued rank-5; common knows wb67 holds y5, b5/g5/k5
//     all played, so inferred narrows to {r5}). r4 just played at
//     T28 -- r5 currently playable.
//   * ord 27 = y4 (clued yellow; y1 fully accounted, y2/y3 trash
//     (y stack=3), y5 dupped to wb67; only y4 left). y3 just played
//     at T42 -- y4 currently playable.
//
// Forced-endgame "two-critical play" rule (src/endgame/forced_endgame.
// cpp, v0.29) preconditions all satisfied:
//   * cards_left == 1
//   * clue_tokens (=1) < num_players (=3)
//   * >= 2 singleton-critical cards in P1's hand
//   * >= 1 of them playable
//
// User report: bot gave a rank-2 clue to P0 instead of playing.
// Bot also did NOT play the correct critical (y4): even if the rule
// had fired, the current implementation picks the first singleton-
// critical-playable in slot order. Slot 1 = r5, slot 4 = y4. The
// rule would pick r5 -- losing y5 (held by wb67, needs y4 played
// first to become playable). y4 must play first because another
// player holds y5.

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

// Deck from hanab.live export (45 cards). (suit_index, rank).
const std::vector<std::pair<int, int>> kDeck = {
    {0,3}, {4,3}, {2,3}, {0,1}, {1,2},
    {0,2}, {2,2}, {2,1}, {2,4}, {3,2},
    {2,5}, {2,2}, {4,1}, {3,3}, {4,5},
    {4,4}, {2,1}, {1,1}, {2,3}, {3,1},
    {1,1}, {0,3}, {3,1}, {0,4}, {0,4},
    {3,2}, {1,4}, {1,4}, {0,1}, {3,5},
    {1,1}, {1,5}, {3,1}, {0,1}, {2,1},
    {4,2}, {0,2}, {3,4}, {3,4}, {1,3},
    {3,3}, {1,2}, {0,5}, {2,4}, {1,3},
};

// Actions T1..T46 in hanab.live shape (type, target, value).
// type 0=Play, 1=Discard, 2=ColourClue, 3=RankClue.
// We apply T1..T46 in the test prefix; T47 = will-bot69's decision.
const std::vector<OrigAction> kActions = {
    {3,2,2},  // T1  P0 rank-2 -> P2
    {0,7,0},  // T2  P1 plays ord 7 = g1
    {0,14,0}, // T3  P2 plays ord 14 = k5 (reversed start)
    {3,2,5},  // T4  P0 rank-5 -> P2
    {0,15,0}, // T5  P1 plays ord 15 = k4
    {0,11,0}, // T6  P2 plays ord 11 = g2
    {3,2,1},  // T7  P0 rank-1 -> P2
    {0,17,0}, // T8  P1 plays ord 17 = y1
    {0,18,0}, // T9  P2 plays ord 18 = g3
    {3,2,3},  // T10 P0 rank-3 -> P2
    {0,8,0},  // T11 P1 plays ord 8  = g4
    {3,1,2},  // T12 P2 rank-2 -> P1
    {0,4,0},  // T13 P0 plays ord 4  = y2
    {0,19,0}, // T14 P1 plays ord 19 = b1
    {3,1,3},  // T15 P2 rank-3 -> P1
    {0,1,0},  // T16 P0 plays ord 1  = k3
    {0,9,0},  // T17 P1 plays ord 9  = b2
    {3,1,3},  // T18 P2 rank-3 -> P1 (re-clue)
    {0,3,0},  // T19 P0 plays ord 3  = r1
    {0,5,0},  // T20 P1 plays ord 5  = r2
    {0,10,0}, // T21 P2 plays ord 10 = g5 (g complete)
    {2,1,2},  // T22 P0 colour-2 (green) -> P1
    {0,21,0}, // T23 P1 plays ord 21 = r3
    {1,20,0}, // T24 P2 discards ord 20 = y1
    {1,26,0}, // T25 P0 discards ord 26 = y4  (!!)
    {3,0,5},  // T26 P1 rank-5 -> P0
    {0,13,0}, // T27 P2 plays ord 13 = b3
    {0,24,0}, // T28 P0 plays ord 24 = r4
    {1,6,0},  // T29 P1 discards ord 6 = g2
    {1,32,0}, // T30 P2 discards ord 32 = b1
    {1,33,0}, // T31 P0 discards ord 33 = r1
    {2,0,1},  // T32 P1 colour-1 (yellow) -> P0
    {0,35,0}, // T33 P2 plays ord 35 = k2
    {2,2,4},  // T34 P0 colour-4 (Black Reversed) -> P2
    {1,25,0}, // T35 P1 discards ord 25 = b2
    {0,37,0}, // T36 P2 plays ord 37 = b4
    {2,1,1},  // T37 P0 colour-1 (yellow) -> P1  <-- clues P1's y4
    {0,29,0}, // T38 P1 plays ord 29 = b5
    {0,12,0}, // T39 P2 plays ord 12 = k1 (k complete)
    {2,2,0},  // T40 P0 colour-0 (red) -> P2
    {1,38,0}, // T41 P1 discards ord 38 = b4
    {0,39,0}, // T42 P2 plays ord 39 = y3 (y stack -> 3)
    {3,1,3},  // T43 P0 rank-3 -> P1
    {2,0,0},  // T44 P1 colour-0 (red) -> P0
    {3,1,5},  // T45 P2 rank-5 -> P1  <-- clues P1's r5
    {3,1,3},  // T46 P0 rank-3 -> P1
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // POV = will-bot69 = orig P1 -> my P0.
  //   orig P0 (will-bot67) -> my P2
  //   orig P1 (will-bot69) -> my P0  (POV)
  //   orig P2 (yagami)     -> my P1
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
  // my P0 = will-bot69 (POV, own hand hidden).
  // my P1 = yagami (orig P2). Initial orig orders [14,13,12,11,10] =
  //   (4,5)=l5 (Black Reversed 5), (3,3)=b3, (4,1)=l1, (2,2)=g2,
  //   (2,5)=g5.
  // my P2 = will-bot67 (orig P0). Initial orig orders [4,3,2,1,0] =
  //   (1,2)=y2, (0,1)=r1, (2,3)=g3, (4,3)=l3, (0,3)=r3.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"l5", "b3", "l1", "g2", "g5"},
      {"y2", "r1", "g3", "l3", "r3"},
  };
  opts.variant_name = "Black Reversed (5 Suits)";
  // orig P0 (will-bot67 -> my P2) starts.
  opts.starting = TestPlayer::CATHY;
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kActions[i], ctx);
}

}  // namespace

// Diagnostic dump. Prints will-bot69's pre-T47 state so we can see
// which preconditions of the two-critical-play rule are missing (if
// any) at the bot's reasoning level.
TEST(EndgameReplay1899527, T47DumpAndTakeAction) {
  Game g = build_start();
  apply_prefix(g, 46);

  std::cerr << "=== PRE-T47 ===\n";
  std::cerr << "current_player_index=" << g.state.current_player_index << "\n";
  std::cerr << "stacks:";
  for (int s : g.state.play_stacks) std::cerr << " " << s;
  std::cerr << "\nclue_tokens=" << g.state.clue_tokens
            << " strikes=" << g.state.strikes
            << " cards_left=" << g.state.cards_left
            << " pace=" << g.state.pace()
            << " in_endgame=" << g.in_endgame() << "\n";

  const char* names[] = {"will-bot69 (POV/Alice)", "yagami (BOB)",
                          "will-bot67 (CATHY)"};
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

// Regression guard. The two-critical-play forced-endgame rule must
// fire at T47, and it must pick the rank-4 critical (y4) -- not the
// rank-5 critical (r5) -- because the other 5-of-suit (y5) is held
// by another player and needs the 4 played first. Playing r5 first
// loses y5 permanently; playing y4 first leaves y5 playable by wb67
// on their endgame turn.
TEST(EndgameReplay1899527, T47ShouldPlayYellow4) {
  Game g = build_start();
  apply_prefix(g, 46);

  ASSERT_EQ(g.state.current_player_index, 0)
      << "will-bot69 (POV/Alice) must be on turn at T47";
  ASSERT_EQ(g.state.cards_left, 1) << "deck must have 1 card left";
  ASSERT_LT(g.state.clue_tokens, g.state.num_players)
      << "two-critical-play rule needs ct < n";

  // Slot 4 = y4 in this replay's remapping. POV (wb69 = my P0) can't
  // see its own hand via state.deck, so we cross-check the order against
  // the deck array directly: orig order 27 -> my order 27 (orig orders
  // >= 15 map identity-wise) and kDeck[27] = (1, 4) = y4.
  int slot4 = g.state.hands[0][3];
  ASSERT_EQ(slot4, 27) << "slot 4 ord must be orig 27 (= y4)";
  ASSERT_EQ(kDeck[27].first, 1) << "kDeck[27] must be yellow";
  ASSERT_EQ(kDeck[27].second, 4) << "kDeck[27] must be rank 4";

  PerformAction action = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(action))
      << "two-critical-play rule must fire and produce a PerformPlay";
  EXPECT_EQ(std::get<PerformPlay>(action).target, slot4)
      << "must play y4 (slot 4), not r5 (slot 1). r5 first loses y5 "
         "permanently because y5 is held by wb67 and y5 needs y4 "
         "played first to become playable on wb67's endgame turn.";
}
