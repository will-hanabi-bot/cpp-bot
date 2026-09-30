// A fix takes precedence over a reverse reactive (tiiah/CONVENTION.md §1c, §1h;
// v18.10.0).
//
// The reviewer's rule: Cathy played a card matching one of the candidates of Bob's
// called card, so from her view Bob's call is still in superposition with a dead
// identity. When Alice then gives Bob a clue that identifies the called card as
// that dead card, both Bob and Cathy know it is a FIX, not a reverse reactive --
// even though Bob's call is what puts the table in the reverse position. Replay
// 2013726 T5. The control is replay 2013963 T10: a clue that leaves Bob's call
// good is the reverse reactive.
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/variants/hole.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

constexpr Identity kR1{0, 1}, kG1{2, 1}, kB1{3, 1}, kB2{3, 2};

std::optional<ClueInterp> interp_of(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* c = std::get_if<ClueInterp>(&g.move_history.back())) return *c;
  return std::nullopt;
}

IdentitySet ids(std::initializer_list<Identity> list) {
  IdentitySet out = IdentitySet::empty();
  for (auto i : list) out = out.add(i);
  return out;
}

// A SETTLED call, unclued -- a receiver's call whose reaction has been played.
void settled_call(Game& g, int order, IdentitySet reading) {
  g.with_thought(order, [reading](const Thought& t) {
    Thought out = t;
    out.inferred = reading;
    return out;
  });
  g.with_meta(order, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = false;
  });
}

SetupOptions opts(std::vector<std::string> bob, std::vector<std::string> cathy) {
  SetupOptions o;
  o.variant_name = "Throw It in a Hole (5 Suits)";
  o.hands = {{"xx", "xx", "xx", "xx", "xx"}, std::move(bob), std::move(cathy)};
  o.starting = TestPlayer::ALICE;
  o.clue_tokens = 6;
  use_tiiah(o);
  return o;
}

}  // namespace

// From the GIVER's seat. Cathy threw her r1 into the hole as a `{r1,g1}`; Bob's r1
// is called as `{r1,b2}`. Our Red narrows it to the r1: a fix.
TEST(TiiahFixBeforeReverseReactive, TheGiverReadsItsRedAsAFix) {
  SetupOptions o = opts({"r1", "y4", "g4", "b4", "p4"}, {"r1", "y3", "g3", "b3", "p3"});
  o.starting = TestPlayer::CATHY;
  Game g = setup(std::move(o));
  const int cathys = order_at(g, TestPlayer::CATHY, 1);
  const int bobs = order_at(g, TestPlayer::BOB, 1);
  g = hidden_action(std::move(g), TestPlayer::CATHY, 1, /*reached_the_hole=*/true);
  g.with_meta(cathys, [](ConvData& m) { m.superposition = ids({kR1, kG1}); });
  settled_call(g, bobs, ids({kR1, kB2}));
  ASSERT_TRUE(reactor::variants::reverse_reactive_position(g, 0))
      << "guard: Bob's settled call puts the table in the reverse position";

  g = take_turn(std::move(g), "Alice clues red to Bob");

  EXPECT_EQ(interp_of(g), ClueInterp::FIX);
  EXPECT_TRUE(g.waiting.empty()) << "no reverse reactive is installed";
}

// From CATHY's seat -- we are the giver's Cathy, and it was OUR blind play. We
// cannot name it, but the Red naming Bob's card as the r1 tells us: a fix.
TEST(TiiahFixBeforeReverseReactive, CathyReadsItFromHerOwnBlindPlay) {
  Game g = setup(opts({"y4", "g4", "b4", "p4", "r4"}, {"r1", "y3", "g3", "b3", "p3"}));
  const int ours = order_at(g, TestPlayer::ALICE, 1);
  const int cathys = order_at(g, TestPlayer::CATHY, 1);
  g = hidden_action(std::move(g), TestPlayer::ALICE, 1, /*reached_the_hole=*/true);
  g.with_meta(ours, [](ConvData& m) { m.superposition = ids({kR1, kG1}); });
  // Bob gives next: his Bob is Cathy, who holds the settled `{r1,b2}` call, and
  // his Cathy is us.
  settled_call(g, cathys, ids({kR1, kB2}));
  ASSERT_TRUE(reactor::variants::reverse_reactive_position(g, 1));

  g = take_turn(std::move(g), "Bob clues red to Cathy");

  EXPECT_EQ(interp_of(g), ClueInterp::FIX);
  EXPECT_TRUE(g.waiting.empty());
}

// The control: Bob's call is `{b1,b2}`, and a Blue leaves it on both -- nothing is
// named dead, so the clue to Bob is the reverse reactive it would be anyway.
TEST(TiiahFixBeforeReverseReactive, AClueThatLeavesTheCallGoodIsTheReverseReactive) {
  Game g = setup(opts({"b1", "y4", "g4", "r4", "p4"}, {"r1", "y3", "g3", "b3", "p3"}));
  const int bobs = order_at(g, TestPlayer::BOB, 1);
  settled_call(g, bobs, ids({kB1, kB2}));
  ASSERT_TRUE(reactor::variants::reverse_reactive_position(g, 0));

  g = take_turn(std::move(g), "Alice clues blue to Bob");

  EXPECT_NE(interp_of(g), ClueInterp::FIX);
  EXPECT_FALSE(g.waiting.empty()) << "read as the reverse reactive";
}
