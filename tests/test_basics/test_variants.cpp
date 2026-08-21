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

// --- Funnels ---

TEST(Variants, FunnelsFlagParsed) {
  const Variant& v = get_variant("Funnels (5 Suits)");
  EXPECT_TRUE(v.funnels);
  EXPECT_FALSE(v.chimneys);
}

TEST(Variants, FunnelsNormalSuitTouchesRankLeqClueValue) {
  // Funnels: rank-K clue touches all ranks ≤ K in non-pinkish, non-brownish
  // suits.
  const Variant& v = get_variant("Funnels (5 Suits)");
  for (int clue_rank = 1; clue_rank <= 5; ++clue_rank) {
    for (int actual_rank = 1; actual_rank <= 5; ++actual_rank) {
      bool expected = actual_rank <= clue_rank;
      EXPECT_EQ(v.id_touched(Identity(0, actual_rank), ClueKind::RANK, clue_rank),
                expected)
          << "Funnels rank-" << clue_rank << " on rank-" << actual_rank;
    }
  }
}

TEST(Variants, FunnelsColourClueUnchanged) {
  // Funnels affects only rank clues. Colour clues still touch a single suit.
  const Variant& v = get_variant("Funnels (5 Suits)");
  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_TRUE(v.id_touched(Identity(0, rank), ClueKind::COLOUR, 0));
    EXPECT_FALSE(v.id_touched(Identity(1, rank), ClueKind::COLOUR, 0));
  }
}

TEST(Variants, FunnelsPinkSuitKeepsPinkishRule) {
  // In "Funnels & Pink (5 Suits)" the Pink suit is still touched by every
  // rank clue (pinkish rule wins; funnels rule applies to other suits).
  const Variant& v = get_variant("Funnels & Pink (5 Suits)");
  const int pink_index = 4;
  for (int clue_rank = 1; clue_rank <= 5; ++clue_rank) {
    for (int actual_rank = 1; actual_rank <= 5; ++actual_rank) {
      EXPECT_TRUE(v.id_touched(Identity(pink_index, actual_rank), ClueKind::RANK,
                                clue_rank));
    }
  }
  // Normal suit still follows the funnels rule.
  for (int clue_rank = 1; clue_rank <= 5; ++clue_rank) {
    for (int actual_rank = 1; actual_rank <= 5; ++actual_rank) {
      bool expected = actual_rank <= clue_rank;
      EXPECT_EQ(v.id_touched(Identity(0, actual_rank), ClueKind::RANK, clue_rank),
                expected);
    }
  }
}

TEST(Variants, FunnelsBrownSuitKeepsBrownishRule) {
  // In "Funnels & Brown (5 Suits)" the Brown suit is never touched by any
  // rank clue (brownish rule wins; funnels rule applies to other suits).
  const Variant& v = get_variant("Funnels & Brown (5 Suits)");
  const int brown_index = 4;
  for (int clue_rank = 1; clue_rank <= 5; ++clue_rank) {
    for (int actual_rank = 1; actual_rank <= 5; ++actual_rank) {
      EXPECT_FALSE(v.id_touched(Identity(brown_index, actual_rank), ClueKind::RANK,
                                 clue_rank));
    }
  }
}

// --- Chimneys ---

TEST(Variants, ChimneysFlagParsed) {
  const Variant& v = get_variant("Chimneys (5 Suits)");
  EXPECT_FALSE(v.funnels);
  EXPECT_TRUE(v.chimneys);
}

TEST(Variants, ChimneysNormalSuitTouchesRankGeqClueValue) {
  // Chimneys: rank-K clue touches all ranks ≥ K in non-pinkish, non-brownish
  // suits.
  const Variant& v = get_variant("Chimneys (5 Suits)");
  for (int clue_rank = 1; clue_rank <= 5; ++clue_rank) {
    for (int actual_rank = 1; actual_rank <= 5; ++actual_rank) {
      bool expected = actual_rank >= clue_rank;
      EXPECT_EQ(v.id_touched(Identity(0, actual_rank), ClueKind::RANK, clue_rank),
                expected)
          << "Chimneys rank-" << clue_rank << " on rank-" << actual_rank;
    }
  }
}

TEST(Variants, ChimneysPinkSuitKeepsPinkishRule) {
  const Variant& v = get_variant("Chimneys & Pink (5 Suits)");
  const int pink_index = 4;
  for (int clue_rank = 1; clue_rank <= 5; ++clue_rank) {
    for (int actual_rank = 1; actual_rank <= 5; ++actual_rank) {
      EXPECT_TRUE(v.id_touched(Identity(pink_index, actual_rank), ClueKind::RANK,
                                clue_rank));
    }
  }
  for (int clue_rank = 1; clue_rank <= 5; ++clue_rank) {
    for (int actual_rank = 1; actual_rank <= 5; ++actual_rank) {
      bool expected = actual_rank >= clue_rank;
      EXPECT_EQ(v.id_touched(Identity(0, actual_rank), ClueKind::RANK, clue_rank),
                expected);
    }
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

// --- Reversed direction (orthogonal to inverted/Orange) ---

TEST(Variants, ReversedSuitDetected) {
  // "Reversed (5 Suits)" puts Purple Reversed at suit index 4.
  const Variant& v = get_variant("Reversed (5 Suits)");
  EXPECT_TRUE(v.suits[4].suit_type.reversed);
  EXPECT_FALSE(v.suits[4].suit_type.inverted);
  for (int i = 0; i < 4; ++i) {
    EXPECT_FALSE(v.suits[i].suit_type.reversed);
  }
}

TEST(Variants, ReversedCardCounts) {
  // Non-dark reversed suits flip the rarity table: rank-1 is the
  // unique critical (1 copy) and rank-5 is the most common (3 copies).
  const Variant& v = get_variant("Reversed (5 Suits)");
  const int rev_index = 4;
  EXPECT_EQ(v.card_count(Identity(rev_index, 1)), 1);
  EXPECT_EQ(v.card_count(Identity(rev_index, 2)), 2);
  EXPECT_EQ(v.card_count(Identity(rev_index, 3)), 2);
  EXPECT_EQ(v.card_count(Identity(rev_index, 4)), 2);
  EXPECT_EQ(v.card_count(Identity(rev_index, 5)), 3);
  // Normal suits unchanged.
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(v.card_count(Identity(i, 1)), 3);
    EXPECT_EQ(v.card_count(Identity(i, 5)), 1);
  }
}

TEST(Variants, BlackReversedOneOfEach) {
  // Dark suits (Black Reversed) keep 1-of-each regardless of direction.
  const Variant& v = get_variant("Black Reversed (5 Suits)");
  EXPECT_TRUE(v.suits[4].suit_type.dark);
  EXPECT_TRUE(v.suits[4].suit_type.reversed);
  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_EQ(v.card_count(Identity(4, rank)), 1);
  }
}

// --- Ambiguous family (multiple suits share one colour clue) -------------

// Ambiguous (6 Suits): six suits collapse to three colour clues.
// Red touches suits 0 (Tomato) + 1 (Mahogany); Green touches 2
// (Emerald) + 3 (Olive); Blue touches 4 (Berry) + 5 (Navy).
TEST(Variants, AmbiguousSixSuitsThreeColours) {
  const Variant& v = get_variant("Ambiguous (6 Suits)");
  ASSERT_EQ(v.suits.size(), 6u);
  EXPECT_EQ(v.clue_colour_names.size(), 3u);
  EXPECT_EQ(v.colourable_suits().size(), 3u);
  EXPECT_EQ(v.clue_colour_names,
             (std::vector<std::string>{"Red", "Green", "Blue"}));

  struct Case { int suit; int colour; bool touched; };
  for (Case c : std::vector<Case>{
           {0, 0, true},  {1, 0, true},  {2, 0, false}, {3, 0, false},
           {4, 0, false}, {5, 0, false}, {0, 1, false}, {1, 1, false},
           {2, 1, true},  {3, 1, true},  {4, 1, false}, {5, 1, false},
           {0, 2, false}, {1, 2, false}, {2, 2, false}, {3, 2, false},
           {4, 2, true},  {5, 2, true},
       }) {
    EXPECT_EQ(v.id_touched(Identity(c.suit, 3), ClueKind::COLOUR, c.colour),
              c.touched)
        << "suit=" << c.suit << " colour=" << c.colour
        << " expected=" << c.touched;
  }
}

// Very Ambiguous (6 Suits): six suits collapse to two colours. Red
// touches Tomato/Mahogany/Carrot (suits 0,1,2); Blue touches
// Berry/Navy/Sky (suits 3,4,5).
TEST(Variants, VeryAmbiguousSixSuitsTwoColours) {
  const Variant& v = get_variant("Very Ambiguous (6 Suits)");
  ASSERT_EQ(v.suits.size(), 6u);
  EXPECT_EQ(v.clue_colour_names.size(), 2u);
  EXPECT_EQ(v.colourable_suits().size(), 2u);
  EXPECT_EQ(v.clue_colour_names,
             (std::vector<std::string>{"Red", "Blue"}));

  for (int suit = 0; suit < 6; ++suit) {
    bool red_touches = (suit <= 2);
    bool blue_touches = (suit >= 3);
    EXPECT_EQ(v.id_touched(Identity(suit, 3), ClueKind::COLOUR, 0),
              red_touches)
        << "Red should touch suits 0-2 only; suit=" << suit;
    EXPECT_EQ(v.id_touched(Identity(suit, 3), ClueKind::COLOUR, 1),
              blue_touches)
        << "Blue should touch suits 3-5 only; suit=" << suit;
  }
}

// Extremely Ambiguous (6 Suits): all six suits share a single colour
// (Blue). One clue colour for the whole variant.
TEST(Variants, ExtremelyAmbiguousSixSuitsOneColour) {
  const Variant& v = get_variant("Extremely Ambiguous (6 Suits)");
  ASSERT_EQ(v.suits.size(), 6u);
  EXPECT_EQ(v.clue_colour_names.size(), 1u);
  EXPECT_EQ(v.colourable_suits().size(), 1u);
  EXPECT_EQ(v.clue_colour_names, (std::vector<std::string>{"Blue"}));

  for (int suit = 0; suit < 6; ++suit) {
    EXPECT_TRUE(v.id_touched(Identity(suit, 3), ClueKind::COLOUR, 0))
        << "Blue must touch every Extremely Ambiguous suit; suit=" << suit;
  }
}

// Sanity: a single Suit catalog entry now exposes its clue_colors.
TEST(Variants, SuitCatalogExposesClueColors) {
  const auto& catalog = load_suit_catalog();
  ASSERT_TRUE(catalog.count("Tomato"));
  EXPECT_EQ(catalog.at("Tomato").clue_colors,
             (std::vector<std::string>{"Red"}));
  ASSERT_TRUE(catalog.count("Mahogany"));
  EXPECT_EQ(catalog.at("Mahogany").clue_colors,
             (std::vector<std::string>{"Red"}));
  ASSERT_TRUE(catalog.count("Lime"));
  // Lime is a dual-colour suit (Yellow + Green) used in other variants.
  EXPECT_EQ(catalog.at("Lime").clue_colors,
             (std::vector<std::string>{"Yellow", "Green"}));
}

// --- Matryoshka -----------------------------------------------------------
//
// Nested colour touches: Red hits every suit, Yellow everything from Yam on,
// Green from Geas on, Blue from Beatnik on, Purple only Plum and Taupe, Teal
// only Taupe.
//
// No production rule implements this. It falls out of `data/suits.json`, where
// each suit lists exactly the colours that reach it (Ruby ["Red"] ... Taupe
// ["Red","Yellow","Green","Blue","Purple","Teal"]), and `id_touched`'s colour
// branch matches the clue colour NAME against that list. These tests pin the
// data and the derivation together, since either drifting would break the
// variant silently.
TEST(Variants, MatryoshkaColoursAreNested) {
  const Variant& v = get_variant("Matryoshka (6 Suits)");
  ASSERT_EQ(v.suits.size(), 6u);
  EXPECT_EQ(v.clue_colour_names,
            (std::vector<std::string>{"Red", "Yellow", "Green", "Blue",
                                      "Purple", "Teal"}));
  EXPECT_EQ(v.colourable_suit_indices, (std::vector<int>{0, 1, 2, 3, 4, 5}));

  // colour c touches suit s exactly when s >= c.
  for (int colour = 0; colour < 6; ++colour) {
    for (int suit = 0; suit < 6; ++suit) {
      const bool expected = suit >= colour;
      EXPECT_EQ(v.id_touched(Identity(suit, 3), ClueKind::COLOUR, colour),
                expected)
          << "colour=" << v.clue_colour_names[colour]
          << " suit=" << v.suits[suit].name << " expected=" << expected;
    }
  }
}

// The two ends of the nesting, stated directly so a failure names the rule
// rather than a loop index.
TEST(Variants, MatryoshkaRedTouchesAllAndTealOnlyTaupe) {
  const Variant& v = get_variant("Matryoshka (6 Suits)");
  for (int suit = 0; suit < 6; ++suit) {
    EXPECT_TRUE(v.id_touched(Identity(suit, 1), ClueKind::COLOUR, 0))
        << "Red must touch " << v.suits[suit].name;
    EXPECT_EQ(v.id_touched(Identity(suit, 1), ClueKind::COLOUR, 5), suit == 5)
        << "Teal must touch Taupe alone; suit=" << v.suits[suit].name;
  }
  // The negative that separates Matryoshka from a plain variant: Yellow does
  // NOT reach Ruby, even though Red reaches Yam.
  EXPECT_FALSE(v.id_touched(Identity(0, 1), ClueKind::COLOUR, 1))
      << "Yellow must not touch Ruby";
  EXPECT_TRUE(v.id_touched(Identity(1, 1), ClueKind::COLOUR, 0))
      << "but Red does touch Yam";
}

// Matryoshka carries no special flag and none of its suit names collide with a
// `SuitType::of_name` substring, so every suit must classify as plain.
TEST(Variants, MatryoshkaSuitsAreAllPlain) {
  const Variant& v = get_variant("Matryoshka (6 Suits)");
  for (const Suit& s : v.suits) {
    const SuitType& t = s.suit_type;
    EXPECT_FALSE(t.whitish || t.rainbowish || t.pinkish || t.brownish ||
                 t.dark || t.prism || t.muddy || t.inverted || t.reversed)
        << s.name << " must classify as a plain suit";
  }
  EXPECT_FALSE(v.odds_and_evens);
  EXPECT_EQ(v.clue_ranks, (std::vector<int>{1, 2, 3, 4, 5}));
}

// --- Odds and Evens -------------------------------------------------------

// A rank clue names a PARITY, not a rank: value 1 is "odd" and touches ranks
// 1/3/5, value 2 is "even" and touches 2/4.
TEST(Variants, OddsAndEvensRankCluesTouchByParity) {
  const Variant& v = get_variant("Odds and Evens (5 Suits)");
  ASSERT_TRUE(v.odds_and_evens);
  EXPECT_EQ(v.clue_ranks, (std::vector<int>{1, 2}));

  for (int suit = 0; suit < 5; ++suit) {
    for (int rank = 1; rank <= 5; ++rank) {
      const bool odd = rank % 2 == 1;
      EXPECT_EQ(v.id_touched(Identity(suit, rank), ClueKind::RANK, 1), odd)
          << "odd clue, suit=" << suit << " rank=" << rank;
      EXPECT_EQ(v.id_touched(Identity(suit, rank), ClueKind::RANK, 2), !odd)
          << "even clue, suit=" << suit << " rank=" << rank;
    }
  }
  // Colour clues are untouched by the variant.
  EXPECT_TRUE(v.id_touched(Identity(0, 4), ClueKind::COLOUR, 0));
  EXPECT_FALSE(v.id_touched(Identity(1, 4), ClueKind::COLOUR, 0));
}

// A pinkish suit keeps its own rule inside the variant: the parity branch sits
// BELOW the pinkish/brownish checks, so Pink still takes every rank clue.
TEST(Variants, OddsAndEvensPinkStillTakesEveryRankClue) {
  const Variant& v = get_variant("Odds and Evens & Pink (5 Suits)");
  ASSERT_TRUE(v.odds_and_evens);
  const int pink = static_cast<int>(v.suits.size()) - 1;
  ASSERT_TRUE(v.suits[pink].suit_type.pinkish) << "fixture: last suit is Pink";

  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_TRUE(v.id_touched(Identity(pink, rank), ClueKind::RANK, 1));
    EXPECT_TRUE(v.id_touched(Identity(pink, rank), ClueKind::RANK, 2));
  }
  // ...while a plain suit in the same variant still splits by parity.
  EXPECT_TRUE(v.id_touched(Identity(0, 3), ClueKind::RANK, 1));
  EXPECT_FALSE(v.id_touched(Identity(0, 3), ClueKind::RANK, 2));
}

// `clueRanks` decides which rank clues exist at all. Absent means the full
// 1-5; Odds and Evens gives {1,2}; the Number Mute family gives {} and offers
// no rank clue whatsoever.
TEST(Variants, ClueRanksLoadsFromTheVariantData) {
  EXPECT_EQ(get_variant("No Variant").clue_ranks,
            (std::vector<int>{1, 2, 3, 4, 5}));
  EXPECT_EQ(get_variant("Odds and Evens (5 Suits)").clue_ranks,
            (std::vector<int>{1, 2}));
  EXPECT_TRUE(get_variant("Number Mute (5 Suits)").clue_ranks.empty());
  // Pink-Ones drops rank 1, which `rank_blocked` already enforced -- the two
  // must agree, or `all_valid_clues` would start offering a clue it rejects.
  EXPECT_EQ(get_variant("Pink-Ones (5 Suits)").clue_ranks,
            (std::vector<int>{2, 3, 4, 5}));
}
