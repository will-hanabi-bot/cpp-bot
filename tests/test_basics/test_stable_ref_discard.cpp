// Basic / regression tests for Reactor's stable rank-clue
// referential-discard convention. Captures the contract that the
// rest of the convention layer relies on: when Alice gives a rank
// clue to Bob whose interpretation lands in the stable path,
// ref_discard stamps `CALLED_TO_DISCARD` on the first unclued slot
// to the *right* of the focus.
//
// `RankClueMarksNextUncluedSlotAsCTD` is the basic-coverage case
// (user-requested in the 1899044 bug report): pin the convention's
// intended slot so future refactors don't silently move the target.
//
// `RankClueWithEmptyInferredOnTargetStillStampsCTD` is the
// regression for the v0.30 fix. The setup mirrors replay 1899044 T16
// in miniature: the target slot has a previously-narrowed inferred
// set whose only id is exactly the rank the new clue is about to
// remove from untouched cards. Without the fix, elim()'s Step 1
// "inferred empty -> reset everything" rule wipes the just-stamped
// CTD; with the fix, reset_inferences is called first so the CTD
// stamp survives.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

// Minimal stable rank-clue ref_discard. Alice clues rank-3 to Bob.
// Bob's slot 1 is the only rank-3 card touched, so focus = slot 1.
// The target_index loop starts at slot_pos+1 and picks the first
// unclued slot, which is slot 2. ref_discard must stamp slot 2 as
// CALLED_TO_DISCARD.
TEST(StableRefDiscard, RankClueMarksNextUncluedSlotAsCTD) {
  SetupOptions opts;
  opts.hands = {
      // Alice (giver / POV). Filler chosen so no rank-3s land in
      // Alice's hand (avoids reflexive touch).
      {"r5", "y5", "g5", "b5", "p5"},
      // Bob (receiver). Slot 1 = r3 (will be the rank-3 focus). All
      // other slots are unclued. The convention's first-unclued-
      // after-focus rule -> slot 2 = CTD target.
      {"r3", "g2", "b1", "p1", "y4"},
      // Cathy filler.
      {"r1", "y1", "g1", "b1", "p1"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  int bob_s1 = g.state.hands[static_cast<int>(TestPlayer::BOB)][0];
  int bob_s2 = g.state.hands[static_cast<int>(TestPlayer::BOB)][1];

  EXPECT_TRUE(g.state.deck[bob_s1].clued)
      << "rank-3 must mark Bob's slot 1 (r3) as clued";
  EXPECT_TRUE(g.meta[bob_s1].focused)
      << "rank-3 ref_discard focus = slot 1";
  EXPECT_EQ(g.meta[bob_s2].status, CardStatus::CALLED_TO_DISCARD)
      << "rank-3 ref_discard must stamp the next unclued slot (slot 2 "
         "= g2) as CALLED_TO_DISCARD. Convention contract: focus + "
         "first-unclued-to-the-right.";
}

// Regression for replay 1899044 T16 (v0.30 fix). The target slot's
// inferred is a singleton {g3} before the clue (mimicking the
// SarcasticLink narrowing that fires at T9 in the original replay).
// Alice's rank-3 clue is untouched on slot 4, so on_clue applies
// `inferred.difference(rank-3 set)` -> inferred becomes empty.
// ref_discard chooses slot 4 as the discard target. Without the fix,
// elim's Step 1 sees inferred empty and resets status to NONE,
// silently dropping the CTD signal. With the fix, ref_discard
// reset_inferences()s first so elim leaves the CTD alone.
//
// To trigger the bug we need:
//  - target slot UNCLUED (so ref_discard's target_index picks it);
//  - target slot UNTOUCHED by the test clue (so on_clue diffs its
//    inferred down rather than narrowing it);
//  - target slot's prev inferred ⊆ touch_set of the clue (so the
//    untouched-diff actually empties it).
//
// We park the singleton on Bob's slot 4 (b1 actual): the rank-3
// clue doesn't touch b1, so slot 4 is the untouched-diff target.
// Bob's slot 1 = r3 supplies the focus; slot 2 = g2 is pre-clued
// (colour clue on a different rank so the rank-3 untouched-diff on
// slot 4 still strips its seeded {g3}); slot 3 = g5 supplies a card
// that isn't touched by either clue but is clued via a manual
// `clued` flag flip to force ref_discard's target_index loop to skip
// slot 3 and land on slot 4. We then seed common.thoughts[slot 4]'s
// inferred to {g3} directly via with_thought.
TEST(StableRefDiscard, RankClueWithEmptyInferredOnTargetStillStampsCTD) {
  SetupOptions opts;
  opts.hands = {
      // Alice (giver / POV).
      {"r5", "y5", "g5", "b5", "p5"},
      // Bob (receiver). Slot 1 = r3 (focus), slot 2 = g2 (pre-clued
      // rank-2 below so slot 2 is clued for ref_discard to skip),
      // slot 3 = b5 (will be manually marked clued without on_clue
      // narrowing — we want the slot-4 inferred untouched until our
      // seed), slot 4 = b1 (target; we seed inferred = {g3}),
      // slot 5 = p1.
      {"r3", "g2", "b5", "b1", "p1"},
      // Cathy filler.
      {"r1", "y1", "g1", "b1", "p1"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;

  Game g = setup(std::move(opts));

  // Manually mark Bob's slots 2 and 3 as clued (clued flag only — we
  // don't want on_clue's narrowing to remove anything from slot 4's
  // possibles). ref_discard's target_index loop will skip those two
  // on the clued check and land on slot 4.
  int bob_s2 = g.state.hands[static_cast<int>(TestPlayer::BOB)][1];
  int bob_s3 = g.state.hands[static_cast<int>(TestPlayer::BOB)][2];
  g.with_card(bob_s2, [](Card& c) { c.clued = true; });
  g.with_card(bob_s3, [](Card& c) { c.clued = true; });

  // Seed Bob's slot 4's inferred to singleton {g3}. This mirrors the
  // SarcasticLink narrowing from replay 1899044 T9 → T13 (g3 was
  // discarded by yagami at T9 as sarcastic; the link narrowed will-
  // bot67's slot-4 candidate to a singleton g3 by T13).
  int bob_s4 = g.state.hands[static_cast<int>(TestPlayer::BOB)][3];
  Identity g3{2, 3};  // suit 2 = green, rank 3.
  g.with_thought(bob_s4, [g3](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(g3);
    return out;
  });

  ASSERT_EQ(g.common.thoughts[bob_s4].inferred.length(), 1u)
      << "test setup must leave Bob's slot 4 inferred as singleton {g3}";
  ASSERT_FALSE(g.state.deck[bob_s4].clued)
      << "test setup must leave Bob's slot 4 unclued";
  ASSERT_TRUE(g.state.deck[bob_s3].clued)
      << "test setup must mark Bob's slot 3 as clued so ref_discard "
         "skips it";

  // Alice clues rank-3 to Bob. Touches slot 1 (r3) only (slot 4 =
  // b1 has rank 1, not rank 3). on_clue's untouched-diff strips
  // {r3,y3,g3,b3,p3} from slot 4's inferred → inferred becomes
  // empty. ref_discard then picks slot 4 (target_index loop skips
  // clued slot 3, lands on unclued slot 4).
  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_EQ(g.meta[bob_s4].status, CardStatus::CALLED_TO_DISCARD)
      << "ref_discard must keep its CTD stamp on Bob's slot 4 even "
         "after elim. Pre-fix bug (replay 1899044 T16): the singleton "
         "inferred {g3} was stripped by on_clue's untouched-diff "
         "(rank-3 removed), ref_discard stamped CTD, elim's Step 1 "
         "then saw inferred empty and reset status to NONE -- "
         "silently dropping the convention signal. v0.30 fix in "
         "src/conventions/reactor/interpret_clue.cpp calls "
         "reset_inferences before stamping CTD so the empty-check "
         "fails when elim runs.";
}
