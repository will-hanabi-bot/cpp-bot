// Hanab.live replay 1883510 turn 22 (action index 21). Variant
// "White-Fives & Omni (3 Suits)". Players: orig P0=will-bot69 (our bot POV),
// P1=yagami_black, P2=will-bot67.
//
// At action 21 will-bot69 originally gave rank-3 to yagami_black. yagami's
// chop (slot 5) was omni-2 (order 6); rank-3 touches all three omni cards
// (orders 6, 7, 9) since Omni is pinkish. The clue violates the pink-promise
// convention — a rank-3 clue that newly touches the chop "promises" the chop
// has rank 3, but the chop is rank 2.
//
// Why pre-fix the bot gave it (task 2 from the bug report): try_stable's
// rank-handling iterated only Identity(s, clue.value), so only (R,3),(B,3),
// (K,3) were checked. (R,3) and (B,3) are basic trash; (K,3) is playable —
// the loop concluded playable_rank=true and stamped order 6 (the chop) as
// CALLED_TO_PLAY with inferred={(K,3)}. hypo_plays then accepted that as
// a legitimate play and eval_action scored the clue positively. Rank-4 to
// will-bot67 (the genuinely superior reactive clue calling omni-3→omni-4)
// lost the tiebreak.
//
// Post-fix: try_stable iterates touch_possibilities (so (K,4),(K,5) are
// also considered — playable_rank becomes false) and a dedicated
// violates_pink_promise gate short-circuits the clue to MISTAKE. The bot
// now picks rank-4 to will-bot67.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// hanab.live export deck. Suits: 0=Red, 1=Blue, 2=Omni. 30 cards.
const std::vector<std::pair<int, int>> kDeck = {
    {0, 1}, {0, 3}, {1, 5}, {0, 1}, {1, 1},
    {1, 3}, {2, 2}, {2, 3}, {2, 3}, {2, 5},
    {1, 2}, {1, 1}, {2, 4}, {0, 4}, {0, 4},
    {1, 4}, {2, 2}, {2, 4}, {1, 4}, {2, 1},
    {0, 2}, {0, 3}, {1, 2}, {2, 1}, {0, 2},
    {2, 1}, {0, 1}, {1, 3}, {0, 5}, {1, 1},
};

// Actions 0..20 — drives the game to just before will-bot69's rank-3 clue
// to yagami_black (action 21).
const std::vector<OrigAction> kOrigActions = {
    {2, 2, 0},   // 0:  P0 → P2 colour red
    {1, 8, 0},   // 1:  P1 discards order 8 (o3)
    {0, 11, 0},  // 2:  P2 plays order 11 (b1)
    {2, 2, 1},   // 3:  P0 → P2 colour blue
    {3, 0, 5},   // 4:  P1 → P0 rank-5
    {3, 0, 1},   // 5:  P2 → P0 rank-1
    {0, 0, 0},   // 6:  P0 plays order 0 (r1)
    {3, 2, 2},   // 7:  P1 → P2 rank-2
    {0, 10, 0},  // 8:  P2 plays order 10 (b2)
    {3, 2, 1},   // 9:  P0 → P2 rank-1
    {0, 5, 0},   // 10: P1 plays order 5 (b3)
    {0, 18, 0},  // 11: P2 plays order 18 (b4)
    {3, 2, 2},   // 12: P0 → P2 rank-2
    {0, 19, 0},  // 13: P1 plays order 19 (o1)
    {0, 20, 0},  // 14: P2 plays order 20 (r2)
    {3, 2, 3},   // 15: P0 → P2 rank-3
    {0, 21, 0},  // 16: P1 plays order 21 (r3)
    {0, 14, 0},  // 17: P2 plays order 14 (r4)
    {0, 2, 0},   // 18: P0 plays order 2 (b5)
    {1, 23, 0},  // 19: P1 discards order 23 (o1, basic trash)
    {0, 16, 0},  // 20: P2 plays order 16 (o2)
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // Identity mapping: orig P0/1/2 == our P0/1/2 (will-bot69 = observer P0).
  ctx.orig_to_my_player = {0, 1, 2};
  ctx.orig_to_my_order.resize(30);
  for (int o = 0; o < 30; ++o) ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(30);
  for (int i = 0; i < 30; ++i) ctx.my_order_to_id[i] = kDeck[i];
  return ctx;
}

Game build_game_through_action_20() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (P0, observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = yagami_black (P1): orig [9,8,7,6,5] = [o5, o3, o3, o2, b3].
      {"o5", "o3", "o3", "o2", "b3"},
      // Cathy = will-bot67 (P2): orig [14,13,12,11,10] = [r4, r4, o4, b1, b2].
      {"r4", "r4", "o4", "b1", "b2"},
  };
  opts.variant_name = "White-Fives & Omni (3 Suits)";
  opts.starting = TestPlayer::ALICE;  // orig P0 starts.
  Game g = setup(std::move(opts));

  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  return g;
}

}  // namespace

// The pink-promise-violating rank-3 clue must evaluate to MISTAKE (-100),
// and the rank-4-to-will-bot67 clue must outscore it. take_action picks
// rank-4 to will-bot67.
TEST(EndgameReplay1883510, RankThreeToBobViolatesPinkPromise) {
  Game g = build_game_through_action_20();

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE))
      << "should be will-bot69's turn at action 21";

  // Sanity: yagami_black's chop (slot 5, rightmost unclued) is omni-2.
  const auto& bob_hand = g.state.hands[1];
  ASSERT_FALSE(bob_hand.empty());
  int bob_chop = bob_hand.back();
  ASSERT_EQ(bob_chop, 6) << "Bob's chop should be order 6 (omni-2)";
  ASSERT_EQ(g.state.deck[bob_chop].suit_index, 2);
  ASSERT_EQ(g.state.deck[bob_chop].rank, 2);

  // The pink-promise gate makes rank-3 → yagami_black evaluate as a
  // MISTAKE (-100 sentinel from eval_action).
  ClueAction rank3_to_p1{
      /*giver=*/0,
      /*target=*/1,
      g.state.clue_touched(g.state.hands[1], ClueKind::RANK, 3),
      BaseClue(ClueKind::RANK, 3),
  };
  double rank3_score = hanabi::reactor::eval_action(g, Action{rank3_to_p1});
  EXPECT_LE(rank3_score, -50.0)
      << "rank-3 to yagami_black should be a MISTAKE (-100), got "
      << rank3_score;

  // Rank-4 to will-bot67 should be a healthy positive (it reactively calls
  // yagami's omni-3 to be played before will-bot67's omni-4).
  ClueAction rank4_to_p2{
      /*giver=*/0,
      /*target=*/2,
      g.state.clue_touched(g.state.hands[2], ClueKind::RANK, 4),
      BaseClue(ClueKind::RANK, 4),
  };
  double rank4_score = hanabi::reactor::eval_action(g, Action{rank4_to_p2});
  EXPECT_GT(rank4_score, 0.0)
      << "rank-4 to will-bot67 should be a positive-value clue, got "
      << rank4_score;

  // And take_action picks rank-4 to will-bot67.
  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformRank>(perform))
      << "expected a rank clue";
  auto picked = std::get<PerformRank>(perform);
  EXPECT_EQ(picked.target, static_cast<int>(TestPlayer::CATHY));
  EXPECT_EQ(picked.value, 4);
}
