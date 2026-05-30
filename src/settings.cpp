#include "hanabi/settings.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <stdexcept>

namespace hanabi {

namespace {

std::string getenv_str(const std::string& key) {
  const char* v = std::getenv(key.c_str());
  return v ? std::string(v) : std::string();
}

std::string trim(std::string s) {
  auto is_ws = [](unsigned char c) { return std::isspace(c); };
  while (!s.empty() && is_ws(s.front())) s.erase(s.begin());
  while (!s.empty() && is_ws(s.back())) s.pop_back();
  return s;
}

bool starts_with(const std::string& s, const std::string& prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool to_bool(const std::string& s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                  [](unsigned char c) { return std::tolower(c); });
  return lower == "true";
}

}  // namespace

BotConfig BotConfig::from_env(const std::unordered_map<std::string, std::string>& args) {
  auto get_arg = [&](const std::string& k) -> std::string {
    auto it = args.find(k);
    return it != args.end() ? it->second : std::string();
  };

  int index = 0;
  if (auto it = args.find("index"); it != args.end()) index = std::stoi(it->second);

  BotConfig c;
  c.index = index;

  std::string env_user = getenv_str("HANABI_USERNAME" + std::to_string(index));
  std::string env_pass = getenv_str("HANABI_PASSWORD" + std::to_string(index));
  c.username = !env_user.empty() ? env_user : get_arg("username");
  c.password = !env_pass.empty() ? env_pass : get_arg("password");
  if (c.username.empty()) {
    throw std::runtime_error("Missing HANABI_USERNAME" + std::to_string(index) + " env variable");
  }
  if (c.password.empty()) {
    throw std::runtime_error("Missing HANABI_PASSWORD" + std::to_string(index) + " env variable");
  }

  std::string env_host = getenv_str("HANABI_HOST");
  if (!env_host.empty()) {
    c.host = env_host;
  } else if (auto it = args.find("host"); it != args.end()) {
    c.host = it->second;
  }

  c.use_https = !(starts_with(c.host, "localhost") || starts_with(c.host, "127.") ||
                  starts_with(c.host, "0.0.0.0"));

  if (auto it = args.find("bot_to_join"); it != args.end()) c.bot_to_join = it->second;
  if (auto it = args.find("convention"); it != args.end()) c.convention = it->second;
  if (auto it = args.find("table"); it != args.end()) c.table_name = it->second;
  if (auto it = args.find("max_players"); it != args.end()) c.max_num_players = std::stoi(it->second);
  if (auto it = args.find("disconnect_on_game_end"); it != args.end()) {
    c.disconnect_on_game_end = to_bool(it->second);
  }

  return c;
}

std::unordered_map<std::string, std::string> parse_argv(int argc, char** argv) {
  std::unordered_map<std::string, std::string> out;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    auto eq = arg.find('=');
    if (eq == std::string::npos) {
      throw std::invalid_argument("Invalid argument '" + arg + "' (expected key=value)");
    }
    out[arg.substr(0, eq)] = arg.substr(eq + 1);
  }
  return out;
}

bool load_dotenv(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) return false;
  std::string line;
  while (std::getline(f, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#') continue;
    auto eq = line.find('=');
    if (eq == std::string::npos) continue;
    std::string key = trim(line.substr(0, eq));
    std::string value = trim(line.substr(eq + 1));
    // Strip surrounding quotes.
    if (value.size() >= 2 && (value.front() == '"' || value.front() == '\'') &&
        value.front() == value.back()) {
      value = value.substr(1, value.size() - 2);
    }
    // Only set if not already in env.
    if (std::getenv(key.c_str()) == nullptr) {
      ::setenv(key.c_str(), value.c_str(), /*overwrite=*/0);
    }
  }
  return true;
}

}  // namespace hanabi
