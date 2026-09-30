// Rung 3.6b: in place of 3.7's lock, below 3.1's clue count, a stable play clue to a
// stuck Bob with a non-critical chop (reactor0/DECISION_MAKING.md priority 3,
// v18.16.0).
//
// A lock and a play cost the same one clue. The lock commits Bob's whole hand and
// leaves him nothing to do; the play gives him his turn, and what he may then
// throw is not critical. A critical chop keeps 3.8-3.10, which save it for good.
//
// Human diagnostic v18_human_vs_bot_diagnostics/2014538.md T24 (TIIAH): green, on
// one token, locked black with a 4 on a non-critical y4 when Green would have had
// him play the g3. The rung is reactor0's, so the fixture is too.
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <variant>
#include <vector>

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

bool offers(const Game& g, ClueShape shape) {
  for (const ClueCandidate& c :
       hanabi::reactor0::analyse_clues(g, all_candidate_clues(g))) {
    if (c.reading.shape == shape) return true;
  }
  return false;
}

// One token. Red, green, blue and purple on 1, so Bob's four 2s are all playable
// and he does not know it: he is stuck, and his hand is "nearly all close to
// playable" (3.7). Cathy holds nothing playable, so Bob cannot handle her himself,
// and her chop (an r4) is neither playable nor critical. Nothing she holds follows
// a card of Bob's, so no reactive play exists to outrank rung 3. Bob's chop is
// `bobs_chop`.
Game position(const char* bobs_chop) {
  SetupOptions opts;
  opts.play_stacks = {1, 0, 1, 1, 1};
  opts.clue_tokens = 1;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},          // Alice (us)
      {bobs_chop, "r2", "g2", "b2", "p2"},     // Bob
      {"r4", "y3", "g4", "b4", "p4"},          // Cathy
  };
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return setup(std::move(opts));
}

}  // namespace

// A non-critical y4 on chop: Alice gives Bob a stable play rather than locking him.
TEST(Reactor0StablePlayBeforeLock, APlayGoesAheadOfALockOfANonCriticalChop) {
  Game g = position("y4");
  ASSERT_TRUE(hanabi::reactor0::priority_3_applies(g)) << "guard: Bob is stuck";
  ASSERT_TRUE(offers(g, ClueShape::STABLE_PLAY)) << "guard: a stable play exists";
  ASSERT_TRUE(offers(g, ClueShape::STABLE_LOCK)) << "guard: so does a lock";

  EXPECT_EQ(chosen_shape(g), ClueShape::STABLE_PLAY)
      << "one token buys Bob a play, not a lock that leaves him nothing to do";
}

// The control: a critical y5 on chop keeps the lock, which saves it for good.
TEST(Reactor0StablePlayBeforeLock, ACriticalChopStillTakesTheLock) {
  Game g = position("y5");
  ASSERT_TRUE(hanabi::reactor0::priority_3_applies(g)) << "guard: Bob is stuck";
  ASSERT_TRUE(offers(g, ClueShape::STABLE_PLAY)) << "guard: a stable play exists";
  ASSERT_TRUE(offers(g, ClueShape::STABLE_LOCK)) << "guard: so does a lock";

  EXPECT_EQ(chosen_shape(g), ClueShape::STABLE_LOCK)
      << "the last y5 is saved for good";
}
