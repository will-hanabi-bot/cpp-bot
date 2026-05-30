// Port of python-bot/tests/test_basics/test_action.py.
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "hanabi/basics/action.h"
#include "hanabi/basics/clue.h"

using nlohmann::json;
using namespace hanabi;

// --- Action: properties ---

TEST(Action, StatusActionProperties) {
  StatusAction a{5, 10, 25};
  EXPECT_EQ(a.player_index(), -1);
  EXPECT_FALSE(a.requires_draw());
  EXPECT_FALSE(a.is_player_action());
}

TEST(Action, TurnActionProperties) {
  TurnAction a{3, 2};
  EXPECT_EQ(a.player_index(), 2);
  EXPECT_FALSE(a.is_player_action());
}

TEST(Action, ClueActionProperties) {
  ClueAction a{0, 1, {3, 4}, BaseClue(ClueKind::COLOUR, 2)};
  EXPECT_EQ(a.player_index(), 0);
  EXPECT_TRUE(a.is_player_action());
  EXPECT_FALSE(a.requires_draw());
}

TEST(Action, PlayActionRequiresDraw) {
  PlayAction a{0, 5, 0, 1};
  EXPECT_TRUE(a.requires_draw());
  EXPECT_TRUE(a.is_player_action());
}

TEST(Action, DiscardActionRequiresDraw) {
  DiscardAction a{0, 5, 0, 1, false};
  EXPECT_TRUE(a.requires_draw());
  DiscardAction bombed{0, 5, -1, -1, true};
  EXPECT_TRUE(bombed.failed);
}

TEST(Action, StrikeActionProperties) {
  StrikeAction a{1, 10, 20};
  EXPECT_EQ(a.player_index(), -1);
}

TEST(Action, GameOverActionProperties) {
  GameOverAction a{1, 0};
  EXPECT_EQ(a.player_index(), 0);
}

// --- Action::from_json ---

TEST(Action, FromJsonStatus) {
  auto a = action_from_json({{"type", "status"}, {"clues", 5}, {"score", 10}, {"maxScore", 25}});
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(std::holds_alternative<StatusAction>(*a));
  EXPECT_EQ(std::get<StatusAction>(*a), (StatusAction{5, 10, 25}));
}

TEST(Action, FromJsonTurn) {
  auto a = action_from_json({{"type", "turn"}, {"num", 3}, {"currentPlayerIndex", 1}});
  ASSERT_TRUE(a.has_value());
  EXPECT_EQ(std::get<TurnAction>(*a), (TurnAction{3, 1}));
}

TEST(Action, FromJsonClueColour) {
  auto a = action_from_json({
      {"type", "clue"},
      {"giver", 0},
      {"target", 1},
      {"list", {3, 4}},
      {"clue", {{"type", 0}, {"value", 2}}},
  });
  ASSERT_TRUE(a.has_value());
  EXPECT_EQ(std::get<ClueAction>(*a),
            (ClueAction{0, 1, {3, 4}, BaseClue(ClueKind::COLOUR, 2)}));
}

TEST(Action, FromJsonClueRank) {
  auto a = action_from_json({
      {"type", "clue"},
      {"giver", 0},
      {"target", 1},
      {"list", {3}},
      {"clue", {{"type", 1}, {"value", 3}}},
  });
  ASSERT_TRUE(a.has_value());
  EXPECT_EQ(std::get<ClueAction>(*a),
            (ClueAction{0, 1, {3}, BaseClue(ClueKind::RANK, 3)}));
}

TEST(Action, FromJsonDraw) {
  auto a = action_from_json(
      {{"type", "draw"}, {"playerIndex", 0}, {"order", 5}, {"suitIndex", -1}, {"rank", -1}});
  EXPECT_EQ(std::get<DrawAction>(*a), (DrawAction{0, 5, -1, -1}));
}

TEST(Action, FromJsonPlay) {
  auto a = action_from_json(
      {{"type", "play"}, {"playerIndex", 0}, {"order", 5}, {"suitIndex", 0}, {"rank", 1}});
  EXPECT_EQ(std::get<PlayAction>(*a), (PlayAction{0, 5, 0, 1}));
}

TEST(Action, FromJsonDiscard) {
  auto a = action_from_json({{"type", "discard"},
                              {"playerIndex", 0},
                              {"order", 5},
                              {"suitIndex", 1},
                              {"rank", 2},
                              {"failed", false}});
  EXPECT_EQ(std::get<DiscardAction>(*a), (DiscardAction{0, 5, 1, 2, false}));
}

TEST(Action, FromJsonStrike) {
  auto a = action_from_json({{"type", "strike"}, {"num", 1}, {"turn", 10}, {"order", 20}});
  EXPECT_EQ(std::get<StrikeAction>(*a), (StrikeAction{1, 10, 20}));
}

TEST(Action, FromJsonGameOver) {
  auto a = action_from_json({{"type", "gameOver"}, {"endCondition", 1}, {"playerIndex", 0}});
  EXPECT_EQ(std::get<GameOverAction>(*a), (GameOverAction{1, 0}));
}

TEST(Action, FromJsonUnknownReturnsNullopt) {
  EXPECT_FALSE(action_from_json({{"type", "weird-unknown"}}).has_value());
}

// --- PerformAction ---

TEST(PerformAction, PerformPlayJson) {
  PerformPlay a{3};
  EXPECT_FALSE(a.is_clue());
  EXPECT_EQ(a.hash_int(), 3);
  EXPECT_EQ(a.to_json(42), (json{{"tableID", 42}, {"type", 0}, {"target", 3}}));
}

TEST(PerformAction, PerformDiscardJson) {
  PerformDiscard a{2};
  EXPECT_FALSE(a.is_clue());
  EXPECT_EQ(a.hash_int(), 12);
  EXPECT_EQ(a.to_json(42), (json{{"tableID", 42}, {"type", 1}, {"target", 2}}));
}

TEST(PerformAction, PerformColourJson) {
  PerformColour a{1, 0};
  EXPECT_TRUE(a.is_clue());
  EXPECT_EQ(a.hash_int(), 21);
  EXPECT_EQ(a.to_json(7), (json{{"tableID", 7}, {"type", 2}, {"target", 1}, {"value", 0}}));
}

TEST(PerformAction, PerformRankJson) {
  PerformRank a{1, 3};
  EXPECT_TRUE(a.is_clue());
  EXPECT_EQ(a.hash_int(), 331);
  EXPECT_EQ(a.to_json(7), (json{{"tableID", 7}, {"type", 3}, {"target", 1}, {"value", 3}}));
}

TEST(PerformAction, PerformTerminateJson) {
  PerformTerminate a{0, 0};
  EXPECT_EQ(a.hash_int(), -1);
  EXPECT_EQ(a.to_json(7), (json{{"tableID", 7}, {"type", 4}, {"target", 0}, {"value", 0}}));
}

TEST(PerformAction, FromJsonRoundTrip) {
  std::vector<PerformAction> cases = {
      PerformPlay{2}, PerformDiscard{3}, PerformColour{1, 4},
      PerformRank{1, 5}, PerformTerminate{0, 0},
  };
  for (const auto& orig : cases) {
    json encoded = hanabi::to_json(orig, 99);
    auto decoded = perform_action_from_json(encoded);
    EXPECT_EQ(decoded, orig);
  }
}

TEST(PerformAction, FromJsonInvalidTypeThrows) {
  EXPECT_THROW(perform_action_from_json({{"type", 99}, {"target", 0}}), std::invalid_argument);
}
