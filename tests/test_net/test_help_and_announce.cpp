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

// --- `tableGone` must not clear the flag ---------------------------------
//
// WHAT THESE CAN AND CANNOT SEE. `queue_send` drops silently when the transport
// is not connected (`ws_transport.cpp:222`) and never touches `pending_`, so the
// chat line itself is invisible to a test. The flag is the only seam -- and it
// is a latch, so it cannot COUNT announcements.
//
// It does not need to. `announce_join` sends iff
// `announced_tables_.insert(tid).second`, i.e. iff the id was absent. So:
//
//     no second send when the table comes back
//   <=>  the id was present when it came back
//   <=>  the flag survived the `tableGone` in between
//
// The assertion immediately after `tableGone` is therefore exactly equivalent
// to the send being suppressed, and it is the one that discriminates: on the
// build before v9.4.0 the flag reads false there.

// `tableGone` is not only sent when we leave. The server sends it at GAME END
// and then re-sends the same id as the shared replay, still `joined: true`.
// Clearing here made the joined-diff in `on_table` read that as a fresh join, so
// the bot announced itself again after every game.
TEST(BotAnnounce, ATableComingBackAfterTableGoneDoesNotReAnnounce) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("table", table_payload(7, true));
  ASSERT_TRUE(client.debug_announced_table(7)) << "guard: the real join spoke";

  client.handle_message("tableGone", json{{"tableID", 7}});
  EXPECT_TRUE(client.debug_announced_table(7))
      << "the flag outlives the table: it records that we have already "
         "introduced ourselves at this id, not that the table exists";

  client.handle_message("table", table_payload(7, true));
  EXPECT_TRUE(client.debug_announced_table(7));
}

// The live sequence, transcribed from logs/bot-0.log table 327 (lines 855-1040),
// which is the shape that actually reached the lobby.
TEST(BotAnnounce, EndOfGameReplayTableDoesNotReAnnounce) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  auto table_327 = [](const char* name, bool running) {
    return json{{"id", 327},
                {"joined", true},
                {"running", running},
                {"name", name},
                {"players", json::array({"TestBot"})}};
  };

  // 855 -- we sit down. This is the one announcement that is owed.
  client.handle_message("table", table_327("threw imperiling cf", false));
  ASSERT_TRUE(client.debug_announced_table(327)) << "guard: the real join spoke";

  // 865 -- the game starts; the table is resent, still joined. No new edge.
  client.handle_message("table", table_327("threw imperiling cf", true));

  // 1019 -- game end. THE DISCRIMINATOR: before v9.4.0 this cleared the flag,
  // which is precisely what let the replay below speak.
  client.handle_message("tableGone", json{{"tableID", 327}});
  EXPECT_TRUE(client.debug_announced_table(327))
      << "game end must not make the bot forget it has already introduced "
         "itself at table 327";

  // 1026 -- the game is stored.
  client.handle_message("finishOngoingGame",
                        json{{"tableID", 327}, {"databaseID", 1939888}});

  // 1031, 1036, 1038 -- back as the shared replay, same id, still joined. With
  // the flag intact `announce_join`'s insert fails and it returns without
  // sending.
  client.handle_message("table",
                        table_327("threw imperiling cf (Game #1939888)", true));
  client.handle_message("table",
                        table_327("threw imperiling cf (Game #1939888)", true));
  EXPECT_TRUE(client.debug_announced_table(327));

  // 1040 -- and it goes for good, still without clearing.
  client.handle_message("tableGone", json{{"tableID", 327}});
  EXPECT_TRUE(client.debug_announced_table(327))
      << "so a later resend of this id cannot speak either";
}

// The fix must suppress the REPLAY, not the feature.
TEST(BotAnnounce, AFreshTableIdStillAnnounces) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("table", table_payload(327, true));
  ASSERT_TRUE(client.debug_announced_table(327)) << "guard: 327 has spoken";
  client.handle_message("tableGone", json{{"tableID", 327}});

  client.handle_message("table", table_payload(400, true));
  EXPECT_TRUE(client.debug_announced_table(400))
      << "a table id we have never introduced ourselves at still announces";
}
