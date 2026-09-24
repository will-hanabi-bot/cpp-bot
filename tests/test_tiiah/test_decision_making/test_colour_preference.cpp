// Throw It in a Hole prefers a play clue the receiver can NAME
// (tiiah/CONVENTION.md §2; reactor0/DECISION_MAKING.md §3.1's carve-out).
//
// Replay 2008145 T1 is what this is for. Stacks empty, Bob holding `g1 r1 y1`
// on slots 1-3, and the bot gave rank 1 — which touches all three and so wins
// the default tiebreak 5.97 to 1.99. Under this variant that is backwards: the
// rank call leaves Bob choosing between five identities, so when he plays it he
// learns nothing, the card goes in the hole unnamed and `common_play_stacks`
// never advances. A colour clue names the card outright.
//
// The rule is implemented as the principle rather than as "colour beats rank":
// a call is preferred when the receiver can read it back to ONE identity. The
// spec's exception — a rank whose identity is pinned because only one card of
// that rank is playable — then falls out of it, and so does a play reveal,
// which names its card by construction. The reveal has no fixture here: in a
// plain five-suit hand the rank ladder reads one as a STALL, so it never
// reaches the stable-play pool at all and there is nothing to demote.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;
using hanabi::reactor0::ClueCandidate;
using hanabi::reactor0::ClueShape;

namespace {

// Every clue Alice could legally give, analysed — the same set and the same
// single-simulation pass `take_action` hands to `choose_clue`. Mirrors the
// helper in `test_reactor0/test_decision_making/test_clue_priority.cpp`.
std::vector<std::pair<PerformAction, Action>> all_candidate_clues(const Game& g) {
  const State& s = g.state;
  std::vector<std::pair<PerformAction, Action>> out;
  for (int target = 0; target < s.num_players; ++target) {
    if (target == s.our_player_index) continue;
    for (const Clue& clue : s.all_valid_clues(target)) {
      PerformAction perform =
          clue.kind == ClueKind::COLOUR
              ? PerformAction{PerformColour{clue.target, clue.value}}
              : PerformAction{PerformRank{clue.target, clue.value}};
      ClueAction act{s.our_player_index, clue.target,
                     s.clue_touched(s.hands[target], clue.kind, clue.value),
                     clue.base()};
      out.emplace_back(perform, Action{act});
    }
  }
  return out;
}

// The one candidate matching a kind and value, or nothing.
const ClueCandidate* find_clue(const std::vector<ClueCandidate>& cs, int target,
                               ClueKind kind, int value) {
  for (const ClueCandidate& c : cs) {
    if (c.action.target == target && c.action.clue.kind == kind &&
        c.action.clue.value == value) {
      return &c;
    }
  }
  return nullptr;
}

std::vector<ClueCandidate> analysed(const Game& g) {
  return hanabi::reactor0::analyse_clues(g, all_candidate_clues(g));
}

// Replay 2008145 T1, to the card: stacks empty, Bob holding three playable 1s.
SetupOptions replay_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g1", "r1", "y1", "y5", "r3"},
      {"p5", "p3", "r4", "b5", "g4"},
  };
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 0};
  return opts;
}

}  // namespace

// The behaviour the amendment is about.
TEST(TiiahColourPreference, TheReplayShapeGivesAClueThatNamesItsCard) {
  SetupOptions opts = replay_opts();
  use_tiiah(opts);
  Game g = setup(std::move(opts));

  PerformAction a;
  ASSERT_NO_THROW(a = g.take_action());
  ASSERT_TRUE(hanabi::is_clue(a)) << "a play clue to Bob is the rung that fires";
  EXPECT_TRUE(std::holds_alternative<PerformColour>(a))
      << "rank 1 touches three cards and wins the default tiebreak, but it "
         "leaves Bob choosing between five identities";
  EXPECT_EQ(std::visit([](const auto& v) { return v.target; }, a), 1);
}

// ...and the reason, read off the candidates directly. Rank 1 names nothing;
// each of the three colours names its card.
TEST(TiiahColourPreference, OnlyTheColourCandidatesNameTheirCard) {
  SetupOptions opts = replay_opts();
  use_tiiah(opts);
  Game g = setup(std::move(opts));

  const auto cs = analysed(g);
  const ClueCandidate* rank1 = find_clue(cs, 1, ClueKind::RANK, 1);
  ASSERT_NE(rank1, nullptr);
  EXPECT_EQ(rank1->reading.shape, ClueShape::STABLE_PLAY)
      << "it is a play clue — that is why it was chosen before";
  EXPECT_FALSE(rank1->names_its_card) << "{r1,y1,g1,b1,p1}";

  for (int colour : {0, 1, 2}) {  // red, yellow, green
    const ClueCandidate* c = find_clue(cs, 1, ClueKind::COLOUR, colour);
    ASSERT_NE(c, nullptr) << "colour " << colour;
    EXPECT_EQ(c->reading.shape, ClueShape::STABLE_PLAY) << "colour " << colour;
    EXPECT_TRUE(c->names_its_card) << "colour " << colour;
  }
}

// The spec's exception, derived rather than coded. Every other suit has its 1
// on the stacks, so the r1 is the only 1 a call can mean — and a rank-1 clue
// pins it exactly as a colour clue would.
//
// The rank ladder's own condition makes this the ONLY shape the exception can
// take: a rank clue is a direct play call only when every useful identity of
// that rank is playable (`playable_rank`, reactor0/interpret_clue.cpp), so "one
// playable identity of the rank" means the rest are trash.
//
// Not demoted, then, means the term has nothing to say: both candidates name
// their card, so `settle` skips the term and the default tiebreak decides, as
// it does under reactor0.
TEST(TiiahColourPreference, ARankCallThatPinsItsCardIsNotDemoted) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y4", "b4", "p4", "y3"},
      {"p5", "p3", "r4", "b5", "g4"},
  };
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 1, 1, 1, 1};
  use_tiiah(opts);
  Game g = setup(std::move(opts));

  const auto cs = analysed(g);
  const ClueCandidate* rank1 = find_clue(cs, 1, ClueKind::RANK, 1);
  ASSERT_NE(rank1, nullptr);
  ASSERT_EQ(rank1->reading.shape, ClueShape::STABLE_PLAY);
  EXPECT_TRUE(rank1->names_its_card)
      << "every other 1 is already on its stack, so the call means the r1";

  const ClueCandidate* red = find_clue(cs, 1, ClueKind::COLOUR, 0);
  ASSERT_NE(red, nullptr);
  EXPECT_EQ(red->names_its_card, rank1->names_its_card)
      << "the rule separates nothing here, which is what 'not demoted' means";
}

// The control. The same hands under reactor0 still take the rank clue, which is
// what proves the term inert outside the variant: `settle` skips a term that is
// false of everything.
TEST(TiiahColourPreference, ReactorZeroIsUnmoved) {
  SetupOptions opts = replay_opts();
  opts.variant_name = "No Variant";
  hanabi::test::reactor0::use_reactor0(opts);
  Game g = setup(std::move(opts));

  PerformAction a;
  ASSERT_NO_THROW(a = g.take_action());
  ASSERT_TRUE(hanabi::is_clue(a));
  EXPECT_TRUE(std::holds_alternative<PerformRank>(a))
      << "three useful touches beat one, as they always have under reactor0";
}
