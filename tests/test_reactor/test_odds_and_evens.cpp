// Reactor under Odds and Evens.
//
// Reactor0 is gated to three players (`src/net/commands.cpp`), so a real Odds
// and Evens game at 4+ seats runs REACTOR. Both conventions therefore need the
// swap, and they must agree on what a clue means or the two bots would read the
// same table differently.
//
//   normally   RANK   = even parity -- double play, or double discard
//              COLOUR = odd parity  -- exactly one play
//   O&E        COLOUR = even parity
//              RANK   = odd parity
//
// The anchor differs from reactor0's in the vanilla case: reactor's
// `reactive_focus` is POSITIONAL (the focus slot) and only reads the clue value
// for rainbowish colour clues or pinkish rank clues. Odds and Evens overrides
// it for rank the same way pinkish does, because an odd/even value cannot name
// a rank -- odd -> 3, even -> 4, matching reactor0 exactly.
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

// Alice gives, Bob reacts, Cathy receives. Stacks at 0 so Bob has playables
// and Cathy's slot 1 is her leftmost playable.
SetupOptions oe_opts(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r4", "y4", "g4", "b4", "p4"},  // Alice (giver, us)
      {"r1", "y1", "g1", "b1", "p1"},  // Bob   (reacter) -- all playable
      {"r1", "y2", "g3", "b4", "p5"},  // Cathy (receiver)
  };
  return opts;  // no init hook -> reactor
}

// `last_clue_interp` lives in the reactor0 helper header, which this target
// does not include; the body is three lines, so inline it rather than reach
// across convention test trees.
std::optional<ClueInterp> last_interp(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  const auto& last = g.move_history.back();
  if (auto* ci = std::get_if<ClueInterp>(&last)) return *ci;
  return std::nullopt;
}

int count_status(const Game& g, TestPlayer who, CardStatus st) {
  int n = 0;
  for (int o : g.state.hands[static_cast<int>(who)]) {
    if (g.meta[o].status == st) ++n;
  }
  return n;
}

// Total PLAY calls across reacter and receiver -- the convention's own name for
// the two rulesets ("odd plays" / "even plays"), and independent of which seat
// the odd-parity family hands the play to.
int play_calls(const Game& g) {
  return count_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY) +
         count_status(g, TestPlayer::CATHY, CardStatus::CALLED_TO_PLAY);
}

}  // namespace

// --- the anchor -----------------------------------------------------------

TEST(ReactorOddsAndEvens, RankAnchorIsThreeForOddAndFourForEven) {
  Game g = setup(oe_opts("Odds and Evens (5 Suits)"));
  g = take_turn(std::move(g), "Alice clues 1 to Cathy");
  ASSERT_FALSE(g.waiting.empty()) << "an odd clue to Cathy must read reactive";
  EXPECT_EQ(g.waiting.front().focus_slot, 3)
      << "odd -> 3, the same mapping reactor0 uses";

  Game g2 = setup(oe_opts("Odds and Evens (5 Suits)"));
  g2 = take_turn(std::move(g2), "Alice clues 2 to Cathy");
  ASSERT_FALSE(g2.waiting.empty());
  EXPECT_EQ(g2.waiting.front().focus_slot, 4) << "even -> 4";
}

// The mirror: in a plain variant reactor's rank anchor stays POSITIONAL, so it
// is the focus slot rather than the clue value. Cathy's only 2 is on slot 2, so
// the two coincide numerically -- her only 5 is on slot 5, which separates
// them: a rank-5 clue anchors on slot 5 either way, but a rank-3 clue whose
// focus is slot 3 would anchor on 3 positionally.
TEST(ReactorOddsAndEvens, PlainVariantRankAnchorStaysPositional) {
  Game g = setup(oe_opts("No Variant"));
  // Cathy's g3 is on slot 3, and it is the only 3 she holds.
  g = take_turn(std::move(g), "Alice clues 3 to Cathy");
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().focus_slot, 3)
      << "the focus slot, which here is also the rank";
}

// --- the parity swap ------------------------------------------------------

TEST(ReactorOddsAndEvens, ColourClueTakesTheEvenParityRuleset) {
  Game oe = setup(oe_opts("Odds and Evens (5 Suits)"));
  oe = take_turn(std::move(oe), "Alice clues red to Cathy");
  ASSERT_EQ(last_interp(oe), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(oe) % 2, 0)
      << "O&E colour is the even-parity family; got " << play_calls(oe)
      << " play calls";

  Game plain = setup(oe_opts("No Variant"));
  plain = take_turn(std::move(plain), "Alice clues red to Cathy");
  ASSERT_EQ(last_interp(plain), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(plain), 1)
      << "plain colour is the odd-parity family; got " << play_calls(plain);
}

TEST(ReactorOddsAndEvens, RankClueTakesTheOddParityRuleset) {
  Game oe = setup(oe_opts("Odds and Evens (5 Suits)"));
  oe = take_turn(std::move(oe), "Alice clues 1 to Cathy");
  ASSERT_EQ(last_interp(oe), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(oe), 1)
      << "O&E rank is the odd-parity family; got " << play_calls(oe);

  Game plain = setup(oe_opts("No Variant"));
  plain = take_turn(std::move(plain), "Alice clues 1 to Cathy");
  ASSERT_EQ(last_interp(plain), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(plain) % 2, 0)
      << "plain rank is the even-parity family; got " << play_calls(plain);
}
