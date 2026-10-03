// Rung 3.7 under Throw It in a Hole: no lock over a chop worse than one away from
// playable (reactor0/DECISION_MAKING.md priority 3, item 7, v20.2.0).
//
// Human diagnostic v18_human_vs_bot_diagnostics/2018365.md T7: green, holding a
// called y1, locked black with a 2 over a y4 chop on empty yellow. The user's
// ruling: the variant is hard enough that a whole hand is committed only for a chop
// that is critical, playable or one away. 3.6b, which only ever replaces 3.7's
// lock, goes with it. Reactor0 keeps the lock.
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
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

// The shape of the clue `choose_clue` picks, or nullopt when it declines.
std::optional<ClueShape> chosen_shape(const Game& g) {
  const auto cands = hanabi::reactor0::analyse_clues(g, all_candidate_clues(g));
  const auto pick = hanabi::reactor0::choose_clue(g, cands);
  if (!pick) return std::nullopt;
  auto key = [](const PerformAction& a) {
    return std::make_pair(a.index(), std::visit([](const auto& x) { return x.hash_int(); }, a));
  };
  for (const ClueCandidate& c : cands) {
    if (key(c.perform) == key(*pick)) return c.reading.shape;
  }
  return std::nullopt;
}

// test_stable_play_before_lock.cpp's position: one token, Bob stuck on four
// playable 2s he does not know about, Cathy with nothing playable and a safe chop.
// Bob's chop is `bobs_chop`, on yellow 0.
Game position(const char* bobs_chop, bool hole) {
  SetupOptions opts;
  if (hole) opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.play_stacks = {1, 0, 1, 1, 1};
  opts.clue_tokens = 1;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},          // Alice (us)
      {bobs_chop, "r2", "g2", "b2", "p2"},     // Bob
      {"r4", "y3", "g4", "b4", "p4"},          // Cathy
  };
  opts.starting = TestPlayer::ALICE;
  if (hole) {
    hanabi::test::tiiah::use_tiiah(opts);
  } else {
    hanabi::test::reactor0::use_reactor0(opts);
  }
  return setup(std::move(opts));
}

}  // namespace

TEST(TiiahNoLockOverFarChop, ThePredicateReadsCriticalPlayableAndOneAway) {
  using hanabi::reactor0::chop_worth_a_lock;
  const int bob = static_cast<int>(TestPlayer::BOB);
  EXPECT_FALSE(chop_worth_a_lock(position("y4", true), bob)) << "y4 on yellow 0: three away";
  EXPECT_FALSE(chop_worth_a_lock(position("y3", true), bob)) << "y3: two away";
  EXPECT_TRUE(chop_worth_a_lock(position("y2", true), bob)) << "y2: one away";
  EXPECT_TRUE(chop_worth_a_lock(position("y1", true), bob)) << "y1: playable";
  EXPECT_TRUE(chop_worth_a_lock(position("y5", true), bob)) << "y5: critical";
  EXPECT_FALSE(chop_worth_a_lock(position("r1", true), bob)) << "r1 on red 1: trash";
}

// A far chop: neither the lock nor 3.6b's stable play in its place.
TEST(TiiahNoLockOverFarChop, AFarChopIsNotLocked) {
  Game g = position("y4", true);
  ASSERT_TRUE(hanabi::reactor0::priority_3_applies(g)) << "guard: Bob is stuck";
  const auto shape = chosen_shape(g);
  EXPECT_NE(shape, ClueShape::STABLE_LOCK) << "a y4 three away is not worth a lock";
  EXPECT_NE(shape, ClueShape::STABLE_PLAY) << "3.6b only replaces a lock 3.7 would give";
}

// A one-away chop still reaches 3.7, and at one token 3.6b's play takes its place.
TEST(TiiahNoLockOverFarChop, AOneAwayChopStillReachesTheLockRung) {
  Game g = position("y2", true);
  ASSERT_TRUE(hanabi::reactor0::priority_3_applies(g)) << "guard: Bob is stuck";
  EXPECT_EQ(chosen_shape(g), ClueShape::STABLE_PLAY)
      << "3.7 fires, and below 3.1's clue count 3.6b gives the play in its place";
}

// Reactor0 is untouched: the same far chop still reaches 3.7 / 3.6b.
TEST(TiiahNoLockOverFarChop, Reactor0KeepsTheLockRung) {
  Game g = position("y4", false);
  ASSERT_TRUE(hanabi::reactor0::priority_3_applies(g)) << "guard: Bob is stuck";
  EXPECT_EQ(chosen_shape(g), ClueShape::STABLE_PLAY)
      << "outside the hole, 3.7 fires and 3.6b replaces its lock";
}
