// GameLogger::finalize_with_database_id — the game-end rewrite.
//
// The hanab.live database id (the one replay URLs use) is only revealed
// after the game is over, by which point every record a consumer reads is
// already on disk. So finalization rewrites the log rather than renaming
// it, stamping `database_id` onto every record so the finished file is
// uniform. See CLAUDE.md "Debugging a bug report".
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "hanabi/logging/game_logger.h"

namespace fs = std::filesystem;
using hanabi::logging::GameLogger;
using nlohmann::json;

namespace {

struct TempDir {
  fs::path dir;
  TempDir() {
    dir = fs::temp_directory_path() /
          ("gl_finalize_test_" + std::to_string(::getpid()) + "_" +
           std::to_string(reinterpret_cast<uintptr_t>(this)));
    fs::create_directories(dir);
  }
  ~TempDir() {
    std::error_code ec;
    fs::remove_all(dir, ec);
  }
};

std::vector<std::string> read_lines(const std::string& path) {
  std::vector<std::string> out;
  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty()) out.push_back(line);
  }
  return out;
}

}  // namespace

TEST(GameLoggerFinalize, StampsDatabaseIdOnEveryPreExistingRecord) {
  TempDir tmp;
  GameLogger gl("Test Bot", 1136, tmp.dir.string());
  gl.emit_lifecycle("game_init", json{{"variant", "No Variant"}});
  gl.emit(json{{"ch", "STATE"}, {"turn", 3}});
  gl.emit(json{{"ch", "DECIDE"}, {"msg", "branch"}});

  ASSERT_TRUE(gl.finalize_with_database_id(1940573));

  std::string expected = GameLogger::log_path("Test Bot", 1940573, tmp.dir.string());
  EXPECT_EQ(gl.path(), expected);
  EXPECT_TRUE(fs::exists(expected));
  EXPECT_FALSE(fs::exists(GameLogger::log_path("Test Bot", 1136, tmp.dir.string())))
      << "the table-id log must not survive alongside the database-id one";

  auto lines = read_lines(expected);
  ASSERT_EQ(lines.size(), 4u) << "3 original records + the mapping record";
  for (size_t i = 0; i < lines.size(); ++i) {
    json rec = json::parse(lines[i]);
    EXPECT_EQ(rec.value("database_id", -1), 1940573)
        << "record " << i << " lacks database_id: " << lines[i];
    EXPECT_EQ(rec.value("game_id", -1), 1136)
        << "record " << i << " must keep the live table id too";
  }
  // Original content survives the rewrite.
  EXPECT_NE(lines[0].find("game_init"), std::string::npos);
  EXPECT_NE(lines[1].find("STATE"), std::string::npos);
  EXPECT_NE(lines[2].find("DECIDE"), std::string::npos);
  // ...and the mapping record is appended last.
  json last = json::parse(lines[3]);
  EXPECT_EQ(last.value("event", ""), "database_id");
  EXPECT_EQ(last.value("ch", ""), "LIFECYCLE");
}

TEST(GameLoggerFinalize, KeepsAppendingWithBothIdsAfterwards) {
  TempDir tmp;
  GameLogger gl("bot", 7, tmp.dir.string());
  gl.emit_lifecycle("before");
  ASSERT_TRUE(gl.finalize_with_database_id(1900500));
  EXPECT_EQ(gl.database_id().value_or(-1), 1900500);

  gl.emit_lifecycle("after");
  auto lines = read_lines(gl.path());
  json after = json::parse(lines.back());
  EXPECT_EQ(after.value("event", ""), "after");
  EXPECT_EQ(after.value("database_id", -1), 1900500)
      << "emit() must stamp database_id once finalization has run";
  EXPECT_EQ(after.value("game_id", -1), 7);
}

TEST(GameLoggerFinalize, PreservesUnparseableLinesVerbatim) {
  TempDir tmp;
  GameLogger gl("bot", 11, tmp.dir.string());
  gl.emit_lifecycle("good");
  // Simulate a torn final record from a crashed run.
  {
    std::ofstream out(gl.path(), std::ios::app);
    out << "{\"ch\":\"DECIDE\",\"trunc\n";
  }

  ASSERT_TRUE(gl.finalize_with_database_id(1900600));
  auto lines = read_lines(gl.path());
  bool found = false;
  for (const auto& l : lines) {
    if (l == "{\"ch\":\"DECIDE\",\"trunc") found = true;
  }
  EXPECT_TRUE(found) << "a torn record is evidence; it must not be dropped";
}

TEST(GameLoggerFinalize, RefusesToClobberExistingTarget) {
  TempDir tmp;
  GameLogger gl("bot", 3, tmp.dir.string());
  gl.emit_lifecycle("keep");
  std::string old_path = gl.path();

  std::string blocked = GameLogger::log_path("bot", 1900700, tmp.dir.string());
  { std::ofstream(blocked) << "existing\n"; }

  EXPECT_FALSE(gl.finalize_with_database_id(1900700));
  EXPECT_EQ(gl.path(), old_path) << "a refused finalize keeps the original path";
  EXPECT_EQ(read_lines(blocked).size(), 1u);
  EXPECT_EQ(read_lines(blocked)[0], "existing") << "target must be untouched";

  gl.emit_lifecycle("still_appending");
  auto lines = read_lines(old_path);
  EXPECT_NE(lines.back().find("still_appending"), std::string::npos);
}

TEST(GameLoggerFinalize, SameIdIsANoOp) {
  TempDir tmp;
  GameLogger gl("bot", 555, tmp.dir.string());
  gl.emit_lifecycle("x");
  std::string before = gl.path();
  EXPECT_TRUE(gl.finalize_with_database_id(555));
  EXPECT_EQ(gl.path(), before);
  EXPECT_EQ(gl.database_id().value_or(-1), 555);
}
