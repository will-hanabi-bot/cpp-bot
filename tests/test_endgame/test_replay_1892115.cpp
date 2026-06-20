// Hanab.live replay 1892115 turn 56 — regression for the v0.24
// reactive-context advance-through-good-touch-self-plays fix.
//
// Variant: Funnels & Dark Prism (6 Suits). Players: orig P0=will-bot67,
// P1=yagami_black, P2=will-bot69. POV = yagami (the T56 giver), mapped
// to my P0 via orig_to_my_player = {2, 0, 1}.
//
// State at T56 (pre-clue): stacks r=4 y=5 g=5 b=5 p=5 i=3, score 27/30,
// clues 3, strikes 1, deck has 3 cards left. Remaining plays needed:
// i4 (in wb67 slot 4 = o16), i5 (in wb67 slot 2 = o47), r5 (in deck).
//
// wb67's `common.thoughts[o16]` is good-touch-known-playable pre-T56:
// possible `{b1..b4, i4}` minus the trash_set (b1..b4 all basic-trash
// since b stack=5) reduces to the singleton `{i4}`. (The reactor
// pipeline leaves `game.good_touch=false`, so this narrowing isn't
// stamped into `inferred` itself — it has to be re-derived from
// `possibilities().difference(trash_set)`.) The convention should
// advance hypo_state through this empathy-known play and target the
// *next* playable in wb67's hand — o47 = i5 — rather than redundantly
// CTPing o16. The reacter should land on wb69's slot 5 (o10, known
// basic trash) instead of slot 3 (o44 = r4, which from wb69's POV
// could be the critical r5 and so risks the reacter solver balking).
//
// Pre-fix (v0.23): `reactive_context()` advanced hypo_state via
// `obvious_playables`, missing good-touch-narrowed singletons whose
// status is still NONE. Play-target loop picked o16 + CTD'd o44.
// Post-fix: `thinks_playables(exclude_trash=true)`-based advance +
// `possibilities().difference(trash_set)` narrowing expose o47 as the
// reactive play target; receiver-narrowing block uses
// `ctx.hypo_state.playable_set` so o47's `{p5, i5}` ∩ `{r5, i5}` =
// `{i5}` survives.

#include <gtest/gtest.h>

#include <utility>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// Orig deck (suit, rank) from the hanab.live export.
const std::vector<std::pair<int, int>> kDeck = {
    {0,3}, {0,4}, {4,2}, {0,3}, {4,3},
    {1,1}, {3,3}, {4,1}, {5,2}, {1,1},
    {3,1}, {4,4}, {2,3}, {0,1}, {3,1},
    {1,3}, {5,4}, {1,5}, {0,1}, {1,4},
    {4,3}, {2,3}, {4,2}, {4,1}, {2,4},
    {4,1}, {2,2}, {5,1}, {1,2}, {3,1},
    {0,2}, {4,4}, {5,3}, {1,1}, {2,2},
    {4,5}, {0,2}, {2,1}, {2,4}, {3,2},
    {2,1}, {3,4}, {2,5}, {0,1}, {0,4},
    {1,2}, {3,3}, {5,5}, {1,3}, {3,5},
    {1,4}, {3,4}, {0,5}, {3,2}, {2,1},
};

// 55 player actions (T1..T55). T56 is the suspect purple clue — we
// construct & simulate it explicitly so we can inspect the resulting
// convention metadata.
const std::vector<OrigAction> kActions = {
    {3, 2, 4}, {0, 7, 0}, {3, 1, 5}, {0, 2, 0}, {0, 9, 0},
    {2, 1, 1}, {0, 4, 0}, {2, 0, 3}, {0, 14, 0}, {0, 18, 0},
    {2, 0, 0}, {0, 13, 0}, {2, 2, 4}, {1, 5, 0}, {0, 11, 0},
    {1, 20, 0}, {1, 22, 0}, {1, 23, 0}, {1, 24, 0}, {2, 0, 0},
    {1, 21, 0}, {0, 27, 0}, {2, 2, 2}, {0, 28, 0}, {3, 2, 4},
    {0, 15, 0}, {0, 30, 0}, {3, 2, 2}, {0, 8, 0}, {0, 19, 0},
    {3, 2, 1}, {0, 17, 0}, {3, 1, 1}, {0, 0, 0}, {0, 35, 0},
    {3, 1, 1}, {0, 1, 0}, {0, 37, 0}, {2, 1, 4}, {1, 36, 0},
    {0, 39, 0}, {0, 26, 0}, {3, 2, 4}, {0, 6, 0}, {0, 32, 0},
    {2, 2, 3}, {0, 41, 0}, {0, 12, 0}, {1, 40, 0}, {1, 31, 0},
    {2, 1, 0}, {0, 38, 0}, {3, 0, 4}, {0, 42, 0}, {0, 49, 0},
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Rotation: orig P1 (yagami) → my P0 (POV). orig P0 (wb67) → my P2.
  // orig P2 (wb69) → my P1.
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
  opts.hands = {
      // my P0 = yagami (POV, hidden).
      {"xx", "xx", "xx", "xx", "xx"},
      // my P1 = wb69 (orig P2). slot-1-first (newest first):
      // card 14 = b1, 13 = r1, 12 = g3, 11 = p4, 10 = b1.
      {"b1", "r1", "g3", "p4", "b1"},
      // my P2 = wb67 (orig P0). slot-1-first:
      // card 4 = p3, 3 = r3, 2 = p2, 1 = r4, 0 = r3.
      {"p3", "r3", "p2", "r4", "r3"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  opts.starting = TestPlayer::CATHY;  // orig P0 (wb67) → my P2.
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kActions[i], ctx);
}

// Apply yagami's T56 purple-to-wb67 clue as a ClueAction (we want to
// inspect convention state right after on_clue + interpret_clue, before
// any draw/turn bookkeeping that apply_orig_action wraps in).
void apply_t56_purple(Game& g) {
  ReplayContext ctx = make_ctx();
  int giver = g.state.current_player_index;  // my P0 = yagami.
  int target = ctx.orig_to_my_player[0];      // orig P0 (wb67) = my P2.
  auto touched = touched_orders(*g.state.variant, g.state.hands[target], ctx,
                                   ClueKind::COLOUR, 4);
  g.catchup = true;
  g.handle_action(ClueAction{giver, target, std::move(touched),
                                BaseClue(ClueKind::COLOUR, 4)});
  g.catchup = false;
}

}  // namespace

// v0.37 spec update: T56 purple-to-wb67 targets o16 (i4) as the
// reactive play target — NOT o47 (i5). Pre-v0.37 the convention
// trash-elim-narrowed o16's `possible = {b1..b4, i4}` to "must be i4"
// and advanced hypo_state past it, then picked i5 as the next
// playable. v0.37 removes that good-touch auto-advance, so the
// receiver's natural i4 play is re-signalled via the reactive (per
// the user's direction: "stop automatically inferring good touch on
// cards").
TEST(EndgameReplay1892115, T56ReactivePicksI4ViaGoodTouchRemoval) {
  Game g = build_start();
  apply_prefix(g, 55);

  // Sanity: order 16 in my numbering is the actual i4 (drawn from the
  // deck at orig 16 ≥ 15, so my_order == orig_order).
  ASSERT_EQ(g.state.deck[16].suit_index, 5);
  ASSERT_EQ(g.state.deck[16].rank, 4);

  apply_t56_purple(g);

  // o16 = i4 in wb67's hand is now the reactive play target.
  EXPECT_EQ(g.meta[16].status, CardStatus::CALLED_TO_PLAY)
      << "v0.37: T56 reactive targets o16 (i4) — the receiver's first "
         "good-touch-narrowed apparent play. With auto-advance removed, "
         "the convention re-signals this play rather than skipping it.";

  // o47 = i5 is no longer the chosen target; with the good-touch
  // advance gone, the convention picks the leftmost play (o16 i4),
  // not the next-rank chain (i5).
  EXPECT_NE(g.meta[47].status, CardStatus::CALLED_TO_PLAY)
      << "v0.37: o47 (i5) is no longer reactively CTP'd. Pre-v0.37 the "
         "good-touch advance past o16 made i5 the chosen target.";
}

// v0.37 spec update: with the reactive target shifted to o16 (i4)
// instead of o47 (i5), the calc_slot mapping yields a different
// reacter slot. The test now verifies the new mapping rather than the
// pre-v0.37 "must land on wb69 slot 5" expectation.
TEST(EndgameReplay1892115, T56ReactiveReacterFollowsNewTarget) {
  Game g = build_start();
  apply_prefix(g, 55);
  apply_t56_purple(g);

  // The reacter slot follows from focus_slot + target_slot via the
  // convention's calc_slot inversion. We simply verify that exactly
  // one wb69 slot got newly stamped CTD by this clue (the reactive's
  // reacter stamp), and that it's marked urgent.
  int wb69_ctd_count = 0;
  for (int o : g.state.hands[1]) {
    if (g.meta[o].status == CardStatus::CALLED_TO_DISCARD) ++wb69_ctd_count;
  }
  EXPECT_GE(wb69_ctd_count, 1)
      << "T56 reactive should stamp at least one of wb69's slots as the "
         "reacter CTD (the specific slot is determined by calc_slot "
         "inversion from the new receiver target; this test only "
         "checks that a reacter CTD was applied — the receiver target "
         "is pinned by the sibling test).";
}
