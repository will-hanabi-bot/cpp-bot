// Odds and Evens: the parity swap and the clue enumeration it implies.
//
// In this family a rank clue names a PARITY class rather than a rank (1 = odd,
// touching 1/3/5; 2 = even, touching 2/4), and the reactive roles of the two
// clue kinds swap:
//
//   normally   RANK   = even parity (double play, or double discard)
//              COLOUR = odd parity  (exactly one play)
//   O&E        COLOUR = even parity
//              RANK   = odd parity
//
// A rank clue's value no longer names a rank, so it cannot serve as the anchor
// directly; it maps odd -> 3, even -> 4. Colour anchors are unchanged.
//
// The touch rule itself lives in `Variant::id_touched` and is covered in
// test_variants.cpp. This file pins the predicates every convention site reads,
// the clue enumeration, and the /settings text partners rely on.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/variants/predicates.h"
#include "hanabi/conventions/variants/reactive_table.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using hanabi::reactor::variants::format_reactive_settings;
using hanabi::reactor::variants::rank_reactive_value;
using hanabi::reactor::variants::uses_even_parity;

// --- the two predicates ---------------------------------------------------

TEST(OddsAndEvens, ParityRolesSwap) {
  const Variant& plain = get_variant("No Variant");
  EXPECT_TRUE(uses_even_parity(plain, ClueKind::RANK))
      << "normally a rank clue is the double play / double discard";
  EXPECT_FALSE(uses_even_parity(plain, ClueKind::COLOUR));

  const Variant& oe = get_variant("Odds and Evens (5 Suits)");
  EXPECT_FALSE(uses_even_parity(oe, ClueKind::RANK))
      << "under O&E the rank clue becomes the one-play reactive";
  EXPECT_TRUE(uses_even_parity(oe, ClueKind::COLOUR))
      << "and the colour clue becomes the double play / double discard";
}

TEST(OddsAndEvens, RankAnchorMapsOddToThreeAndEvenToFour) {
  const Variant& plain = get_variant("No Variant");
  for (int v = 1; v <= 5; ++v) {
    EXPECT_EQ(rank_reactive_value(plain, v), v)
        << "normally the clue value IS the rank, so it doubles as the anchor";
  }

  const Variant& oe = get_variant("Odds and Evens (5 Suits)");
  EXPECT_EQ(rank_reactive_value(oe, 1), 3) << "odd -> 3";
  EXPECT_EQ(rank_reactive_value(oe, 2), 4) << "even -> 4";
}

// --- clue enumeration -----------------------------------------------------

namespace {

// A hand holding every rank, so which clues get offered is decided by the
// variant rather than by what happens to be in the hand.
Game every_rank_position(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.hands = {
      {"r1", "r2", "r3", "r4", "r5"},
      {"b1", "b2", "b3", "b4", "b5"},
      {"g1", "g2", "g3", "g4", "g5"},
  };
  return setup(std::move(opts));
}

std::vector<int> offered_ranks(const Game& g, int target) {
  std::vector<int> out;
  for (const Clue& c : g.state.all_valid_clues(target)) {
    if (c.kind == ClueKind::RANK) out.push_back(c.value);
  }
  return out;
}

}  // namespace

TEST(OddsAndEvens, OnlyOddAndEvenRankCluesAreOffered) {
  Game g = every_rank_position("Odds and Evens (5 Suits)");
  EXPECT_EQ(offered_ranks(g, 1), (std::vector<int>{1, 2}))
      << "a 3/4/5 clue does not exist in this variant";
}

TEST(OddsAndEvens, APlainVariantStillOffersEveryRank) {
  Game g = every_rank_position("No Variant");
  EXPECT_EQ(offered_ranks(g, 1), (std::vector<int>{1, 2, 3, 4, 5}));
}

// The free win from reading `clueRanks`: Number Mute has no rank clues at all,
// and before this the bot would happily offer five of them.
TEST(OddsAndEvens, NumberMuteOffersNoRankClues) {
  Game g = every_rank_position("Number Mute (5 Suits)");
  EXPECT_TRUE(offered_ranks(g, 1).empty());
  bool any_colour = false;
  for (const Clue& c : g.state.all_valid_clues(1)) {
    if (c.kind == ClueKind::COLOUR) any_colour = true;
  }
  EXPECT_TRUE(any_colour) << "colour clues are unaffected";
}

// --- /settings ------------------------------------------------------------

// The text a human partner reads to learn what the bot will do. Printing the
// vanilla wording under O&E would tell them the exact opposite.
TEST(OddsAndEvens, SettingsTextSwapsTheHalves) {
  const Variant& oe = get_variant("Odds and Evens (5 Suits)");
  const std::string out = format_reactive_settings(oe, 5, /*all_plays=*/false);
  EXPECT_EQ(out, "odd plays: {-, -, odd, even, -}, even plays: {slot focus}");
}

TEST(OddsAndEvens, SettingsTextIsUnchangedForAPlainVariant) {
  const Variant& plain = get_variant("No Variant");
  EXPECT_EQ(format_reactive_settings(plain, 5, /*all_plays=*/false),
            "odd plays: {slot focus}, even plays: {slot focus}");
}
