// A reaction's negative narrows WITHIN the inference; it never widens it
// (§1d.2 / §1i, v16.17.0).
//
// The negative is an argument of the form "if that slot had been an X, the clue
// would have named it instead". It can only ever REMOVE candidates — so it must
// not be able to hand a card back a candidate its inference had already ruled
// out, and when it would remove every candidate the promise wins.
//
// It used to take its keep-set as `possible − set`. On a card already narrowed to
// one identity that intersects the inference to nothing, so `narrow_thought`
// escalated to empathy and returned `possible − set`: a widening, and one that
// silently replaced a promise with the complement of it.
//
// TIIAH replay 2010329 is the case that found it — yagami's order 8 was called to
// play and read `{p1}`, its call was later withdrawn (which by §1i keeps the
// inference), and the negative then rewrote it as `{p3,p4,p5}`, the one set that
// excludes what the card was. The rule is reactor0's, so the fix and these tests
// live here rather than under tiiah.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Red on 1, green on 1, so the playables are {r2,y1,g2,b1,p1}. Cathy acts first
// so the fixture can arm the capture and have her action fire it — the same shape
// `test_deferred_elim.cpp` uses, and for the same reason: the arming is covered
// end to end by the reactive suites.
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
// playable set — the firing half is what is under test.
void arm(Game& g) {
  Game::PendingReactionElim p;
  p.active = true;
  p.receiver = static_cast<int>(TestPlayer::CATHY);
  p.target_slot = 3;
  p.receiver_hand = g.state.hands[static_cast<int>(TestPlayer::CATHY)];
  p.target_order = p.receiver_hand[2];
  p.reacter_suit = 3;  // the reacter advanced BLUE: an ordinary double play
  const int slots = static_cast<int>(p.receiver_hand.size());
  p.direct_elim.assign(slots, g.state.playable_set);
  p.finesse_elim.assign(slots, IdentitySet::empty());
  p.trash_elim.assign(slots, IdentitySet::empty());
  g.pending_reaction_elim = std::move(p);
}

// Pin one of Cathy's passed-over slots to a single identity, the way a call
// would have. `common` is the view the negative reads.
void pin(Game& g, int order, Identity id) {
  const IdentitySet one = IdentitySet::single(id);
  g.with_thought(order, [&one](const Thought& t) {
    Thought out = t;
    out.inferred = one;
    return out;
  });
}

}  // namespace

// A promise of one identity survives a negative that would have removed it.
TEST(Reactor0ReactionNegative, APromiseSurvivesANegativeThatWouldEmptyIt) {
  Game g = setup(opts_for());
  const std::vector<int> cathy = g.state.hands[static_cast<int>(TestPlayer::CATHY)];
  const int passed_over = cathy[0];  // slot 1
  const Identity r2{0, 2};           // directly playable, so `direct_elim` has it

  pin(g, passed_over, r2);
  arm(g);
  g = take_turn(std::move(g), "Cathy plays g2 (slot 3)", "y1");

  EXPECT_EQ(g.common.thoughts[passed_over].inferred, IdentitySet::single(r2))
      << "the negative would remove the r2 and leave nothing, so the promise "
         "wins -- it is not evidence that what the team inferred was wrong. "
         "Before v16.17.0 this came back as `possible` minus the playables";
  EXPECT_TRUE(g.common.thoughts[passed_over].inferred.length() == 1)
      << "and it is certainly not WIDER than it was";
}

// The control: on a card with room to lose a candidate, the negative still does
// its job.
TEST(Reactor0ReactionNegative, ANegativeStillRemovesWhatItRefutes) {
  Game g = setup(opts_for());
  const std::vector<int> cathy = g.state.hands[static_cast<int>(TestPlayer::CATHY)];
  const int passed_over = cathy[0];
  const Identity r2{0, 2};   // playable  -> refuted by the negative
  const Identity r4{0, 4};   // not playable -> survives

  IdentitySet two = IdentitySet::empty();
  two = two.add(r2);
  two = two.add(r4);
  g.with_thought(passed_over, [&two](const Thought& t) {
    Thought out = t;
    out.inferred = two;
    return out;
  });
  arm(g);
  g = take_turn(std::move(g), "Cathy plays g2 (slot 3)", "y1");

  EXPECT_FALSE(g.common.thoughts[passed_over].inferred.contains(r2))
      << "a slot the walk passed over was not directly playable";
  EXPECT_TRUE(g.common.thoughts[passed_over].inferred.contains(r4))
      << "and what the negative says nothing about is left alone";
}

// A card with no inference of its own still falls back to `possible`, so the
// negative applies to it in full.
TEST(Reactor0ReactionNegative, AnUninferredCardIsStillNarrowed) {
  Game g = setup(opts_for());
  const std::vector<int> cathy = g.state.hands[static_cast<int>(TestPlayer::CATHY)];
  const int passed_over = cathy[1];  // slot 2, untouched by any clue
  const Identity r2{0, 2};

  ASSERT_TRUE(g.common.thoughts[passed_over].inferred.contains(r2))
      << "guard: it admits the playable to begin with";
  arm(g);
  g = take_turn(std::move(g), "Cathy plays g2 (slot 3)", "y1");

  EXPECT_FALSE(g.common.thoughts[passed_over].inferred.contains(r2))
      << "nothing was promised here, so the negative is the only thing speaking";
}
