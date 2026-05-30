// Port of python-bot/tests/test_reactor/test_reactive_table.py.
#include <gtest/gtest.h>

#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor/reactive_table.h"

using namespace hanabi;
using hanabi::reactor::reactive_value_table;

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
