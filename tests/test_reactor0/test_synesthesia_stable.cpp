// Synesthesia's stable clue convention (CONVENTION.md §1f, v11.0.0).
//
// Synesthesia can never give a rank clue -- it carries `clueRanks: []` -- so
// once target parity stands down at 50% and a clue to Bob is stable again, the
// ordinary colour/rank ladders have nothing to express it with. Its clue colours
// instead name an action outright, from a fixed table:
//
//     Red 1 pitch | Yellow 2 pitch | Green 3 pitch | Blue 4 pitch
//     Purple 5 pitch | Orange 1 chuck | anything else 4 chuck
//
// ...and, while every playable card Bob could hold is INVERTED, every pitch row
// reads as a chuck at the same slot (the v14.0.0 flip, pinned at the foot).
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
      {"Green", kPitch, 3},
      // Blue moved from chuck/2 to pitch/4 in v14.0.0, which is what brought the
      // five named colours into line with `colour_clue_value` (Red=1 .. Purple=5).
      // They agree on these five and nowhere else -- Orange and the catch-all
      // still diverge, and `colour_clue_value` has no button at all.
      {"Blue", kPitch, 4},
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

// v14.0.0: the catch-all is a CHUCK of slot 4, where it used to be a pitch.
TEST(Reactor0Synesthesia, EveryOtherColourChucksSlotFour) {
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
    EXPECT_EQ(c.button, kChuck) << variant;
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

// A clue to Bob is STABLE at pace <= 1 (v14.0.0, was a score fraction).
//
// With stacks of 3 across five suits, `pace() == 13 - discards`: the score and
// the cards it consumed cancel out of `score + cards_left`, so the ONLY lever is
// how many cards have been thrown away. Every card in the pool below is a spare
// -- a second 1 or 2 of a suit already on 3 -- so none of them moves `max_score`,
// which pace also reads.
constexpr int kSynBasePace = 13;

std::vector<std::string> syn_discards_for_pace(int target_pace) {
  // Every entry is a spare copy that NO fixture below puts in a hand, so the
  // pool is safe whichever Bob hand a test substitutes. Twelve is exactly what
  // pace 1 costs; there is no slack, so a new fixture that wants one of these
  // cards has to grow the pool rather than borrow from it.
  static const std::vector<std::string> kPool = {
      "y1", "y1", "r3", "y3", "g3", "b2",
      "b3", "p2", "p3", "b1", "p1", "g1"};
  std::vector<std::string> out;
  for (int i = 0; i < kSynBasePace - target_pace && i < (int)kPool.size(); ++i) {
    out.push_back(kPool[i]);
  }
  return out;
}

// Bob's slot 1 is playable, slot 3 is trash. `target_pace` 1 or 0 puts the
// fixture on the STABLE side of the switch; 2 or more leaves it reactive.
SetupOptions syn_opts(int target_pace = 1) {
  SetupOptions opts;
  opts.variant_name = "Synesthesia (5 Suits)";
  opts.play_stacks = {3, 3, 3, 3, 3};
  opts.clue_tokens = 7;
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r5", "y5", "g5", "b5", "p5"},   // Alice
      {"r4", "y2", "g1", "b4", "p4"},   // Bob: slot 1 playable, slot 3 trash
      {"y4", "g4", "b1", "p1", "r1"},   // Cathy
  };
  opts.discarded = syn_discards_for_pace(target_pace);
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
  ASSERT_EQ(g.state.pace(), 1) << "guard: on the stable side of the switch";
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

// v14.0.0: Green moved from chuck/3 to PITCH/3. Bob's slot 3 is a g1, which is
// trash with green on 3 -- so the pitch is unobeyable and the clue is a STALL,
// which is the interesting half: the table names the slot either way, and what
// changed is what Bob is asked to do with it.
TEST(Reactor0Synesthesia, GreenPitchesSlotThree) {
  Game g = setup(syn_opts());
  EXPECT_EQ(call_for("Synesthesia (5 Suits)", "Green").button,
            CardStatus::CALLED_TO_PLAY);
  Game h = clued(g, "Green", TestPlayer::BOB);
  EXPECT_EQ(h.meta[order_at(h, TestPlayer::BOB, 3)].status, CardStatus::NONE)
      << "a dead g1 cannot be pitched, so nothing is stamped";
}

TEST(Reactor0Synesthesia, AbovePaceOneTheSameClueIsStillReactive) {
  Game g = setup(syn_opts(/*target_pace=*/2));
  ASSERT_EQ(g.state.pace(), 2);
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

// --- the inverted flip (v14.0.0) ------------------------------------------
//
// When the only playable cards Bob could be holding are on an INVERTED suit,
// every pitch row of the table reads as a chuck instead. Pressing Play on an
// orange throws it away and pressing Discard stacks it, so an unflipped pitch
// call would be unobeyable there.
//
// The fixtures drive the condition through the STACKS rather than through Bob's
// empathy, which is what makes them readable: with every plain suit complete,
// `is_playable` is true of orange identities and nothing else, whatever Bob's
// cards might turn out to be.

namespace {

SetupOptions orange_opts(std::vector<int> stacks) {
  SetupOptions opts;
  opts.variant_name = "Synesthesia & Orange (5 Suits)";
  opts.play_stacks = std::move(stacks);
  opts.clue_tokens = 7;
  opts.starting = TestPlayer::ALICE;
  // Every card here is a SPARE of a suit the stacks have finished with, so the
  // layout is legal at each of the three stack settings below. Critically, the
  // single r5 is left in the DECK: parking it in a visible hand would prove Bob
  // could not be holding it, and `effective_possible_for` would drop the one
  // plain play the middle fixture is built around.
  opts.hands = {
      {"y1", "g1", "b1", "y2", "g2"},   // Alice
      {"o1", "o2", "o3", "o4", "r4"},   // Bob
      {"r3", "y3", "g3", "b3", "b2"},   // Cathy
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

TEST(Reactor0SynesthesiaFlip, FiresWhenEveryReachablePlayIsInverted) {
  // Four plain suits complete, orange on 0: o1 is the only playable identity in
  // the variant, so every playable reading of every card of Bob's is inverted.
  Game g = setup(orange_opts({5, 5, 5, 5, 0}));
  EXPECT_TRUE(hanabi::reactor0::synesthesia_pitch_flips(
      g, static_cast<int>(TestPlayer::BOB)));
}

TEST(Reactor0SynesthesiaFlip, DoesNotFireWhileAPlainPlayIsReachable) {
  // Red on 4, so r5 is playable and plain. One reachable plain play is enough:
  // the pitch button stays obeyable and the table stands.
  Game g = setup(orange_opts({4, 5, 5, 5, 0}));
  ASSERT_TRUE(g.state.playable_set.contains(Identity{0, 5}))
      << "guard: r5 is playable, plain, and still in the deck";
  EXPECT_FALSE(hanabi::reactor0::synesthesia_pitch_flips(
      g, static_cast<int>(TestPlayer::BOB)));
}

// The NON-VACUOUS half, and the reason it is written that way: "every playable
// reading is inverted" is trivially true of a hand with no playable reading at
// all, and thirty of the thirty-six Synesthesia variants have no inverted suit
// for the rule to be about. A vacuous flip would rewrite the table in all of
// them.
TEST(Reactor0SynesthesiaFlip, DoesNotFireWithNothingPlayableAtAll) {
  Game g = setup(orange_opts({5, 5, 5, 5, 5}));
  ASSERT_TRUE(g.state.playable_set.is_empty()) << "guard: the game is complete";
  EXPECT_FALSE(hanabi::reactor0::synesthesia_pitch_flips(
      g, static_cast<int>(TestPlayer::BOB)));
}

TEST(Reactor0SynesthesiaFlip, IsAbsentFromAVariantWithNoInvertedSuit) {
  // Plain Synesthesia, everything played but one plain suit: the only reachable
  // play is plain, so the rule is inert exactly as it should be.
  Game g = setup(syn_opts());
  EXPECT_FALSE(hanabi::reactor0::synesthesia_pitch_flips(
      g, static_cast<int>(TestPlayer::BOB)));
}

// End to end: at pace 1 the clue is stable, the flip is active, and Red -- an
// `f1` in the table -- stamps a CHUCK on slot 1 rather than a pitch.
//
// Three suits keeps the arithmetic small: 30 cards, 15 dealt, so
// `pace() == 3 - discards` and two spare 1s are enough to reach the switch.
//
// The contrast is the point. Bob's slot 1 is a playable o1. Pressing Discard on
// an orange STACKS it; pressing Play THROWS IT AWAY. Unflipped, Red would have
// named a pitch of a playable orange -- and the giver-only vet would not have
// stopped it, because a spare o1 is not critical.
TEST(Reactor0SynesthesiaFlip, AFlippedRedChucksSlotOneInsteadOfPitchingIt) {
  SetupOptions opts;
  opts.variant_name = "Synesthesia & Orange (3 Suits)";
  opts.play_stacks = {5, 5, 0};   // Red, Blue, Orange
  opts.clue_tokens = 7;
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"b2", "b3", "b4", "o5", "o4"},   // Alice
      {"o1", "o2", "o3", "r1", "b1"},   // Bob: slot 1 is a playable o1
      {"r2", "r3", "r4", "o1", "o2"},   // Cathy
  };
  opts.discarded = {"r1", "b1"};        // the two spares that reach pace 1
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.state.pace(), 1) << "guard: a clue to Bob is stable here";
  ASSERT_FALSE(hanabi::reactor0::bob_clue_is_reactive(g.state));
  ASSERT_TRUE(hanabi::reactor0::synesthesia_pitch_flips(
      g, static_cast<int>(TestPlayer::BOB)))
      << "guard: red and blue are complete, so o1 is the only play left";
  ASSERT_EQ(call_for("Synesthesia & Orange (3 Suits)", "Red").button,
            CardStatus::CALLED_TO_PLAY)
      << "guard: the UNFLIPPED table still calls Red a pitch";

  Game h = clued(g, "Red", TestPlayer::BOB);
  expect_status(h, TestPlayer::BOB, 1, CardStatus::CALLED_TO_DISCARD);
  EXPECT_TRUE(h.waiting.empty()) << "a stable clue pends no reaction";
}
