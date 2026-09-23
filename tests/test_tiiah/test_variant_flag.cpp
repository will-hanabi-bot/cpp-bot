// The `throwItInAHole` flag, end to end from data/variants.json.
//
// Before v16.0.0 nothing read this key, so all 44 variants loaded with ordinary
// rules and were played as if the hole did not exist — reactor's CONVENTION.md
// §1b.8 listed it among the fields that make a variant "load and be played with
// incorrect rules".
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/variant.h"

using namespace hanabi;

TEST(TiiahVariantFlag, TheFlagIsParsed) {
  EXPECT_TRUE(get_variant("Throw It in a Hole (4 Suits)").throw_it_in_a_hole);
  EXPECT_TRUE(get_variant("Throw It in a Hole (5 Suits)").throw_it_in_a_hole);
  EXPECT_TRUE(get_variant("Throw It in a Hole (6 Suits)").throw_it_in_a_hole);
  EXPECT_TRUE(
      get_variant("Throw It in a Hole & Orange (4 Suits)").throw_it_in_a_hole);
  EXPECT_TRUE(
      get_variant("Throw It in a Hole & Rainbow (6 Suits)").throw_it_in_a_hole);
}

TEST(TiiahVariantFlag, OrdinaryVariantsAreUnaffected) {
  EXPECT_FALSE(get_variant("No Variant").throw_it_in_a_hole);
  EXPECT_FALSE(get_variant("Orange (4 Suits)").throw_it_in_a_hole);
  EXPECT_FALSE(get_variant("Rainbow (5 Suits)").throw_it_in_a_hole);
}

// A count, because the failure mode this guards against is silent: a typo in
// the JSON key name would leave every `throw_it_in_a_hole` false and every test
// above would simply stop testing anything.
TEST(TiiahVariantFlag, EveryTiiahVariantCarriesIt) {
  int flagged = 0;
  int named = 0;
  for (const auto& [name, variant] : load_variants()) {
    const bool by_name = name.find("Throw It in a Hole") != std::string::npos;
    if (by_name) ++named;
    if (variant.throw_it_in_a_hole) ++flagged;
    EXPECT_EQ(by_name, variant.throw_it_in_a_hole)
        << "the name and the flag must agree for " << name;
  }
  EXPECT_EQ(named, 44);
  EXPECT_EQ(flagged, 44);
}
