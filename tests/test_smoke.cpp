// Phase-0 smoke: verify the build picks up nlohmann_json, that data/ ships with
// the binary, and that variants.json + suits.json parse as JSON arrays. Real
// Variant parsing arrives in Phase 1 (tests/test_basics/test_variants.cpp).

#include <fstream>
#include <string>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace {

nlohmann::json load_json(const std::string& filename) {
  const std::string path = std::string(HANABI_DATA_DIR) + "/" + filename;
  std::ifstream f(path);
  if (!f.is_open()) {
    throw std::runtime_error("missing data file: " + path);
  }
  return nlohmann::json::parse(f);
}

}  // namespace

TEST(Smoke, SuitsJsonLoads) {
  const auto suits = load_json("suits.json");
  ASSERT_TRUE(suits.is_array());
  EXPECT_GT(suits.size(), 10u);

  const auto& red = *std::find_if(suits.begin(), suits.end(), [](const auto& s) {
    return s.value("name", std::string{}) == "Red";
  });
  EXPECT_EQ(red["name"], "Red");
}

TEST(Smoke, VariantsJsonLoads) {
  const auto variants = load_json("variants.json");
  ASSERT_TRUE(variants.is_array());
  EXPECT_GT(variants.size(), 50u);

  // Spot-check the No Variant entry (id 0).
  const auto& no_variant = *std::find_if(variants.begin(), variants.end(), [](const auto& v) {
    return v.value("id", -1) == 0;
  });
  EXPECT_EQ(no_variant["name"], "No Variant");
  ASSERT_TRUE(no_variant["suits"].is_array());
  EXPECT_EQ(no_variant["suits"].size(), 5u);
}
