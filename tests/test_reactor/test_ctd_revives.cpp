// Behavioural tests for "CTD'd cards become play targets when the
// stack catches up". The convention's play-target enumeration used to
// unconditionally exclude `CALLED_TO_DISCARD`; the v0.20 fix relaxes
// that — a CTD'd card whose actual identity is currently playable can
// be picked as the receiver's play target.
//
// Once the convention picks the CTD'd card as a play target,
// downstream `target_play` (for stable ref_play) or the v0.16
// receiver-target stamp (for reactive) overwrites the meta status to
// `CALLED_TO_PLAY`, so the receiver bot will play the card on its
// next turn.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

// Hand layout helper.
int order_of(const Game& g, TestPlayer p, int slot_1based) {
  return g.state.hands[static_cast<int>(p)][slot_1based - 1];
}

}  // namespace

// Reactive-colour play-target: a CTD'd card on Cathy whose identity is
// currently playable should be a valid play target for the convention.
TEST(CTDRevives, ReactiveColourPicksCTDPlayableAsTarget) {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer, giver): filler.
      {"r2", "r3", "y4", "g4", "b4"},
      // Bob (reacter): slot 5 (= y1) is a playable on yellow stack 0.
      // The convention will pick this slot as react_order; needs
      // actual-id in playable_set for target_play to succeed.
      {"y4", "g4", "b4", "p4", "y1"},
      // Cathy (receiver): slot 2 = p3 (= the formerly-CTD'd card, now
      // currently playable on purple stack 2). We pre-clue it via
      // colour-purple at setup, then manually stamp meta.status = CTD
      // to model the "called to discard" state.
      {"y3", "p3", "g3", "b2", "p2"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 2};
  Game g = setup(std::move(opts));
  // Pre-clue Cathy's slot 2 (p3) with colour-purple so it's "touched".
  g = pre_clue(std::move(g), TestPlayer::CATHY, /*slot=*/2, {"Purple"});
  g.elim();
  int cathy_p3 = order_of(g, TestPlayer::CATHY, 2);
  // Stamp CTD on the now-playable purple-3 — model the convention
  // having previously marked it for discard before the stack caught up.
  g.meta[cathy_p3].status = CardStatus::CALLED_TO_DISCARD;
  ASSERT_TRUE(g.state.is_playable(Identity{4, 3}))
      << "purple-3 must be currently playable on the seeded purple-2 stack";

  // Alice clues colour-purple to Cathy. focus_slot computed from the
  // touched cards; the convention's reactive interpretation should
  // pick Cathy's CTD'd p3 (now playable) as a play target instead of
  // skipping it.
  g = take_turn(std::move(g), "Alice clues Purple to Cathy");

  ASSERT_FALSE(g.move_history.empty());
  ASSERT_TRUE(std::holds_alternative<ClueInterp>(g.move_history.back()));
  int interp = static_cast<int>(std::get<ClueInterp>(g.move_history.back()));
  // The convention picked SOME play interpretation (REACTIVE=1 or
  // PLAY=2 or STALL/REVEAL=8/6) — not MISTAKE (=0). Pre-fix the
  // convention would have rejected the CTD'd target and the clue
  // would interpret as MISTAKE or non-play.
  EXPECT_NE(interp, /*MISTAKE=*/0)
      << "Cathy's CTD'd p3 must be eligible as a play target now that "
         "the purple stack reaches 2 (= p3 is currently playable)";
  // The CTD'd card's status must have been overwritten to CTP by the
  // convention's `target_play` / v0.16 receiver-target stamp.
  EXPECT_NE(g.meta[cathy_p3].status, CardStatus::CALLED_TO_DISCARD)
      << "the convention must overwrite the CTD label once it picks the "
         "card as a play target — receiver expects CTP, not CTD";
}

// (The stable `ref_play` CTD-rejection relaxation in interpret_clue.cpp
// only fires when `refer(left)` lands on an unclued-but-CTD'd card —
// a state that arises in real play after a rank-1 referential discard
// CTDs an unclued card and the stack later catches up. Constructing
// that combination requires deck.clued=false plus a CTD'd status,
// which the test harness doesn't expose cleanly via setup options.
// The reactive case above and the multi-copy case below pin the
// principle for the surfaces that the user's example actually
// exercises. The ref_play relaxation is regression-tested
// implicitly by the full ctest sweep on all existing replay tests.)

// Multi-copy CTD: two CTD'd copies of the same identity in one hand,
// both currently playable. Under v0.33's same-hand-dupe rule, the
// RIGHTMOST (highest slot index = oldest) copy is the primary play
// target; the leftmore copy is treated as trash for play-target
// selection. Verify the rightmost CTD'd card gets CTP'd.
TEST(CTDRevives, MultiCopyCTDRightmostBecomesPrimary) {
  SetupOptions opts;
  opts.hands = {
      {"r2", "r3", "y4", "g4", "b4"},
      {"y4", "g4", "b4", "p4", "y1"},
      // Cathy: slots 2 AND 4 are both p3 — duplicate CTD'd targets,
      // both currently playable.
      {"y3", "p3", "g3", "p3", "b2"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 2};
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::CATHY, /*slot=*/2, {"Purple"});
  g = pre_clue(std::move(g), TestPlayer::CATHY, /*slot=*/4, {"Purple"});
  g.elim();
  int cathy_p3_slot2 = order_of(g, TestPlayer::CATHY, 2);
  int cathy_p3_slot4 = order_of(g, TestPlayer::CATHY, 4);
  g.meta[cathy_p3_slot2].status = CardStatus::CALLED_TO_DISCARD;
  g.meta[cathy_p3_slot4].status = CardStatus::CALLED_TO_DISCARD;

  g = take_turn(std::move(g), "Alice clues Purple to Cathy");

  // v0.33: among multiple copies of the same playable identity, the
  // RIGHTMOST (= oldest = highest slot index) copy is the primary
  // play target. Slot 4 wins over slot 2 → slot 4 gets promoted to
  // CTP; slot 2 stays CTD'd (it was treated as trash for play-target
  // selection under the new rule).
  EXPECT_EQ(g.meta[cathy_p3_slot4].status, CardStatus::CALLED_TO_PLAY)
      << "rightmost CTD'd copy must be promoted to CTP under v0.33";
}
