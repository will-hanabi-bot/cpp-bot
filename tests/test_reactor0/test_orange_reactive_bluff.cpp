#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

Game orange_bluff(int pov = 2, const std::string& reaction = "o1",
                  bool duplicate = false, const std::vector<std::string>& known_bob = {}) {
  SetupOptions opts;
  opts.variant_name = "Orange (5 Suits)";
  opts.play_stacks = {1, 0, 0, 0, 1};
  opts.clue_tokens = 4;
  opts.hands = {
      {"r4", "y2", "g2", "b2", "o2"},
      {reaction, "y4", "g4", "o4", "y5"},
      {"y3", "r3", duplicate ? "r3" : "g5", "b5", "o5"},
  };
  if (!known_bob.empty()) opts.hands[1] = known_bob;
  opts.hands[pov] = {"xx", "xx", "xx", "xx", "xx"};
  opts.constructor = [pov](int id, State s) {
    s.our_player_index = pov;
    return Game::create(id, std::move(s));
  };
  use_reactor0(opts);
  auto g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::CATHY, 2, {"3"});
  if (!known_bob.empty()) {
    for (int i = 0; i < 5; ++i)
      g = fully_known(std::move(g), TestPlayer::BOB, i + 1, known_bob[i]);
  }
  return take_turn(std::move(g), duplicate ? "Alice clues 3 to Cathy (slots 1,2,3)"
                                         : "Alice clues 3 to Cathy (slots 1,2)");
}

}  // namespace

TEST(Reactor0OrangeBluff, TrashPitchDoesNotCallTheReceiverToThrowAwayO3) {
  auto g = orange_bluff();
  const int target = order_at(g, TestPlayer::CATHY, 2);
  g = take_turn(std::move(g), "Bob plays o1 (slot 1)", "r1");
  expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"r3"});
  EXPECT_EQ(g.meta[target].status, CardStatus::NONE);
  EXPECT_EQ(g.meta[target].note_mark, NoteMark::RESET);
}

TEST(Reactor0OrangeBluff, PlainBluffCannotNameTheLaterOfTwoOrangeDuplicates) {
  auto g = orange_bluff(2, "b1");
  const int target = order_at(g, TestPlayer::CATHY, 2);
  g = take_turn(std::move(g), "Bob plays b1 (slot 1)", "r1");
  // Only slots1/2 can be o3, and the earlier pairing cannot be ruled out.
  expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"r3"});
  EXPECT_EQ(g.meta[target].status, CardStatus::NONE);
}

TEST(Reactor0OrangeBluff, RealConnectorKeepsTheOrdinaryPlayCall) {
  auto g = orange_bluff(2, "r2");
  const int target = order_at(g, TestPlayer::CATHY, 2);
  g = take_turn(std::move(g), "Bob plays r2 (slot 1)", "r1");
  expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"r3"});
  EXPECT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY);
}

TEST(Reactor0OrangeBluff, PossibleDuplicateAndBluffRemainAmbiguous) {
  auto g = orange_bluff(2, "b1", true);
  const int target = order_at(g, TestPlayer::CATHY, 2);
  g = take_turn(std::move(g), "Bob plays b1 (slot 1)", "r1");
  expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"r3", "o3"});
  EXPECT_EQ(g.meta[target].status, CardStatus::NONE);
}

TEST(Reactor0OrangeBluff, KnownNonConnectorPreservesLegitimateDuplicatePitch) {
  auto g = orange_bluff(2, "b1", true, {"b1", "y1", "g1", "r2", "y4"});
  const int target = order_at(g, TestPlayer::CATHY, 2);
  g = take_turn(std::move(g), "Bob plays b1 (slot 1)", "r1");
  expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"o3"});
  EXPECT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY);
}

TEST(Reactor0OrangeBluff, AnUnusableEarlierPairAllowsTheLaterDuplicate) {
  auto g = orange_bluff(2, "b1", false, {"b1", "y4", "g4", "o4", "y5"});
  const int target = order_at(g, TestPlayer::CATHY, 2);
  g = take_turn(std::move(g), "Bob plays b1 (slot 1)", "r1");
  // o3 in slots1/2: slot1 pairs with known y4, so slot2 can name the pitch.
  expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"o3"});
  EXPECT_EQ(g.meta[target].status, CardStatus::CALLED_TO_PLAY);
}

TEST(Reactor0OrangeBluff, ThrowingAwayUsefulOrangeDoesNotConfirmABluff) {
  auto g = orange_bluff(2, "o2");
  g = take_turn(std::move(g), "Bob plays o2 (slot 1)", "r1");
  const int target = order_at(g, TestPlayer::CATHY, 2);
  EXPECT_NE(g.common.thoughts[target].inferred, IdentitySet::single(Identity(0, 3)));
  EXPECT_EQ(g.meta[target].status, CardStatus::NONE);
  EXPECT_EQ(g.state.strikes, 0);
}

TEST(Reactor0OrangeBluff, TrashPitchIsReadConsistentlyFromEverySeat) {
  for (int pov : {0, 1, 2}) {
    SCOPED_TRACE(pov);
    auto g = orange_bluff(pov);
    const int target = order_at(g, TestPlayer::CATHY, 2);
    g = take_turn(std::move(g), "Bob plays o1 (slot 1)", "r1");
    expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"r3"});
    EXPECT_EQ(g.meta[target].status, CardStatus::NONE);
  }
}

TEST(Reactor0OrangeBluff, AStrikeDoesNotConfirmABluff) {
  auto g = orange_bluff(2, "r4");
  g = take_turn(std::move(g), "Bob bombs r4 (slot 1)", "r1");
  const int target = order_at(g, TestPlayer::CATHY, 2);
  EXPECT_NE(g.common.thoughts[target].inferred, IdentitySet::single(Identity(0, 3)));
  EXPECT_EQ(g.meta[target].status, CardStatus::NONE);
  EXPECT_EQ(g.state.strikes, 1);
}

TEST(Reactor0OrangeBluff, TrashBluffSurvivesADeferredReaction) {
  auto g = orange_bluff();
  const int target = order_at(g, TestPlayer::CATHY, 2);
  ASSERT_TRUE(g.pending_reactions[2].has_value());
  const auto before = g.pending_reactions[2]->reacter_before;
  ASSERT_EQ(before.size(), 5u);

  g = take_turn(std::move(g), "Bob clues 4 to Alice (slot 1)");
  g = take_turn(std::move(g), "Cathy clues 2 to Alice (slots 2,3,4,5)");
  g = take_turn(std::move(g), "Alice discards b2 (slot 4)", "y1");
  ASSERT_TRUE(g.pending_reactions[2].has_value());
  EXPECT_EQ(g.pending_reactions[2]->reacter_before, before);

  g = take_turn(std::move(g), "Bob plays o1 (slot 1)", "r1");
  expect_infs(g, std::nullopt, TestPlayer::CATHY, 2, {"r3"});
  EXPECT_EQ(g.meta[target].status, CardStatus::NONE);
  EXPECT_FALSE(g.pending_reactions[2].has_value());
}
