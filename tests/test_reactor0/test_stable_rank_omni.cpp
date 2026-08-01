// Reactor0 §1c priority 1 in an OMNI variant (bug_report_1.txt 1.2 and 1.3).
//
// An omni suit is both pinkish and rainbowish, so `Variant::id_touched`
// returns true for it on EVERY rank clue (src/basics/variant.cpp:230). The
// rank classification used to scan `variant->touch_possibilities`, which for
// a rank-N clue therefore contained the omni suit at ranks 1-5. A single
// useful-but-unplayable omni rank set `playable_rank = false`, so priority 1
// (direct play) essentially never fired in these variants and every rank clue
// fell through to the referential discard at the bottom of the ladder.
//
// It now classifies over what the TOUCHED CARDS can be, after applying the
// pink promise. Both steps are load-bearing: the promise is what removes the
// omni suit's other ranks (nothing else eliminates them early in a game), and
// the per-card empathy is what removes an omni card of the clued rank that is
// already visible elsewhere.
//
// Replay 1942517 #1 is the direct source of the first test: at all-zero
// stacks a rank-1 clue was read as a referential discard, CTD-ing a card and
// (via the pink promise inside ref_discard) narrowing another to the 1s, so
// it looked like "a playable 1 AND a referential discard".
#include <gtest/gtest.h>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// 3 players, "Extremely Ambiguous & Dark Omni (6 Suits)" — the variant both
// bug reports came from. Suits are Ice/Aqua/Sky/Berry/Navy EA + Dark Omni
// with shorts i/a/s/b/n/o respectively (pick_short falls back to the first
// letter for the EA suits, and Dark Omni's catalogue abbreviation is "O").
// Note scripts/show_turn.py prints a hardcoded r/y/g/b/p/i instead, so log
// output does NOT show these letters.
SetupOptions omni_opts() {
  SetupOptions opts;
  opts.variant_name = "Extremely Ambiguous & Dark Omni (6 Suits)";
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return opts;
}

}  // namespace

// 1.2: at all-zero stacks every actual 1 is playable, so a rank-1 clue is a
// direct play clue — terminal. It must NOT also call a discard.
TEST(Reactor0StableRankOmni, RankOneIsADirectPlayClueNotAReferentialDiscard) {
  SetupOptions opts = omni_opts();
  opts.play_stacks = {0, 0, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"i3", "a1", "s3", "b3", "n3"},  // Bob: exactly one 1, on slot 2
      {"s2", "b2", "n2", "i2", "a2"},
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY)
      << "every 1 is playable at zero stacks — this is a direct play clue";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY)
      << "the touched 1 is called to play";
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_DISCARD))
      << "a direct play clue is terminal — it must not ALSO be a referential "
         "discard (bug_report_1.txt 1.2)";
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CHOP_MOVED))
      << "nor a lock";
}

// 1.3's shape: the clued rank is not playable across the board, but every
// useful identity the touched card can actually hold IS. Here Sky is at 3, so
// Sky 4 is playable, while Ice/Aqua/Berry/Navy 4 are trash at stacks of 5 and
// the Dark Omni 4 is visible in another hand.
TEST(Reactor0StableRankOmni, RankFourIsADirectPlayWhenOnlyUsefulOneIsPlayable) {
  SetupOptions opts = omni_opts();
  opts.play_stacks = {5, 5, 3, 5, 5, 2};
  opts.hands = {
      {"o4", "xx", "xx", "xx", "xx"},  // Alice visibly holds the Dark Omni 4
      {"s4", "i1", "a1", "b1", "n1"},  // Bob: the touched 4 is Sky 4
      {"i3", "a3", "b3", "n3", "s1"},
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY)
      << "Sky 4 is the only useful 4 the card can be, and it is playable";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY);
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_DISCARD));
}

// The negative that keeps the rule honest: when a useful identity the touched
// card can hold is genuinely NOT playable, priority 1 must stay silent and
// the clue must fall through to the referential readings.
TEST(Reactor0StableRankOmni, UnplayableUsefulIdentityStillBlocksDirectPlay) {
  SetupOptions opts = omni_opts();
  opts.play_stacks = {0, 0, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"i3", "a3", "s3", "b3", "n1"},  // Bob: 3s, none playable at zero stacks
      {"s2", "b2", "n2", "i2", "a2"},
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  EXPECT_NE(last_clue_interp(g), ClueInterp::PLAY)
      << "no 3 is playable at zero stacks, so this cannot be a direct play";
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
}

// The pinkish focus is the LEFTMOST newly-touched card. `playable_rank_focus`
// used to return the oldest (rightmost) because it took the minimum order,
// while slot 1 is the newest and carries the highest order.
TEST(Reactor0StableRankOmni, PinkishFocusIsTheLeftmostTouchedCard) {
  SetupOptions opts = omni_opts();
  opts.play_stacks = {0, 0, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"i1", "a3", "s3", "b1", "n3"},  // two 1s: slots 1 and 4
      {"s2", "b2", "n2", "i2", "a2"},
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  ASSERT_EQ(last_clue_interp(g), ClueInterp::PLAY);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY)
      << "slot 1 is leftmost and must be the one called";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_PLAY)
      << "slot 4 is the rightmost touched 1 — calling it was the bug";
}
