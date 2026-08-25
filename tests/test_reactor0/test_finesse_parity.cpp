// The finesse rule keys on the PARITY BUCKET, not on the clue kind.
//
// A finesse is reactive Phase B, which lives in `reactive_rank` -- the
// EVEN-parity ruleset. Normally that is the rank clue, so "Phase B is rank-only"
// looked true and `clue_gets_finesse` was written that way. Odds and Evens makes
// the COLOUR clue the even bucket, and `/set` can move an individual clue, so
// the kind test made VH1 unreachable in those variants: a finesse became
// invisible to the pre-check that outranks everything else.
//
// Replay 1967416 T1 is the case -- Cathy held y2 on slot 1, Bob held y1 on slot
// 1, Yellow's reactive value of 2 was exactly the anchor that pairs them, and
// will-bot69 clued yellow to BOB for a stable single play instead.
//
// Each test here has a mirror in a plain variant where the roles are reversed,
// so what is pinned is the parity and not a particular clue kind.
#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::ClueCandidate;
using hanabi::reactor0::ClueShape;

namespace {

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

// Is any VH1 candidate a clue of this KIND, aimed at Cathy?
//
// `clue_is_vh1` is asked directly rather than read off `ClueCandidate::tier`, so
// that a rule promoted into VERY HIGH later cannot make these tests pass for a
// reason that has nothing to do with parity.
bool has_vh1_of_kind(const Game& g, ClueKind kind) {
  for (const auto& c : hanabi::reactor0::analyse_clues(g, all_candidate_clues(g))) {
    if (c.action.clue.kind != kind ||
        c.action.target != static_cast<int>(TestPlayer::CATHY)) {
      continue;
    }
    const Game hypo = g.simulate(Action{c.action});
    if (hanabi::reactor0::clue_is_vh1(g, hypo, c.action)) return true;
  }
  return false;
}

// Cathy holds a one-away y2 on slot 1 and Bob the y1 that unblocks it -- the
// finesse shape from replay 1967416. Cathy's chop must not be trash or a
// same-hand dupe, which VH1 also requires.
SetupOptions finesse_opts(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.hands = {
      {"r4", "g4", "b4", "p4", "r5"},  // Alice (giver, us)
      {"y1", "g2", "b2", "p2", "r2"},  // Bob (reacter) -- y1 on slot 1
      {"y2", "g3", "b3", "p3", "r3"},  // Cathy (receiver) -- y2 on slot 1
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

// In a plain variant the even bucket is RANK, so the finesse is a rank clue and
// never a colour one.
TEST(Reactor0FinesseParity, PlainVariantFinesseIsARankClue) {
  Game g = setup(finesse_opts("No Variant"));
  EXPECT_TRUE(has_vh1_of_kind(g, ClueKind::RANK))
      << "rank is the even-parity family here, so it carries Phase B";
  EXPECT_FALSE(has_vh1_of_kind(g, ClueKind::COLOUR))
      << "colour is the odd-parity family and has no Phase B";
}

// Odds and Evens swaps them, so the very same shape is a COLOUR finesse. Before
// v7.28.0 `clue_gets_finesse` returned false for every colour clue and this
// found nothing at all.
TEST(Reactor0FinesseParity, OddsAndEvensFinesseIsAColourClue) {
  Game g = setup(finesse_opts("Odds and Evens (5 Suits)"));
  EXPECT_TRUE(has_vh1_of_kind(g, ClueKind::COLOUR))
      << "colour is the even-parity family under Odds and Evens";
  EXPECT_FALSE(has_vh1_of_kind(g, ClueKind::RANK))
      << "and rank is the odd one, which has no Phase B";
}

// The shape classifier moves with the same rule. A reactive clue's SHAPE feeds
// rungs 1 and 2 of the General Clue Evaluation List, so an inverted reading
// there mis-ranks every reactive clue in the variant.
TEST(Reactor0FinesseParity, ReactiveShapeFollowsTheParityToo) {
  auto shapes_for = [](const Game& g, ClueKind kind) {
    std::vector<ClueShape> out;
    for (const auto& c :
         hanabi::reactor0::analyse_clues(g, all_candidate_clues(g))) {
      if (c.action.clue.kind == kind &&
          c.action.target == static_cast<int>(TestPlayer::CATHY)) {
        out.push_back(c.reading.shape);
      }
    }
    return out;
  };
  auto has_double_play = [](const std::vector<ClueShape>& v) {
    for (ClueShape s : v) {
      if (s == ClueShape::REACTIVE_PLAY) return true;
    }
    return false;
  };

  // Plain: the double play comes from a RANK clue.
  Game plain = setup(finesse_opts("No Variant"));
  EXPECT_TRUE(has_double_play(shapes_for(plain, ClueKind::RANK)));
  EXPECT_FALSE(has_double_play(shapes_for(plain, ClueKind::COLOUR)));

  // Odds and Evens: from a COLOUR clue.
  Game oe = setup(finesse_opts("Odds and Evens (5 Suits)"));
  EXPECT_TRUE(has_double_play(shapes_for(oe, ClueKind::COLOUR)));
  EXPECT_FALSE(has_double_play(shapes_for(oe, ClueKind::RANK)));
}
