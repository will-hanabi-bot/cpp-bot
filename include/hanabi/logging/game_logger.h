// Per-game JSON Lines logger for the iteration-tooling overhaul.
//
// One GameLogger instance per active table: file is `logs/{bot}-{game_id}.log`,
// opened append-only. Every emit() flushes a single JSONL record. The logger
// is shared between the network thread (LIFECYCLE events on inbound actions)
// and the compute thread (STATE before take_action, DECIDE traces, LIFECYCLE
// outbound, TIMING). Concurrency is gated by an internal mutex.
//
// A thread-local "current" pointer routes scope-local helpers (LogScope,
// ScopedTimer) to the right game. The compute thread sets it for the duration
// of take_action(); the network thread sets it before each apply_action call.
#pragma once

#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "hanabi/instrumentation/timer.h"

namespace hanabi::logging {

// One per active game. Owns the open log file + the per-game Aggregator.
class GameLogger {
 public:
  GameLogger(std::string bot_name, int game_id, const std::string& log_dir);
  ~GameLogger();

  GameLogger(const GameLogger&) = delete;
  GameLogger& operator=(const GameLogger&) = delete;

  int game_id() const { return game_id_; }
  const std::string& bot_name() const { return bot_name_; }
  const std::string& path() const { return path_; }

  // Write one JSONL record. Adds `ts` if missing.
  void emit(nlohmann::json record);

  // Convenience: emit with a channel + extra fields merged in.
  void emit_lifecycle(std::string_view event, nlohmann::json extra = {});

  instr::Aggregator& aggregator() { return aggregator_; }
  const instr::Aggregator& aggregator() const { return aggregator_; }

  // Snapshot of aggregator from start of current turn (used to compute
  // per-turn delta). Set via mark_turn_start() at the start of take_action.
  std::unordered_map<std::string, instr::ScopeStats> turn_start_snapshot() const {
    std::lock_guard<std::mutex> lk(turn_mu_);
    return turn_start_;
  }
  void mark_turn_start();

  // Decide-trace depth (controlled by LogScope). Stored on the logger so
  // each game has its own indent counter — multi-game bots don't interleave.
  int decide_depth() const { return decide_depth_; }
  void push_decide_depth() { ++decide_depth_; }
  void pop_decide_depth() {
    if (decide_depth_ > 0) --decide_depth_;
  }

 private:
  std::string bot_name_;
  int game_id_;
  std::string path_;
  std::ofstream stream_;
  std::mutex mu_;
  instr::Aggregator aggregator_;

  mutable std::mutex turn_mu_;
  std::unordered_map<std::string, instr::ScopeStats> turn_start_;

  int decide_depth_ = 0;  // bumped by LogScope ctor, decremented in dtor.
};

// Thread-local "current logger". Used by ScopedTimer (via Aggregator
// thread-local) and by LogScope / log_branch to route events without
// passing the logger down the call stack.
GameLogger* current_logger();
void set_current_logger(GameLogger* l);

// RAII helper: sets current_logger + current_aggregator for the duration
// of a scope. Compute-thread take_action() wraps itself in one of these.
class CurrentLoggerGuard {
 public:
  explicit CurrentLoggerGuard(GameLogger* l);
  ~CurrentLoggerGuard();

  CurrentLoggerGuard(const CurrentLoggerGuard&) = delete;
  CurrentLoggerGuard& operator=(const CurrentLoggerGuard&) = delete;

 private:
  GameLogger* prev_logger_;
  instr::Aggregator* prev_agg_;
};

// ISO-8601 timestamp with millisecond precision, suitable for the "ts" field
// of a JSONL record.
std::string iso_timestamp_now();

}  // namespace hanabi::logging
