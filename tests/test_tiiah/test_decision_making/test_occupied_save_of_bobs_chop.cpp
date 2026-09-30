// An OCCUPIED Alice saves a stuck Bob's chop when Cathy's chop is safe to lose
// (tiiah/CONVENTION.md §2e, v18.13.0).
//
// The reviewer's rule, human diagnostic 2014076 T18: Alice holds a call of her own,
// Bob's chop is a playable card at risk and he has nothing else to do, so he throws it
// next turn. Alice saves it with a clue when Cathy's chop is **not critical** and
// **not a playable card unless duplicated** -- in Cathy's own hand, or as a called
// card with a globally known identity in anyone else's -- and Bob's chop is not itself
// duplicated by such a call. Otherwise she plays her call.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
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

void pin_call(Game& g, int order, Identity id) {
  g.with_thought(order, [id](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(id);
    out.possible = IdentitySet::single(id);
    return out;
  });
  g.with_meta(order, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
}

constexpr Identity kR1{0, 1}, kB1{3, 1};

// Empty stacks. Alice (us) holds a known b1 call, so she is OCCUPIED. Bob's chop
// (slot 1, the newest unclued card) is a playable g1, and he has nothing else to do.
// Cathy's slot 1 is a known r1 call, so her chop is slot 2: `cathys_chop`.
Game position(const char* cathys_chop, const char* cathys_slot_3 = "g3") {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},          // Alice (us)
      {"g1", "r4", "y4", "b4", "p4"},          // Bob
      {"r1", cathys_chop, cathys_slot_3, "b3", "p3"},  // Cathy
  };
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  pin_call(g, order_at(g, TestPlayer::ALICE, 1), kB1);
  pin_call(g, order_at(g, TestPlayer::CATHY, 1), kR1);
  return g;
}

bool saves_bobs_g1(const PerformAction& a) {
  if (const auto* r = std::get_if<PerformRank>(&a)) return r->target == 1 && r->value == 1;
  if (const auto* c = std::get_if<PerformColour>(&a)) return c->target == 1 && c->value == 2;
  return false;
}

bool plays_our_call(const Game& g, const PerformAction& a) {
  const auto* p = std::get_if<PerformPlay>(&a);
  return p && p->target == order_at(g, TestPlayer::ALICE, 1);
}

}  // namespace

// Cathy's chop is an r3: not critical, not playable. Bob's g1 is saved -- even though
// Bob could give Cathy a colour play clue on her y1 himself, which is what keeps the
// save LOW (H1c) and so behind the tier gate, as at 2014076 T18.
TEST(TiiahOccupiedSaveOfBobsChop, SavedWhenCathysChopIsSafe) {
  Game g = position("r3", "y1");
  const PerformAction a = g.take_action();
  EXPECT_TRUE(saves_bobs_g1(a)) << "a 1 or Green to Bob";
}

// Cathy's chop is a b1: playable, but our own known b1 call duplicates it.
TEST(TiiahOccupiedSaveOfBobsChop, SavedWhenCathysPlayableChopIsACalledDupe) {
  Game g = position("b1");
  const PerformAction a = g.take_action();
  EXPECT_TRUE(saves_bobs_g1(a)) << "a 1 or Green to Bob";
}

// The controls: Cathy's chop is a y5, critical...
TEST(TiiahOccupiedSaveOfBobsChop, NotWhenCathysChopIsCritical) {
  Game g = position("y5");
  const PerformAction a = g.take_action();
  EXPECT_TRUE(plays_our_call(g, a)) << "Alice plays her b1";
}

// ...or a y1, playable with no other copy anyone is holding a call on.
TEST(TiiahOccupiedSaveOfBobsChop, NotWhenCathysChopIsAnUnduplicatedPlayable) {
  Game g = position("y1");
  const PerformAction a = g.take_action();
  EXPECT_TRUE(plays_our_call(g, a)) << "Alice plays her b1";
}

// ...or Bob's g1 is one our own known call already duplicates: a clue on it would
// call a second g1 to play, not save anything (self-play seed 119 T17, v18.13.0).
TEST(TiiahOccupiedSaveOfBobsChop, NotWhenOurOwnCallDuplicatesBobsChop) {
  Game g = position("r3", "y1");
  pin_call(g, order_at(g, TestPlayer::ALICE, 1), Identity{2, 1});
  const PerformAction a = g.take_action();
  EXPECT_TRUE(plays_our_call(g, a)) << "Alice plays her g1";
}

// ...or Cathy holds the other g1 in plain view: Bob's chop is playable but not at
// risk, so there is nothing to save (self-play seed 105 T2, v18.13.0).
TEST(TiiahOccupiedSaveOfBobsChop, NotWhenBobsChopIsNotAtRisk) {
  Game g = position("r3", "g1");
  const PerformAction a = g.take_action();
  EXPECT_TRUE(plays_our_call(g, a)) << "Alice plays her b1";
}
