#include "hanabi/net/commands.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "hanabi/basics/action.h"
#include "hanabi/basics/options.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor/reactive_table.h"

namespace hanabi::net {

namespace {

using nlohmann::json;

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> out;
  std::string tok;
  std::istringstream iss(s);
  while (std::getline(iss, tok, sep)) out.push_back(tok);
  return out;
}

bool starts_with(const std::string& s, const std::string& prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::string replace_all(std::string s, const std::string& from, const std::string& to) {
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from.size(), to);
    pos += to.size();
  }
  return s;
}

}  // namespace

BotClient::BotClient(BotTransport& transport, const BotConfig& config)
    : transport_(transport), config_(config) {}

void BotClient::handle_message(const std::string& command, const json& payload) {
  try {
    if (command == "welcome") on_welcome(payload);
    else if (command == "error") on_error(payload);
    else if (command == "warning") on_warning(payload);
    else if (command == "chat") on_chat(payload);
    else if (command == "table") on_table(payload);
    else if (command == "tableList") on_table_list(payload);
    else if (command == "tableGone") on_table_gone(payload);
    else if (command == "tableStart") on_table_start(payload);
    else if (command == "init") on_init(payload);
    else if (command == "gameAction") on_game_action(payload);
    else if (command == "gameActionList") on_game_action_list(payload);
    else if (command == "connected") on_connected(payload);
    else if (command == "gameOver") on_game_over(payload);
    // Other game-tick events (clock, user, databaseID, noteListPlayer,
    // voteChange, spectators) are informational and don't require a response.
  } catch (const std::exception& e) {
    std::cerr << "!! handler for " << command << " raised: " << e.what() << "\n";
  }
}

// --- Inbound -------------------------------------------------------------

void BotClient::on_welcome(const json& data) {
  username_ = data.value("username", "");
  std::cerr << "welcomed as \"" << username_ << "\"\n";
  if (config_.bot_to_join && *config_.bot_to_join == "create") {
    chat_create_table();
  }
}

void BotClient::on_error(const json& data) {
  std::cerr << "server error: " << data.dump() << "\n";
  // If the server tells us another login took over our account, do NOT
  // reconnect - that would just kick out whoever just logged in, and we'd
  // bounce back and forth forever. Tell the transport to exit cleanly so
  // the user can investigate (stale bot process, browser tab, etc.).
  std::string err_msg = data.value("error", "");
  if (err_msg.find("logged on from somewhere else") != std::string::npos) {
    std::cerr << "another session took over this account - exiting "
                  "(check for other running bot processes or browser tabs)\n";
    transport_.stop();
  }
}

void BotClient::on_warning(const json& data) {
  std::cerr << "server warning: " << data.dump() << "\n";
}

void BotClient::on_chat(const json& data) {
  std::string msg = data.value("msg", "");
  if (msg.empty() || msg[0] != '/') return;

  std::string recipient = data.value("recipient", "");
  std::string room = data.value("room", "");
  bool in_pm = recipient == username_;
  bool in_room = recipient.empty() && starts_with(room, "table");
  if (!in_pm && !in_room) return;

  std::vector<std::string> args = split(msg.substr(1), ' ');
  if (args.empty()) return;
  const std::string& cmd = args[0];

  // Commands available from both PM and in-table chat.
  if (cmd == "leaveall") {
    chat_leaveall(room);
    return;
  }
  if (cmd == "settings") {
    chat_settings(room);
    return;
  }

  // Remaining commands are PM-only (matches Python).
  if (!in_pm) return;

  if (cmd == "join") {
    std::optional<std::string> target;
    if (args.size() > 1) target = args[1];
    chat_join(data, target);
  } else if (cmd == "create") {
    chat_create_table();
  } else if (cmd == "start") {
    chat_start();
  } else if (cmd == "setvariant" || cmd == "set_variant") {
    std::optional<std::string> variant;
    if (args.size() > 1) {
      // Decode _ -> space and + -> " & " (matches Python's URL-like munging).
      std::string v = args[1];
      v = replace_all(std::move(v), "_", " ");
      v = replace_all(std::move(v), "+", " & ");
      variant = v;
    }
    chat_set_variant(variant);
  } else if (cmd == "terminate") {
    std::optional<int> tid;
    if (args.size() > 1) {
      try {
        tid = std::stoi(args[1]);
      } catch (...) {
        // ignore parse error; fall through with nullopt
      }
    }
    chat_terminate(tid);
  } else {
    chat_reply("Unknown command: /" + cmd, data.value("who", ""));
  }
}

void BotClient::on_table(const json& data) {
  int tid = data.value("id", -1);
  if (tid != -1) tables_[tid] = data;
}

void BotClient::on_table_list(const json& data) {
  // Server snapshot - can be a JSON array or {list: [...]}.
  json entries;
  if (data.is_array()) {
    entries = data;
  } else if (data.is_object() && data.contains("list")) {
    entries = data["list"];
  } else {
    return;
  }
  for (const auto& entry : entries) {
    int tid = entry.value("id", -1);
    if (tid != -1) tables_[tid] = entry;
  }
  std::cerr << "saw " << tables_.size() << " open table(s) in lobby\n";
}

void BotClient::on_table_gone(const json& data) {
  int tid = data.value("tableID", -1);
  if (tid != -1) tables_.erase(tid);
}

void BotClient::on_table_start(const json& data) {
  int tid = data.value("tableID", -1);
  if (tid == -1) return;
  // Server tells us a game we're in started; request init info.
  transport_.queue_send("getGameInfo1", json{{"tableID", tid}});
}

void BotClient::on_init(const json& data) {
  int tid = data.value("tableID", -1);
  if (tid == -1) return;
  bool is_replay = data.value("replay", false);
  if (!is_replay) games_in_progress_.insert(tid);

  // Build a fresh Game for this table.
  std::vector<std::string> names;
  for (const auto& n : data.at("playerNames")) names.push_back(n.get<std::string>());
  int our_idx = data.value("ourPlayerIndex", 0);
  std::string variant_name = "No Variant";
  TableOptions table_opts;
  if (data.contains("options") && data["options"].is_object()) {
    table_opts = TableOptions::from_json(data["options"]);
    variant_name = data["options"].value("variantName", "No Variant");
  } else {
    table_opts.num_players = static_cast<int>(names.size());
    table_opts.variant_name = variant_name;
  }

  const Variant* variant = nullptr;
  try {
    variant = &get_variant(variant_name);
  } catch (const std::exception& e) {
    std::cerr << "unknown variant '" << variant_name << "': " << e.what() << "\n";
    return;
  }

  State s = State::create(std::move(names), our_idx, *variant, std::move(table_opts));
  auto game = std::make_unique<Game>(Game::create(tid, std::move(s)));
  game->in_progress = !is_replay;
  game->catchup = true;
  games_[tid] = std::move(game);
  action_time_[tid] = false;
  everyone_connected_[tid] = false;
  transport_.queue_send("getGameInfo2", json{{"tableID", tid}});
}

void BotClient::on_game_action(const json& data) {
  int tid = data.value("tableID", -1);
  if (tid == -1) return;
  apply_action(tid, data.at("action"));
  maybe_take_turn(tid);
}

void BotClient::on_game_action_list(const json& data) {
  int tid = data.value("tableID", -1);
  if (tid == -1) return;
  if (data.contains("list") && data["list"].is_array()) {
    for (const auto& raw : data["list"]) apply_action(tid, raw);
  }
  // Done loading. Exit catchup mode and check if it's our turn.
  auto it = games_.find(tid);
  if (it != games_.end()) {
    Game& g = *it->second;
    g.catchup = false;
    bool our_turn = g.state.current_player_index == g.state.our_player_index;
    action_time_[tid] = our_turn;
  }
  transport_.queue_send("loaded", json{{"tableID", tid}});
  maybe_take_turn(tid);
}

void BotClient::on_connected(const json& data) {
  int tid = data.value("tableID", -1);
  if (tid == -1) return;
  bool all_connected = true;
  if (data.contains("list") && data["list"].is_array()) {
    for (const auto& v : data["list"]) {
      if (!v.is_boolean() || !v.get<bool>()) {
        all_connected = false;
        break;
      }
    }
  }
  everyone_connected_[tid] = all_connected;
  maybe_take_turn(tid);
}

void BotClient::apply_action(int table_id, const json& raw_action) {
  auto it = games_.find(table_id);
  if (it == games_.end()) return;
  auto act = action_from_json(raw_action);
  if (!act) return;
  try {
    it->second->handle_action(*act);
  } catch (const std::exception& e) {
    std::cerr << "!! handle_action failed at table " << table_id << ": " << e.what()
              << "\n";
    return;
  }
  if (std::holds_alternative<TurnAction>(*act)) {
    const auto& ta = std::get<TurnAction>(*act);
    bool our_turn = ta.current_player_index == it->second->state.our_player_index;
    action_time_[table_id] = our_turn;
  }
}

void BotClient::maybe_take_turn(int table_id) {
  auto it = games_.find(table_id);
  if (it == games_.end()) return;
  Game& g = *it->second;
  bool everyone = everyone_connected_[table_id];
  bool action_time = action_time_[table_id];
  if (g.catchup || !g.in_progress || !everyone || !action_time) return;
  if (g.state.current_player_index != g.state.our_player_index) return;

  PerformAction perform;
  try {
    perform = g.take_action();
  } catch (const std::exception& e) {
    std::cerr << "!! take_action failed for table " << table_id << ": " << e.what()
              << "\n";
    return;
  }
  std::cerr << "-> action " << hanabi::to_json(perform, table_id).dump() << "\n";
  transport_.queue_send("action", hanabi::to_json(perform, table_id));
  action_time_[table_id] = false;
}

void BotClient::on_game_over(const json& data) {
  int tid = data.value("tableID", -1);
  if (tid == -1) return;
  games_in_progress_.erase(tid);
  std::cerr << "game over at table " << tid << "\n";
  if (config_.disconnect_on_game_end) {
    transport_.queue_send("tableUnattend", json{{"tableID", tid}});
  }
}

// --- Outbound chat commands ----------------------------------------------

void BotClient::chat_reply(const std::string& message, const std::string& who) {
  transport_.queue_send(
      "chatPM", json{{"msg", message}, {"recipient", who}, {"room", "lobby"}});
}

void BotClient::chat_join(const json& data, std::optional<std::string> target) {
  std::string sender = data.value("who", "");
  std::string target_name = target.value_or(sender);

  for (const auto& [tid, table] : tables_) {
    if (table.value("joined", false)) continue;
    if (table.value("running", false)) continue;  // game already started; can't join
    const auto players_it = table.find("players");
    if (players_it == table.end() || !players_it->is_array()) continue;
    bool has_target = false;
    for (const auto& p : *players_it) {
      if (p.is_string() && p.get<std::string>() == target_name) {
        has_target = true;
        break;
      }
    }
    if (!has_target) continue;
    transport_.queue_send("tableJoin", json{{"tableID", tid}});
    return;
  }
  chat_reply("No open table containing '" + target_name + "' to join", sender);
}

void BotClient::chat_create_table() {
  transport_.queue_send(
      "tableCreate",
      json{
          {"name", config_.table_name},
          {"options",
            json{
                {"variantName", "No Variant"},
                {"speedrun", false},
                {"deckPlays", false},
                {"emptyClues", false},
                {"oneExtraCard", false},
                {"oneLessCard", false},
                {"detrimentalCharacters", false},
            }},
          {"password", ""},
          {"maxPlayers", config_.max_num_players},
      });
}

void BotClient::chat_start() {
  for (const auto& [tid, table] : tables_) {
    if (table.value("joined", false) && !table.value("running", false)) {
      transport_.queue_send("tableStart", json{{"tableID", tid}});
      return;
    }
  }
}

void BotClient::chat_set_variant(std::optional<std::string> variant) {
  if (!variant) return;
  for (const auto& [tid, table] : tables_) {
    if (table.value("joined", false) && !table.value("running", false)) {
      transport_.queue_send(
          "tableSetVariant",
          json{{"tableID", tid}, {"options", json{{"variantName", *variant}}}});
      return;
    }
  }
}

void BotClient::chat_terminate(std::optional<int> table_id) {
  std::vector<int> targets;
  if (table_id) {
    targets.push_back(*table_id);
  } else {
    for (int tid : games_in_progress_) targets.push_back(tid);
  }
  for (int tid : targets) {
    PerformTerminate term{0, 0};
    transport_.queue_send("action", term.to_json(tid));
  }
}

std::optional<int> BotClient::resolve_target_table(const std::string& room) const {
  if (starts_with(room, "table")) {
    try {
      return std::stoi(room.substr(5));
    } catch (...) {
      return std::nullopt;
    }
  }
  // PM context: use the single known game/table if there's exactly one.
  if (games_in_progress_.size() == 1) return *games_in_progress_.begin();
  if (tables_.size() == 1) return tables_.begin()->first;
  return std::nullopt;
}

void BotClient::chat_leaveall(const std::string& room) {
  auto tid = resolve_target_table(room);
  if (!tid) return;
  bool in_game = games_in_progress_.count(*tid) > 0;
  const char* cmd = in_game ? "tableUnattend" : "tableLeave";
  transport_.queue_send(cmd, json{{"tableID", *tid}});
}

void BotClient::chat_settings(const std::string& room) {
  auto tid = resolve_target_table(room);
  if (!tid) return;

  // Determine the variant + hand_size.
  const Variant* variant = nullptr;
  int hand_size = 0;

  auto table_it = tables_.find(*tid);
  if (table_it == tables_.end()) return;
  const auto& table = table_it->second;
  std::string variant_name;
  if (table.contains("options") && table["options"].is_object()) {
    variant_name = table["options"].value("variantName", "");
  }
  if (variant_name.empty()) {
    variant_name = table.value("variant", "");
  }
  if (variant_name.empty()) return;

  try {
    variant = &get_variant(variant_name);
  } catch (const std::exception& e) {
    std::cerr << "settings: unknown variant '" << variant_name << "': " << e.what() << "\n";
    return;
  }

  int num_players = 3;
  if (table.contains("options") && table["options"].is_object() &&
      table["options"].contains("numPlayers")) {
    num_players = table["options"].value("numPlayers", 3);
  } else if (table.contains("numPlayers")) {
    num_players = table.value("numPlayers", 3);
  }
  if (num_players < 2 || num_players >= static_cast<int>(kHandSize.size())) {
    num_players = 3;
  }
  hand_size = kHandSize[num_players];

  std::string msg = hanabi::reactor::format_reactive_settings(*variant, hand_size);
  transport_.queue_send(
      "chat",
      json{{"msg", msg}, {"recipient", ""}, {"room", "table" + std::to_string(*tid)}});
}

}  // namespace hanabi::net
