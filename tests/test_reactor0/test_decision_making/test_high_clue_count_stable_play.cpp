// General Clue Evaluation List §3 at a high clue count (reactor0 DECISION_MAKING.md
// §3, v20.22.0, the user's amendment): with 6 or more tokens, §3.1's stable play
// clue to Bob is given even when Bob's chop is not worth a clue. Only 3.1 opens --
// the locks and discard calls below it still want a stuck Bob
// (`Reactor0CluePriority.FillInOutranksTheLockAtEightTokens`). Replay 2019598 T37
// is the motivating case.
//
// The fixture: Bob holds a playable r1 in slot 2. His chop (slot 1, the newest
// unclued card) is a g3 Cathy also holds, so it is neither endangered nor playable,
// and §3 has no claim of its own. Cathy holds no playable card and no trash, so
// neither reactive priority has anything to offer.
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
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

Game bob_not_stuck_at(int clue_tokens) {
  SetupOptions opts;
  opts.clue_tokens = clue_tokens;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g3", "r1", "y4", "b4", "p4"},
      {"g3", "r4", "y3", "b3", "p3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return setup(std::move(opts));
}

// The candidate `choose_clue` picked, if any.
const ClueCandidate* picked(const std::vector<ClueCandidate>& cands,
                            const std::optional<PerformAction>& pick) {
  if (!pick) return nullptr;
  for (const auto& c : cands) {
    if (hash_int(c.perform) == hash_int(*pick)) return &c;
  }
  return nullptr;
}

}  // namespace

TEST(Reactor0HighClueCount, GuardBobIsNotStuck) {
  Game g = bob_not_stuck_at(7);
  EXPECT_FALSE(hanabi::reactor0::priority_3_applies(g))
      << "guard: Bob's chop is not worth a clue, so section 3 has no claim of its own";
}

TEST(Reactor0HighClueCount, AStablePlayClueToBobAtSevenTokens) {
  Game g = bob_not_stuck_at(7);
  const auto cands = hanabi::reactor0::analyse_clues(g, all_candidate_clues(g));
  const auto pick = hanabi::reactor0::choose_clue(g, cands);
  const ClueCandidate* c = picked(cands, pick);
  ASSERT_NE(c, nullptr) << "a clue is given at 7 tokens";
  EXPECT_EQ(c->action.target, 1) << "to Bob";
  EXPECT_EQ(c->reading.shape, ClueShape::STABLE_PLAY) << "section 3.1's stable play clue";
}

// Control: below 6 tokens, a Bob who is not stuck gets no section-3 clue.
TEST(Reactor0HighClueCount, NoStablePlayClueToBobAtFiveTokens) {
  Game g = bob_not_stuck_at(5);
  const auto cands = hanabi::reactor0::analyse_clues(g, all_candidate_clues(g));
  const auto pick = hanabi::reactor0::choose_clue(g, cands);
  const ClueCandidate* c = picked(cands, pick);
  EXPECT_FALSE(c && c->action.target == 1 && c->reading.shape == ClueShape::STABLE_PLAY);
}
