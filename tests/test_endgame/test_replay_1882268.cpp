// Hanab.live replay 1882268 turn 9. Players: P0=will-bot69 (our bot POV),
// P1=will-bot67, P2=yagami_black. Variant "Light-Pink-Ones & Brown
// (3 Suits)".
//
// At turn 7 (action index 7), will-bot67 (P1) gives a red color clue to
// will-bot69 (P0) — target=P0=cathy from P1's POV, so this is a
// reactive-shape clue. Pre-v0.39 the dispatcher's reacter-search loop
// vacuously picked P0 (no obvious_playables → empty old_play → vacuous
// match) and, because reacter==target, routed to interpret_stable.
// try_stable's ref_play stamped CTP on slot 5 (order 0 = b3, NOT playable
// since the blue stack is 0); when yagami at T8 played an unexpected
// brown 3, the response-inversion rewind reverted the bad stable to
// reactive.
//
// v1.2 (replay 1915981 fix): with bob (yagami) loaded via the T5
// reveal (known r2), the clue-to-cathy reads STABLE unless the stable
// interpretation is bad. The giver and bob can see the pushed b3 is
// unplayable (bad stable → they read reactive); cathy can't see her own
// hand, so she provisionally CTPs slot 5 and yagami's unexpected
// brown-3 play at T8 triggers the response-inversion rewind back to the
// reactive read. Bob always acts before cathy, so the provisional CTP
// is never played.
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

// hanab.live export deck. Suit indices: 0=Red, 1=Blue, 2=Brown.
const std::vector<std::pair<int, int>> kDeck = {
    {1, 3}, {1, 3}, {2, 1}, {0, 5}, {2, 4},
    {0, 2}, {1, 2}, {0, 1}, {2, 3}, {2, 1},
    {0, 3}, {2, 1}, {2, 3}, {0, 2}, {1, 2},
    {0, 3}, {2, 5}, {2, 2}, {0, 4}, {1, 4},
    {0, 1}, {1, 1}, {1, 5}, {0, 1}, {1, 4},
    {1, 1}, {2, 4}, {2, 2}, {0, 4}, {1, 1},
};

// First 9 actions of the replay — enough to reach the symptom (slot 5
// would be marked CALLED_TO_PLAY post-stable, and yagami's brown-3 play
// should trigger the rewind).
const std::vector<OrigAction> kOrigActions = {
    {3, 2, 2},  // 0: P0 → P2 rank-2 (touches orders 13, 14)
    {0, 7, 0},  // 1: P1 plays order 7 (r1)
    {0, 11, 0}, // 2: P2 plays order 11 (k1)
    {1, 4, 0},  // 3: P0 discards order 4 (k4 slot 1)
    {2, 2, 0},  // 4: P1 → P2 red clue (touches orders 10, 13)
    {2, 0, 0},  // 5: P2 → P0 red clue (touches order 3 = r5)
    {0, 17, 0}, // 6: P0 plays order 17 (k2 just drawn)
    {2, 0, 0},  // 7: P1 → P0 red clue (touches orders 3, 18) — the bad stable
    {0, 12, 0}, // 8: P2 plays order 12 (k3 brown 3 from slot 4) — unexpected
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Identity mapping: orig P0/1/2 = my P0/1/2.
  ctx.orig_to_my_player = {0, 1, 2};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int i = 0; i < 30; ++i) ctx.my_order_to_id[i] = kDeck[i];
  return ctx;
}

}  // namespace

TEST(EndgameReplay1882268, BadStableRewindsToReactiveOnUnexpectedPlay) {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (P0, observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot67 (P1): orig orders [9,8,7,6,5] = [n1, n3, r1, b2, r2].
      {"n1", "n3", "r1", "b2", "r2"},
      // Cathy = yagami_black (P2): orig [14,13,12,11,10] = [b2, r2, n3, n1, r3].
      {"b2", "r2", "n3", "n1", "r3"},
  };
  opts.variant_name = "Light-Pink-Ones & Brown (3 Suits)";
  opts.starting = TestPlayer::ALICE;  // orig P0 starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  // Apply actions 0..6 (everything up to but not including the bad clue).
  for (size_t i = 0; i < 7; ++i) apply_orig_action(g, kOrigActions[i], ctx);

  // Sanity: yagami's slot-2 (my-order 13 = r2) is the known loaded play
  // from the earlier rank-2 + red clues.
  EXPECT_TRUE(std::find(g.state.hands[2].begin(), g.state.hands[2].end(), 13) !=
              g.state.hands[2].end());
  auto cathy_known_plays = g.common.obvious_playables(g, 2);
  EXPECT_NE(std::find(cathy_known_plays.begin(), cathy_known_plays.end(), 13),
            cathy_known_plays.end())
      << "yagami should already be loaded with known r2 (my-order 13)";

  // Apply action 7 (the red clue from will-bot67 → will-bot69).
  apply_orig_action(g, kOrigActions[7], ctx);

  // v1.2 (replay 1915981 fix): with bob (yagami) loaded, a clue to
  // cathy reads STABLE unless the stable interpretation is bad. Cathy
  // can't see her own slot 5 to tell the ref_play push (b3) is bad, so
  // from her POV she provisionally CTPs it and relies on bob's action
  // to correct her — the giver and bob, who CAN see the b3, read the
  // clue as reactive via bad_stable. Bob always acts before cathy, so
  // the provisional CTP can never be played before the rewind fires.
  EXPECT_EQ(g.meta[0].status, CardStatus::CALLED_TO_PLAY)
      << "cathy's provisional stable read must CTP slot 5 until bob's "
         "reaction corrects it";

  // Apply action 8 (yagami plays brown 3 from slot 4).
  apply_orig_action(g, kOrigActions[8], ctx);

  // Slot 5 should still not be CTP'd after action 8.
  EXPECT_NE(g.meta[0].status, CardStatus::CALLED_TO_PLAY)
      << "slot 5 b3 should never have been CTP'd";

  // And take_action on the bot's turn must NOT play order 0 (the misplay).
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  PerformAction perform = g.take_action();
  if (std::holds_alternative<PerformPlay>(perform)) {
    EXPECT_NE(std::get<PerformPlay>(perform).target, 0)
        << "bot should not play slot 5 (b3) after the bad-stable rewind";
  }
}
