// Entry point for the hanabi bot.
// Phase 6 scope: parses argv, loads .env, logs into hanab.live, opens a
// WebSocket, and dispatches incoming messages. Convention logic (Phase 4
// tail) and full chat-command dispatch (Phase 6 tail) are stubbed - this
// binary verifies the wire connection works end-to-end.
#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

#include "hanabi/net/auth.h"
#include "hanabi/net/ws_transport.h"
#include "hanabi/settings.h"

namespace {

hanabi::net::BotTransport* g_transport = nullptr;

void handle_signal(int /*sig*/) {
  std::cerr << "\nstop requested\n";
  if (g_transport) g_transport->stop();
}

}  // namespace

int main(int argc, char** argv) {
  try {
    hanabi::load_dotenv(".env");
    auto args = hanabi::parse_argv(argc, argv);
    auto config = hanabi::BotConfig::from_env(args);

    std::cerr << "logging in as " << config.username << " to " << config.host << "...\n";
    std::string cookie = hanabi::net::login(config.login_url(), config.username,
                                              config.password);

    hanabi::net::BotTransport transport(
        config.ws_url(), cookie,
        [](const std::string& command, const nlohmann::json& payload) {
          // Minimal dispatch: log the command + first 200 chars of the payload.
          std::string body = payload.dump();
          if (body.size() > 200) body = body.substr(0, 200) + "...";
          std::cerr << "<- " << command << " " << body << "\n";
        });
    g_transport = &transport;
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    bool clean = transport.run();
    return clean ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << "fatal: " << e.what() << "\n";
    return 1;
  }
}
