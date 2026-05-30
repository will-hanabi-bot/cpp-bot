#include "hanabi/net/codec.h"

#include <stdexcept>

namespace hanabi::net {

DecodedMessage decode(const std::string& message) {
  auto sep = message.find(' ');
  if (sep == std::string::npos) {
    return {message, nlohmann::json::object()};
  }
  std::string command = message.substr(0, sep);
  std::string body = message.substr(sep + 1);
  try {
    return {std::move(command), nlohmann::json::parse(body)};
  } catch (const nlohmann::json::parse_error& e) {
    throw std::invalid_argument("Invalid JSON body for command " + command + ": " + e.what());
  }
}

std::string encode(const std::string& command, const nlohmann::json& payload) {
  if (payload.is_null()) return command;
  if (payload.is_object() && payload.empty()) return command;
  if (payload.is_array() && payload.empty()) return command;
  return command + " " + payload.dump();
}

}  // namespace hanabi::net
