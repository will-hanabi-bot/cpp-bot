// Pink tempo, pink trash and pink identity clues (reactor0 CONVENTION.md §1c,
// priority 0; v19.0.0).
//
// A stable RANK clue that touches no new card and only cards known to be
// pinkish once it lands:
//   1. pink tempo -- two or more of them, and the clue value is the SLOT of one:
//      that card is called to play as the next playable pink;
//   2. pink trash -- the rank is already played in every pinkish suit: the
//      leftmost touched pink card whose rank identity is still open is trash;
//   3. pink identity -- otherwise, that card has the clued rank.
// The cases are the user's worked examples.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

SetupOptions pink_opts(std::vector<std::string> bob, std::vector<int> stacks) {
  SetupOptions opts;
  opts.variant_name = "Pink (6 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      std::move(bob),
      {"r4", "y4", "g3", "b4", "p4"},
  };
  opts.play_stacks = std::move(stacks);
  opts.clue_tokens = 5;
  use_reactor0(opts);
  return opts;
}

bool infs_are(const Game& g, TestPlayer p, int slot, std::vector<std::string> ids) {
  IdentitySet want = IdentitySet::empty();
  for (const auto& s : ids) want = want.add(g.state.expand_short(s));
  return g.common.thoughts[order_at(g, p, slot)].possibilities() == want;
}

}  // namespace

// Pink on 3. Slots 3 and 4 were touched by 2; a 3 re-touches both and reveals
// them as pink. 3 is the slot of one of them, so slot 3 is the next pink: i4.
TEST(Reactor0PinkRankClues, APinkTempoClueCallsTheCardInTheCluedSlot) {
  Game g = setup(pink_opts({"g5", "r1", "i4", "i5", "b5"}, {0, 0, 0, 0, 0, 3}));
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"2"});
  g = pre_clue(std::move(g), TestPlayer::BOB, 4, {"2"});

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_PLAY);
  EXPECT_NE(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(infs_are(g, TestPlayer::BOB, 3, {"i4"}));
}

// Pink on 2, two known pinks on slots 3-4. A 2 is not a tempo clue (slot 2 is
// not pink) and 2 is down, so the leftmost open pink card is trash. Another
// such clue later moves on to the next one, the first being known trash now.
TEST(Reactor0PinkRankClues, APinkTrashClueMarksTheLeftmostOpenPinkCard) {
  Game g = setup(pink_opts({"g5", "g4", "i1", "i2", "b3"}, {0, 0, 0, 0, 0, 2}));
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"pink"});
  g = pre_clue(std::move(g), TestPlayer::BOB, 4, {"pink"});

  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REVEAL);
  EXPECT_TRUE(g.meta[order_at(g, TestPlayer::BOB, 3)].trash);
  EXPECT_FALSE(g.meta[order_at(g, TestPlayer::BOB, 4)].trash);
  EXPECT_TRUE(infs_are(g, TestPlayer::BOB, 3, {"i1", "i2"}));

  // Round the table: Bob's discard moves his pinks to slots 4-5.
  g = take_turn(std::move(g), "Bob discards b3 (slot 5)", "b5");
  g = take_turn(std::move(g), "Cathy discards p4 (slot 5)", "y5");
  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REVEAL);
  EXPECT_TRUE(g.meta[order_at(g, TestPlayer::BOB, 5)].trash)
      << "the i1 is already known trash, so the clue is about the i2";
}

// Pink on 3 and only ONE pink card: no tempo clue, however it sits. 3 is down
// on the only pink stack, so it is trash.
TEST(Reactor0PinkRankClues, OnePinkCardIsNeverATempoClue) {
  Game g = setup(pink_opts({"g5", "g4", "i1", "y1", "b2"}, {0, 0, 0, 0, 0, 3}));
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"pink"});

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REVEAL);
  EXPECT_NE(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(g.meta[order_at(g, TestPlayer::BOB, 3)].trash);
}

// Pink on 2. Slots 3 and 5 were touched by 2; a 4 reveals both as pink. No pink
// card sits on slot 4 and 4 is not down, so this names the leftmost: slot 3 is
// a 4.
TEST(Reactor0PinkRankClues, APinkIdentityClueNamesTheLeftmostOpenPinkCard) {
  Game g = setup(pink_opts({"g5", "g3", "i4", "b3", "i5"}, {0, 0, 0, 0, 0, 2}));
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"2"});
  g = pre_clue(std::move(g), TestPlayer::BOB, 5, {"2"});

  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REVEAL);
  EXPECT_TRUE(infs_are(g, TestPlayer::BOB, 3, {"i4"}));
  EXPECT_FALSE(infs_are(g, TestPlayer::BOB, 5, {"i4"}));
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
}

// The giver never calls a card it can see the tempo reading misnames: slot 3 is
// the i5, and the next pink is the i4.
TEST(Reactor0PinkRankClues, TheGiverDoesNotGiveAMisnamingTempoClue) {
  Game g = setup(pink_opts({"g5", "r1", "i5", "i4", "b5"}, {0, 0, 0, 0, 0, 3}));
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"2"});
  g = pre_clue(std::move(g), TestPlayer::BOB, 4, {"2"});

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::MISTAKE);
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
}

// Pink-Ones: every rank clue but the 1 touches the 1s, and the tempo reading
// (only the tempo reading) applies to them. Two known 1s, a 3 on slot 3: play it.
TEST(Reactor0PinkRankClues, ATempoClueAppliesToPinkishOnes) {
  SetupOptions opts;
  opts.variant_name = "Pink-Ones (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y4", "r1", "b1", "g4", "p4"},
      {"r4", "y5", "g2", "b4", "p5"},
  };
  opts.clue_tokens = 5;
  use_reactor0(opts);
  Game g = setup(opts);
  g = pre_clue(std::move(g), TestPlayer::BOB, 2, {"2"});
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"2"});

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_PLAY);
  EXPECT_NE(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY);
}

// --- the pink promise (user ruling, v19.0.0) ----------------------------------
//
// A rank clue's pink promise -- "this pink card is the clued rank" -- is made
// only on the receiver's LOCK slot, and never in a pinkish-ones / pinkish-fives
// variant. A referential discard makes none.

// Pink 0. A 4 touches only the i1 on slot 3 and points at slot 4 to discard.
// The i1 is NOT promised to be a 4 (replay 2015070 T6), so the clue stands.
TEST(Reactor0PinkRankClues, AReferentialDiscardMakesNoPinkPromise) {
  Game g = setup(pink_opts({"b5", "g3", "i1", "r3", "y2"}, {0, 0, 0, 0, 0, 0}));

  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::DISCARD);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_DISCARD);
  EXPECT_TRUE(g.common.thoughts[order_at(g, TestPlayer::BOB, 3)]
                  .possibilities()
                  .contains(g.state.expand_short("i1")))
      << "the touched pink card is not narrowed to a 4";
}

// Pink-Ones: a 3 touches the lock slot's p1. No promise in this variant, so
// the lock stands rather than being read as a broken promise.
TEST(Reactor0PinkRankClues, APinkishOnesLockMakesNoPinkPromise) {
  SetupOptions opts;
  opts.variant_name = "Pink-Ones (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y4", "g3", "r4", "b2", "p1"},
      {"r5", "y5", "g2", "b4", "p5"},
  };
  opts.clue_tokens = 5;
  use_reactor0(opts);
  Game g = setup(opts);

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::LOCK);
}
