// Synesthesia: a card of rank N is ALSO touched by the Nth colour clue.
//
// The variant carries `clueRanks: []`, so colour is the only clue kind on
// offer; the rank rule is how rank information reaches the table at all. Two
// carve-outs, and the tests below pin both because they come from different
// places in `Variant::id_touched`:
//
//   BROWN  is excluded by the rule itself -- a brown card answers to `Brown`
//          and never to the colour of its rank.
//   WHITE  is excluded by SITTING BELOW the existing whitish early-return, so
//          it is untouched by every colour clue including the rank one. That
//          matches how hanab.live currently behaves: White in a Synesthesia
//          variant is indistinguishable from Null.
//
// The clue VALUE is 0-indexed into `clue_colour_names` while the rank is
// 1-indexed, so the rule reads `rank - 1 == value`: a 1 is touched by the first
// colour (value 0), a 5 by the fifth (value 4). Every fixture below states the
// value it is passing so that off-by-one is visible rather than assumed.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

// Suit index of `name` in the variant, or -1.
int suit_of(const Variant& v, const std::string& name) {
  for (size_t i = 0; i < v.suits.size(); ++i) {
    if (v.suits[i].name == name) return static_cast<int>(i);
  }
  return -1;
}

// Colour clue value that names `name`, or -1.
int colour_of(const Variant& v, const std::string& name) {
  for (size_t i = 0; i < v.clue_colour_names.size(); ++i) {
    if (v.clue_colour_names[i] == name) return static_cast<int>(i);
  }
  return -1;
}

bool touched(const Variant& v, int suit, int rank, int colour_value) {
  return v.id_touched(Identity{suit, rank}, ClueKind::COLOUR, colour_value);
}

}  // namespace

// --- an ordinary suit -----------------------------------------------------

// The whole point of the variant: a Red 3 answers to Red AND to the third
// colour. Green is third in Red/Yellow/Green/Blue/Purple, so a Red 3 is touched
// by a Green clue.
TEST(Synesthesia, AnOrdinarySuitAnswersToItsOwnColourAndItsRanks) {
  const Variant& v = get_variant("Synesthesia (5 Suits)");
  ASSERT_TRUE(v.synesthesia) << "guard: the flag is parsed";
  const int red = suit_of(v, "Red");
  ASSERT_GE(red, 0);

  EXPECT_TRUE(touched(v, red, 3, colour_of(v, "Red")))
      << "its own colour still touches it";
  EXPECT_TRUE(touched(v, red, 3, colour_of(v, "Green")))
      << "rank 3 -> the third colour, which is Green";
  EXPECT_FALSE(touched(v, red, 3, colour_of(v, "Blue")))
      << "and nothing else: Blue is neither its colour nor its rank's";
}

// The 1-indexed rank against the 0-indexed clue value, at both ends.
TEST(Synesthesia, TheRankToColourMapIsOffByOne) {
  const Variant& v = get_variant("Synesthesia (5 Suits)");
  const int blue = suit_of(v, "Blue");
  ASSERT_GE(blue, 0);

  EXPECT_TRUE(touched(v, blue, 1, 0)) << "a 1 is the FIRST colour, value 0";
  EXPECT_FALSE(touched(v, blue, 1, 1)) << "not the second";
  EXPECT_TRUE(touched(v, blue, 5, 4)) << "a 5 is the FIFTH colour, value 4";
  EXPECT_FALSE(touched(v, blue, 5, 5)) << "there is no sixth colour here";
}

// A plain variant must be untouched by any of this.
TEST(Synesthesia, APlainVariantDoesNotGainTheRankRule) {
  const Variant& v = get_variant("No Variant");
  ASSERT_FALSE(v.synesthesia);
  const int red = suit_of(v, "Red");
  EXPECT_FALSE(touched(v, red, 3, colour_of(v, "Green")))
      << "outside Synesthesia a Red 3 answers to Red alone";
}

// --- brown: exempt by the rule --------------------------------------------

TEST(Synesthesia, BrownAnswersOnlyToBrown) {
  const Variant& v = get_variant("Synesthesia & Brown (5 Suits)");
  ASSERT_TRUE(v.synesthesia);
  const int brown = suit_of(v, "Brown");
  ASSERT_GE(brown, 0);
  const int third = colour_of(v, "Green");
  ASSERT_GE(third, 0) << "guard: Green is the third colour here too";

  EXPECT_TRUE(touched(v, brown, 3, colour_of(v, "Brown")))
      << "Brown still touches a brown card";
  EXPECT_FALSE(touched(v, brown, 3, third))
      << "but a brown 3 must NOT answer to the third colour";
  // The control: the same rank in a non-brown suit in the same variant does.
  EXPECT_TRUE(touched(v, suit_of(v, "Red"), 3, third))
      << "the rule is alive in this variant -- brown is the exception";
}

// --- white and null: exempt by position -----------------------------------

// White is "no colour clue touches it", and under Synesthesia that wins: the
// rank rule sits below the whitish early-return, so a White 3 is unreachable.
TEST(Synesthesia, WhiteIsUntouchedByItsRanksColourToo) {
  const Variant& v = get_variant("Synesthesia & White (5 Suits)");
  ASSERT_TRUE(v.synesthesia);
  const int white = suit_of(v, "White");
  ASSERT_GE(white, 0);

  for (int value = 0; value < static_cast<int>(v.clue_colour_names.size());
       ++value) {
    EXPECT_FALSE(touched(v, white, 3, value))
        << "White 3 must be untouched by colour value " << value;
  }
  EXPECT_TRUE(touched(v, suit_of(v, "Red"), 3, colour_of(v, "Green")))
      << "control: the rule is alive in this variant";
}

// Null is whitish AND brownish, so it is excluded twice over -- which is what
// Null is supposed to mean.
TEST(Synesthesia, NullStaysCompletelyUntouchable) {
  const Variant& v = get_variant("Synesthesia & Null (5 Suits)");
  const int null_suit = suit_of(v, "Null");
  ASSERT_GE(null_suit, 0);

  for (int rank = 1; rank <= 5; ++rank) {
    for (int value = 0; value < static_cast<int>(v.clue_colour_names.size());
         ++value) {
      EXPECT_FALSE(touched(v, null_suit, rank, value))
          << "Null " << rank << " touched by colour " << value;
    }
  }
}

// --- rainbow and black ----------------------------------------------------

// Rainbow returns true before the rank rule is reached, so it is unaffected --
// it was already touched by everything.
TEST(Synesthesia, RainbowIsStillTouchedByEveryColour) {
  const Variant& v = get_variant("Synesthesia & Rainbow (5 Suits)");
  const int rainbow = suit_of(v, "Rainbow");
  ASSERT_GE(rainbow, 0);
  for (int value = 0; value < static_cast<int>(v.clue_colour_names.size());
       ++value) {
    EXPECT_TRUE(touched(v, rainbow, 2, value));
  }
}

// Black is an ordinary colour as far as touching goes, so it gets the rule.
TEST(Synesthesia, BlackGetsTheRankRuleLikeAnyOtherColour) {
  const Variant& v = get_variant("Synesthesia & Black (5 Suits)");
  const int black = suit_of(v, "Black");
  ASSERT_GE(black, 0);
  EXPECT_TRUE(touched(v, black, 4, colour_of(v, "Black")));
  EXPECT_TRUE(touched(v, black, 4, 3)) << "rank 4 -> the fourth colour, value 3";
}

// --- clue enumeration -----------------------------------------------------

// `clueRanks: []` is already honoured by `all_valid_clues`; pinned here because
// the convention change downstream depends on colour being the only kind.
TEST(Synesthesia, OffersNoRankCluesAtAll) {
  const Variant& v = get_variant("Synesthesia (5 Suits)");
  EXPECT_TRUE(v.clue_ranks.empty()) << "guard: the Number Mute half of it";

  SetupOptions opts;
  opts.variant_name = "Synesthesia (5 Suits)";
  opts.hands = {
      {"r1", "y2", "g3", "b4", "p5"},
      {"r2", "y3", "g4", "b5", "p1"},
      {"r3", "y4", "g5", "b1", "p2"},
  };
  Game g = setup(std::move(opts));
  for (const Clue& c : g.state.all_valid_clues(1)) {
    EXPECT_EQ(c.kind, ClueKind::COLOUR) << "a rank clue was offered";
  }
}

// And the enumeration reflects the rank rule: in a hand of Red 1..5 a single
// Red clue touches all of them, but so does every other colour, one card each.
TEST(Synesthesia, EveryColourReachesTheHandThroughRank) {
  SetupOptions opts;
  opts.variant_name = "Synesthesia (5 Suits)";
  opts.hands = {
      {"r1", "r2", "r3", "r4", "r5"},  // Alice
      {"r1", "r2", "r3", "r4", "r5"},  // Bob -- one card per rank
      {"y1", "y2", "y3", "y4", "y5"},
  };
  Game g = setup(std::move(opts));
  const State& s = g.state;

  EXPECT_EQ(s.clue_touched(s.hands[1], ClueKind::COLOUR, colour_of(*s.variant, "Red"))
                .size(),
            5u)
      << "Red touches all five as their own colour";
  const int green = colour_of(*s.variant, "Green");
  EXPECT_EQ(s.clue_touched(s.hands[1], ClueKind::COLOUR, green).size(), 1u)
      << "Green touches exactly the 3, through the rank rule";
}
