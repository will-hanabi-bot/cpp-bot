// A finished game must be RELEASED, not just marked finished.
//
// Until v9.4.0 nothing ever erased `games_`: `on_init` inserted a
// `unique_ptr<Game>` per table and no code path removed it. `finish_game`
// cleared `games_in_progress_` and `on_database_id` cleared `game_loggers_`, so
// the leak was invisible in every other seam -- the bot looked idle while still
// holding every game it had ever played.
//
// A `Game` is not small. It carries a `Player` per seat, each with a `thoughts`
// vector the size of the deck, plus `common`, plus `base` -- a second deep copy
// of the state, meta, players and common taken at game start for rewinds. One
// live transcript (logs/bot-0.log) records 719 games in a single run, and the
// endgame solver copies a `Game` per search node on top of whatever is already
// resident. The bots died of `std::bad_alloc`.
//
// `debug_game_snapshot` is the seam: it returns nullopt for a table with no
// entry in `games_`, so "the game was released" is directly observable.
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

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

// on_init writes into a relative "logs" dir; keep the repo's logs/ clean.
struct ScopedTempCwd {
  fs::path prev, dir;
  ScopedTempCwd() {
    prev = fs::current_path();
    dir = fs::temp_directory_path() /
          ("gamelifecycle_" + std::to_string(::getpid()) + "_" +
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

json init_payload(int tid, bool replay = false) {
  json j{{"tableID", tid},
         {"playerNames", json::array({"alice", "bob", "TestBot"})},
         {"ourPlayerIndex", 2},
         {"options", json{{"variantName", "No Variant"}, {"numPlayers", 3}}}};
  if (replay) j["replay"] = true;
  return j;
}

// Game end only ever arrives as a `gameOver` gameAction -- the server sends no
// top-level `gameOver` command to a socket that played in the game. That is
// pinned by test_finish_ongoing_game.cpp; this file depends on it.
json game_over_action(int tid) {
  return json{{"tableID", tid},
              {"action", json{{"type", "gameOver"},
                              {"endCondition", 4},
                              {"playerIndex", 0}}}};
}

json draw_action(int tid, int order) {
  return json{{"tableID", tid},
              {"action", json{{"type", "draw"},
                              {"playerIndex", 0},
                              {"order", order},
                              {"suitIndex", 0},
                              {"rank", 1}}}};
}

}  // namespace

// --- the normal path ------------------------------------------------------

TEST(GameLifecycle, FinishedGameIsReleased) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(1136));
  ASSERT_TRUE(client.debug_game_snapshot(1136).has_value())
      << "guard: the game exists while it is being played";

  client.handle_message("gameAction", game_over_action(1136));
  EXPECT_FALSE(client.debug_game_snapshot(1136).has_value())
      << "the Game must be released at game end, not held for the life of the "
         "process";
}

// `finish_game` is idempotent on `games_in_progress_`, and the release sits
// below that gate -- so a second game-end for the same table is a no-op rather
// than a double erase or a resurrection.
TEST(GameLifecycle, ASecondGameEndIsHarmless) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(42));
  client.handle_message("gameAction", game_over_action(42));
  ASSERT_FALSE(client.debug_game_snapshot(42).has_value());

  EXPECT_NO_THROW(
      client.handle_message("gameOver", {{"tableID", 42}, {"endCondition", 4}}));
  EXPECT_FALSE(client.debug_game_snapshot(42).has_value());
}

// The release must not outlive its table: a NEW game at the same id gets a
// fresh Game, exactly as before.
TEST(GameLifecycle, ANewGameAtTheSameTableStillWorks) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(77));
  client.handle_message("gameAction", game_over_action(77));
  ASSERT_FALSE(client.debug_game_snapshot(77).has_value());

  client.handle_message("init", init_payload(77));
  EXPECT_TRUE(client.debug_game_snapshot(77).has_value())
      << "the table plays again and gets a new Game";
}

// --- the paths that never reach finish_game -------------------------------

// `on_init` only records `games_in_progress_` when `!is_replay`, so a replay
// never reaches `finish_game` and would leak on its own. `on_table_gone` is the
// catch-all.
TEST(GameLifecycle, ReplayIsReleasedOnTableGone) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(500, /*replay=*/true));
  ASSERT_TRUE(client.debug_game_snapshot(500).has_value())
      << "guard: a replay does build a Game";

  client.handle_message("tableGone", json{{"tableID", 500}});
  EXPECT_FALSE(client.debug_game_snapshot(500).has_value())
      << "a replay never sees gameOver, so tableGone has to release it";
}

// And a table abandoned mid-game -- terminated, or the socket dropped out from
// under it -- reaches neither `gameOver` nor a second `init`.
TEST(GameLifecycle, AbandonedGameIsReleasedOnTableGone) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(501));
  ASSERT_TRUE(client.debug_game_snapshot(501).has_value());

  client.handle_message("tableGone", json{{"tableID", 501}});
  EXPECT_FALSE(client.debug_game_snapshot(501).has_value());
}

// --- what the release relies on -------------------------------------------

// The release is only safe because every reader of `games_` does a `find` and
// returns on a miss. A `gameAction` arriving after the release -- the server
// still has messages in flight when the game ends -- must be a no-op, not a
// dereference of an absent entry.
TEST(GameLifecycle, ALateGameActionAfterReleaseIsHarmless) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(1136));
  client.handle_message("gameAction", game_over_action(1136));
  ASSERT_FALSE(client.debug_game_snapshot(1136).has_value());

  EXPECT_NO_THROW(client.handle_message("gameAction", draw_action(1136, 15)));
  EXPECT_FALSE(client.debug_game_snapshot(1136).has_value())
      << "a late action must not resurrect the table either";
}

// `finishOngoingGame` always arrives AFTER game end, and the release happens at
// game end -- so the log-rename path has to work with the Game already gone. It
// reads only `game_loggers_`, which `finish_game` deliberately leaves open for
// exactly this.
TEST(GameLifecycle, DatabaseIdStillArrivesAfterTheGameIsReleased) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init", init_payload(1136));
  client.handle_message("gameAction", game_over_action(1136));
  ASSERT_FALSE(client.debug_game_snapshot(1136).has_value());

  EXPECT_NO_THROW(client.handle_message(
      "finishOngoingGame",
      json{{"tableID", 1136}, {"databaseID", 1940573}}));
  EXPECT_TRUE(fs::exists(fs::path("logs") / "TestBot-1940573.log"))
      << "the log is still renamed under the database id";
}
