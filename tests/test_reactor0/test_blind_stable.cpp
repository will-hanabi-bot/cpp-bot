// reactor0's stable convention for the BLIND families (CONVENTION.md §1f).
//
// A blind clue touches nothing, so it can say nothing about WHICH cards it
// reached: its whole meaning is a fixed table.
//
//   colour   Red f1 | Yellow f2 | Green f3 | Blue f4 | Purple f5 | other LOCK
//   rank     1 d1 | 2 d2 | 3 d3 | 4 d4 | 5 LOCK
//
// `f` is a pitch (press Play), `d` a chuck (press Discard). Keyed on the clue's
// own KIND, so Color Blind leaves rank clues on the ordinary ladder and Totally
// Blind is simply both tables at once.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/blind_stable.h"
#include "hanabi/conventions/reactor0/reactive_assignment.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::blind_call;
using hanabi::reactor0::BlindCall;
using hanabi::reactor0::clue_kind_is_blind;

namespace {

constexpr CardStatus kPitch = CardStatus::CALLED_TO_PLAY;
constexpr CardStatus kChuck = CardStatus::CALLED_TO_DISCARD;

int colour_index(const Variant& v, const std::string& name) {
  for (size_t i = 0; i < v.clue_colour_names.size(); ++i) {
    if (v.clue_colour_names[i] == name) return static_cast<int>(i);
  }
  return -1;
}

// Alice clues Bob. Every fixture here is a STABLE clue, which under reactor0 is
// exactly a clue to the seat that acts next.
Game clued(const Game& g, ClueKind kind, int value) {
  const int bob = static_cast<int>(TestPlayer::BOB);
  ClueAction act{g.state.our_player_index, bob,
                 g.state.clue_touched(g.state.hands[bob], kind, value),
                 BaseClue{kind, value}};
  return g.simulate(Action{act});
}

// Bob's slot 1 is a playable r1 and slot 4 a dead r1 -- so a pitch of slot 1 and
// a chuck of slot 4 are both legal calls, which is what most cases below need.
SetupOptions blind_opts(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 7;
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"g5", "b5", "g4", "b4", "g3"},   // Alice -- nothing playable
      {"r1", "r4", "r5", "g1", "b1"},   // Bob
      {"b3", "g2", "b2", "r3", "r2"},   // Cathy
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

// --- the tables ------------------------------------------------------------

TEST(Reactor0Blind, TheColourTableNamesAPitchPerColour) {
  const Variant& v = get_variant("Color Blind (5 Suits)");
  const std::pair<const char*, int> rows[] = {
      {"Red", 1}, {"Yellow", 2}, {"Green", 3}, {"Blue", 4}, {"Purple", 5}};
  for (const auto& [name, slot] : rows) {
    const int idx = colour_index(v, name);
    ASSERT_GE(idx, 0) << name;
    BlindCall c = blind_call(v, ClueKind::COLOUR, idx);
    EXPECT_FALSE(c.lock) << name;
    EXPECT_EQ(c.button, kPitch) << name;
    EXPECT_EQ(c.slot, slot) << name;
  }
}

// The sixth colour has no slot of its own, so it locks. This is the only way a
// COLOUR clue locks, and it exists only in the 6-suit members.
TEST(Reactor0Blind, AnyOtherColourLocks) {
  const Variant& v = get_variant("Color Blind (6 Suits)");
  const int teal = colour_index(v, "Teal");
  ASSERT_GE(teal, 0) << "guard: the 6-suit member's extra colour";
  EXPECT_TRUE(blind_call(v, ClueKind::COLOUR, teal).lock);

  // ...and the five named colours still do not, in the same variant.
  EXPECT_FALSE(blind_call(v, ClueKind::COLOUR, colour_index(v, "Red")).lock);
}

TEST(Reactor0Blind, TheRankTableNamesAChuckAndFiveLocks) {
  const Variant& v = get_variant("Number Blind (5 Suits)");
  for (int rank = 1; rank <= 4; ++rank) {
    BlindCall c = blind_call(v, ClueKind::RANK, rank);
    EXPECT_FALSE(c.lock) << rank;
    EXPECT_EQ(c.button, kChuck) << "a rank names a CHUCK where a colour names a "
                                   "pitch -- rank " << rank;
    EXPECT_EQ(c.slot, rank) << rank;
  }
  EXPECT_TRUE(blind_call(v, ClueKind::RANK, 5).lock);
}

// --- the dispatch is per KIND, not per variant -----------------------------

TEST(Reactor0Blind, OnlyTheBlindedKindUsesTheTable) {
  const Variant& cb = get_variant("Color Blind (5 Suits)");
  EXPECT_TRUE(clue_kind_is_blind(cb, ClueKind::COLOUR));
  EXPECT_FALSE(clue_kind_is_blind(cb, ClueKind::RANK));

  const Variant& nb = get_variant("Number Blind (5 Suits)");
  EXPECT_FALSE(clue_kind_is_blind(nb, ClueKind::COLOUR));
  EXPECT_TRUE(clue_kind_is_blind(nb, ClueKind::RANK));

  const Variant& tb = get_variant("Totally Blind (5 Suits)");
  EXPECT_TRUE(clue_kind_is_blind(tb, ClueKind::COLOUR));
  EXPECT_TRUE(clue_kind_is_blind(tb, ClueKind::RANK));

  const Variant& plain = get_variant("No Variant");
  EXPECT_FALSE(clue_kind_is_blind(plain, ClueKind::COLOUR));
  EXPECT_FALSE(clue_kind_is_blind(plain, ClueKind::RANK));
}

// In Color Blind a RANK clue is an ordinary clue that really touches cards, so it
// must NOT be read off the blind table -- a "3" there is a normal rank clue, not
// a chuck of slot 3.
TEST(Reactor0Blind, ColorBlindLeavesRankCluesOnTheOrdinaryLadder) {
  Game g = setup(blind_opts("Color Blind (5 Suits)"));
  // Bob's r1/r4/r5/g1/b1: a rank-1 clue touches slots 1, 4 and 5.
  Game h = clued(g, ClueKind::RANK, 1);
  EXPECT_EQ(h.meta[order_at(h, TestPlayer::BOB, 4)].status, CardStatus::NONE)
      << "the blind table would have chucked slot 4; the ordinary ladder must "
         "not";
  EXPECT_TRUE(h.state.deck[order_at(h, TestPlayer::BOB, 1)].clued)
      << "guard: a rank clue really does touch in Color Blind";
}

// --- the interpretation ----------------------------------------------------

TEST(Reactor0Blind, AColourClueStampsThePitchItNames) {
  Game g = setup(blind_opts("Totally Blind (5 Suits)"));
  const int red = colour_index(*g.state.variant, "Red");
  Game h = clued(g, ClueKind::COLOUR, red);

  expect_status(h, TestPlayer::BOB, 1, CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(h.waiting.empty()) << "a stable clue pends no reaction";
  EXPECT_FALSE(h.state.deck[order_at(h, TestPlayer::BOB, 1)].clued)
      << "the call is positional -- the card is named, not touched";
}

TEST(Reactor0Blind, ARankClueStampsTheChuckItNames) {
  Game g = setup(blind_opts("Totally Blind (5 Suits)"));
  // Rank 2 = chuck slot 2, and Bob's slot 2 is an r4 -- not the last copy, so
  // throwing it away is a legal thing to be asked.
  Game h = clued(g, ClueKind::RANK, 2);
  expect_status(h, TestPlayer::BOB, 2, CardStatus::CALLED_TO_DISCARD);
}

TEST(Reactor0Blind, TheLockRowStampsTheWholeHand) {
  Game g = setup(blind_opts("Totally Blind (5 Suits)"));
  Game h = clued(g, ClueKind::RANK, 5);
  for (int slot = 1; slot <= 5; ++slot) {
    EXPECT_EQ(h.meta[order_at(h, TestPlayer::BOB, slot)].status,
              CardStatus::CHOP_MOVED)
        << "rank 5 locks, so every card of the hand is chop-moved (slot " << slot
        << ")";
  }
}

// A slot the hand does not have names nothing, so the clue is a stall rather than
// a mistake -- the giver and the receiver can both see the hand is too short.
TEST(Reactor0Blind, AnOutOfRangeSlotIsAStall) {
  SetupOptions opts = blind_opts("Totally Blind (5 Suits)");
  opts.hands[1] = {"r1", "r4", "r5", "g1"};  // four cards: no slot 5
  Game g = setup(std::move(opts));
  const int purple = colour_index(*g.state.variant, "Purple");
  Game h = clued(g, ClueKind::COLOUR, purple);
  for (int slot = 1; slot <= 4; ++slot) {
    EXPECT_EQ(h.meta[order_at(h, TestPlayer::BOB, slot)].status, CardStatus::NONE)
        << "Purple names slot 5, which is not there -- nothing is stamped";
  }
}

// The giver-only reject: Alice can see Bob's slot 3 is an r5, so pitching it
// would strike. Bob cannot see that, so the clue must never be OFFERED rather
// than quietly degrading to a stall he would not know about.
TEST(Reactor0Blind, AGiverOnlyBadPitchIsNeverOffered) {
  Game g = setup(blind_opts("Totally Blind (5 Suits)"));
  const int green = colour_index(*g.state.variant, "Green");
  ASSERT_GE(green, 0);
  ASSERT_EQ(g.state.log_id_by_order(order_at(g, TestPlayer::BOB, 3)), "r5")
      << "guard: Green names slot 3, and Alice can see it is unplayable";

  auto clues = g.find_all_clues(static_cast<int>(TestPlayer::ALICE));
  bool offered = false;
  for (const auto& perform : clues) {
    if (auto* pc = std::get_if<PerformColour>(&perform)) {
      if (pc->target == static_cast<int>(TestPlayer::BOB) && pc->value == green) {
        offered = true;
      }
    }
  }
  EXPECT_FALSE(offered)
      << "pitching an r5 on an empty red stack strikes, and only Alice can see "
         "it -- so the clue is a MISTAKE and must be dropped";
}

// The `/settings` line a human reads in chat to know the convention in play.
// Pinned verbatim, as the plain and Synesthesia lines are, so it cannot drift
// from what the bot actually says -- which is how README's sample of the
// target-parity line went stale before v14.0.0.
TEST(Reactor0Blind, SettingsReportsTheBlindTables) {
  const std::vector<ReactiveOverride> none;
  EXPECT_EQ(hanabi::reactor0::format_settings(
                get_variant("Totally Blind (5 Suits)"), none, /*rlocks=*/false),
            "reactor0 — even reactive values: {1=1, 2=2, 3=3, 4=4, 5=5}, odd "
            "reactive values: {Red=1, Yellow=2, Green=3, Blue=4, Purple=5}, "
            "COLOUR touches nothing; stable: Red=f1, Yellow=f2, Green=f3, "
            "Blue=f4, Purple=f5, other=lock, RANK touches nothing; stable: "
            "1=d1, 2=d2, 3=d3, 4=d4, 5=lock, rlocks: off");
  EXPECT_EQ(hanabi::reactor0::format_settings(
                get_variant("Color Blind (5 Suits)"), none, /*rlocks=*/false),
            "reactor0 — even reactive values: {1=1, 2=2, 3=3, 4=4, 5=5}, odd "
            "reactive values: {Red=1, Yellow=2, Green=3, Blue=4, Purple=5}, "
            "COLOUR touches nothing; stable: Red=f1, Yellow=f2, Green=f3, "
            "Blue=f4, Purple=f5, other=lock, rlocks: off");
}
