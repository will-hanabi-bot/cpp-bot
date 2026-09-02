// Reactor0's reactive table: two parity buckets, and `/set` to move a clue.
//
// Every clue sits in one of two buckets -- even (double play, or double
// discard) or odd (exactly one play) -- and carries a reactive value there.
// Normally rank clues are the even bucket and colour clues the odd; Odds and
// Evens swaps them; `/set` moves a single clue.
//
// The `/settings` strings below are pinned VERBATIM, because they are the
// contract a human partner reads to know what the bot will do.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/colour_value.h"
#include "hanabi/conventions/reactor0/reactive_assignment.h"
#include "hanabi/conventions/variants/predicates.h"

using namespace hanabi;
using hanabi::reactor0::build_reactive_table;
using hanabi::reactor0::clue_label;
using hanabi::reactor0::format_settings;
using hanabi::reactor0::parse_clue_label;
using hanabi::reactor0::reactive_assignment;

namespace {
const std::vector<ReactiveOverride> kNone;
}  // namespace

// --- the two default layouts ----------------------------------------------

TEST(Reactor0ReactiveAssignment, PlainVariantSettingsLine) {
  const Variant& v = get_variant("No Variant");
  EXPECT_EQ(format_settings(v, kNone, /*rlocks=*/true),
            "reactor0 — even reactive values: {1=1, 2=2, 3=3, 4=4, 5=5}, "
            "odd reactive values: {Red=1, Yellow=2, Green=3, Blue=4, Purple=5}, "
            "rlocks: on");
}

// Odds and Evens swaps the buckets, and a rank clue names a parity rather than
// a rank -- so the labels are Odd / Even and the values are 3 / 4.
TEST(Reactor0ReactiveAssignment, OddsAndEvensSettingsLine) {
  const Variant& v = get_variant("Odds and Evens & Orange (3 Suits)");
  EXPECT_EQ(format_settings(v, kNone, /*rlocks=*/true),
            "reactor0 — even reactive values: {Red=1, Blue=4, Orange=2}, "
            "odd reactive values: {Odd=3, Even=4}, rlocks: on");
}

// --- target-parity variants -----------------------------------------------
//
// Neither of these was covered until v13.5.0, which is exactly how README's
// sample of this line came to drift from what the code emits. The line is what a
// human reads in chat to know the convention in play, and hanab.live truncates
// it, so its LENGTH is part of the contract -- see the Synesthesia case.

TEST(Reactor0ReactiveAssignment, AlternatingCluesSettingsLine) {
  const Variant& v = get_variant("Alternating Clues (5 Suits)");
  EXPECT_EQ(format_settings(v, kNone, /*rlocks=*/false),
            "reactor0 — no stable clues above pace 1: to Bob = odd, to Cathy = "
            "even, reactive values: {1=1, 2=2, 3=3, 4=4, 5=5, Red=1, Yellow=2, "
            "Green=3, Blue=4, Purple=5}, at pace <= 1 a clue to Bob is STABLE, "
            "rlocks: off");
}

// Synesthesia carries `clueRanks: []`, so the stable clues it falls back to at
// 50% read off a fixed colour table instead of the usual ladders. It is rendered
// ABBREVIATED -- `fN` for a pitch, `dN` for a chuck -- because the full
// `pitchN`/`chuckN` words pushed the line past hanab.live's chat limit and the
// tail that fell off included `rlocks`. GLOSSARY.md *synesthesia table* carries
// the mapping in full words.
TEST(Reactor0ReactiveAssignment, SynesthesiaSettingsLineIsAbbreviated) {
  const Variant& v = get_variant("Synesthesia & Black (6 Suits)");
  const std::string line = format_settings(v, kNone, /*rlocks=*/false);
  EXPECT_EQ(line, "reactor0 — no stable clues above pace 1: to Bob = odd, to Cathy = "
      "even, reactive values: {Red=1, Yellow=2, Green=3, Blue=4, Purple=5, "
      "Black=1}, at pace <= 1 a clue to Bob is STABLE: Red=f1, Yellow=f2, "
      "Green=f3, Blue=f4, Purple=f5, Orange=d1, other=d4, rlocks: off");
  EXPECT_EQ(line.find("pitch"), std::string::npos)
      << "the long spelling is what overflowed the chat limit";
  EXPECT_EQ(line.find("chuck"), std::string::npos);
}

// --- /set -----------------------------------------------------------------

// `/set Yellow even 4`: Yellow leaves the odd bucket and joins the even one at
// the END, with value 4.
TEST(Reactor0ReactiveAssignment, SetMovesAClueBetweenBuckets) {
  const Variant& v = get_variant("No Variant");
  auto clue = parse_clue_label(v, "Yellow");
  ASSERT_TRUE(clue.has_value());
  const std::vector<ReactiveOverride> ov{
      ReactiveOverride{clue->first, clue->second, /*even=*/true, 4}};

  EXPECT_EQ(format_settings(v, ov, /*rlocks=*/true),
            "reactor0 — even reactive values: {1=1, 2=2, 3=3, 4=4, 5=5, Yellow=4}, "
            "odd reactive values: {Red=1, Green=3, Blue=4, Purple=5}, "
            "rlocks: on");
}

TEST(Reactor0ReactiveAssignment, AnOverrideChangesTheAssignmentItself) {
  const Variant& v = get_variant("No Variant");
  auto yellow = parse_clue_label(v, "yellow");  // case-insensitive
  ASSERT_TRUE(yellow.has_value());
  EXPECT_EQ(yellow->first, ClueKind::COLOUR);

  const auto before = reactive_assignment(v, kNone, yellow->first, yellow->second);
  EXPECT_FALSE(before.even) << "a colour clue is normally the odd bucket";
  EXPECT_EQ(before.value, 2) << "Yellow=2 by the built-in table";

  const std::vector<ReactiveOverride> ov{
      ReactiveOverride{yellow->first, yellow->second, true, 4}};
  const auto after = reactive_assignment(v, ov, yellow->first, yellow->second);
  EXPECT_TRUE(after.even);
  EXPECT_EQ(after.value, 4);

  // Every other clue is untouched.
  const auto red = reactive_assignment(v, ov, ClueKind::COLOUR, 0);
  EXPECT_FALSE(red.even);
  EXPECT_EQ(red.value, 1);
}

// --- defaults must reproduce the old behaviour ----------------------------

// With no overrides the assignment IS the built-in table. This is what keeps
// every existing reactor0 test meaningful.
TEST(Reactor0ReactiveAssignment, EmptyOverridesReproduceTheBuiltInTable) {
  for (const char* name : {"No Variant", "Orange (3 Suits)",
                           "Odds and Evens & Orange (3 Suits)",
                           "Black (6 Suits)"}) {
    const Variant& v = get_variant(name);
    for (int r : v.clue_ranks) {
      const auto a = reactive_assignment(v, kNone, ClueKind::RANK, r);
      EXPECT_EQ(a.even, hanabi::reactor::variants::uses_even_parity(v, ClueKind::RANK))
          << name << " rank " << r;
      EXPECT_EQ(a.value, hanabi::reactor::variants::rank_reactive_value(v, r))
          << name << " rank " << r;
    }
    for (int i = 0; i < static_cast<int>(v.clue_colour_names.size()); ++i) {
      const auto a = reactive_assignment(v, kNone, ClueKind::COLOUR, i);
      EXPECT_EQ(a.even,
                hanabi::reactor::variants::uses_even_parity(v, ClueKind::COLOUR))
          << name << " colour " << i;
      EXPECT_EQ(a.value, hanabi::reactor0::colour_clue_value(v, i))
          << name << " colour " << i;
    }
  }
}

// `clue_ranks` decides which rank rows exist, so the Number Mute family
// contributes none at all.
TEST(Reactor0ReactiveAssignment, NumberMuteHasNoRankRows) {
  const Variant& v = get_variant("Number Mute (5 Suits)");
  const auto table = build_reactive_table(v, kNone);
  for (const auto& row : table.even) {
    EXPECT_FALSE(row.label.size() == 1 && row.label[0] >= '1' && row.label[0] <= '5')
        << "no rank clue exists in this variant, but the table lists " << row.label;
  }
  EXPECT_TRUE(table.even.empty())
      << "ranks are the even bucket here, and there are no rank clues";
  EXPECT_EQ(table.odd.size(), v.clue_colour_names.size());
}

// --- label parsing --------------------------------------------------------

TEST(Reactor0ReactiveAssignment, ParsesColoursAndRanks) {
  const Variant& v = get_variant("No Variant");
  auto four = parse_clue_label(v, "4");
  ASSERT_TRUE(four.has_value());
  EXPECT_EQ(four->first, ClueKind::RANK);
  EXPECT_EQ(four->second, 4);

  EXPECT_FALSE(parse_clue_label(v, "Teal").has_value())
      << "not a colour in this variant";
  EXPECT_FALSE(parse_clue_label(v, "9").has_value());
  EXPECT_FALSE(parse_clue_label(v, "").has_value());
}

// Under Odds and Evens the rank clues answer to their labels AND to the raw
// digits, since the table shows them as Odd / Even.
TEST(Reactor0ReactiveAssignment, ParsesOddAndEvenLabels) {
  const Variant& v = get_variant("Odds and Evens & Orange (3 Suits)");
  EXPECT_EQ(clue_label(v, ClueKind::RANK, 1), "Odd");
  EXPECT_EQ(clue_label(v, ClueKind::RANK, 2), "Even");

  for (const auto& [text, want] : std::vector<std::pair<std::string, int>>{
           {"Odd", 1}, {"odd", 1}, {"1", 1}, {"Even", 2}, {"even", 2}, {"2", 2}}) {
    auto got = parse_clue_label(v, text);
    ASSERT_TRUE(got.has_value()) << text;
    EXPECT_EQ(got->first, ClueKind::RANK) << text;
    EXPECT_EQ(got->second, want) << text;
  }
  EXPECT_FALSE(parse_clue_label(v, "3").has_value())
      << "rank 3 is not a clue in this variant";
}
