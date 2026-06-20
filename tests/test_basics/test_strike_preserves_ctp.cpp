// Regression test: a strike (failed discard / bomb) must not reset
// CALLED_TO_PLAY status on cards in any player's hand. The convention
// chain that produced the misplay is broken, but CTPs stamped by
// *unrelated* clues remain valid.
//
// Symptom this guards against: replay 1899552 T9 — wb69 dupe-strikes
// b1, and the bombed-discard handler in `Game::interpret_discard`
// (`src/basics/game.cpp:205-220`) nuked all conv info via `cleared()`,
// including a CTP that the rank-4 clue at T6 had stamped on wb67's
// slot 2 b4. The v0.26 elim-driven CTP preservation runs *after* the
// bombed-discard handler, too late to save the stamp.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

// Setup: Alice (POV) has r1 in slot 1 (trash, will be the bomb).
// Bob's slot 2 = b2 is manually marked CALLED_TO_PLAY (mimicking what
// the convention would do for a separate ref_play / playable_rank clue
// that committed before the strike). After Alice bombs r1, Bob's slot 2
// must still be CTP'd.
TEST(StrikePreservesCTP, BombDoesNotResetCTPOnOtherPlayersCards) {
  SetupOptions opts;
  opts.hands = {
      // Alice (POV). Slot 1 = r1 — Alice will bomb this.
      {"r1", "y2", "g3", "b4", "p4"},
      // Bob. Slot 2 = b2 (will be marked CTP).
      {"r1", "b2", "g4", "y4", "p3"},
      // Cathy filler.
      {"y1", "g2", "b3", "p2", "g4"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  // Default play_stacks (all 0) — r1 is currently playable, but Alice
  // will explicitly "bomb" rather than "play", so the engine treats it
  // as a misplay (strike+1) regardless of actual playability.
  Game g = setup(std::move(opts));

  // Pre-clue Bob's slot 2 with both colour and rank so common.thoughts'
  // inferred is the singleton {b2}. Then stamp CTP manually — that's
  // the state a `target_play` or `playable_rank` interpretation would
  // produce.
  g = fully_known(std::move(g), TestPlayer::BOB, 2, "b2");
  int bob_s2 = g.state.hands[static_cast<int>(TestPlayer::BOB)][1];
  g.with_meta(bob_s2, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
  });

  ASSERT_EQ(g.meta[bob_s2].status, CardStatus::CALLED_TO_PLAY)
      << "test setup must leave Bob's slot 2 CTP'd";

  // Alice bombs r1. This is a failed discard (the engine respects the
  // `failed=true` flag and increments strikes regardless of whether
  // r1 happens to be basic-trash).
  g = take_turn(std::move(g), "Alice bombs r1 (slot 1)");

  EXPECT_EQ(g.state.strikes, 1) << "Alice's bomb must increment strikes";
  EXPECT_EQ(g.meta[bob_s2].status, CardStatus::CALLED_TO_PLAY)
      << "Bob's slot 2 CTP must survive the strike. The bombed-discard "
         "handler in interpret_discard previously did `m = m.cleared()` "
         "unconditionally, which set status to NONE. Cards explicitly "
         "marked CTP by *unrelated* clues should keep their stamp -- "
         "the v0.26 elim-driven CTP preservation runs only after this "
         "handler, too late to save the stamp.";
}
