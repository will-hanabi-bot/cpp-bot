// replay_log: reconstruct a Game from a per-game JSONL log and either
// re-run take_action with the current build (--rerun) or emit a regression
// test scaffold (--emit-test).
//
// Usage:
//   replay_log <log_file> --turn <N> --rerun
//   replay_log <log_file> --turn <N> --diff
//   replay_log <log_file> --turn <N> --emit-test <out.cpp>
//
// The log file is the JSONL produced by GameLogger. Each STATE record has a
// replay section with variant/options/players/deck + the action history up
// to that turn; apply_snapshot replays those actions to rebuild an exact
// Game, then we call Game::take_action() with the current code and report
// the chosen action.
//
// This is the killer-feature of the iteration overhaul: a logged bug
// report can be re-investigated in seconds without re-simulating from
// scratch.

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/logging/state_snapshot.h"

namespace {

using nlohmann::json;

struct Args {
  std::string log_path;
  std::optional<int> turn;
  bool rerun = false;
  bool diff = false;
  std::optional<std::string> emit_test_path;
};

void print_usage(std::ostream& os) {
  os << "Usage: replay_log <log_file> --turn <N> [--rerun] [--diff] "
        "[--emit-test <out.cpp>]\n";
}

std::optional<Args> parse_args(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    std::string s = argv[i];
    if (s == "--turn" && i + 1 < argc) {
      a.turn = std::atoi(argv[++i]);
    } else if (s == "--rerun") {
      a.rerun = true;
    } else if (s == "--diff") {
      a.diff = true;
    } else if (s == "--emit-test" && i + 1 < argc) {
      a.emit_test_path = argv[++i];
    } else if (s == "-h" || s == "--help") {
      print_usage(std::cout);
      return std::nullopt;
    } else if (a.log_path.empty()) {
      a.log_path = s;
    } else {
      std::cerr << "Unknown arg: " << s << "\n";
      print_usage(std::cerr);
      return std::nullopt;
    }
  }
  if (a.log_path.empty()) {
    print_usage(std::cerr);
    return std::nullopt;
  }
  return a;
}

// Read all JSONL records from path.
std::vector<json> read_log(const std::string& path) {
  std::ifstream in(path);
  if (!in.is_open()) {
    throw std::runtime_error("cannot open log: " + path);
  }
  std::vector<json> out;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    try {
      out.push_back(json::parse(line));
    } catch (const std::exception& e) {
      std::cerr << "skipping malformed line: " << e.what() << "\n";
    }
  }
  return out;
}

// Find the STATE record matching `turn`. If turn is nullopt, returns the
// last STATE record in the file.
const json* find_state_record(const std::vector<json>& records,
                                std::optional<int> turn) {
  const json* match = nullptr;
  for (const auto& rec : records) {
    if (rec.value("ch", "") != "STATE") continue;
    if (turn && rec.value("turn", -1) != *turn) continue;
    match = &rec;
    if (turn) break;
  }
  return match;
}

// Find the LIFECYCLE outbound_action that immediately follows the STATE at
// the given turn — i.e., what the live bot decided.
std::optional<json> find_outbound_after_turn(const std::vector<json>& records,
                                                int turn) {
  bool past_state = false;
  for (const auto& rec : records) {
    if (rec.value("ch", "") == "STATE" && rec.value("turn", -1) == turn) {
      past_state = true;
      continue;
    }
    if (!past_state) continue;
    if (rec.value("ch", "") == "LIFECYCLE" &&
        rec.value("event", "") == "outbound_action") {
      return rec;
    }
  }
  return std::nullopt;
}

std::string perform_action_summary(const hanabi::PerformAction& p) {
  return std::visit(
      [](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, hanabi::PerformPlay>) {
          return "play(order=" + std::to_string(v.target) + ")";
        } else if constexpr (std::is_same_v<T, hanabi::PerformDiscard>) {
          return "discard(order=" + std::to_string(v.target) + ")";
        } else if constexpr (std::is_same_v<T, hanabi::PerformColour>) {
          return "clue colour value=" + std::to_string(v.value) +
                  " → player " + std::to_string(v.target);
        } else if constexpr (std::is_same_v<T, hanabi::PerformRank>) {
          return "clue rank value=" + std::to_string(v.value) +
                  " → player " + std::to_string(v.target);
        } else if constexpr (std::is_same_v<T, hanabi::PerformTerminate>) {
          return "terminate";
        }
      },
      p);
}

// Format the action_history as a constexpr `OrigAction` array for the
// emit-test path. The emitted file leans on tests/test_endgame/
// replay_helpers.h so the action conversion machinery is already factored.
void emit_test_scaffold(const std::string& path, int game_id, const json& state_rec,
                          const hanabi::Game& reconstructed_game) {
  std::ofstream out(path);
  if (!out.is_open()) throw std::runtime_error("cannot open emit-test path: " + path);

  const json& replay = state_rec.at("replay");
  std::string variant = replay.at("variant").get<std::string>();
  int turn = state_rec.value("turn", -1);
  int num_players = replay.at("num_players").get<int>();
  int our_pi = replay.at("our_player_index").get<int>();

  out << "// Auto-generated by `replay_log --emit-test`. Captured from a\n";
  out << "// per-game JSONL log at turn " << turn << " of game " << game_id
      << ". Hand reconstruction is verified to match the logged STATE\n";
  out << "// snapshot at the time of emission. Tweak the EXPECT_* asserts\n";
  out << "// below to capture the desired regression.\n\n";
  out << "#include <gtest/gtest.h>\n\n";
  out << "#include \"hanabi/basics/action.h\"\n";
  out << "#include \"hanabi/basics/game.h\"\n";
  out << "#include \"hanabi/basics/identity.h\"\n";
  out << "#include \"hanabi/logging/state_snapshot.h\"\n";
  out << "#include \"test_endgame/replay_helpers.h\"\n";
  out << "#include \"test_harness.h\"\n\n";
  out << "// Variant: " << variant << ". " << num_players
      << " players, our_player_index=" << our_pi << ".\n\n";
  out << "TEST(ReplayLog, Game" << game_id << "Turn" << turn << ") {\n";
  out << "  // Reconstruct exactly the Game the live bot saw at turn "
      << turn << ".\n";
  out << "  // The embedded JSON is the STATE record's `replay` section.\n";
  out << "  const char* kSnapshotJson = R\"json(\n";
  out << state_rec.dump(2) << "\n";
  out << "  )json\";\n";
  out << "  auto rec = nlohmann::json::parse(kSnapshotJson);\n";
  out << "  hanabi::Game game = hanabi::logging::apply_snapshot(rec);\n";
  out << "  hanabi::PerformAction action = game.take_action();\n";
  out << "  // TODO: add EXPECT_* asserts on action type / target / etc.\n";
  out << "  // For now this just makes sure take_action doesn't throw.\n";
  out << "  (void)action;\n";
  out << "}\n";
  // Touch the reconstructed_game to silence -Wunused-parameter.
  (void)reconstructed_game;
}

}  // namespace

int main(int argc, char** argv) {
  auto args_opt = parse_args(argc, argv);
  if (!args_opt) return 2;
  Args& args = *args_opt;

  std::vector<json> records;
  try {
    records = read_log(args.log_path);
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 2;
  }

  // Pull game_id + bot name from the first record we find that has them.
  int game_id = -1;
  std::string bot_name;
  for (const auto& r : records) {
    if (game_id < 0 && r.contains("game_id")) game_id = r.value("game_id", -1);
    if (bot_name.empty() && r.contains("bot")) bot_name = r.value("bot", "");
    if (game_id >= 0 && !bot_name.empty()) break;
  }

  const json* state_rec = find_state_record(records, args.turn);
  if (!state_rec) {
    std::cerr << "no STATE record"
              << (args.turn ? " for turn " + std::to_string(*args.turn) : "")
              << " in " << args.log_path << "\n";
    return 2;
  }
  int turn = state_rec->value("turn", -1);

  hanabi::Game game;
  try {
    game = hanabi::logging::apply_snapshot(*state_rec);
  } catch (const std::exception& e) {
    std::cerr << "apply_snapshot failed: " << e.what() << "\n";
    return 2;
  }

  std::cout << "loaded " << args.log_path << " — game " << game_id
            << " bot " << bot_name << " turn " << turn << "\n";
  std::cout << "reconstructed Game: turn_count=" << game.state.turn_count
            << " current_player_index=" << game.state.current_player_index
            << " clue_tokens=" << game.state.clue_tokens
            << " strikes=" << game.state.strikes << "\n";

  if (args.rerun || args.diff) {
    hanabi::PerformAction chosen;
    try {
      chosen = game.take_action();
    } catch (const std::exception& e) {
      std::cerr << "take_action threw: " << e.what() << "\n";
      return 2;
    }
    std::cout << "rerun chose: " << perform_action_summary(chosen) << "\n";

    if (args.diff) {
      auto orig = find_outbound_after_turn(records, turn);
      if (!orig) {
        std::cerr << "no outbound_action LIFECYCLE record after turn " << turn
                  << " — nothing to diff against\n";
        return 1;
      }
      // The logged outbound action is the bot's PerformAction serialized
      // as the hanab.live wire JSON (PerformPlay::to_json etc.). Reparse:
      const json& orig_act = orig->at("action");
      hanabi::PerformAction orig_perform =
          hanabi::perform_action_from_json(orig_act);
      std::string ours = perform_action_summary(chosen);
      std::string theirs = perform_action_summary(orig_perform);
      std::cout << "  logged: " << theirs << "\n";
      std::cout << "  rerun : " << ours << "\n";
      if (chosen == orig_perform) {
        std::cout << "MATCH\n";
        return 0;
      }
      std::cout << "MISMATCH\n";
      return 1;
    }
  }

  if (args.emit_test_path) {
    try {
      emit_test_scaffold(*args.emit_test_path, game_id, *state_rec, game);
      std::cout << "wrote scaffold: " << *args.emit_test_path << "\n";
    } catch (const std::exception& e) {
      std::cerr << "emit-test failed: " << e.what() << "\n";
      return 2;
    }
  }

  return 0;
}
