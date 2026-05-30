#include "hanabi/net/auth.h"

#include <iostream>
#include <stdexcept>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

namespace hanabi::net {

namespace {

std::string url_encode(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char c : s) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~') {
      out += static_cast<char>(c);
    } else {
      static const char hex[] = "0123456789ABCDEF";
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 0xF];
    }
  }
  return out;
}

// Parse "scheme://host[:port]/path" into pieces. Returns true if parsed.
bool parse_url(const std::string& url, std::string& scheme, std::string& host,
                int& port, std::string& path) {
  auto scheme_end = url.find("://");
  if (scheme_end == std::string::npos) return false;
  scheme = url.substr(0, scheme_end);
  auto rest = url.substr(scheme_end + 3);
  auto path_start = rest.find('/');
  std::string host_port = (path_start == std::string::npos) ? rest : rest.substr(0, path_start);
  path = (path_start == std::string::npos) ? "/" : rest.substr(path_start);
  auto colon = host_port.find(':');
  if (colon != std::string::npos) {
    host = host_port.substr(0, colon);
    port = std::stoi(host_port.substr(colon + 1));
  } else {
    host = host_port;
    port = (scheme == "https") ? 443 : 80;
  }
  return true;
}

}  // namespace

std::string login(const std::string& login_url, const std::string& username,
                    const std::string& password, double timeout_seconds) {
  std::cerr << "Authenticating to \"" << login_url << "\" with username = \""
            << username << "\".\n";

  std::string scheme, host, path;
  int port;
  if (!parse_url(login_url, scheme, host, port, path)) {
    throw AuthError("Invalid login URL: " + login_url);
  }

  std::string body = "username=" + url_encode(username) +
                     "&password=" + url_encode(password) +
                     "&version=bot";
  const auto timeout_secs = static_cast<int>(timeout_seconds);

  if (scheme == "https") {
    httplib::SSLClient cli(host, port);
    cli.set_connection_timeout(timeout_secs, 0);
    cli.set_read_timeout(timeout_secs, 0);
    cli.set_follow_location(false);
    auto res = cli.Post(path.c_str(), body, "application/x-www-form-urlencoded");
    if (!res) {
      throw AuthError("HTTP request failed (TLS error / no response)");
    }
    if (res->status != 200) {
      throw AuthError("Authentication failed: HTTP " + std::to_string(res->status));
    }
    auto cookie = res->get_header_value("Set-Cookie");
    if (cookie.empty()) {
      throw AuthError("No Set-Cookie header in login response");
    }
    return cookie;
  }

  httplib::Client cli(host, port);
  cli.set_connection_timeout(timeout_secs, 0);
  cli.set_read_timeout(timeout_secs, 0);
  cli.set_follow_location(false);
  auto res = cli.Post(path.c_str(), body, "application/x-www-form-urlencoded");
  if (!res) {
    throw AuthError("HTTP request failed (no response)");
  }
  if (res->status != 200) {
    throw AuthError("Authentication failed: HTTP " + std::to_string(res->status));
  }
  auto cookie = res->get_header_value("Set-Cookie");
  if (cookie.empty()) {
    throw AuthError("No Set-Cookie header in login response");
  }
  return cookie;
}

}  // namespace hanabi::net
