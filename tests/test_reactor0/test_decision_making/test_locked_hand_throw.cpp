// A LOCKED hand below 8 tokens throws the card least likely to be critical
// (reactor0 DECISION_MAKING.md rung 12, src/conventions/reactor0/calls.cpp, v17.6.0).
//
// A hand with no chop and nothing on either list used to pitch slot 1 blind. A
// blind pitch that misses throws the card away as surely as a discard does, and
// strikes on top: over 500 self-play games it missed 140 times in 181. Now the
// card thrown is the one whose reading is least likely to be critical, and among
// those the one most likely to be trash; at 8 tokens a discard is illegal and the
// leftmost is still pitched.
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/reactor0/calls.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Every card of Alice's is clued, so she has no chop. Red and yellow are on 1, so
// an r1 is trash; nothing else is on the stacks.
Game locked(int tokens) {
  SetupOptions opts;
  opts.hands = {
      {"r5", "r1", "p4", "y4", "g4"},
      {"r2", "y3", "b3", "p3", "g3"},
      {"y2", "b2", "p2", "g2", "r4"},
  };
  opts.play_stacks = {1, 1, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = tokens;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  for (int slot = 1; slot <= 5; ++slot) {
    g.state.deck[order_at(g, TestPlayer::ALICE, slot)].clued = true;
  }
  auto read = [&](int slot, IdentitySet set) {
    g.players[0].thoughts[order_at(g, TestPlayer::ALICE, slot)].inferred = set;
  };
  // No reading is all trash (the chuck list would take it) or all playable (the
  // pitch list would), so the floor is what decides.
  const Identity r5{0, 5}, y5{1, 5}, r1{0, 1}, b1{3, 1}, p4{4, 4}, y4{1, 4}, g2{2, 2},
      g4{2, 4};
  read(1, IdentitySet::single(r5).add(y5));  // certainly critical
  read(2, IdentitySet::single(r1).add(b1));  // never critical, and maybe trash
  read(3, IdentitySet::single(p4).add(y5));  // might be critical
  read(4, IdentitySet::single(y4).add(g2));  // never critical, never trash
  read(5, IdentitySet::single(g4));          // never critical, never trash
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

TEST(Reactor0LockedHandThrow, TheCardLeastLikelyCriticalAndMostLikelyTrashGoes) {
  Game g = locked(/*tokens=*/0);
  ASSERT_FALSE(g.chop(0).has_value()) << "guard: every card is clued";
  const auto action = hanabi::reactor0::choose_action(g);
  EXPECT_EQ(played(action), -1) << "no blind pitch below 8 tokens";
  EXPECT_EQ(discarded(action), order_at(g, TestPlayer::ALICE, 2))
      << "the {r1,b1}, never critical and maybe trash -- not the leftmost {r5,y5}";
}

TEST(Reactor0LockedHandThrow, AtEightTokensTheLeftmostIsStillPitched) {
  Game g = locked(/*tokens=*/8);
  ASSERT_FALSE(g.chop(0).has_value());
  EXPECT_EQ(played(hanabi::reactor0::choose_action(g)), order_at(g, TestPlayer::ALICE, 1))
      << "a discard is illegal at 8 tokens";
}
