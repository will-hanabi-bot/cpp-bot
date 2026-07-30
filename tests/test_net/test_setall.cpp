// /setall and /rlocks chat handling.
//
// The load-bearing property: `/setall` is a shared command name. Other bot
// families in the same table room answer it with their own grammar (a real
// transcript shows will-bot2 responding to "/setall 3"), so anything we do
// not recognise must be ignored SILENTLY — no reply, no state change.
#include <gtest/gtest.h>

#include <filesystem>

#include <nlohmann/json.hpp>

#include "hanabi/basics/convention.h"
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

// on_init writes into a relative "logs" dir; keep the repo's logs/ clean.
struct ScopedTempCwd {
  fs::path prev, dir;
  ScopedTempCwd() {
    prev = fs::current_path();
    dir = fs::temp_directory_path() /
          ("setall_test_" + std::to_string(::getpid()) + "_" +
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

json chat(const std::string& msg) {
  return json{{"msg", msg}, {"recipient", ""}, {"room", "table7"},
              {"who", "yagami_black"}};
}

json init_payload(int tid, int num_players) {
  json names = json::array({"alice", "bob", "TestBot"});
  if (num_players == 4) names.push_back("dave");
  return json{{"tableID", tid},
              {"playerNames", names},
              {"ourPlayerIndex", 2},
              {"options", json{{"variantName", "No Variant"},
                               {"numPlayers", num_players}}}};
}

}  // namespace

TEST(SetAll, ForeignGrammarIsIgnoredSilently) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  // will-bot2's grammar — must not throw, must not change our convention.
  EXPECT_NO_THROW(client.handle_message("chat", chat("/setall 3")));
  EXPECT_NO_THROW(client.handle_message("chat", chat("/setall")));
  EXPECT_NO_THROW(client.handle_message("chat", chat("/setall hgroup")));

  client.handle_message("init", init_payload(7, 3));
  auto rec = client.debug_game_snapshot(7);
  ASSERT_TRUE(rec.has_value());
  EXPECT_EQ(rec->convention, Convention::REACTOR0)
      << "the default must survive a foreign /setall";
}

TEST(SetAll, SwitchesConventionForNewGamesOnly) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(1, 3));
  ASSERT_EQ(client.debug_game_snapshot(1)->convention, Convention::REACTOR0);

  client.handle_message("chat", chat("/setall reactor"));
  // The running game keeps its convention — a mid-game switch would desync.
  EXPECT_EQ(client.debug_game_snapshot(1)->convention, Convention::REACTOR0);

  client.handle_message("init", init_payload(2, 3));
  EXPECT_EQ(client.debug_game_snapshot(2)->convention, Convention::REACTOR)
      << "the switch lands at the next game";

  client.handle_message("chat", chat("/setall reactor0"));
  client.handle_message("init", init_payload(3, 3));
  EXPECT_EQ(client.debug_game_snapshot(3)->convention, Convention::REACTOR0);
}

TEST(SetAll, FourPlayerGameFallsBackToReactor) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(9, 4));
  EXPECT_EQ(client.debug_game_snapshot(9)->convention, Convention::REACTOR)
      << "reactor0 is a 3-player convention for now";
}

TEST(SetAll, RlocksOverridesTheVariantDefault) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(4, 3));
  EXPECT_TRUE(client.debug_game_snapshot(4)->allow_reactive_locks)
      << "No Variant 3p is below the 1.42 threshold";

  // /rlocks retro-applies to running games (unlike /setall).
  client.handle_message("chat", chat("/rlocks off"));
  EXPECT_FALSE(client.debug_game_snapshot(4)->allow_reactive_locks);

  client.handle_message("init", init_payload(5, 3));
  EXPECT_FALSE(client.debug_game_snapshot(5)->allow_reactive_locks)
      << "and the override persists into new games";
}
