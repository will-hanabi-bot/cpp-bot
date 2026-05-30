// Port of python-bot/tests/test_basics/test_variants.py.
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/variant.h"

using namespace hanabi;

namespace {

struct SuitTypeCase {
  const char* name;
  bool whitish;
  bool rainbowish;
  bool pinkish;
  bool brownish;
  bool dark;
  bool prism;
  bool muddy;
};

class SuitTypeTest : public ::testing::TestWithParam<SuitTypeCase> {};

}  // namespace

TEST_P(SuitTypeTest, OfName) {
  const auto& p = GetParam();
  SuitType st = SuitType::of_name(p.name);
  EXPECT_EQ(st.whitish, p.whitish);
  EXPECT_EQ(st.rainbowish, p.rainbowish);
  EXPECT_EQ(st.pinkish, p.pinkish);
  EXPECT_EQ(st.brownish, p.brownish);
  EXPECT_EQ(st.dark, p.dark);
  EXPECT_EQ(st.prism, p.prism);
  EXPECT_EQ(st.muddy, p.muddy);
}

INSTANTIATE_TEST_SUITE_P(
    AllSuits, SuitTypeTest,
    ::testing::Values(
        SuitTypeCase{"Red", false, false, false, false, false, false, false},
        SuitTypeCase{"White", true, false, false, false, false, false, false},
        SuitTypeCase{"Rainbow", false, true, false, false, false, false, false},
        SuitTypeCase{"Pink", false, false, true, false, false, false, false},
        SuitTypeCase{"Brown", false, false, false, true, false, false, false},
        SuitTypeCase{"Black", false, false, false, false, true, false, false},
        SuitTypeCase{"Prism", false, false, false, false, false, true, false},
        SuitTypeCase{"Muddy Rainbow", false, true, false, true, false, false, true},
        SuitTypeCase{"Dark Rainbow", false, true, false, false, true, false, false},
        SuitTypeCase{"Cocoa Rainbow", false, true, false, true, true, false, true},
        SuitTypeCase{"Gray", true, false, false, false, true, false, false},
        SuitTypeCase{"Omni", false, true, true, false, false, false, false},
        SuitTypeCase{"Null", true, false, false, true, false, false, false},
        SuitTypeCase{"Light Pink", true, false, true, false, false, false, false}));

// --- Variant loading ---

TEST(Variants, LoadReturnsFullCatalog) {
  const auto& vs = load_variants();
  EXPECT_GT(vs.size(), 2000u);
  EXPECT_TRUE(vs.count("No Variant"));
}

TEST(Variants, GetUnknownThrows) {
  EXPECT_THROW(get_variant("This Variant Does Not Exist"), std::out_of_range);
}

// --- No Variant baseline ---

TEST(Variants, NoVariantStructure) {
  const Variant& v = get_variant("No Variant");
  ASSERT_EQ(v.suits.size(), 5u);
  std::vector<std::string> names;
  for (const auto& s : v.suits) names.push_back(s.name);
  EXPECT_EQ(names,
            (std::vector<std::string>{"Red", "Yellow", "Green", "Blue", "Purple"}));
  EXPECT_EQ(v.colourable_suits().size(), 5u);
  EXPECT_EQ(v.total_cards(), 5 * 10);
}

TEST(Variants, NoVariantCardCount) {
  const Variant& v = get_variant("No Variant");
  EXPECT_EQ(v.card_count(Identity(0, 1)), 3);
  EXPECT_EQ(v.card_count(Identity(0, 2)), 2);
  EXPECT_EQ(v.card_count(Identity(0, 3)), 2);
  EXPECT_EQ(v.card_count(Identity(0, 4)), 2);
  EXPECT_EQ(v.card_count(Identity(0, 5)), 1);
}

TEST(Variants, NoVariantColourClueTouchesOneSuit) {
  const Variant& v = get_variant("No Variant");
  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_TRUE(v.id_touched(Identity(0, rank), ClueKind::COLOUR, 0));
    EXPECT_FALSE(v.id_touched(Identity(1, rank), ClueKind::COLOUR, 0));
  }
}

TEST(Variants, NoVariantRankClueTouchesOneRank) {
  const Variant& v = get_variant("No Variant");
  for (int suit = 0; suit < 5; ++suit) {
    EXPECT_TRUE(v.id_touched(Identity(suit, 3), ClueKind::RANK, 3));
    EXPECT_FALSE(v.id_touched(Identity(suit, 4), ClueKind::RANK, 3));
  }
}

// --- Pink ---

TEST(Variants, PinkPinkishTouchedByEveryRankClue) {
  const Variant& v = get_variant("Pink (5 Suits)");
  const int pink_index = 4;
  for (int clue_rank = 1; clue_rank <= 5; ++clue_rank) {
    for (int actual_rank = 1; actual_rank <= 5; ++actual_rank) {
      EXPECT_TRUE(v.id_touched(Identity(pink_index, actual_rank), ClueKind::RANK, clue_rank));
    }
  }
}

TEST(Variants, PinkNormalSuitsUnaffectedByRankClue) {
  const Variant& v = get_variant("Pink (5 Suits)");
  EXPECT_TRUE(v.id_touched(Identity(0, 3), ClueKind::RANK, 3));
  EXPECT_FALSE(v.id_touched(Identity(0, 4), ClueKind::RANK, 3));
}

// --- Rainbow ---

TEST(Variants, RainbowRainbowishTouchedByEveryColourClue) {
  const Variant& v = get_variant("Rainbow (5 Suits)");
  const int rainbow_index = 4;
  EXPECT_EQ(v.colourable_suits().size(), 4u);
  for (size_t clue_color = 0; clue_color < v.colourable_suits().size(); ++clue_color) {
    for (int rank = 1; rank <= 5; ++rank) {
      EXPECT_TRUE(v.id_touched(Identity(rainbow_index, rank), ClueKind::COLOUR,
                                static_cast<int>(clue_color)));
    }
  }
}

TEST(Variants, RainbowRankClueWorksNormally) {
  const Variant& v = get_variant("Rainbow (5 Suits)");
  const int rainbow_index = 4;
  EXPECT_TRUE(v.id_touched(Identity(rainbow_index, 3), ClueKind::RANK, 3));
  EXPECT_FALSE(v.id_touched(Identity(rainbow_index, 4), ClueKind::RANK, 3));
}

// --- White ---

TEST(Variants, WhiteWhitishUntouchedByAnyColourClue) {
  const Variant& v = get_variant("White (5 Suits)");
  const int white_index = 4;
  for (size_t clue_color = 0; clue_color < v.colourable_suits().size(); ++clue_color) {
    for (int rank = 1; rank <= 5; ++rank) {
      EXPECT_FALSE(v.id_touched(Identity(white_index, rank), ClueKind::COLOUR,
                                 static_cast<int>(clue_color)));
    }
  }
}

// --- Brown ---

TEST(Variants, BrownBrownishUntouchedByAnyRankClue) {
  const Variant& v = get_variant("Brown (5 Suits)");
  const int brown_index = 4;
  for (int clue_rank = 1; clue_rank <= 5; ++clue_rank) {
    for (int actual_rank = 1; actual_rank <= 5; ++actual_rank) {
      EXPECT_FALSE(v.id_touched(Identity(brown_index, actual_rank), ClueKind::RANK, clue_rank));
    }
  }
}

// --- Black ---

TEST(Variants, BlackDarkCardCounts) {
  const Variant& v = get_variant("Black (5 Suits)");
  const int black_index = 4;
  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_EQ(v.card_count(Identity(black_index, rank)), 1);
  }
  EXPECT_EQ(v.card_count(Identity(0, 1)), 3);
  EXPECT_EQ(v.card_count(Identity(0, 5)), 1);
}

// --- Prism ---

TEST(Variants, PrismRankDeterminesColour) {
  const Variant& v = get_variant("Prism (5 Suits)");
  const int prism_index = 4;
  EXPECT_EQ(v.colourable_suits().size(), 4u);
  for (int rank = 1; rank <= 5; ++rank) {
    const int expected_colour = (rank - 1) % 4;
    for (int clue_colour = 0; clue_colour < 4; ++clue_colour) {
      EXPECT_EQ(v.id_touched(Identity(prism_index, rank), ClueKind::COLOUR, clue_colour),
                clue_colour == expected_colour);
    }
  }
}

// --- Muddy Rainbow ---

TEST(Variants, MuddyRainbowBrownishAndRainbowish) {
  const Variant& v = get_variant("Muddy Rainbow (5 Suits)");
  const int muddy_index = 4;
  for (size_t clue_color = 0; clue_color < v.colourable_suits().size(); ++clue_color) {
    EXPECT_TRUE(v.id_touched(Identity(muddy_index, 1), ClueKind::COLOUR,
                              static_cast<int>(clue_color)));
  }
  for (int clue_rank = 1; clue_rank <= 5; ++clue_rank) {
    EXPECT_FALSE(v.id_touched(Identity(muddy_index, 1), ClueKind::RANK, clue_rank));
  }
}

// --- Dark Rainbow ---

TEST(Variants, DarkRainbowDarkAndRainbowish) {
  const Variant& v = get_variant("Dark Rainbow (5 Suits)");
  const int dr_index = 4;
  for (size_t clue_color = 0; clue_color < v.colourable_suits().size(); ++clue_color) {
    EXPECT_TRUE(v.id_touched(Identity(dr_index, 1), ClueKind::COLOUR,
                              static_cast<int>(clue_color)));
  }
  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_EQ(v.card_count(Identity(dr_index, rank)), 1);
  }
}
