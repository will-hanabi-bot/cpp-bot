#include "hanabi/net/ws_transport.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
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

// One session = one async-driven connection. All operations run on the
// io_context's single thread; queue_send + stop post to it from outside.
struct BotTransport::Session : std::enable_shared_from_this<Session> {
  asio::io_context ioc;
  std::optional<websocket::stream<beast::tcp_stream>> ws_plain;
  std::optional<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> ws_tls;
  std::optional<ssl::context> ssl_ctx;
  asio::signal_set signals;
  asio::steady_timer write_timer;
  beast::flat_buffer buffer;

  std::deque<std::string> queue;
  // Low-priority lane (notes): drained only when `queue` is empty so
  // cosmetic traffic never delays handshakes or actions.
  std::deque<std::string> low_queue;
  std::string msg_in_flight;
  bool write_in_flight = false;
  std::atomic<bool>* stop_flag;
  std::atomic<int>* pending;  // owned by BotTransport; queued-message gauge

  std::chrono::milliseconds send_interval;
  OnMessageFn on_message;
  std::atomic<bool> connection_alive{true};

  Session(std::chrono::milliseconds si, OnMessageFn on_msg, std::atomic<bool>* sf,
          std::atomic<int>* pending_gauge)
      : signals(ioc, SIGINT, SIGTERM),
        write_timer(ioc),
        stop_flag(sf),
        pending(pending_gauge),
        send_interval(si),
        on_message(std::move(on_msg)) {}

  template <typename Stream>
  void run_async(Stream& ws) {
    do_read(ws);
    setup_signal_wait(ws);
  }

  template <typename Stream>
  void setup_signal_wait(Stream& ws) {
    signals.async_wait([this, &ws](const boost::system::error_code& ec, int) {
      if (ec) return;
      std::cerr << "\nstop requested\n";
      stop_flag->store(true);
      shutdown_socket(ws);
    });
  }

  template <typename Stream>
  void do_read(Stream& ws) {
    buffer.consume(buffer.size());
    ws.async_read(buffer, [this, &ws](const boost::system::error_code& ec,
                                         std::size_t /*bytes*/) {
      if (ec) {
        connection_alive.store(false);
        if (ec != websocket::error::closed && ec != asio::error::operation_aborted) {
          std::cerr << "ws recv error: " << ec.message() << "\n";
        }
        // Cancel the remaining async ops so ioc.run() can return and the
        // outer run() loop reaches the reconnect path. Without this the
        // signal_set keeps the io_context alive forever — the bot would
        // hang silently after the server closes our socket.
        boost::system::error_code cec;
        signals.cancel(cec);
        write_timer.cancel();
        shutdown_socket(ws);
        return;
      }
      std::string frame = beast::buffers_to_string(buffer.data());
      try {
        auto decoded = decode(frame);
        on_message(decoded.command, decoded.payload);
      } catch (const std::exception& e) {
        std::cerr << "decode/handler error: " << e.what() << "\n";
      }
      do_read(ws);
    });
  }

  template <typename Stream>
  void enqueue_send(Stream& ws, std::string msg, bool low_priority) {
    asio::post(ioc, [this, &ws, m = std::move(msg), low_priority]() mutable {
      (low_priority ? low_queue : queue).push_back(std::move(m));
      pending->fetch_add(1);
      try_write_next(ws);
    });
  }

  template <typename Stream>
  void try_write_next(Stream& ws) {
    if (write_in_flight || (queue.empty() && low_queue.empty()) ||
        !connection_alive.load()) {
      return;
    }
    write_in_flight = true;
    auto& q = queue.empty() ? low_queue : queue;
    msg_in_flight = std::move(q.front());
    q.pop_front();
    pending->fetch_sub(1);
    ws.async_write(asio::buffer(msg_in_flight),
                    [this, &ws](const boost::system::error_code& ec, std::size_t) {
                      if (ec) {
                        connection_alive.store(false);
                        write_in_flight = false;
                        return;
                      }
                      write_timer.expires_after(send_interval);
                      write_timer.async_wait(
                          [this, &ws](const boost::system::error_code& tec) {
                            write_in_flight = false;
                            if (tec) return;
                            try_write_next(ws);
                          });
                    });
  }

  template <typename Stream>
  void shutdown_socket(Stream& ws) {
    boost::system::error_code ec;
    auto& sock = beast::get_lowest_layer(ws).socket();
    sock.shutdown(tcp::socket::shutdown_both, ec);
    sock.close(ec);
  }
};

BotTransport::BotTransport(std::string ws_url, std::string cookie, OnMessageFn on_message,
                              std::chrono::milliseconds send_interval, int max_retries)
    : ws_url_(std::move(ws_url)),
      cookie_(std::move(cookie)),
      on_message_(std::move(on_message)),
      send_interval_(send_interval),
      max_retries_(max_retries) {}

BotTransport::~BotTransport() { stop(); }

// --- Session state held across queue_send / stop calls --------------------
// We use a global pointer protected by a mutex to dispatch from external
// threads into the active session's io_context. Set by run_one_connection
// while a session is alive, nulled out between sessions.
namespace {
std::mutex g_active_mu;
std::shared_ptr<BotTransport::Session> g_active_session;
}  // namespace

void BotTransport::queue_send(const std::string& command, const nlohmann::json& payload,
                              bool low_priority) {
  std::string msg = encode(command, payload);
  std::shared_ptr<Session> sess;
  {
    std::lock_guard<std::mutex> lock(g_active_mu);
    sess = g_active_session;
  }
  if (!sess) return;  // not connected; drop silently
  asio::post(sess->ioc, [sess, m = std::move(msg), low_priority]() mutable {
    if (sess->ws_tls) sess->enqueue_send(*sess->ws_tls, std::move(m), low_priority);
    else if (sess->ws_plain) {
      sess->enqueue_send(*sess->ws_plain, std::move(m), low_priority);
    }
  });
}

void BotTransport::stop() {
  stop_.store(true);
  std::shared_ptr<Session> sess;
  {
    std::lock_guard<std::mutex> lock(g_active_mu);
    sess = g_active_session;
  }
  if (!sess) return;
  asio::post(sess->ioc, [sess]() {
    if (sess->ws_tls) sess->shutdown_socket(*sess->ws_tls);
    else if (sess->ws_plain) sess->shutdown_socket(*sess->ws_plain);
  });
}

bool BotTransport::run_one_connection() {
  ParsedUrl url = parse_ws_url(ws_url_);
  auto sess = std::make_shared<Session>(send_interval_, on_message_, &stop_,
                                        &pending_);

  // Synchronous connect + handshake (single-shot, no concurrency).
  tcp::resolver resolver(sess->ioc);
  auto endpoints = resolver.resolve(url.host, url.port);

  if (url.secure) {
    sess->ssl_ctx.emplace(ssl::context::tls_client);
    sess->ssl_ctx->set_default_verify_paths();
    sess->ssl_ctx->set_verify_mode(ssl::verify_peer);
    sess->ws_tls.emplace(sess->ioc, *sess->ssl_ctx);
    if (!SSL_set_tlsext_host_name(sess->ws_tls->next_layer().native_handle(),
                                    url.host.c_str())) {
      throw std::runtime_error("SSL_set_tlsext_host_name failed");
    }
    beast::get_lowest_layer(*sess->ws_tls).connect(endpoints);
    sess->ws_tls->next_layer().handshake(ssl::stream_base::client);
    sess->ws_tls->set_option(
        websocket::stream_base::decorator([this](websocket::request_type& req) {
          req.set(beast::http::field::cookie, cookie_);
        }));
    sess->ws_tls->handshake(url.host, url.target);
  } else {
    sess->ws_plain.emplace(sess->ioc);
    beast::get_lowest_layer(*sess->ws_plain).connect(endpoints);
    sess->ws_plain->set_option(
        websocket::stream_base::decorator([this](websocket::request_type& req) {
          req.set(beast::http::field::cookie, cookie_);
        }));
    sess->ws_plain->handshake(url.host, url.target);
  }

  std::cerr << "connected to " << ws_url_ << "\n";

  // Publish session pointer + start async loop.
  {
    std::lock_guard<std::mutex> lock(g_active_mu);
    g_active_session = sess;
  }
  if (sess->ws_tls) sess->run_async(*sess->ws_tls);
  else sess->run_async(*sess->ws_plain);

  // Block here; the io_context runs all async ops on this thread, so there
  // is no concurrent access to the SSL stream.
  try {
    sess->ioc.run();
  } catch (const std::exception& e) {
    std::cerr << "ws session error: " << e.what() << "\n";
  }

  // Tear down. Queued-but-unsent messages die with the session.
  {
    std::lock_guard<std::mutex> lock(g_active_mu);
    g_active_session.reset();
  }
  pending_.store(0);
  return !stop_.load();
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
    std::cerr << "WS connection dropped; reconnecting in 1s\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    attempt = 0;
  }
  return true;
}

}  // namespace hanabi::net
