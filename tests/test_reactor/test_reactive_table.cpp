// Port of python-bot/tests/test_reactor/test_reactive_table.py.
#include <gtest/gtest.h>

#include "hanabi/basics/variant.h"
#include "hanabi/conventions/variants/reactive_table.h"

using namespace hanabi;
using hanabi::reactor::variants::format_reactive_settings;
using hanabi::reactor::variants::reactive_value_table;

namespace {

Suit make_suit(const std::string& name) {
  Suit s;
  s.name = name;
  s.abbreviation = static_cast<char>(std::tolower(name[0]));
  s.suit_type = SuitType::of_name(name);
  return s;
}

Variant make_variant(std::initializer_list<std::string> names,
                       const std::string& vname = "test") {
  Variant v;
  v.id = 999;
  v.name = vname;
  int idx = 0;
  for (const auto& n : names) {
    Suit s = make_suit(n);
    v.suits.push_back(s);
    v.short_forms.push_back(*s.abbreviation);
    v.colourable_suit_indices.push_back(idx++);
  }
  return v;
}

}  // namespace

TEST(ReactiveTable, ThreeColorRedBluePink) {
  Variant v = make_variant({"Red", "Blue", "Pink"}, "t1");
  EXPECT_EQ(reactive_value_table(v, 5), (std::vector<int>{1, 4, 5}));
}

TEST(ReactiveTable, FiveWithWrap) {
  Variant v = make_variant({"Red", "Green", "Blue", "Pink", "Brown"}, "t2");
  EXPECT_EQ(reactive_value_table(v, 5), (std::vector<int>{1, 3, 4, 5, 2}));
}

TEST(ReactiveTable, SixAllUsedDefaultsToOne) {
  Variant v = make_variant({"Red", "Yellow", "Green", "Blue", "Pink", "Brown"}, "t3");
  EXPECT_EQ(reactive_value_table(v, 5), (std::vector<int>{1, 2, 3, 4, 5, 1}));
}

TEST(ReactiveTable, RedPlusSpecials) {
  Variant v = make_variant({"Red", "Pink", "Brown"}, "t4");
  EXPECT_EQ(reactive_value_table(v, 5), (std::vector<int>{1, 2, 3}));
}

TEST(ReactiveTable, FourPlayerHandSizeMod) {
  Variant v = make_variant({"Red", "Yellow", "Green", "Blue", "Purple", "Teal"}, "t5");
  EXPECT_EQ(reactive_value_table(v, 4), (std::vector<int>{1, 2, 3, 4, 1, 2}));
}

TEST(ReactiveTable, FourPlayerSpecialsWrapToOne) {
  Variant v = make_variant({"Red", "Yellow", "Green", "Blue", "Pink"}, "t6");
  EXPECT_EQ(reactive_value_table(v, 4), (std::vector<int>{1, 2, 3, 4, 1}));
}

// --- Ambiguous rainbowy variants: values keyed by clue colour -------------
// The representative suits are named Tomato / Berry / ..., but the clue a
// partner gives is Red / Blue — the reactive value anchors to the colour.

TEST(ReactiveTable, AmbiguousRainbowUsesClueColours) {
  // Tomato + Mahogany clue as Red → 1; Berry + Navy clue as Blue → 4.
  const Variant& v = get_variant("Ambiguous & Rainbow (5 Suits)");
  EXPECT_EQ(reactive_value_table(v, 5), (std::vector<int>{1, 4}));
}

TEST(ReactiveTable, AmbiguousOmniUsesClueColours) {
  const Variant& v = get_variant("Ambiguous & Omni (5 Suits)");
  EXPECT_EQ(reactive_value_table(v, 5), (std::vector<int>{1, 4}));
}

TEST(ReactiveTable, VeryAmbiguousOmniAllBlue) {
  // Berry / Navy / Sky all clue as Blue → single colour slot 4.
  const Variant& v = get_variant("Very Ambiguous & Omni (4 Suits)");
  EXPECT_EQ(reactive_value_table(v, 5), (std::vector<int>{4}));
}

TEST(ReactiveTable, ExtremelyAmbiguousOmniAllBlue) {
  const Variant& v = get_variant("Extremely Ambiguous & Omni (6 Suits)");
  EXPECT_EQ(reactive_value_table(v, 5), (std::vector<int>{4}));
}

// --- format_reactive_settings: /allplays on/off output --------------------

TEST(ReactiveTableSettings, VanillaOff) {
  Variant v = make_variant({"Red", "Yellow", "Green", "Blue", "Purple"}, "tv");
  EXPECT_EQ(format_reactive_settings(v, 5, /*all_plays=*/false),
             "odd plays: {slot focus}, even plays: {slot focus}");
}

TEST(ReactiveTableSettings, VanillaOnCollapsed) {
  Variant v = make_variant({"Red", "Yellow", "Green", "Blue", "Purple"}, "tv2");
  // Both halves are {slot focus} → collapse to one.
  EXPECT_EQ(format_reactive_settings(v, 5, /*all_plays=*/true),
             "even plays: {slot focus}");
}

TEST(ReactiveTableSettings, RainbowyOff) {
  Variant v = make_variant({"Red", "Yellow", "Green", "Blue", "Purple"}, "tr1");
  v.rainbow_s = true;
  EXPECT_EQ(format_reactive_settings(v, 5, /*all_plays=*/false),
             "odd plays: {r, y, g, b, p}, even plays: {slot focus}");
}

TEST(ReactiveTableSettings, RainbowyOnCombined) {
  Variant v = make_variant({"Red", "Yellow", "Green", "Blue", "Purple"}, "tr2");
  v.rainbow_s = true;
  EXPECT_EQ(format_reactive_settings(v, 5, /*all_plays=*/true),
             "even plays: {slot focus} + {r, y, g, b, p}");
}

TEST(ReactiveTableSettings, PinkishOff) {
  // Special rank 3 is blocked under a pinkish flag → that rank renders as '-'.
  Variant v = make_variant({"Red", "Yellow", "Green", "Blue", "Purple"}, "tp1");
  v.pink_s = true;
  v.special_rank = 3;
  EXPECT_EQ(format_reactive_settings(v, 5, /*all_plays=*/false),
             "odd plays: {slot focus}, even plays: {1, 2, -, 4, 5}");
}

TEST(ReactiveTableSettings, PinkishOnCombined) {
  Variant v = make_variant({"Red", "Yellow", "Green", "Blue", "Purple"}, "tp2");
  v.pink_s = true;
  v.special_rank = 3;
  EXPECT_EQ(format_reactive_settings(v, 5, /*all_plays=*/true),
             "even plays: {1, 2, -, 4, 5} + {slot focus}");
}

TEST(ReactiveTableSettings, RainbowAndPinkishOn) {
  Variant v = make_variant({"Red", "Yellow", "Green", "Blue", "Purple"}, "trp");
  v.rainbow_s = true;
  v.pink_s = true;
  v.special_rank = 3;
  EXPECT_EQ(format_reactive_settings(v, 5, /*all_plays=*/true),
             "even plays: {1, 2, -, 4, 5} + {r, y, g, b, p}");
}
