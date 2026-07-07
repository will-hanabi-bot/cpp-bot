#include "hanabi/logging/game_logger.h"

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace hanabi::logging {

namespace {
thread_local GameLogger* g_current = nullptr;

std::string sanitize_for_filename(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' ||
        c == '.') {
      out.push_back(c);
    } else {
      out.push_back('_');
    }
  }
  if (out.empty()) out = "anon";
  return out;
}
}  // namespace

std::string iso_timestamp_now() {
  using namespace std::chrono;
  auto now = system_clock::now();
  auto t = system_clock::to_time_t(now);
  auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
  std::tm tm{};
  ::localtime_r(&t, &tm);
  std::ostringstream os;
  os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S") << "." << std::setw(3)
     << std::setfill('0') << ms;
  return os.str();
}

std::string GameLogger::log_path(const std::string& bot_name, int game_id,
                                 const std::string& log_dir) {
  return (std::filesystem::path(log_dir) /
          (sanitize_for_filename(bot_name) + "-" + std::to_string(game_id) +
           ".log"))
      .string();
}

GameLogger::GameLogger(std::string bot_name, int game_id,
                         const std::string& log_dir)
    : bot_name_(std::move(bot_name)), game_id_(game_id) {
  std::filesystem::create_directories(log_dir);
  path_ = log_path(bot_name_, game_id_, log_dir);
  stream_.open(path_, std::ios::app);
  if (!stream_.is_open()) {
    std::cerr << "!! GameLogger: failed to open " << path_ << " for append\n";
  }
}

bool GameLogger::rename_file(const std::string& new_path) {
  std::lock_guard<std::mutex> lk(mu_);
  if (new_path == path_) return true;
  std::error_code ec;
  if (std::filesystem::exists(new_path, ec)) {
    std::cerr << "!! GameLogger: rename target already exists: " << new_path
              << "\n";
    return false;
  }
  if (stream_.is_open()) stream_.close();
  std::filesystem::rename(path_, new_path, ec);
  if (ec) {
    std::cerr << "!! GameLogger: rename " << path_ << " -> " << new_path
              << " failed: " << ec.message() << "\n";
    stream_.open(path_, std::ios::app);
    return false;
  }
  path_ = new_path;
  stream_.open(path_, std::ios::app);
  return true;
}

GameLogger::~GameLogger() {
  if (stream_.is_open()) stream_.close();
}

void GameLogger::emit(nlohmann::json record) {
  if (!record.contains("ts")) record["ts"] = iso_timestamp_now();
  if (!record.contains("game_id")) record["game_id"] = game_id_;
  if (!record.contains("bot")) record["bot"] = bot_name_;
  std::string line = record.dump();
  std::lock_guard<std::mutex> lk(mu_);
  if (!stream_.is_open()) return;
  stream_ << line << "\n";
  stream_.flush();
}

void GameLogger::emit_lifecycle(std::string_view event, nlohmann::json extra) {
  nlohmann::json rec = std::move(extra);
  rec["ch"] = "LIFECYCLE";
  rec["event"] = std::string(event);
  emit(std::move(rec));
}

void GameLogger::mark_turn_start() {
  auto snap = aggregator_.snapshot();
  std::lock_guard<std::mutex> lk(turn_mu_);
  turn_start_ = std::move(snap);
}

GameLogger* current_logger() { return g_current; }
void set_current_logger(GameLogger* l) { g_current = l; }

CurrentLoggerGuard::CurrentLoggerGuard(GameLogger* l)
    : prev_logger_(current_logger()),
      prev_agg_(instr::current_aggregator()) {
  set_current_logger(l);
  instr::set_current_aggregator(l ? &l->aggregator() : nullptr);
}

CurrentLoggerGuard::~CurrentLoggerGuard() {
  set_current_logger(prev_logger_);
  instr::set_current_aggregator(prev_agg_);
}

}  // namespace hanabi::logging
