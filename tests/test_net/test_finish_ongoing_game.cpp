// The game-end wire path, end to end.
//
// Regression cover for the bug where a finished game's log kept its live
// table id: the dispatcher listened for a top-level `databaseID` command
// that the server never sends. What actually arrives is
//   finishOngoingGame {"databaseID":1940573,"tableID":1136}
// and, separately, game end only ever shows up as a `gameOver` gameAction —
// there is no top-level `gameOver` command either. Both payloads below are
// copied from a real transcript (logs/bot-0.log).
//
// on_database_id writes to a hardcoded relative "logs" dir, so each test
// runs inside a temp working directory to keep the repo's logs/ untouched.
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "hanabi/net/commands.h"
#include "hanabi/net/ws_transport.h"
#include "hanabi/settings.h"

namespace fs = std::filesystem;
using namespace hanabi;
using namespace hanabi::net;
using nlohmann::json;

namespace {

BotConfig make_config() {
  BotConfig c;
  c.username = "TestBot";
  c.password = "x";
  c.host = "localhost";
  c.use_https = false;
  c.table_name = "test_table";
  c.max_num_players = 5;
  return c;
}

// Runs the body in a scratch cwd so the "logs" dir the client writes to is
// disposable. Restores the previous cwd even if a test assertion throws.
struct ScopedTempCwd {
  fs::path prev;
  fs::path dir;
  ScopedTempCwd() {
    prev = fs::current_path();
    dir = fs::temp_directory_path() /
          ("finish_game_test_" + std::to_string(::getpid()) + "_" +
           std::to_string(reinterpret_cast<uintptr_t>(this)));
    fs::create_directories(dir);
    fs::current_path(dir);
  }
  ~ScopedTempCwd() {
    std::error_code ec;
    fs::current_path(prev, ec);
    fs::remove_all(dir, ec);
  }
};

json init_payload(int tid) {
  return json{{"tableID", tid},
              {"playerNames", json::array({"alice", "bob", "TestBot"})},
              {"ourPlayerIndex", 2},
              {"options", json{{"variantName", "No Variant"}, {"numPlayers", 3}}}};
}

json game_over_action(int tid) {
  return json{{"tableID", tid},
              {"action", json{{"type", "gameOver"},
                              {"endCondition", 4},
                              {"playerIndex", 0}}}};
}

std::vector<json> read_records(const fs::path& p) {
  std::vector<json> out;
  std::ifstream in(p);
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    json rec = json::parse(line, nullptr, /*allow_exceptions=*/false);
    if (!rec.is_discarded()) out.push_back(rec);
  }
  return out;
}

}  // namespace

TEST(FinishOngoingGame, RenamesLogToDatabaseId) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);

  client.handle_message("welcome", {{"username", "TestBot"}});
  client.handle_message("init", init_payload(1136));
  ASSERT_TRUE(fs::exists("logs/TestBot-1136.log"));

  client.handle_message("gameAction", game_over_action(1136));
  client.handle_message(
      "finishOngoingGame",
      {{"databaseID", 1940573}, {"tableID", 1136}, {"sharedReplayLeader", "x"}});

  EXPECT_TRUE(fs::exists("logs/TestBot-1940573.log"))
      << "the log must land under the id a replay link uses";
  EXPECT_FALSE(fs::exists("logs/TestBot-1136.log"))
      << "the table-id log must not be left behind";

  auto records = read_records("logs/TestBot-1940573.log");
  ASSERT_FALSE(records.empty());
  for (const auto& r : records) {
    EXPECT_EQ(r.value("database_id", -1), 1940573)
        << "every record carries the database id: " << r.dump();
    EXPECT_EQ(r.value("game_id", -1), 1136)
        << "every record keeps the live table id: " << r.dump();
  }
}

TEST(FinishOngoingGame, GameOverActionEmitsPerGameTiming) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);

  client.handle_message("welcome", {{"username", "TestBot"}});
  client.handle_message("init", init_payload(42));
  client.handle_message("gameAction", game_over_action(42));

  auto records = read_records("logs/TestBot-42.log");
  int game_over = 0, per_game = 0;
  for (const auto& r : records) {
    if (r.value("event", "") == "game_over") {
      ++game_over;
      EXPECT_EQ(r.value("end_condition", -1), 4);
    }
    if (r.value("scope", "") == "per_game") ++per_game;
  }
  EXPECT_EQ(game_over, 1) << "game end arrives as a gameAction, not a command";
  EXPECT_EQ(per_game, 1) << "log_summary.py documents this aggregate";
}

TEST(FinishOngoingGame, SecondGameEndForSameTableDoesNotDoubleFire) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);

  client.handle_message("welcome", {{"username", "TestBot"}});
  client.handle_message("init", init_payload(9));
  client.handle_message("gameAction", game_over_action(9));
  // A top-level gameOver command has never been observed live, but it is
  // still dispatched — it must not re-emit the aggregate.
  client.handle_message("gameOver", {{"tableID", 9}, {"endCondition", 4}});

  auto records = read_records("logs/TestBot-9.log");
  int per_game = 0;
  for (const auto& r : records) {
    if (r.value("scope", "") == "per_game") ++per_game;
  }
  EXPECT_EQ(per_game, 1) << "finish_game must be idempotent per table";
}

TEST(FinishOngoingGame, MissingDatabaseIdIsHarmless) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);

  client.handle_message("welcome", {{"username", "TestBot"}});
  client.handle_message("init", init_payload(5));
  EXPECT_NO_THROW(client.handle_message("finishOngoingGame", {{"tableID", 5}}));
  EXPECT_TRUE(fs::exists("logs/TestBot-5.log")) << "log left alone";
}

TEST(FinishOngoingGame, TableIdReuseDoesNotLeakTheOldLogger) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);

  client.handle_message("welcome", {{"username", "TestBot"}});
  client.handle_message("init", init_payload(77));
  // Same table id again without an intervening finishOngoingGame.
  client.handle_message("init", init_payload(77));
  client.handle_message("gameAction", game_over_action(77));
  client.handle_message("finishOngoingGame",
                        {{"databaseID", 1901234}, {"tableID", 77}});

  EXPECT_TRUE(fs::exists("logs/TestBot-1901234.log"));
}
