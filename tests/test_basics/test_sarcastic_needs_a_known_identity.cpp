// A sarcastic discard is a SIGNAL -- "you hold the other copy of THIS card" --
// and a signal needs something to point at.
//
// `try_finding` (src/basics/sarcastic.cpp) has a fallback for the case where no
// copy of the discarded card is visible in any hand: it assumes the copy must
// be in OUR hand, since ours is the one hand we cannot see, and links over it.
// That is sound once a signal has actually been sent. It is badly unsound when
// one has not, because the copy can equally be sitting in the DECK -- and the
// resulting link narrows over time onto some innocent card and pins it to an
// identity it does not have.
//
// Replay 1974218 T8: a clued i4 was discarded whose empathy was all six 4s, a
// rank clue having named the rank and nothing else. The second i4 was still in
// the deck. Fourteen turns later the invented link collapsed onto a cardinal 2
// and stamped it {i4}, which made a reactive play clue unreadable and cost the
// game (tests/test_reactor0/test_misc/test_replay_1974218_*).
//
// So the gate (`useful_dc`, src/basics/decide.cpp) now asks whether the team
// already KNEW what was being thrown. The pair of tests below is the whole
// rule: the same card, the same discard, differing only in whether its identity
// was pinned first.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

// Alice is us, so a link over "our own hand" is a link over Alice's.
//
// Alice's slots 1-4 are pre-clued rank 1, which drops every 4 out of their
// `possible`; slot 5 is untouched. So exactly ONE of Alice's cards is a viable
// transfer candidate for a g4, and a link -- if one is created -- narrows to it
// immediately and stamps it. That makes the effect visible in a single turn
// instead of the fourteen it took in the replay.
//
// Nobody else holds a g4, and the green stack is on 0, so the g4 is useful but
// not playable: the plain sarcastic arm, not the gentleman's-discard one.
SetupOptions base() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},          // Alice (us)
      {"r1", "y1", "g4", "b1", "p1"},          // Bob -- one 4, the g4, slot 3
      {"r2", "y2", "b2", "p2", "r3"},          // Cathy -- no g4, no 4s
  };
  return opts;
}

Game seed(SetupOptions opts) {
  Game g = setup(std::move(opts));
  for (int slot = 1; slot <= 4; ++slot) {
    g = pre_clue(std::move(g), TestPlayer::ALICE, slot, {"1"});
  }
  return g;
}

int alice_slot5(const Game& g) {
  return g.state.hands[static_cast<int>(TestPlayer::ALICE)][4];
}

const Identity kG4{2, 4};

}  // namespace

// The bug. A rank-4 clue tells Bob the rank and nothing else, so when he throws
// the card there is no identity anyone can point at -- and no copy of it is
// visible anywhere, because the second g4 is in the deck.
TEST(SarcasticDiscard, ATouchedCardCarriesNoSignal) {
  Game g = seed(base());
  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  const int bob_g4 = g.state.hands[static_cast<int>(TestPlayer::BOB)][2];
  ASSERT_TRUE(g.state.deck[bob_g4].clued) << "guard: the g4 is clued";
  ASSERT_FALSE(g.common.thoughts[bob_g4].id(/*infer=*/true, /*symmetric=*/true))
      << "guard: but its identity is NOT pinned -- a rank-4 clue leaves "
         "{r4,y4,g4,b4,p4}";

  const int a5 = alice_slot5(g);
  g = take_turn(std::move(g), "Bob discards g4 (slot 3)", "b3");

  EXPECT_NE(g.meta[a5].status, CardStatus::SARCASTIC)
      << "no signal was sent, so no card of ours may be claimed for it";
  EXPECT_NE(g.common.thoughts[a5].inferred, IdentitySet::single(kG4))
      << "and our slot 5 must not be pinned to a g4 that is sitting in the "
         "deck";
}

// The control, and the reason the gate is a precondition rather than a blanket
// disable: the identical discard, with the identity pinned first, still reads
// as sarcastic and still claims our slot 5.
TEST(SarcasticDiscard, AKnownCardStillSignals) {
  // Bob leads, since there is no clue to give first; and one token is already
  // spent, because discarding at the cap of 8 is illegal.
  SetupOptions opts = base();
  opts.starting = TestPlayer::BOB;
  opts.clue_tokens = 7;
  Game g = seed(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, 3, "g4");

  const int bob_g4 = g.state.hands[static_cast<int>(TestPlayer::BOB)][2];
  ASSERT_EQ(g.common.thoughts[bob_g4].id(/*infer=*/true, /*symmetric=*/true),
            kG4)
      << "guard: this time the team knows exactly what it is";

  const int a5 = alice_slot5(g);
  g = take_turn(std::move(g), "Bob discards g4 (slot 3)", "b3");

  EXPECT_EQ(g.meta[a5].status, CardStatus::SARCASTIC)
      << "a deliberate throw of a known useful card is the signal, and our "
         "slot 5 is its only viable home";
  EXPECT_EQ(g.common.thoughts[a5].inferred, IdentitySet::single(kG4));
}

// Scoping: the gate asks what the team knew BEFORE the discard. A discard
// reveals the card to everyone, so asking afterwards would let every discard
// through and the gate would do nothing at all.
TEST(SarcasticDiscard, TheQuestionIsAskedOfTheStateBeforeTheDiscard) {
  Game g = seed(base());
  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  const int bob_g4 = g.state.hands[static_cast<int>(TestPlayer::BOB)][2];
  const int a5 = alice_slot5(g);
  g = take_turn(std::move(g), "Bob discards g4 (slot 3)", "b3");

  ASSERT_EQ(g.state.deck[bob_g4].id(), kG4)
      << "guard: after the fact the identity is public -- that is exactly why "
         "the gate reads `prev`";
  EXPECT_NE(g.meta[a5].status, CardStatus::SARCASTIC);
}
