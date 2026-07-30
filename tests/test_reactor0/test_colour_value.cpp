// Reactor0's fixed colour-value table (colour_value.cpp). The spec's rules:
// Red=1 Yellow=2 Green=3 Blue=4 Purple=5 Teal=1; Black/Pink/Brown take the
// first untaken of {4,3,5,2,1} (all taken -> 1); Orange the first untaken
// of {2,5,4,3,1} (all taken -> 2).
#include <gtest/gtest.h>

#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/colour_value.h"

using namespace hanabi;
using hanabi::reactor0::colour_value_table;

namespace {

int value_of(const Variant& v, const std::string& colour) {
  auto table = colour_value_table(v);
  for (size_t i = 0; i < v.clue_colour_names.size(); ++i) {
    if (v.clue_colour_names[i] == colour) return table[i];
  }
  ADD_FAILURE() << "colour " << colour << " not in " << v.name;
  return -1;
}

}  // namespace

TEST(Reactor0ColourValue, VanillaFiveSuits) {
  const Variant& v = get_variant("No Variant");
  EXPECT_EQ(value_of(v, "Red"), 1);
  EXPECT_EQ(value_of(v, "Yellow"), 2);
  EXPECT_EQ(value_of(v, "Green"), 3);
  EXPECT_EQ(value_of(v, "Blue"), 4);
  EXPECT_EQ(value_of(v, "Purple"), 5);
}

TEST(Reactor0ColourValue, TealCollidesWithRed) {
  const Variant& v = get_variant("6 Suits");
  EXPECT_EQ(value_of(v, "Red"), 1);
  EXPECT_EQ(value_of(v, "Teal"), 1) << "collisions are allowed by design";
}

TEST(Reactor0ColourValue, SpecWorkedExample) {
  // Red / Blue / Brown / Orange -> red=1, blue=4, brown=3, orange=2.
  const Variant& v = get_variant("Brown & Orange (4 Suits)");
  EXPECT_EQ(value_of(v, "Red"), 1);
  EXPECT_EQ(value_of(v, "Blue"), 4);
  EXPECT_EQ(value_of(v, "Brown"), 3);
  EXPECT_EQ(value_of(v, "Orange"), 2);
}

TEST(Reactor0ColourValue, OrangePrefersTwoThenFive) {
  // R,Y,G,B + Orange: {1,2,3,4} taken; orange prefs {2,5,4,3,1} -> 5.
  const Variant& v = get_variant("Orange (5 Suits)");
  EXPECT_EQ(value_of(v, "Orange"), 5);
}

TEST(Reactor0ColourValue, ExhaustedDarkFallsBackToOne) {
  // All five vanilla values taken -> Black gets the fallback 1.
  const Variant& black = get_variant("Black (6 Suits)");
  EXPECT_EQ(value_of(black, "Black"), 1);
  const Variant& pink = get_variant("Pink (6 Suits)");
  EXPECT_EQ(value_of(pink, "Pink"), 1);
}
