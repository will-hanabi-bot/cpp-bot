#include "hanabi/net/ws_transport.h"

#include <iostream>
#include <stdexcept>
#include <thread>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include "hanabi/net/codec.h"

namespace hanabi::net {

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
using tcp = asio::ip::tcp;

namespace {

struct ParsedUrl {
  bool secure;
  std::string host;
  std::string port;
  std::string target;
};

ParsedUrl parse_ws_url(const std::string& url) {
  ParsedUrl p;
  std::string rest;
  if (url.rfind("wss://", 0) == 0) {
    p.secure = true;
    rest = url.substr(6);
  } else if (url.rfind("ws://", 0) == 0) {
    p.secure = false;
    rest = url.substr(5);
  } else {
    throw std::invalid_argument("URL must start with ws:// or wss://");
  }
  auto slash = rest.find('/');
  std::string host_port = (slash == std::string::npos) ? rest : rest.substr(0, slash);
  p.target = (slash == std::string::npos) ? "/" : rest.substr(slash);
  auto colon = host_port.find(':');
  if (colon != std::string::npos) {
    p.host = host_port.substr(0, colon);
    p.port = host_port.substr(colon + 1);
  } else {
    p.host = host_port;
    p.port = p.secure ? "443" : "80";
  }
  return p;
}

}  // namespace

BotTransport::BotTransport(std::string ws_url, std::string cookie, OnMessageFn on_message,
                              std::chrono::milliseconds send_interval, int max_retries)
    : ws_url_(std::move(ws_url)),
      cookie_(std::move(cookie)),
      on_message_(std::move(on_message)),
      send_interval_(send_interval),
      max_retries_(max_retries) {}

BotTransport::~BotTransport() { stop(); }

void BotTransport::queue_send(const std::string& command, const nlohmann::json& payload) {
  std::string msg = encode(command, payload);
  {
    std::lock_guard<std::mutex> lock(queue_mu_);
    queue_.push_back(std::move(msg));
  }
  queue_cv_.notify_one();
}

void BotTransport::stop() {
  stop_.store(true);
  queue_cv_.notify_all();
}

bool BotTransport::run_one_connection() {
  ParsedUrl url = parse_ws_url(ws_url_);
  asio::io_context ioc;
  tcp::resolver resolver(ioc);
  auto endpoints = resolver.resolve(url.host, url.port);

  std::optional<websocket::stream<beast::tcp_stream>> ws_plain;
  std::optional<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> ws_tls;
  std::optional<ssl::context> ssl_ctx;

  if (url.secure) {
    ssl_ctx.emplace(ssl::context::tls_client);
    ssl_ctx->set_default_verify_paths();
    ssl_ctx->set_verify_mode(ssl::verify_peer);
    ws_tls.emplace(ioc, *ssl_ctx);
    if (!SSL_set_tlsext_host_name(ws_tls->next_layer().native_handle(), url.host.c_str())) {
      throw std::runtime_error("SSL_set_tlsext_host_name failed");
    }
    beast::get_lowest_layer(*ws_tls).connect(endpoints);
    ws_tls->next_layer().handshake(ssl::stream_base::client);
    ws_tls->set_option(websocket::stream_base::decorator(
        [this](websocket::request_type& req) { req.set(beast::http::field::cookie, cookie_); }));
    ws_tls->handshake(url.host, url.target);
  } else {
    ws_plain.emplace(ioc);
    beast::get_lowest_layer(*ws_plain).connect(endpoints);
    ws_plain->set_option(websocket::stream_base::decorator(
        [this](websocket::request_type& req) { req.set(beast::http::field::cookie, cookie_); }));
    ws_plain->handshake(url.host, url.target);
  }

  std::cerr << "connected to " << ws_url_ << "\n";

  std::atomic<bool> conn_stopped{false};

  // shutdown_socket() unblocks a pending blocking read by tearing down the
  // underlying TCP socket. It's safe to call from another thread (asio
  // socket ops are documented thread-safe for this purpose). Called from
  // the signal-handling thread below; also from the receive-loop cleanup
  // path when the receive loop notices stop_ on its own.
  auto shutdown_socket = [&]() {
    boost::system::error_code ec;
    if (ws_tls) {
      auto& sock = beast::get_lowest_layer(*ws_tls).socket();
      sock.shutdown(tcp::socket::shutdown_both, ec);
      sock.close(ec);
    } else {
      auto& sock = beast::get_lowest_layer(*ws_plain).socket();
      sock.shutdown(tcp::socket::shutdown_both, ec);
      sock.close(ec);
    }
  };

  // Background io_context hosts an asio::signal_set that converts SIGINT /
  // SIGTERM into a socket shutdown (which makes the blocking ws read return).
  // boost::asio::signal_set is the correct way to handle signals in an asio
  // program - the signal is delivered via the io_context, not via the
  // (async-signal-unsafe) C-handler path, so it's safe to call non-trivial
  // socket operations from the handler.
  asio::io_context signal_ioc;
  asio::signal_set signals(signal_ioc, SIGINT, SIGTERM);
  signals.async_wait([&](const boost::system::error_code& ec, int /*sig*/) {
    if (ec) return;  // canceled on clean shutdown
    std::cerr << "\nstop requested\n";
    stop_.store(true);
    queue_cv_.notify_all();
    shutdown_socket();
  });
  std::thread signal_thread([&signal_ioc]() {
    try {
      signal_ioc.run();
    } catch (const std::exception& e) {
      std::cerr << "signal thread: " << e.what() << "\n";
    }
  });

  auto write_one = [&](const std::string& msg) {
    if (ws_tls) ws_tls->write(asio::buffer(msg));
    else ws_plain->write(asio::buffer(msg));
  };
  auto close_socket = [&]() {
    beast::error_code ec;
    if (ws_tls) ws_tls->close(websocket::close_code::normal, ec);
    else ws_plain->close(websocket::close_code::normal, ec);
  };

  // Sender thread.
  std::thread sender([&]() {
    while (!stop_.load() && !conn_stopped.load()) {
      std::string msg;
      {
        std::unique_lock<std::mutex> lock(queue_mu_);
        queue_cv_.wait(lock, [&]() {
          return !queue_.empty() || stop_.load() || conn_stopped.load();
        });
        if (stop_.load() || conn_stopped.load()) break;
        msg = std::move(queue_.front());
        queue_.pop_front();
      }
      try {
        write_one(msg);
      } catch (const std::exception&) {
        conn_stopped.store(true);
        break;
      }
      std::this_thread::sleep_for(send_interval_);
    }
  });

  // Receive loop on the main thread.
  beast::flat_buffer buffer;
  try {
    while (!stop_.load()) {
      buffer.consume(buffer.size());
      if (ws_tls) ws_tls->read(buffer);
      else ws_plain->read(buffer);
      std::string frame = beast::buffers_to_string(buffer.data());
      try {
        auto decoded = decode(frame);
        on_message_(decoded.command, decoded.payload);
      } catch (const std::exception& e) {
        std::cerr << "decode error: " << e.what() << "\n";
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "ws recv error: " << e.what() << "\n";
  }

  conn_stopped.store(true);
  queue_cv_.notify_all();
  if (sender.joinable()) sender.join();
  close_socket();

  // Tear down the signal handler: cancel the pending wait (so signal_ioc.run()
  // returns), stop the io_context, and join the thread.
  {
    boost::system::error_code ec;
    signals.cancel(ec);
  }
  signal_ioc.stop();
  if (signal_thread.joinable()) signal_thread.join();

  return !stop_.load();  // return true if the disconnect was unexpected
}

bool BotTransport::run() {
  int attempt = 0;
  while (!stop_.load()) {
    bool unexpected_disconnect = false;
    try {
      unexpected_disconnect = run_one_connection();
    } catch (const std::exception& e) {
      if (attempt >= max_retries_) {
        std::cerr << "WS giving up after " << attempt << " retries: " << e.what() << "\n";
        return false;
      }
      ++attempt;
      int delay = 1 << attempt;
      std::cerr << "WS connection lost (attempt " << attempt << "/" << max_retries_
                << "): " << e.what() << "; retrying in " << delay << "s\n";
      std::this_thread::sleep_for(std::chrono::seconds(delay));
      continue;
    }
    if (!unexpected_disconnect) break;
    if (stop_.load()) break;
    // run_one_connection returned (didn't throw) with "unexpected disconnect"
    // — the receive loop saw the connection drop and exited cleanly. Always
    // wait at least 1 second before reconnecting so we don't hammer the
    // server, and don't race with whatever just kicked us off.
    std::cerr << "WS connection dropped; reconnecting in 1s\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    attempt = 0;  // clean (non-exception) close resets backoff
  }
  return true;
}

}  // namespace hanabi::net
