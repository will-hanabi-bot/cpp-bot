// Defect C (findings #6/#8). POV discipline: giver-only knowledge may only
// REJECT a clue, never redirect it to a different target -- otherwise the
// seats decode the same clue differently and the table desyncs.
//
// `would_lose_inverted_reacter` (src/conventions/variants/inverted.cpp:49-51)
// reads `state.deck[react_order].id()`, which is nullopt for the card's own
// holder. The giver sees Bob's orange and `continue`s to another target; Bob
// cannot see his own card, the guard returns false for him, and he computes
// the ORIGINAL target. Phase B already gets this right by returning nullopt
// (interpret_reactive.cpp:252-255); Phase A and colour mode 1 do not.
//
// Phase A additionally has no giver veto at all on a visibly unplayable react
// card, unlike colour mode 2 -- so it offers a clue that must strike.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

TEST(Reactor0PovReject, InvertedReacterSlotRejectsRatherThanRetargets) {
  SetupOptions opts;
  // "Orange (3 Suits)": Red(0) Blue(1) Orange(2), orange is inverted.
  // Cathy has playables at slot 1 (r1) and slot 3 (b1). Red = anchor 1, so
  // slot 1 -> react_slot calc_slot(1,1,5) = 5 and slot 3 -> react_slot 3.
  // Bob's slot 5 is a NON-playable orange: the giver sees it and bails.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b2", "b3", "r2", "b4", "o3"},   // Bob: slot 5 = o3, unplayable orange
      {"r1", "o2", "b1", "o4", "r4"},   // Cathy: playables slots 1 and 3
  };
  opts.variant_name = "Orange (3 Suits)";
  use_reactor0(opts);
  Game g = setup(opts);
  ASSERT_TRUE(g.state.variant->suits[2].suit_type.inverted)
      << "fixture assumes suit 2 is the inverted (Orange) suit";

  auto before = hand_marks(g, TestPlayer::BOB);
  g = take_turn(std::move(g), "Alice clues red to Cathy");

  // The giver may only reject on knowledge Bob lacks. Silently moving to
  // Cathy's slot-3 target is the desync: Bob would still compute slot 1.
  EXPECT_EQ(last_clue_interp(g), ClueInterp::MISTAKE)
      << "giver-only knowledge must reject the clue, not pick another target";
  EXPECT_TRUE(newly_marked(before, g, TestPlayer::BOB).empty())
      << "a rejected clue must leave the reacter unstamped";
}

TEST(Reactor0PovReject, RankPhaseAVetoesVisiblyUnplayableReactCard) {
  SetupOptions opts;
  // Cathy's only playable is r1 at slot 2. Rank 3 = anchor 3, so
  // react_slot = calc_slot(3,2,5) = 1. Bob's slot 1 is pre-clued "4", so
  // common empathy allows a playable (b1 is not in {4}s... ) -- we instead
  // rely on the actual id: Bob's slot 1 is y4, visibly NOT playable and not
  // a connector, but its common `possible` still admits playables.
  opts.play_stacks = {{0, 0, 0, 0, 0}};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y4", "y3", "b3", "g4", "p4"},   // Bob: slot 1 = y4, unplayable
      {"g3", "r1", "b4", "p3", "y2"},   // Cathy: r1 playable at slot 2
  };
  use_reactor0(opts);
  Game g = setup(opts);

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  // Colour mode 2 and rank Phase B both veto a visibly unworkable react
  // card. Phase A must too, or the giver hands out a clue that strikes.
  if (last_clue_interp(g) == ClueInterp::REACTIVE) {
    int react = g.waiting.empty() ? -1 : g.waiting.front().react_order;
    ASSERT_GE(react, 0);
    auto id = g.state.deck[react].id();
    ASSERT_TRUE(id.has_value()) << "giver can see the reacter's card";
    EXPECT_TRUE(g.state.is_playable(*id))
        << "Phase A must not call the reacter to blind-play a card the "
           "giver can see is unplayable";
  }
}
