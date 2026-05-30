// Port of python-bot/tests/test_basics/test_clue.py.
#include <set>

#include <gtest/gtest.h>

#include "hanabi/basics/clue.h"

using hanabi::BaseClue;
using hanabi::CardClue;
using hanabi::Clue;
using hanabi::ClueKind;

TEST(Clue, ClueKindValues) {
  EXPECT_EQ(static_cast<int>(ClueKind::COLOUR), 0);
  EXPECT_EQ(static_cast<int>(ClueKind::RANK), 1);
}

TEST(Clue, BaseClueHashIntDistinctPerKindAndValue) {
  EXPECT_EQ(BaseClue(ClueKind::COLOUR, 0).hash_int(), 0);
  EXPECT_EQ(BaseClue(ClueKind::COLOUR, 4).hash_int(), 4);
  EXPECT_EQ(BaseClue(ClueKind::RANK, 0).hash_int(), 10);
  EXPECT_EQ(BaseClue(ClueKind::RANK, 5).hash_int(), 15);
  std::set<int> hashes;
  for (int v = 0; v < 6; ++v) hashes.insert(BaseClue(ClueKind::COLOUR, v).hash_int());
  for (int v = 1; v < 6; ++v) hashes.insert(BaseClue(ClueKind::RANK, v).hash_int());
  EXPECT_EQ(hashes.size(), 11u);
}

TEST(Clue, BaseClueToClue) {
  BaseClue bc(ClueKind::COLOUR, 2);
  EXPECT_EQ(hanabi::to_clue(bc, 3), Clue(ClueKind::COLOUR, 2, 3));
}

TEST(Clue, CardClueBase) {
  CardClue cc(ClueKind::RANK, 3, /*giver=*/1, /*turn=*/5);
  EXPECT_EQ(cc.base(), BaseClue(ClueKind::RANK, 3));
}

TEST(Clue, ClueBase) {
  Clue c(ClueKind::COLOUR, 1, /*target=*/0);
  EXPECT_EQ(c.base(), BaseClue(ClueKind::COLOUR, 1));
}
