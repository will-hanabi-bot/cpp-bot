// Rainbowy colour pinning, and a clue from a superpositioned giver
// (tiiah/CONVENTION.md §1f).
//
// A colour clue in a rainbowy variant would ordinarily leave the receiver
// superposed between that colour's own suit and the rainbowy one. Here it does
// not: the call means the next playable of the colour's OWN suit, and re-pins to
// the rainbowy suit only when that is immediately impossible.
//
// The second half is §1e's rule reaching the stable ladder: the stacks a clue is
// read against are the SHARED ones, so a giver who has played into the hole
// without knowing what they played is read as though they had not played it.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Red, Yellow, Green, Blue, Purple, Rainbow. Nobody holds a known play, so a
// clue to Bob is stable (§1c's dispatch does not reverse).
SetupOptions rainbow_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole & Rainbow (6 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y4", "g4", "b4", "p4"},
      {"y3", "g3", "b3", "p3", "m3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

}  // namespace

// The rule itself. A red clue on turn 1 would leave Bob choosing between `r1`
// and `m1` — both are red-touched and both play. It does not: he writes `r1`.
TEST(TiiahRainbowy, AColourClueNamesItsOwnSuitNotTheRainbow) {
  Game g = setup(rainbow_opts());

  g = take_turn(std::move(g), "Alice clues red to Bob");

  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::BOB, /*slot=*/1, {"r1"});
}

// ...and a RANK play clue on a new card in slot 1 makes it the rainbow playable,
// unless that is directly impossible (v20.13.0, the user's ruling; replay 2019249
// T14). Until v20.13.0 a rank clue was never pinned.
TEST(TiiahRainbowy, ARankClueOnANewSlotOneCardIsTheRainbow) {
  Game g = setup(rainbow_opts());

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::BOB, /*slot=*/1, {"m1"});
}

// ...and not when the rainbow playable is directly impossible: rainbow is already on
// 1, so its next card is no 1 at all.
TEST(TiiahRainbowy, ARankClueIsNotPinnedPastTheRainbow) {
  SetupOptions opts = rainbow_opts();
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 0, 1};
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  const IdentitySet infs =
      g.common.thoughts[order_at(g, TestPlayer::BOB, 1)].inferred;
  EXPECT_GT(infs.length(), 1);
}

// The control: the 1 calls Bob's slot 2, not his slot 1, so reactor0's rank ladder
// reads it as it always did, a playable 1 of no particular suit.
TEST(TiiahRainbowy, ARankClueOffSlotOneIsNotPinned) {
  SetupOptions opts = rainbow_opts();
  opts.hands[1] = {"y4", "r1", "g4", "b4", "p4"};
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  ASSERT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY);
  const IdentitySet infs =
      g.common.thoughts[order_at(g, TestPlayer::BOB, 2)].inferred;
  EXPECT_GT(infs.length(), 1)
      << "a rank 1 clue off slot 1 says a playable 1, and does not say which suit";
}

// The exception: red is finished, so "the next playable red" does not exist and
// the call re-pins to the rainbowy suit's next playable instead.
TEST(TiiahRainbowy, TheCallRePinsWhenTheOwnSuitIsFinished) {
  SetupOptions opts = rainbow_opts();
  opts.play_stacks = std::vector<int>{5, 0, 0, 0, 0, 1};  // red done, rainbow on 1
  opts.hands[1] = {"m2", "y4", "g4", "b4", "p4"};
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues red to Bob");

  ASSERT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::BOB, /*slot=*/1, {"m2"});
}

// §1f's second half. Bob plays a card he cannot name — we watched it and know it
// was the purple 1, but he did not — and then clues purple. Our own stacks say
// purple is on 1, so "the next playable purple" would be the `p2` to us alone. The
// clue is read on the GIVER's own stacks (§1e, v20.4.0): Bob cannot know his card
// was the p1, so he meant the `p1`, and the reading is `{p1}`. Cathy, who watched
// his p1 go in, throws hers as the dupe. Until v20.4.0 the giver's own hole cards
// widened the call to `{p1, p2}`.
TEST(TiiahRainbowy, ASuperpositionedGiverIsReadOnItsOwnStacks) {
  SetupOptions opts = rainbow_opts();
  opts.starting = TestPlayer::BOB;
  // Two discards follow below, and a clue given at 8 tokens reads as a stall
  // rather than a play call — which is not what this test is about.
  opts.clue_tokens = 3;
  opts.hands[1] = {"y4", "g4", "b4", "r4", "p1"};  // the p1 he is about to play
  opts.hands[2] = {"p1", "y3", "g3", "b3", "r3"};  // the other copy, on her slot 1
  Game g = setup(std::move(opts));

  // Unclued, so his empathy cannot name it: the play is superposed.
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/5,
                    /*reached_the_hole=*/true, "y5");
  ASSERT_EQ(g.state.play_stacks[4], 1) << "we watched it, so we believe it landed";
  ASSERT_EQ(g.state.common_play_stacks[4], 0) << "and nobody else can say that";
  // Round the table back to Bob.
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/5,
                    /*reached_the_hole=*/false, "g5");
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/5,
                    /*reached_the_hole=*/false);

  g = take_turn(std::move(g), "Bob clues purple to Cathy");

  // Her draw above pushed the p1 from slot 1 to slot 2.
  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/2, {"p1"});
}
