// Hanab.live replay 1892112 turn 11 — regression for the v0.23
// holder-POV fix in `effective_possible_for` and the continue-on-
// failure change in the reactive play_targets loop.
//
// Variant: Funnels & Dark Prism (6 Suits). Players: orig P0=yagami,
// P1=will-bot67, P2=will-bot69. Our test POV = will-bot67 (P1, the
// giver of the suspect T11 rank-2 clue). We rotate so orig P1 → my P0.
//
// Pre-fix bug: effective_possible_for iterated all hands using the
// computing bot's `state.deck[o].id()`. Will-bot67 (giver) could see
// will-bot69's slot-3 p2, which dropped p2 from will-bot69's slot-5
// effective_possible → the convention skipped target_slot=2 (b2) and
// picked target_slot=3 (g3) → will-bot69's slot 4 (y1) reacts. From
// will-bot69's POV (own slot 3 hidden), p2 stayed in possible →
// convention picked target_slot=2 (b2) → will-bot69's slot 5 (p1)
// reacts → strike.
//
// Post-fix: effective_possible_for uses the HOLDER's POV (excludes
// the holder's own hand from the visibility count). Now will-bot67
// and will-bot69 both see slot-5's p2 as still possible. The first
// play_target (slot 2 b2 → react_slot 5 p1) fails target_play because
// the giver-visible p1 isn't in the {p2}-narrowed inferred; with
// continue-on-failure the convention falls through to slot 3 (g3) →
// react_slot 4 (y1 playable). Both bots agree on this interp. No
// strike. The clue is no longer "bad".

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

const std::vector<std::pair<int, int>> kDeck = {
    {3,1}, {1,2}, {3,2}, {2,1}, {2,3},
    {5,3}, {1,5}, {4,4}, {4,2}, {3,3},
    {4,1}, {1,1}, {4,2}, {2,2}, {4,1},
    {1,4}, {3,2}, {1,2}, {1,3}, {0,1},
    {5,1}, {0,5}, {0,2},
};

const std::vector<OrigAction> kActions = {
    {2, 2, 4},  // T1 P0 colour-p → P2
    {1, 8, 0},  // T2 P1 discards order 8 (p2)
    {3, 0, 1},  // T3 P2 rank-1 → P0
    {0, 3, 0},  // T4 P0 plays order 3 (g1)
    {1,15, 0},  // T5 P1 discards order 15 (y4)
    {0,14, 0},  // T6 P2 plays order 14 (p1)
    {0, 0, 0},  // T7 P0 plays order 0 (b1)
    {3, 0, 3},  // T8 P1 rank-3 → P0
    {0,13, 0},  // T9 P2 plays order 13 (g2)
    {0,19, 0},  // T10 P0 plays order 19 (r1)
    {3, 0, 2},  // T11 P1 rank-2 → P0 (the suspect)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Rotation: orig P1 (will-bot67) → my P0 (POV).
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
      // my P0 = will-bot67 (POV, hidden).
      {"xx", "xx", "xx", "xx", "xx"},
      // my P1 = will-bot69 (orig P2). Initial hand slot-1-first:
      // (4,1)=p1, (2,2)=g2, (4,2)=p2, (1,1)=y1, (4,1)=p1.
      {"p1", "g2", "p2", "y1", "p1"},
      // my P2 = yagami (orig P0). Initial hand slot-1-first:
      // (2,3)=g3, (2,1)=g1, (3,2)=b2, (1,2)=y2, (3,1)=b1.
      {"g3", "g1", "b2", "y2", "b1"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  opts.starting = TestPlayer::CATHY;  // orig P0 (yagami) → my P2.
  return setup(std::move(opts));
}

void apply_prefix(Game& g, size_t count) {
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < count; ++i) apply_orig_action(g, kActions[i], ctx);
}

}  // namespace

// Regression: T11's reactive interp must NOT CTP will-bot69's slot 5
// (= p1 actual, would misplay since p stack=1). The play_target sort
// picks slot 2 (b2) first, but the resulting react_slot 5 (p1) fails
// target_play because the giver-visible identity (p1) isn't in the
// narrowed-to-{p2} inferred. The continue-on-failure logic then falls
// through to slot 3 (g3) → react_slot 4 (y1 playable).
TEST(EndgameReplay1892112, T11DoesNotCTPBadReacterSlot) {
  Game g = build_start();
  apply_prefix(g, 11);

  // my P1 = will-bot69. Slot 5 = ord 5 = orig 10 = p1 (already played
  // copy at T6 → p stack=1, so p1 is now basic trash).
  int wb69_slot5 = g.state.hands[1][4];
  ASSERT_EQ(wb69_slot5, 5);
  EXPECT_NE(g.meta[wb69_slot5].status, CardStatus::CALLED_TO_PLAY)
      << "T11 must NOT CTP will-bot69's slot 5 (= p1, basic trash) "
         "— pre-fix the giver could see slot 3's p2 and narrowed slot 5 "
         "to {p2}, which target_play accepted under giver visibility "
         "but failed under receiver visibility, causing a strike-at-T12 "
         "desync between giver and receiver.";
}

// Sanity: the convention now picks a valid alternative play_target.
// Specifically slot 3 (g3) → react_slot 4 = will-bot69's slot 4
// (y1 actual, currently playable).
TEST(EndgameReplay1892112, T11FallsThroughToValidReactiveTarget) {
  Game g = build_start();
  apply_prefix(g, 11);

  int wb69_slot4 = g.state.hands[1][3];
  ASSERT_EQ(wb69_slot4, 6);
  // Will-bot69's slot 4 = ord 6 = orig 11 = y1 (playable on y stack=0).
  EXPECT_EQ(g.meta[wb69_slot4].status, CardStatus::CALLED_TO_PLAY)
      << "post-fix: convention falls through to target_slot=3 (g3) "
         "with react_slot=4 (y1 playable). Both giver and receiver "
         "agree — no desync.";
  EXPECT_TRUE(g.meta[wb69_slot4].urgent);
}
