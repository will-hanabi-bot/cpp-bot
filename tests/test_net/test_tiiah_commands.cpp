// `/setall tiiah`, and what `/settings` says at a Throw It in a Hole table.
//
// `tiiah` parses — a snapshot has to round-trip through the name — but it is
// resolved from the VARIANT and never chosen. Setting it as the bot-wide mode
// would leave something that silently means reactor at every table that is not
// a TIIAH one, so the command is refused.
#include <gtest/gtest.h>

#include <filesystem>

#include <nlohmann/json.hpp>

#include "hanabi/basics/convention.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/tiiah/settings.h"
#include "hanabi/net/commands.h"
#include "hanabi/net/ws_transport.h"

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
          ("tiiah_cmd_test_" + std::to_string(::getpid()) + "_" +
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
  return json{{"msg", msg},
              {"recipient", ""},
              {"room", "table7"},
              {"who", "yagami_black"}};
}

json init_payload(int tid, const std::string& variant) {
  return json{{"tableID", tid},
              {"playerNames", json::array({"alice", "bob", "TestBot"})},
              {"ourPlayerIndex", 2},
              {"options", json{{"variantName", variant}, {"numPlayers", 3}}}};
}

}  // namespace

// The refusal: the mode is left alone, so the next ordinary table still runs
// whatever it ran before.
TEST(TiiahCommands, SetallTiiahDoesNotChangeTheMode) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  EXPECT_NO_THROW(client.handle_message("chat", chat("/setall tiiah")));

  client.handle_message("init", init_payload(7, "No Variant"));
  auto rec = client.debug_game_snapshot(7);
  ASSERT_TRUE(rec.has_value());
  EXPECT_EQ(rec->convention, Convention::REACTOR0)
      << "`/setall tiiah` must not leave a mode that means reactor everywhere";
}

// ...and the variant still selects it, which is the only way in.
TEST(TiiahCommands, TheVariantSelectsTheConvention) {
  ScopedTempCwd cwd;
  BotConfig cfg = make_config();
  BotTransport transport("ws://localhost/ws", "", [](auto, auto) {});
  BotClient client(transport, cfg);
  client.handle_message("welcome", {{"username", "TestBot"}});

  client.handle_message("init",
                        init_payload(8, "Throw It in a Hole (5 Suits)"));
  auto rec = client.debug_game_snapshot(8);
  ASSERT_TRUE(rec.has_value());
  EXPECT_EQ(rec->convention, Convention::TIIAH);
}

// The `/settings` line. Nothing the bot SENDS is observable from a test, so the
// line is pinned where it is built. It is the user's text, verbatim (v24.0.0): one
// for five or six non-inverted suits, whose shift follows the epoch, and one for
// three or four. The first is exactly the chat limit.
TEST(TiiahCommands, TheSettingsLinesAreTheUsersText) {
  const std::string five_six =
      "Each non-orange suit gets a # based on its stack position starting with "
      "red=0. Epoch = ceiling(turn # / 3). All clues to Cathy are even reactive. "
      "Role inversion on. Rank means Cathy's suit = Bob's suit + (1 if odd epoch "
      "else 2), color means Cathy = Bob - (1 if odd epoch else 2). Orange tgt = 2 "
      "disc";
  const std::string three_four =
      "Each non-orange suit gets a # based on its stack position starting with "
      "red=0. All clues to Cathy are even reactive. Role inversion on. Rank means "
      "Cathy's suit = Bob's suit + 1, while color means Cathy's suit = Bob's suit "
      "- 1. Orange tgt = 2 disc";

  for (const char* name : {"Throw It in a Hole (5 Suits)",
                           "Throw It in a Hole & Black (6 Suits)",
                           "Throw It in a Hole & Orange (6 Suits)"}) {
    EXPECT_EQ(hanabi::tiiah::format_settings(get_variant(name)), five_six) << name;
  }
  for (const char* name : {"Throw It in a Hole (4 Suits)",
                           "Throw It in a Hole & Orange (5 Suits)",
                           "Throw It in a Hole & Orange (4 Suits)"}) {
    EXPECT_EQ(hanabi::tiiah::format_settings(get_variant(name)), three_four) << name;
  }
  EXPECT_EQ(five_six.size(), kMaxChatLength) << "the user's measure of the limit";
}

// Every TIIAH variant's line fits in one chat message.
TEST(TiiahCommands, EverySettingsLineFitsTheChatLimit) {
  int checked = 0;
  for (const auto& [name, v] : load_variants()) {
    if (!v.throw_it_in_a_hole) continue;
    ++checked;
    EXPECT_LE(hanabi::tiiah::format_settings(v).size(), kMaxChatLength) << name;
  }
  EXPECT_EQ(checked, 44);
}
