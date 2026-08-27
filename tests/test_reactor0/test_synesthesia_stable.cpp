// Synesthesia's stable clue convention (CONVENTION.md §1f, v11.0.0).
//
// Synesthesia can never give a rank clue -- it carries `clueRanks: []` -- so
// once target parity stands down at 60% and a clue to Bob is stable again, the
// ordinary colour/rank ladders have nothing to express it with. Its clue colours
// instead name an action outright, from a fixed table:
//
//     Red 1 pitch | Yellow 2 pitch | Green 3 chuck | Blue 2 chuck
//     Purple 5 pitch | Orange 1 chuck | anything else 4 pitch
//
// *pitch* is pressing Play, *chuck* is pressing Discard -- the glossary's names
// for the two buttons, which an inverted suit swaps the effects of.
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include <unordered_map>
#include <utility>
#include "hanabi/conventions/reactor0/interpret_reactive.h"
#include "hanabi/conventions/reactor0/synesthesia_stable.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::synesthesia_call;
using hanabi::reactor0::SynesthesiaCall;

namespace {

int colour_index(const Variant& v, const std::string& name) {
  for (size_t i = 0; i < v.clue_colour_names.size(); ++i) {
    if (v.clue_colour_names[i] == name) return static_cast<int>(i);
  }
  return -1;
}

SynesthesiaCall call_for(const std::string& variant_name,
                         const std::string& colour) {
  const Variant& v = get_variant(variant_name);
  const int i = colour_index(v, colour);
  EXPECT_GE(i, 0) << colour << " is not a clue colour of " << variant_name;
  return synesthesia_call(v, i);
}

constexpr CardStatus kPitch = CardStatus::CALLED_TO_PLAY;
constexpr CardStatus kChuck = CardStatus::CALLED_TO_DISCARD;

}  // namespace

// --- the table ------------------------------------------------------------

TEST(Reactor0Synesthesia, EveryNamedColourNamesItsAction) {
  struct Row { const char* colour; CardStatus button; int slot; };
  const Row rows[] = {
      {"Red", kPitch, 1},
      {"Yellow", kPitch, 2},
      {"Green", kChuck, 3},
      // The one that is NOT the existing `colour_clue_value` table, which gives
      // Blue 4. Starred in the ruling, so it is starred here.
      {"Blue", kChuck, 2},
      {"Purple", kPitch, 5},
  };
  for (const Row& r : rows) {
    SynesthesiaCall c = call_for("Synesthesia (5 Suits)", r.colour);
    EXPECT_EQ(c.button, r.button) << r.colour << " button";
    EXPECT_EQ(c.slot, r.slot) << r.colour << " slot";
  }
}

TEST(Reactor0Synesthesia, OrangeChucksSlotOneAndDarkOrangeIsOrange) {
  SynesthesiaCall plain = call_for("Synesthesia & Orange (6 Suits)", "Orange");
  EXPECT_EQ(plain.button, kChuck);
  EXPECT_EQ(plain.slot, 1);

  // `data/suits.json`: Dark Orange carries `clueColors: ["Orange"]`, so it is
  // clued as Orange and takes the Orange row. That is the case where chuck and
  // pitch carry their literal inverted meanings, so it must not fall through to
  // the catch-all.
  SynesthesiaCall dark = call_for("Synesthesia & Dark Orange (6 Suits)", "Orange");
  EXPECT_EQ(dark.button, kChuck);
  EXPECT_EQ(dark.slot, 1);
}

TEST(Reactor0Synesthesia, EveryOtherColourPitchesSlotFour) {
  const std::pair<const char*, const char*> cases[] = {
      {"Synesthesia & Black (6 Suits)", "Black"},
      {"Synesthesia & Brown (6 Suits)", "Brown"},
      // Teal is the sixth suit of plain Synesthesia (6 Suits).
      {"Synesthesia (6 Suits)", "Teal"},
      // Dark Brown clues as "Brown" (suits.json), so it lands here too.
      {"Synesthesia & Dark Brown (6 Suits)", "Brown"},
  };
  for (const auto& [variant, colour] : cases) {
    SynesthesiaCall c = call_for(variant, colour);
    EXPECT_EQ(c.button, kPitch) << variant;
    EXPECT_EQ(c.slot, 4) << variant;
  }
}

// The property that makes a FIXED table safe here, where `colour_clue_value`
// needed a "first untaken" dance: no Synesthesia variant can offer two colours
// that land on the same row, because it offers at most one non-RYGBP suit and
// only some of those are clueable at all.
TEST(Reactor0Synesthesia, NoVariantCanCollideTwoColours) {
  for (const auto& [name, v] : load_variants()) {
    if (!v.synesthesia) continue;
    std::vector<std::pair<int, int>> seen;
    for (size_t i = 0; i < v.clue_colour_names.size(); ++i) {
      SynesthesiaCall c = synesthesia_call(v, static_cast<int>(i));
      std::pair<int, int> key{static_cast<int>(c.button), c.slot};
      for (const auto& prev : seen) {
        EXPECT_NE(prev, key)
            << name << ": two clue colours name the same action, so the fixed "
                       "table is ambiguous there";
      }
      seen.push_back(key);
    }
  }
}

// --- the interpretation ---------------------------------------------------

namespace {

// Five suits -> cap 25 -> the switch is at 15, so stacks of 3 put every fixture
// on the stable side. Bob's slot 1 is playable, slot 3 is trash.
SetupOptions syn_opts() {
  SetupOptions opts;
  opts.variant_name = "Synesthesia (5 Suits)";
  opts.play_stacks = {3, 3, 3, 3, 3};
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r5", "y5", "g5", "b5", "p5"},   // Alice
      {"r4", "y2", "g1", "b4", "p4"},   // Bob: slot 1 playable, slot 3 trash
      {"y4", "g4", "b1", "p1", "r1"},   // Cathy
  };
  use_reactor0(opts);
  return opts;
}

Game clued(const Game& g, const std::string& colour, TestPlayer to) {
  const int v = colour_index(*g.state.variant, colour);
  EXPECT_GE(v, 0);
  ClueAction act{g.state.our_player_index, static_cast<int>(to),
                 g.state.clue_touched(g.state.hands[static_cast<int>(to)],
                                      ClueKind::COLOUR, v),
                 BaseClue{ClueKind::COLOUR, v}};
  return g.simulate(Action{act});
}

}  // namespace

// Every fixture below has to SEPARATE the table from `stable_colour`, or it
// passes against either. Synesthesia's touch rule is that a colour clue of value
// v touches its own colour AND every card of rank v+1, so Red (index 0) touches
// the reds and the ones -- and the table's slot 1 is chosen to be neither.
TEST(Reactor0Synesthesia, PastTheThresholdAClueToBobIsStableAndNamesTheSlot) {
  SetupOptions opts = syn_opts();
  // Red = pitch slot 1. Slot 1 is a playable y4, which RED DOES NOT TOUCH; the
  // reds it does touch sit at slots 2 and 3. The ordinary ladder would stamp one
  // of those, so a stamp on slot 1 can only have come from the table.
  opts.hands[1] = {"y4", "r2", "g1", "b4", "p4"};
  Game g = setup(std::move(opts));
  ASSERT_EQ(g.state.score(), 15) << "guard: on the stable side of the switch";
  ASSERT_FALSE(hanabi::reactor0::bob_clue_is_reactive(g.state));

  Game h = clued(g, "Red", TestPlayer::BOB);
  EXPECT_TRUE(h.waiting.empty())
      << "a stable clue pends no reaction, so there is no waiting connection";
  expect_status(h, TestPlayer::BOB, 1, CardStatus::CALLED_TO_PLAY);
  EXPECT_FALSE(h.meta[order_at(h, TestPlayer::BOB, 1)].urgent)
      << "stable: it names an action without demanding the very next turn";
  for (int slot : {2, 3, 4, 5}) {
    EXPECT_EQ(h.meta[order_at(h, TestPlayer::BOB, slot)].status,
              CardStatus::NONE)
        << "the colour names slot 1 outright, so nothing it TOUCHED is stamped "
           "(slot " << slot << ")";
  }
}

TEST(Reactor0Synesthesia, GreenChucksSlotThree) {
  Game g = setup(syn_opts());
  // Green = chuck slot 3; Bob's slot 3 is a g1, trash with green on 3.
  Game h = clued(g, "Green", TestPlayer::BOB);
  expect_status(h, TestPlayer::BOB, 3, CardStatus::CALLED_TO_DISCARD);
}

TEST(Reactor0Synesthesia, BelowTheThresholdTheSameClueIsStillReactive) {
  SetupOptions opts = syn_opts();
  opts.play_stacks = {3, 3, 3, 3, 2};  // 14 of 25
  Game g = setup(std::move(opts));
  ASSERT_EQ(g.state.score(), 14);
  ASSERT_TRUE(hanabi::reactor0::bob_clue_is_reactive(g.state));

  Game h = clued(g, "Red", TestPlayer::BOB);
  EXPECT_FALSE(h.waiting.empty())
      << "below the switch a clue to Bob is reactive, so it pends a connection";
}

// The vet, on COMMON knowledge, so giver and receiver agree that no call was
// made. A pinned trash card cannot be pitched -- Play would strike -- so the
// clue stamps nothing and is a stall.
TEST(Reactor0Synesthesia, AnUnpitchableSlotMakesTheClueAStall) {
  SetupOptions opts = syn_opts();
  // Red = pitch slot 1, and slot 1 is an r1 -- trash with red on 3 -- which is
  // fully clued, so Bob's OWN empathy knows it. Common knowledge can see that
  // Play would strike, so nothing is stamped and the clue is a stall.
  //
  // Slot 2 is a playable r4 that Red also touches: the ordinary ladder would
  // gladly stamp it, so "nothing anywhere" is what separates the two readings.
  opts.hands[1] = {"r1", "r4", "g4", "b4", "p4"};
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, 1, "r1");

  Game h = clued(g, "Red", TestPlayer::BOB);
  for (int slot = 1; slot <= 5; ++slot) {
    EXPECT_EQ(h.meta[order_at(h, TestPlayer::BOB, slot)].status,
              CardStatus::NONE)
        << "the named action is bad by common knowledge, so the clue stamps "
           "NOTHING and is a stall (slot " << slot << ")";
  }
  EXPECT_TRUE(h.waiting.empty());
}

// The other side of §1g. Here the empathy still admits a good pitch, so the
// common vet passes and Bob would act -- but Alice can SEE the card is trash.
// She must not give the clue at all; degrading it to a stall would be a
// cancellation Bob has no way to decode.
TEST(Reactor0Synesthesia, AGiverOnlyBadPitchIsNeverOffered) {
  SetupOptions opts = syn_opts();
  // Red = pitch slot 1. Slot 1 is a g2 -- trash with green on 3, and NOT touched
  // by Red -- while the r4 Red does touch is playable. So the ordinary ladder
  // would happily offer this clue, and only the table makes it a mistake.
  opts.hands[1] = {"g2", "r4", "y2", "b4", "p4"};
  Game g = setup(std::move(opts));

  const int red = colour_index(*g.state.variant, "Red");
  ASSERT_GE(red, 0);
  auto clues = g.find_all_clues(static_cast<int>(TestPlayer::ALICE));
  bool offered = false;
  for (const auto& perform : clues) {
    if (auto* pc = std::get_if<PerformColour>(&perform)) {
      if (pc->target == static_cast<int>(TestPlayer::BOB) && pc->value == red) {
        offered = true;
      }
    }
  }
  EXPECT_FALSE(offered)
      << "Red pitches Bob's slot 1, which Alice can see is a trash r1 -- a "
         "MISTAKE, so find_all_clues must drop it";
}

// Alternating Clues has both kinds, so its stable clues keep the ordinary
// ladders. The table is Synesthesia's alone.
TEST(Reactor0Synesthesia, AlternatingCluesDoesNotUseTheTable) {
  SetupOptions opts = syn_opts();
  opts.variant_name = "Alternating Clues (5 Suits)";
  Game g = setup(std::move(opts));
  ASSERT_FALSE(hanabi::reactor0::bob_clue_is_reactive(g.state))
      << "guard: past the switch, so a clue to Bob is stable here too";

  // Green would be "chuck slot 3" under the table. Bob's slot 3 is a trash g1,
  // so the table would stamp CTD there. The ordinary colour ladder must not.
  Game h = clued(g, "Green", TestPlayer::BOB);
  EXPECT_NE(h.meta[order_at(h, TestPlayer::BOB, 3)].status,
            CardStatus::CALLED_TO_DISCARD)
      << "Alternating Clues keeps stable_colour, which reads what the clue "
         "TOUCHES rather than a slot the colour names";
}

// The positive counterpart of the MISTAKE test above. A convention the bot can
// READ but never GIVE is half a convention, so assert the giver side directly
// rather than inferring it from the reading.
TEST(Reactor0Synesthesia, TheGiverActuallyOffersAGoodTableClue) {
  SetupOptions opts = syn_opts();
  // Red = pitch slot 1, and slot 1 is a playable r4 that Red does not touch.
  opts.hands[1] = {"r4", "y2", "g1", "b4", "p4"};
  Game g = setup(std::move(opts));

  const int red = colour_index(*g.state.variant, "Red");
  ASSERT_GE(red, 0);
  bool offered = false;
  for (const auto& perform : g.find_all_clues(static_cast<int>(TestPlayer::ALICE))) {
    if (auto* pc = std::get_if<PerformColour>(&perform)) {
      if (pc->target == static_cast<int>(TestPlayer::BOB) && pc->value == red) {
        offered = true;
      }
    }
  }
  EXPECT_TRUE(offered)
      << "Red pitches a playable r4, so it reads cleanly and the giver must be "
         "able to offer it";
}
