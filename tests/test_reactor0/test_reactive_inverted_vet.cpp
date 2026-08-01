// bug_report_4.txt 4.1 — the react-slot vet must follow the inverted swap.
//
// Both reactor0 reactive paths swap the reacter's action when the receiver's
// target is on an inverted (Orange / Dark Orange) suit, so that the receiver's
// standard reading of (clue kind + reacter action) still lands on the
// physically correct button:
//
//   rank Phase A:   target_play  on the reacter -> target_discard when inverted
//   colour mode 1:  target_discard             -> target_play    when inverted
//
// The vetting used to ask about the UN-swapped call in both. Vetting a discard
// call for playability threw away good clues (4.1 proper, replay 1942777 #10);
// vetting a play call for criticality let a blind play through with no
// playability check at all, which strikes. `vet_react_slot` now takes a
// `reacter_plays` flag that the call sites derive from the same
// `target_is_inverted` test that drives the swap.
//
// Each test below pairs the swapped case with the un-swapped control, so a
// regression that "fixes" one by breaking the other cannot pass.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// "Orange (4 Suits)" — Red(r), Green(g), Blue(b), Orange(o). Colour values are
// Red=1, Green=3, Blue=4 by the fixed table, and Orange takes the first
// untaken of {2,5,4,3,1} = 2.
SetupOptions orange_opts() {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  use_reactor0(opts);
  return opts;
}

}  // namespace

// --- rank Phase A ---------------------------------------------------------

// The receiver's only playable is an orange, so Phase A will call the reacter
// to DISCARD. The react slot is a known 4 — it cannot possibly be playable, so
// the old playability vet skipped this target, Phase A found nothing, and the
// clue collapsed into Phase C's lock. A discard call does not care whether the
// card is playable, only that it is safe to throw away.
TEST(Reactor0ReactiveInvertedVet, RankPhaseAVetsTheSwappedDiscardNotAPlay) {
  SetupOptions opts = orange_opts();
  opts.play_stacks = {0, 0, 0, 1};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "g3", "b3", "g4", "r5"},  // Bob (reacter): slot 4 becomes a known 4
      {"r3", "g3", "o2", "b3", "r4"},  // Cathy: o2 at slot 3 is the ONLY playable
  };
  Game g = setup(std::move(opts));
  // Slot 4 pinned to the 4s: none is playable at these stacks, and none is
  // critical, so it is a legal discard but an illegal play.
  g = pre_clue(std::move(g), TestPlayer::BOB, 4, {"4"});

  // Rank 2 -> anchor 2. Target slot 3 -> react slot (2+5-3)%5 = 4.
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_DISCARD)
      << "the swap calls the reacter to discard, so the vet must be the "
         "discard vet (bug_report_4.txt 4.1)";
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 4));
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_DISCARD)
      << "an orange play target is stamped CTD — Discard is the button that "
         "advances its stack";
  EXPECT_FALSE(any_status(g, TestPlayer::CATHY, CardStatus::CHOP_MOVED))
      << "and it is not the Phase C lock the old vet fell through to";
}

// Control: with a NON-inverted target the reacter is called to play, so the
// playability vet still governs and an unplayable react slot still retargets.
TEST(Reactor0ReactiveInvertedVet, RankPhaseAKeepsThePlayVetWhenNotSwapped) {
  SetupOptions opts = orange_opts();
  opts.play_stacks = {0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "g3", "b4", "g4", "r5"},  // Bob: slot 3 becomes a known 4
      {"r3", "g3", "b1", "g4", "r4"},  // Cathy: b1 at slot 3 is the ONLY playable
  };
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"4"});

  // Rank 1 -> anchor 1. Target slot 3 -> react slot (1+5-3)%5 = 3.
  g = take_turn(std::move(g), "Alice clues 1 to Cathy");

  EXPECT_NE(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_PLAY)
      << "a blind play still requires the react slot to be possibly playable";
  EXPECT_NE(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_PLAY)
      << "so Phase A must not have fired on this target";
}

// Control: the giver-only reject on the play side survives the refactor. The
// react slot is unclued, so shared knowledge cannot rule it out — but the
// giver can see it is an unplayable 5, and the reacter would blind-play it and
// strike. Reject the clue rather than retarget (§1g).
TEST(Reactor0ReactiveInvertedVet, RankPhaseAStillRejectsOnGiverOnlyKnowledge) {
  SetupOptions opts = orange_opts();
  opts.play_stacks = {0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "g3", "r5", "g4", "b5"},  // Bob: slot 3 = r5, visibly unplayable
      {"r3", "g3", "b1", "g4", "r4"},  // Cathy: b1 at slot 3 is the ONLY playable
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 1 to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::MISTAKE)
      << "giver-only knowledge rejects the clue, it never retargets";
}

// --- colour mode 1 --------------------------------------------------------

// The mirror defect. Mode 1 normally makes the reacter discard, but for an
// inverted target it calls `target_play` — a blind play. It used to check only
// that the react slot was not all-critical, so it would happily blind-play a
// known 4. It must now retarget to the next leftmost playable.
TEST(Reactor0ReactiveInvertedVet, ColourModeOneVetsTheSwappedPlayNotADiscard) {
  SetupOptions opts = orange_opts();
  opts.play_stacks = {0, 0, 0, 1};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "g4", "b3", "r5", "g2"},  // Bob: slot 2 becomes a known 4
      {"o2", "g3", "r1", "b3", "g4"},  // Cathy: o2 (slot 1) and r1 (slot 3)
  };
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 2, {"4"});

  // Green -> anchor 3. The leftmost playable is the orange at slot 1, whose
  // react slot is (3+5-1)%5 = 2 — and for an inverted target that slot would
  // be BLIND-PLAYED, which a known 4 cannot survive. Retarget to r1 at slot 3,
  // react slot (3+5-3)%5 = 5, a plain discard.
  g = take_turn(std::move(g), "Alice clues green to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::NONE)
      << "the react slot for the orange target cannot be blind-played";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_DISCARD);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_PLAY);
  EXPECT_NE(status_at(g, TestPlayer::CATHY, 1), CardStatus::CALLED_TO_DISCARD)
      << "the orange target was abandoned, so it must carry no chuck call";
}

// Control: with a NON-inverted target mode 1 still makes the reacter discard,
// so a react slot that cannot possibly be playable is perfectly fine. This is
// the assertion that fails if the play vet is applied unconditionally.
TEST(Reactor0ReactiveInvertedVet, ColourModeOneKeepsTheDiscardVetWhenNotSwapped) {
  SetupOptions opts = orange_opts();
  opts.play_stacks = {0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "g3", "b3", "r5", "g4"},  // Bob: slot 5 becomes a known 4
      {"r3", "g3", "b1", "b3", "r4"},  // Cathy: b1 at slot 3 is the ONLY playable
  };
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 5, {"4"});

  // Green -> anchor 3. Target slot 3 -> react slot (3+5-3)%5 = 5.
  g = take_turn(std::move(g), "Alice clues green to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_DISCARD)
      << "a discard call does not require the react slot to be playable";
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_PLAY);
}
