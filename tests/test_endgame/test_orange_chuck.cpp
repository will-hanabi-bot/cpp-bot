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
#include "hanabi/endgame/winnable.h"
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

// bug_report_4_1_0.txt 4.1.0a (replay 1957936 #41). Unlike
// `KnownPlayableOrangeIsOfferedAsAChuck` above — which is one card from the
// max score and so short-circuits at `solver.cpp`'s trivial check — this needs
// the SEARCH: a four-card orange chain across three hands, where every link is
// a chuck.
//
// Stacks [5,5,5,1] with one card left. Alice holds Orange 2 and Orange 5, Bob
// the Orange 3, Cathy the Orange 4. Alice chucks the 2 and draws the last card,
// which starts the final round: Bob chucks the 3, Cathy the 4, and Alice — who
// acts LAST, because the drawer of the final card still gets one more turn —
// chucks the 5 for 20/20.
//
// The feasibility layers used to emit `PerformPlay` for each of those
// follow-ups (`player_known_plays`' consumer, `clueless_winnable`), which the
// inversion turns into a pitch, so every continuation of the winning chuck
// scored as a loss and the line was pruned UNWINNABLE before it was scored.
TEST(EndgameOrange, OrangeChuckChainAcrossHandsIsFound) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  // Every card is concrete and Alice's whole hand is pinned below, so the only
  // unseen card is the single one left in the deck — which is what the
  // solver's remaining-card accounting requires.
  opts.hands = {
      {"o2", "o5", "r1", "r1", "r2"},
      {"o3", "g1", "g1", "g2", "g3"},
      {"o4", "b1", "b1", "b2", "b3"},
  };
  opts.play_stacks = std::vector<int>{5, 5, 5, 1};
  // Eight discards bring the deck to its last card. r3/r4/g4/b4 are trash (their
  // suits are finished), the two spare Orange 1s are trash (the stack is at 1),
  // and the spare o2/o3 leave the hand copies as the last ones. Max stays 20.
  opts.discarded = {"r3", "r4", "g4", "b4", "o1", "o1", "o2", "o3"};
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "o2");
  g = fully_known(std::move(g), TestPlayer::ALICE, 2, "o5");
  g = fully_known(std::move(g), TestPlayer::ALICE, 3, "r1");
  g = fully_known(std::move(g), TestPlayer::ALICE, 4, "r1");
  g = fully_known(std::move(g), TestPlayer::ALICE, 5, "r2");
  // Bob and Cathy must know their links too, or they cannot chuck them.
  g = fully_known(std::move(g), TestPlayer::BOB, 1, "o3");
  g = fully_known(std::move(g), TestPlayer::CATHY, 1, "o4");
  for (const auto& hand : g.state.hands) {
    for (int o : hand) g.common.dirty.insert(o);
  }
  g.elim();

  ASSERT_EQ(g.state.cards_left, 1) << "fixture must leave exactly one card";
  ASSERT_EQ(g.state.max_score(), 20) << "no discard may cost a point";

  EndgameSolver solver{/*mc=*/true, /*timeout=*/10.0};
  SolveResult r = solver.solve(g);
  ASSERT_TRUE(r.ok()) << r.error;
  ASSERT_TRUE(std::holds_alternative<PerformDiscard>(r.action))
      << "the winning move is a CHUCK of the Orange 2 — Discard is the button "
         "that advances an inverted stack";
  EXPECT_EQ(std::get<PerformDiscard>(r.action).target, g.state.hands[0][0]);
  EXPECT_EQ(r.winrate, Fraction(1))
      << "the chain is forced, so the win is certain";
}

// `advance_state` is the State-level step used by `clueless_winnable`. Unlike
// its Game-level sibling `advance_game`/`perform_to_action`, it had NO inverted
// branch at all, so it credited a stack advance for a physical Play on an
// orange — a hallucinated win, and the opposite error from the pruning ones
// above. TODO entry 10 listed the two `winnable.cpp` sites but not this one.
TEST(EndgameOrange, AdvanceStateRunsTheInvertedButtonSwap) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  opts.hands = {
      {"o1", "r1", "g1", "b1", "r2"},
      {"o2", "r3", "g3", "b3", "r4"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 0, 0};
  Game g = setup(std::move(opts));
  const State& s = g.state;
  const int o1 = s.hands[0][0];
  ASSERT_TRUE(s.is_playable(Identity{3, 1})) << "orange 1 is playable at 0";

  // PITCH: press Play. The inversion sends it to the discard pile and regains
  // a clue; the stack does NOT move.
  State pitched = hanabi::endgame::advance_state(s, PerformPlay{o1}, 0,
                                                  std::nullopt);
  EXPECT_EQ(pitched.play_stacks[3], 0)
      << "pressing Play on an orange discards it — no stack progress";
  EXPECT_EQ(pitched.strikes, s.strikes) << "and it is not a misplay";

  // CHUCK: press Discard. That is the play attempt, and it advances the stack.
  State chucked = hanabi::endgame::advance_state(s, PerformDiscard{o1}, 0,
                                                  std::nullopt);
  EXPECT_EQ(chucked.play_stacks[3], 1)
      << "pressing Discard on a playable orange is what stacks it";
  EXPECT_EQ(chucked.strikes, s.strikes);
}

// --- bug_report_5_0_0.txt: the solver's discard candidate ------------------

// `Game::find_all_discards` is the ONLY source of discard candidates for the
// endgame solver (`solver.cpp:305-312`), and it returns exactly one action. It
// used to be an unconditional `PerformDiscard`, which on an inverted suit is a
// CHUCK — a play attempt that strikes unless the card is currently playable.
//
// Replay 1957953 T30: a fully known Orange 2 with the orange stack already at
// 5. The solver's only discard candidate was a guaranteed misplay, and since it
// optimises win probability — where a first strike is not by itself fatal — the
// chuck cleared the 1% gate with no pitch to lose to. It struck.
TEST(EndgameOrange, FindAllDiscardsPitchesAKnownTrashOrange) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  opts.hands = {
      {"o2", "r1", "g1", "b1", "r2"},
      {"xx", "xx", "xx", "xx", "xx"},
  };
  // Orange is complete, so the o2 is basic trash; the other suits sit at 0, so
  // r1/g1/b1 are playable and never enter `thinks_trash`.
  opts.play_stacks = std::vector<int>{0, 0, 0, 5};
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "o2");

  const int o2 = g.state.hands[0][0];
  auto discards = g.find_all_discards(0);
  ASSERT_EQ(discards.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(discards.front()))
      << "throwing an orange away is the PITCH — press Play; pressing Discard "
         "is a play attempt that strikes on trash";
  EXPECT_EQ(std::get<PerformPlay>(discards.front()).target, o2);
}

// Knowing the SUIT is enough to know which button to press, so the rule is
// "every possibility is inverted", not "pinned to one identity". A card clued
// orange with its rank still open is exactly as safe to pitch — and would
// strike just as surely on a chuck.
TEST(EndgameOrange, FindAllDiscardsPitchesAnOrangeOfUnknownRank) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  opts.hands = {
      {"o2", "r1", "g1", "b1", "r2"},
      {"xx", "xx", "xx", "xx", "xx"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 0, 5};
  Game g = setup(std::move(opts));
  // Colour only: Alice knows it is orange, not which orange. Every orange is
  // trash at a stack of 5, so it is still the trash front.
  g = pre_clue(std::move(g), TestPlayer::ALICE, 1, {"orange"});

  const int o2 = g.state.hands[0][0];
  ASSERT_GT(g.common.thoughts[o2].possible.length(), 1)
      << "fixture must leave the rank open";

  auto discards = g.find_all_discards(0);
  ASSERT_EQ(discards.size(), 1u);
  EXPECT_TRUE(std::holds_alternative<PerformPlay>(discards.front()))
      << "the suit alone decides the button";
}

// The control: a non-orange trash card is still an ordinary discard. Without
// this, "always pitch" would pass the two tests above and break every normal
// suit — pressing Play on a non-orange trash card is a misplay.
TEST(EndgameOrange, FindAllDiscardsStillDiscardsNonOrangeTrash) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  opts.hands = {
      {"r1", "g1", "b1", "o1", "r2"},
      {"xx", "xx", "xx", "xx", "xx"},
  };
  // Red is complete, so the r1 is trash; g1/b1/o1 are playable.
  opts.play_stacks = std::vector<int>{5, 0, 0, 0};
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "r1");

  const int r1 = g.state.hands[0][0];
  auto discards = g.find_all_discards(0);
  ASSERT_EQ(discards.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<PerformDiscard>(discards.front()))
      << "a normal suit keeps the normal button";
  EXPECT_EQ(std::get<PerformDiscard>(discards.front()).target, r1);
}
