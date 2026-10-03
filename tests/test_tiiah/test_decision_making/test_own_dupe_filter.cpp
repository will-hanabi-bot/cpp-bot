// The own-dupe candidate filter (reactor0/DECISION_MAKING.md, tiiah/CONVENTION.md
// §2; v17.2.0, v18.7.0).
//
// A giver whose own hole card may have been a g1 does not call a partner's g1 to
// play unnamed: in that world it is a duplicate, and the partner strikes. A NAMED
// call is given (v18.18.0; v20.4.0, which reads a stable call on the giver's own
// stacks): the partner watched the hole card and throws a dupe. Except Bob's
// chop, when Bob is stuck with it (§3's precondition): the choice there is between
// a possible dupe and a certain loss, and a human saves it. Human diagnostic
// 2013726 T17 (v18_human_vs_bot_diagnostics/2013726.md).
#include <gtest/gtest.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

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

// Alice throws her slot 1 into the hole as a `{g1,b1}`; Bob's hand is `bob`.
Game position(std::vector<std::string> bob) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                std::move(bob),
                {"r3", "y3", "b3", "p3", "r5"}};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 6;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  const int mine = order_at(g, TestPlayer::ALICE, 1);
  g = hidden_action(std::move(g), TestPlayer::ALICE, 1, /*reached_the_hole=*/true);
  g.with_meta(mine, [](ConvData& m) {
    m.superposition = IdentitySet::empty().add(Identity{2, 1}).add(Identity{3, 1});
  });
  return g;
}

// Whether a clue of this kind and value to Bob survives the candidate filters.
bool offers(const Game& g, ClueKind kind, int value) {
  const auto cands = hanabi::reactor0::analyse_clues(g, all_candidate_clues(g));
  return std::any_of(cands.begin(), cands.end(), [kind, value](const auto& c) {
    return c.action.target == static_cast<int>(TestPlayer::BOB) &&
           c.action.clue.kind == kind && c.action.clue.value == value;
  });
}

bool some_clue_touches(const Game& g, int order) {
  const auto cands = hanabi::reactor0::analyse_clues(g, all_candidate_clues(g));
  return std::any_of(cands.begin(), cands.end(), [order](const auto& c) {
    return std::find(c.action.list_.begin(), c.action.list_.end(), order) !=
           c.action.list_.end();
  });
}

}  // namespace

// Bob's g1 is not his chop. A clue that calls it without naming it -- the 1, read
// as any playable 1 -- is dropped, since our own hole card may have been that g1
// and Bob could not tell the dupe from a play.
TEST(TiiahOwnDupeFilter, AnUnnamedCallOnAPossibleDupeIsDropped) {
  Game g = position({"y4", "g1", "r4", "b4", "p4"});
  EXPECT_FALSE(offers(g, ClueKind::RANK, 1));
}

// Green names it: read on our own stacks (v20.4.0) the call is exactly the g1, so it
// is given. Had our hole card been the g1, Bob watched it go in and throws his as
// the dupe (tiiah/CONVENTION.md §2c's named exception; replay 2018435 T11).
TEST(TiiahOwnDupeFilter, ANamedCallOnAPossibleDupeIsGiven) {
  Game g = position({"y4", "g1", "r4", "b4", "p4"});
  EXPECT_TRUE(offers(g, ClueKind::COLOUR, 2));
}

// The same g1 on Bob's chop, with Bob stuck: the save survives the filter.
TEST(TiiahOwnDupeFilter, BobsChopIsSavedAnyway) {
  Game g = position({"g1", "y4", "r4", "b4", "p4"});
  EXPECT_TRUE(some_clue_touches(g, order_at(g, TestPlayer::BOB, 1)));
}
