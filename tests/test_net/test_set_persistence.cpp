// `/set` reassignments must survive a game start.
//
// A player sets clue meanings in the lobby (or in a replay) and expects the
// games that follow to use them. What happened instead: `chat_set` stored the
// override on the client AND retro-applied it to every running game, but
// `on_init` never copied it into the game it was building, and `Game::create`
// leaves the field empty. So the moment a game started the table silently
// reverted to the variant's built-in meanings -- while `reactive_overrides_`
// still held the player's choices, so the next unrelated `/set` re-pushed the
// whole list and everything reappeared at once.
//
// The overrides are keyed to a variant because `ReactiveOverride::clue_value`
// is a raw index into `Variant::clue_colour_names`: the same entry carried onto
// another variant would point at a different colour. `overrides_for` is the one
// place that test lives.
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "hanabi/basics/clue.h"
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
          ("setpersist_" + std::to_string(::getpid()) + "_" +
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

// The LOBBY view. `/set` before any game exists resolves its variant through
// this, so a test that skips it is exercising the replay path instead.
json table_payload(int tid, const std::string& variant) {
  return json{{"id", tid},
              {"options", json{{"variantName", variant}, {"numPlayers", 3}}}};
}

json init_payload(int tid, const std::string& variant) {
  return json{{"tableID", tid},
              {"playerNames", json::array({"alice", "bob", "TestBot"})},
              {"ourPlayerIndex", 2},
              {"options", json{{"variantName", variant}, {"numPlayers", 3}}}};
}

// Yellow is index 1 in No Variant and odd with reactive value 2 by default
// (pinned by Reactor0ReactiveAssignment.DefaultYellowIsOddWithValueTwo).
// "/set Yellow even 4" moves it into the other bucket.
bool has_yellow_even_four(const std::vector<ReactiveOverride>& ovs) {
  for (const auto& o : ovs) {
    if (o.kind == ClueKind::COLOUR && o.clue_value == 1 && o.even &&
        o.reactive_value == 4) {
      return true;
    }
  }
  return false;
}

}  // namespace

// --- the regression -------------------------------------------------------

TEST(SetPersistence, AnOverrideSetInTheLobbySurvivesTheGameStart) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("table", table_payload(7, "No Variant"));
  client.handle_message("chat", chat("/set Yellow even 4"));

  client.handle_message("init", init_payload(7, "No Variant"));
  auto rec = client.debug_game_snapshot(7);
  ASSERT_TRUE(rec.has_value());
  EXPECT_TRUE(has_yellow_even_four(rec->reactive_overrides))
      << "the game must start with the meanings the player set, not the "
         "variant's built-in table";
}

TEST(SetPersistence, AndSurvivesTheNextGameToo) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("table", table_payload(7, "No Variant"));
  client.handle_message("chat", chat("/set Yellow even 4"));
  client.handle_message("init", init_payload(7, "No Variant"));
  client.handle_message("init", init_payload(8, "No Variant"));

  EXPECT_TRUE(
      has_yellow_even_four(client.debug_game_snapshot(8)->reactive_overrides))
      << "persistence is between games, not just into the first one";
}

// --- the variant is the invalidation key ----------------------------------

TEST(SetPersistence, ADifferentVariantGetsItsOwnBuiltInTable) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("table", table_payload(7, "No Variant"));
  client.handle_message("chat", chat("/set Yellow even 4"));

  client.handle_message("init", init_payload(9, "Black (6 Suits)"));
  EXPECT_TRUE(client.debug_game_snapshot(9)->reactive_overrides.empty())
      << "colour indices belong to the variant they were authored against";
}

// Non-destructive: a game on another variant must not COST the player their
// settings, or opening one replay would wipe them.
TEST(SetPersistence, AnotherVariantDoesNotDestroyTheStoredList) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("table", table_payload(7, "No Variant"));
  client.handle_message("chat", chat("/set Yellow even 4"));

  client.handle_message("init", init_payload(9, "Black (6 Suits)"));
  ASSERT_TRUE(client.debug_game_snapshot(9)->reactive_overrides.empty());

  client.handle_message("init", init_payload(10, "No Variant"));
  EXPECT_TRUE(
      has_yellow_even_four(client.debug_game_snapshot(10)->reactive_overrides))
      << "back on the original variant, the settings are still there";
}

// But a `/set` on the new variant does start clean -- the dedupe key is
// `(kind, clue_value)`, so a carried-over entry would collide by index.
TEST(SetPersistence, ASetOnANewVariantDropsTheOldListRatherThanMerging) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("table", table_payload(7, "No Variant"));
  client.handle_message("chat", chat("/set Yellow even 4"));

  // Same table id, now a different variant -- the lobby view is replaced the
  // way the server's echoed `table` message replaces it after /setvariant.
  client.handle_message("table", table_payload(7, "Black (6 Suits)"));
  client.handle_message("chat", chat("/set 3 odd 3"));

  client.handle_message("init", init_payload(11, "Black (6 Suits)"));
  const auto& ovs = client.debug_game_snapshot(11)->reactive_overrides;
  EXPECT_EQ(ovs.size(), 1u) << "the No Variant entry must not have been carried";
  EXPECT_FALSE(has_yellow_even_four(ovs));
}

// --- the replay path ------------------------------------------------------

// A replay never appears in the lobby view, so `table_info` used to fail and
// `/set` was a silent no-op there -- which made persistence FROM a replay
// meaningless.
TEST(SetPersistence, SetWorksInsideAReplay) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  // No `table` message: this is the replay shape.
  client.handle_message("init", init_payload(7, "No Variant"));
  ASSERT_TRUE(client.debug_game_snapshot(7)->reactive_overrides.empty());

  client.handle_message("chat", chat("/set Yellow even 4"));
  EXPECT_TRUE(
      has_yellow_even_four(client.debug_game_snapshot(7)->reactive_overrides))
      << "/set retro-applies to the open replay";

  client.handle_message("init", init_payload(12, "No Variant"));
  EXPECT_TRUE(
      has_yellow_even_four(client.debug_game_snapshot(12)->reactive_overrides))
      << "and carries into the next real game";
}
