// A `"<Base> Reversed"` suit must keep its base suit's clue colour.
//
// `data/variants.json` names a reversed suit by its DISPLAY name ("Orange
// Reversed"), but `data/suits.json` is keyed on the base suit -- the reversal is
// a variant-level modifier (the newID for "Orange Reversed (4 Suits)" is
// "R+G+B+Or:R"). Looking the display name up directly misses, and the stub that
// used to result carried an EMPTY `clue_colors`, which is a suit's entire link
// to its colour clue: the suit contributed no entry to `clue_colour_names`, the
// clue value fell off the end of that list, and `id_touched` answered "no" for
// every identity. Every card the clue touched was then narrowed to nothing.
//
// Replay 1969696 is what that looked like from the table: an Orange clue in
// "Orange Reversed (4 Suits)" emptied `possible` on all four cards it touched,
// so the orange ladder -- which asks `id_touched` and reads `possible` -- could
// not see the clue at either of its gates. 33 shipped variants across 12 such
// names were affected, and in each the bot was blind to one whole colour.
#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/variant.h"

using namespace hanabi;

namespace {

int colour_index(const Variant& v, const std::string& name) {
  auto it = std::find(v.clue_colour_names.begin(), v.clue_colour_names.end(), name);
  return it == v.clue_colour_names.end()
             ? -1
             : static_cast<int>(it - v.clue_colour_names.begin());
}

int suit_index(const Variant& v, const std::string& name) {
  for (size_t i = 0; i < v.suits.size(); ++i) {
    if (v.suits[i].name == name) return static_cast<int>(i);
  }
  return -1;
}

}  // namespace

// --- the colour survives the suffix ---------------------------------------

TEST(VariantReversed, ReversedOrangeStillAnswersToItsColour) {
  const Variant& v = get_variant("Orange Reversed (4 Suits)");
  const int orange_suit = suit_index(v, "Orange Reversed");
  ASSERT_NE(orange_suit, -1) << "the display name is kept, not rewritten";

  ASSERT_EQ(v.clue_colour_names.size(), 4u)
      << "four suits, four colour clues -- the server offers Orange and the "
         "bot has to be able to name it";
  const int orange_value = colour_index(v, "Orange");
  ASSERT_NE(orange_value, -1);

  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_TRUE(v.id_touched(Identity{orange_suit, rank}, ClueKind::COLOUR,
                             orange_value))
        << "the Orange clue must touch Orange " << rank;
  }
  // And it touches nothing else.
  for (int si = 0; si < static_cast<int>(v.suits.size()); ++si) {
    if (si == orange_suit) continue;
    EXPECT_FALSE(v.id_touched(Identity{si, 1}, ClueKind::COLOUR, orange_value))
        << v.suits[si].name << " must not answer to the Orange clue";
  }
}

// The suffix carries the reversal, the base carries the colour, and both have to
// survive together.
TEST(VariantReversed, BothFlagsSurvive) {
  const Variant& v = get_variant("Orange Reversed (4 Suits)");
  const int si = suit_index(v, "Orange Reversed");
  ASSERT_NE(si, -1);
  EXPECT_TRUE(v.suits[si].suit_type.reversed) << "from the suffix";
  EXPECT_TRUE(v.suits[si].suit_type.inverted)
      << "from the base name -- Orange swaps the action buttons";

  // Reversed rarity: the singleton sits at rank 1, not rank 5.
  EXPECT_EQ(v.card_count(Identity{si, 1}), 1);
  EXPECT_EQ(v.card_count(Identity{si, 5}), 3);
}

TEST(VariantReversed, DarkReversedKeepsItsDarkness) {
  const Variant& v = get_variant("Dark Orange Reversed (5 Suits)");
  const int si = suit_index(v, "Dark Orange Reversed");
  ASSERT_NE(si, -1);
  EXPECT_TRUE(v.suits[si].suit_type.dark);
  EXPECT_TRUE(v.suits[si].suit_type.inverted);
  EXPECT_TRUE(v.suits[si].suit_type.reversed);
  EXPECT_NE(colour_index(v, "Orange"), -1)
      << "Dark Orange lists Orange as its clue colour, and the suffix must not "
         "lose that either";
  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_EQ(v.card_count(Identity{si, rank}), 1) << "dark is one-of-each";
  }
}

// The mirror: a flag-driven suit contributes no colour name, and the fix must
// not invent one for it just because the lookup now succeeds.
TEST(VariantReversed, WhitishReversedStillContributesNoColour) {
  const Variant& v = get_variant("White Reversed (6 Suits)");
  const int si = suit_index(v, "White Reversed");
  ASSERT_NE(si, -1);
  EXPECT_TRUE(v.suits[si].suit_type.whitish);
  EXPECT_TRUE(v.suits[si].suit_type.reversed);
  EXPECT_EQ(colour_index(v, "White"), -1)
      << "a whitish suit is touched by no colour clue; its rule is flag-driven";
  EXPECT_EQ(v.clue_colour_names.size(), 5u)
      << "six suits, but the white one names no colour";
}

// --- short forms ----------------------------------------------------------

// Short forms are deliberately NOT resolved through the base name. Every
// recorded log and replay fixture using one of these variants already spells
// its cards with the letter the plain fallback picks -- "Black Reversed" is
// 'l', not Black's 'k' -- and rewriting that would break them all for a purely
// cosmetic gain. What must hold is only that they stay unique.
TEST(VariantReversed, ShortFormsAreUnchangedAndUnique) {
  const Variant& v = get_variant("Black Reversed (6 Suits)");
  const int si = suit_index(v, "Black Reversed");
  ASSERT_NE(si, -1);
  EXPECT_EQ(v.short_forms[si], 'l')
      << "the fallback letter, kept for compatibility with existing fixtures";
  std::vector<char> sorted = v.short_forms;
  std::sort(sorted.begin(), sorted.end());
  EXPECT_EQ(std::adjacent_find(sorted.begin(), sorted.end()), sorted.end())
      << "short forms must stay unique";
}

// --- the whole catalog ----------------------------------------------------

// Every shipped variant must end up with one colour clue per colourable suit.
// This is the assertion that would have caught the bug on the day the Reversed
// variants were added, without anyone needing to play one.
TEST(VariantReversed, EveryShippedVariantResolvesAllItsSuits) {
  int checked = 0;
  for (const auto& [name, v] : load_variants()) {
    size_t colourable = 0;
    for (const Suit& s : v.suits) {
      const SuitType& t = s.suit_type;
      if (t.rainbowish || t.whitish || t.prism) continue;  // flag-driven
      ++colourable;
    }
    // Ambiguous variants deliberately collapse several suits onto one colour,
    // so the count can be lower -- but it must never be ZERO while colourable
    // suits exist, which is exactly the empty-`clue_colors` failure.
    if (colourable > 0) {
      EXPECT_FALSE(v.clue_colour_names.empty())
          << name << " has " << colourable
          << " colourable suits but no colour clue names them";
    }
    for (const Suit& s : v.suits) {
      const SuitType& t = s.suit_type;
      if (t.rainbowish || t.whitish || t.prism) continue;
      EXPECT_FALSE(s.clue_colors.empty())
          << name << ": suit '" << s.name << "' resolved to no clue colour";
    }
    ++checked;
  }
  EXPECT_GT(checked, 2000) << "guard: the catalog actually loaded";
}
