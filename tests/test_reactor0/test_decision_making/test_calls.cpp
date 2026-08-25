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

// The chuck list is CTD-stamped cards plus everything chuckable, and
// "chuckable" is judged from the HOLDER's view, never from the deck. A player
// cannot see their own hand, so a card that happens to be trash is not
// chuckable until they can tell.
TEST(Reactor0Calls, ChuckListTakesCtdAndKnownTrash) {
  SetupOptions opts = base();
  opts.hands[1] = {"r1", "y3", "p5", "g5", "g2"};
  opts.play_stacks = {1, 0, 0, 0, 0};  // r1 is now basic trash
  Game g = setup(std::move(opts));

  const int r1 = order_at(g, TestPlayer::BOB, 1);
  const int ctd = order_at(g, TestPlayer::BOB, 3);
  const int g5 = order_at(g, TestPlayer::BOB, 4);
  call(g, TestPlayer::BOB, 3, CardStatus::CALLED_TO_DISCARD, false);

  auto on_chuck_list = [](const ActionLists& l, int o) {
    return std::find(l.chuck.begin(), l.chuck.end(), o) != l.chuck.end();
  };

  // Before Bob knows anything about it, his r1 is NOT chuckable — this is the
  // whole point of judging from his view rather than from `state.deck`.
  ActionLists blind = hanabi::reactor0::action_lists(g, 1);
  EXPECT_FALSE(on_chuck_list(blind, r1))
      << "Bob cannot see his own r1, so he cannot know it is safe to throw";
  EXPECT_TRUE(on_chuck_list(blind, ctd))
      << "an explicit CTD is on the chuck list whatever the holder knows";

  // Once his inference pins it to red 1, it becomes chuckable.
  g.players[1].thoughts[r1].inferred = IdentitySet::single(Identity{0, 1});
  ActionLists knowing = hanabi::reactor0::action_lists(g, 1);
  EXPECT_TRUE(on_chuck_list(knowing, r1))
      << "known basic trash costs nothing to throw";
  EXPECT_FALSE(on_chuck_list(knowing, g5))
      << "g5 is neither called nor chuckable";
}

// A pinned identity is not required — a card every one of whose possibilities
// is basic trash is known trash, even if the holder cannot say which.
TEST(Reactor0Calls, AllPossibilitiesTrashIsChuckable) {
  SetupOptions opts = base();
  opts.hands[1] = {"r1", "y3", "p5", "g5", "g2"};
  opts.play_stacks = {1, 1, 0, 0, 0};  // r1 and y1 both played
  Game g = setup(std::move(opts));

  const int o = order_at(g, TestPlayer::BOB, 1);
  g.players[1].thoughts[o].inferred =
      IdentitySet::from_iter({Identity{0, 1}, Identity{1, 1}});

  ActionLists lists = hanabi::reactor0::action_lists(g, 1);
  EXPECT_NE(std::find(lists.chuck.begin(), lists.chuck.end(), o),
            lists.chuck.end())
      << "r1 or y1 — either way it is trash, so it is chuckable";
}

// --- the Actionable Card Priority walk -----------------------------------

// Rungs 2-8 PITCH (press Play) and 9-11 CHUCK (press Discard); 12 and 13 are a
// floor, so the walk always answers. Rung 1 is absent by design — it is
// `take_action`'s urgent return, which runs above the clue phase.
namespace {

// Alice's hand is real but unknown to her, which is what a hand normally is.
// Each fixture then pins exactly the inferences the rung under test needs.
SetupOptions alice_position() {
  SetupOptions opts;
  opts.hands = {
      {"r1", "b4", "p4", "y4", "g4"},
      {"r2", "y3", "b3", "p3", "g3"},
      {"y2", "b2", "p2", "g2", "r4"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return opts;
}

void alice_knows(Game& g, int slot, Identity id) {
  g.players[0].thoughts[order_at(g, TestPlayer::ALICE, slot)].inferred =
      IdentitySet::single(id);
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

// Rung 2 — pitch a card that sets up a card a partner already holds. Alice's
// slot 1 is r1 and Bob holds r2, so playing it unblocks him.
TEST(Reactor0Action, PitchesTheCardThatSetsUpAPartner) {
  Game g = setup(alice_position());
  const int r1 = order_at(g, TestPlayer::ALICE, 1);
  call(g, TestPlayer::ALICE, 1, CardStatus::CALLED_TO_PLAY, false);
  alice_knows(g, 1, Identity{0, 1});

  EXPECT_EQ(played(hanabi::reactor0::choose_action(g)), r1)
      << "rung 2: r1 connects to Bob's r2";
}

// Rung 4 — with nothing to set up, pitch the head of a chain that has
// dependants, because playing it is what unblocks the rest of the chain.
TEST(Reactor0Action, PitchesTheHeadOfADependentChain) {
  SetupOptions opts = alice_position();
  // No partner holds a connector for either of Alice's called cards.
  opts.hands[1] = {"y3", "b3", "p3", "g3", "y4"};
  opts.hands[2] = {"y2", "b2", "p2", "g2", "b5"};
  opts.hands[0] = {"r1", "r2", "p4", "y4", "g4"};
  Game g = setup(std::move(opts));

  const int r1 = order_at(g, TestPlayer::ALICE, 1);  // newer slot
  const int r2 = order_at(g, TestPlayer::ALICE, 2);
  call(g, TestPlayer::ALICE, 1, CardStatus::CALLED_TO_PLAY, false);
  call(g, TestPlayer::ALICE, 2, CardStatus::CALLED_TO_PLAY, false);
  alice_knows(g, 1, Identity{0, 1});
  alice_knows(g, 2, Identity{0, 2});

  auto lists = hanabi::reactor0::action_lists(g, 0);
  ASSERT_EQ(lists.pitch_chains.size(), 1u) << "guard: both reds are one chain";
  ASSERT_EQ(lists.pitch_chains[0].size(), 2u);

  EXPECT_EQ(played(hanabi::reactor0::choose_action(g)), r1)
      << "rung 4: the chain head, not r2 which sits behind it";
  EXPECT_NE(played(hanabi::reactor0::choose_action(g)), r2);
}

// Rungs 5-7 — among critical cards, the lowest rank in play direction goes
// first. Here Alice holds a critical 1 and a critical 2 and neither sets
// anything up, so rung 5 must beat rung 6.
TEST(Reactor0Action, PitchesTheCriticalOneBeforeTheCriticalTwo) {
  SetupOptions opts = alice_position();
  // Different SUITS on purpose. Two critical cards of the same suit would be a
  // dependence chain, and rung 4 would take the chain head before rungs 5-7
  // ever ran — correctly, but it would stop this fixture testing them.
  opts.hands[0] = {"y2", "r1", "p4", "b4", "g4"};
  opts.hands[1] = {"b3", "p3", "g3", "b4", "p5"};
  opts.hands[2] = {"b2", "p2", "g2", "b5", "g5"};
  // Enough copies gone that both of Alice's are the last of their kind.
  opts.discarded = {"r1", "r1", "y2"};
  Game g = setup(std::move(opts));

  const int y2 = order_at(g, TestPlayer::ALICE, 1);
  const int r1 = order_at(g, TestPlayer::ALICE, 2);
  call(g, TestPlayer::ALICE, 1, CardStatus::CALLED_TO_PLAY, false);
  call(g, TestPlayer::ALICE, 2, CardStatus::CALLED_TO_PLAY, false);
  alice_knows(g, 1, Identity{1, 2});
  alice_knows(g, 2, Identity{0, 1});
  ASSERT_TRUE(g.state.is_critical(Identity{0, 1})) << "guard: r1 is critical";
  ASSERT_TRUE(g.state.is_critical(Identity{1, 2})) << "guard: y2 is critical";

  auto lists = hanabi::reactor0::action_lists(g, 0);
  ASSERT_EQ(lists.pitch_chains.size(), 2u)
      << "guard: different suits, so two singleton chains and no rung 4";

  const int pick = played(hanabi::reactor0::choose_action(g));
  EXPECT_EQ(pick, r1) << "rung 5 (critical 1) outranks rung 6 (critical 2)";
  EXPECT_NE(pick, y2);
}

// Rung 12 — both lists empty, so Alice discards her chop.
TEST(Reactor0Action, FloorDiscardsTheChop) {
  SetupOptions opts = alice_position();
  opts.clue_tokens = 4;  // below 8, so a discard is legal and rung 13 is off
  Game g = setup(std::move(opts));
  auto lists = hanabi::reactor0::action_lists(g, 0);
  ASSERT_TRUE(lists.pitch.empty()) << "guard: nothing to pitch";
  ASSERT_TRUE(lists.chuck.empty()) << "guard: nothing to chuck";

  auto chop = g.chop(0);
  ASSERT_TRUE(chop.has_value());
  EXPECT_EQ(discarded(hanabi::reactor0::choose_action(g)), *chop)
      << "rung 12: the floor is a chop discard";
}

// Rung 13 — at 8 clue tokens a discard is illegal, so the same empty-list
// position pitches the chop instead of discarding it.
TEST(Reactor0Action, AtEightCluesTheFloorPitchesTheChop) {
  SetupOptions opts = alice_position();
  opts.clue_tokens = 8;
  Game g = setup(std::move(opts));

  auto chop = g.chop(0);
  ASSERT_TRUE(chop.has_value());
  auto action = hanabi::reactor0::choose_action(g);
  EXPECT_EQ(played(action), *chop)
      << "rung 13: at 8 tokens the chop is pitched, never discarded";
  EXPECT_EQ(discarded(action), -1);
}

// The walk always answers. `take_action` has to return a move, so a floor that
// could decline would be a bug.
TEST(Reactor0Action, AlwaysReturnsAnAction) {
  for (int tokens : {0, 1, 4, 8}) {
    SetupOptions opts = alice_position();
    opts.clue_tokens = tokens;
    Game g = setup(std::move(opts));
    EXPECT_TRUE(hanabi::reactor0::choose_action(g).has_value())
        << "no action at " << tokens << " clue tokens";
  }
}

// A playable card on an INVERTED suit is played by CHUCKING it, never by
// pitching it: pressing Play sends an orange to the discard pile. So empathy
// thinking it "playable" must not put it on the pitch list.
//
// Regression for the one divergence phase 2's splice caused. Replay 1959065
// holds a called Dark Orange 2 with the stack at 1. `thinks_playables` reported
// it, the pitch list took it, and rung 8 pitched it — the right card by the
// wrong button, throwing away a oneOfEach card that would have advanced its
// stack.
TEST(Reactor0Action, APlayableInvertedCardIsChuckedNotPitched) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  opts.hands = {
      {"o2", "b4", "r4", "g4", "b5"},
      {"r3", "g3", "b3", "r5", "g5"},
      {"r2", "g2", "b2", "r4", "o5"},
  };
  opts.play_stacks = {0, 0, 0, 1};  // the orange stack (suit 3) is at 1
  opts.clue_tokens = 4;
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int orange = 3;
  ASSERT_TRUE(g.state.variant->suits[orange].suit_type.inverted)
      << "guard: suit 3 is the inverted one";
  const int o2 = order_at(g, TestPlayer::ALICE, 1);
  alice_knows(g, 1, Identity{orange, 2});

  auto lists = hanabi::reactor0::action_lists(g, 0);
  EXPECT_EQ(std::find(lists.pitch.begin(), lists.pitch.end(), o2),
            lists.pitch.end())
      << "a playable orange must not be pitchable — Play throws it away";
  EXPECT_NE(std::find(lists.chuck.begin(), lists.chuck.end(), o2),
            lists.chuck.end())
      << "it belongs on the chuck list, where Discard advances its stack";

  auto action = hanabi::reactor0::choose_action(g);
  EXPECT_EQ(discarded(action), o2) << "chucked, which is how an orange plays";
  EXPECT_NE(played(action), o2);
}

// --- the stamp is the instruction ----------------------------------------

// A card stamped CTD is chucked (press Discard) and a card stamped CTP is
// pitched (press Play), always -- until some later information proves that
// button would misplay. So a standing call is on its list regardless of what
// `is_chuckable` would say about an uncalled card.
TEST(Reactor0Calls, AStandingCallIsAlwaysOnItsList) {
  SetupOptions opts = base();
  opts.play_stacks = {0, 0, 0, 0, 0};
  Game g = setup(std::move(opts));

  // Neither card is chuckable or playable on its own merits.
  call(g, TestPlayer::BOB, 2, CardStatus::CALLED_TO_DISCARD, false);
  call(g, TestPlayer::BOB, 4, CardStatus::CALLED_TO_PLAY, false);

  ActionLists lists = hanabi::reactor0::action_lists(g, 1);
  EXPECT_NE(std::find(lists.chuck.begin(), lists.chuck.end(),
                      order_at(g, TestPlayer::BOB, 2)),
            lists.chuck.end())
      << "a CTD is chucked because it is a CTD, not because it looks chuckable";
  EXPECT_NE(std::find(lists.pitch.begin(), lists.pitch.end(),
                      order_at(g, TestPlayer::BOB, 4)),
            lists.pitch.end())
      << "and a CTP is pitched for the same reason";
}

// The exception, on your worked example. An Orange 1 stamped CTD is chucked
// happily while the orange stack is at 0. Once the other Orange 1 plays and the
// holder learns their card is orange, the chuck would strike -- so the call
// stops being actionable and drops off the list.
TEST(Reactor0Calls, ACallStopsBeingActionableOnceItsButtonWouldStrike) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  opts.hands = {
      {"r3", "b4", "g4", "r5", "b5"},
      {"o1", "r2", "g3", "b3", "r4"},
      {"g2", "b2", "r3", "g5", "o4"},
  };
  opts.play_stacks = {0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int orange = 3;
  ASSERT_TRUE(g.state.variant->suits[orange].suit_type.inverted);
  const int o = order_at(g, TestPlayer::BOB, 1);
  call(g, TestPlayer::BOB, 1, CardStatus::CALLED_TO_DISCARD, false);
  // The holder knows only that it is orange, not which orange.
  g.players[1].thoughts[o].inferred = IdentitySet::from_iter(
      {Identity{orange, 1}, Identity{orange, 2}, Identity{orange, 3}});

  EXPECT_TRUE(hanabi::reactor0::call_is_actionable(g, 1, o))
      << "with the orange stack at 0, chucking could still advance it";

  // The other Orange 1 plays, and the holder narrows to Orange 1.
  g.state.play_stacks[orange] = 1;
  g.players[1].thoughts[o].inferred = IdentitySet::single(Identity{orange, 1});

  EXPECT_FALSE(hanabi::reactor0::call_is_actionable(g, 1, o))
      << "an orange below its own stack cannot be chucked without striking";
  ActionLists lists = hanabi::reactor0::action_lists(g, 1);
  EXPECT_EQ(std::find(lists.chuck.begin(), lists.chuck.end(), o),
            lists.chuck.end())
      << "so it leaves the chuck list rather than being offered as a chuck";
}

// --- an INVESTED card needs `possible`, not just `inferred` ----------------

// Throwing away a card the team spent a clue on is irreversible, and `inferred`
// is a convention deduction that can be wrong. Replay 1971788 T29, "Odds and
// Evens & Dark Omni": a lock's rank promise narrowed slot 5 to {r1,y1,g1,b1,p1}
// -- all trash -- while `possible` still held d3, d4 and d5, each a single
// copy. It was the d5. It was the only chuckable card, so rung 11 threw it and
// the max score fell 30 to 29 while an actual chop sat unclued in slot 1.
//
// So for a clued or stamped card the first arm of `is_chuckable` now needs
// `possible` to agree. An untouched card is unaffected: nothing narrowed it, so
// the two sets are the same.
namespace {

// Bob's slot 1, read as trash by inference while `possible` stays wider.
// g5 is neither trash nor reachable here, so `possible` genuinely disagrees.
int seed_divergent_reading(Game& g, bool clued) {
  const int o = order_at(g, TestPlayer::BOB, 1);
  g.players[1].thoughts[o].inferred =
      IdentitySet::from_iter({Identity{0, 1}, Identity{1, 1}});
  g.players[1].thoughts[o].possible = IdentitySet::from_iter(
      {Identity{0, 1}, Identity{1, 1}, Identity{3, 5}});  // b5 is critical
  if (clued) g.state.deck[o].clued = true;
  return o;
}

bool on_chuck_list(const ActionLists& l, int o) {
  return std::find(l.chuck.begin(), l.chuck.end(), o) != l.chuck.end();
}

}  // namespace

TEST(Reactor0Calls, ACluedCardIsNotChuckedOnInferenceAlone) {
  SetupOptions opts = base();
  opts.hands[1] = {"r1", "y3", "p5", "g5", "g2"};
  opts.play_stacks = {1, 1, 0, 0, 0};  // r1 and y1 both played
  Game g = setup(std::move(opts));
  const int o = seed_divergent_reading(g, /*clued=*/true);

  EXPECT_FALSE(on_chuck_list(hanabi::reactor0::action_lists(g, 1), o))
      << "the reading says trash, but `possible` still admits a b5 -- a clue "
         "was spent on this card and the throw is irreversible";
}

// Scoping: the guard is about INVESTED cards. An untouched card with the same
// reading is still chuckable, because nothing narrowed it in the first place.
TEST(Reactor0Calls, AnUncluedCardWithTheSameReadingIsStillChuckable) {
  SetupOptions opts = base();
  opts.hands[1] = {"r1", "y3", "p5", "g5", "g2"};
  opts.play_stacks = {1, 1, 0, 0, 0};
  Game g = setup(std::move(opts));
  const int o = seed_divergent_reading(g, /*clued=*/false);

  EXPECT_TRUE(on_chuck_list(hanabi::reactor0::action_lists(g, 1), o));
}

// A stamp counts as investment too -- 1971788's card was CHOP_MOVED, not clued.
TEST(Reactor0Calls, AChopMovedCardIsNotChuckedOnInferenceAlone) {
  SetupOptions opts = base();
  opts.hands[1] = {"r1", "y3", "p5", "g5", "g2"};
  opts.play_stacks = {1, 1, 0, 0, 0};
  Game g = setup(std::move(opts));
  const int o = seed_divergent_reading(g, /*clued=*/false);
  g.meta[o].status = CardStatus::CHOP_MOVED;

  EXPECT_FALSE(on_chuck_list(hanabi::reactor0::action_lists(g, 1), o));
}

// And when `possible` agrees, the clued card is chuckable as before -- the
// guard asks for proof, not for the card to be untouched.
TEST(Reactor0Calls, ACluedCardWhosePossibleIsAllTrashStaysChuckable) {
  SetupOptions opts = base();
  opts.hands[1] = {"r1", "y3", "p5", "g5", "g2"};
  opts.play_stacks = {1, 1, 0, 0, 0};
  Game g = setup(std::move(opts));
  const int o = order_at(g, TestPlayer::BOB, 1);
  g.players[1].thoughts[o].inferred =
      IdentitySet::from_iter({Identity{0, 1}, Identity{1, 1}});
  g.players[1].thoughts[o].possible =
      IdentitySet::from_iter({Identity{0, 1}, Identity{1, 1}});
  g.state.deck[o].clued = true;

  EXPECT_TRUE(on_chuck_list(hanabi::reactor0::action_lists(g, 1), o));
}
