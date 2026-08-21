// A colour clue that resolves WHICH BUTTON a card needs is a play reveal.
//
// `obvious_playables` answers "could this card play". In an inverted variant
// that is not the same question as "can the holder act on it": a card whose
// readings are {r2, o2} with both stacks on 1 is playable either way, but r2
// needs the Play button and o2 needs Discard, so the holder cannot move. A clue
// that pins the button is a genuine reveal even though playability did not
// change.
//
// Replay 1967279 T8 is the case that was wrong. `find_play_reveal` saw nothing
// new, §1b fell through to the direct play at priority 5, and that stamped the
// leftmost TOUCHED card with a promise the clue never made -- while the card
// the clue actually resolved got nothing.
//
// v7.25.0 also makes the reveal STAMP: the revealed card goes into the
// receiver-CTP queue rather than being left to empathy.
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// "Muddy Rainbow & Orange (3 Suits)" -- r / m / o. Muddy Rainbow is rainbowish
// (every colour clue touches it) AND brownish (no rank clue does), which is
// what lets a rank-2 clue leave a card as exactly {r2, o2}: the muddy 2 is
// never touched by rank, and orange is its own colour.
SetupOptions reveal_opts() {
  SetupOptions opts;
  opts.variant_name = "Muddy Rainbow & Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {1, 1, 1};
  opts.hands = {
      {"r4", "m4", "r5", "m5", "o4"},  // Alice (giver, us)
      {"m3", "r3", "o3", "r2", "m1"},  // Bob (receiver) -- r2 on slot 4
      {"o1", "m2", "o2", "r1", "o5"},  // Cathy
  };
  use_reactor0(opts);
  return opts;
}

int calls_of_status(const Game& g, TestPlayer who, CardStatus st) {
  int n = 0;
  for (int o : g.state.hands[static_cast<int>(who)]) {
    if (g.meta[o].status == st) ++n;
  }
  return n;
}

}  // namespace

TEST(Reactor0ButtonReveal, ResolvingTheButtonIsAPlayReveal) {
  Game g = setup(reveal_opts());
  // Rank 2 leaves Bob's slot 4 as exactly {r2, o2}: both playable on stacks of
  // 1, but r2 wants Play and o2 wants Discard.
  g = pre_clue(std::move(g), TestPlayer::BOB, 4, {"2"});
  // `pre_clue` writes COMMON knowledge, which is also what `find_play_reveal`
  // reads, so the guard has to look there and not at Bob's own view.
  expect_infs(g, std::nullopt, TestPlayer::BOB, 4, {"r2", "o2"});

  g = take_turn(std::move(g), "Alice clues red to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REVEAL)
      << "the clue resolved the button, which is a reveal even though the card "
         "was already 'playable' both ways";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_PLAY)
      << "the REVEALED card goes into the receiver-CTP queue";
  EXPECT_EQ(calls_of_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY), 1)
      << "and nothing else is called -- in particular not the leftmost touched "
         "card, which is what replay 1967279 T8 wrongly promised";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
}

// The negative that keeps the rule honest: a clue leaving the card still
// button-ambiguous is not a reveal. Orange touches Bob's slot 4 without pinning
// it, so {r2, o2} survives and nothing is resolved.
TEST(Reactor0ButtonReveal, StillAmbiguousAfterTheClueIsNotAReveal) {
  Game g = setup(reveal_opts());
  g = pre_clue(std::move(g), TestPlayer::BOB, 4, {"2"});
  const IdentitySet before = g.common.thoughts[order_at(g, TestPlayer::BOB, 4)].inferred;

  // A muddy clue would touch slot 4 only if it were muddy, which rank 2 has
  // already ruled out -- so use a clue that leaves slot 4 untouched entirely
  // and check the card is not claimed as a reveal.
  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_EQ(g.common.thoughts[order_at(g, TestPlayer::BOB, 4)].inferred, before)
      << "slot 4 is untouched, so nothing about it was resolved";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_PLAY)
      << "and it must not be claimed as a revealed playable";
}
