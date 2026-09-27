// Floor rung 12 on a hand with no chop (src/conventions/reactor0/calls.cpp,
// DECISION_MAKING.md "Decision phase 2", v16.23.0).
//
// `Game::chop` skips any card drawn after the team ran out of clues -- the
// zero-clue safety promise protects the card that WAS chop. A hand whose other
// cards are all clued therefore has no chop at all, and the floor used to pitch
// slot 1 blind. The ruling: a card drawn during the stall is discarded instead,
// the FIRST one drawn if there are several. At 8 tokens a discard is illegal and
// the pitch stands; with no stall-drawn card the hand really is locked and the
// pitch stands too.
//
// Replay 2011327 T32 is the case: will-bot67 pitched order 30, drawn at T29 one
// turn after the team hit zero clues, into a strike.
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/calls.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

constexpr int kZcsTurn = 5;

// Alice's slots 3-5 are clued (so none of them is chop); slots 1 and 2 are
// unclued and were drawn at the given turns.
Game position(int tokens, int slot1_drawn, int slot2_drawn) {
  SetupOptions opts;
  opts.hands = {
      {"r1", "b4", "p4", "y4", "g4"},
      {"r2", "y3", "b3", "p3", "g3"},
      {"y2", "b2", "p2", "g2", "r4"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = tokens;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  for (int slot : {3, 4, 5}) g.state.deck[order_at(g, TestPlayer::ALICE, slot)].clued = true;
  g.state.deck[order_at(g, TestPlayer::ALICE, 1)].turn_drawn = slot1_drawn;
  g.state.deck[order_at(g, TestPlayer::ALICE, 2)].turn_drawn = slot2_drawn;
  g.zcs_turn = kZcsTurn;
  return g;
}

int played(const std::optional<PerformAction>& a) {
  auto* p = a ? std::get_if<PerformPlay>(&*a) : nullptr;
  return p ? p->target : -1;
}

int discarded(const std::optional<PerformAction>& a) {
  auto* d = a ? std::get_if<PerformDiscard>(&*a) : nullptr;
  return d ? d->target : -1;
}

}  // namespace

TEST(Reactor0StallDrawnCard, TheFirstCardDrawnInTheStallIsDiscarded) {
  // Slot 2 was drawn at turn 6, slot 1 at turn 7: both after the lock.
  Game g = position(/*tokens=*/1, /*slot1_drawn=*/7, /*slot2_drawn=*/6);
  ASSERT_FALSE(g.chop(0).has_value()) << "guard: both unclued cards postdate the lock";

  auto action = hanabi::reactor0::choose_action(g);
  EXPECT_EQ(discarded(action), order_at(g, TestPlayer::ALICE, 2))
      << "the first card drawn during the stall, not a blind pitch of slot 1";
  EXPECT_EQ(played(action), -1);
}

TEST(Reactor0StallDrawnCard, AtEightCluesItIsStillPitched) {
  Game g = position(/*tokens=*/8, 7, 6);
  ASSERT_FALSE(g.chop(0).has_value());
  EXPECT_EQ(played(hanabi::reactor0::choose_action(g)),
            order_at(g, TestPlayer::ALICE, 1))
      << "a discard is illegal at 8 tokens";
}

TEST(Reactor0StallDrawnCard, AHandThatIsReallyLockedStillPitchesSlotOne) {
  Game g = position(/*tokens=*/1, 7, 6);
  for (int slot : {1, 2}) g.state.deck[order_at(g, TestPlayer::ALICE, slot)].clued = true;
  ASSERT_FALSE(g.chop(0).has_value());
  EXPECT_EQ(played(hanabi::reactor0::choose_action(g)),
            order_at(g, TestPlayer::ALICE, 1))
      << "every card clued: nothing was drawn in the stall to throw";
}

TEST(Reactor0StallDrawnCard, ACardDrawnBeforeTheLockIsStillTheChop) {
  // Slot 2 predates the lock, so it is the chop and the ordinary floor applies.
  Game g = position(/*tokens=*/1, /*slot1_drawn=*/7, /*slot2_drawn=*/3);
  auto chop = g.chop(0);
  ASSERT_TRUE(chop.has_value());
  EXPECT_EQ(*chop, order_at(g, TestPlayer::ALICE, 2));
  EXPECT_EQ(discarded(hanabi::reactor0::choose_action(g)), *chop);
}
