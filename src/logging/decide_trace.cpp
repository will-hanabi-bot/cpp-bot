#include "hanabi/logging/decide_trace.h"

#include "hanabi/logging/game_logger.h"

namespace hanabi::logging {

namespace {
nlohmann::json make_decide_record(std::string_view kind, int depth,
                                    std::string_view name, nlohmann::json info) {
  nlohmann::json rec = std::move(info);
  rec["ch"] = "DECIDE";
  rec["kind"] = std::string(kind);
  rec["depth"] = depth;
  rec["name"] = std::string(name);
  return rec;
}
}  // namespace

void log_branch(std::string_view name, nlohmann::json info) {
  GameLogger* gl = current_logger();
  if (!gl) return;
  gl->emit(make_decide_record("branch", gl->decide_depth(), name, std::move(info)));
}

void log_decision(std::string_view summary, nlohmann::json info) {
  GameLogger* gl = current_logger();
  if (!gl) return;
  gl->emit(make_decide_record("decision", gl->decide_depth(), summary,
                                std::move(info)));
}

LogScope::LogScope(std::string_view name, nlohmann::json info)
    : name_(name) {
  GameLogger* gl = current_logger();
  if (!gl) {
    depth_at_enter_ = 0;
    return;
  }
  active_ = true;
  depth_at_enter_ = gl->decide_depth();
  gl->emit(make_decide_record("enter", depth_at_enter_, name_, std::move(info)));
  gl->push_decide_depth();
}

LogScope::~LogScope() {
  if (!active_) return;
  GameLogger* gl = current_logger();
  if (!gl) return;
  gl->pop_decide_depth();
  gl->emit(make_decide_record("exit", depth_at_enter_, name_,
                                std::move(exit_info_)));
}

void LogScope::set_result(std::string_view key, nlohmann::json value) {
  exit_info_[std::string(key)] = std::move(value);
}

}  // namespace hanabi::logging
