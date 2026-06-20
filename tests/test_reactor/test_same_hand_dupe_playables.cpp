// Convention v0.33: in non-finesse reactive interpretation, among
// multiple copies of the same currently-playable identity in the
// receiver's hand, only the RIGHTMOST copy (highest slot index =
// oldest) counts as a primary play target. The lefter copies are
// treated as trash for play-target selection; if no primary react_
// slot resolves on Bob, the convention iterates the excluded dupes
// right-to-left as a fallback. The finesse path is unaffected.
//
// All scenarios run on No Variant with empty stacks. Alice gives
// the clue. Bob is the reacter, Cathy the receiver. Each test
// asserts which slot of Bob's gets CTP'd — that slot uniquely
// determines the receiver's play target via the calc_slot inversion.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

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

SetupOptions empty_stacks_base() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 0};
  return opts;
}

}  // namespace

// Example 1: Cathy `g2 b1 b1 p1 b1`. Three copies of playable b1
// (slots 2, 3, 5). Under v0.33 only the RIGHTMOST b1 (slot 5) is a
// primary play target; slots 2 and 3 are treated as trash. The
// resulting primary set = {p1@4, b1@5}, leftmost-by-slot = p1@4.
// Alice clues rank-2 (focuses Cathy slot 1 = g2). target_slot=4,
// focus=1 → react_slot = calc_slot(1,4,5) = 2. Bob's slot 2 (y1) is
// playable on the yellow stack.
TEST(SameHandDupePlayables, Example1MultipleDupesLeftmostBecomesP1) {
  SetupOptions opts = empty_stacks_base();
  opts.hands = {
      {"r4", "y4", "g4", "p4", "p3"},   // Alice: filler (no rank-2).
      {"r5", "y1", "p5", "g5", "g4"},   // Bob: slot 2 = y1 (playable).
      {"g2", "b1", "b1", "p1", "b1"},   // Cathy: 3 b1 copies + p1.
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  EXPECT_EQ(find_ctp_slot(g, TestPlayer::BOB), 2)
      << "b1 slots 2 and 3 are treated as trash; primary playables = "
         "p1@4, b1@5. Leftmost primary = p1@4 → focus=1 + target=4 → "
         "react_slot=2 (Bob's y1)";
}

// Example 2: Cathy `g2 g3 g1 g1 b1`. Two copies of playable g1
// (slots 3, 4). Under v0.33 only g1@4 is primary; g1@3 is trash.
// Primary set = {g1@4, b1@5}. Leftmost = g1@4. focus=1 + target=4 →
// react_slot=2 (Bob's y1).
TEST(SameHandDupePlayables, Example2SingleDupeLeftmostBecomesG1) {
  SetupOptions opts = empty_stacks_base();
  opts.hands = {
      {"r4", "y4", "g4", "p4", "p3"},
      {"r5", "y1", "p5", "g5", "g4"},   // Bob: slot 2 = y1.
      {"g2", "g3", "g1", "g1", "b1"},   // Cathy: 2 g1 copies.
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  EXPECT_EQ(find_ctp_slot(g, TestPlayer::BOB), 2)
      << "g1@3 is the leftmost copy of a duped playable → trash. "
         "Primary playables = g1@4, b1@5. Leftmost primary = g1@4 → "
         "focus=1 + target=4 → react_slot=2";
}

// Example 3.1: Cathy `r4 r3 r1 r1 b1`. r1 has two copies (slots 3,
// 4); r1@4 is primary, r1@3 is dupe. b1@5 is unique primary.
// Primary list (slot ASC): r1@4, b1@5. Bob `p1 r5(rank-clued) r3 p4
// p3`: Bob's slot 2 = r5 is clued rank-5 — possible narrowed to
// rank-5 ids, none playable. The primary r1@4 → react_slot=2 maps
// to Bob's r5, which fails the "possible includes a playable" check
// → skip. Next primary b1@5 → react_slot=calc_slot(1,5,5)=1. Bob's
// slot 1 = p1 (playable on p stack=0).
TEST(SameHandDupePlayables, Example3_1RetargetsThroughBlockedReactSlot) {
  SetupOptions opts = empty_stacks_base();
  opts.hands = {
      {"y4", "y3", "g4", "g3", "p2"},
      {"p1", "r5", "r3", "p4", "p3"},   // Bob: slot 2 = r5 (rank-clued, blocked).
      {"r4", "r3", "r1", "r1", "b1"},   // Cathy: 2 r1 copies + b1.
  };
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, /*slot=*/2, {"5"});

  g = take_turn(std::move(g), "Alice clues 4 to Cathy");

  EXPECT_EQ(find_ctp_slot(g, TestPlayer::BOB), 1)
      << "r1@4 primary blocked because Bob's react_slot=2 (r5) is "
         "rank-clued to unplayable rank-5. Next primary b1@5 → "
         "react_slot=1 (Bob's p1, playable)";
}

// Example 3.2: Cathy `r4 r1 r1 r1 b3`. r1 has three copies (slots
// 2, 3, 4); r1@4 primary, r1@2 and r1@3 dupes (b3 is not playable).
// Primary list: [r1@4]. r1@4 → react_slot=2 → Bob's r5 (blocked).
// Fall through to dupes right-to-left: try r1@3 → react_slot=3 →
// Bob's slot 3 = p1 (playable). Bob plays slot 3.
TEST(SameHandDupePlayables, Example3_2DupeFallbackRightToLeft) {
  SetupOptions opts = empty_stacks_base();
  opts.hands = {
      {"y4", "y3", "g4", "g3", "p2"},
      {"r3", "r5", "p1", "p4", "p3"},   // Bob: slot 3 = p1 (playable).
      {"r4", "r1", "r1", "r1", "b3"},   // Cathy: 3 r1 copies.
  };
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, /*slot=*/2, {"5"});

  g = take_turn(std::move(g), "Alice clues 4 to Cathy");

  EXPECT_EQ(find_ctp_slot(g, TestPlayer::BOB), 3)
      << "Only primary r1@4 → react_slot=2 (Bob's blocked r5). "
         "Fallback iterates dupes right-to-left: r1@3 → react_slot=3 "
         "(Bob's p1)";
}

// Example 4: finesse path is unaffected. Cathy `r2 b5 r2 r3 r4`.
// No card in Cathy is currently playable (r2 needs r1, b5 needs
// b4 etc.) → primary/dupe play_targets both empty → fall through
// to finesse_targets. Alice clues rank-5 (focus=2, b5). The
// finesse fallback iterates react_slot in {1, 5, 4, 3, 2}; react=1
// → target_slot=calc_slot(2,1,5)=1 → Cathy slot 1 (r2,
// playable_away=1). Bob's slot 1 = r1 (playable on r stack=0) → r1
// is the finesse prereq → target_play stamps Bob's slot 1 CTP.
TEST(SameHandDupePlayables, Example4FinessePathUnaffected) {
  SetupOptions opts = empty_stacks_base();
  opts.hands = {
      {"y2", "y3", "g4", "p4", "p3"},
      {"r1", "b1", "g1", "y1", "p1"},   // Bob: slot 1 = r1 (playable).
      {"r2", "b5", "r2", "r3", "r4"},   // Cathy: no currently-playable.
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 5 to Cathy");

  EXPECT_EQ(find_ctp_slot(g, TestPlayer::BOB), 1)
      << "no primary play_targets in Cathy → finesse fallback fires; "
         "react=1 maps to Cathy slot 1 (r2 playable_away=1) and Bob's "
         "r1 is the prereq → Bob slot 1 CTP'd";
}
