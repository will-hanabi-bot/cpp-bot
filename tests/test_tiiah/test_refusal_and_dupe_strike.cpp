// Two ways the team learns that a card is ALREADY DOWN without learning which hole
// card put it there (tiiah/CONVENTION.md §1c, §1e rules 5 and 8, v16.27.0).
//
//   * The REFUSAL is given even when the tier gate would reject it, and reading
//     it no longer settles the first of several giver hole cards that admit the
//     refused card.
//   * A partner's STRIKE on a duplicate is common knowledge when the copy that is
//     down went into the hole in front of the striker.
#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/tiiah/superposition.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

constexpr int kPurple = 4;
constexpr Identity kP1{4, 1};

IdentitySet ids(std::initializer_list<Identity> list) {
  IdentitySet out = IdentitySet::empty();
  for (auto i : list) out = out.add(i);
  return out;
}

SetupOptions opts_for(std::vector<std::vector<std::string>> hands,
                      std::vector<int> stacks, int clue_tokens) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = std::move(hands);
  opts.play_stacks = std::move(stacks);
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = clue_tokens;
  use_tiiah(opts);
  return opts;
}

// A failed discard with the identity withheld: how a strike arrives in this
// variant. `draw` is what the striker draws.
Game strike(Game g, TestPlayer who, int slot, const std::string& draw) {
  const int pi = static_cast<int>(who);
  const int order = order_at(g, who, slot);
  const int next_order = g.state.next_card_order;
  g.catchup = true;
  g.handle_action(DiscardAction{pi, order, -1, -1, /*failed=*/true});
  const Identity id = g.state.expand_short(draw);
  g.handle_action(DrawAction{pi, next_order, id.suit_index, id.rank});
  g.handle_action(TurnAction{g.state.turn_count, g.state.next_player_index(pi)});
  g.catchup = false;
  return g;
}

}  // namespace

// --- the refusal ---------------------------------------------------------------

// We are Bob, the reacter, holding the urgent call a dead reactive left us, with
// three clue tokens and a deck to go: OCCUPIED, inside the gate's window, and every
// refusal LOW. The gate used to reject them all and we answered the reaction.
// Replay 2011854 T27.
TEST(TiiahRefusalGate, TheRefusalIsGivenWhileOccupied) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"y1", "g1", "r1", "b2", "p2"},
                           {"y2", "g2", "b1", "p1", "r4"}},
                          {1, 0, 0, 0, 0}, /*clue_tokens=*/3));
  const int dead = order_at(g, TestPlayer::BOB, 3);  // the spare r1
  ReactorWC wc;
  wc.giver = 2;
  wc.reacter = 0;
  wc.receiver = 1;
  wc.receiver_hand = g.state.hands[1];
  wc.clue = Clue{ClueKind::RANK, 1, 1};
  wc.focus_slot = 1;
  wc.turn = g.state.turn_count;
  wc.even_parity = true;
  wc.receiver_target_order = dead;
  const int ours = order_at(g, TestPlayer::ALICE, 2);
  wc.react_order = ours;
  g.waiting.push_back(wc);
  g.with_meta(ours, [&g](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
    m.by = 2;
    m = m.signal(g.state.turn_count);
  });

  const PerformAction action = g.take_action();
  const auto* rank = std::get_if<PerformRank>(&action);
  const auto* colour = std::get_if<PerformColour>(&action);
  ASSERT_TRUE(rank || colour) << "we refuse rather than answer a reaction on a dead r1";
  EXPECT_EQ(rank ? rank->target : colour->target, 1);
}

// Two of the giver's hole cards admit the refused p1. The refusal says one of them
// was it, not which: neither is settled, the joint fact is kept, and the shared
// view learns purple is on 1. Replay 2011854 T27, where settling the first (a b1)
// put the shared view on red 3.
TEST(TiiahRefusalGate, SeveralHoleCardsCouldBeTheRefusedCard) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"y3", "g3", "b3", "p3", "y4"},
                           {"b1", "p1", "g4", "b4", "p4"}},
                          {0, 0, 0, 0, 0}, 5));
  const int first = order_at(g, TestPlayer::CATHY, 1);
  const int second = order_at(g, TestPlayer::CATHY, 2);
  g = hidden_action(std::move(g), TestPlayer::CATHY, 1, /*reached_the_hole=*/true, "r5");
  // The r5 she drew is on slot 1 now, so her p1 is on slot 2.
  g = hidden_action(std::move(g), TestPlayer::CATHY, 2, /*reached_the_hole=*/true, "y5");
  g.with_meta(first, [](ConvData& m) { m.superposition = ids({Identity{3, 1}, kP1}); });
  g.with_meta(second, [](ConvData& m) { m.superposition = ids({Identity{0, 1}, kP1}); });
  const std::size_t reqs = g.hole_requirements.size();

  ASSERT_TRUE(hanabi::tiiah::collapse_refused_target(g, /*giver=*/2, kP1));
  EXPECT_TRUE(g.meta[first].superposed()) << "the first by card order was a guess";
  EXPECT_TRUE(g.meta[second].superposed());
  ASSERT_EQ(g.hole_requirements.size(), reqs + 1);
  EXPECT_EQ(g.hole_requirements.back().id, kP1);
  EXPECT_EQ(g.hole_requirements.back().orders.size(), 2u);
  EXPECT_EQ(g.state.common_play_stacks[kPurple], 1);
}

// --- the dupe strike -------------------------------------------------------------

// Cathy throws a p1 into the hole unknowing; Bob watched it go in. Bob then strikes
// with another p1. Every seat can tell purple is on 1 -- we and Cathy saw the p1
// strike, and Bob saw hers land -- so the shared view says so. Replay 2011854 T28.
TEST(TiiahDupeStrike, AWatchedCopyMakesTheStrikeCommonKnowledge) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"g3", "p1", "b3", "y3", "y4"},
                           {"p1", "g4", "b4", "r4", "p4"}},
                          {0, 0, 0, 0, 0}, 5));
  g = hidden_action(std::move(g), TestPlayer::ALICE, 5, /*reached_the_hole=*/false);
  g = hidden_action(std::move(g), TestPlayer::BOB, 5, /*reached_the_hole=*/false, "r5");
  g = hidden_action(std::move(g), TestPlayer::CATHY, 1, /*reached_the_hole=*/true, "y5");
  ASSERT_EQ(g.state.play_stacks[kPurple], 1) << "guard: we watched her p1 land";
  ASSERT_EQ(g.state.common_play_stacks[kPurple], 0) << "guard: she could not name it";

  g = hidden_action(std::move(g), TestPlayer::ALICE, 5, /*reached_the_hole=*/false);
  g = strike(std::move(g), TestPlayer::BOB, 3, "r3");  // his p1, now on slot 3
  EXPECT_EQ(g.state.common_play_stacks[kPurple], 1);
}

// The control: the p1 that is down is BOB's own hole card, so Bob -- the striker --
// never saw it, cannot see what struck, and learns nothing. Not common knowledge.
TEST(TiiahDupeStrike, TheStrikersOwnCopyIsNotCommonKnowledge) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"p1", "p1", "b3", "y3", "y4"},
                           {"g3", "g4", "b4", "r4", "p4"}},
                          {0, 0, 0, 0, 0}, 5));
  g = hidden_action(std::move(g), TestPlayer::ALICE, 5, /*reached_the_hole=*/false);
  g = hidden_action(std::move(g), TestPlayer::BOB, 1, /*reached_the_hole=*/true, "r5");
  ASSERT_EQ(g.state.play_stacks[kPurple], 1) << "guard: we watched his p1 land";
  g = hidden_action(std::move(g), TestPlayer::CATHY, 5, /*reached_the_hole=*/false, "y5");

  g = hidden_action(std::move(g), TestPlayer::ALICE, 5, /*reached_the_hole=*/false);
  g = strike(std::move(g), TestPlayer::BOB, 2, "r3");  // his other p1
  EXPECT_EQ(g.state.common_play_stacks[kPurple], 0);
}
