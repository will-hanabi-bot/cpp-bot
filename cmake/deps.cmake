include(FetchContent)

set(FETCHCONTENT_QUIET ON)

# nlohmann/json
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)

# spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.3
  GIT_SHALLOW TRUE
)

# GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
  GIT_SHALLOW TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# cpp-httplib for HTTP/HTTPS login (Phase 6).
FetchContent_Declare(
  cpp_httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.7
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(nlohmann_json spdlog googletest cpp_httplib)
