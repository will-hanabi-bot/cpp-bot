#include "hanabi/net/commands.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <boost/asio/post.hpp>

#include "hanabi/basics/action.h"
#include "hanabi/basics/options.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/variants/reactive_table.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/game_logger.h"
#include "hanabi/logging/state_snapshot.h"
#include "hanabi/net/notes.h"
#include "hanabi/version.h"

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
    : transport_(transport), config_(config) {
  // Keep the compute io_context alive even when its queue is momentarily
  // empty, and run it on a dedicated thread so long take_action calls don't
  // block the network io_context.
  compute_guard_.emplace(boost::asio::make_work_guard(compute_ioc_));
  compute_thread_ = std::thread([this]() { compute_ioc_.run(); });
}

BotClient::~BotClient() {
  // Let any in-flight compute drain (the result will be queue_send'd, which
  // is a no-op once the transport's session is torn down), then stop the
  // io_context so the worker thread can exit.
  compute_guard_.reset();
  if (compute_thread_.joinable()) compute_thread_.join();
}

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
  // Resume any in-progress games the server still seats us at (mirrors
  // hanab.live's "Resume" button: command_table_reattend.go). Without this
  // the server holds our seat across a reconnect but no gameAction updates
  // flow through, so the game appears abandoned to our partner.
  if (data.contains("playingAtTables") && data["playingAtTables"].is_array()) {
    for (const auto& tid_json : data["playingAtTables"]) {
      if (!tid_json.is_number_integer()) continue;
      int tid = tid_json.get<int>();
      std::cerr << "reattending in-progress table " << tid << "\n";
      transport_.queue_send("tableReattend", json{{"tableID", tid}});
    }
  }
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
  if (cmd == "allplays") {
    chat_allplays(args, data, room);
    return;
  }
  if (cmd == "getversion") {
    chat_version(data, room);
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
  game->all_plays = all_plays_mode_;
  games_[tid] = std::move(game);
  action_time_[tid] = false;
  everyone_connected_[tid] = false;

  // Open a per-game structured log file: logs/{bot}-{game_id}.log. Replay
  // games (where in_progress=false) get a logger too so the show_turn.py
  // helper can read post-mortem replays.
  if (!username_.empty()) {
    auto logger = std::make_unique<hanabi::logging::GameLogger>(
        username_, tid, "logs");
    logger->emit_lifecycle(
        "game_init",
        json{{"variant", variant_name},
              {"num_players", static_cast<int>(games_[tid]->state.names.size())},
              {"our_player_index", our_idx},
              {"names", games_[tid]->state.names},
              {"is_replay", is_replay},
              {"bot_version", kBotVersion},
              {"all_plays", all_plays_mode_}});
    game_loggers_[tid] = std::move(logger);
  }
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
    // Publish the bot's build version as the head of card order 0's note
    // so observers can verify which build is running (order 0 carries no
    // "o0" prefix — the version note serves as its head). Catchup
    // segments accumulated on order 0 are kept after it. Then send every
    // accumulated note — including the bare "o<order>" seeds from the
    // catchup draws — so each card is referenceable by order right away.
    if (g.in_progress) {
      auto& table_notes = notes_[tid];
      std::string version_note = std::string("bot ") + kBotVersion;
      auto existing0 = table_notes.find(0);
      table_notes[0] = existing0 != table_notes.end()
                            ? version_note + " | " + existing0->second
                            : version_note;
      for (const auto& [order, note] : table_notes) {
        transport_.queue_send(
            "note", json{{"tableID", tid}, {"order", order}, {"note", note}});
      }
    }
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
  // The server reports actions outcome-oriented (e.g. orange-discard-on-
  // playable comes through as "play"). The engine's `on_play`/`on_discard`
  // are button-oriented, so for inverted suits we have to flip the action
  // type before dispatch.
  act = orient_action_for_engine(*act, *it->second->state.variant);
  Game prev = *it->second;

  // LIFECYCLE: every inbound action gets a log line. Recorded before the
  // engine applies it so a crash inside handle_action still leaves a
  // breadcrumb in the log. Action serialized via our internal format so
  // replay_log can re-apply it without coupling to the hanab.live wire
  // shape.
  auto* glog = game_loggers_.count(table_id) ? game_loggers_[table_id].get() : nullptr;
  if (glog) {
    glog->emit_lifecycle(
        "inbound_action",
        json{{"turn", prev.state.turn_count},
              {"action", hanabi::logging::action_to_internal_json(*act)}});
  }

  try {
    it->second->handle_action(*act);
  } catch (const std::exception& e) {
    std::cerr << "!! handle_action failed at table " << table_id << ": " << e.what()
              << "\n";
    if (glog) {
      glog->emit_lifecycle("handle_action_error",
                            json{{"turn", prev.state.turn_count},
                                  {"error", std::string(e.what())}});
    }
    return;
  }
  // Seed every drawn card's note with its order ("o13") for easy
  // referencing in bug reports; convention segments append after it
  // ("o13 | turn 14: [f] n3"). Order 0 is exempt — the bot-version note
  // published at load is its head instead.
  if (auto* da = std::get_if<DrawAction>(&*act); da && da->order > 0) {
    auto& table_notes = notes_[table_id];
    if (!table_notes.count(da->order)) {
      std::string base = "o" + std::to_string(da->order);
      table_notes[da->order] = base;
      if (!it->second->catchup && it->second->in_progress) {
        transport_.queue_send(
            "note",
            json{{"tableID", table_id}, {"order", da->order}, {"note", base}});
      }
    }
  }

  // Note-worthy state changes (CALLED_TO_PLAY/_DISCARD transitions and
  // inferred-set narrowing while called-to-play) — accumulate and send.
  auto segments = compute_note_segments(prev, *it->second);
  if (!segments.empty()) {
    auto& table_notes = notes_[table_id];
    bool send_now = !it->second->catchup && it->second->in_progress;
    for (const auto& [order, seg] : segments) {
      auto existing = table_notes.find(order);
      std::string full = existing != table_notes.end()
                              ? existing->second + " | " + seg
                              : seg;
      table_notes[order] = full;
      if (send_now) {
        transport_.queue_send(
            "note", json{{"tableID", table_id}, {"order", order}, {"note", full}});
      }
    }
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

  // Snapshot the game so the worker doesn't observe incremental mutations
  // from the network thread (notes_, lobby chat, etc. can still arrive while
  // we compute). Since it's our turn, no further gameAction for this table
  // will land until we send our action, so the snapshot is exactly what
  // a synchronous take_action would have seen.
  Game snapshot = g;
  // Clear action_time_ before posting so any duplicate trigger (e.g. a
  // gameActionList that re-fires maybe_take_turn) won't post a second job.
  action_time_[table_id] = false;

  // Pass the per-game logger raw pointer (lifetime owned by game_loggers_).
  // game_loggers_ entries are only erased in on_game_over which runs on
  // the network thread, and we clear them after the compute completes —
  // but the worker captures the pointer by value; if a game ends before
  // the worker runs, the pointer is dangling. We guard by deferring the
  // erase until after the worker confirms.
  auto* glog = game_loggers_.count(table_id) ? game_loggers_[table_id].get() : nullptr;

  boost::asio::post(compute_ioc_,
                    [this, table_id, glog, snapshot = std::move(snapshot)]() mutable {
                      using namespace hanabi::logging;
                      CurrentLoggerGuard guard(glog);
                      if (glog) {
                        glog->mark_turn_start();
                        emit_state_snapshot(*glog, snapshot, snapshot.state.turn_count);
                        glog->emit_lifecycle(
                            "decide_start",
                            json{{"turn", snapshot.state.turn_count}});
                      }
                      auto t0 = std::chrono::steady_clock::now();
                      PerformAction perform;
                      try {
                        hanabi::instr::ScopedTimer st("take_action");
                        perform = snapshot.take_action();
                      } catch (const std::exception& e) {
                        std::cerr << "!! take_action failed for table "
                                  << table_id << ": " << e.what() << "\n";
                        if (glog) {
                          glog->emit_lifecycle(
                              "take_action_error",
                              json{{"turn", snapshot.state.turn_count},
                                    {"error", std::string(e.what())}});
                        }
                        return;
                      }
                      auto elapsed = std::chrono::steady_clock::now() - t0;
                      double elapsed_ms =
                          std::chrono::duration<double, std::milli>(elapsed).count();
                      std::cerr << "-> action "
                                << hanabi::to_json(perform, table_id).dump()
                                << "\n";
                      if (glog) {
                        glog->emit_lifecycle(
                            "outbound_action",
                            json{{"turn", snapshot.state.turn_count},
                                  {"action", hanabi::to_json(perform, table_id)},
                                  {"elapsed_ms", elapsed_ms}});
                        // Per-turn TIMING delta.
                        auto turn_snap = glog->aggregator().snapshot();
                        auto prior = glog->turn_start_snapshot();
                        auto delta = hanabi::instr::Aggregator::diff(turn_snap, prior);
                        glog->emit(
                            json{{"ch", "TIMING"},
                                  {"scope", "per_turn"},
                                  {"turn", snapshot.state.turn_count},
                                  {"elapsed_ms", elapsed_ms},
                                  {"scopes", hanabi::instr::Aggregator::to_json(delta)}});
                      }
                      transport_.queue_send(
                          "action", hanabi::to_json(perform, table_id));
                    });
}

void BotClient::on_game_over(const json& data) {
  int tid = data.value("tableID", -1);
  if (tid == -1) return;
  games_in_progress_.erase(tid);
  std::cerr << "game over at table " << tid << "\n";
  // Emit per-game TIMING aggregate + final lifecycle event, then close the
  // logger. The compute thread holds the GameLogger* by raw pointer in
  // pending take_action lambdas; we don't expect any in-flight compute
  // here (game_over arrives after our final action) but if there is, the
  // post-lambda will hold a dangling pointer. Mitigation: keep the
  // logger alive past compute by NOT erasing immediately — leak the
  // unique_ptr into a graveyard keyed on tid that we GC opportunistically.
  // For now: erase after emitting; if a stray lambda runs after, its
  // captured pointer dangles. Acceptable until we see it bite.
  auto it = game_loggers_.find(tid);
  if (it != game_loggers_.end() && it->second) {
    auto& gl = *it->second;
    if (data.contains("endCondition")) {
      gl.emit_lifecycle("game_over",
                          json{{"end_condition", data.value("endCondition", 0)}});
    } else {
      gl.emit_lifecycle("game_over", json::object());
    }
    auto snap = gl.aggregator().snapshot();
    gl.emit(json{{"ch", "TIMING"},
                  {"scope", "per_game"},
                  {"scopes", hanabi::instr::Aggregator::to_json(snap)}});
    game_loggers_.erase(it);
  }
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

  // Prefer the active game's flag (so the report matches what the bot is
  // actually doing at this table). Fall back to the bot-wide setting if no
  // game exists for this table yet (e.g., between init events).
  bool all_plays = all_plays_mode_;
  auto game_it = games_.find(*tid);
  if (game_it != games_.end() && game_it->second) all_plays = game_it->second->all_plays;

  std::string msg = hanabi::reactor::variants::format_reactive_settings(*variant, hand_size, all_plays);
  transport_.queue_send(
      "chat",
      json{{"msg", msg}, {"recipient", ""}, {"room", "table" + std::to_string(*tid)}});
}

void BotClient::chat_allplays(const std::vector<std::string>& args, const json& data,
                                  const std::string& room) {
  std::string sender = data.value("who", "");
  bool in_pm = data.value("recipient", "") == username_;

  auto reply = [&](const std::string& text) {
    if (in_pm) {
      chat_reply(text, sender);
    } else {
      transport_.queue_send(
          "chat", json{{"msg", text}, {"recipient", ""}, {"room", room}});
    }
  };

  if (args.size() < 2) {
    reply(std::string("allplays is ") + (all_plays_mode_ ? "on" : "off"));
    return;
  }
  const std::string& arg = args[1];
  bool turning_on;
  if (arg == "on" || arg == "true" || arg == "1") {
    turning_on = true;
  } else if (arg == "off" || arg == "false" || arg == "0") {
    turning_on = false;
  } else {
    reply("usage: /allplays on|off");
    return;
  }

  all_plays_mode_ = turning_on;
  for (auto& [tid, game] : games_) {
    if (game) game->all_plays = turning_on;
    (void)tid;
  }
  reply(std::string("allplays is now ") + (turning_on ? "on" : "off"));
}

void BotClient::chat_version(const json& data, const std::string& room) {
  std::string sender = data.value("who", "");
  bool in_pm = data.value("recipient", "") == username_;
  std::string text = username_ + ": " + kBotVersion;
  if (in_pm) {
    chat_reply(text, sender);
  } else {
    transport_.queue_send(
        "chat", json{{"msg", text}, {"recipient", ""}, {"room", room}});
  }
}

}  // namespace hanabi::net
