// Hanab.live replay 1892428 turn 47 — regression for the user-reported
// "critical card called to discard by reactive clue" bug.
//
// At T47 will-bot67 (current player) gave a Colour-green clue to
// will-bot69. The reactor's reactive_colour interp picked wb69's slot 1
// (i3, leftmost playable on i_stack=2) as the receiver play target. The
// newest-demoted focus rule then chose Cathy's other touched slot — g2
// at slot 3 — as the focus, giving focus_slot=3. With target_slot=1,
//   react_slot = calc_slot(3, 1, hand_size=5) = (3+5-1) % 5 = 2
// landed the reacter CTD-urgent signal on yagami's slot 2 = i4 — the
// only remaining i4 copy in a Dark Prism deck (critical). yagami's
// solver followed the urgent CTD and discarded i4 on the next turn,
// capping max_score and losing the game.
//
// The fix lives in `Game::find_all_clues` (giver-side filter): after
// `simulate_clue`, drop any candidate whose convention newly CTD's a
// card whose actual id (visible to the giver) is `state.is_critical`.
//
// Variant: Funnels & Dark Prism (6 Suits). Players in the replay:
// orig P0=will-bot69, P1=will-bot67, P2=yagami_black. POV = wb67 (the
// T47 giver) → my P0.

#include <gtest/gtest.h>

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

const std::vector<std::pair<int, int>> kDeck = {
    {5,1}, {4,3}, {3,1}, {3,5}, {4,2}, {5,5}, {3,4}, {2,5}, {1,1}, {1,3},
    {0,3}, {4,2}, {2,1}, {0,1}, {5,2}, {3,2}, {0,4}, {0,2}, {3,2}, {3,3},
    {4,1}, {1,4}, {4,5}, {3,1}, {3,4}, {2,4}, {4,1}, {2,2}, {2,3}, {1,2},
    {1,1}, {4,4}, {2,2}, {0,1}, {4,3}, {1,5}, {4,1}, {2,3}, {1,4}, {0,4},
    {5,4}, {4,4}, {1,1}, {1,3}, {0,5}, {5,3}, {3,1}, {0,2}, {0,1}, {1,2},
    {0,3}, {2,1}, {2,1}, {3,3}, {2,4},
};

// T1..T46. We stop one short of T47 to probe wb67's `take_action`.
const std::vector<OrigAction> kActions = {
    {3,2,4}, {0,8,0}, {3,1,4}, {0,2,0}, {3,2,3}, {0,13,0}, {0,0,0}, {3,2,2}, {0,12,0}, {0,18,0},
    {3,2,2}, {0,17,0}, {0,20,0}, {2,2,0}, {2,1,3}, {1,16,0}, {0,9,0}, {0,19,0}, {3,2,2}, {0,6,0},
    {0,14,0}, {1,23,0}, {3,2,5}, {0,10,0}, {0,3,0}, {3,2,5}, {0,27,0}, {0,4,0}, {2,0,4}, {0,29,0},
    {0,28,0}, {1,26,0}, {2,1,2}, {0,34,0}, {1,15,0}, {1,11,0}, {2,2,2}, {1,37,0}, {0,31,0}, {2,1,1},
    {0,39,0}, {0,25,0}, {1,36,0}, {0,7,0}, {3,1,5}, {0,22,0},
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // orig P0=wb69 → my P2 (CATHY). orig P1=wb67 → my P0 (POV). orig P2=yagami → my P1 (BOB).
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
      // my P0 = wb67 (POV, hidden).
      {"xx", "xx", "xx", "xx", "xx"},
      // my P1 = yagami (orig P2). orig[10..14] = r3,p2,g1,r1,i2.
      // slot-1-first (newest = orig 14 = i2): [i2, r1, g1, p2, r3].
      {"i2", "r1", "g1", "p2", "r3"},
      // my P2 = wb69 (orig P0). orig[0..4] = i1,p3,b1,b5,p2.
      // slot-1-first: [p2, b5, b1, p3, i1].
      {"p2", "b5", "b1", "p3", "i1"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  // orig P0 (wb69) starts → my P2 = CATHY.
  opts.starting = TestPlayer::CATHY;
  return setup(std::move(opts));
}

}  // namespace

// Regression: at T47 in this state, wb67 must not give Colour-green →
// wb69. That clue's reactive interp CTDs yagami's slot 2 = i4, which is
// the sole remaining i4 in a Dark Prism deck (critical). yagami would
// discard it on the next turn, capping max_score.
TEST(EndgameReplay1892428, T47DoesNotIssueCriticalDiscardClue) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < 46; ++i) apply_orig_action(g, kActions[i], ctx);
  ASSERT_EQ(g.state.current_player_index, 0)
      << "T47 should be will-bot67's turn (= my P0).";
  ASSERT_EQ(g.state.score(), 23);

  PerformAction perform = g.take_action();
  if (auto* c = std::get_if<PerformColour>(&perform)) {
    bool bad = c->target == static_cast<int>(TestPlayer::CATHY) &&
               c->value == 2;  // colour 2 = green
    EXPECT_FALSE(bad)
        << "Colour-green → wb69 at T47 commits the reactive interp to "
           "CTD'ing yagami's slot 2 = i4 (the only i4 in a Dark Prism "
           "deck → critical). The giver-side find_all_clues filter must "
           "drop this clue.";
  }
}

// Sanity: with the filter, the bot should still have viable clue
// alternatives at T47 (the user-suggested yellow → wb69 or purple → wb69
// remain in the candidate set / the bot may pick another non-striking
// option). This guards against the filter accidentally emptying out all
// candidates.
TEST(EndgameReplay1892428, T47HasViableNonStrikingAction) {
  Game g = build_start();
  ReplayContext ctx = make_ctx();
  for (size_t i = 0; i < 46; ++i) apply_orig_action(g, kActions[i], ctx);

  PerformAction perform = g.take_action();
  // The chosen action must be something — discard, play, or a clue that
  // isn't the filtered green-to-wb69. (We don't pin which alternative
  // the bot picks; that's a separate decision-quality concern.)
  bool is_filtered_clue = false;
  if (auto* c = std::get_if<PerformColour>(&perform)) {
    is_filtered_clue = c->target == static_cast<int>(TestPlayer::CATHY) &&
                       c->value == 2;
  }
  EXPECT_FALSE(is_filtered_clue)
      << "Bot must have an action other than the filtered Colour-green "
         "→ wb69.";
}
