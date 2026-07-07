// Convention v0.32: COLOR reactive clues only retarget a different
// card to discard if the new target is genuinely "new trash" — not
// already CTD'd, and not a duplicate of an already-CTD'd card in the
// same hand. When no eligible new trash exists, the convention falls
// through to the existing sacrifice (leftmost-furthest-from-playable)
// pool. The five tests below pin the four scenarios from the
// convention spec plus the "all good" fallback. A sixth test pins
// the behavioural counterpart: when a hand has multiple CTD'd cards,
// the bot always discards the most-recently-signaled one.
//
// All scenarios run on No Variant with play_stacks = {1,1,1,1,1}
// (all 1s played). Alice gives the red clue. Bob is the reacter.
// Cathy is the receiver of both the clue and the dc-target.
//
// Assertion model: the discard-target path of `interpret_reactive_
// colour` stamps the REACTER's slot CTP (via `target_play`) but does
// NOT stamp the receiver's slot — the receiver derives the discard
// target from focus_slot + react_slot via calc_slot. Each test
// therefore asserts which slot of Bob's gets CTP'd; that slot
// uniquely determines the dc-target on Cathy via the inverse
// calc_slot mapping.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

int order_of(const Game& g, TestPlayer p, int slot) {
  return g.state.hands[static_cast<int>(p)][slot - 1];
}

SetupOptions all_ones_played_base() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{1, 1, 1, 1, 1};
  return opts;
}

// Find which slot of `player`'s hand is currently CALLED_TO_PLAY.
// Returns -1 if none is.
int find_ctp_slot(const Game& g, TestPlayer player) {
  int pi = static_cast<int>(player);
  for (size_t i = 0; i < g.state.hands[pi].size(); ++i) {
    int o = g.state.hands[pi][i];
    if (g.meta[o].status == CardStatus::CALLED_TO_PLAY) {
      return static_cast<int>(i) + 1;
    }
  }
  return -1;
}

}  // namespace

// Example 1: Cathy's only trash is g1 (color-clued green previously).
// No "new" unclued trash exists, so the convention re-targets the
// already-known g1 rather than falling through to a sacrifice. With
// focus=1 (newly-clued r5) and target_slot=2 (g1), Bob's react_slot
// = calc_slot(1,2,5) = 4. Bob's slot 4 (r2) is currently playable.
TEST(DCTargetRetarget, Example1RetargetsToOnlyKnownTrash) {
  SetupOptions opts = all_ones_played_base();
  opts.hands = {
      {"y2", "y3", "y4", "p2", "p3"},   // Alice: filler.
      {"r3", "r4", "g3", "r2", "g4"},   // Bob: slot 4 = r2 (playable).
      {"r5", "g1", "b3", "g4", "g5"},   // Cathy: slot 2 = g1 (trash).
  };
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::CATHY, /*slot=*/2, {"Green"});

  g = take_turn(std::move(g), "Alice clues Red to Cathy");

  EXPECT_EQ(find_ctp_slot(g, TestPlayer::BOB), 4)
      << "g1 (Cathy slot 2) is the only trash; with focus=1 the "
         "convention maps target_slot=2 → react_slot=4. Bob's slot 4 "
         "(r2) is currently playable and must be CTP'd";
}

// Example 2 (v1.4, replay 1916813): Cathy holds the clued-but-not-
// globally-known trash g1 (colour-clued only — empathy {g1..g5}) AND an
// unclued same-hand-duplicated b3 on slot 3. Per the dc-target rule the
// LEFTMOST CLUED not-known trash outranks unclued trash: marking it
// teaches Cathy that a card she was keeping for its clue is trash.
// focus=1 + target=2 → react_slot = calc_slot(1,2,5) = 4.
TEST(DCTargetRetarget, Example2PrefersCluedUnknownTrashOverUncluedDup) {
  SetupOptions opts = all_ones_played_base();
  opts.hands = {
      {"y2", "y3", "y4", "p2", "p3"},
      {"r3", "r4", "r2", "g3", "g4"},
      {"r5", "g1", "b3", "b3", "b4"},   // Cathy: slots 3,4 = b3 dup.
  };
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::CATHY, /*slot=*/2, {"Green"});

  g = take_turn(std::move(g), "Alice clues Red to Cathy");

  EXPECT_EQ(find_ctp_slot(g, TestPlayer::BOB), 4)
      << "Cathy's slot 2 (g1, clued but not globally known trash) is the "
         "convention's dc target — clued-unknown trash outranks the "
         "unclued b3 dup. focus=1 + target=2 → react_slot=4";
}

// Example 3: Cathy's slot 2 (g3) was previously CTD'd. The new
// convention forbids re-CTD'ing the same slot, so the pre_clued_trash
// candidate (g3) is filtered out. With no other trash candidates the
// pool falls back to sacrifices, picking the leftmost non-critical
// (b4 on slot 1). focus=3 + target=1 → react_slot = calc_slot(3,1,5)
// = 2. Bob's slot 2 (r2) is playable.
TEST(DCTargetRetarget, Example3SkipsAlreadyCTDdSlotFallsBackToSacrifice) {
  SetupOptions opts = all_ones_played_base();
  opts.hands = {
      {"y2", "y3", "y4", "p2", "p3"},
      {"r4", "r2", "r3", "g4", "b4"},   // Bob: slot 2 = r2 (playable).
      {"b4", "g3", "r5", "b5", "p5"},   // Cathy: slot 2 g3 pre-CTD.
  };
  Game g = setup(std::move(opts));
  // Stamp g3 as CTD'd from a prior turn, with a fully-known identity
  // so common-knowledge sees it as trash (order_trash via the CTD
  // status alone is enough; fully_known also pins inferred=singleton
  // so the pre_clued_trash pool finds it).
  g = fully_known(std::move(g), TestPlayer::CATHY, /*slot=*/2, "g3");
  int cathy_g3 = order_of(g, TestPlayer::CATHY, 2);
  g.meta[cathy_g3].status = CardStatus::CALLED_TO_DISCARD;
  g.meta[cathy_g3].signal_turn = 0;
  g.elim();

  g = take_turn(std::move(g), "Alice clues Red to Cathy");

  EXPECT_EQ(find_ctp_slot(g, TestPlayer::BOB), 2)
      << "g3 (already CTD'd) is filtered out; convention falls back "
         "to the leftmost sacrifice (b4 slot 1). focus=3 + target=1 "
         "→ react_slot=2 (Bob's r2)";
}

// Example 4: Cathy's slot 4 (b4) was previously CTD'd. Slot 1 (also
// b4) is the unknown-trash leftmost candidate (same-hand-dup of slot
// 4), but re-targeting it would call BOTH copies of b4 to discard,
// losing the identity entirely. The convention's new filter rejects
// it; the next candidate is y1 (slot 3, basic-trash). focus=2 +
// target=3 → react_slot = calc_slot(2,3,5) = 4. Bob's slot 4 (r2)
// is playable.
TEST(DCTargetRetarget, Example4BlocksDupOfCTDdIdentity) {
  SetupOptions opts = all_ones_played_base();
  opts.hands = {
      {"y2", "y3", "y4", "p2", "p3"},
      {"r4", "g4", "r3", "r2", "y4"},   // Bob: slot 4 = r2 (playable).
      {"b4", "r5", "y1", "b4", "g5"},   // Cathy: slot 1,4 = b4 dup.
  };
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::CATHY, /*slot=*/4, "b4");
  int cathy_b4_old = order_of(g, TestPlayer::CATHY, 4);
  g.meta[cathy_b4_old].status = CardStatus::CALLED_TO_DISCARD;
  g.meta[cathy_b4_old].signal_turn = 0;
  g.elim();

  g = take_turn(std::move(g), "Alice clues Red to Cathy");

  EXPECT_EQ(find_ctp_slot(g, TestPlayer::BOB), 4)
      << "b4 slot 1 is blocked (dup of CTD'd b4 slot 4); next trash "
         "is y1 slot 3. focus=2 + target=3 → react_slot=4 (Bob's r2)";
}

// Example 5: Cathy holds no trash, no CTD, no duplicates. Convention
// falls through to sacrifices and picks per the existing
// furthest-from-playable / leftmost principle (slot 1 g3 wins the
// sort). focus=5 + target=1 → react_slot = calc_slot(5,1,5) = 4.
// Bob's slot 4 (y2) is playable. Verifies the new code path is a
// pure superset of the old behaviour when no CTD constraints apply.
TEST(DCTargetRetarget, Example5AllGoodFallsThroughToSacrifice) {
  SetupOptions opts = all_ones_played_base();
  opts.hands = {
      {"y4", "p4", "p3", "p2", "p3"},
      {"r4", "g4", "b4", "y2", "p4"},   // Bob: slot 4 = y2 (playable).
      {"g3", "b3", "p3", "y3", "r3"},   // Cathy: all rank-3, no trash.
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues Red to Cathy");

  EXPECT_EQ(find_ctp_slot(g, TestPlayer::BOB), 4)
      << "with no trash/CTD/dup in Cathy's hand the convention picks "
         "the leftmost-furthest sacrifice (g3 slot 1). focus=5 + "
         "target=1 → react_slot=4 (Bob's y2)";
}

// Behavioural counterpart: when the bot's hand has multiple CTD'd
// cards (an older CTD from a prior turn plus a fresher CTD just
// signalled), the bot's discard pick is the MOST-RECENTLY signalled
// one. Older CTDs remain CTD'd for future turns once the newer one
// is consumed.
TEST(DCTargetRetarget, DiscardsMostRecentlySignaledCTD) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;  // Alice = our_player_index = 0.
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 0};
  opts.clue_tokens = 0;  // Force discard branch.
  opts.hands = {
      // Alice holds two CTD'd cards: slot 4 (older signal) and slot 1
      // (newer signal). Both have singleton-inferred identities so
      // the discard branch's expected pool sees them as safe-trash.
      {"y4", "g4", "b4", "p4", "r4"},
      {"y2", "g2", "b2", "p2", "y1"},
      {"y3", "g3", "b3", "p3", "y2"},
  };
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y4");
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/4, "p4");
  int new_ctd = order_of(g, TestPlayer::ALICE, 1);
  int old_ctd = order_of(g, TestPlayer::ALICE, 4);
  // Old CTD was signalled on turn 1; new CTD on turn 5.
  g.meta[old_ctd].status = CardStatus::CALLED_TO_DISCARD;
  g.meta[old_ctd].signal_turn = 1;
  g.meta[new_ctd].status = CardStatus::CALLED_TO_DISCARD;
  g.meta[new_ctd].signal_turn = 5;
  g.elim();

  PerformAction action = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformDiscard>(action))
      << "with ct=0 and two CTD'd safe-trash cards, the bot must "
         "discard one of them";
  EXPECT_EQ(std::get<PerformDiscard>(action).target, new_ctd)
      << "between two CTD'd cards the bot must pick the one with the "
         "most recent signal_turn (slot 1, signal_turn=5) over the "
         "older signal_turn=1 on slot 4";
}
