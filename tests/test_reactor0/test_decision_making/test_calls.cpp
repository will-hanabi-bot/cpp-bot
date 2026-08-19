// Reactor0's per-player call tracking and the two action lists
// (src/conventions/reactor0/calls.cpp, DECISION_MAKING.md "Decision phase 2").
//
// The spec describes four structures per player with push/pop rules. They are
// DERIVED rather than stored, so what these tests pin is that the derivation
// agrees with the spec's semantics — in particular that `urgent` really does
// separate the reacter's call from the receiver's, since the whole design rests
// on it.
#include <gtest/gtest.h>

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
using hanabi::reactor0::ActionLists;
using hanabi::reactor0::PlayerCalls;

namespace {

SetupOptions base() {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r2", "r1", "p5", "g5", "g2"},
      {"y3", "b3", "p3", "y2", "b2"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return opts;
}

// Stamp a call by hand. The interpretation layer is tested elsewhere; here the
// subject is the derivation, so the input is set directly.
void call(Game& g, TestPlayer p, int slot, CardStatus st, bool urgent) {
  int o = order_at(g, p, slot);
  g.meta[o].status = st;
  g.meta[o].urgent = urgent;
}

}  // namespace

// --- calls_of -------------------------------------------------------------

// The load-bearing claim: `urgent` is what separates the reacter's call from
// the receiver's. If this ever stops being true the derived view is wrong
// everywhere, so it is pinned before anything else.
TEST(Reactor0Calls, UrgentSeparatesReacterFromReceiver) {
  Game g = setup(base());
  call(g, TestPlayer::BOB, 1, CardStatus::CALLED_TO_PLAY, /*urgent=*/true);
  call(g, TestPlayer::BOB, 3, CardStatus::CALLED_TO_PLAY, /*urgent=*/false);

  PlayerCalls c = hanabi::reactor0::calls_of(g, 1);
  EXPECT_EQ(c.reacter_ctp, order_at(g, TestPlayer::BOB, 1));
  EXPECT_EQ(c.reacter_ctd, -1);
  ASSERT_EQ(c.receiver_ctp.size(), 1u);
  EXPECT_EQ(c.receiver_ctp[0], order_at(g, TestPlayer::BOB, 3));
  EXPECT_TRUE(c.has_reaction());
}

// The receiver-CTP deque runs newest slot first, which rule 1 of
// `enforce_call_invariants` also makes play order.
TEST(Reactor0Calls, ReceiverCtpDequeRunsNewestFirst) {
  Game g = setup(base());
  call(g, TestPlayer::BOB, 4, CardStatus::CALLED_TO_PLAY, false);
  call(g, TestPlayer::BOB, 2, CardStatus::CALLED_TO_PLAY, false);

  PlayerCalls c = hanabi::reactor0::calls_of(g, 1);
  ASSERT_EQ(c.receiver_ctp.size(), 2u);
  EXPECT_EQ(c.receiver_ctp[0], order_at(g, TestPlayer::BOB, 2))
      << "slot 2 is newer than slot 4, so it is at the front";
  EXPECT_EQ(c.receiver_ctp[1], order_at(g, TestPlayer::BOB, 4));
  EXPECT_GT(c.receiver_ctp[0], c.receiver_ctp[1])
      << "newer means a larger order, which is what depends_on compares";
}

// A reaction pops its own slot for free: the reacted card leaves the hand, and
// the derivation only ever walks the hand. PLAN.md section 7 flagged the
// orphaned meta entry as a problem for a STORED structure; a derived one never
// sees it.
TEST(Reactor0Calls, ReactedCardLeavingTheHandEmptiesTheReacterSlot) {
  Game g = setup(base());
  call(g, TestPlayer::BOB, 1, CardStatus::CALLED_TO_PLAY, /*urgent=*/true);
  ASSERT_TRUE(hanabi::reactor0::calls_of(g, 1).has_reaction());

  const int reacted = order_at(g, TestPlayer::BOB, 1);
  auto& hand = g.state.hands[1];
  hand.erase(std::find(hand.begin(), hand.end(), reacted));

  PlayerCalls c = hanabi::reactor0::calls_of(g, 1);
  EXPECT_FALSE(c.has_reaction())
      << "the stamp survives in meta, but the card is gone from the hand";
  EXPECT_EQ(c.reacter_ctp, -1);
}

// --- dependence -----------------------------------------------------------

// Dependence is directional: only a card AHEAD in the deque (a larger order,
// i.e. a newer slot) can be depended upon.
TEST(Reactor0Calls, DependenceIsDirectional) {
  Game g = setup(base());
  const int newer = order_at(g, TestPlayer::BOB, 2);
  const int older = order_at(g, TestPlayer::BOB, 4);
  ASSERT_GT(newer, older) << "guard: slot 2 is the newer card";

  EXPECT_TRUE(hanabi::reactor0::depends_on(g, 1, older, newer))
      << "the older card may depend on the newer one ahead of it";
  EXPECT_FALSE(hanabi::reactor0::depends_on(g, 1, newer, older))
      << "never the other way round";
  EXPECT_FALSE(hanabi::reactor0::depends_on(g, 1, newer, newer))
      << "a card does not depend on itself";
}

// Two cards whose inferences cannot share a suit are independent, so they head
// separate chains and both reach the pitch list.
TEST(Reactor0Calls, DisjointSuitsAreIndependent) {
  Game g = setup(base());
  const int a = order_at(g, TestPlayer::BOB, 4);  // older
  const int b = order_at(g, TestPlayer::BOB, 2);  // newer

  // Pin each to a different suit from Bob's own view.
  g.players[1].thoughts[a].inferred = IdentitySet::single(Identity{0, 1});
  g.players[1].thoughts[b].inferred = IdentitySet::single(Identity{1, 1});
  EXPECT_FALSE(hanabi::reactor0::depends_on(g, 1, a, b))
      << "red cannot be yellow, so nothing is stranded by playing one first";

  // Same suit, and the dependence appears.
  g.players[1].thoughts[b].inferred = IdentitySet::single(Identity{0, 2});
  EXPECT_TRUE(hanabi::reactor0::depends_on(g, 1, a, b))
      << "both are red, so the newer card may be the prerequisite";
}

// --- the action lists -----------------------------------------------------

// The dependence machinery, on DECISION_MAKING.md's worked-example position.
//
// NOTE ON THE DOC'S EXAMPLE. That example prints the queue as `[r1, r2]` with
// r1 at the front, and a pitch list of `[g2, r1]`. Those are NOT reactor0's
// answers for this position, and the difference is deliberate: the example
// illustrates the dependence machinery generically, and it is drawn from a
// ruleset where new cards enter the queue at the BACK (reactor). **Reactor0
// inserts at the front**, so its deque runs newest slot first and the head of
// the red chain is r2, not r1. The doc says so inline.
//
// Only g1 is played. Bob holds `r2 r1 p5 g5 g2` with g2 fully clued, and both
// reds are called. g2 is independent, so the chains are [[r2, r1], [g2]] and
// the pitch list is the front of each.
TEST(Reactor0Calls, DependenceChainsPartitionThePitchList) {
  SetupOptions opts = base();
  opts.play_stacks = {0, 0, 1, 0, 0};  // g1 played, so g2 is playable
  Game g = setup(std::move(opts));

  const int r2 = order_at(g, TestPlayer::BOB, 1);  // newest of the two reds
  const int r1 = order_at(g, TestPlayer::BOB, 2);
  const int g2 = order_at(g, TestPlayer::BOB, 5);
  ASSERT_GT(r2, r1) << "guard: slot 1 really is the newer card";

  call(g, TestPlayer::BOB, 2, CardStatus::CALLED_TO_PLAY, false);
  call(g, TestPlayer::BOB, 1, CardStatus::CALLED_TO_PLAY, false);
  // g2 is fully clued (the doc says "previously touched with both 2 and
  // green"), so Bob's own empathy already knows it plays.
  g.state.deck[g2].clued = true;
  g.players[1].thoughts[g2].inferred = IdentitySet::single(Identity{2, 2});
  // Both reds to Bob, which is what makes them one chain.
  g.players[1].thoughts[r1].inferred = IdentitySet::single(Identity{0, 1});
  g.players[1].thoughts[r2].inferred = IdentitySet::single(Identity{0, 2});

  ActionLists lists = hanabi::reactor0::action_lists(g, 1);

  ASSERT_EQ(lists.pitch_chains.size(), 2u)
      << "two chains: the red pair, and the independent g2";
  auto on_pitch_list = [&lists](int o) {
    return std::find(lists.pitch.begin(), lists.pitch.end(), o) !=
           lists.pitch.end();
  };
  EXPECT_TRUE(on_pitch_list(g2)) << "g2 heads its own chain";
  EXPECT_TRUE(on_pitch_list(r2))
      << "reactor0 inserts at the front, so the NEWER red heads the red chain";
  EXPECT_FALSE(on_pitch_list(r1))
      << "r1 sits behind r2 in its chain, so it is not on the pitch list";

  for (const auto& chain : lists.pitch_chains) {
    if (chain.front() != r2) continue;
    ASSERT_EQ(chain.size(), 2u);
    EXPECT_EQ(chain[1], r1) << "r1 depends on r2 and follows it";
  }
}

// The chuck list is CTD-stamped cards plus everything chuckable: trash on a
// plain suit, or playable on an inverted one.
TEST(Reactor0Calls, ChuckListTakesCtdAndTrash) {
  SetupOptions opts = base();
  opts.hands[1] = {"r1", "y3", "p5", "g5", "g2"};
  opts.play_stacks = {1, 0, 0, 0, 0};  // r1 is now basic trash
  Game g = setup(std::move(opts));
  call(g, TestPlayer::BOB, 3, CardStatus::CALLED_TO_DISCARD, false);

  ActionLists lists = hanabi::reactor0::action_lists(g, 1);
  EXPECT_NE(std::find(lists.chuck.begin(), lists.chuck.end(),
                      order_at(g, TestPlayer::BOB, 1)),
            lists.chuck.end())
      << "r1 is basic trash, so pressing Discard on it costs nothing";
  EXPECT_NE(std::find(lists.chuck.begin(), lists.chuck.end(),
                      order_at(g, TestPlayer::BOB, 3)),
            lists.chuck.end())
      << "an explicit CTD is on the chuck list whatever the card is";
  EXPECT_EQ(std::find(lists.chuck.begin(), lists.chuck.end(),
                      order_at(g, TestPlayer::BOB, 4)),
            lists.chuck.end())
      << "g5 is neither called nor chuckable";
}
