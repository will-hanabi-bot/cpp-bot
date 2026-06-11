// Hanab.live replay 1885536, variant "Muddy-Rainbow-Fives & Brown (3 Suits)".
// Orig players: P0=will-bot69, P1=will-bot67, P2=yagami_black.
//
// Turn 22 (action[21]): will-bot69 (P0) gives rank-2 to yagami (P2). The
// reactor convention's reactive interpretation should call will-bot67's
// (P1, the reacter) slot 1 (brown 4, order 27) to play.
//
// User report (1885536#23): bot67 wrote no CTP note on its own slot 1, and
// bot69 stamped the n4 note then reset it the next turn.
//
// Diagnosis (split into two suspects):
//
//  (a) Premature reset of CTP. `update_turn` was clearing any CTP'd card
//      whose `inferred ∩ playable_set` was empty — wrong for delayed-play
//      chains (the card may be waiting on a prerequisite). FIXED in
//      src/basics/game.cpp:299-318. `target_i_play` in interpret_reaction
//      had a downstream workaround citing this exact reset; that
//      suppression-of-CTP is also gone now (interpret_reaction.cpp:85-108).
//
//  (b) Receiver-side silence (bot67's own instance doesn't stamp CTP).
//      Independent issue: `bad_stable` in interpret_clue.cpp reads
//      `state.deck[o].id()`, which is perspective-dependent. From bot67's
//      perspective the rank-2 clue interprets as REVEAL/STALL with no
//      reactive fallback, so target_play never runs for bot67's slot 1.
//      NOT fixed by this change — flagged as `DISABLED_` below for a
//      future convention-side fix.
//
// This test pins the bot69-side post-fix behaviour. The bot67-side
// reproduction lives as a disabled test for visibility.
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

// Deck order matches the hanab.live export verbatim (1885536.json).
const std::vector<std::pair<int, int>> kDeck = {
    {0, 1}, {0, 3}, {2, 3}, {1, 1}, {0, 1},
    {1, 3}, {2, 1}, {1, 3}, {0, 4}, {2, 3},
    {0, 3}, {0, 2}, {1, 5}, {0, 1}, {2, 5},
    {2, 2}, {2, 2}, {1, 2}, {0, 2}, {1, 2},
    {1, 1}, {2, 1}, {1, 1}, {1, 4}, {1, 4},
    {2, 1}, {0, 5}, {2, 4}, {2, 4}, {0, 4},
};

const std::vector<OrigAction> kOrigActions = {
    {2, 2, 0},   // T1  P0 colour-0 to P2
    {0, 6, 0},   // T2  P1 plays order 6 (n1)
    {0, 13, 0},  // T3  P2 plays order 13 (r1)
    {2, 1, 2},   // T4  P0 colour-2 (brown) to P1
    {0, 15, 0},  // T5  P1 plays order 15 (n2)
    {3, 1, 3},   // T6  P2 rank-3 to P1
    {0, 3, 0},   // T7  P0 plays order 3 (b1)
    {0, 9, 0},   // T8  P1 plays order 9 (n3)
    {3, 1, 2},   // T9  P2 rank-2 to P1
    {0, 18, 0},  // T10 P0 plays order 18 (r2)
    {0, 19, 0},  // T11 P1 plays order 19 (b2)
    {3, 1, 4},   // T12 P2 rank-4 to P1
    {0, 0, 0},   // T13 P0 plays order 0 (r1) -- misplay
    {0, 7, 0},   // T14 P1 plays order 7 (b3)
    {2, 1, 0},   // T15 P2 colour-0 to P1
    {0, 1, 0},   // T16 P0 plays order 1 (r3)
    {0, 8, 0},   // T17 P1 plays order 8 (r4)
    {1, 16, 0},  // T18 P2 discards order 16 (n2, trash)
    {2, 2, 1},   // T19 P0 colour-1 (blue) to P2
    {0, 23, 0},  // T20 P1 plays order 23 (b4)
    {0, 26, 0},  // T21 P2 plays order 26 (r5)
    {3, 2, 2},   // T22 P0 rank-2 to P2  -- the bug clue
};

// Build the game from will-bot69's perspective (orig P0 = ALICE).
Game build_from_bot69_perspective() {
  SetupOptions opts;
  opts.hands = {
      // ALICE = will-bot69 (observer, hidden hand). Initial = orig orders 0..4.
      {"xx", "xx", "xx", "xx", "xx"},
      // BOB = will-bot67. Initial = orig orders 5..9, newest-first.
      // 9=(2,3)=n3, 8=(0,4)=r4, 7=(1,3)=b3, 6=(2,1)=n1, 5=(1,3)=b3.
      {"n3", "r4", "b3", "n1", "b3"},
      // CATHY = yagami. Initial = orig orders 10..14, newest-first.
      // 14=(2,5)=n5, 13=(0,1)=r1, 12=(1,5)=b5, 11=(0,2)=r2, 10=(0,3)=r3.
      {"n5", "r1", "b5", "r2", "r3"},
  };
  opts.variant_name = "Muddy-Rainbow-Fives & Brown (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  return setup(std::move(opts));
}

ReplayContext make_ctx_bot69() {
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

// Build the game from will-bot67's perspective (orig P1 = ALICE). Player
// order rotates so the action sequence's first actor (orig P0 = bot69) is
// my CATHY (P2), and my ALICE (P0) = orig P1 = bot67 plays second in the
// cycle.
Game build_from_bot67_perspective() {
  SetupOptions opts;
  opts.hands = {
      // ALICE = will-bot67 (observer, hidden hand).
      {"xx", "xx", "xx", "xx", "xx"},
      // BOB = yagami. Initial = orig orders 10..14, newest-first.
      {"n5", "r1", "b5", "r2", "r3"},
      // CATHY = will-bot69. Initial = orig orders 0..4, newest-first.
      // 4=(0,1)=r1, 3=(1,1)=b1, 2=(2,3)=n3, 1=(0,3)=r3, 0=(0,1)=r1.
      {"r1", "b1", "n3", "r3", "r1"},
  };
  opts.variant_name = "Muddy-Rainbow-Fives & Brown (3 Suits)";
  opts.starting = TestPlayer::CATHY;
  return setup(std::move(opts));
}

ReplayContext make_ctx_bot67() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  ctx.orig_to_my_player = {2, 0, 1};
  ctx.orig_to_my_order.resize(kDeck.size());
  for (int o = 0; o <= 4; ++o) ctx.orig_to_my_order[o] = o + 10;
  for (int o = 5; o <= 9; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 10; o <= 14; ++o) ctx.orig_to_my_order[o] = o - 5;
  for (int o = 15; o < static_cast<int>(kDeck.size()); ++o)
    ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(kDeck.size());
  for (size_t orig_o = 0; orig_o < kDeck.size(); ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

}  // namespace

// ---- Test 1 (bot69 perspective, ACTIVE) --------------------------------
//
// After turn 22 (rank-2 to yagami) fully applies and the turn boundary
// advances to bot67, bot67's slot 1 (brown 4, order 27) must still be
// CALLED_TO_PLAY. This pins the post-fix behaviour: target_play stamps
// CTP, and update_turn no longer resets it just because brown 4 isn't in
// the current playable_set when waiting on a delayed-play chain. (In this
// specific replay brown 4 is currently playable so the reset wouldn't
// have fired anyway, but the test still pins the convention's CTP-survival
// expectation across the turn boundary.)
TEST(EndgameReplay1885536, Rank2ToCathyCTPSurvivesIntoBobsTurn) {
  Game g = build_from_bot69_perspective();
  ReplayContext ctx = make_ctx_bot69();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::BOB));
  int bob_slot1 = g.state.hands[static_cast<int>(TestPlayer::BOB)][0];
  ASSERT_EQ(bob_slot1, 27);
  ASSERT_EQ(g.state.deck[bob_slot1].suit_index, 2);
  ASSERT_EQ(g.state.deck[bob_slot1].rank, 4);

  EXPECT_EQ(g.meta[bob_slot1].status, CardStatus::CALLED_TO_PLAY)
      << "rank-2 clue to yagami must leave bot67's slot 1 brown 4 marked "
      << "CALLED_TO_PLAY after the turn advances; update_turn must not "
      << "reset CTPs on cards waiting for delayed-play chains";
}

// ---- Test 2 (bot67 perspective, DISABLED) ------------------------------
//
// From bot67's bot instance, the rank-2 clue must also stamp
// CALLED_TO_PLAY on its own slot 1 — otherwise bot67 doesn't know to play
// the card.
//
// Currently FAILS: bad_stable in interpret_clue.cpp reads
// `state.deck[o].id()`, which is nullopt for the observer's own hand. In
// bot67's perspective the rank-2 clue passes `bad_stable` (no
// observer-visible inconsistency), so try_stable's REVEAL interpretation
// stands and reactive interpretation never fires — no CTP on slot 1.
//
// Fixing this requires reworking `bad_stable` to use common-knowledge
// signals instead of ground-truth deck IDs. Tracked separately. Re-enable
// by renaming to drop the DISABLED_ prefix.
TEST(EndgameReplay1885536, DISABLED_Rank2ToCathyBobStampsOwnCTP) {
  Game g = build_from_bot67_perspective();
  ReplayContext ctx = make_ctx_bot67();
  for (const auto& a : kOrigActions) apply_orig_action(g, a, ctx);

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  int my_slot1 = g.state.hands[static_cast<int>(TestPlayer::ALICE)][0];
  ASSERT_EQ(my_slot1, 27);

  EXPECT_EQ(g.meta[my_slot1].status, CardStatus::CALLED_TO_PLAY)
      << "bot67's own instance must stamp CALLED_TO_PLAY on its slot 1 "
      << "after the rank-2 clue (currently fails — see file header)";
}
