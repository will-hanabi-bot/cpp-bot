// A reaction's negative inference waits for the RECEIVER to act.
//
// The negatives say "the slots the walk passed over were not playable". Which
// slots, and which set, depend on what the receiver does with their target --
// and that is not settled when the reacter acts. On an inverted suit the
// reacter's own action does not even distinguish a play from a discard: a CTD
// is a CHUCK, which puts the card on its stack.
//
// So the whole inference is captured at reaction time and fired when the
// receiver actions the target (`Game::PendingReactionElim`,
// `Game::fire_reaction_elim`). Three readings, decided by what the receiver put
// on the table:
//
//   receiver advanced a stack, NOT the reacter's -> passed-over slots only
//   receiver advanced the SAME stack (a FINESSE) -> whole hand, plus one-away
//                                                   on the passed-over slots
//   receiver advanced nothing (they discarded)   -> whole hand
//
// Before v8.0.0 this was keyed on the receiver's card IDENTITY instead: the
// hold was released as soon as anyone could prove the called card was or was
// not a playable inverted. That answered a narrower question (was the chuck a
// play?) and could not distinguish a finesse from an ordinary double play at
// all.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Red on 1, green on 1; everything else at 0. So the playables are
// {r2, y1, g2, b1, p1} and the one-aways are {r3, y2, g3, b2, p2}.
//
// Cathy acts first so a fixture can arm the capture and then have her action
// fire it, without threading a whole clue + reaction through the harness -- the
// arming is exercised end-to-end by the reactive suites.
SetupOptions elim_opts() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::CATHY;
  opts.play_stacks = {1, 0, 1, 0, 0};
  opts.clue_tokens = 5;  // a discard is illegal at 8
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us)
      {"r4", "y4", "b4", "p4", "y3"},  // Bob
      // Cathy: slot 3 is the reactive target. Slots 1-2 are the passed-over
      // ones; slots 4-5 are outside the walk.
      {"y5", "p5", "g2", "b5", "p3"},  // Cathy
  };
  use_reactor0(opts);
  return opts;
}

// The capture `arm_reaction_elim` makes at reaction time, with the reacter's
// advanced suit as the only knob.
//
// The per-slot sets are filled in UNFILTERED -- every slot gets the whole
// playable set and the whole one-away set, as though the reacter's paired slot
// could have supplied any reading. That is deliberate: these tests are about
// the FIRING half, which decides which slots get which of the three sets. The
// filtering half -- "could that alternative have existed at all" -- is what
// `test_reaction_elim_filter.cpp` covers, against `arm_reaction_elim` itself.
//
// `even` decides whether a finesse was on the table at all; `arm_reaction_elim`
// leaves `finesse_elim` empty under odd parity, so the fixture does too.
void arm(Game& g, int reacter_suit, bool even = true) {
  Game::PendingReactionElim p;
  p.active = true;
  p.receiver = static_cast<int>(TestPlayer::CATHY);
  p.target_slot = 3;
  p.receiver_hand = g.state.hands[static_cast<int>(TestPlayer::CATHY)];
  p.target_order = p.receiver_hand[2];
  p.reacter_suit = reacter_suit;

  const int n = static_cast<int>(g.state.variant->suits.size()) * 5;
  const IdentitySet playable = g.state.playable_set;
  const IdentitySet one_away = IdentitySet::create(
      [&g](Identity i) { return g.state.playable_away(i) == 1; }, n);
  const IdentitySet trash = IdentitySet::create(
      [&g](Identity i) { return g.state.is_basic_trash(i); }, n);

  const int slots = static_cast<int>(p.receiver_hand.size());
  p.direct_elim.assign(slots, playable);
  p.finesse_elim.assign(slots, even ? one_away : IdentitySet::empty());
  p.trash_elim.assign(slots, trash);
  g.pending_reaction_elim = std::move(p);
}

// Does this card still admit the identity? Keyed on ORDER, not slot: the
// receiver's action shifts every slot number behind it, so a slot lookup after
// the fact would read the freshly drawn card.
bool admits(const Game& g, int order, Identity id) {
  return g.common.thoughts[order].inferred.contains(id);
}

// Cathy's hand as orders, captured before she acts.
std::vector<int> cathy_orders(const Game& g) {
  return g.state.hands[static_cast<int>(TestPlayer::CATHY)];
}

const Identity kR2{0, 2};  // playable
const Identity kB1{3, 1};  // playable
const Identity kR3{0, 3};  // one away
const Identity kB2{3, 2};  // one away

}  // namespace

// --- the hold -------------------------------------------------------------

TEST(Reactor0ReactionElim, NothingIsAppliedUntilTheReceiverActs) {
  Game g = setup(elim_opts());
  const auto o = cathy_orders(g);
  arm(g, /*reacter_suit=*/3);

  EXPECT_TRUE(admits(g, o[0], kR2))
      << "the inference is captured, not applied -- what it says depends on "
         "what the receiver does, which has not happened yet";
  EXPECT_TRUE(g.pending_reaction_elim.active);
}

// --- the three readings ---------------------------------------------------

// The receiver played a card on a stack the reacter did not touch: an ordinary
// double play. Only the slots the walk passed over are spoken for.
TEST(Reactor0ReactionElim, DifferentStackClearsThePassedOverSlotsOnly) {
  Game g = setup(elim_opts());
  const auto o = cathy_orders(g);
  arm(g, /*reacter_suit=*/3);  // the reacter advanced BLUE

  // Cathy plays the g2 on her slot 3 -- green, not blue.
  g = take_turn(std::move(g), "Cathy plays g2 (slot 3)", "y1");

  EXPECT_FALSE(admits(g, o[0], kR2)) << "slot 1 was passed over";
  EXPECT_FALSE(admits(g, o[1], kR2)) << "slot 2 was passed over";
  EXPECT_TRUE(admits(g, o[3], kR2))
      << "slot 4 is to the RIGHT of the target -- the walk never reached it, "
         "so this reading owes it nothing";
  EXPECT_TRUE(admits(g, o[0], kR3))
      << "and only DIRECT playables are cleared; one-away is the finesse's "
         "extra negative, not this one";
}

// The receiver completed the very stack the reacter had just advanced: a
// FINESSE. Their card was one away, not directly playable -- so nothing in the
// hand was, and the passed-over slots were not one away either.
TEST(Reactor0ReactionElim, SameStackIsAFinesseAndClearsTheWholeHand) {
  Game g = setup(elim_opts());
  const auto o = cathy_orders(g);
  arm(g, /*reacter_suit=*/2);  // the reacter advanced GREEN, to 1

  // Cathy's g2 completes that same green stack.
  g = take_turn(std::move(g), "Cathy plays g2 (slot 3)", "y1");

  EXPECT_FALSE(admits(g, o[0], kR2));
  EXPECT_FALSE(admits(g, o[3], kR2))
      << "a finesse speaks for the WHOLE hand, not just the passed-over slots";
  EXPECT_FALSE(admits(g, o[0], kR3))
      << "and the passed-over slots were not one away either, or one of them "
         "would have been the target";
  EXPECT_FALSE(admits(g, o[1], kB2));
  EXPECT_TRUE(admits(g, o[3], kB2))
      << "the one-away negative is scoped to the passed-over slots";
}

// The receiver stacked nothing: they discarded. They would have been called to
// play a playable, so they had none anywhere.
//
// What it says about ONE-AWAY cards depends on the parity, because that is what
// decides whether a finesse was ever on the table. The even bucket is the
// double play, so a finesse was available and the receiver not being given one
// means nothing in the hand was one away either. The odd bucket is "exactly one
// play" -- no finesse to prefer -- so a one-away card is not spoken for.
TEST(Reactor0ReactionElim, NoStackAtEvenParityAlsoRulesOutOneAways) {
  Game g = setup(elim_opts());
  const auto o = cathy_orders(g);
  arm(g, /*reacter_suit=*/3, /*even=*/true);

  g = take_turn(std::move(g), "Cathy discards g2 (slot 3)", "y1");

  EXPECT_FALSE(admits(g, o[0], kR2));
  EXPECT_FALSE(admits(g, o[3], kB1))
      << "a discard speaks for the whole hand";
  EXPECT_FALSE(admits(g, o[0], kR3))
      << "a finesse was available in the even bucket and was not taken";
  EXPECT_FALSE(admits(g, o[3], kB2))
      << "and that reaches the whole hand too, not just the passed-over slots";
}

TEST(Reactor0ReactionElim, NoStackAtOddParitySaysNothingAboutOneAways) {
  Game g = setup(elim_opts());
  const auto o = cathy_orders(g);
  arm(g, /*reacter_suit=*/3, /*even=*/false);

  g = take_turn(std::move(g), "Cathy discards g2 (slot 3)", "y1");

  EXPECT_FALSE(admits(g, o[0], kR2))
      << "the direct-playable negative does not depend on parity";
  EXPECT_TRUE(admits(g, o[0], kR3))
      << "the odd bucket is exactly one play -- there was no finesse to prefer";
}

// --- scope ----------------------------------------------------------------

// A card carrying its own call is spoken for by that call. The inference must
// not touch it -- that is what let an earlier version strip a playable out of a
// standing reactive-CTP (replay 1966710).
TEST(Reactor0ReactionElim, ACardWithItsOwnCallIsLeftAlone) {
  Game g = setup(elim_opts());
  const auto o = cathy_orders(g);
  g.with_meta(o[0], [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
  arm(g, /*reacter_suit=*/3);

  g = take_turn(std::move(g), "Cathy discards g2 (slot 3)", "y1");

  EXPECT_TRUE(admits(g, o[0], kR2))
      << "slot 1 carries a play call; this inference is not evidence about it";
}

// Only the receiver's action on the TARGET fires it.
TEST(Reactor0ReactionElim, AnotherCardDoesNotFireIt) {
  Game g = setup(elim_opts());
  const auto o = cathy_orders(g);
  arm(g, /*reacter_suit=*/3);

  g = take_turn(std::move(g), "Cathy discards p3 (slot 5)", "y1");

  EXPECT_TRUE(admits(g, o[0], kR2))
      << "the hold is waiting on the reactive target, not on any discard";
  EXPECT_TRUE(g.pending_reaction_elim.active) << "and it is still waiting";
}
