// Defect D (finding #2). The finesse picks its target with the
// direction-aware `state.playable_away(id) == 1` (state.h:116-121 flips for
// reversed suits) but derives the connector with `Identity::prev()`
// (identity.h:33), which is unconditionally rank-1.
//
// On a reversed suit the card that makes a one-away playable is next(), not
// prev(): with the stack at its 6 sentinel, p5 is playable and p4 is one
// away, and playing p5 is what brings p4 into range. Computing prev() gives
// p3 -- a card that can never enable p4 -- so the giver either rejects a
// correct finesse or pins the reacter to a guaranteed strike.
//
// Note: a reversed suit has NO colour clue (its name is absent from
// data/suits.json, so clue_colors is empty), and the harness cannot parse
// "purple reversed" as a colour. This test therefore drives the rank path,
// which is where the finesse lives anyway. play_stacks is left unset because
// the harness pre-seed is not reversed-aware (see tests/test_reactor/
// test_reversed.cpp:78-86).
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

TEST(Reactor0ReversedFinesse, ConnectorIsNextNotPrevOnReversedSuit) {
  SetupOptions opts;
  // "Reversed (5 Suits)": Red Yellow Green Blue + Purple Reversed (idx 4).
  // Stacks start empty; the reversed purple stack is the 6 sentinel, so p5
  // is playable and p4 is exactly one away.
  // Cathy's ONLY one-away card must be the reversed p4 — Phase B walks
  // targets leftmost-first, so any earlier one-away (a plain rank 2 off an
  // empty stack) would be tried first and decide the clue before the
  // reversed suit is ever reached.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "p5", "g3", "b3", "y4"},   // Bob: slot 2 = p5, the connector
      {"r4", "p4", "b4", "y4", "g4"},   // Cathy: slot 2 = p4, only one-away
  };
  opts.variant_name = "Reversed (5 Suits)";
  use_reactor0(opts);
  Game g = setup(opts);

  ASSERT_TRUE(g.state.is_playable(Identity{4, 5}))
      << "reversed purple: p5 playable off the 6 sentinel";
  ASSERT_EQ(g.state.playable_away(Identity{4, 4}), 1)
      << "reversed purple: p4 is the one-away finesse target";

  // Rank 4 = anchor 4. Cathy's one-away p4 sits at slot 2, so
  // react_slot = calc_slot(4,2,5) = 2 -- Bob's p5, the true connector.
  g = take_turn(std::move(g), "Alice clues 4 to Cathy");

  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE)
      << "the p5->p4 finesse is legal and must be found; computing the "
         "connector as prev() (p3) rejects it";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 2));
  // The connector must be pinned to p5, never p3.
  expect_infs(g, std::nullopt, TestPlayer::BOB, 2, {"p5"});
}
