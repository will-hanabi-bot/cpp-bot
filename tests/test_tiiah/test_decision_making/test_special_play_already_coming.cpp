// The one exception to the special-suit VERY HIGH (reactor0 DECISION_MAKING.md,
// Clue Tier Definitions; v23.21.0, the user's ruling; replay 2026415 T44).
//
// A reactive play that gets a dark special-suit or Null card played is VERY HIGH,
// unless it BOTH gets a card a pending reaction will call anyway once its reacter
// reacts, AND gets a card the receiver already holds called or knows plays.
//
// The live T44 position does not replay from the log (the replay parts from the
// live state well before it, at every build back to v23.18.0), so this pins the
// predicate on its shape: Alice owes a reaction whose receiver target is Bob's u2,
// and a 1 to Cathy would pair that u2 with Cathy's r5, already called.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using hanabi::reactor0::ClueCandidate;
using hanabi::reactor0::ClueShape;
using hanabi::reactor0::Outcome;
using hanabi::test::tiiah::order_at;

namespace {

constexpr int kAlice = static_cast<int>(TestPlayer::ALICE);
constexpr int kBob = static_cast<int>(TestPlayer::BOB);
constexpr int kCathy = static_cast<int>(TestPlayer::CATHY);

// Red on 4, the null suit on 1. Bob's slot 1 is the u2; Cathy's slot 5 the r5.
Game position(bool r5_called, bool reaction_owed) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole & Dark Null (6 Suits)";
  opts.play_stacks = std::vector<int>{4, 5, 5, 0, 4, 1};
  opts.clue_tokens = 2;
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"u2", "u5", "g3", "b3", "b5"},
                {"p4", "r4", "y3", "r1", "r5"}};
  opts.starting = TestPlayer::ALICE;
  hanabi::test::tiiah::use_tiiah(opts);
  Game g = setup(std::move(opts));
  const int r5 = order_at(g, TestPlayer::CATHY, 5);
  if (r5_called) {
    g.state.deck[r5].clued = true;
    g.with_thought(r5, [](const Thought& t) {
      Thought out = t;
      out.inferred = IdentitySet::single(Identity{0, 5});
      return out;
    });
    g.with_meta(r5, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
  }
  if (reaction_owed) {
    // Cathy's earlier clue: Alice reacts, Bob receives, and the walk named his u2.
    ReactorWC wc;
    wc.giver = kCathy;
    wc.reacter = kAlice;
    wc.receiver = kBob;
    wc.receiver_target_order = order_at(g, TestPlayer::BOB, 1);
    g.waiting.push_back(wc);
  }
  return g;
}

// The 1 to Cathy: Bob reacts with the u2, Cathy plays the r5.
ClueCandidate one_to_cathy(const Game& g) {
  const int r1 = order_at(g, TestPlayer::CATHY, 4);
  ClueCandidate c{PerformAction{PerformRank{kCathy, 1}},
                  ClueAction{kAlice, kCathy, {r1}, BaseClue(ClueKind::RANK, 1)},
                  hanabi::reactor0::ClueReading{}, hanabi::reactor0::ClueTier::LOW, 0.0};
  c.reading.shape = ClueShape::REACTIVE_PLAY;
  c.reading.reacter_side.order = order_at(g, TestPlayer::BOB, 1);
  c.reading.reacter_side.holder = kBob;
  c.reading.reacter_side.outcome = Outcome::PLAY;
  c.reading.receiver_side.order = order_at(g, TestPlayer::CATHY, 5);
  c.reading.receiver_side.holder = kCathy;
  c.reading.receiver_side.outcome = Outcome::PLAY;
  return c;
}

}  // namespace

// Both: the u2 is coming from Alice's reaction, and the r5 is already called.
TEST(TiiahSpecialPlayAlreadyComing, BothConditionsDemote) {
  Game g = position(/*r5_called=*/true, /*reaction_owed=*/true);
  EXPECT_TRUE(hanabi::reactor0::special_play_already_coming(g, one_to_cathy(g)));
}

// No reaction owed: the u2 is not coming by itself, so the clue keeps VERY HIGH.
TEST(TiiahSpecialPlayAlreadyComing, NothingOwedKeepsVeryHigh) {
  Game g = position(/*r5_called=*/true, /*reaction_owed=*/false);
  EXPECT_FALSE(hanabi::reactor0::special_play_already_coming(g, one_to_cathy(g)));
}

// The r5 is not yet called or known: the clue gets it new, so it keeps VERY HIGH.
TEST(TiiahSpecialPlayAlreadyComing, ANewReceiverCardKeepsVeryHigh) {
  Game g = position(/*r5_called=*/false, /*reaction_owed=*/true);
  EXPECT_FALSE(hanabi::reactor0::special_play_already_coming(g, one_to_cathy(g)));
}
