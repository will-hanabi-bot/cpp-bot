// Port of python-bot/tests/test_basics/test_identity_set.py.
#include <gtest/gtest.h>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"

using hanabi::Identity;
using hanabi::IdentitySet;

// --- Identity ---

TEST(Identity, ToOrdRoundTrip) {
  for (int suit_index = 0; suit_index < 6; ++suit_index) {
    for (int rank = 1; rank <= 5; ++rank) {
      Identity id(suit_index, rank);
      EXPECT_EQ(Identity::from_ord(id.to_ord()), id);
    }
  }
}

TEST(Identity, FromOrdOutOfRange) {
  EXPECT_THROW(Identity::from_ord(-1), std::invalid_argument);
  EXPECT_THROW(Identity::from_ord(30), std::invalid_argument);
}

TEST(Identity, PrevNext) {
  EXPECT_FALSE(Identity(0, 1).prev().has_value());
  EXPECT_EQ(Identity(0, 2).prev(), Identity(0, 1));
  EXPECT_FALSE(Identity(0, 5).next().has_value());
  EXPECT_EQ(Identity(0, 4).next(), Identity(0, 5));
}

TEST(Identity, PlayedBefore) {
  EXPECT_TRUE(Identity(0, 1).played_before(Identity(0, 2)));
  EXPECT_FALSE(Identity(0, 2).played_before(Identity(0, 1)));
  EXPECT_FALSE(Identity(0, 1).played_before(Identity(1, 2)));  // different suit
  EXPECT_FALSE(Identity(0, 2).played_before(Identity(0, 2)));  // equal
}

// --- IdentitySet: construction ---

TEST(IdentitySet, Empty) {
  IdentitySet s = IdentitySet::empty();
  EXPECT_EQ(s.length(), 0);
  EXPECT_TRUE(s.is_empty());
  EXPECT_FALSE(s.non_empty());
  EXPECT_TRUE(s.to_list().empty());
}

TEST(IdentitySet, Single) {
  Identity id(2, 3);
  IdentitySet s = IdentitySet::single(id);
  EXPECT_EQ(s.length(), 1);
  EXPECT_TRUE(s.contains(id));
  EXPECT_EQ(s.head(), id);
  EXPECT_TRUE(s.is_exactly(id));
  EXPECT_FALSE(s.is_exactly(Identity(2, 4)));
}

TEST(IdentitySet, FromIterIteratesSortedByOrd) {
  IdentitySet s = IdentitySet::from_iter(
      {Identity(2, 1), Identity(0, 3), Identity(1, 5), Identity(0, 1)});
  EXPECT_EQ(s.length(), 4);
  auto list = s.to_list();
  for (size_t i = 1; i < list.size(); ++i) {
    EXPECT_LT(list[i - 1].to_ord(), list[i].to_ord());
  }
}

TEST(IdentitySet, FromIterDedupes) {
  IdentitySet s = IdentitySet::from_iter(
      {Identity(0, 1), Identity(0, 1), Identity(0, 2)});
  EXPECT_EQ(s.length(), 2);
}

TEST(IdentitySet, CreateWithPredicate) {
  // All rank-5s across the standard 5-suit range (ordinals 4, 9, 14, 19, 24).
  IdentitySet s =
      IdentitySet::create([](Identity i) { return i.rank == 5; }, 25);
  EXPECT_EQ(s.length(), 5);
  for (Identity id : s) EXPECT_EQ(id.rank, 5);
}

// --- IdentitySet: bitwise operators preserve type ---

namespace {
IdentitySet make_a() {
  return IdentitySet::from_iter(
      {Identity(0, 1), Identity(0, 2), Identity(1, 1)});
}
IdentitySet make_b() {
  return IdentitySet::from_iter(
      {Identity(0, 2), Identity(1, 1), Identity(2, 3)});
}
}  // namespace

TEST(IdentitySet, Or) {
  IdentitySet result = make_a() | make_b();
  EXPECT_EQ(result.length(), 4);
}

TEST(IdentitySet, And) {
  IdentitySet result = make_a() & make_b();
  EXPECT_EQ(result.length(), 2);
  EXPECT_TRUE(result.contains(Identity(0, 2)));
  EXPECT_TRUE(result.contains(Identity(1, 1)));
}

TEST(IdentitySet, Xor) {
  IdentitySet result = make_a() ^ make_b();
  EXPECT_EQ(result.length(), 2);  // (0,1) and (2,3)
}

TEST(IdentitySet, Sub) {
  IdentitySet result = make_a() - make_b();
  EXPECT_EQ(result.length(), 1);
  EXPECT_TRUE(result.contains(Identity(0, 1)));
}

TEST(IdentitySet, DifferenceViaInvert) {
  // a & ~b should equal a - b.
  EXPECT_EQ((make_a() & ~make_b()), make_a() - make_b());
}

// --- IdentitySet: high-level set ops ---

TEST(IdentitySet, UnionWithIdentity) {
  IdentitySet result = make_a().union_with(Identity(3, 4));
  EXPECT_EQ(result.length(), 4);
  EXPECT_TRUE(result.contains(Identity(3, 4)));
}

TEST(IdentitySet, UnionWithIterable) {
  std::vector<Identity> extras = {Identity(3, 4), Identity(3, 5)};
  IdentitySet result = make_a().union_with(extras);
  EXPECT_EQ(result.length(), 5);
}

TEST(IdentitySet, IntersectWithIterable) {
  std::vector<Identity> extras = {Identity(0, 1), Identity(5, 5)};
  IdentitySet result = make_a().intersect(extras);
  EXPECT_EQ(result.length(), 1);
  EXPECT_TRUE(result.contains(Identity(0, 1)));
}

TEST(IdentitySet, DifferenceWithIdentity) {
  IdentitySet result = make_a().difference(Identity(0, 1));
  EXPECT_EQ(result.length(), 2);
  EXPECT_FALSE(result.contains(Identity(0, 1)));
}

TEST(IdentitySet, AddRemove) {
  IdentitySet plus = make_a().add(Identity(5, 5));
  EXPECT_EQ(plus.length(), 4);
  EXPECT_TRUE(plus.contains(Identity(5, 5)));

  IdentitySet minus = make_a().remove(Identity(0, 1));
  EXPECT_EQ(minus.length(), 2);
  EXPECT_FALSE(minus.contains(Identity(0, 1)));
}

// --- IdentitySet: predicates ---

TEST(IdentitySet, Filter) {
  IdentitySet result = make_a().filter([](Identity i) { return i.rank == 1; });
  EXPECT_EQ(result.length(), 2);
  for (Identity id : result) EXPECT_EQ(id.rank, 1);
}

TEST(IdentitySet, ForallExists) {
  IdentitySet a = make_a();
  EXPECT_TRUE(a.forall([](Identity i) { return i.suit_index == 0 || i.suit_index == 1; }));
  EXPECT_FALSE(a.forall([](Identity i) { return i.suit_index == 0; }));
  EXPECT_TRUE(a.exists([](Identity i) { return i.rank == 2; }));
  EXPECT_FALSE(a.exists([](Identity i) { return i.rank == 5; }));
}

TEST(IdentitySet, Find) {
  IdentitySet a = make_a();
  auto found = a.find([](Identity i) { return i.rank == 2; });
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(*found, Identity(0, 2));
  EXPECT_FALSE(a.find([](Identity i) { return i.rank == 5; }).has_value());
}

TEST(IdentitySet, Count) {
  IdentitySet a = make_a();
  EXPECT_EQ(a.count([](Identity i) { return i.rank == 1; }), 2);
  EXPECT_EQ(a.count([](Identity i) { return i.rank == 5; }), 0);
}

TEST(IdentitySet, WhenEmpty) {
  IdentitySet empty = IdentitySet::empty();
  IdentitySet fallback = IdentitySet::single(Identity(0, 1));
  EXPECT_EQ(empty.when_empty(fallback), fallback);
  EXPECT_EQ(fallback.when_empty(empty), fallback);
}

// --- IdentitySet: head edge cases ---

TEST(IdentitySet, HeadOfEmptyThrows) {
  EXPECT_THROW(IdentitySet::empty().head(), std::out_of_range);
}

TEST(IdentitySet, HeadReturnsLowestOrd) {
  IdentitySet s = IdentitySet::from_iter(
      {Identity(3, 5), Identity(1, 2), Identity(2, 1)});
  // ords: 19, 6, 11. Lowest is 6 = Identity(1, 2).
  EXPECT_EQ(s.head(), Identity(1, 2));
}

// --- IdentitySet: repr ---

TEST(IdentitySet, ToString) {
  EXPECT_NE(make_a().to_string().find("IdentitySet"), std::string::npos);
}
