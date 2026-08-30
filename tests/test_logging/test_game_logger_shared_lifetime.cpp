// The per-game logger must outlive the map entry that owns it (v13.1.0).
//
// `BotClient` posts each turn's `take_action` to a compute thread and hands the
// job the game's logger. The network thread meanwhile erases
// `game_loggers_[table_id]` the moment the game ends -- `on_database_id` on the
// normal path, `on_init` for a reused table id. While that map held a
// `unique_ptr` and the job held a RAW pointer, the erase freed the logger under
// a running solve and the job wrote through the dangling pointer as soon as
// `take_action` returned.
//
// That is what killed will-bot69 at table 5055 / game 1978386: it ran out of
// clock, so it alone still had a solve in flight when the game ended;
// will-bot67 had none and survived. A segfault leaves no record in the game
// log, which is why the only trace was the network transcript.
//
// The fix is shared ownership, and this pins the two properties it buys:
//   1. a reference taken the way the compute post takes one keeps the logger
//      alive after the owning map entry is erased, and
//   2. a record emitted through that reference AFTER finalisation lands in the
//      finalised file rather than resurrecting the table-id one.
//
// The race itself is not reproduced here: there is no sanitizer build in this
// tree, and without one a use-after-free frequently does not fault, so a
// "drive it and see" test would risk passing while broken. This tests the
// ownership contract the fix rests on, not the timing.
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
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
          ("gl_shared_test_" + std::to_string(::getpid()) + "_" +
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

TEST(GameLoggerSharedLifetime, AJobsReferenceOutlivesTheOwningMapEntry) {
  TempDir tmp;
  // The shape `BotClient::game_loggers_` has, and the copy the compute post
  // captures by value.
  std::unordered_map<int, std::shared_ptr<GameLogger>> loggers;
  loggers[5055] = std::make_shared<GameLogger>("Test Bot", 5055, tmp.dir.string());
  std::shared_ptr<GameLogger> job_ref = loggers[5055];
  ASSERT_EQ(job_ref.use_count(), 2) << "the map and the job each hold one";

  job_ref->emit_lifecycle("decide_start", json{{"turn", 57}});

  // Game over: the network thread finalises and drops the map's reference while
  // the job is still running.
  ASSERT_TRUE(loggers[5055]->finalize_with_database_id(1978386));
  loggers.erase(5055);
  EXPECT_EQ(job_ref.use_count(), 1)
      << "the job now holds the only reference -- under unique_ptr the object "
         "would already be destroyed and the next line a use-after-free";

  // The solve returns and writes its result. This must be safe.
  job_ref->emit_lifecycle("outbound_action", json{{"turn", 57}});

  const std::string finalised =
      GameLogger::log_path("Test Bot", 1978386, tmp.dir.string());
  const std::string table_path =
      GameLogger::log_path("Test Bot", 5055, tmp.dir.string());
  job_ref.reset();  // the job ends; last reference goes, file is flushed

  EXPECT_TRUE(fs::exists(finalised));
  EXPECT_FALSE(fs::exists(table_path))
      << "a late write must not resurrect the table-id log";

  auto lines = read_lines(finalised);
  bool saw_late = false;
  for (const auto& l : lines) {
    json rec = json::parse(l);
    if (rec.value("event", "") == "outbound_action") saw_late = true;
  }
  EXPECT_TRUE(saw_late)
      << "the record the job emitted after finalisation belongs in the "
         "finalised file";
}
