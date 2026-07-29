// Thin portability shims for the handful of POSIX calls the bot makes.
//
// The codebase is otherwise platform-neutral (std::filesystem for paths,
// std::thread for concurrency, std::chrono for time), so this header stays
// deliberately small. Everything here is a header-only inline wrapper: on
// POSIX it forwards to the POSIX call, on Windows (MSVC CRT / mingw-w64 with
// the UCRT) it forwards to the `_s` / `_putenv_s` equivalent.
//
// mingw-w64 targeting the UCRT provides neither `setenv` nor `localtime_r`,
// which is what makes this header necessary for the MSYS2 build.
#pragma once

#include <cstdlib>
#include <ctime>

namespace hanabi::platform {

// Thread-safe localtime. Returns true on success. Note the argument order
// difference between the two platform calls: POSIX localtime_r takes
// (time, out), Windows localtime_s takes (out, time).
inline bool localtime_local(const std::time_t& t, std::tm& out) {
#if defined(_WIN32)
  return ::localtime_s(&out, &t) == 0;
#else
  return ::localtime_r(&t, &out) != nullptr;
#endif
}

// Set an environment variable only if it is not already present. Returns
// true if the variable ends up set to `value` (i.e. it was absent and the
// write succeeded).
//
// This mirrors POSIX `setenv(key, value, /*overwrite=*/0)`; `_putenv_s` has
// no overwrite flag, so the "already present" check is done here on both
// platforms rather than relying on the callee.
inline bool setenv_if_unset(const char* key, const char* value) {
  if (std::getenv(key) != nullptr) return false;
#if defined(_WIN32)
  return ::_putenv_s(key, value) == 0;
#else
  return ::setenv(key, value, /*overwrite=*/0) == 0;
#endif
}

}  // namespace hanabi::platform
