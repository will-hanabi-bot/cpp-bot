// THE RE-TOUCH RANK CALL (tiiah/CONVENTION.md §1b, v22.2.0, the user's ruling;
// replay 2021455 T61).
//
// A stable rank clue from Alice to Bob that touches no new card -- from an Alice
// who is not locked and not at 8 clues, in a variant with no pinkish suit -- calls
// the RIGHTMOST touched card that could be playable, read as its playable
// identities. Until v22.2.0 it was a stall.
#include <gtest/gtest.h>

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

// Purple on `purple` (4 by default), blue on 2. Bob's slots 4 and 5 were clued 5 long ago and read
// `{b5,p5}` (slot 5) and `{r5,b5}` (slot 4).
Game position(int clue_tokens, int purple = 4) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.play_stacks = std::vector<int>{0, 0, 0, 2, purple};
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"r1", "y2", "g3", "r5", "p5"},
                {"g4", "y3", "r3", "b4", "y4"}};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = clue_tokens;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  const Identity r5{0, 5}, b5{3, 5}, p5{4, 5};
  known_as(g, order_at(g, TestPlayer::BOB, 5), IdentitySet::empty().add(b5).add(p5));
  known_as(g, order_at(g, TestPlayer::BOB, 4), IdentitySet::empty().add(r5).add(b5));
  return g;
}

}  // namespace

// The 5 re-touches both. The rightmost, slot 5, could be the p5: it is called, `{p5}`.
TEST(TiiahRetouchRankCall, TheRightmostPossiblePlayIsCalled) {
  Game g = take_turn(position(4), "Alice clues 5 to Bob");
  const int slot5 = order_at(g, TestPlayer::BOB, 5);
  EXPECT_EQ(g.meta[slot5].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[slot5].inferred, IdentitySet::single(Identity{4, 5}));
  EXPECT_NE(g.meta[order_at(g, TestPlayer::BOB, 4)].status, CardStatus::CALLED_TO_PLAY);
}

// At 8 clues the same clue is a stall.
TEST(TiiahRetouchRankCall, NotAtEightClues) {
  Game g = take_turn(position(8), "Alice clues 5 to Bob");
  EXPECT_NE(g.meta[order_at(g, TestPlayer::BOB, 5)].status, CardStatus::CALLED_TO_PLAY);
}

// With purple on 3 nothing it touches could be playable: a stall.
TEST(TiiahRetouchRankCall, NothingPlayableIsAStall) {
  Game g = take_turn(position(4, /*purple=*/3), "Alice clues 5 to Bob");
  EXPECT_NE(g.meta[order_at(g, TestPlayer::BOB, 5)].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_NE(g.meta[order_at(g, TestPlayer::BOB, 4)].status, CardStatus::CALLED_TO_PLAY);
}
