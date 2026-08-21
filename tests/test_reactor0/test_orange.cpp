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

#include <cstdio>
#include <string>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/calls.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
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
// the discard pile.
//
// Until v7.19.0 that made the whole reading decline, and the clue fell through
// to the referential discard and locked. It no longer does: o1 is dropped from
// the INFERENCES instead of killing the reading, so the focus is stamped CTP
// and promised {r2, b2}. Declining cost replay 1966696 an entire clue — a
// rank-1 on {r1,b1,o1} read as a bare REVEAL, and will-bot67 carried an
// unstamped playable 1 for six turns before discarding its chop on T8.
//
// The original complaint in bug_report_3.txt 3.1 is still honoured: the card
// must not be stamped CTD, which is what `order_trash` reported as known trash.
TEST(Reactor0Orange, RankDirectPlayPitchesTheMixedSetPlainly) {
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

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY)
      << "a mixed useful set is a direct play clue: the button is Play, and "
         "the inferences narrow to the identities that button advances";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_PLAY)
      << "CTP is the Play button — a pitch";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_DISCARD)
      << "bug_report_3.txt 3.1's actual complaint still holds: the lock slot "
         "must not be stamped CTD, since order_trash reports a CTD as known "
         "trash and that was the '[kt]' the report saw";
  // o1 is dropped: pressing Play on an inverted card discards it, so an
  // inverted identity is never what a pitch call means.
  expect_infs(g, TestPlayer::BOB, TestPlayer::BOB, 5, {"r2", "b2"});
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
  // instead: the card is pinned to Orange 1, and src/basics/decide.cpp:889-908
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

// §1g on the STABLE side. The chuck target is chosen from common knowledge, so
// the receiver computes the same card whatever the giver can see. If the giver
// can see that card is not currently playable, the receiver's chuck is a
// misplay strike — so the clue must be REJECTED, not quietly retargeted to the
// next orange (replay 1957905 #31, bug_report_4_1_0.txt).
TEST(Reactor0Orange, ColourChuckRejectsAnUnplayableOrangeTheGiverCanSee) {
  SetupOptions opts = orange_opts("Orange (3 Suits)");
  // Orange sits at 1, so Bob's slot 1 (o1) is already on the stacks and a
  // chuck of it strikes. Bob cannot see that: his slot 1 could still be the
  // playable o2, so `could_reach_stacks` is true and he will chuck it.
  opts.play_stacks = {0, 0, 1};
  opts.hands = {
      {"r4", "b4", "r5", "b5", "r3"},
      {"o1", "o2", "r3", "b3", "r2"},
      {"r2", "b2", "b3", "r1", "b1"},
  };
  Game g = setup(std::move(opts));
  ASSERT_LE(g.state.pace(), 3) << "fixture must land in the chuck branch";

  auto before = hand_marks(g, TestPlayer::BOB);
  g = take_turn(std::move(g), "Alice clues orange to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::MISTAKE)
      << "giver-only knowledge must reject the clue; walking on to slot 2 "
         "would desync, because Bob still computes slot 1";
  EXPECT_TRUE(newly_marked(before, g, TestPlayer::BOB).empty())
      << "a rejected clue must leave the receiver unstamped";
}

// The convention bug_report_4_1_0.txt asks for: when every other suit's copy of
// the clued rank is already on the stacks, the rank names the orange one and
// nothing else. The button is then unambiguous, so the clue is a direct play
// clue actioned as a CHUCK rather than the usual pitch.
//
// Stacks [2, 2, 1]: r2 and b2 are basic trash, o2 is the rank's only useful
// identity and it is playable.
TEST(Reactor0Orange, RankDirectPlayChucksWhenEveryUsefulIdentityIsOrange) {
  SetupOptions opts = orange_opts("Orange (3 Suits)");
  opts.play_stacks = {2, 2, 1};
  opts.hands = {
      {"r4", "b4", "r5", "b5", "r3"},
      // Bob's only rank-2 card is the orange one, so the clue touches slot 1
      // alone.
      {"o2", "r3", "b3", "r4", "b4"},
      {"r1", "b1", "o4", "o5", "r3"},
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY)
      << "the rank's only useful identity is orange, so this is a direct play "
         "clue after all";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_DISCARD)
      << "CTD is the Discard button, which for an orange card is the chuck "
         "that advances its stack";
  expect_infs(g, TestPlayer::BOB, TestPlayer::BOB, 1, {"o2"});
}

// The companion to the orange-only chuck above: a MIXED useful set pitches.
// Stacks [1, 1, 1] leave r2, b2 AND o2 useful and playable. The button is not
// undecidable — the default for a rank direct play clue is Play, so the focus
// is stamped CTP and its inferences narrow to the identities a pitch actually
// advances, {r2, b2}.
//
// Bob's slot 1 really is an Orange 2 here, so acting on this call would pitch
// it into the discard pile. That is a GIVER-side error rather than a flaw in
// the reading: Alice can see Bob's hand and would not give this clue. The
// fixture keeps the orange focus deliberately, to make the hazard concrete.
TEST(Reactor0Orange, RankDirectPlayPitchesAMixedUsefulSet) {
  SetupOptions opts = orange_opts("Orange (3 Suits)");
  opts.play_stacks = {1, 1, 1};
  opts.hands = {
      {"r4", "b4", "r5", "b5", "r3"},
      {"o2", "r3", "b3", "r4", "b4"},
      {"r1", "b1", "o4", "o5", "r3"},
  };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::PLAY)
      << "a mixed useful set is a direct play clue, not a decline";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_PLAY)
      << "the default button for a rank direct play clue is Play";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 1), CardStatus::CALLED_TO_DISCARD);
  // o2 is dropped from the inferences — a pitch cannot advance it.
  expect_infs(g, TestPlayer::BOB, TestPlayer::BOB, 1, {"r2", "b2"});
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

// --- bug_report_6_2_0.txt: a chuck call must survive its own resolution -----

// A CTD is the PHYSICAL label "press Discard", so on an inverted suit it is a
// CHUCK — a play call. `target_i_discard`, which resolves the receiver's target
// when the reacter acts, narrowed `inferred` to the NON-CRITICAL identities:
// the "which of these can you afford to lose?" reading. Dark Orange is
// `oneOfEach`, so every identity is critical and the set emptied. The card was
// then marked `meta.trash`, and `Game::elim`'s step-1 sweep reset it to global
// empathy and cleared the status — the chuck signal destroyed one turn after it
// was given (replay 1959065 T5-T6).
//
// The clue has to TOUCH the target for common to know the suit, which is what
// the replay does: Alice clues Dark Orange to Cathy, touching her o1 on slot 2.
// Dark Orange's colour value is 5, so target_slot 2 -> react_slot
// calc_slot(5,2,5) = 3, and the inverted target swaps the reacter to a play —
// Bob's slot 3 is a playable r1. Bob playing it runs the resolution.
TEST(Reactor0Orange, DarkOrangeChuckCallSurvivesReactionResolution) {
  SetupOptions opts = orange_opts("Dark Orange (5 Suits)");
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.hands = {
      {"y5", "g5", "b5", "r5", "y4"},
      {"y3", "g3", "r1", "b3", "y4"},  // Bob (reacter): slot 3 = r1, playable
      {"r3", "o1", "g4", "b4", "y4"},  // Cathy: o1 on slot 2 is her only playable
  };
  Game g = setup(std::move(opts));
  const int dark_orange = suit_index_of(g, "Dark Orange");

  g = clue_colour(std::move(g), TestPlayer::ALICE, TestPlayer::CATHY, dark_orange);
  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::CALLED_TO_DISCARD)
      << "an inverted play target is stamped CTD — Discard is the button that "
         "advances its stack";

  // The reacter acts: this is the resolution that used to destroy the call.
  g = take_turn(std::move(g), "Bob plays r1 (slot 3)", "y2");

  const int target = order_at(g, TestPlayer::CATHY, 2);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::CALLED_TO_DISCARD)
      << "the chuck call must survive the resolution";
  EXPECT_FALSE(g.meta[target].trash)
      << "'every identity is critical' means the card is irreplaceable, not "
         "that it is trash";
  // Narrowed (in common) to what a chuck would actually stack.
  expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"o1"});
}

// The control, and the reason the branch is scoped to "the ordinary reading
// came out empty" rather than to "the target is orange": in NON-dark Orange the
// o1 has spare copies, so it is not critical, the ordinary non-critical
// narrowing yields something, and the old reading is kept untouched. Only the
// self-destructing case changes.
TEST(Reactor0Orange, NonDarkOrangeTargetKeepsTheOrdinaryDiscardReading) {
  SetupOptions opts = orange_opts("Orange (5 Suits)");
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.hands = {
      {"y5", "g5", "b5", "r5", "y4"},
      {"y3", "g3", "r1", "b3", "y4"},
      {"r3", "o1", "g4", "b4", "y4"},
  };
  Game g = setup(std::move(opts));
  const int orange = suit_index_of(g, "Orange");

  g = clue_colour(std::move(g), TestPlayer::ALICE, TestPlayer::CATHY, orange);
  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);

  g = take_turn(std::move(g), "Bob plays r1 (slot 3)", "y2");

  const int target = order_at(g, TestPlayer::CATHY, 2);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 2), CardStatus::CALLED_TO_DISCARD);
  EXPECT_FALSE(g.meta[target].trash)
      << "the ordinary narrowing found non-critical ids, so nothing is trash";
  EXPECT_FALSE(g.common.thoughts[target].info_lock.has_value())
      << "the chuck branch pins with info_lock; the ordinary reading does not, "
         "so this proves the fix did not broaden to every orange";
}

// --- §1b: the ladder is for clues that NAME orange ------------------------

// Replay 1966119 T1. "Rainbow-Ones & Orange (3 Suits)": every colour clue
// touches every 1, so a BLUE clue touches the holder's Blue 1 AND their Orange
// 1 AND their Red 1.
//
// The orange ladder selects on `orange_touched`, which is "could this card be
// orange" read off `possible` — and under Rainbow-Ones that is true of every
// card a blue clue touches. So the ladder claimed a Blue 1, chucked it, and
// narrowed its inference to exactly {Orange 1}: a card the holder does not
// hold, and one that pressing Discard on would have thrown away rather than
// played.
//
// The spec always said the ladder is for "a colour clue naming the inverted
// suit". Nothing tested that until now.
TEST(Reactor0Orange, ColourClueNotNamingOrangeSkipsTheLadder) {
  SetupOptions opts;
  opts.variant_name = "Rainbow-Ones & Orange (3 Suits)";
  // Seats relabelled so the receiver is index 0. Their slot 1 is Blue 1.
  opts.hands = {
      {"b1", "o1", "r1", "b5", "o4"},
      {"o3", "b4", "o2", "r2", "b2"},
      {"b2", "o4", "r3", "o2", "r5"},
  };
  opts.play_stacks = {0, 0, 0};
  opts.starting = TestPlayer::CATHY;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int orange = 2;
  ASSERT_TRUE(g.state.variant->suits[orange].suit_type.inverted)
      << "guard: suit 2 is the inverted one";

  // Blue touches slots 1-4: the two real blues, plus the rainbow Red 1 and
  // Orange 1.
  g = take_turn(std::move(g), "Cathy clues blue to Alice (slot 1,2,3,4)");

  const int slot1 = order_at(g, TestPlayer::ALICE, 1);
  EXPECT_EQ(status_at(g, TestPlayer::ALICE, 1), CardStatus::CALLED_TO_PLAY)
      << "a blue clue does not name orange, so this is an ordinary play call "
         "(a PITCH), not the ladder's chuck";
  EXPECT_NE(status_at(g, TestPlayer::ALICE, 1), CardStatus::CALLED_TO_DISCARD);

  // The load-bearing assertion: the holder must not come away believing they
  // hold a card they do not. Blue 1 is what they actually have.
  const IdentitySet& inf = g.common.thoughts[slot1].inferred;
  EXPECT_TRUE(inf.contains(Identity{1, 1}))
      << "Blue 1 is the true card and must survive the narrowing";
  int n = 0;
  for (Identity i : inf) {
    (void)i;
    ++n;
  }
  EXPECT_GT(n, 1)
      << "narrowing to a single identity here is the bug: it pinned Orange 1";
  EXPECT_FALSE(inf.is_exactly(Identity{orange, 1}))
      << "the holder must not be told their Blue 1 is an Orange 1";
}

// The candidate sets a stamped button admits (DECISION_MAKING.md, "the stamp
// is the instruction"). A call's inferred set is built to match its button, so
// there is never a case where the stamp is right but the button is wrong.
//
// Both fixtures below are the worked examples from the two replays that forced
// this rule out into the open.

// PITCH. Replay 1966286's position: Rainbow-Ones & Orange, stacks r1/b1/o1.
// A card stamped CTP, if completely untouched, admits {r2, b2, o1, o3, o4} --
// the playable plain cards, plus the oranges that are NOT playable and are
// affordable to throw away. o2 is excluded because it is the playable orange
// (Play would pitch away the card its stack is waiting for) and o5 because it
// is critical.
TEST(Reactor0Orange, PitchCandidatesMatchTheWorkedExample) {
  SetupOptions opts;
  opts.variant_name = "Rainbow-Ones & Orange (3 Suits)";
  opts.hands = {
      {"r3", "b3", "r4", "b4", "r5"},
      {"o2", "o3", "r5", "r2", "b2"},
      {"o4", "o5", "b5", "r3", "b3"},
  };
  opts.play_stacks = {1, 1, 1};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  IdentitySet got = hanabi::reactor0::pitch_candidates(g.state);
  IdentitySet want = IdentitySet::from_iter({
      Identity{0, 2},  // r2, playable plain
      Identity{1, 2},  // b2, playable plain
      Identity{2, 1},  // o1, orange past its stack
      Identity{2, 3},  // o3, orange not yet playable
      Identity{2, 4},  // o4, likewise
  });
  EXPECT_EQ(got, want)
      << "a pitch call admits playable plain cards and spare oranges only";
}

// CHUCK. Replay 1966569's position: Muddy Rainbow & Orange, stacks r1/m0/o3.
// A card stamped CTD admits {r1, r3, r4, m2, m3, m4, o4} -- the plain cards
// that are neither playable nor critical, plus the one orange that IS playable,
// since Discard puts an orange on its stack.
TEST(Reactor0Orange, ChuckCandidatesMatchTheWorkedExample) {
  SetupOptions opts;
  opts.variant_name = "Muddy Rainbow & Orange (3 Suits)";
  opts.hands = {
      {"r2", "m1", "r3", "m2", "r4"},
      {"o1", "o2", "m3", "m4", "r5"},
      {"o4", "o5", "m5", "r3", "m2"},
  };
  opts.play_stacks = {1, 0, 3};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  IdentitySet got = hanabi::reactor0::chuck_candidates(g.state);
  IdentitySet want = IdentitySet::from_iter({
      Identity{0, 1}, Identity{0, 3}, Identity{0, 4},  // r1 r3 r4
      Identity{1, 2}, Identity{1, 3}, Identity{1, 4},  // m2 m3 m4
      Identity{2, 4},                                  // o4, the playable orange
  });
  EXPECT_EQ(got, want)
      << "a chuck call admits unplayable non-critical plain cards, and the "
         "orange the stack is waiting for";
}

// Replay 1966569 T10. The chuck list takes "trash NON-INVERTED or playable
// inverted", and the non-inverted half of that first arm is load-bearing.
//
// A card that is trash but could be ORANGE is not safe to chuck: pressing
// Discard on an inverted card is a play attempt, and a trash orange is by
// definition not playable, so the chuck STRIKES. will-bot67 held {r1, o1} with
// the red stack at 1 and the orange stack at 3 -- both identities trash -- and
// chucked it into a strike. Such a card is pitched, never chucked.
TEST(Reactor0Orange, TrashThatCouldBeOrangeIsNotChuckable) {
  SetupOptions opts;
  opts.variant_name = "Muddy Rainbow & Orange (3 Suits)";
  opts.hands = {
      {"r1", "m4", "m5", "r4", "r5"},
      {"o5", "m2", "m3", "r3", "o2"},
      {"m1", "r2", "o4", "m2", "r4"},
  };
  opts.play_stacks = {1, 0, 3};  // r1 trash, o1 trash (orange is past it)
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int orange = 2;
  ASSERT_TRUE(g.state.variant->suits[orange].suit_type.inverted);
  ASSERT_TRUE(g.state.is_basic_trash(Identity{0, 1})) << "guard: r1 is trash";
  ASSERT_TRUE(g.state.is_basic_trash(Identity{orange, 1}))
      << "guard: o1 is trash, the orange stack is past it";
  ASSERT_FALSE(g.state.is_playable(Identity{orange, 1}))
      << "guard: so chucking it is a misplay, not a play";

  // Alice's slot 1, pinned to {r1, o1} -- every identity trash, one of them
  // orange.
  const int o = order_at(g, TestPlayer::ALICE, 1);
  g.with_thought(o, [](const Thought& t) {
    Thought out = t;
    out.inferred =
        IdentitySet::from_iter({Identity{0, 1}, Identity{2, 1}});
    return out;
  });

  auto lists = hanabi::reactor0::action_lists(g, 0);
  EXPECT_EQ(std::find(lists.chuck.begin(), lists.chuck.end(), o),
            lists.chuck.end())
      << "all-trash is not enough: if it could be orange, Discard is a play "
         "attempt and the chuck strikes";
}
