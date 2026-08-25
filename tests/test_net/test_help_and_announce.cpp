// `/help`, and the announcement the bot makes when it sits down at a table.
//
// Nothing the bot SENDS is observable from a test: `BotTransport::queue_send`
// is non-virtual and drops silently when not connected, which
// `test_commands.cpp` documents at length. So the two texts live in pure
// functions that are pinned directly here -- the same arrangement that lets
// `reactor0::format_settings` be pinned verbatim -- and the join EDGE is
// pinned through the `debug_announced_table` seam.
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "hanabi/basics/convention.h"
#include "hanabi/net/commands.h"
#include "hanabi/net/ws_transport.h"
#include "hanabi/settings.h"
#include "hanabi/version.h"

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
          ("helpannounce_" + std::to_string(::getpid()) + "_" +
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

json table_payload(int tid, bool joined) {
  return json{{"id", tid},
              {"joined", joined},
              {"running", false},
              {"players", json::array({"TestBot"})}};
}

std::string all_help(std::string_view who) {
  std::string out;
  for (const std::string& line : help_lines(who)) out += line + "\n";
  return out;
}

}  // namespace

// --- /help ----------------------------------------------------------------

// The load-bearing property: `/help` has to mention EVERY command the
// dispatcher answers. Listing the triggers here means adding a command without
// updating the help fails a test rather than drifting quietly, which is what
// happened to the README's own table more than once.
TEST(BotHelp, MentionsEveryCommandTheDispatcherAnswers) {
  const std::string text = all_help("TestBot");
  for (const char* cmd : {"/help", "/settings", "/setall", "/set ", "/rlocks",
                          "/allplays", "/getversion", "/leaveall", "/join",
                          "/create", "/start", "/setvariant", "/terminate"}) {
    EXPECT_NE(text.find(cmd), std::string::npos)
        << "`/help` never mentions " << cmd;
  }
}

// Several bots answer at once, so every line has to say who is talking -- the
// same reason `/getversion` and `/rlocks` prefix theirs.
TEST(BotHelp, EveryLineIsUsernamePrefixed) {
  const auto lines = help_lines("will-bot67");
  ASSERT_FALSE(lines.empty());
  for (const std::string& line : lines) {
    EXPECT_EQ(line.rfind("will-bot67", 0), 0u)
        << "unattributable help line: " << line;
  }
}

// A chat message is one line, so the text must not smuggle newlines into a
// single send -- it is split across sends instead.
TEST(BotHelp, NoLineContainsANewline) {
  for (const std::string& line : help_lines("TestBot")) {
    EXPECT_EQ(line.find('\n'), std::string::npos) << line;
  }
}

TEST(BotHelp, AnswersFromARoomAndFromAPm) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  // The ceiling of what the harness can see: that neither path throws.
  EXPECT_NO_THROW(client.handle_message(
      "chat", json{{"msg", "/help"},
                   {"recipient", ""},
                   {"room", "table7"},
                   {"who", "yagami_black"}}));
  EXPECT_NO_THROW(client.handle_message(
      "chat", json{{"msg", "/help"},
                   {"recipient", "TestBot"},
                   {"room", ""},
                   {"who", "yagami_black"}}));
}

// --- the announcement text ------------------------------------------------

TEST(BotAnnounce, RendersTheConventionAndTheVersion) {
  const std::string s =
      join_announcement("will-bot67", Convention::REACTOR0, kBotVersion);
  EXPECT_NE(s.find("will-bot67"), std::string::npos);
  EXPECT_NE(s.find("reactor0"), std::string::npos);
  EXPECT_NE(s.find(kBotVersion), std::string::npos);
  EXPECT_EQ(s.find('\n'), std::string::npos) << "one line, one send";
}

TEST(BotAnnounce, TracksTheSelectedConvention) {
  EXPECT_NE(join_announcement("b", Convention::REACTOR, "v1.0.0").find("reactor"),
            std::string::npos);
  // "reactor0" must not be reported as plain "reactor" or the other way round.
  EXPECT_EQ(join_announcement("b", Convention::REACTOR, "v1.0.0").find("reactor0"),
            std::string::npos);
}

// --- the join edge --------------------------------------------------------

TEST(BotAnnounce, JoiningATableAnnouncesOnce) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  EXPECT_FALSE(client.debug_announced_table(7)) << "nothing said yet";
  client.handle_message("table", table_payload(7, /*joined=*/true));
  EXPECT_TRUE(client.debug_announced_table(7));
}

// `table` is a snapshot the server resends on any change, so the edge has to be
// a diff -- otherwise every update to a joined table would re-announce.
TEST(BotAnnounce, ASecondTableMessageDoesNotReAnnounce) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("table", table_payload(7, true));
  ASSERT_TRUE(client.debug_announced_table(7));
  // Idempotent: the flag is already set, so the second edge cannot fire.
  EXPECT_NO_THROW(client.handle_message("table", table_payload(7, true)));
  EXPECT_TRUE(client.debug_announced_table(7));
}

TEST(BotAnnounce, ATableWeHaveNotJoinedIsNotAnnounced) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("table", table_payload(7, /*joined=*/false));
  EXPECT_FALSE(client.debug_announced_table(7))
      << "seeing a table in the lobby is not sitting down at it";
}

// The bulk snapshot is "here is the world", not "this changed" -- announcing
// from it would greet every table we were already in on every reconnect.
TEST(BotAnnounce, TheBulkTableListDoesNotAnnounce) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("tableList", json::array({table_payload(7, true)}));
  EXPECT_FALSE(client.debug_announced_table(7));
}

// Leaving and rejoining the same id is a real join, so it announces again.
TEST(BotAnnounce, RejoiningAfterTheTableIsGoneAnnouncesAgain) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("table", table_payload(7, true));
  ASSERT_TRUE(client.debug_announced_table(7));

  client.handle_message("tableGone", json{{"tableID", 7}});
  EXPECT_FALSE(client.debug_announced_table(7)) << "the flag is cleared";

  client.handle_message("table", table_payload(7, true));
  EXPECT_TRUE(client.debug_announced_table(7));
}
