// Port of python-bot/tests/conftest.py.
// Original Scala: scala-bot/src/test/util.scala + exAsserts.scala.
//
// Test harness primitives:
// - setup(...) builds a Game in a specific position from short-string hands.
// - parse_action / take_turn apply natural-language actions.
// - pre_clue / fully_known seed conventional clue history.
// - expect_infs / expect_poss / expect_status are assertion helpers.
#pragma once

#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

namespace hanabi::test {

enum class TestPlayer : int {
  ALICE = 0,
  BOB = 1,
  CATHY = 2,
  DONALD = 3,
  EMILY = 4,
};

inline constexpr const char* kPlayerName(TestPlayer p) {
  switch (p) {
    case TestPlayer::ALICE: return "Alice";
    case TestPlayer::BOB: return "Bob";
    case TestPlayer::CATHY: return "Cathy";
    case TestPlayer::DONALD: return "Donald";
    case TestPlayer::EMILY: return "Emily";
  }
  return "?";
}

struct SetupOptions {
  std::vector<std::vector<std::string>> hands;
  std::optional<std::vector<int>> play_stacks;
  std::vector<std::string> discarded;
  int strikes = 0;
  int clue_tokens = 8;
  TestPlayer starting = TestPlayer::ALICE;
  std::string variant_name = "No Variant";
  std::function<void(Game&)> init = [](Game&) {};
  // Optional constructor override (defaults to Game::create).
  std::function<Game(int, State)> constructor = nullptr;
};

Game setup(SetupOptions opts);

// Parse a clue string. Rank: "1".."5". Colour: case-insensitive suit name.
BaseClue str_to_clue(const State& state, std::string_view s);

// Parse a natural-language action.
Action parse_action(const State& state, std::string_view raw);

// Apply an action and the next-turn draw / TurnAction.
Game take_turn(Game game, std::string_view raw_action,
                 std::string_view draw = "");

// Pre-clue: imagine `clues` were already given to (player, slot).
Game pre_clue(Game game, TestPlayer player, int slot,
                std::initializer_list<std::string> clues);

// Fully known: pre-clue both colour and rank for `short`.
Game fully_known(Game game, TestPlayer player, int slot, std::string_view short_);

// Assertion helpers.
void expect_infs(const Game& game, std::optional<TestPlayer> according_to,
                  TestPlayer target, int slot,
                  std::initializer_list<std::string> expected);
void expect_poss(const Game& game, std::optional<TestPlayer> according_to,
                  TestPlayer target, int slot,
                  std::initializer_list<std::string> expected);
void expect_status(const Game& game, TestPlayer target, int slot, CardStatus status);

}  // namespace hanabi::test
