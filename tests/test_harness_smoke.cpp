// Smoke tests for the C++ test harness (test_harness.h/.cpp).
// Mirrors python-bot/tests/test_harness.py.
#include <gtest/gtest.h>

#include "hanabi/basics/identity.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

SetupOptions two_player_hands() {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g2", "b1", "r2", "r3", "g5"},
  };
  return opts;
}

}  // namespace

TEST(Harness, SetupBasic2p) {
  Game g = setup(two_player_hands());
  EXPECT_EQ(g.state.num_players, 2);
  ASSERT_EQ(g.state.names.size(), 2u);
  EXPECT_EQ(g.state.names[0], "Alice");
  EXPECT_EQ(g.state.names[1], "Bob");
  for (const auto& h : g.state.hands) EXPECT_EQ(h.size(), 5u);
  int bob_slot_1 = g.state.hands[1][0];
  EXPECT_EQ(g.state.deck[bob_slot_1].suit_index, 2);
  EXPECT_EQ(g.state.deck[bob_slot_1].rank, 2);
  int bob_slot_5 = g.state.hands[1][4];
  EXPECT_EQ(g.state.deck[bob_slot_5].suit_index, 2);
  EXPECT_EQ(g.state.deck[bob_slot_5].rank, 5);
}

TEST(Harness, SetupStartingPlayer) {
  SetupOptions opts = two_player_hands();
  opts.starting = TestPlayer::BOB;
  Game g = setup(opts);
  EXPECT_EQ(g.state.current_player_index, 1);
}

TEST(Harness, SetupClueTokensAndStrikes) {
  SetupOptions opts = two_player_hands();
  opts.clue_tokens = 3;
  opts.strikes = 2;
  Game g = setup(opts);
  EXPECT_EQ(g.state.clue_tokens, 3);
  EXPECT_EQ(g.state.strikes, 2);
}

TEST(Harness, SetupWithPlayStacks) {
  SetupOptions opts = two_player_hands();
  opts.play_stacks = std::vector<int>{2, 0, 0, 0, 0};
  Game g = setup(opts);
  EXPECT_EQ(g.state.play_stacks, (std::vector<int>{2, 0, 0, 0, 0}));
  EXPECT_EQ(g.state.score(), 2);
  EXPECT_EQ(g.state.base_count[Identity(0, 1).to_ord()], 1);
  EXPECT_EQ(g.state.base_count[Identity(0, 2).to_ord()], 1);
  int hand_total = 0;
  for (const auto& h : g.state.hands) hand_total += static_cast<int>(h.size());
  EXPECT_EQ(g.state.cards_left, g.state.cards_total - 2 - hand_total);
}

TEST(Harness, SetupWithDiscarded) {
  SetupOptions opts = two_player_hands();
  opts.discarded = {"r1", "y5"};
  Game g = setup(opts);
  // y5 critical -> max rank for suit 1 lowered to 4
  EXPECT_EQ(g.state.max_ranks[1], 4);
  EXPECT_EQ(g.state.discard_stacks[0][0].size(), 1u);  // r1 discarded
  EXPECT_EQ(g.state.discard_stacks[1][4].size(), 1u);  // y5 discarded
}

TEST(Harness, SetupVariantName) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y1", "g1", "i1", "i5"},
  };
  opts.variant_name = "Prism (5 Suits)";
  Game g = setup(opts);
  EXPECT_EQ(g.state.variant->name, "Prism (5 Suits)");
  EXPECT_TRUE(g.state.variant->suits[4].suit_type.prism);
}

TEST(Harness, SetupRejectsOversizedHand) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx", "xx"},
      {"r1", "r1", "r2", "r3", "r4"},
  };
  EXPECT_THROW(setup(opts), std::invalid_argument);
}

TEST(Harness, ParseActionClueRank) {
  Game g = setup(two_player_hands());
  Action a = parse_action(g.state, "Alice clues 1 to Bob");
  ASSERT_TRUE(std::holds_alternative<ClueAction>(a));
  const auto& ca = std::get<ClueAction>(a);
  EXPECT_EQ(ca.giver, 0);
  EXPECT_EQ(ca.target, 1);
  EXPECT_EQ(ca.clue.kind, ClueKind::RANK);
  EXPECT_EQ(ca.clue.value, 1);
}

TEST(Harness, TakeTurnClueAdvancesCurrentPlayer) {
  Game g = setup(two_player_hands());
  EXPECT_EQ(g.state.current_player_index, 0);
  g = take_turn(std::move(g), "Alice clues 1 to Bob");
  EXPECT_EQ(g.state.current_player_index, 1);
  EXPECT_EQ(g.state.clue_tokens, 7);
}

TEST(Harness, AliceSeesBobsHandIdentities) {
  Game g = setup(two_player_hands());
  // Alice can see Bob's hand directly via Thought.id() (suit_index != -1 branch).
  // (We don't assert on `inferred` here because the inferred-set widening is a
  // consequence of card_count vs certain_map and not a per-card pin; see Python's
  // test_empathy.py for cases that assert specific narrowed inferences.)
  const auto& alice = g.players[0];
  int bob_slot_1 = g.state.hands[1][0];
  auto seen = alice.thoughts[bob_slot_1].id();
  ASSERT_TRUE(seen.has_value());
  EXPECT_EQ(*seen, Identity(2, 2));
}

TEST(Harness, FullyKnownPinsInferenceOnCommon) {
  SetupOptions opts;
  opts.hands = {
      {"g3", "xx", "xx", "xx", "xx"},
      {"xx", "xx", "xx", "xx", "xx"},
  };
  Game g = setup(opts);
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "g3");
  // fully_known writes into common.thoughts directly (without an elim pass);
  // common's inferred for the targeted slot should be exactly {g3}.
  expect_infs(g, std::nullopt, TestPlayer::ALICE, 1, {"g3"});
}
