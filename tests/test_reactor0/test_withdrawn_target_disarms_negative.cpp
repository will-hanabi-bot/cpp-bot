// A withdrawn target disarms the reaction's held negative (§1d.2, v18.14.0).
//
// The negative waits for the receiver to action their target, and reads what they
// do as their answer to the reaction: a discard means "I had no playable". Once the
// call on that card has been withdrawn -- a dead call erased by the call
// invariants, say -- the receiver throws it because the team reads it as dead, and
// that is no answer to the reaction at all.
//
// TIIAH replay 2014402 T27 is the case: green's o10 had been erased as a dead i2,
// green threw it, and the "receiver discarded" row stripped every playable and
// one-away out of its hand. The rule is reactor0's, so the tests live here.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/conventions/reactor0/call_invariants.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Red on 1, green on 1, so the playables are {r2,y1,g2,b1,p1}. Cathy acts first,
// so the fixture can arm the capture and have her action fire it -- the shape
// `test_reaction_negative_keeps_inference.cpp` uses.
SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::CATHY;
  opts.play_stacks = {1, 0, 1, 0, 0};
  opts.clue_tokens = 5;  // a discard is illegal at 8
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us)
      {"r4", "y4", "b4", "p4", "y3"},  // Bob
      // Cathy: slot 3 is the reactive target, slots 1-2 are passed over.
      {"y5", "p5", "g2", "b5", "p3"},  // Cathy
  };
  use_reactor0(opts);
  return opts;
}

// The capture `arm_reaction_elim` would make, with every slot given the whole
// playable set.
void arm(Game& g) {
  Game::PendingReactionElim p;
  p.active = true;
  p.receiver = static_cast<int>(TestPlayer::CATHY);
  p.target_slot = 3;
  p.receiver_hand = g.state.hands[static_cast<int>(TestPlayer::CATHY)];
  p.target_order = p.receiver_hand[2];
  p.reacter_suit = 3;
  const int slots = static_cast<int>(p.receiver_hand.size());
  p.direct_elim.assign(slots, g.state.playable_set);
  p.finesse_elim.assign(slots, IdentitySet::empty());
  p.trash_elim.assign(slots, IdentitySet::empty());
  g.pending_reaction_elim = std::move(p);
}

// Stamp Cathy's target called to play as `id`.
void call(Game& g, int order, Identity id) {
  g.with_thought(order, [id](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(id);
    return out;
  });
  g.with_meta(order, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
}

constexpr Identity kR1{0, 1}, kR2{0, 2}, kG2{2, 2};

}  // namespace

// The target's call reads as a dead r1 (red is on 1), so the call invariants erase
// it -- and with it the negative waiting on that card. Cathy then throws it, and
// her passed-over slot keeps the playable r2.
TEST(Reactor0WithdrawnTarget, AnErasedCallDisarmsTheNegative) {
  Game g = setup(opts_for());
  const std::vector<int> cathy = g.state.hands[static_cast<int>(TestPlayer::CATHY)];
  const int target = cathy[2];
  const int passed_over = cathy[1];

  arm(g);
  call(g, target, kR1);
  hanabi::reactor0::enforce_call_invariants(g);
  ASSERT_NE(g.meta[target].status, CardStatus::CALLED_TO_PLAY) << "guard: erased as dead";
  EXPECT_FALSE(g.pending_reaction_elim.active) << "the negative went with the call";

  g = take_turn(std::move(g), "Cathy discards g2 (slot 3)", "y1");
  EXPECT_TRUE(g.common.thoughts[passed_over].possibilities().contains(kR2))
      << "throwing a withdrawn card is no answer to the reaction";
}

// The control: the call on the target stands, Cathy throws it anyway, and the
// "receiver discarded" row still narrows the passed-over slot.
TEST(Reactor0WithdrawnTarget, AStandingCallStillFiresTheNegative) {
  Game g = setup(opts_for());
  const std::vector<int> cathy = g.state.hands[static_cast<int>(TestPlayer::CATHY)];
  const int target = cathy[2];
  const int passed_over = cathy[1];

  arm(g);
  call(g, target, kG2);
  hanabi::reactor0::enforce_call_invariants(g);
  ASSERT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY) << "guard: a live call";
  ASSERT_TRUE(g.pending_reaction_elim.active);

  g = take_turn(std::move(g), "Cathy discards g2 (slot 3)", "y1");
  EXPECT_FALSE(g.common.thoughts[passed_over].possibilities().contains(kR2))
      << "she had no playable, so the passed-over slot was not one";
}
