// Entry point for the hanabi bot.
// Parses argv, loads .env, logs into hanab.live, opens a WebSocket, and
// dispatches incoming messages via net::BotClient. Signal handling lives
// inside BotTransport (boost::asio::signal_set), which can safely shut down
// the live socket to unblock the receive loop on Ctrl+C.
//
// Logging: stderr is tee'd to logs/bot-<index>.log (append mode) so each run
// leaves a permanent record. stdout is also tee'd (in case any third-party
// library writes there). Terminal output is unchanged.
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <streambuf>
#include <string>
#include <unistd.h>

#include "hanabi/net/auth.h"
#include "hanabi/net/commands.h"
#include "hanabi/net/ws_transport.h"
#include "hanabi/settings.h"

namespace {

// A streambuf that writes every character to two underlying buffers
// (e.g. terminal stderr + a log file). Thread-safe via an internal mutex
// so the sender / receive threads don't interleave bytes mid-line.
class TeeBuf : public std::streambuf {
 public:
  TeeBuf(std::streambuf* primary, std::streambuf* secondary)
      : primary_(primary), secondary_(secondary) {}

 protected:
  int overflow(int c) override {
    if (c == EOF) return !EOF;
    std::lock_guard<std::mutex> lock(mu_);
    int r1 = primary_->sputc(static_cast<char>(c));
    int r2 = secondary_->sputc(static_cast<char>(c));
    return (r1 == EOF || r2 == EOF) ? EOF : c;
  }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    std::lock_guard<std::mutex> lock(mu_);
    std::streamsize n1 = primary_->sputn(s, n);
    std::streamsize n2 = secondary_->sputn(s, n);
    return std::min(n1, n2);
  }
  int sync() override {
    std::lock_guard<std::mutex> lock(mu_);
    int r1 = primary_->pubsync();
    int r2 = secondary_->pubsync();
    return (r1 == 0 && r2 == 0) ? 0 : -1;
  }

 private:
  std::streambuf* primary_;
  std::streambuf* secondary_;
  std::mutex mu_;
};

std::string timestamp_now() {
  auto t = std::time(nullptr);
  std::tm tm{};
  ::localtime_r(&t, &tm);
  std::ostringstream os;
  os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
  return os.str();
}

}  // namespace

int main(int argc, char** argv) {
  // --- Set up tee logging before anything else can print -----------------
  std::filesystem::create_directories("logs");

  // Parse args once early just to pick the log file name; full BotConfig
  // resolution happens after .env is loaded.
  std::string log_index = "0";
  for (int i = 1; i < argc; ++i) {
    std::string a(argv[i]);
    auto eq = a.find('=');
    if (eq != std::string::npos && a.substr(0, eq) == "index") {
      log_index = a.substr(eq + 1);
      break;
    }
  }
  std::string log_path = "logs/bot-" + log_index + ".log";
  std::ofstream log_file(log_path, std::ios::app);

  TeeBuf tee_err(std::cerr.rdbuf(), log_file.rdbuf());
  TeeBuf tee_out(std::cout.rdbuf(), log_file.rdbuf());
  std::streambuf* orig_cerr = std::cerr.rdbuf(&tee_err);
  std::streambuf* orig_cout = std::cout.rdbuf(&tee_out);

  std::cerr << "\n=== bot start " << timestamp_now() << " pid=" << ::getpid()
            << " index=" << log_index << " ===\n";

  int rc = 1;
  try {
    hanabi::load_dotenv(".env");
    auto args = hanabi::parse_argv(argc, argv);
    auto config = hanabi::BotConfig::from_env(args);

    std::cerr << "logging in as " << config.username << " to " << config.host << "...\n";
    std::string cookie = hanabi::net::login(config.login_url(), config.username,
                                              config.password);

    hanabi::net::BotClient* client_ptr = nullptr;
    hanabi::net::BotTransport transport(
        config.ws_url(), cookie,
        [&client_ptr](const std::string& command, const nlohmann::json& payload) {
          std::string body = payload.dump();
          if (body.size() > 200) body = body.substr(0, 200) + "...";
          std::cerr << "<- " << command << " " << body << "\n";
          if (client_ptr) client_ptr->handle_message(command, payload);
        });
    hanabi::net::BotClient client(transport, config);
    client_ptr = &client;

    bool clean = transport.run();
    rc = clean ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << "fatal: " << e.what() << "\n";
    rc = 1;
  }

  std::cerr << "=== bot exit " << timestamp_now() << " rc=" << rc << " ===\n";
  // Restore so the streambufs we own don't outlive themselves.
  std::cerr.rdbuf(orig_cerr);
  std::cout.rdbuf(orig_cout);
  return rc;
}
