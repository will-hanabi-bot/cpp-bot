// Behavioral test for the /allplays toggle. Under /allplays off (standard
// Reactor convention), a color reactive clue marks the reacter as
// CALLED_TO_DISCARD (play+dc). Under /allplays on, the color reactive is
// promoted to play+play and the reacter is instead CALLED_TO_PLAY.
#include <gtest/gtest.h>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

SetupOptions baseline_opts() {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer): hidden hand
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob (reacter): slot 5 (calc_slot(5,5,5)=5) must hold an identity
      // that's actually playable — common's view of Bob's slots is the deck
      // identity, so the convention's "possible.exists(playable)" check
      // only passes if the deck identity is a playable. Put g1 there.
      {"y4", "g3", "y3", "g2", "g1"},
      // Cathy (receiver): r1 at slot 5 (the only playable card she holds);
      // the rest are non-playable so a red clue uniquely identifies r1 as
      // the focus.
      {"y3", "g3", "y2", "g2", "r1"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  return opts;
}

bool any_status(const Game& g, TestPlayer player, CardStatus s) {
  int pi = static_cast<int>(player);
  for (int o : g.state.hands[pi]) {
    if (g.meta[o].status == s) return true;
  }
  return false;
}

}  // namespace

TEST(AllPlays, ColourClueMarksReacterDiscardWhenOff) {
  Game g = setup(baseline_opts());
  g.all_plays = false;
  g = take_turn(std::move(g), "Alice clues red to Cathy");

  // /allplays off: color clue is the standard "play+dc" reactive. Bob (the
  // reacter) gets a CALLED_TO_DISCARD mark; he should NOT be marked
  // CALLED_TO_PLAY.
  EXPECT_TRUE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_DISCARD));
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
  // The WC carries the snapshot of all_plays for the reaction layer.
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_FALSE(g.waiting.front().all_plays);
}

TEST(AllPlays, ColourClueMarksReacterPlayWhenOn) {
  Game g = setup(baseline_opts());
  g.all_plays = true;
  g = take_turn(std::move(g), "Alice clues red to Cathy");

  // /allplays on: color clue is promoted to play+play. Bob (the reacter)
  // gets a CALLED_TO_PLAY mark and no CALLED_TO_DISCARD.
  EXPECT_TRUE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_DISCARD));
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_TRUE(g.waiting.front().all_plays);
}
