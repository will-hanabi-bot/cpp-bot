// Alternating Clues: no two CONSECUTIVE clues may be of the same kind.
//
// A server rule rather than a convention, so it belongs in the clue
// enumeration: `State::all_valid_clues` drops the offending kind, keyed on
// `State::last_clue_kind`, which `Game::on_clue` records for every clue real or
// simulated. Filtering there rather than in the callers is what makes the
// endgame solver and `eval.cpp` agree with the bot's own clue list -- otherwise
// the search happily costs out lines built on clues the server would reject.
//
// Two properties are easy to get wrong and are pinned separately below:
//   * a PLAY or DISCARD in between does not reset it. The rule is about
//     consecutive clues, not consecutive turns.
//   * it is about the last clue given by ANYONE, not by this seat.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

SetupOptions alt_opts() {
  SetupOptions opts;
  opts.variant_name = "Alternating Clues (5 Suits)";
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r1", "y2", "g3", "b4", "p5"},  // Alice
      {"r2", "y3", "g4", "b5", "p1"},  // Bob
      {"r3", "y4", "g5", "b1", "p2"},  // Cathy
  };
  return opts;
}

bool offers(const Game& g, int target, ClueKind kind) {
  for (const Clue& c : g.state.all_valid_clues(target)) {
    if (c.kind == kind) return true;
  }
  return false;
}

}  // namespace

TEST(AlternatingClues, TheFlagIsParsed) {
  EXPECT_TRUE(get_variant("Alternating Clues (5 Suits)").alternating_clues);
  EXPECT_FALSE(get_variant("No Variant").alternating_clues);
}

// Nothing has been clued yet, so neither kind is blocked.
TEST(AlternatingClues, TheFirstClueMayBeEitherKind) {
  Game g = setup(alt_opts());
  ASSERT_FALSE(g.state.last_clue_kind.has_value()) << "guard: no clue yet";
  EXPECT_TRUE(offers(g, 1, ClueKind::RANK));
  EXPECT_TRUE(offers(g, 1, ClueKind::COLOUR));
}

TEST(AlternatingClues, ARankClueBlocksTheNextRankClue) {
  Game g = setup(alt_opts());
  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  ASSERT_EQ(g.state.last_clue_kind, std::optional<ClueKind>{ClueKind::RANK});
  EXPECT_FALSE(offers(g, 2, ClueKind::RANK))
      << "two rank clues in a row is not a legal move";
  EXPECT_TRUE(offers(g, 2, ClueKind::COLOUR))
      << "colour is what the alternation leaves";
}

TEST(AlternatingClues, AColourClueBlocksTheNextColourClue) {
  Game g = setup(alt_opts());
  g = take_turn(std::move(g), "Alice clues Red to Bob");

  ASSERT_EQ(g.state.last_clue_kind, std::optional<ClueKind>{ClueKind::COLOUR});
  EXPECT_FALSE(offers(g, 2, ClueKind::COLOUR));
  EXPECT_TRUE(offers(g, 2, ClueKind::RANK));
}

// The rule counts CLUES, not turns.
TEST(AlternatingClues, APlayInBetweenDoesNotResetIt) {
  Game g = setup(alt_opts());
  g = take_turn(std::move(g), "Alice clues 2 to Bob");
  g = take_turn(std::move(g), "Bob plays p1 (slot 5)", "r4");

  ASSERT_EQ(g.state.last_clue_kind, std::optional<ClueKind>{ClueKind::RANK})
      << "a play must not clear the record";
  EXPECT_FALSE(offers(g, 0, ClueKind::RANK));
  EXPECT_TRUE(offers(g, 0, ClueKind::COLOUR));
}

TEST(AlternatingClues, ADiscardInBetweenDoesNotResetItEither) {
  Game g = setup(alt_opts());
  g = take_turn(std::move(g), "Alice clues Red to Bob");
  g = take_turn(std::move(g), "Bob discards p1 (slot 5)", "r4");

  ASSERT_EQ(g.state.last_clue_kind, std::optional<ClueKind>{ClueKind::COLOUR});
  EXPECT_FALSE(offers(g, 0, ClueKind::COLOUR));
  EXPECT_TRUE(offers(g, 0, ClueKind::RANK));
}

// It tracks the last clue by ANY player, so it flips back as play continues.
TEST(AlternatingClues, ItAlternatesAcrossPlayers) {
  Game g = setup(alt_opts());
  g = take_turn(std::move(g), "Alice clues 3 to Bob");
  EXPECT_FALSE(offers(g, 2, ClueKind::RANK));

  g = take_turn(std::move(g), "Bob clues Red to Cathy");
  EXPECT_TRUE(offers(g, 1, ClueKind::RANK))
      << "Bob's colour clue re-opens rank for the next giver";
  EXPECT_FALSE(offers(g, 1, ClueKind::COLOUR));
}

// The filter must not leak into any other variant.
TEST(AlternatingClues, APlainVariantIsUnaffected) {
  SetupOptions opts = alt_opts();
  opts.variant_name = "No Variant";
  Game g = setup(std::move(opts));
  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  ASSERT_EQ(g.state.last_clue_kind, std::optional<ClueKind>{ClueKind::RANK})
      << "the field is recorded everywhere -- only the FILTER is gated";
  EXPECT_TRUE(offers(g, 2, ClueKind::RANK))
      << "outside Alternating Clues two rank clues in a row are fine";
}

// A hypo must carry the constraint too, or the decision layer and the endgame
// solver would evaluate lines the server rejects.
TEST(AlternatingClues, ASimulatedClueConstrainsTheHypo) {
  Game g = setup(alt_opts());
  const State& s = g.state;
  ClueAction rank3{0, 1, s.clue_touched(s.hands[1], ClueKind::RANK, 3),
                   BaseClue{ClueKind::RANK, 3}};
  Game hypo = g.simulate(Action{rank3});

  EXPECT_EQ(hypo.state.last_clue_kind, std::optional<ClueKind>{ClueKind::RANK});
  EXPECT_FALSE(offers(hypo, 2, ClueKind::RANK))
      << "the hypo must know a rank clue was just given";
}
