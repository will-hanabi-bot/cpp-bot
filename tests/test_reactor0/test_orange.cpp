// reactor0's readings for inverted (Orange / Dark Orange) suits.
//
// Vocabulary, from reactor's GLOSSARY "pitch / chuck": a **pitch** presses the
// Play button, a **chuck** presses Discard. For an inverted suit the game
// swaps their outcomes — a chuck advances the orange stack, a pitch sends the
// card to the discard pile and regains a clue. CardStatus names the BUTTON, so
// CALLED_TO_PLAY == pitch and CALLED_TO_DISCARD == chuck.
//
// The rules under test (reactor0 CONVENTION.md §1b/§1c):
//
//   * A rank DIRECT PLAY clue always means pitch. Pitching an orange discards
//     it, so priority 1 can never put an orange card onto its stack — which is
//     why a rank-1 clue cannot be used to play an orange 1. Priorities 2 and 3
//     (play reveal / trash reveal) are untouched and still pick a playable
//     orange up.
//   * An orange COLOUR clue that reveals a playable orange is a play reveal:
//     the receiver chucks the revealed card, and that reading takes priority.
//   * Otherwise, non-dark orange at pace > 3 pitches the leftmost touched
//     orange the receiver does not know is critical; at pace <= 3, or in Dark
//     Orange, it chucks the leftmost touched orange that could still reach the
//     stacks; if none could, it stalls.
//
// bug_report_3.txt 3.1 (replay 1942709 #4) is the first test: a rank-2 clue on
// the lock slot was read as a direct play call and stamped CTD, which
// `Player::order_trash` then reports as known trash.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

SetupOptions orange_opts(std::string variant) {
  SetupOptions opts;
  opts.variant_name = std::move(variant);
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return opts;
}

int suit_index_of(const Game& g, std::string_view name) {
  for (size_t i = 0; i < g.state.variant->suits.size(); ++i) {
    if (g.state.variant->suits[i].name == name) return static_cast<int>(i);
  }
  throw std::invalid_argument("suit not in variant");
}

// `take_turn`'s "<who> clues <colour> to <who>" form cannot name a two-word
// suit — its regex takes a single `\w+` token and `str_to_clue` matches the
// full suit name, so "Dark Orange" is unreachable from a string. Apply the
// ClueAction directly instead; a clue draws nothing, so this is just
// handle_action + the turn bump that take_turn would do.
Game clue_colour(Game g, TestPlayer giver, TestPlayer target, int suit_index) {
  const int gi = static_cast<int>(giver);
  const int ti = static_cast<int>(target);
  const int turn_before = g.state.turn_count;
  auto touched =
      g.state.clue_touched(g.state.hands[ti], ClueKind::COLOUR, suit_index);
  g.catchup = true;
  g.handle_action(ClueAction{gi, ti, std::move(touched),
                             BaseClue(ClueKind::COLOUR, suit_index)});
  g.handle_action(TurnAction{turn_before, g.state.next_player_index(gi)});
  g.catchup = false;
  return g;
}

}  // namespace

// --- 3.1: rank direct play declines orange -------------------------------

// Replay 1942709 #4, "Pink-Ones & Orange (3 Suits)" — Red / Blue / Orange,
// specialRank 1 with specialRankAllClueRanks, so every clue rank ALSO touches
// the 1s and rank-1 clues do not exist. Stacks [1, 1, 0].
//
// Alice clues 2 to Bob, touching only his lock slot. The touched card's
// promise set is {rank 2} u {rank 1}; both copies of Orange 2 sit in Cathy's
// hand, so per-card visibility narrows it to {r1, r2, b1, b2, o1} — exactly
// the `poss` the log recorded. r1/b1 are basic trash at stacks of 1, r2/b2 are
// playable, and o1 is playable *on paper*. It is not playable by this clue,
// because a rank direct play clue pitches, and pitching Orange 1 puts it in
// the discard pile. So priority 1 must not fire, and the clue falls through to
// the referential discard, which locks because the focus is the lock slot.
TEST(Reactor0Orange, RankDirectPlayDeclinesOrangeAndLocksInstead) {
  SetupOptions opts = orange_opts("Pink-Ones & Orange (3 Suits)");
  opts.play_stacks = {1, 1, 0};
  opts.hands = {
      {"r4", "b4", "r5", "b5", "r3"},
      // Bob: no 1s and no other 2s, so the rank-2 clue touches slot 5 alone —
      // the oldest slot, i.e. the lock slot.
      {"r3", "b3", "r4", "b4", "r2"},
      // Cathy holds BOTH Orange 2s, which is what lets Bob eliminate o2 and
      // leaves o1 as the only inverted identity his card could be.
      {"o2", "o2", "b3", "o4", "o5"},
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::LOCK)
      << "priority 1 must decline a promise set whose only inverted member is "
         "useful — a pitch would discard it (bug_report_3.txt 3.1)";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_DISCARD)
      << "the lock slot must not be stamped CTD: order_trash reports a CTD as "
         "known trash, which is the '[kt]' the report saw";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_PLAY)
      << "nor called to play";
}

// The companion negative: (b) is confined to priority 1. A rank clue that
// REVEALS a playable orange still reads as a REVEAL, because a reveal is
// actioned as a chuck rather than a pitch.
//
// Bob's slot 2 is pre-clued orange, so it is already known to be on the
// inverted suit but not yet to be playable. Orange is at 0, and every other
// Orange 1 is visible to Bob (Cathy holds the second copy), so the rank clue
// pins slot 2 to Orange 1 and makes it an obvious playable.
TEST(Reactor0Orange, RankClueStillRevealsAPlayableOrange) {
  SetupOptions opts = orange_opts("Orange (4 Suits)");
  opts.play_stacks = {0, 0, 0, 0};
  opts.hands = {
      {"r4", "g4", "b4", "r5", "g5"},
      {"r3", "o1", "g3", "b3", "r2"},
      {"o1", "g2", "b2", "o4", "o5"},
  };
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 2, {"orange"});

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REVEAL)
      << "priority 2 is untouched: a revealed playable orange is still a "
         "reveal, so declining orange at priority 1 does not strand it";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY)
      << "and it is not turned into a pitch call, which would discard it";
  // A rank reveal stamps nothing — deliberately, since the clarification only
  // moves the ORANGE COLOUR clue's reveal. The chuck comes from empathy
  // instead: the card is pinned to Orange 1, and src/basics/decide.cpp:885-894
  // routes an empathy-pinned playable orange through PerformDiscard.
  expect_poss(g, TestPlayer::BOB, TestPlayer::BOB, 2, {"o1"});
}

// --- the orange colour ladder --------------------------------------------

// Non-dark orange with pace to spare: pitch the leftmost touched orange the
// receiver does not know is critical. "Orange (4 Suits)" opens at pace 8.
TEST(Reactor0Orange, ColourPitchesLeftmostNonCriticalAtHighPace) {
  SetupOptions opts = orange_opts("Orange (4 Suits)");
  opts.play_stacks = {0, 0, 0, 0};
  opts.hands = {
      {"r4", "g4", "b4", "r5", "g5"},
      {"o3", "o4", "r1", "g1", "b1"},
      {"r2", "g2", "b2", "r3", "g3"},
  };
  Game g = setup(std::move(opts));
  ASSERT_GT(g.state.pace(), 3);

  g = take_turn(std::move(g), "Alice clues orange to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::DISCARD)
      << "a pitch throws the card away — the semantic outcome is a discard";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY)
      << "CTP is the Play button, which for an orange card is the pitch";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY)
      << "one card is named, and it is the leftmost";
}

// Same variant family, but pace <= 3: chuck instead, putting the card on the
// stacks. "Orange (3 Suits)" opens at exactly pace 3.
TEST(Reactor0Orange, ColourChucksLeftmostAtLowPace) {
  SetupOptions opts = orange_opts("Orange (3 Suits)");
  opts.play_stacks = {0, 0, 0};
  opts.hands = {
      {"r4", "b4", "r5", "b5", "r3"},
      {"o1", "o3", "r3", "b3", "r2"},
      {"r2", "b2", "b3", "o4", "o5"},
  };
  Game g = setup(std::move(opts));
  ASSERT_LE(g.state.pace(), 3);

  g = take_turn(std::move(g), "Alice clues orange to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY)
      << "a chuck advances the orange stack";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_DISCARD)
      << "CTD is the Discard button, which for an orange card is the chuck";
}

// Dark Orange chucks at any pace. Every dark card is a singleton
// (src/basics/variant.cpp:183), so pitching one is an unrecoverable loss —
// and it also means `holder_knows_critical` is true for every candidate, so
// the pitch branch would find nothing to name even if it ran.
TEST(Reactor0Orange, ColourChucksInDarkOrangeEvenAtHighPace) {
  SetupOptions opts = orange_opts("Dark Orange (5 Suits)");
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.hands = {
      {"r4", "y4", "g4", "b4", "r5"},
      {"o1", "r3", "y3", "g3", "b3"},
      {"r2", "y2", "g2", "b2", "r1"},
  };
  Game g = setup(std::move(opts));
  ASSERT_GT(g.state.pace(), 3);
  const int orange = suit_index_of(g, "Dark Orange");

  g = clue_colour(std::move(g), TestPlayer::ALICE, TestPlayer::BOB, orange);

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY)
      << "dark forces the chuck reading regardless of pace";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_DISCARD);
}

// The play reveal outranks the ladder. Pace is 8 and the suit is not dark, so
// the pitch branch would otherwise name slot 1; because the clue reveals a
// playable orange on slot 2, that card is chucked instead and slot 1 is left
// alone.
TEST(Reactor0Orange, ColourPlayRevealOutranksThePitch) {
  SetupOptions opts = orange_opts("Orange (4 Suits)");
  opts.play_stacks = {0, 0, 0, 0};
  opts.hands = {
      {"r4", "g4", "b4", "r5", "g5"},
      {"o4", "o1", "r3", "g3", "b3"},
      {"o1", "r2", "g2", "b2", "r1"},
  };
  Game g = setup(std::move(opts));
  // Rank-1 knowledge on slot 2 without colour: the orange clue is what pins it
  // to Orange 1 and reveals the playable.
  g = pre_clue(std::move(g), TestPlayer::BOB, 2, {"1"});
  ASSERT_GT(g.state.pace(), 3);

  g = take_turn(std::move(g), "Alice clues orange to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REVEAL)
      << "a revealed playable orange is a play reveal and takes priority";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_DISCARD)
      << "the revealed card is chucked onto the stack";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY)
      << "the pitch branch must not also fire on the leftmost orange";
}

// Nothing to pitch and nothing to chuck: the clue is a stall. Orange sits at
// 2 and both touched cards are already pinned to ranks above the playable one,
// so no chuck could reach the stacks; pace is 3, so the pitch branch is off.
TEST(Reactor0Orange, ColourStallsWhenNoOrangeCanReachTheStacks) {
  SetupOptions opts = orange_opts("Orange (3 Suits)");
  opts.play_stacks = {0, 0, 2};
  opts.hands = {
      {"r4", "b4", "r5", "b5", "r3"},
      {"o5", "o4", "r3", "b3", "r2"},
      {"r2", "b2", "b3", "r1", "b1"},
  };
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"5"});
  g = pre_clue(std::move(g), TestPlayer::BOB, 2, {"4"});
  ASSERT_LE(g.state.pace(), 3);

  g = take_turn(std::move(g), "Alice clues orange to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::STALL)
      << "no touched orange could reach the stacks and the pitch branch is off";
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_DISCARD));
}
