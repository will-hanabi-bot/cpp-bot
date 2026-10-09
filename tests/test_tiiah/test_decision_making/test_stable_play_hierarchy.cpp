// The STABLE PLAY HIERARCHY (tiiah/CONVENTION.md §2a, v18.17.0).
//
// Human diagnostic human_vs_bot_diagnostics/2014561.md T49/T50: when several
// stable play clues exist, tiebreak them in this order, each judged over what the
// one above left, all from the giver's model of the receiver after the clue:
//   1. fewest identities left on the called card;
//   2. most 1.99 x (ancillary good cards newly touched) - (unknown trash newly
//      touched);
//   3. smallest product of candidate counts over the receiver's other good cards;
//   4. colour over rank (unless the colour could mistake a rainbow card).
// Then the default tiebreak. Each test below is a tie the criterion above leaves,
// broken by the next -- on candidates built by hand, since the keys are what is
// under test; the replay test (2014561 T50) covers how `analyse_clues` fills them.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;
using hanabi::reactor0::ClueCandidate;
using hanabi::reactor0::ClueShape;

namespace {

Game tiiah_game() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"r1", "y1", "g1", "b1", "p1"},
                {"r2", "y2", "g2", "b2", "p2"}};
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return setup(std::move(opts));
}

ClueCandidate stable_play(int inferences, double ancillary, double product, bool colour,
                          double default_score = 0.0) {
  const BaseClue clue(colour ? ClueKind::COLOUR : ClueKind::RANK, 1);
  ClueCandidate c{PerformAction{PerformColour{1, 1}}, ClueAction(0, 1, {}, clue),
                  {}, hanabi::reactor0::ClueTier::LOW, 0.0};
  c.reading.shape = ClueShape::STABLE_PLAY;
  c.target_inferences = inferences;
  c.ancillary_score = ancillary;
  c.others_product = product;
  c.colour_ok = colour;
  c.default_score = default_score;
  return c;
}

const ClueCandidate* best(const Game& g, const std::vector<ClueCandidate>& cs) {
  std::vector<const ClueCandidate*> pool;
  for (const auto& c : cs) pool.push_back(&c);
  return hanabi::reactor0::best_stable_play(g, pool);
}

}  // namespace

// 1. The call that leaves the fewest identities wins, whatever else it does
// (2014561 T49's example: Green's `{g4}` over rank 4's `{r4,y4,g4}`).
TEST(TiiahStablePlayHierarchy, FewestInferencesFirst) {
  Game g = tiiah_game();
  std::vector<ClueCandidate> cs = {stable_play(3, 1.99, 1, true, 9.0),
                                   stable_play(1, 0.0, 99, false)};
  EXPECT_EQ(best(g, cs), &cs[1]);
}

// 2. Tied on 1: the most ancillary value wins.
TEST(TiiahStablePlayHierarchy, AncillaryValueSecond) {
  Game g = tiiah_game();
  std::vector<ClueCandidate> cs = {stable_play(1, 0.0, 1, true),
                                   stable_play(1, 1.99, 99, false)};
  EXPECT_EQ(best(g, cs), &cs[1]);
}

// ...and unknown trash counts against a clue.
TEST(TiiahStablePlayHierarchy, UnknownTrashCostsAncillaryValue) {
  Game g = tiiah_game();
  std::vector<ClueCandidate> cs = {stable_play(1, -1.0, 1, true),
                                   stable_play(1, 0.0, 99, false)};
  EXPECT_EQ(best(g, cs), &cs[1]);
}

// 3. Tied on 1 and 2: the rest of the receiver's hand best known wins (2014561 T50:
// Purple pinned the clued p4).
TEST(TiiahStablePlayHierarchy, SmallestProductThird) {
  Game g = tiiah_game();
  std::vector<ClueCandidate> cs = {stable_play(1, 0.0, 540, true),
                                   stable_play(1, 0.0, 176, true),
                                   stable_play(1, 0.0, 680, false)};
  EXPECT_EQ(best(g, cs), &cs[1]);
}

// 4. Tied on 1-3: colour over rank.
TEST(TiiahStablePlayHierarchy, ColourFourth) {
  Game g = tiiah_game();
  std::vector<ClueCandidate> cs = {stable_play(1, 0.0, 6, false, 5.0),
                                   stable_play(1, 0.0, 6, true, 0.0)};
  EXPECT_EQ(best(g, cs), &cs[1]);
}

// Tied on all four: the default tiebreak decides.
TEST(TiiahStablePlayHierarchy, DefaultTiebreakLast) {
  Game g = tiiah_game();
  std::vector<ClueCandidate> cs = {stable_play(1, 0.0, 6, true, 1.0),
                                   stable_play(1, 0.0, 6, true, 2.0)};
  EXPECT_EQ(best(g, cs), &cs[1]);
}
