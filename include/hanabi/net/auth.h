// Port of python-bot/src/hanabi_bot/net/auth.py.
// HTTP login to hanab.live; returns the session cookie.
#pragma once

#include <stdexcept>
#include <string>

namespace hanabi::net {

class AuthError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// POST username/password to login_url, return the Set-Cookie header value.
// `version=bot` is sent so the server bypasses its version check.
std::string login(const std::string& login_url, const std::string& username,
                    const std::string& password, double timeout_seconds = 30.0);

}  // namespace hanabi::net
