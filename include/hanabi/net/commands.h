// Port of python-bot/src/hanabi_bot/net/commands.py.
//
// Per-connection message dispatcher. Handles inbound messages from
// hanab.live (welcome, chat, table, tableList, init, gameAction,
// gameActionList, connected, gameOver, ...), drives the full game lifecycle
// (per-table Game instances, take_action on our turn via a dedicated compute
// thread), and sends outbound chat commands (/join, /settings, /leaveall,
// /create, /start, /setvariant, /terminate).
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "hanabi/basics/clue.h"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <nlohmann/json.hpp>

#include "hanabi/basics/game.h"
#include "hanabi/logging/game_logger.h"
#include "hanabi/net/ws_transport.h"
#include "hanabi/settings.h"

namespace hanabi::net {

class BotClient {
 public:
  BotClient(BotTransport& transport, const BotConfig& config);
  ~BotClient();

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
  // Per-table Game instance and turn-tracking flags.
  // unique_ptr so Game's address is stable across map rehashes (Game is large).
  std::unordered_map<int, std::unique_ptr<Game>> games_;
  std::unordered_map<int, bool> action_time_;
  std::unordered_map<int, bool> everyone_connected_;
  // Per-table per-card-order accumulated note text (mirrors Python's
  // game.notes). compute_note_segments returns deltas; we append to the
  // full string here and re-send it on each change.
  std::unordered_map<int, std::unordered_map<int, std::string>> notes_;
  // Per-table structured JSONL logger. Created in on_init, closed in
  // on_game_over. See include/hanabi/logging/game_logger.h.
  std::unordered_map<int, std::unique_ptr<hanabi::logging::GameLogger>>
      game_loggers_;

  // Reactor /allplays toggle. Defaults to false (standard convention). When
  // set via "/allplays on" in chat, propagated into every active Game's
  // all_plays field; new games inherit it at on_init.
  bool all_plays_mode_ = false;

  // Which convention NEW games run (seeded from BotConfig::convention in
  // the ctor; changed via "/setall reactor|reactor0" in chat). Deliberately
  // NOT propagated into running games — a mid-game convention switch would
  // desync the table's shared interpretation, so the change lands at the
  // next on_init. 4+ player games always run REACTOR (reactor0 is a
  // 3-player convention for now); the per-game resolution happens in
  // on_init.
  Convention convention_mode_ = Convention::REACTOR0;
  // reactor0 /rlocks override. nullopt = use the variant's default
  // (starting required efficiency <= 1.42). Set via "/rlocks on|off";
  // propagated into every active Game like all_plays.
  std::optional<bool> rlocks_mode_;
  // reactor0 `/set` reassignments, bot-wide. Copied into each new Game and
  // retro-applied to running ones, like rlocks -- so they SURVIVE a game
  // start, which is the point: a player sets them in the lobby or a replay and
  // expects them to hold for the games that follow.
  //
  // They are keyed to a variant. `ReactiveOverride::clue_value` is a raw index
  // into `Variant::clue_colour_names`, so the same override carried onto a
  // different variant would silently re-point at another colour. Applicability
  // is therefore never tested inline -- always through `overrides_for`.
  std::vector<ReactiveOverride> reactive_overrides_;
  // The variant `reactive_overrides_` were authored against. Empty when there
  // are none.
  std::string overrides_variant_;

  // Dedicated compute thread for take_action. Without this, the long-running
  // endgame solver blocks the network io_context (held by BotTransport), the
  // server's WS heartbeat goes unanswered, and the connection is closed (the
  // "ws recv error: End of file" we'd see whenever the solver ran > ~PongWait
  // seconds). The compute thread takes a snapshot of the Game, runs
  // take_action on it, and uses transport_.queue_send (already thread-safe)
  // to send the result. Re-entry is gated by action_time_ — we clear it
  // before posting and the next TurnAction reasserts it once we've acted.
  boost::asio::io_context compute_ioc_;
  std::optional<boost::asio::executor_work_guard<
      boost::asio::io_context::executor_type>>
      compute_guard_;
  std::thread compute_thread_;

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
  void on_game_action(const nlohmann::json& data);
  void on_game_action_list(const nlohmann::json& data);
  void on_connected(const nlohmann::json& data);
  void on_game_over(const nlohmann::json& data);
  void on_database_id(const nlohmann::json& data);

  // Game-end bookkeeping, shared by the two paths that can report it: the
  // `gameOver` gameAction (the one that actually arrives) and a top-level
  // `gameOver` command. Idempotent -- gated on games_in_progress_ -- so a
  // table that reports both only finishes once. Deliberately leaves the
  // logger open; it is closed by on_database_id once the database id
  // arrives, which is strictly later than game end.
  void finish_game(int tid, std::optional<int> end_condition);

  // Apply an inbound action through the game; updates per-table action_time.
  void apply_action(int table_id, const nlohmann::json& raw_action);
  // Check the gating conditions and queue an action if it's our turn.
  void maybe_take_turn(int table_id);

  // --- Outbound chat commands ---
  void chat_reply(const std::string& message, const std::string& who);
  void chat_join(const nlohmann::json& data, std::optional<std::string> target);
  void chat_create_table();
  void chat_start();
  void chat_set_variant(std::optional<std::string> variant);
  void chat_terminate(std::optional<int> table_id);
  void chat_leaveall(const std::string& room);
  void chat_settings(const std::string& room);
  void chat_allplays(const std::vector<std::string>& args, const nlohmann::json& data,
                       const std::string& room);
  void chat_rlocks(const std::vector<std::string>& args, const nlohmann::json& data,
                     const std::string& room);
  // The variant and player count for a table, resolved the way `/settings`
  // does. Nullopt when the table is unknown or names a variant we cannot load.
  struct TableInfo {
    const Variant* variant = nullptr;
    int num_players = 3;
  };
  std::optional<TableInfo> table_info(int table_id) const;

  void chat_set(const std::vector<std::string>& args, const nlohmann::json& data,
                const std::string& room);
  void chat_setall(const std::vector<std::string>& args, const nlohmann::json& data,
                     const std::string& room);
  void chat_version(const nlohmann::json& data, const std::string& room);

  // Helper: for commands that work from PM or a table room, pick the target table.
  std::optional<int> resolve_target_table(const std::string& room) const;

  // The single applicability test for `/set` overrides: they apply to
  // `variant` only if that is the variant they were authored against.
  // Returns an empty list otherwise, which means "use the variant's built-in
  // table" everywhere it is consumed. Non-destructive -- a game on another
  // variant does not discard the stored list, so opening a replay of one does
  // not cost the player their settings.
  const std::vector<ReactiveOverride>& overrides_for(const Variant& variant) const;

 public:
  // Test seam: the convention-selection state a game was created with.
  // Games_ is private and Game is large; this exposes just the two mode
  // fields so tests/test_net/test_setall.cpp can assert what on_init
  // resolved without a friend declaration or a Game copy.
  struct GameModes {
    Convention convention;
    bool allow_reactive_locks;
    bool all_plays;
    std::vector<ReactiveOverride> reactive_overrides;
  };
  std::optional<GameModes> debug_game_snapshot(int table_id) const;
};

}  // namespace hanabi::net
