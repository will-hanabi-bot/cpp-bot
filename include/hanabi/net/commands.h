// Port of python-bot/src/hanabi_bot/net/commands.py.
//
// Per-connection chat-command dispatcher. Handles inbound messages from
// hanab.live (welcome, chat, table, tableList, ...) and outbound chat
// commands (/join, /settings, /leaveall, /create, /start, /setvariant,
// /terminate).
//
// Scope vs. Python: game lifecycle (init/gameAction/gameActionList/connected/
// clock) is stubbed - we track only enough state for the chat commands to
// behave correctly (whether a table has an in-progress game so /leaveall
// picks tableLeave vs tableUnattend). Full game lifecycle + take_action
// land once the reactor convention port is complete.
#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "hanabi/net/ws_transport.h"
#include "hanabi/settings.h"

namespace hanabi::net {

class BotClient {
 public:
  BotClient(BotTransport& transport, const BotConfig& config);

  // Top-level dispatcher: routes a decoded message to the appropriate handler.
  void handle_message(const std::string& command, const nlohmann::json& payload);

 private:
  BotTransport& transport_;
  const BotConfig& config_;
  std::string username_;

  // Tables visible in the lobby (id -> table info from `table` / `tableList`).
  std::unordered_map<int, nlohmann::json> tables_;
  // Table IDs we've received an `init` for and haven't yet gotten `gameOver` for.
  // Used by /leaveall to pick tableLeave (pregame) vs tableUnattend (in game).
  std::unordered_set<int> games_in_progress_;

  // --- Inbound handlers ---
  void on_welcome(const nlohmann::json& data);
  void on_error(const nlohmann::json& data);
  void on_warning(const nlohmann::json& data);
  void on_chat(const nlohmann::json& data);
  void on_table(const nlohmann::json& data);
  void on_table_list(const nlohmann::json& data);
  void on_table_gone(const nlohmann::json& data);
  void on_table_start(const nlohmann::json& data);
  void on_init(const nlohmann::json& data);
  void on_game_over(const nlohmann::json& data);

  // --- Outbound chat commands ---
  void chat_reply(const std::string& message, const std::string& who);
  void chat_join(const nlohmann::json& data, std::optional<std::string> target);
  void chat_create_table();
  void chat_start();
  void chat_set_variant(std::optional<std::string> variant);
  void chat_terminate(std::optional<int> table_id);
  void chat_leaveall(const std::string& room);
  void chat_settings(const std::string& room);

  // Helper: for commands that work from PM or a table room, pick the target table.
  std::optional<int> resolve_target_table(const std::string& room) const;
};

}  // namespace hanabi::net
