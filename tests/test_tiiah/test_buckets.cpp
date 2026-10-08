// The three suit buckets (CONVENTION.md §1a).
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/variant.h"
#include "hanabi/conventions/tiiah/buckets.h"

using namespace hanabi;
using hanabi::tiiah::bucket_of;
using hanabi::tiiah::suit_buckets;

namespace {

std::vector<std::vector<int>> as_rows(const std::array<std::vector<int>, 3>& b) {
  return {b[0], b[1], b[2]};
}

}  // namespace

TEST(TiiahBuckets, FourSuits) {
  // Red Yellow Green Blue.
  EXPECT_EQ(as_rows(suit_buckets(get_variant("Throw It in a Hole (4 Suits)"))),
            (std::vector<std::vector<int>>{{0, 1}, {2}, {3}}));
}

TEST(TiiahBuckets, FiveSuits) {
  EXPECT_EQ(as_rows(suit_buckets(get_variant("Throw It in a Hole (5 Suits)"))),
            (std::vector<std::vector<int>>{{0, 1}, {2, 3}, {4}}));
}

TEST(TiiahBuckets, SixSuits) {
  EXPECT_EQ(as_rows(suit_buckets(get_variant("Throw It in a Hole (6 Suits)"))),
            (std::vector<std::vector<int>>{{0, 1}, {2, 3}, {4, 5}}));
}

// The inverted suit is removed BEFORE the count is taken, and the rest are
// re-indexed — so a 6-suit variant with an Orange in it uses the FIVE-suit row.
// The returned indices are the original ones, which is what a caller holding a
// card's `suit_index` needs.
TEST(TiiahBuckets, InvertedSuitsAreDroppedBeforeCounting) {
  const Variant& six = get_variant("Throw It in a Hole & Orange (6 Suits)");
  ASSERT_TRUE(six.suits[5].suit_type.inverted) << "guard: Orange is last";
  EXPECT_EQ(as_rows(suit_buckets(six)),
            (std::vector<std::vector<int>>{{0, 1}, {2, 3}, {4}}));

  const Variant& five = get_variant("Throw It in a Hole & Orange (5 Suits)");
  EXPECT_EQ(as_rows(suit_buckets(five)),
            (std::vector<std::vector<int>>{{0, 1}, {2}, {3}}));
}

// Red, Green, Blue and an Orange: only three suits are left, which is a row of
// its own — one suit per bucket, so ±1 still tells the three apart.
TEST(TiiahBuckets, ThreeRemainingSuitsTakeOneEach) {
  const Variant& v = get_variant("Throw It in a Hole & Orange (4 Suits)");
  ASSERT_EQ(v.suits.size(), 4u);
  ASSERT_TRUE(v.suits[3].suit_type.inverted);
  EXPECT_EQ(as_rows(suit_buckets(v)),
            (std::vector<std::vector<int>>{{0}, {1}, {2}}));
}

// Six non-inverted suits: one bucket per suit, by suit index (v23.0.0).
TEST(TiiahBuckets, BucketOf) {
  const Variant& v = get_variant("Throw It in a Hole (6 Suits)");
  EXPECT_EQ(bucket_of(v, 0), 0);
  EXPECT_EQ(bucket_of(v, 1), 1);
  EXPECT_EQ(bucket_of(v, 2), 2);
  EXPECT_EQ(bucket_of(v, 3), 3);
  EXPECT_EQ(bucket_of(v, 4), 4);
  EXPECT_EQ(bucket_of(v, 5), 5);
  EXPECT_EQ(bucket_of(v, 6), std::nullopt) << "out of range";

  // An inverted suit has no bucket at all: it is never a reactive target.
  const Variant& orange = get_variant("Throw It in a Hole & Orange (6 Suits)");
  EXPECT_EQ(bucket_of(orange, 5), std::nullopt);
  // Its five non-inverted suits are one bucket each (v23.0.0).
  EXPECT_EQ(bucket_of(orange, 4), 4);
}
