// The BLIND families: a clue of one or both kinds is LEGAL but touches no card.
//
//   Color Blind (3-6 Suits)    colour clues touch nothing
//   Number Blind (3-6 Suits)   rank clues touch nothing
//   Totally Blind (3-6 Suits)  both
//
// Everything here is convention-neutral. Two properties carry the weight:
//
//  1. The clue must still be OFFERED. `all_valid_clues` otherwise requires a
//     clue to touch something, which would leave the bot unable to clue at all in
//     Number Blind and Totally Blind -- silently, and for the whole game.
//  2. The clue must teach NOTHING, negatively as well as positively. "Not touched
//     by Red" implies "not red" everywhere else; here it must imply nothing,
//     because no card is ever touched by Red.
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

bool offers(const std::vector<Clue>& clues, ClueKind kind, int value) {
  return std::any_of(clues.begin(), clues.end(), [&](const Clue& c) {
    return c.kind == kind && c.value == value;
  });
}

SetupOptions blind_opts(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r1", "r2", "r3", "r4", "r5"},
      {"g1", "g2", "g3", "g4", "g5"},
      {"b1", "b2", "b3", "b4", "b5"},
  };
  return opts;
}

}  // namespace

// --- the flags reach the Variant ------------------------------------------

TEST(Blind, TheCatalogFlagsAreParsed) {
  const Variant& cb = get_variant("Color Blind (5 Suits)");
  EXPECT_TRUE(cb.colour_clues_touch_nothing);
  EXPECT_FALSE(cb.rank_clues_touch_nothing);

  const Variant& nb = get_variant("Number Blind (5 Suits)");
  EXPECT_FALSE(nb.colour_clues_touch_nothing);
  EXPECT_TRUE(nb.rank_clues_touch_nothing);

  const Variant& tb = get_variant("Totally Blind (5 Suits)");
  EXPECT_TRUE(tb.colour_clues_touch_nothing);
  EXPECT_TRUE(tb.rank_clues_touch_nothing);

  const Variant& plain = get_variant("No Variant");
  EXPECT_FALSE(plain.colour_clues_touch_nothing);
  EXPECT_FALSE(plain.rank_clues_touch_nothing);
}

// --- id_touched, per kind --------------------------------------------------

TEST(Blind, ColorBlindBlindsColourAndLeavesRankAlone) {
  const Variant& v = get_variant("Color Blind (5 Suits)");
  for (int suit = 0; suit < 5; ++suit) {
    for (int rank = 1; rank <= 5; ++rank) {
      const Identity id{suit, rank};
      for (int value = 0; value < 5; ++value) {
        EXPECT_FALSE(v.id_touched(id, ClueKind::COLOUR, value))
            << "no colour clue touches anything in Color Blind";
      }
      // Rank is untouched by the variant and behaves exactly as No Variant.
      EXPECT_TRUE(v.id_touched(id, ClueKind::RANK, rank));
      EXPECT_FALSE(v.id_touched(id, ClueKind::RANK, rank == 5 ? 1 : rank + 1));
    }
  }
}

TEST(Blind, NumberBlindBlindsRankAndLeavesColourAlone) {
  const Variant& v = get_variant("Number Blind (5 Suits)");
  for (int suit = 0; suit < 5; ++suit) {
    for (int rank = 1; rank <= 5; ++rank) {
      const Identity id{suit, rank};
      for (int value = 1; value <= 5; ++value) {
        EXPECT_FALSE(v.id_touched(id, ClueKind::RANK, value))
            << "no rank clue touches anything in Number Blind";
      }
      EXPECT_TRUE(v.id_touched(id, ClueKind::COLOUR, suit));
      EXPECT_FALSE(v.id_touched(id, ClueKind::COLOUR, (suit + 1) % 5));
    }
  }
}

TEST(Blind, TotallyBlindTouchesNothingAtAll) {
  const Variant& v = get_variant("Totally Blind (5 Suits)");
  for (int suit = 0; suit < 5; ++suit) {
    for (int rank = 1; rank <= 5; ++rank) {
      const Identity id{suit, rank};
      for (int value = 0; value < 5; ++value) {
        EXPECT_FALSE(v.id_touched(id, ClueKind::COLOUR, value));
      }
      for (int value = 1; value <= 5; ++value) {
        EXPECT_FALSE(v.id_touched(id, ClueKind::RANK, value));
      }
    }
  }
}

// --- the clue is still offered --------------------------------------------
//
// The failure this guards is total and silent: a bot that enumerates no clues
// never clues, for the whole game, without erroring.

TEST(Blind, EveryClueIsStillOfferedInTotallyBlind) {
  Game g = setup(blind_opts("Totally Blind (5 Suits)"));
  const auto clues = g.state.all_valid_clues(static_cast<int>(TestPlayer::BOB));
  ASSERT_FALSE(clues.empty()) << "a bot that cannot clue cannot play";
  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_TRUE(offers(clues, ClueKind::RANK, rank)) << "rank " << rank;
  }
  for (int colour = 0; colour < 5; ++colour) {
    EXPECT_TRUE(offers(clues, ClueKind::COLOUR, colour)) << "colour " << colour;
  }
}

TEST(Blind, ColorBlindStillOffersEveryColourAndOnlyTouchingRanks) {
  Game g = setup(blind_opts("Color Blind (5 Suits)"));
  const auto clues = g.state.all_valid_clues(static_cast<int>(TestPlayer::BOB));
  for (int colour = 0; colour < 5; ++colour) {
    EXPECT_TRUE(offers(clues, ClueKind::COLOUR, colour))
        << "colour " << colour << " touches nothing but is legal";
  }
  // Bob holds g1..g5, so every rank touches -- and rank is NOT blinded here, so
  // it goes on offer for the ordinary reason.
  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_TRUE(offers(clues, ClueKind::RANK, rank));
  }
}

// A plain variant must not gain the exception: there a clue touching nothing is
// one the server rejects.
TEST(Blind, APlainVariantStillRequiresAClueToTouchSomething) {
  SetupOptions opts = blind_opts("No Variant");
  Game g = setup(std::move(opts));
  const auto clues = g.state.all_valid_clues(static_cast<int>(TestPlayer::BOB));
  // Bob holds only greens, so no other colour may be offered.
  EXPECT_TRUE(offers(clues, ClueKind::COLOUR, 2)) << "green touches his hand";
  EXPECT_FALSE(offers(clues, ClueKind::COLOUR, 0)) << "red touches nothing";
}

// --- empathy does not move -------------------------------------------------

TEST(Blind, ABlindClueTeachesNothingPositiveOrNegative) {
  Game g = setup(blind_opts("Totally Blind (5 Suits)"));
  const int bob = static_cast<int>(TestPlayer::BOB);
  std::vector<IdentitySet> before;
  for (int o : g.state.hands[bob]) before.push_back(g.common.thoughts[o].possible);

  ClueAction act{g.state.our_player_index, bob,
                 g.state.clue_touched(g.state.hands[bob], ClueKind::COLOUR, 0),
                 BaseClue{ClueKind::COLOUR, 0}};
  ASSERT_TRUE(act.list_.empty()) << "guard: the clue touches nothing";
  Game after = g.simulate(Action{act});

  for (size_t i = 0; i < before.size(); ++i) {
    const int o = after.state.hands[bob][i];
    EXPECT_EQ(after.common.thoughts[o].possible, before[i])
        << "slot " << (i + 1) << ": a blind clue removes no identity -- not the "
           "ones it would have touched, and not the ones it would not";
    EXPECT_FALSE(after.state.deck[o].clued)
        << "an untouched card is not clued";
  }
}
