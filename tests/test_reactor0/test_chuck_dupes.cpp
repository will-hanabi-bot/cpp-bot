// Known same-hand duplicates belong on the chuck list.
//
// A player who can pin TWO of their own cards to the same identity holds one
// card too many: discarding either loses nothing, because the other copy still
// carries the identity. `is_chuckable` (reactor0/calls.cpp:82) does not see
// this — it asks whether the card is basic trash, and a duplicated b4 with the
// blue stack at 1 is not trash, it is needed. So a hand full of known dupes
// produced an EMPTY chuck list, phase 2 fell through to the rung-12 floor, and
// the bot discarded its unknown chop instead.
//
// Replay 1966687 T14 is the motivating case and it cost a strike: will-bot67
// held b4 in slots 3 and 5, threw its slot-1 chop instead, and the chop was an
// Orange 4. Orange is inverted, so pressing Discard on it is a play attempt,
// and o4 on a stack of 1 bombs.
//
// The tiebreak is "leftmost copy", and it is load-bearing rather than
// cosmetic: put BOTH copies on the list and the team can throw the identity
// away entirely.
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <variant>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor0/calls.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Pin one card to a single identity in every view, the way a fully-resolving
// clue sequence would. `Game::with_thought` writes only `common`
// (basics/game.cpp:62), and `action_lists` reads the HOLDER's own view, so the
// per-player thought has to be set too.
void pin(Game& g, int order, Identity id) {
  const IdentitySet one = IdentitySet::single(id);
  g.with_thought(order, [one](const Thought& t) {
    Thought out = t;
    out.inferred = one;
    out.possible = one;
    return out;
  });
  for (Player& p : g.players) {
    p.thoughts[order].inferred = one;
    p.thoughts[order].possible = one;
  }
}

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

// Alice holds two b4 (slots 3 and 5) with blue on 1, so neither is trash and
// neither is playable. Slot 1 is her unclued chop.
SetupOptions dupe_opts() {
  SetupOptions opts;
  opts.variant_name = "White-Fives & Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {4, 1, 1};
  opts.hands = {
      {"o4", "r4", "b4", "o4", "b4"},
      {"r3", "b3", "r2", "b2", "r1"},
      {"o2", "o3", "r5", "b5", "o5"},
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

TEST(Reactor0ChuckDupes, LeftmostKnownDupeJoinsTheChuckList) {
  Game g = setup(dupe_opts());
  const int alice = static_cast<int>(TestPlayer::ALICE);
  const int left = order_at(g, TestPlayer::ALICE, 3);
  const int right = order_at(g, TestPlayer::ALICE, 5);
  pin(g, left, Identity{1, 4});
  pin(g, right, Identity{1, 4});

  auto lists = hanabi::reactor0::action_lists(g, alice);

  EXPECT_TRUE(contains(lists.chuck, left))
      << "the leftmost copy of a known same-hand dupe is expendable and must "
         "be chuckable";
  EXPECT_FALSE(contains(lists.chuck, right))
      << "only ONE copy may be chuckable, or the team can throw the identity "
         "away entirely";
}

TEST(Reactor0ChuckDupes, DupeIsPreferredOverDiscardingTheUnknownChop) {
  Game g = setup(dupe_opts());
  const int left = order_at(g, TestPlayer::ALICE, 3);
  const int right = order_at(g, TestPlayer::ALICE, 5);
  const int chop = order_at(g, TestPlayer::ALICE, 1);
  pin(g, left, Identity{1, 4});
  pin(g, right, Identity{1, 4});
  // Zero tokens so the clue phase cannot fire and this pins phase 2 alone.
  // The question here is only which card gets thrown once a discard is the
  // move, not whether cluing would have been better.
  g.state.clue_tokens = 0;

  PerformAction action = g.take_action();

  ASSERT_TRUE(std::holds_alternative<PerformDiscard>(action))
      << "with a redundant dupe in hand there is a safe discard available";
  EXPECT_EQ(std::get<PerformDiscard>(action).target, left)
      << "must throw the expendable duplicate, not the unknown chop (which in "
         "replay 1966687 T14 was an Orange 4 and struck)";
  EXPECT_NE(std::get<PerformDiscard>(action).target, chop);
}

// The negative that keeps the arm honest. On an INVERTED suit pressing Discard
// is a play attempt, so chucking a duplicated orange strikes unless it happens
// to be playable — and when it is playable `is_chuckable`'s second arm has
// already claimed it. The dupe arm must not add one.
TEST(Reactor0ChuckDupes, InvertedDupeIsNotChuckable) {
  Game g = setup(dupe_opts());
  const int alice = static_cast<int>(TestPlayer::ALICE);
  const int left = order_at(g, TestPlayer::ALICE, 1);
  const int right = order_at(g, TestPlayer::ALICE, 4);
  // Orange is suit 2 and its stack is on 1, so o4 is not playable: chucking it
  // would bomb even though a second copy makes the card itself expendable.
  pin(g, left, Identity{2, 4});
  pin(g, right, Identity{2, 4});

  auto lists = hanabi::reactor0::action_lists(g, alice);

  EXPECT_FALSE(contains(lists.chuck, left))
      << "a duplicated unplayable orange must not be chucked — Discard on an "
         "inverted suit is a play attempt and this one strikes";
  EXPECT_FALSE(contains(lists.chuck, right));
}
