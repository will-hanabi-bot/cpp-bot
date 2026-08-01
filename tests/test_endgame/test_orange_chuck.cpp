// The endgame solver and inverted (Orange / Dark Orange) suits.
//
// Inverted suits swap the two buttons: the action that advances the stack is
// the CHUCK (PerformDiscard), not the pitch (PerformPlay). Nothing in
// src/endgame/ knew that. `possible_actions` enumerated plays from
// obvious_playables / thinks_playables and always emitted PerformPlay, so for
// an orange card the simulated action sent it to the discard pile, every line
// that needed an orange play scored as a loss, and the winning chuck was never
// a candidate.
//
// bug_report_3.txt 3.2 (replay 1942723 #42) is the motivating case: stacks
// [5,5,5,3] with a known Orange 4 in hand and the Orange 5 across the table,
// a 20/20 win, and the bot discarded an unrelated slot instead.
//
// The companion hazard is the `failed` flag. Both endgame perform_to_action
// implementations hardcoded `failed=false`, and `Game::on_discard` calls
// `with_play` for an inverted suit whenever `failed` is false — so a chuck of
// a NON-playable orange would have jumped the stack and let the search
// hallucinate wins.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/endgame/fraction.h"
#include "hanabi/endgame/solver.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::endgame;
using namespace hanabi::test;

// Stacks [5,5,5,4] in "Orange (4 Suits)" — Red / Green / Blue / Orange. The
// only card left to score is Orange 5, which Alice holds and knows. Playing it
// physically (a pitch) throws it in the discard pile and loses; chucking it
// advances the orange stack and wins.
TEST(EndgameOrange, KnownPlayableOrangeIsOfferedAsAChuck) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  opts.hands = {
      {"o5", "xx", "xx", "xx", "xx"},
      {"xx", "xx", "xx", "xx", "xx"},
  };
  opts.play_stacks = std::vector<int>{5, 5, 5, 4};
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "o5");
  g.common.dirty.insert(g.state.hands[0][0]);
  g.elim();

  EndgameSolver solver{/*mc=*/true, /*timeout=*/10.0};
  SolveResult r = solver.solve(g);
  ASSERT_TRUE(r.ok()) << r.error;
  ASSERT_TRUE(std::holds_alternative<PerformDiscard>(r.action))
      << "an orange card is stacked by pressing Discard, not Play "
         "(bug_report_3.txt 3.2)";
  EXPECT_EQ(std::get<PerformDiscard>(r.action).target, g.state.hands[0][0]);
  EXPECT_EQ(r.winrate, Fraction(1));
}

// `perform_to_action` must derive `failed` rather than hardcoding false, or
// the search models a chuck of a non-playable orange as a stack advance.
// Same rule as variants::make_discard_for_simulation.
TEST(EndgameOrange, ChuckOfNonPlayableOrangeIsModelledAsAMisplay) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  opts.hands = {
      {"o4", "o1", "xx", "xx", "xx"},
      {"xx", "xx", "xx", "xx", "xx"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 0, 0};
  Game g = setup(std::move(opts));

  const int o4 = g.state.hands[0][0];
  const int o1 = g.state.hands[0][1];

  Action bad = EndgameSolver::perform_to_action(PerformDiscard{o4}, g, 0);
  ASSERT_TRUE(std::holds_alternative<DiscardAction>(bad));
  EXPECT_TRUE(std::get<DiscardAction>(bad).failed)
      << "Orange 4 on an empty orange stack cannot be chucked successfully";

  Action good = EndgameSolver::perform_to_action(PerformDiscard{o1}, g, 0);
  ASSERT_TRUE(std::holds_alternative<DiscardAction>(good));
  EXPECT_FALSE(std::get<DiscardAction>(good).failed)
      << "Orange 1 is playable, so the chuck succeeds";

  // A normal suit is unaffected: a physical discard is just a discard.
  SetupOptions plain;
  plain.hands = {{"r4", "xx", "xx", "xx", "xx"}, {"xx", "xx", "xx", "xx", "xx"}};
  Game p = setup(std::move(plain));
  Action normal = EndgameSolver::perform_to_action(PerformDiscard{p.state.hands[0][0]}, p, 0);
  ASSERT_TRUE(std::holds_alternative<DiscardAction>(normal));
  EXPECT_FALSE(std::get<DiscardAction>(normal).failed);
}
