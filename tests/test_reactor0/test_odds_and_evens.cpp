// Reactor0 under Odds and Evens: the two clue kinds swap reactive roles.
//
//   normally   RANK   = even parity -- double play, or double discard
//              COLOUR = odd parity  -- exactly one play
//   O&E        COLOUR = even parity
//              RANK   = odd parity
//
// and a rank clue's value names a parity rather than a rank, so it maps
// odd (1) -> anchor 3, even (2) -> anchor 4. Colour anchors are unchanged.
//
// Each test here has a mirror on the same fixture in a plain variant, so what
// is pinned is the SWAP rather than any particular reading.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor0/colour_value.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Alice gives, Bob reacts, Cathy receives -- reactor0's positional dispatch,
// so a clue to Cathy is reactive. Stacks are all 0, so Cathy's leftmost
// playable is her slot 1 and Bob has playables to be called onto.
SetupOptions oe_opts(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r4", "y4", "g4", "b4", "p4"},  // Alice (giver, us)
      {"r1", "y1", "g1", "b1", "p1"},  // Bob   (reacter) -- all playable
      {"r1", "y2", "g3", "b4", "p5"},  // Cathy (receiver) -- red for the
                                       // colour clue, playable r1 on slot 1
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

// --- the anchor -----------------------------------------------------------

TEST(Reactor0OddsAndEvens, RankAnchorIsThreeForOddAndFourForEven) {
  Game g = setup(oe_opts("Odds and Evens (5 Suits)"));
  g = take_turn(std::move(g), "Alice clues 1 to Cathy");
  ASSERT_FALSE(g.waiting.empty()) << "an odd clue to Cathy must read reactive";
  EXPECT_EQ(g.waiting.front().focus_slot, 3) << "odd -> 3";

  Game g2 = setup(oe_opts("Odds and Evens (5 Suits)"));
  g2 = take_turn(std::move(g2), "Alice clues 2 to Cathy");
  ASSERT_FALSE(g2.waiting.empty());
  EXPECT_EQ(g2.waiting.front().focus_slot, 4) << "even -> 4";
}

// The mirror: in a plain variant the anchor is still the rank itself.
TEST(Reactor0OddsAndEvens, PlainVariantRankAnchorIsStillTheRank) {
  Game g = setup(oe_opts("No Variant"));
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().focus_slot, 2) << "the clue value IS the rank";
}

// Colour anchors are untouched by the variant: same colour, same value.
TEST(Reactor0OddsAndEvens, ColourAnchorsAreUnchanged) {
  const Variant& oe = get_variant("Odds and Evens (5 Suits)");
  const Variant& plain = get_variant("No Variant");
  for (int c = 0; c < 5; ++c) {
    EXPECT_EQ(hanabi::reactor0::colour_clue_value(oe, c),
              hanabi::reactor0::colour_clue_value(plain, c))
        << "colour " << c << " must anchor identically in both variants";
  }
}

// --- the parity swap ------------------------------------------------------
//
// The observable difference between the two rulesets is what gets stamped.
// The even-parity family calls a play on BOTH seats (double play) or a discard
// on both; the odd-parity family calls exactly one play.

namespace {

// How many cards in one hand carry a given call.
int count_status(const Game& g, TestPlayer who, CardStatus st) {
  int n = 0;
  for (int o : g.state.hands[static_cast<int>(who)]) {
    if (g.meta[o].status == st) ++n;
  }
  return n;
}

// Total PLAY calls the clue made across the reacter and the receiver. This is
// the convention's own way of naming the two rulesets -- /settings calls them
// "odd plays" and "even plays" -- and unlike any single stamp it does not
// depend on WHICH seat the odd-parity family decided to give the play to.
int play_calls(const Game& g) {
  return count_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY) +
         count_status(g, TestPlayer::CATHY, CardStatus::CALLED_TO_PLAY);
}

}  // namespace

// A COLOUR clue under O&E runs the ruleset a RANK clue normally would: an EVEN
// number of plays (two, or none).
TEST(Reactor0OddsAndEvens, ColourClueTakesTheEvenParityRuleset) {
  Game oe = setup(oe_opts("Odds and Evens (5 Suits)"));
  oe = take_turn(std::move(oe), "Alice clues red to Cathy");
  ASSERT_EQ(last_clue_interp(oe), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(oe) % 2, 0)
      << "O&E colour is the even-parity family; got " << play_calls(oe)
      << " play calls";

  // The mirror on the identical fixture: in a plain variant the same clue is
  // the ODD-parity family, so it calls exactly one play.
  Game plain = setup(oe_opts("No Variant"));
  plain = take_turn(std::move(plain), "Alice clues red to Cathy");
  ASSERT_EQ(last_clue_interp(plain), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(plain), 1)
      << "plain colour is the odd-parity family; got " << play_calls(plain);
}

// ...and a RANK clue under O&E runs the ruleset a COLOUR clue normally would:
// exactly one play.
TEST(Reactor0OddsAndEvens, RankClueTakesTheOddParityRuleset) {
  Game oe = setup(oe_opts("Odds and Evens (5 Suits)"));
  oe = take_turn(std::move(oe), "Alice clues 1 to Cathy");
  ASSERT_EQ(last_clue_interp(oe), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(oe), 1)
      << "O&E rank is the odd-parity family; got " << play_calls(oe);

  Game plain = setup(oe_opts("No Variant"));
  plain = take_turn(std::move(plain), "Alice clues 1 to Cathy");
  ASSERT_EQ(last_clue_interp(plain), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(plain) % 2, 0)
      << "plain rank is the even-parity family; got " << play_calls(plain);
}
