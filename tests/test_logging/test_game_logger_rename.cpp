// GameLogger::rename_file — the database-id rename at game end: the log
// opens under the live table id and is renamed to the hanab.live
// database id once the server reveals it (BotClient::on_database_id).
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "hanabi/logging/game_logger.h"

namespace fs = std::filesystem;
using hanabi::logging::GameLogger;

namespace {

std::string read_all(const std::string& path) {
  std::ifstream in(path);
  std::ostringstream os;
  os << in.rdbuf();
  return os.str();
}

struct TempDir {
  fs::path dir;
  TempDir() {
    dir = fs::temp_directory_path() /
          ("gl_rename_test_" + std::to_string(::getpid()));
    fs::create_directories(dir);
  }
  ~TempDir() {
    std::error_code ec;
    fs::remove_all(dir, ec);
  }
};

}  // namespace

TEST(GameLoggerRename, RenamesFileAndKeepsAppending) {
  TempDir tmp;
  GameLogger gl("Test Bot", 42, tmp.dir.string());
  std::string old_path = GameLogger::log_path("Test Bot", 42, tmp.dir.string());
  ASSERT_EQ(gl.path(), old_path);
  gl.emit_lifecycle("before_rename");

  std::string new_path =
      GameLogger::log_path("Test Bot", 1900001, tmp.dir.string());
  ASSERT_TRUE(gl.rename_file(new_path));
  EXPECT_EQ(gl.path(), new_path);
  EXPECT_FALSE(fs::exists(old_path));
  ASSERT_TRUE(fs::exists(new_path));

  gl.emit_lifecycle("after_rename");
  std::string contents = read_all(new_path);
  EXPECT_NE(contents.find("before_rename"), std::string::npos)
      << "records written before the rename must travel with the file";
  EXPECT_NE(contents.find("after_rename"), std::string::npos)
      << "the reopened stream must append to the renamed file";
}

TEST(GameLoggerRename, RefusesToClobberExistingTarget) {
  TempDir tmp;
  GameLogger gl("bot", 7, tmp.dir.string());
  gl.emit_lifecycle("keep");

  std::string blocked = GameLogger::log_path("bot", 1900002, tmp.dir.string());
  { std::ofstream(blocked) << "existing\n"; }

  EXPECT_FALSE(gl.rename_file(blocked));
  EXPECT_EQ(gl.path(), GameLogger::log_path("bot", 7, tmp.dir.string()))
      << "failed rename keeps the original path";
  gl.emit_lifecycle("still_appending");
  EXPECT_NE(read_all(gl.path()).find("still_appending"), std::string::npos);
  EXPECT_EQ(read_all(blocked), "existing\n") << "target must be untouched";
}
