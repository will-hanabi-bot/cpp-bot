// Port of python-bot/src/hanabi_bot/basics/action.py.
// Original Scala: scala-bot/src/scala_bot/basics/Action.scala.
//
// Action: server → bot events (clue happened, card drawn, strike, ...).
// PerformAction: bot → server commands (play, discard, clue, terminate).
#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/interp.h"

namespace hanabi {

struct Variant;


// --- Inbound: server → bot events ----------------------------------------

struct StatusAction {
  int clues;
  int score;
  int max_score;

  int player_index() const { return -1; }
  bool requires_draw() const { return false; }
  bool is_player_action() const { return false; }

  static StatusAction from_json(const nlohmann::json& obj);
  bool operator==(const StatusAction&) const = default;
};

struct TurnAction {
  int num;
  int current_player_index;

  int player_index() const { return current_player_index; }
  bool requires_draw() const { return false; }
  bool is_player_action() const { return false; }

  static TurnAction from_json(const nlohmann::json& obj);
  bool operator==(const TurnAction&) const = default;
};

struct ClueAction {
  int giver;
  int target;
  std::vector<int> list_;
  BaseClue clue;

  ClueAction(int g, int t, std::vector<int> l, BaseClue c)
      : giver(g), target(t), list_(std::move(l)), clue(c) {}

  int player_index() const { return giver; }
  bool requires_draw() const { return false; }
  bool is_player_action() const { return true; }

  static ClueAction from_json(const nlohmann::json& obj);
  bool operator==(const ClueAction&) const = default;
};

struct DrawAction {
  int player_index_v;
  int order;
  int suit_index;
  int rank;

  int player_index() const { return player_index_v; }
  bool requires_draw() const { return false; }
  bool is_player_action() const { return false; }

  static DrawAction from_json(const nlohmann::json& obj);
  bool operator==(const DrawAction&) const = default;
};

struct PlayAction {
  int player_index_v;
  int order;
  int suit_index;
  int rank;

  int player_index() const { return player_index_v; }
  bool requires_draw() const { return true; }
  bool is_player_action() const { return true; }

  static PlayAction from_json(const nlohmann::json& obj);
  bool operator==(const PlayAction&) const = default;
};

struct DiscardAction {
  int player_index_v;
  int order;
  int suit_index;
  int rank;
  bool failed = false;

  int player_index() const { return player_index_v; }
  bool requires_draw() const { return true; }
  bool is_player_action() const { return true; }

  static DiscardAction from_json(const nlohmann::json& obj);
  bool operator==(const DiscardAction&) const = default;
};

struct StrikeAction {
  int num;
  int turn;
  int order;

  int player_index() const { return -1; }
  bool requires_draw() const { return false; }
  bool is_player_action() const { return false; }

  static StrikeAction from_json(const nlohmann::json& obj);
  bool operator==(const StrikeAction&) const = default;
};

enum class EndCondition : std::uint8_t {
  IN_PROGRESS = 0,
  NORMAL = 1,
  STRIKEOUT = 2,
  TIMEOUT = 3,
  TERMINATED = 4,
  SPEEDRUN_FAIL = 5,
  IDLE_TIMEOUT = 6,
  CHARACTER_SOFTLOCK = 7,
  ALL_OR_NOTHING_FAIL = 8,
  ALL_OR_NOTHING_SOFTLOCK = 9,
  TERMINATED_BY_VOTE = 10,
};

struct GameOverAction {
  int end_condition;
  int player_index_v;

  int player_index() const { return player_index_v; }
  bool requires_draw() const { return false; }
  bool is_player_action() const { return false; }

  static GameOverAction from_json(const nlohmann::json& obj);
  bool operator==(const GameOverAction&) const = default;
};

struct InterpAction {
  Interp interp;

  int player_index() const { return -1; }
  bool requires_draw() const { return false; }
  bool is_player_action() const { return false; }

  bool operator==(const InterpAction&) const = default;
};

using Action = std::variant<StatusAction, TurnAction, ClueAction, DrawAction,
                            PlayAction, DiscardAction, StrikeAction,
                            GameOverAction, InterpAction>;

// Parse a single inbound action. Returns nullopt for unknown types.
std::optional<Action> action_from_json(const nlohmann::json& obj);

// Convert an outcome-oriented server action into the engine's button-oriented
// action shape. Hanab.live's gameAction (and replay export) reports outcomes:
// "play" / type=0 = card landed on a play stack, "discard" / type=1 = card
// landed in the discard pile. cpp-bot's engine `on_play` / `on_discard`
// invert play↔discard for inverted (Orange / Dark Orange) suits, so we have
// to flip the action type before dispatch when the suit is inverted; the
// engine then arrives at the right side of the inversion. No-op for
// non-inverted suits and for non-suit actions (Clue / Draw / Turn / etc).
Action orient_action_for_engine(Action act, const Variant& variant);

// Polymorphic accessors over Action.
inline int player_index(const Action& a) {
  return std::visit([](const auto& v) { return v.player_index(); }, a);
}
inline bool requires_draw(const Action& a) {
  return std::visit([](const auto& v) { return v.requires_draw(); }, a);
}
inline bool is_player_action(const Action& a) {
  return std::visit([](const auto& v) { return v.is_player_action(); }, a);
}

// --- Outbound: bot → server ----------------------------------------------

struct PerformPlay {
  int target;

  bool is_clue() const { return false; }
  int hash_int() const { return target; }
  nlohmann::json to_json(int table_id) const;
  bool operator==(const PerformPlay&) const = default;
};

struct PerformDiscard {
  int target;

  bool is_clue() const { return false; }
  int hash_int() const { return 10 + target; }
  nlohmann::json to_json(int table_id) const;
  bool operator==(const PerformDiscard&) const = default;
};

struct PerformColour {
  int target;  // player index
  int value;   // colourable-suit index

  bool is_clue() const { return true; }
  int hash_int() const { return 20 + target + value * 100; }
  nlohmann::json to_json(int table_id) const;
  bool operator==(const PerformColour&) const = default;
};

struct PerformRank {
  int target;  // player index
  int value;   // rank 1..5

  bool is_clue() const { return true; }
  int hash_int() const { return 30 + target + value * 100; }
  nlohmann::json to_json(int table_id) const;
  bool operator==(const PerformRank&) const = default;
};

struct PerformTerminate {
  int target;
  int value;

  bool is_clue() const { return false; }
  int hash_int() const { return -1; }
  nlohmann::json to_json(int table_id) const;
  bool operator==(const PerformTerminate&) const = default;
};

using PerformAction = std::variant<PerformPlay, PerformDiscard, PerformColour,
                                   PerformRank, PerformTerminate>;

PerformAction perform_action_from_json(const nlohmann::json& obj);

inline bool is_clue(const PerformAction& p) {
  return std::visit([](const auto& v) { return v.is_clue(); }, p);
}
inline int hash_int(const PerformAction& p) {
  return std::visit([](const auto& v) { return v.hash_int(); }, p);
}
inline nlohmann::json to_json(const PerformAction& p, int table_id) {
  return std::visit([&](const auto& v) { return v.to_json(table_id); }, p);
}

}  // namespace hanabi
