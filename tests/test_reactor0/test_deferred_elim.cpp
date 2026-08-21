// A receiver-chuck's negative inference is held until the chuck is known.
//
// Both reactive branches that call the receiver to CHUCK -- colour + reacter
// played (`elim_play_dc`) and rank + reacter discarded (`elim_dc_dc`) -- reason
// "the slots the walk passed over are not playable". That holds only if the
// chuck is really a DISCARD. On an inverted suit a chuck puts the card on its
// stack, so it is a PLAY: nothing was passed over and nothing is owed.
//
// So the inference is captured (`Game::pending_dc_elim`) rather than applied,
// and `resolve_pending_dc_elim` decides its fate later:
//
//   every reading says it was a playable inverted -> void it
//   no reading says so                            -> apply it
//   otherwise                                     -> keep holding
//
// The receiver does NOT have to chuck the card for this to fire -- knowing is
// enough. And the question is asked against the CLUE-TIME playable set, not the
// current stacks.
//
// Replay 1966710 is the motivating case: a rank double discard applied
// `elim_dc_dc` at once, stripping a playable b2 from the receiver's slot 3. The
// receiver's called card was an Orange 1 with the orange stack on 0, so the
// chuck scored and the reaction was never a double discard at all.
#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// "Orange (3 Suits)" -- r / b / o, with orange (index 2) inverted.
constexpr int kOrange = 2;

SetupOptions inv_opts() {
  SetupOptions opts;
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0};
  opts.hands = {
      // Alice (us). Face-down: the harness hands out ground truth otherwise,
      // and the whole point of the receiver-side tests is that we CANNOT see
      // our own card, so `deck[].id()` must come back nullopt.
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "b1", "r2", "b2", "o5"},  // Bob   (reacter) -- slots 1/2 playable
      {"r3", "b3", "o1", "r4", "b4"},  // Cathy (receiver) -- slot 3 is orange
  };
  use_reactor0(opts);
  return opts;
}

// Hand-build the capture `defer_receiver_chuck_elim` would have made.
//
// focus_slot 2 with a hand of 5 maps receiver slot 1 -> reacter slot 1
// (`calc_slot(2,1,5) == 1`), which is a playable in every fixture here, so an
// applied elim is observable on the receiver's slot 1. target_slot 3 makes the
// receiver's slot 3 the called card.
void arm_pending(Game& g, TestPlayer receiver, TestPlayer reacter) {
  const int r = static_cast<int>(receiver);
  Game::PendingDcElim p;
  p.kind = Game::PendingDcElim::Kind::DcDc;
  p.active = true;
  p.hand_size = 5;
  p.focus_slot = 2;
  p.target_slot = 3;
  p.receiver_hand = g.state.hands[r];
  p.reacter_hand = g.state.hands[static_cast<int>(reacter)];
  p.playable = g.state.playable_set;
  p.trash = g.state.trash_set;
  p.critical = g.state.critical_set;
  for (int o : p.receiver_hand) {
    p.receiver_was_clued.push_back(g.state.deck[o].clued ? 1 : 0);
  }
  p.target_order = p.receiver_hand[2];
  p.target_was_clued = g.state.deck[p.target_order].clued;
  g.pending_dc_elim = std::move(p);
}

std::map<int, IdentitySet> hand_inferred(const Game& g, TestPlayer who) {
  std::map<int, IdentitySet> out;
  for (int o : g.state.hands[static_cast<int>(who)]) {
    out[o] = g.common.thoughts[o].inferred;
  }
  return out;
}

void pin_common(Game& g, int order, IdentitySet ids) {
  g.with_thought(order, [ids](const Thought& t) {
    Thought out = t;
    out.inferred = ids;
    return out;
  });
}

}  // namespace

// --- the deferral itself -------------------------------------------------

// The observer half of the rule, through the real reactive path.
//
// Our bot is Alice and the receiver is Cathy, so we can SEE her called card. It
// is an r1 with red on 1 -- plain, and trash -- so the chuck really was a
// discard, the question is settled the instant the reaction lands, and no
// pending survives the turn. An observer therefore behaves exactly as it did
// before the deferral existed, which is the point of ruling 2: everyone who can
// see the card gets the right inference immediately, and only the receiver
// waits.
//
// The receiver half is covered by the unit tests below and end-to-end by
// `MiscReplay1966710`.
TEST(Reactor0DeferredElim, ObserverSettlesTheNegativeAtReactionTime) {
  SetupOptions opts;
  opts.play_stacks = {1, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y2", "y3", "b3", "g4", "y4"},
      {"y4", "b4", "r1", "g4", "p4"},
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 4 to Cathy");
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_DISCARD);
  auto before = hand_inferred(g, TestPlayer::CATHY);
  const int target = order_at(g, TestPlayer::CATHY, 3);

  g = take_turn(std::move(g), "Bob discards y2 (slot 1)", "r3");

  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_DISCARD)
      << "the dc-target is still stamped -- only the ELIM is deferred";
  ASSERT_EQ(g.state.deck[target].suit_index, 0) << "guard: the called card is r1";
  EXPECT_FALSE(g.pending_dc_elim.active)
      << "we can see the called card, so nothing is left hanging -- an "
         "observer never holds a pending inference";
  (void)before;
}

// --- the resolution rule, observer side ----------------------------------
//
// Every seat except the receiver can SEE the called card, so it resolves at
// once. `state.deck[o].id()` is that knowledge and is already POV-aware.

TEST(Reactor0DeferredElim, VoidedWhenTheCalledCardIsAPlayableInverted) {
  Game g = setup(inv_opts());
  // Cathy's slot 3 is an Orange 1 with orange on 0 -- chucking it scores, so
  // the reaction was a play and no negative is owed.
  ASSERT_EQ(g.state.deck[order_at(g, TestPlayer::CATHY, 3)].suit_index, kOrange);
  arm_pending(g, TestPlayer::CATHY, TestPlayer::BOB);
  auto before = hand_inferred(g, TestPlayer::CATHY);

  g.resolve_deferred_elims();

  EXPECT_FALSE(g.pending_dc_elim.active) << "resolved, not left hanging";
  for (const auto& entry : before) {
    EXPECT_EQ(g.common.thoughts[entry.first].inferred, entry.second)
        << "order " << entry.first
        << ": a chuck that scores passed over nothing";
  }
}

TEST(Reactor0DeferredElim, AppliedWhenTheCalledCardIsPlain) {
  SetupOptions opts = inv_opts();
  // Slot 3 is now a plain b3, so the chuck really was a discard.
  opts.hands[2] = {"r3", "b3", "b3", "r4", "b4"};
  Game g = setup(std::move(opts));
  arm_pending(g, TestPlayer::CATHY, TestPlayer::BOB);
  const int slot1 = order_at(g, TestPlayer::CATHY, 1);
  const IdentitySet before = g.common.thoughts[slot1].inferred;
  ASSERT_TRUE(before.exists([&](Identity i) { return g.state.is_playable(i); }))
      << "guard: slot 1 must still admit a playable, or there is nothing to see";

  g.resolve_deferred_elims();

  EXPECT_FALSE(g.pending_dc_elim.active);
  EXPECT_NE(g.common.thoughts[slot1].inferred, before)
      << "the walk passed slot 1 over, so it is not playable";
  EXPECT_FALSE(g.common.thoughts[slot1].inferred.exists(
      [&](Identity i) { return g.state.is_playable(i); }));
}

// --- the resolution rule, receiver side ----------------------------------
//
// The receiver cannot see their own card, so `deck[].id()` is nullopt and the
// rule falls back to empathy. This is the branch that has to WAIT.

TEST(Reactor0DeferredElim, HeldWhileTheReceiverCannotTellWhichItWas) {
  Game g = setup(inv_opts());
  arm_pending(g, TestPlayer::ALICE, TestPlayer::BOB);  // our own hand
  const int target = order_at(g, TestPlayer::ALICE, 3);
  ASSERT_FALSE(g.state.deck[target].id().has_value())
      << "guard: we must not be able to see our own card";
  // Two live readings: a playable orange, and a plain card. Undecided.
  pin_common(g, target,
             IdentitySet::single(Identity{kOrange, 1}).add(Identity{1, 3}));
  auto before = hand_inferred(g, TestPlayer::ALICE);

  g.resolve_deferred_elims();

  EXPECT_TRUE(g.pending_dc_elim.active)
      << "neither answer is settled, so the inference keeps waiting";
  for (const auto& entry : before) {
    EXPECT_EQ(g.common.thoughts[entry.first].inferred, entry.second)
        << "order " << entry.first
        << " must not move while the question is open";
  }
}

TEST(Reactor0DeferredElim, ReceiverResolvesOnKnowledgeWithoutChuckingTheCard) {
  Game g = setup(inv_opts());
  arm_pending(g, TestPlayer::ALICE, TestPlayer::BOB);
  const int target = order_at(g, TestPlayer::ALICE, 3);
  const int slot1 = order_at(g, TestPlayer::ALICE, 1);
  const IdentitySet before = g.common.thoughts[slot1].inferred;
  // We work out that our called card was a plain b3 -- so the chuck was a real
  // discard. The card is still in our hand; we never had to throw it.
  pin_common(g, target, IdentitySet::single(Identity{1, 3}));

  g.resolve_deferred_elims();

  EXPECT_FALSE(g.pending_dc_elim.active);
  EXPECT_NE(g.common.thoughts[slot1].inferred, before)
      << "knowing is enough -- the card need not be chucked for the negative "
         "to become owed";
}

// The mirror: learning it WAS the playable orange voids the inference, again
// with the card still in hand.
TEST(Reactor0DeferredElim, ReceiverLearningItWasTheOrangeVoidsTheInference) {
  Game g = setup(inv_opts());
  arm_pending(g, TestPlayer::ALICE, TestPlayer::BOB);
  const int target = order_at(g, TestPlayer::ALICE, 3);
  pin_common(g, target, IdentitySet::single(Identity{kOrange, 1}));
  // Snapshot AFTER the pin, so the only thing this can catch is the resolver.
  auto before = hand_inferred(g, TestPlayer::ALICE);

  g.resolve_deferred_elims();

  EXPECT_FALSE(g.pending_dc_elim.active);
  for (const auto& entry : before) {
    EXPECT_EQ(g.common.thoughts[entry.first].inferred, entry.second)
        << "order " << entry.first
        << ": the chuck was a play, so nothing is owed";
  }
}
