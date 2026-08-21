// A reacter told to press Discard on a playable INVERTED card is CHUCKING it.
//
// Both layers of the odd-parity path read that button the plain-suit way:
//
//   * `vet_react_slot`'s "safe to throw away" test asks only whether every
//     reading is critical. Every Dark Orange card is one-of-each, hence
//     critical by construction, so a reacter slot whose readings are all dark
//     is always retargeted -- even when the chuck would stack one of them.
//   * `reactor::target_discard` narrows to the NON-critical ids, which empties
//     for the same reason and refuses to stamp at all, so the whole clue reads
//     as a MISTAKE.
//
// Replay 1967491 T36: will-bot67's slot 5 was {d2, d4} with the dark stack on
// 1, so chucking it stacks the d2 -- and the clue was a MISTAKE instead.
// `stamp_orange_chuck` is the stamp that narrows to what the button advances;
// it already existed for the stable orange ladder and is now shared with the
// reactive side, exactly as `stamp_orange_pitch` already was.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// "Dark Orange (5 Suits)" -- r/y/g/b/d, with d dark AND inverted, so every dark
// card is a singleton and therefore critical. The dark stack sits on 1, so d2
// is the one dark card a chuck would stack.
SetupOptions dark_opts() {
  SetupOptions opts;
  opts.variant_name = "Dark Orange (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 1};
  opts.hands = {
      {"r4", "y4", "g4", "b4", "r5"},  // Alice (giver, us)
      {"o2", "r1", "y1", "g1", "b1"},  // Bob (reacter) -- d2 on slot 1
      {"r2", "y2", "g2", "b2", "o4"},  // Cathy (receiver)
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

// The stamp. A card whose readings are all dark cannot be narrowed to a
// non-critical id, so `target_discard` refuses; `stamp_orange_chuck` narrows to
// the identities the Discard button actually advances.
TEST(Reactor0ReactiveDarkChuck, StampNarrowsToWhatTheChuckAdvances) {
  Game g = setup(dark_opts());
  const int mine = order_at(g, TestPlayer::BOB, 1);
  // Pre-clue the dark colour so every reading is a critical dark card -- the
  // shape that defeats both plain-suit tests.
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"dark orange"});

  ClueAction act{static_cast<int>(TestPlayer::ALICE),
                 static_cast<int>(TestPlayer::BOB),
                 {mine},
                 BaseClue(ClueKind::COLOUR, 4)};
  auto interp = hanabi::reactor0::stamp_orange_chuck(g, act, mine,
                                                     /*urgent=*/true);

  ASSERT_TRUE(interp.has_value())
      << "the chuck stamp must not refuse a card every reading of which is a "
         "critical dark -- that is precisely the Dark Orange case";
  EXPECT_EQ(g.meta[mine].status, CardStatus::CALLED_TO_DISCARD)
      << "CTD is the Discard button, which on an inverted suit stacks the card";
  EXPECT_TRUE(g.meta[mine].urgent) << "a reacter call is urgent";
  // o2 (Dark Orange 2) is the only dark the stack is waiting for.
  expect_infs(g, std::nullopt, TestPlayer::BOB, 1, {"o2"});
}

// The mirror: with the dark stack already past d2, nothing that card could be
// would be advanced by a chuck, so the stamp declines rather than promising a
// strike.
TEST(Reactor0ReactiveDarkChuck, StampDeclinesWhenNoReadingWouldStack) {
  SetupOptions opts = dark_opts();
  opts.play_stacks = {0, 0, 0, 0, 5};  // every dark is played; none is playable
  opts.hands[1] = {"r1", "y1", "g1", "b1", "r3"};
  opts.hands[2] = {"r2", "y2", "g2", "b2", "y3"};
  Game g = setup(std::move(opts));
  const int mine = order_at(g, TestPlayer::BOB, 1);

  ClueAction act{static_cast<int>(TestPlayer::ALICE),
                 static_cast<int>(TestPlayer::BOB),
                 {mine},
                 BaseClue(ClueKind::COLOUR, 4)};
  auto interp = hanabi::reactor0::stamp_orange_chuck(g, act, mine,
                                                     /*urgent=*/true);

  EXPECT_FALSE(interp.has_value())
      << "no reading of this card is a dark the stack is waiting for";
  EXPECT_NE(g.meta[mine].status, CardStatus::CALLED_TO_DISCARD);
}

// --- and the layer above the stamp ----------------------------------------

// `vet_react_slot` walks past a react slot that is not "safe to throw away",
// which it decided by asking whether every reading is critical. In Dark Orange
// that is every dark card, so a reacter slot KNOWN to be dark was always
// retargeted -- the one shape where the chuck is most clearly right.
//
// Yellow's reactive value is 2 and `calc_slot(2, 1, 5) == 1`, so a yellow clue
// whose only receiver playable sits on slot 1 pairs it with the reacter's slot
// 1. That slot is pre-clued dark, so its readings are {o1..o5}: all critical,
// and o1 is exactly what the dark stack is waiting for.
TEST(Reactor0ReactiveDarkChuck, AKnownDarkReactSlotIsNotRetargeted) {
  SetupOptions opts = dark_opts();
  // The dark stack sits at 0 here, and deliberately: `possible` keeps a played
  // dark as a reading, and a played card is trash rather than critical, so
  // leaving the stack on 1 would let the OLD test pass for the wrong reason.
  // With nothing played every one of the five readings really is critical.
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.hands = {
      {"r4", "y4", "g4", "b4", "r5"},  // Alice (giver, us)
      {"o1", "r3", "g4", "b4", "y3"},  // Bob (reacter) -- slot 1 is the dark
      {"y1", "r3", "g3", "b3", "y4"},  // Cathy (receiver) -- y1 is her only play
  };
  Game g = setup(std::move(opts));
  const int react = order_at(g, TestPlayer::BOB, 1);
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"dark orange"});

  g = take_turn(std::move(g), "Alice clues yellow to Cathy");

  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE)
      << "the pair exists; before v7.30.0 the react slot was walked past and "
         "the clue came out a MISTAKE";
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().react_order, react)
      << "slot 1 is the react slot the anchor picks";
  EXPECT_EQ(g.meta[react].status, CardStatus::CALLED_TO_DISCARD);
  expect_infs(g, std::nullopt, TestPlayer::BOB, 1, {"o1"});
}
