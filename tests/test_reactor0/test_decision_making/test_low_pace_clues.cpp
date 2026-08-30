// Low pace changes what §3 is for, and takes double discards off the table.
//
// "Low pace" is `pace() <= 2` throughout — the same boundary §2a's tier gate
// draws (see `test_pace_clue_gate.cpp`), and still the boundary rungs 3.2/3.4
// use for the double discard.
//
// §3 fires only for a Bob who is STUCK: his chop is worth something AND he has
// nothing else to do. That is now true at EVERY pace (v13.2.0).
//
// Through v13.1.0 a `pace() <= 2` arm waived the safe-action half, on the
// grounds that late there are few turns left to collect the chop in. It was
// removed because it acted on BOB's behalf even when he had something safe to
// do; the late aggression it was reaching for now lives in §4, which opens at
// `pace() <= 1` when ALICE is stuck. So the file keeps its low-pace fixtures and
// its name, but §3's precondition no longer moves with pace at all.
//
// The other two guards never moved: a locked Bob, and a chop that is neither
// endangered nor playable, skip §3 at any pace — they ask whether the clue is
// worth giving at all.
//
// The double discard goes the other way. It spends a clue to clear two cards
// the team could have thrown anyway, so when turns are the scarce resource it
// is not worth one. Rungs 3.2 and 3.4 both require `pace() >= 3`; §4.8 does
// not, because §4 must always return a clue.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/action.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Calibrated below by the guard assertions.
constexpr int kBasePace = 13;

// All five 1s are on the stacks, so a rank-1 clue makes a card COMMON-KNOWLEDGE
// trash -- which is what `has_no_safe_action` reads. Bob's chop (his oldest
// unclued card) is a playable 2, so §3's "worth a clue" test passes.
//
// Discards drop `pace` without lowering `max_score`: each rank-2 has two copies
// so one can go, and the 1s already on the stacks leave two spare copies each.
// Red is kept out of the discard list because the fixtures hold red cards.
std::vector<std::string> discards_for_pace(int target_pace) {
  static const std::vector<std::string> kPool = {
      "y1", "y1", "g1", "g1", "b1", "b1", "p1", "p1",
      "y2", "g2", "b2", "p2"};
  std::vector<std::string> out;
  for (int i = 0; i < kBasePace - target_pace && i < (int)kPool.size(); ++i) {
    out.push_back(kPool[i]);
  }
  return out;
}

SetupOptions low_pace_base(int target_pace) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {1, 1, 1, 1, 1};
  opts.hands = {
      {"r4", "g4", "b4", "p4", "r3"},   // Alice (us)
      // Bob: chop (index 0) is a playable y2; index 4 is a 1 we will clue, so
      // common knows it is trash and he has a safe discard.
      {"y2", "p4", "b4", "g4", "r1"},
      {"p3", "g3", "b3", "y4", "y3"},   // Cathy
  };
  opts.discarded = discards_for_pace(target_pace);
  use_reactor0(opts);
  return opts;
}

// Bob has a safe discard (the clued 1) AND a chop worth collecting.
Game bob_has_a_safe_action(int target_pace) {
  Game g = setup(low_pace_base(target_pace));
  return pre_clue(std::move(g), TestPlayer::BOB, 5, {"1"});
}

}  // namespace

// --- §3's precondition ----------------------------------------------------

TEST(Reactor0LowPaceClues, SafeActionStopsSectionThreeAtNormalPace) {
  Game g = bob_has_a_safe_action(3);
  ASSERT_EQ(g.state.pace(), 3) << "guard: not low pace";
  ASSERT_TRUE(hanabi::reactor0::has_playable_chop(g, (int)TestPlayer::BOB))
      << "guard: Bob's chop is worth collecting";
  ASSERT_FALSE(g.common.thinks_trash(g, (int)TestPlayer::BOB).empty())
      << "guard: and he has a safe discard, which is what stops §3";

  // Bob's chop is worth a clue, but he is not stuck.
  EXPECT_FALSE(hanabi::reactor0::priority_3_applies(g))
      << "at pace 3 a Bob with something safe to do does not earn §3";
}

// v13.2.0 INVERTED this. Through v13.1.0 low pace waived the safe-action half
// of §3's precondition, and this asserted the waiver. §3 is now the same rule at
// every pace: a Bob with something safe to do does not earn a clue, however few
// turns are left. The late aggression the waiver was reaching for moved to §4,
// which asks whether ALICE is stuck rather than acting on Bob's behalf.
//
// The fixture is unchanged, so this is the same position the waiver used to
// fire on -- only the expectation moved.
TEST(Reactor0LowPaceClues, LowPaceNoLongerWaivesTheSafeActionRequirement) {
  for (int pace : {1, 2}) {
    Game g = bob_has_a_safe_action(pace);
    ASSERT_EQ(g.state.pace(), pace);
    ASSERT_TRUE(hanabi::reactor0::has_playable_chop(g, (int)TestPlayer::BOB))
        << "guard: the chop is still worth collecting at pace " << pace
        << ", so it is the SAFE-ACTION half being tested and nothing else";
    EXPECT_FALSE(hanabi::reactor0::priority_3_applies(g))
        << "at pace " << pace
        << " a Bob who is not stuck still does not earn §3";
  }
}

// --- the two guards low pace must NOT bypass ------------------------------

// A chop that is neither endangered nor playable earns nothing, at any pace.
TEST(Reactor0LowPaceClues, AWorthlessChopStillSkipsSectionThreeAtLowPace) {
  SetupOptions opts = low_pace_base(1);
  // Bob's chop becomes a p3: not playable (purple is on 1, so p2 comes first)
  // and duplicated in Cathy's hand, so it is not endangered either.
  opts.hands[1] = {"p3", "p4", "b4", "g4", "r1"};
  opts.hands[2] = {"p3", "g3", "b3", "y4", "y3"};
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 5, {"1"});

  ASSERT_EQ(g.state.pace(), 1) << "guard: low pace";
  EXPECT_FALSE(hanabi::reactor0::priority_3_applies(g))
      << "the worth-a-clue test is what fails here, independently of pace";
}

// A locked Bob is out of §3's scope at any pace — there is no chop to save.
TEST(Reactor0LowPaceClues, ALockedBobStillSkipsSectionThreeAtLowPace) {
  SetupOptions opts = low_pace_base(1);
  opts.hands[1] = {"y5", "g5", "b5", "p5", "r5"};
  Game g = setup(std::move(opts));
  for (int slot = 1; slot <= 5; ++slot) {
    g = pre_clue(std::move(g), TestPlayer::BOB, slot, {"5"});
  }

  ASSERT_EQ(g.state.pace(), 1) << "guard: low pace";
  ASSERT_TRUE(g.common.thinks_locked(g, static_cast<int>(TestPlayer::BOB)))
      << "guard: Bob is locked";
  EXPECT_FALSE(hanabi::reactor0::priority_3_applies(g))
      << "a locked Bob has no chop; low pace does not change that";
}

// --- double discards ------------------------------------------------------

namespace {

// The position `test_clue_priority.cpp` uses for rung 3.2, with `discarded`
// added to move the pace. Stacks r=1; Bob `g3 r1 b3 g4 y4`, chop g3 with no
// safe action, so §3's precondition holds at EVERY pace here -- isolating the
// test to the double-discard rungs. Rank 5 to Cathy is a Phase C double
// discard calling Bob's r1 and Cathy's r1, both basic trash.
SetupOptions double_discard_position(int target_pace) {
  SetupOptions opts;
  opts.play_stacks = {1, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g3", "r1", "b3", "g4", "y4"},
      {"y4", "b4", "r1", "g4", "p5"},
  };
  // Base pace here is 13 as elsewhere; red's 1 is already on the stack, so the
  // pool below leaves red alone.
  static const std::vector<std::string> kPool = {
      "y1", "y1", "g1", "g1", "b1", "b1", "p1", "p1",
      "y2", "g2", "b2", "p2"};
  for (int i = 0; i < kBasePace - target_pace && i < (int)kPool.size(); ++i) {
    opts.discarded.push_back(kPool[i]);
  }
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return opts;
}

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

// Does the rung walk pick a double discard? Compared against the candidate the
// same walk would return, so this exercises `rung_3` rather than the pools.
bool chose_double_discard(const Game& g) {
  auto cands = hanabi::reactor0::analyse_clues(g, all_candidate_clues(g));
  auto chosen = hanabi::reactor0::choose_clue(g, cands);
  if (!chosen) return false;
  for (const auto& c : cands) {
    if (c.perform == *chosen) {
      return c.reading.shape == hanabi::reactor0::ClueShape::DOUBLE_DISCARD;
    }
  }
  return false;
}

// A double discard EXISTS here at all -- the guard that stops the low-pace
// assertions passing vacuously.
bool double_discard_available(const Game& g) {
  for (const auto& c : hanabi::reactor0::analyse_clues(g, all_candidate_clues(g))) {
    if (c.reading.shape == hanabi::reactor0::ClueShape::DOUBLE_DISCARD) return true;
  }
  return false;
}

}  // namespace

// The control: at pace 3 rung 3.2 takes the double discard, as it always has.
// Without this the low-pace assertions could pass for want of a double discard
// ever being chosen at all.
TEST(Reactor0LowPaceClues, DoubleDiscardIsTakenAtNormalPace) {
  Game g = setup(double_discard_position(3));
  ASSERT_EQ(g.state.pace(), 3);
  ASSERT_TRUE(double_discard_available(g)) << "guard: one is on offer";
  EXPECT_TRUE(chose_double_discard(g))
      << "rung 3.2 takes it when there is still time to spare";
}

TEST(Reactor0LowPaceClues, DoubleDiscardIsOffAtLowPace) {
  for (int pace : {1, 2}) {
    Game g = setup(double_discard_position(pace));
    ASSERT_EQ(g.state.pace(), pace);
    ASSERT_TRUE(double_discard_available(g))
        << "guard: a double discard is on offer at pace " << pace
        << ", so refusing it is a real choice";
    EXPECT_FALSE(chose_double_discard(g))
        << "at pace " << pace
        << " a double discard costs a turn the team cannot spare";
  }
}
