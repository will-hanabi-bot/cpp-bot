// Port of python-bot/src/hanabi_bot/settings.py.
#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace hanabi {

struct BotConfig {
  std::string username;
  std::string password;
  std::string host = "hanab.live";
  int index = 0;
  std::optional<std::string> bot_to_join;  // nullopt = idle; "create" = make table; else username
  std::string convention = "Reactor1";
  std::string table_name = "bots";
  int max_num_players = 5;
  bool disconnect_on_game_end = false;
  bool use_https = true;

  std::string login_url() const {
    return std::string(use_https ? "https://" : "http://") + host + "/login";
  }
  std::string ws_url() const {
    return std::string(use_https ? "wss://" : "ws://") + host + "/ws";
  }

  static BotConfig from_env(const std::unordered_map<std::string, std::string>& args);
};

// Parse key=value style argv (matches the Scala/Python bots' format).
std::unordered_map<std::string, std::string> parse_argv(int argc, char** argv);

// Convenience: load .env at the given path into the process environment if
// the corresponding key isn't already set. Lines starting with '#' or blank
// lines are skipped. Returns false if the file couldn't be opened.
bool load_dotenv(const std::string& path);

}  // namespace hanabi
