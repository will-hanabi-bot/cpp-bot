// Port of python-bot/src/hanabi_bot/net/codec.py.
// hanab.live wire format: "COMMAND_NAME JSON_BODY" (single space separator).
#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace hanabi::net {

struct DecodedMessage {
  std::string command;
  nlohmann::json payload;  // dict or array depending on command
};

// Parse 'COMMAND_NAME {...json...}' or 'COMMAND_NAME [...]'.
// Returns command + {} if no body. Throws std::invalid_argument on bad JSON.
DecodedMessage decode(const std::string& message);

// Encode (command, payload) to wire format. If payload is null or empty, just the command.
std::string encode(const std::string& command,
                    const nlohmann::json& payload = nlohmann::json::value_t::null);

}  // namespace hanabi::net
