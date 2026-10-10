// THE SELF COLOUR BLUFF (tiiah/CONVENTION.md §1b, v23.16.0, experimental; the
// user's convention; self-play Dark Null seed 96 T13).
//
// Alice, not locked and below 8 tokens, gives Bob a COLOUR clue that re-touches
// only cards already clued, none of which could play, and that is no play or trash
// reveal. It calls Bob's leftmost card that could be the special suit's (the
// variant's last suit's) next card. Not under the rainbowish suits, and only where
// the clue is read stable -- the reverse position keeps its reverse reactive.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

void known_as(Game& g, int order, const IdentitySet& set) {
  g.state.deck[order].clued = true;
  g.with_thought(order, [&set](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    out.possible = set;
    return out;
  });
}

// Blue on `blue` (2 by default); everything else on 0, the special suit on
// `special`. Bob's slot 5 is a b5 clued long ago and known.
Game position(const std::string& variant, const std::string& special_one,
              int clue_tokens = 5, int blue = 2, int special = 0,
              const std::string& bob_slot2 = "r3") {
  SetupOptions opts;
  opts.variant_name = variant;
  opts.play_stacks = std::vector<int>{0, 0, 0, blue, 0, special};
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {special_one, bob_slot2, "g4", "y4", "b5"},
                {"g3", "y3", "r4", "p4", "y5"}};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = clue_tokens;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  known_as(g, order_at(g, TestPlayer::BOB, 5), IdentitySet::single(Identity{3, 5}));
  return g;
}

const char* kDarkNull = "Throw It in a Hole & Dark Null (6 Suits)";
const char* kBlack = "Throw It in a Hole & Black (6 Suits)";

}  // namespace

// Dark Null: Blue re-touches only the known b5, so it calls slot 1 as the null 1.
TEST(TiiahSelfColourBluff, DarkNullCallsTheNullOne) {
  Game g0 = position(kDarkNull, "u1");
  Game g = take_turn(g0, "Alice clues blue to Bob");
  const int slot1 = order_at(g, TestPlayer::BOB, 1);
  EXPECT_EQ(g.meta[slot1].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[slot1].inferred, IdentitySet::single(Identity{5, 1}));
  EXPECT_NE(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY);
}

// Black: the same clue calls the black 1.
TEST(TiiahSelfColourBluff, BlackCallsTheBlackOne) {
  Game g = take_turn(position(kBlack, "k1"), "Alice clues blue to Bob");
  const int slot1 = order_at(g, TestPlayer::BOB, 1);
  EXPECT_EQ(g.meta[slot1].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[slot1].inferred, IdentitySet::single(Identity{5, 1}));
}

// The special suit's next card is the 3 when it is on 2.
TEST(TiiahSelfColourBluff, CallsTheNextSpecialCard) {
  Game g = take_turn(position(kBlack, "k3", 5, 2, /*special=*/2), "Alice clues blue to Bob");
  const int slot1 = order_at(g, TestPlayer::BOB, 1);
  EXPECT_EQ(g.meta[slot1].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[slot1].inferred, IdentitySet::single(Identity{5, 3}));
}

// At 8 tokens it is a stall.
TEST(TiiahSelfColourBluff, NotAtEightClues) {
  Game g = take_turn(position(kDarkNull, "u1", 8), "Alice clues blue to Bob");
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
}

// With blue on 4 the touched b5 plays, so the clue is no bluff.
TEST(TiiahSelfColourBluff, APlayableTouchedCardIsNotABluff) {
  Game g = take_turn(position(kDarkNull, "u1", 5, /*blue=*/4), "Alice clues blue to Bob");
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
}

// A clue that touches a new card is read by the ordinary ladder.
TEST(TiiahSelfColourBluff, ANewlyTouchedCardIsNotABluff) {
  Game g = take_turn(position(kDarkNull, "u1", 5, 2, 0, /*bob_slot2=*/"b3"),
                     "Alice clues blue to Bob");
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY);
}

// A completed special suit has no next card: a stall.
TEST(TiiahSelfColourBluff, NothingWhenTheSpecialSuitIsDone) {
  Game g = take_turn(position(kDarkNull, "r2", 5, 2, /*special=*/5), "Alice clues blue to Bob");
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
}

// Not under a rainbowish suit.
TEST(TiiahSelfColourBluff, NotUnderRainbow) {
  Game g = take_turn(position("Throw It in a Hole & Rainbow (6 Suits)", "y1"),
                     "Alice clues blue to Bob");
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
}

// Only a clue to Bob: the same re-touch to Cathy keeps its reactive reading, and
// calls nothing in Cathy's hand by itself.
TEST(TiiahSelfColourBluff, NotToCathy) {
  SetupOptions opts;
  opts.variant_name = kDarkNull;
  opts.play_stacks = std::vector<int>{0, 0, 0, 2, 0, 0};
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"g3", "y3", "r4", "p4", "y5"},
                {"u1", "r3", "g4", "y4", "b5"}};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  Game g0 = setup(std::move(opts));
  known_as(g0, order_at(g0, TestPlayer::CATHY, 5), IdentitySet::single(Identity{3, 5}));
  Game g = take_turn(g0, "Alice clues blue to Cathy");
  const int slot1 = order_at(g, TestPlayer::CATHY, 1);
  EXPECT_FALSE(g.meta[slot1].status == CardStatus::CALLED_TO_PLAY &&
               g.common.thoughts[slot1].inferred == IdentitySet::single(Identity{5, 1}));
}
