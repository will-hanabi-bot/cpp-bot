// Decide-trace primitives for the iteration-tooling overhaul.
//
// LogScope is RAII: instantiated at the top of a major decision-branch
// function, it emits an "enter" record with the scope name + a depth index,
// and an "exit" record (with optional result fields) on destruction. The
// depth counter lives on the current GameLogger so per-game traces don't
// interleave when multiple games are active.
//
// log_branch() is a one-shot record for a mid-scope branch decision (e.g.
// "stable interpretation rejected: bad_stable=true"). log_decision() is for
// terminal action choice + candidates.
//
// All helpers no-op if no current_logger() is set (catchup, tests, no game).
#pragma once

#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace hanabi::logging {

class GameLogger;

// Emit a one-line DECIDE record. No-ops if no current logger.
void log_branch(std::string_view name, nlohmann::json info = {});

// Emit a terminal DECIDE record for the final action choice.
void log_decision(std::string_view summary, nlohmann::json info = {});

// RAII: emits an enter+exit pair around the scope's lifetime. Tracks depth
// on the current logger so nested branches indent. On destruction the exit
// record may include caller-provided result fields via set_result().
class LogScope {
 public:
  explicit LogScope(std::string_view name, nlohmann::json info = {});
  ~LogScope();

  LogScope(const LogScope&) = delete;
  LogScope& operator=(const LogScope&) = delete;

  // Mutate the exit record's payload before destruction.
  void set_result(std::string_view key, nlohmann::json value);

 private:
  std::string name_;
  int depth_at_enter_;
  nlohmann::json exit_info_;
  bool active_ = false;
};

}  // namespace hanabi::logging
