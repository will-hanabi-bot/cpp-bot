// `known_safe_discard` -- can the holder PROVE that pressing Discard here is a
// safe burn?
//
// Two halves, and the tests below pin each separately.
//
// 1. PRIVATE SIGHT. `sight_narrowed` starts from raw empathy and REFUTES
//    identities whose every copy the seat can already place in somebody else's
//    hand. Common knowledge cannot do this -- `card_elim` accounts for a copy
//    only when a CLUE pins it -- so a duplicate sitting unclued in a partner's
//    hand never reaches the Player layer.
//
// 2. THE SUIT MUST BE PLAIN. Pressing Discard on an inverted card is a PLAY
//    attempt, so a trash orange STRIKES rather than burning (replay 1966569
//    T10). `provably_trash` is true for such a card and `known_safe_discard`
//    must not be -- that gap is the whole reason the second predicate exists.
//
// The endgame fork's `prefer_known_discard` (v11.6.0, `src/basics/decide.cpp`)
// is the caller: it refuses to burn a card it cannot prove is worthless while
// one it can sits in the same hand. Replay 1977971 T22 is the motivating game.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// r/y/g/b/o with orange INVERTED. Red is on 2 and orange on 2, so r1/r2 and
// o1/o2 are all basic trash -- one plain, one inverted.
SetupOptions opts(const std::vector<std::string>& bob) {
  SetupOptions o;
  o.variant_name = "Orange (5 Suits)";
  o.play_stacks = {2, 0, 0, 0, 2};
  o.starting = TestPlayer::ALICE;
  o.hands = {{"r1", "r2", "y1", "g1", "b1"}, bob, {"y3", "g3", "b3", "y4", "g4"}};
  use_reactor0(o);
  return o;
}

// Empathy is what `sight_narrowed` starts from, so the test writes it directly
// into COMMON -- which is the layer the predicate reads.
void believe(Game& g, int order, std::initializer_list<Identity> ids) {
  const IdentitySet set = IdentitySet::from_iter(ids);
  g.with_thought(order, [set](const Thought& t) {
    Thought out = t;
    out.possible = set;
    out.inferred = set;
    return out;
  });
}

constexpr Identity kR1{0, 1};
constexpr Identity kR5{0, 5};
constexpr Identity kO1{4, 1};

}  // namespace

// The r5 is the only one in the game. With it face-up in Bob's hand, a card
// that empathy still calls {r1, r5} can only be the r1 -- and the r1 is trash
// on a plain suit, so the burn is provably safe.
TEST(Reactor0KnownSafeDiscard, SightRefutesTheCriticalTwin) {
  Game g = setup(opts({"r5", "y2", "g2", "b2", "y5"}));
  const int order = g.state.hands[0][0];
  believe(g, order, {kR1, kR5});

  EXPECT_TRUE(hanabi::reactor0::provably_trash(g, order));
  EXPECT_TRUE(hanabi::reactor0::known_safe_discard(g, order))
      << "the last r5 is visible in Bob's hand, so this card can only be the "
         "trash r1";
}

// The same empathy, the same card -- but nothing accounts for the r5, so it
// might BE the r5. Burning it could cost the team two points, and the predicate
// must say so. This is the control that shows the test above turns on sight
// rather than on the fixture.
TEST(Reactor0KnownSafeDiscard, WithoutSightTheSameCardIsNotProvable) {
  Game g = setup(opts({"y2", "g2", "b2", "y5", "g5"}));
  const int order = g.state.hands[0][0];
  believe(g, order, {kR1, kR5});

  EXPECT_FALSE(hanabi::reactor0::provably_trash(g, order));
  EXPECT_FALSE(hanabi::reactor0::known_safe_discard(g, order))
      << "the r5 is unaccounted for, so this card is not provably worthless";
}

// Both possibilities are trash, so `provably_trash` is TRUE -- and the card
// still must not be chucked, because if it is the o1 the Discard button plays
// it and a trash orange is not playable. The predicates must disagree here.
TEST(Reactor0KnownSafeDiscard, TrashOnAnInvertedSuitIsNotASafeBurn) {
  Game g = setup(opts({"y2", "g2", "b2", "y5", "g5"}));
  const int order = g.state.hands[0][0];
  believe(g, order, {kR1, kO1});
  ASSERT_TRUE(g.state.is_basic_trash(kO1)) << "guard: orange is on 2";

  EXPECT_TRUE(hanabi::reactor0::provably_trash(g, order))
      << "every possibility is trash, which is all this predicate asks";
  EXPECT_FALSE(hanabi::reactor0::known_safe_discard(g, order))
      << "chucking it would be a PLAY attempt on a trash orange -- a strike";
}

// An unclued card with full empathy proves nothing, so neither predicate fires.
TEST(Reactor0KnownSafeDiscard, NoKnowledgeIsNotAProof) {
  Game g = setup(opts({"y2", "g2", "b2", "y5", "g5"}));
  const int order = g.state.hands[0][4];
  EXPECT_FALSE(hanabi::reactor0::known_safe_discard(g, order));
}
