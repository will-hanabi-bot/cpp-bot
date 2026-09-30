// Deferred collapse on a re-touched clued card (tiiah/CONVENTION.md §1e, v18.12.0).
//
// The reviewer's rule, human diagnostic 2014076 T14: a COLOUR clue that singles out
// an already-clued card whose identity one of the hole cards could be -- playable
// in some worlds, trash in others -- is not yet evidence that the team still needs
// it. The collapse waits for the holder: a discard says the card was already
// played (rule 7's shared form), a play is evidence by itself.
#include <gtest/gtest.h>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

constexpr Identity kR1{0, 1}, kR2{0, 2}, kY1{1, 1};

IdentitySet r1_or_r2() { return IdentitySet::empty().add(kR1).add(kR2); }
IdentitySet r1_or_y1() { return IdentitySet::empty().add(kR1).add(kY1); }

// We threw a `{r1,y1}` into the hole and then a `{r1,r2}` -- 2014076's shape: the
// worlds are (r1, r2), where red is on 2, and (y1, r1), where red is on 1. Cathy
// holds an r2 (slot 2), touched by `first` if one is given: trash in the first world,
// playable in the second. It is Bob's turn, and a clue from him to Cathy, his Bob,
// is stable.
struct Position {
  Game g;
  int first_hole;
  int ours;
  int cathys_r2;
};

Position position(const char* first = nullptr) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"y4", "g4", "b4", "p4", "r4"},
                {"y3", "r2", "g3", "b3", "p5"}};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 6;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  if (first) g = pre_clue(std::move(g), TestPlayer::CATHY, /*slot=*/2, {first});
  const int first_hole = order_at(g, TestPlayer::ALICE, 1);
  const int cathys_r2 = order_at(g, TestPlayer::CATHY, 2);
  g = hidden_action(std::move(g), TestPlayer::ALICE, 1, /*reached_the_hole=*/true);
  // The whole table reads these sets: nothing was settled privately.
  g.with_meta(first_hole, [](ConvData& m) {
    m.superposition = r1_or_y1();
    m.shared_left = IdentitySet::empty();
  });
  const int ours = order_at(g, TestPlayer::ALICE, 1);
  g = hidden_action(std::move(g), TestPlayer::ALICE, 1, /*reached_the_hole=*/true);
  g.with_meta(ours, [](ConvData& m) {
    m.superposition = r1_or_r2();
    m.shared_left = IdentitySet::empty();
  });
  return {std::move(g), first_hole, ours, cathys_r2};
}

}  // namespace

// A Red re-touching Cathy's 2-clued r2: our hole card stays `{r1,r2}` until she acts.
TEST(TiiahDeferredCollapse, AColourReTouchWaitsForTheHolder) {
  Position p = position("2");
  p.g = take_turn(std::move(p.g), "Bob clues red to Cathy");

  EXPECT_EQ(p.g.meta[p.ours].superposition, r1_or_r2())
      << "the r2 may be the dupe Cathy wants thrown";
  EXPECT_EQ(p.g.meta[p.first_hole].superposition, r1_or_y1());
}

// ...and when Cathy throws it, every seat learns the r2 was already played.
TEST(TiiahDeferredCollapse, TheHoldersDiscardSaysItWasAlreadyPlayed) {
  Position p = position("2");
  p.g = take_turn(std::move(p.g), "Bob clues red to Cathy");
  p.g = hidden_action(std::move(p.g), TestPlayer::CATHY, 2, /*reached_the_hole=*/false);

  EXPECT_FALSE(p.g.meta[p.ours].superposed()) << "settled: it was the r2";
  EXPECT_EQ(p.g.state.play_stacks[0], 2) << "red on 2";
  EXPECT_EQ(p.g.state.play_stacks[1], 0) << "so the first was the r1, not the y1";
  EXPECT_EQ(p.g.state.common_play_stacks[0], 2) << "the shared view has red on 2";
}

// The control, the reviewer's "cluedness is important": the same Red on an r2 NOT
// clued before is a call at once, and a call on it is only good where red is on 1
// -- so the first hole card was the y1 and ours the r1.
TEST(TiiahDeferredCollapse, AFirstTouchCollapsesAtOnce) {
  Position p = position();
  p.g = take_turn(std::move(p.g), "Bob clues red to Cathy");

  EXPECT_FALSE(p.g.meta[p.ours].superposed()) << "settled";
  EXPECT_FALSE(p.g.meta[p.first_hole].superposed()) << "settled";
  EXPECT_EQ(p.g.state.play_stacks[0], 1) << "red on 1: ours was the r1";
  EXPECT_EQ(p.g.state.play_stacks[1], 1) << "yellow on 1: the first was the y1";
}
