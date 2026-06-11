// Hanab.live replay 1885527, variant "Orange (3 Suits)". Players: orig
// P0=will-bot69, P1=will-bot67, P2=yagami_black.
//
// Probes two reported bugs:
//
// 1) Turn 1: P0 (will-bot69) gives colour-red to P1 (will-bot67). This
//    is a stable colour clue. ref_play picks a referenced target via
//    common.refer(left=true). If that referenced slot is an inverted
//    (orange) card, target_play unconditionally stamps CTP — but for
//    an orange CTP, the engine's on_play rule sends the card to the
//    discard pile (no stack advance). The user wants this clue to be
//    HEAVILY PENALIZED at eval time so will-bot69 doesn't pick it.
//
// 2) Turn 7: P0 (will-bot69) gives rank-1 to P1 (will-bot67). The user
//    suspects the convention interprets this as a stable play clue
//    that calls the receiver to perform-play an orange card (= send to
//    discard pile, opposite of intended advance).
//
// This probe captures the current behaviour so the diagnosis is clear.
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
    {2, 1}, {1, 3}, {0, 1}, {0, 4}, {1, 5},
    {1, 4}, {0, 3}, {0, 4}, {2, 1}, {2, 2},
    {2, 4}, {2, 4}, {0, 1}, {0, 3}, {0, 5},
    {2, 1}, {2, 3}, {1, 3}, {0, 2}, {0, 1},
    {0, 2}, {1, 2}, {1, 4}, {1, 1}, {2, 5},
    {1, 2}, {1, 1}, {1, 1}, {2, 2}, {2, 3},
};

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // ALICE = will-bot69 (orig P0, observer), BOB = will-bot67 (orig P1),
  // CATHY = yagami (orig P2). starting=ALICE.
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

Game build_turn_1() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = will-bot67: orig [9,8,7,6,5] newest-first.
      // deck[9]=(2,2)=o2, deck[8]=(2,1)=o1, deck[7]=(0,4)=r4,
      // deck[6]=(0,3)=r3, deck[5]=(1,4)=b4.
      {"o2", "o1", "r4", "r3", "b4"},
      // Cathy = yagami: orig [14,13,12,11,10] newest-first.
      // deck[14]=(0,5)=r5, deck[13]=(0,3)=r3, deck[12]=(0,1)=r1,
      // deck[11]=(2,4)=o4, deck[10]=(2,4)=o4.
      {"r5", "r3", "r1", "o4", "o4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  return setup(std::move(opts));
}

}  // namespace

// At turn 1, the stable-colour red clue from will-bot69 (P0) to
// will-bot67 (P1) used to land on an orange focus via ref_play and
// stamp CTP — which under the engine's orange game-rule sends the card
// to the discard pile (lost). After the ref_play orange-target reject,
// that clue is interpreted as MISTAKE (eval −10) and the bot picks a
// rank clue that resolves to CTD on the orange (= PerformDiscard =
// orange play attempt = stack advance).
TEST(EndgameReplay1885527, Turn1StableColourOnOrangeIsRejected) {
  Game g = build_turn_1();
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));

  // The colour-red→Bob clue must score as MISTAKE now.
  Clue red_to_bob{ClueKind::COLOUR, 0, static_cast<int>(TestPlayer::BOB)};
  ClueAction red_act{g.state.our_player_index, red_to_bob.target,
                      g.state.clue_touched(g.state.hands[red_to_bob.target],
                                            red_to_bob.kind, red_to_bob.value),
                      red_to_bob.base()};
  double red_score = hanabi::reactor::eval_action(g, Action{red_act});
  EXPECT_LT(red_score, -50.0)
      << "ref_play onto an orange target must be MISTAKE-penalized";

  // Simulating the red clue yields ClueInterp::MISTAKE (== 0).
  Game hypo = g.simulate(Action{red_act});
  ASSERT_FALSE(hypo.move_history.empty());
  ASSERT_TRUE(std::holds_alternative<ClueInterp>(hypo.move_history.back()));
  EXPECT_EQ(static_cast<int>(std::get<ClueInterp>(hypo.move_history.back())), 0)
      << "expected ClueInterp::MISTAKE";

  // None of bob's cards should be CTP'd by the rejected clue.
  for (int o : hypo.state.hands[static_cast<int>(TestPlayer::BOB)]) {
    EXPECT_NE(hypo.meta[o].status, CardStatus::CALLED_TO_PLAY)
        << "rejected stable colour clue must not CTP any of bob's cards";
  }

  // The bot's chosen action must not be the red→Bob clue.
  PerformAction perform = g.take_action();
  if (std::holds_alternative<PerformColour>(perform)) {
    auto p = std::get<PerformColour>(perform);
    EXPECT_FALSE(p.value == 0 && p.target == static_cast<int>(TestPlayer::BOB))
        << "bot must not pick the penalized red→Bob clue";
  }
}

// Turn 7 follow-up diagnosis. The user's complaint #2 ("queued CTDs
// are not loaded, the rank-1 clue is wrongly a stable play clue that
// would CTP an orange") is partly resolved by other code paths: at
// this position the convention interprets rank-1→Bob as REVEAL (not
// PLAY), so no CTP is set and Bob's order 15 stays CTD'd. Bob will
// still PerformDiscard order 15 → orange play attempt → advance.
//
// The remaining concern (the bot's *eval* favouring rank-1 over other
// useful actions like discarding Alice's own CTD'd o1) is a separate
// heuristic issue not covered by this ref_play fix. This test just
// pins the convention's REVEAL interpretation so future regressions
// surface.
TEST(EndgameReplay1885527, Turn7Rank1IsRevealNotPlay) {
  Game g = build_turn_1();
  ReplayContext ctx = make_ctx();
  // Actions 0..5 = turns 1..6.
  const std::vector<OrigAction> kOrigActions = {
      {2, 1, 0},   // 0 (turn 1): P0 colour-red to P1
      {1, 8, 0},   // 1 (turn 2): P1 discards order 8 (o1, orange-rule sends to discard pile)
      {3, 1, 3},   // 2 (turn 3): P2 → P1 rank-3
      {1, 2, 0},   // 3 (turn 4): P0 discards order 2 (r1)
      {2, 0, 0},   // 4 (turn 5): P1 colour-red to P0
      {0, 12, 0},  // 5 (turn 6): P2 plays order 12 (r1)
  };
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));

  // Bob's order 15 (o1) was already CTD'd by an earlier convention
  // pass — the convention's REVEAL interpretation of the rank-1 clue
  // must leave that CTD intact (not flip it to CTP).
  int bob_slot1 = g.state.hands[static_cast<int>(TestPlayer::BOB)][0];
  EXPECT_EQ(bob_slot1, 15);
  EXPECT_EQ(g.meta[bob_slot1].status, CardStatus::CALLED_TO_DISCARD);

  Clue r1_to_bob{ClueKind::RANK, 1, static_cast<int>(TestPlayer::BOB)};
  ClueAction r1_act{g.state.our_player_index, r1_to_bob.target,
                     g.state.clue_touched(g.state.hands[r1_to_bob.target],
                                            r1_to_bob.kind, r1_to_bob.value),
                     r1_to_bob.base()};
  Game hypo = g.simulate(Action{r1_act});
  ASSERT_FALSE(hypo.move_history.empty());
  ASSERT_TRUE(std::holds_alternative<ClueInterp>(hypo.move_history.back()));
  // ClueInterp values per include/hanabi/basics/interp.h:
  // MISTAKE=0, REACTIVE=1, PLAY=2, SAVE=3, DISCARD=4, LOCK=5, REVEAL=6,
  // FIX=7, STALL=8. We want REVEAL (= 6), not PLAY (= 2).
  EXPECT_EQ(static_cast<int>(std::get<ClueInterp>(hypo.move_history.back())), 6)
      << "rank-1 on an already-CTD'd focus must be REVEAL, not PLAY";
  EXPECT_EQ(hypo.meta[bob_slot1].status, CardStatus::CALLED_TO_DISCARD)
      << "REVEAL must not promote the CTD'd focus to CTP";
}
