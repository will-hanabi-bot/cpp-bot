// Port of python-bot/src/hanabi_bot/net/ws_transport.py.
// WebSocket transport using boost::beast in async mode.
//
// All SSL stream operations run on a single io_context (a single thread),
// which is the only thread-safe way to use SSL with boost::asio - concurrent
// SSL_read/SSL_write from different threads corrupts the cipher state machine
// and produces "bad record mac" errors.
//
// Architecture:
// - One io_context (ioc_) runs on the thread that calls run().
// - async_read continuously pumps frames into on_message_.
// - queue_send posts a message to the io_context's queue; a chained
//   async_write drains it with 500 ms spacing via an asio::steady_timer.
// - asio::signal_set on the same io_context handles SIGINT / SIGTERM.
#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <string>

#include <nlohmann/json.hpp>

namespace hanabi::net {

using OnMessageFn = std::function<void(const std::string& command,
                                          const nlohmann::json& payload)>;

class BotTransport {
 public:
  BotTransport(std::string ws_url, std::string cookie, OnMessageFn on_message,
                std::chrono::milliseconds send_interval = std::chrono::milliseconds(500),
                int max_retries = 5);
  ~BotTransport();

  // Enqueue an outbound message. Thread-safe (posts to the io_context).
  // The write loop drains one message per `send_interval` (default 500 ms,
  // hanab.live rate-limit pacing), so every queued message delays the ones
  // behind it. `low_priority` messages (notes) go to a second lane that is
  // drained only when the main lane is empty — cosmetic traffic must never
  // delay handshakes ("loaded") or actions (replay 2580: a 16-message note
  // burst at load stalled game start by ~8 s).
  void queue_send(const std::string& command,
                    const nlohmann::json& payload = nlohmann::json::value_t::null,
                    bool low_priority = false);

  // Messages currently waiting in the send lanes (excludes the one in
  // flight). Diagnostic for the per-game log's latency records.
  int pending_sends() const { return pending_.load(); }

  // Request stop. Thread-safe; causes run() to return cleanly.
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
  std::atomic<bool> stop_{false};
  std::atomic<int> pending_{0};

  // The session impl lives in ws_transport.cpp - we use the pimpl idiom so
  // we don't have to drag boost/asio headers into a public header.
 public:
  struct Session;  // public so queue_send / stop helpers in ws_transport.cpp
                   // can name it through a shared_ptr.

 private:
  bool run_one_connection();
};

}  // namespace hanabi::net
