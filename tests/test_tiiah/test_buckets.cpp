// The suit buckets (CONVENTION.md §1a): one per non-inverted suit, in every TIIAH
// variant (five or six suits since v23.0.0, three or four since v24.0.0).
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/variant.h"
#include "hanabi/conventions/tiiah/buckets.h"

using namespace hanabi;
using hanabi::tiiah::bucket_of;
using hanabi::tiiah::variant_buckets;

using Rows = std::vector<std::vector<int>>;

// Red Yellow Green Blue. Until v24.0.0 red and yellow shared bucket 0.
TEST(TiiahBuckets, FourSuits) {
  EXPECT_EQ(variant_buckets(get_variant("Throw It in a Hole (4 Suits)")),
            (Rows{{0}, {1}, {2}, {3}}));
}

TEST(TiiahBuckets, FiveSuits) {
  EXPECT_EQ(variant_buckets(get_variant("Throw It in a Hole (5 Suits)")),
            (Rows{{0}, {1}, {2}, {3}, {4}}));
}

TEST(TiiahBuckets, SixSuits) {
  EXPECT_EQ(variant_buckets(get_variant("Throw It in a Hole (6 Suits)")),
            (Rows{{0}, {1}, {2}, {3}, {4}, {5}}));
}

// The inverted suit is in no bucket, and the rest are numbered by their position
// among the non-inverted suits. The returned indices are the original ones, which
// is what a caller holding a card's `suit_index` needs.
TEST(TiiahBuckets, InvertedSuitsAreDroppedBeforeCounting) {
  const Variant& six = get_variant("Throw It in a Hole & Orange (6 Suits)");
  ASSERT_TRUE(six.suits[5].suit_type.inverted) << "guard: Orange is last";
  EXPECT_EQ(variant_buckets(six), (Rows{{0}, {1}, {2}, {3}, {4}}));

  const Variant& five = get_variant("Throw It in a Hole & Orange (5 Suits)");
  EXPECT_EQ(variant_buckets(five), (Rows{{0}, {1}, {2}, {3}}));
}

// Red, Green, Blue and an Orange: three suits are left, one bucket each.
TEST(TiiahBuckets, ThreeRemainingSuitsTakeOneEach) {
  const Variant& v = get_variant("Throw It in a Hole & Orange (4 Suits)");
  ASSERT_EQ(v.suits.size(), 4u);
  ASSERT_TRUE(v.suits[3].suit_type.inverted);
  EXPECT_EQ(variant_buckets(v), (Rows{{0}, {1}, {2}}));
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
