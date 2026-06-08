// Hanab.live replay 1883650 turn 2 (action index 1). Variant "Orange
// (3 Suits)". Players: orig P0=yagami_black, P1=will-bot67 (our bot POV),
// P2=will-bot69.
//
// At action 0 yagami clues rank-5 to will-bot67 (touches will-bot67's b5
// and o5). At action 1 will-bot67 (P1) was observed picking rank-2 to
// yagami (P0), which the convention interprets reactively: target_play on
// P2's slot 2 (which is orange-1) — so will-bot69 ends up issuing
// PerformPlay on the orange-1, the orange-suit game-rule then sends it to
// the discard pile (no stack advance, +1 clue). The user reports this as
// "the orange variant immediately fails": P1 should not have picked this
// clue because there is no clue-shape that lets will-bot67 mark
// will-bot69's slot 2 as CALLED_TO_DISCARD (which would be the only way
// to get the orange-1 *played onto the stack* via the inverted rule).
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

const std::vector<std::pair<int, int>> kDeck = {
    {0, 3}, {2, 3}, {2, 2}, {2, 4}, {0, 1},
    {2, 5}, {2, 2}, {0, 3}, {1, 1}, {1, 5},
    {0, 2}, {1, 1}, {1, 2}, {2, 1}, {0, 4},
    {1, 3}, {0, 2}, {0, 1}, {1, 3}, {2, 4},
    {1, 2}, {0, 1}, {0, 5}, {1, 4}, {1, 4},
    {2, 1}, {0, 4}, {1, 1}, {2, 1}, {2, 3},
};

// Actions 0..0 — we want to set up the state right after yagami's rank-5
// clue (action 0), at which point will-bot67 (our bot) takes its turn.
const std::vector<OrigAction> kOrigActions = {
    {3, 1, 5},  // 0: P0 → P1 rank-5
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Remap so that will-bot67 is the observer at index 0 (ALICE in our
  // test harness).
  // orig P0 (yagami) → our P2 (CATHY)
  // orig P1 (will-bot67) → our P0 (ALICE)
  // orig P2 (will-bot69) → our P1 (BOB)
  ctx.orig_to_my_player = {2, 0, 1};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int i = 0; i < 30; ++i) ctx.my_order_to_id[i] = kDeck[i];
  return ctx;
}

Game build_game_through_action_0() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot67 (orig P1, observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot69 (orig P2): orig orders [14,13,12,11,10] = [r4,o1,b2,b1,r2].
      {"r4", "o1", "b2", "b1", "r2"},
      // Cathy = yagami_black (orig P0): orig orders [4,3,2,1,0] = [r1,o4,o2,o3,r3].
      {"r1", "o4", "o2", "o3", "r3"},
  };
  opts.variant_name = "Orange (3 Suits)";
  // orig P0 (yagami) starts. In our remap that's CATHY.
  opts.starting = TestPlayer::CATHY;
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

// The pre-fix bot picked rank-2 to yagami; the reactive convention's
// play-target r1 yielded target_play on will-bot69's slot 2 (orange-1),
// so on will-bot69's turn the bot would PerformPlay the orange — the
// game-rule inversion then drops it in the discard pile (orange stack
// stays 0). Post-fix, the would_lose_inverted_reacter gate rejects any
// reactive play/finesse path that resolves to target_play on an orange
// reacter card, so rank-2-to-Cathy evaluates well below alternatives
// (red-to-Bob, which marks Bob's b1 CTP via ref_play, is what
// take_action picks here). The user's framing is "this isn't possible
// in this scenario as there's no color clue focusing slot 3" — the
// bot's job is to recognize that and pick a different, safe clue.
TEST(EndgameReplay1883650, OrangeReacterRejectsRankPlayClue) {
  Game g = build_game_through_action_0();

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE))
      << "should be will-bot67's turn at action 1";

  // The convention now penalizes rank-2 to Cathy because the only
  // reactive play-target (r1) routes through target_play on the orange
  // reacter card, which the new gate rejects.
  ClueAction rank2_to_cathy{
      /*giver=*/0,
      /*target=*/static_cast<int>(TestPlayer::CATHY),
      g.state.clue_touched(g.state.hands[static_cast<int>(TestPlayer::CATHY)],
                             ClueKind::RANK, 2),
      BaseClue(ClueKind::RANK, 2),
  };
  double rank2_score = hanabi::reactor::eval_action(g, Action{rank2_to_cathy});

  ClueAction red_to_bob{
      /*giver=*/0,
      /*target=*/static_cast<int>(TestPlayer::BOB),
      g.state.clue_touched(g.state.hands[static_cast<int>(TestPlayer::BOB)],
                             ClueKind::COLOUR, 0),
      BaseClue(ClueKind::COLOUR, 0),
  };
  double red_to_bob_score = hanabi::reactor::eval_action(g, Action{red_to_bob});

  EXPECT_LT(rank2_score, red_to_bob_score)
      << "rank-2 to Cathy must score strictly below red-to-Bob now that the "
         "convention rejects target_play on the orange reacter";

  PerformAction perform = g.take_action();
  // The bot must not pick rank-2 to Cathy.
  if (std::holds_alternative<PerformRank>(perform)) {
    auto p = std::get<PerformRank>(perform);
    EXPECT_FALSE(p.target == static_cast<int>(TestPlayer::CATHY) && p.value == 2)
        << "bot should not give rank-2 to yagami_black "
           "(would CTP will-bot69's orange-1, losing it to the discard pile)";
  }
}
