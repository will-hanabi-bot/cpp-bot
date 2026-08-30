// A reaction the reacter DEFERRED is still owed (CONVENTION.md §1e, v12.0.0).
//
// `waiting` is cleared the moment the reacter clues instead of reacting, which
// is right for the reacter -- a deferring clue is itself reactive and carries
// the intent forward -- and was silently fatal for the RECEIVER, who simply
// forgot what they had been told. Precedence step 1 lets a VERY HIGH clue
// out-rank a reaction, so deferring is legal and the signal was being thrown
// away on purpose. Replay 1975464 is the case.
//
// `Game::pending_reactions` is the durable copy, one per receiver, resolved by
// the reacter's next NON-CLUE action. The rules, in the order they are tested:
//
//   1. the reacter's next non-clue action resolves it;
//   2. a new reactive clue TO THE SAME RECEIVER replaces it, and one aimed
//      elsewhere does not;
//   3. a reacter who SUCCESSFULLY ADVANCES A STACK with a card that was dead at
//      clue time drops it -- the giver cannot have meant that;
//   4. otherwise the target is read out of the CLUE-TIME hand; if it has left
//      the receiver's hand, drop.
//
// Seats: Alice 0 is US and the RECEIVER, Bob 1 the giver, Cathy 2 the reacter.
// That is forced -- the harness pins `our_player_index` at 0, and with three
// seats a clue from Bob to Alice is the only shape that makes us the receiver.
// Our hand therefore carries real ids, which a real game would not show us; the
// giver's target selection needs them, and the receiver decodes positionally out
// of `wc.receiver_hand` rather than from ids, so nothing under test reads them.
//
// THE PAIRING IS COMPUTED, NOT DISCOVERED. `wc.react_order` is -1 from the
// receiver's own seat by design (§1d: the receiver never runs target selection),
// so these tests choose the slot Cathy acts on and derive the target with
// `calc_slot` -- which is the arithmetic under test anyway.
//
//   rank 1 to Alice -> anchor 1, and Cathy acting on her slot 1 gives
//   `calc_slot(1, 1, 5) = 5`: our target is the card in our CLUE-TIME slot 5.
#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <string>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

SetupOptions deferral_opts() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::BOB;  // the giver moves first
  opts.hands = {
      // rank 1 picks out slots 1, 3, 5; rank 2 picks out 2 and 4.
      {"r1", "y2", "g1", "b2", "p1"},  // Alice 0 -- us, the RECEIVER
      {"r4", "y4", "g4", "b4", "p4"},  // Bob 1   -- the giver, nothing playable
      {"y1", "b1", "g2", "b3", "p3"},  // Cathy 2 -- the reacter
  };
  use_reactor0(opts);
  return opts;
}

const ReactorWC* pending_for(const Game& g, int receiver) {
  if (receiver >= static_cast<int>(g.pending_reactions.size())) return nullptr;
  const auto& p = g.pending_reactions[receiver];
  return p ? &*p : nullptr;
}

// The card our clue-time slot `s` held.
int clue_time_order(const ReactorWC& wc, int s) { return wc.receiver_hand[s - 1]; }

bool alice_has_a_call(const Game& g) {
  for (int o : g.state.hands[0]) {
    if (g.meta[o].status != CardStatus::NONE) return true;
  }
  return false;
}

}  // namespace

// --- the record itself ----------------------------------------------------

TEST(Reactor0DeferredReaction, AReactiveClueRecordsWhatTheReceiverIsOwed) {
  Game g = setup(deferral_opts());
  g = take_turn(std::move(g), "Bob clues 1 to Alice (slot 1,3,5)");

  const ReactorWC* p = pending_for(g, 0);
  ASSERT_NE(p, nullptr) << "the receiver is owed a reaction";
  EXPECT_EQ(p->reacter, 2) << "Cathy reacts";
  EXPECT_EQ(p->receiver, 0) << "we receive";
  EXPECT_EQ(p->receiver_hand, g.state.hands[0])
      << "our hand as it was when the clue was given -- the frame the target is "
         "read out of later";
  EXPECT_EQ(p->clue_play_stacks, g.state.play_stacks)
      << "and the stacks, which is what a deferred reading is built against";
  EXPECT_EQ(hanabi::reactor::calc_slot(p->focus_slot, 1, 5), 5)
      << "guard on the arithmetic every test below relies on: Cathy acting on "
         "her slot 1 names our slot 5";
}

// Rule 1. The reacter clues instead of reacting, which kills `waiting`; the
// durable record must survive it and resolve when they finally act.
TEST(Reactor0DeferredReaction, SurvivesADeferralAndResolvesOnTheNextAction) {
  Game g = setup(deferral_opts());
  g = take_turn(std::move(g), "Bob clues 1 to Alice (slot 1,3,5)");
  const ReactorWC wc = *pending_for(g, 0);
  const int target = clue_time_order(wc, 5);

  // Cathy DEFERS -- the licence Precedence step 1 grants a VERY HIGH clue.
  g = take_turn(std::move(g), "Cathy clues 4 to Bob");
  EXPECT_TRUE(g.waiting.empty() || g.waiting.front().receiver != 0)
      << "guard: our live connection is gone, which is the whole bug";
  ASSERT_NE(pending_for(g, 0), nullptr) << "but the durable record is not";

  // We clue rather than act, so our own hand does not move and the slot
  // arithmetic under test is the only thing in play.
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");
  g = take_turn(std::move(g), "Bob discards p4 (slot 5)", "p3");

  g = take_turn(std::move(g), "Cathy plays y1 (slot 1)", "r3");

  EXPECT_EQ(pending_for(g, 0), nullptr) << "the record is consumed";
  EXPECT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY)
      << "and we finally learn which card was named -- three turns after the "
         "clue that named it. Cathy's slot 1 pairs with our clue-time slot 5.";
}

// Rule 2. One per receiver: a second reactive clue to us replaces the first.
TEST(Reactor0DeferredReaction, ASecondReactiveClueToUsReplacesTheFirst) {
  Game g = setup(deferral_opts());
  g = take_turn(std::move(g), "Bob clues 1 to Alice (slot 1,3,5)");
  const ReactorWC first = *pending_for(g, 0);

  g = take_turn(std::move(g), "Cathy clues 4 to Bob");   // defer
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");
  g = take_turn(std::move(g), "Bob clues 2 to Alice (slot 2,4)");  // to US again

  const ReactorWC* p = pending_for(g, 0);
  ASSERT_NE(p, nullptr);
  EXPECT_NE(p->turn, first.turn)
      << "the newer clue owns the record -- nobody tracks two at once";
  EXPECT_NE(p->focus_slot, first.focus_slot)
      << "and it is the new clue's anchor that will be decoded, not the old one";
}

// ...but a reactive clue to a DIFFERENT receiver must leave ours standing. This
// is the case replay 1975464 turns on: the deferring clue was itself reactive,
// so it owed somebody else a reaction while we were still owed ours. One global
// slot would have clobbered one of them.
TEST(Reactor0DeferredReaction, AClueToAnotherReceiverLeavesOursStanding) {
  Game g = setup(deferral_opts());
  g = take_turn(std::move(g), "Bob clues 1 to Alice (slot 1,3,5)");
  const ReactorWC ours = *pending_for(g, 0);

  // Cathy's Bob is Alice, so her clue to Bob is reactive and receives at seat 1.
  g = take_turn(std::move(g), "Cathy clues 4 to Bob");

  const ReactorWC* still = pending_for(g, 0);
  ASSERT_NE(still, nullptr) << "ours survives a clue aimed elsewhere";
  EXPECT_EQ(still->turn, ours.turn) << "and it is the SAME record, not a new one";
  const ReactorWC* other = pending_for(g, 1);
  ASSERT_NE(other, nullptr) << "while seat 1 is now owed one of its own";
  EXPECT_EQ(other->reacter, 0) << "with us reacting for it";
}

// Rule 3. A reacter who successfully advances a stack with a card that was DEAD
// when the clue was given was not answering this reaction -- it only became
// playable because somebody else moved the stack in between, so the play is
// opportunism and the pairing is dropped.
TEST(Reactor0DeferredReaction, DroppedWhenTheReacterPlaysWhatWasDeadAtClueTime) {
  SetupOptions opts = deferral_opts();
  // Cathy's SLOT 1 is the dead card, so the pairing points at our clue-time
  // slot 5 -- which survives. Without rule 3 this reaction would resolve and
  // stamp that card, which is what makes the test discriminate rather than
  // agreeing with rule 4 by accident.
  opts.hands[1] = {"y1", "y4", "g4", "b4", "p4"};  // Bob can advance yellow
  opts.hands[2] = {"y2", "b1", "g2", "b3", "p3"};  // Cathy's y2, dead for now
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Bob clues 1 to Alice (slot 1,3,5)");
  const ReactorWC wc = *pending_for(g, 0);
  const int target = clue_time_order(wc, 5);
  ASSERT_FALSE(g.state.is_playable(Identity{1, 2}))
      << "guard: Cathy's y2 is two away when the clue is given -- dead";

  g = take_turn(std::move(g), "Cathy clues 4 to Bob");    // defer
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");  // keep our hand still
  g = take_turn(std::move(g), "Bob plays y1 (slot 1)", "p3");
  ASSERT_TRUE(g.state.is_playable(Identity{1, 2}))
      << "guard: the y2 only came alive while Cathy was deferring";
  ASSERT_NE(std::count(g.state.hands[0].begin(), g.state.hands[0].end(), target), 0)
      << "guard: our target is still in hand, so only rule 3 can drop this";

  g = take_turn(std::move(g), "Cathy plays y2 (slot 1)", "r3");

  EXPECT_EQ(pending_for(g, 0), nullptr) << "consumed either way";
  EXPECT_EQ(g.meta[target].status, CardStatus::NONE)
      << "but DROPPED, not resolved: Bob cannot have built a reaction around a "
         "card he could see was two away when he clued. Without rule 3 this "
         "card would have been stamped CALLED_TO_PLAY.";
}

// Rule 4's other drop: the named card has already left our hand, so there is
// nothing to stamp and the reaction lapses rather than landing on some other
// card.
TEST(Reactor0DeferredReaction, DroppedWhenTheTargetHasLeftOurHand) {
  Game g = setup(deferral_opts());
  g = take_turn(std::move(g), "Bob clues 1 to Alice (slot 1,3,5)");
  const ReactorWC wc = *pending_for(g, 0);
  const int target = clue_time_order(wc, 5);

  g = take_turn(std::move(g), "Cathy clues 4 to Bob");  // defer
  // Throw away the very card the pairing points at.
  g = take_turn(std::move(g), "Alice plays p1 (slot 5)", "p5");
  ASSERT_EQ(std::count(g.state.hands[0].begin(), g.state.hands[0].end(), target), 0)
      << "guard: the target is gone";
  g = take_turn(std::move(g), "Bob discards p4 (slot 5)", "p3");

  g = take_turn(std::move(g), "Cathy plays y1 (slot 1)", "r3");

  EXPECT_EQ(pending_for(g, 0), nullptr) << "consumed";
  EXPECT_FALSE(alice_has_a_call(g))
      << "nothing left to point at, so it lapses -- it must NOT slide onto "
         "whatever card now occupies slot 5";
}

// The undeferred case must be untouched: the reacter answers immediately, the
// live `waiting` path resolves it as it always did, and the durable record is
// retired rather than firing again on some later turn.
TEST(Reactor0DeferredReaction, AnImmediateReactionRetiresTheRecord) {
  Game g = setup(deferral_opts());
  g = take_turn(std::move(g), "Bob clues 1 to Alice (slot 1,3,5)");
  const ReactorWC wc = *pending_for(g, 0);
  const int target = clue_time_order(wc, 5);

  g = take_turn(std::move(g), "Cathy plays y1 (slot 1)", "r3");

  EXPECT_EQ(pending_for(g, 0), nullptr)
      << "retired by the live path, so it cannot resolve a second time later";
  EXPECT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY)
      << "and the reaction still resolved, exactly as before v12.0.0";
}

// Rule 4's frame, which is the part a deferral makes visible at all. The
// reading is built against the stacks AS THEY WERE WHEN THE CLUE WAS GIVEN,
// not as they are when it finally resolves -- the giver chose the target in the
// old frame, so that is the promise.
//
// The discriminator is the g1. Clue time has every stack on 0; we then play our
// own g1 while Cathy is deferring, so by the time she reacts green is on 1 and
// the g1 is trash. Read live, it drops out of the target's inference; read in
// the clue-time frame -- which is what the convention says -- it stays.
TEST(Reactor0DeferredReaction, TheReadingIsBuiltInTheClueTimeFrame) {
  Game g = setup(deferral_opts());
  g = take_turn(std::move(g), "Bob clues 1 to Alice (slot 1,3,5)");
  const ReactorWC wc = *pending_for(g, 0);
  const int target = clue_time_order(wc, 5);
  ASSERT_EQ(wc.clue_play_stacks, std::vector<int>({0, 0, 0, 0, 0}))
      << "guard: nothing is played yet, so the g1 is playable in this frame";

  g = take_turn(std::move(g), "Cathy clues 4 to Bob");     // defer
  g = take_turn(std::move(g), "Alice plays g1 (slot 3)", "p5");
  g = take_turn(std::move(g), "Bob discards p4 (slot 5)", "p3");
  ASSERT_EQ(g.state.play_stacks[2], 1)
      << "guard: green moved while Cathy was deferring, so the g1 is now trash";

  // y1 WAS playable at clue time, so rule 3 does not drop this one.
  g = take_turn(std::move(g), "Cathy plays y1 (slot 1)", "r3");

  ASSERT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY)
      << "guard: the reaction resolved";
  EXPECT_TRUE(g.common.thoughts[target].inferred.contains(Identity{2, 1}))
      << "the g1 must survive in the reading: it was playable when the clue was "
         "given, which is the frame the giver chose the target in. Reading the "
         "LIVE stacks would have struck it out, since green is on 1 by now.";
}

// --- Rule 5: the frame meets the present -----------------------------------
//
// Rule 5's DISCRIMINATING coverage is the replay test
// `test_misc/test_replay_1974194_stale_reading_is_not_acted_on.cpp`, not a
// fixture here. A synthetic drop is hard to stage honestly: `receiver_ctp_set`
// already judges the reading against "the stacks the reacter LEAVES BEHIND", so
// the reacter's own advance cannot make a reading stale, and a third party's
// play has to thread past rules 3 and 6 to reach rule 5 at all. Two attempts at
// such a fixture passed with rule 5 removed -- i.e. proved nothing -- so they
// were dropped in favour of the real game. What remains here is the CONTROL,
// which is what stops rule 5 from silently disabling the feature.
//
// Rule 4 reads the promise in the giver's frame. Rule 5 stops that frame from
// outliving its usefulness: while the reacter deferred, somebody may have played
// the very identity the target was called as, and the clue-time frame cannot see
// it. Stamping anyway hands the receiver a bomb.
//
// Every stack but purple starts on 1, so at clue time the ONLY playable one is
// the p1 -- which makes the target's whole reading a single identity and lets a
// single play kill it outright.
SetupOptions rule5_opts() {
  SetupOptions opts = deferral_opts();
  opts.play_stacks = {1, 1, 1, 1, 0};
  return opts;
}

// The control that stops rule 5 from silently disabling the feature: same
// fixture, same deferral, but the reacter answers with a card that leaves our
// target alone. It must still resolve.
TEST(Reactor0DeferredReaction, KeptWhenTheTargetIsStillPlayableAtResolution) {
  SetupOptions opts = rule5_opts();
  opts.hands[2] = {"r2", "b1", "g2", "b3", "p3"};  // advances RED, not purple
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Bob clues 1 to Alice (slot 1,3,5)");
  const ReactorWC wc = *pending_for(g, 0);
  const int target = clue_time_order(wc, 5);

  g = take_turn(std::move(g), "Cathy clues 4 to Bob");    // defer
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");
  g = take_turn(std::move(g), "Bob discards p4 (slot 5)", "y3");

  g = take_turn(std::move(g), "Cathy plays r2 (slot 1)", "r3");
  ASSERT_TRUE(g.state.is_playable(Identity{4, 1}))
      << "guard: purple never moved, so our target is still live";

  EXPECT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY)
      << "rule 5 must only fire on a reading nothing can satisfy -- here the "
         "reaction is as good as it ever was and must resolve";
}

// --- Rule 6: a superseding call claims the action ---------------------------
//
// We are owed a reaction from Cathy. We then clue Cathy ourselves, and OUR clue
// calls one of her cards. When she acts on that card we cannot tell whether we
// are being answered or merely watching our own clue obeyed -- so we drop what
// we were owed rather than decode a slot from it.
//
// This is the shape replay 1969792 hit: the reacter was discharging a different
// reaction, one in which THEY were the receiver.
TEST(Reactor0DeferredReaction, DroppedWhenALaterCallExplainsTheAction) {
  Game g = setup(deferral_opts());
  g = take_turn(std::move(g), "Bob clues 1 to Alice (slot 1,3,5)");
  const ReactorWC ours = *pending_for(g, 0);
  const int target = clue_time_order(ours, 5);

  g = take_turn(std::move(g), "Cathy clues 4 to Bob");  // defer

  // OUR clue, aimed at the very seat that owes us. Alice gives it, so the
  // reacter is Bob and the receiver is Cathy -- the only shape at three seats
  // that can make our reacter somebody's receiver.
  g = take_turn(std::move(g), "Alice clues 2 to Cathy (slot 3)");
  const ReactorWC* theirs = pending_for(g, 2);
  ASSERT_NE(theirs, nullptr) << "guard: Cathy is now owed a reaction of ours";

  // Bob answers it, which stamps a card in Cathy's hand -- a call laid down
  // LATER than the clue we are still owed.
  g = take_turn(std::move(g), "Bob plays r4 (slot 1)", "y3");
  int called = -1;
  for (int o : g.state.hands[2]) {
    if (g.meta[o].status == CardStatus::CALLED_TO_PLAY &&
        g.meta[o].signal_turn && *g.meta[o].signal_turn > ours.turn) {
      called = o;
    }
  }
  ASSERT_NE(called, -1)
      << "guard: our clue must actually have called one of Cathy's cards, or "
         "there is nothing for rule 6 to notice";

  const int slot = static_cast<int>(
      std::find(g.state.hands[2].begin(), g.state.hands[2].end(), called) -
      g.state.hands[2].begin()) + 1;
  g = take_turn(std::move(g),
                "Cathy plays " + g.state.log_id_by_order(called) +
                    " (slot " + std::to_string(slot) + ")",
                "r3");

  EXPECT_EQ(pending_for(g, 0), nullptr) << "consumed either way";
  EXPECT_EQ(g.meta[target].status, CardStatus::NONE)
      << "DROPPED: her play is explained by OUR later clue, so reading a slot "
         "out of it would be decoding an action that was never aimed at us.";
}
