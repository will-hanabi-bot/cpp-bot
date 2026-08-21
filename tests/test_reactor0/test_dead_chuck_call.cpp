// A chuck call that has gone dead is erased, and the card is pitched instead.
//
// A chuck presses Discard, which on an INVERTED suit is a play attempt. So a
// CTD dies the moment every reading left is an inverted card that is not the
// next for its stack -- call invariant rule 4, the mirror of rule 3's
// `drop_dead_play_calls`.
//
// Leaving the dead call in place is not harmless: `Game::chop`'s first pass
// returns a CTD, so a stale one silently becomes the hand's chop, and
// `requires_high_tier` counts it, so the holder stays "occupied".
//
// Once erased the card is on NEITHER action list -- chucking it strikes and it
// is not playable, so `thinks_playables` never offers it. Rung 11b picks it up:
// pressing Play on an inverted card sends it to the discard pile, which is the
// only safe way to be rid of it.
//
// Replay 1967287: a CTD stamped when the orange stack was on 0 promised the one
// chuckable orange, o1. Two turns later o1 was on the stacks, and the call was
// still being chucked -- for a strike.
#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/reactor0/call_invariants.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

constexpr int kOrange = 2;

// "Orange (3 Suits)" -- r / b / o, orange inverted. The orange stack sits on 1,
// so o1 is trash and o2 is the only chuckable orange.
SetupOptions dead_chuck_opts() {
  SetupOptions opts;
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 1};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us) -- face down
      {"r1", "b1", "r2", "b2", "r3"},
      {"b3", "r4", "b4", "o2", "o3"},
  };
  use_reactor0(opts);
  return opts;
}

// Pin one card in every view and stamp it CALLED_TO_DISCARD, the way a
// resolved receiver-CTD would leave it.
void stamp_dead_chuck(Game& g, int order, Identity id) {
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
  g.with_meta(order, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m = m.reason(0).signal(0);
  });
}

}  // namespace

TEST(Reactor0DeadChuckCall, RuleFourErasesAChuckThatWouldOnlyStrike) {
  Game g = setup(dead_chuck_opts());
  const int order = order_at(g, TestPlayer::ALICE, 3);
  stamp_dead_chuck(g, order, Identity{kOrange, 1});  // o1, trash on a stack of 1
  ASSERT_EQ(g.meta[order].status, CardStatus::CALLED_TO_DISCARD);

  hanabi::reactor0::enforce_call_invariants(g);

  EXPECT_NE(g.meta[order].status, CardStatus::CALLED_TO_DISCARD)
      << "chucking a dead orange can only strike, so the call is erased";
}

// The call survives while the chuck is still sound: o2 IS the next orange.
TEST(Reactor0DeadChuckCall, ALiveChuckCallIsUntouched) {
  Game g = setup(dead_chuck_opts());
  const int order = order_at(g, TestPlayer::ALICE, 3);
  stamp_dead_chuck(g, order, Identity{kOrange, 2});  // o2, playable

  hanabi::reactor0::enforce_call_invariants(g);

  EXPECT_EQ(g.meta[order].status, CardStatus::CALLED_TO_DISCARD)
      << "the stamp is the instruction while the chuck still stacks the card";
}

// Rung 11b: once erased, the dead orange is disposed of with the PLAY button.
TEST(Reactor0DeadChuckCall, TheDeadOrangeIsPitched) {
  Game g = setup(dead_chuck_opts());
  const int order = order_at(g, TestPlayer::ALICE, 3);
  stamp_dead_chuck(g, order, Identity{kOrange, 1});
  hanabi::reactor0::enforce_call_invariants(g);
  // Zero tokens so the clue phase cannot fire; the Precedence puts clues above
  // phase 2, which is why "pitched LATER" is the right description of 11b.
  g.state.clue_tokens = 0;

  PerformAction action = g.take_action();

  ASSERT_TRUE(std::holds_alternative<PerformPlay>(action))
      << "a pitch presses Play, which on an inverted suit discards the card";
  EXPECT_EQ(std::get<PerformPlay>(action).target, order);
}

// The negative for 11b: a dead orange that is CRITICAL is not thrown away.
// Pitching is a permanent loss, so the rung declines and the card is left for
// the floor to reason about.
TEST(Reactor0DeadChuckCall, ACriticalDeadOrangeIsNotPitchedByElevenB) {
  Game g = setup(dead_chuck_opts());
  const int order = order_at(g, TestPlayer::ALICE, 3);
  stamp_dead_chuck(g, order, Identity{kOrange, 5});  // o5: unplayable AND critical
  hanabi::reactor0::enforce_call_invariants(g);
  ASSERT_NE(g.meta[order].status, CardStatus::CALLED_TO_DISCARD)
      << "rule 4 still erases it -- o5 is not chuckable either";
  g.state.clue_tokens = 0;

  PerformAction action = g.take_action();

  const auto* play = std::get_if<PerformPlay>(&action);
  EXPECT_FALSE(play != nullptr && play->target == order)
      << "11b must not throw away a critical card";
}
