// Port of python-bot/src/hanabi_bot/net/ws_transport.py.
// WebSocket transport using boost::beast.
//
// Threading model (faithful port of the Python async loop):
// - A receive thread reads frames, decodes, and dispatches to on_message.
// - A send thread drains an outbound queue with 500 ms spacing.
// - The main thread calls run() which blocks until stop() is called,
//   handling reconnect with exponential backoff.
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

namespace hanabi::net {

using OnMessageFn = std::function<void(const std::string& command, const nlohmann::json& payload)>;

class BotTransport {
 public:
  BotTransport(std::string ws_url, std::string cookie, OnMessageFn on_message,
                std::chrono::milliseconds send_interval = std::chrono::milliseconds(500),
                int max_retries = 5);
  ~BotTransport();

  // Enqueue an outbound message. Thread-safe.
  void queue_send(const std::string& command,
                    const nlohmann::json& payload = nlohmann::json::value_t::null);

  // Request stop. Causes run() to return once the current operation completes.
  void stop();

  // Run the transport with reconnect/backoff. Blocks until stop() or
  // max_retries exhausted. Returns true on clean stop; false on retry exhaustion.
  bool run();

 private:
  std::string ws_url_;
  std::string cookie_;
  OnMessageFn on_message_;
  std::chrono::milliseconds send_interval_;
  int max_retries_;

  std::mutex queue_mu_;
  std::condition_variable queue_cv_;
  std::deque<std::string> queue_;
  std::atomic<bool> stop_{false};

  bool run_one_connection();
};

}  // namespace hanabi::net
