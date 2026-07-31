// /allplays is a reactor-only switch.
//
// It promotes reactor's colour reactives to play+play. reactor0 fixes parity
// by clue kind instead (colour = one play, rank = even), so the flag has no
// meaning there — and a set flag is worse than meaningless: reaction
// resolution would read the same clue as the opposite of what clue-time
// selection agreed. The flag must therefore never land on a reactor0 game,
// whether the game is created while the mode is on or the mode is flipped on
// while the game is running.
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
          ("allplays_test_" + std::to_string(::getpid()) + "_" +
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

TEST(AllPlaysScope, ModeOnDoesNotReachANewReactor0Game) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("chat", chat("/allplays on"));

  // 3p defaults to reactor0 — the mode must not carry into it.
  client.handle_message("init", init_payload(1, 3));
  auto r0 = client.debug_game_snapshot(1);
  ASSERT_TRUE(r0.has_value());
  ASSERT_EQ(r0->convention, Convention::REACTOR0);
  EXPECT_FALSE(r0->all_plays)
      << "/allplays is a reactor concept; a reactor0 game must never carry it";

  // 4p falls back to reactor, where the mode is meaningful and must apply.
  client.handle_message("init", init_payload(2, 4));
  auto r = client.debug_game_snapshot(2);
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->convention, Convention::REACTOR);
  EXPECT_TRUE(r->all_plays)
      << "the mode must still reach reactor games — this is not a global "
         "disable";
}

TEST(AllPlaysScope, RetroApplySkipsRunningReactor0Games) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(3, 3));   // reactor0
  client.handle_message("init", init_payload(4, 4));   // reactor
  ASSERT_EQ(client.debug_game_snapshot(3)->convention, Convention::REACTOR0);
  ASSERT_EQ(client.debug_game_snapshot(4)->convention, Convention::REACTOR);
  ASSERT_FALSE(client.debug_game_snapshot(3)->all_plays);
  ASSERT_FALSE(client.debug_game_snapshot(4)->all_plays);

  // Unlike /setall, /allplays retro-applies to running games — but only the
  // ones the flag means something for.
  client.handle_message("chat", chat("/allplays on"));

  EXPECT_FALSE(client.debug_game_snapshot(3)->all_plays)
      << "flipping the mode mid-game must not desync a running reactor0 game";
  EXPECT_TRUE(client.debug_game_snapshot(4)->all_plays)
      << "the running reactor game must still pick it up";
}
