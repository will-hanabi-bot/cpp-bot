cmake_minimum_required(VERSION 3.20)
project(hanabi_bot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

include(cmake/warnings.cmake)
include(cmake/deps.cmake)

# --- Core library: basics, conventions, endgame -------------------------
find_package(OpenSSL REQUIRED)
# Boost 1.90+ provides asio/beast as header-only; use the headers target.
find_package(Boost CONFIG REQUIRED)

add_library(hanabi_core STATIC
  src/basics/action.cpp
  src/basics/card.cpp
  src/basics/clue_result.cpp
  src/basics/connection.cpp
  src/basics/decide.cpp
  src/basics/eval.cpp
  src/basics/fix.cpp
  src/basics/game.cpp
  src/basics/identity_set.cpp
  src/basics/options.cpp
  src/basics/player.cpp
  src/basics/player_elim.cpp
  src/basics/player_game.cpp
  src/basics/sarcastic.cpp
  src/basics/state.cpp
  src/basics/variant.cpp
  src/conventions/reactor/interpret_clue.cpp
  src/conventions/reactor/interpret_reaction.cpp
  src/conventions/reactor/interpret_reactive.cpp
  src/conventions/reactor/state_eval.cpp
  src/conventions/variants/brownish.cpp
  src/conventions/variants/inverted.cpp
  src/conventions/variants/pinkish.cpp
  src/conventions/variants/predicates.cpp
  src/conventions/variants/reactive_table.cpp
  src/endgame/forced_endgame.cpp
  src/endgame/helper.cpp
  src/endgame/solver.cpp
  src/endgame/winnable.cpp
  src/instrumentation/timer.cpp
  src/logging/decide_trace.cpp
  src/logging/game_logger.cpp
  src/logging/state_snapshot.cpp
  src/net/auth.cpp
  src/net/codec.cpp
  src/net/commands.cpp
  src/net/notes.cpp
  src/net/ws_transport.cpp
  src/settings.cpp
)
target_include_directories(hanabi_core PUBLIC include)
target_include_directories(hanabi_core PRIVATE ${cpp_httplib_SOURCE_DIR})
target_link_libraries(hanabi_core PUBLIC nlohmann_json::nlohmann_json spdlog::spdlog
                                          OpenSSL::SSL OpenSSL::Crypto Boost::headers)
target_compile_definitions(hanabi_core PUBLIC
  HANABI_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)
hanabi_set_warnings(hanabi_core)

# --- Tests --------------------------------------------------------------
enable_testing()
include(GoogleTest)

add_executable(hanabi_tests
  tests/test_smoke.cpp
  tests/test_harness.cpp
  tests/test_harness_smoke.cpp
  tests/test_basics/test_identity_set.cpp
  tests/test_basics/test_clue.cpp
  tests/test_basics/test_action.cpp
  tests/test_basics/test_variants.cpp
  tests/test_basics/test_state.cpp
  tests/test_basics/test_game.cpp
  tests/test_basics/test_reactive.cpp
  tests/test_basics/test_stable_ref_discard.cpp
  tests/test_basics/test_strike_preserves_ctp.cpp
  tests/test_basics/test_snapshot_roundtrip.cpp
  tests/test_reactor/test_reactive_table.cpp
  tests/test_reactor/test_ctd_revives.cpp
  tests/test_reactor/test_reactive_inferred_narrow.cpp
  tests/test_reactor/test_reversed.cpp
  tests/test_reactor/test_allplays.cpp
  tests/test_reactor/test_orange.cpp
  tests/test_reactor/test_discard_penalty.cpp
  tests/test_reactor/test_dc_target_retarget.cpp
  tests/test_reactor/test_same_hand_dupe_playables.cpp
  tests/test_endgame/test_forced_endgame.cpp
  tests/test_endgame/test_helper.cpp
  tests/test_endgame/test_replay_1874799.cpp
  tests/test_endgame/test_replay_1875252.cpp
  tests/test_endgame/test_replay_1875304.cpp
  tests/test_endgame/test_replay_1875305.cpp
  tests/test_endgame/test_replay_1876746.cpp
  tests/test_endgame/test_replay_1882268.cpp
  tests/test_endgame/test_replay_1882549.cpp
  tests/test_endgame/test_replay_1882573.cpp
  tests/test_endgame/test_replay_1883183.cpp
  tests/test_endgame/test_replay_1883510.cpp
  tests/test_endgame/test_replay_1883535.cpp
  tests/test_endgame/test_replay_1883650.cpp
  tests/test_endgame/test_replay_1883676.cpp
  tests/test_endgame/test_replay_1883728.cpp
  tests/test_endgame/test_replay_1884033.cpp
  tests/test_endgame/test_replay_1884157.cpp
  tests/test_endgame/test_replay_1884192.cpp
  tests/test_endgame/test_replay_1884348.cpp
  tests/test_endgame/test_replay_1884355.cpp
  tests/test_endgame/test_replay_1884410.cpp
  tests/test_endgame/test_replay_1884469.cpp
  tests/test_endgame/test_replay_1885375.cpp
  tests/test_endgame/test_replay_1885467.cpp
  tests/test_endgame/test_replay_1885527.cpp
  tests/test_endgame/test_replay_1885536.cpp
  tests/test_endgame/test_replay_1885550.cpp
  tests/test_endgame/test_replay_1885749.cpp
  tests/test_endgame/test_replay_1885855.cpp
  tests/test_endgame/test_replay_1885899.cpp
  tests/test_endgame/test_replay_1888951.cpp
  tests/test_endgame/test_replay_1888970.cpp
  tests/test_endgame/test_replay_1889209.cpp
  tests/test_endgame/test_replay_1889318.cpp
  tests/test_endgame/test_replay_1890204.cpp
  tests/test_endgame/test_replay_1892112.cpp
  tests/test_endgame/test_replay_1892115.cpp
  tests/test_endgame/test_replay_1892197.cpp
  tests/test_endgame/test_replay_1892259.cpp
  tests/test_endgame/test_replay_1892397.cpp
  tests/test_endgame/test_replay_1892428.cpp
  tests/test_endgame/test_replay_1892511.cpp
  tests/test_endgame/test_replay_1899044.cpp
  tests/test_endgame/test_replay_1899527.cpp
  tests/test_endgame/test_replay_1899552.cpp
  tests/test_endgame/test_replay_1899567.cpp
  tests/test_endgame/test_replay_1899623.cpp
  tests/test_endgame/test_replay_1900437.cpp
  tests/test_endgame/test_replay_1915981.cpp
  tests/test_endgame/test_replay_1916791.cpp
  tests/test_endgame/test_solver_smoke.cpp
  tests/test_net/test_codec.cpp
  tests/test_net/test_commands.cpp
)
target_include_directories(hanabi_tests PRIVATE tests)
target_link_libraries(hanabi_tests PRIVATE hanabi_core GTest::gtest_main)
hanabi_set_warnings(hanabi_tests)
gtest_discover_tests(hanabi_tests)

# --- Decision-making tests -----------------------------------------------
# Behavioural-quality tests. Failures here mean the bot made a suboptimal
# (but not necessarily incorrect) decision. They live under a separate
# executable + ctest label so they can be skipped via `ctest -LE
# decision_making` when only correctness regression matters. Per
# CLAUDE.md, decision-test failures need manual review (either bot fix or
# test change) before being silenced.
add_executable(hanabi_decision_tests
  tests/test_harness.cpp
  tests/test_decision_making/test_low_clue_count.cpp
  tests/test_decision_making/test_high_value_clue.cpp
)
target_include_directories(hanabi_decision_tests PRIVATE tests)
target_link_libraries(hanabi_decision_tests PRIVATE hanabi_core GTest::gtest_main)
hanabi_set_warnings(hanabi_decision_tests)
gtest_discover_tests(hanabi_decision_tests PROPERTIES LABELS "decision_making")

# --- Apps ----------------------------------------------------------------
add_executable(hanabi_bot apps/hanabi_bot.cpp)
target_link_libraries(hanabi_bot PRIVATE hanabi_core)
hanabi_set_warnings(hanabi_bot)

add_executable(bench_endgame apps/bench_endgame.cpp)
target_link_libraries(bench_endgame PRIVATE hanabi_core)
hanabi_set_warnings(bench_endgame)

# Standalone tool: reconstruct a Game from a per-game JSONL log and either
# re-run take_action with the current build (`--rerun`) or emit a regression
# test scaffold (`--emit-test`). The point of the logging overhaul: turn
# 30-minute "re-simulate from turn 1" loops into 1-second reruns.
add_executable(replay_log apps/replay_log.cpp)
target_link_libraries(replay_log PRIVATE hanabi_core)
hanabi_set_warnings(replay_log)
