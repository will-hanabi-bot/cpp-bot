// self_play: run N seeded self-play games and report the issues the detectors
// find. See self_play_diagnostics/README.md.
//
//   build/self_play.exe --seeds 1..100 --jobs 12 \
//       --out self_play_diagnostics/runs/v18.2.0 \
//       --report self_play_diagnostics/results/v18.2.0.md

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#ifdef _WIN32
#include <windows.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "diagnostics.h"
#include "hanabi/version.h"
#include "sim.h"

using nlohmann::json;
using namespace hanabi::selfplay;

namespace {

// The seed each worker thread is playing, so a crash can say which game it was in.
thread_local std::uint64_t g_current_seed = 0;

void on_crash(int sig) {
  std::fprintf(stderr, "\nCRASH (signal %d) while playing seed %llu\n", sig,
               static_cast<unsigned long long>(g_current_seed));
#ifdef _WIN32
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr));
  void* frames[24];
  const USHORT n = CaptureStackBackTrace(0, 24, frames, nullptr);
  for (USHORT k = 0; k < n; ++k) {
    std::fprintf(stderr, "  frame %u: image offset 0x%llx\n", k,
                 static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(frames[k]) - base));
  }
#endif
  std::fflush(stderr);
  std::_Exit(128 + sig);
}

#ifdef _WIN32
// An access violation on a worker thread never reaches `signal()` on Windows; the
// unhandled-exception filter runs on the faulting thread, so it can name the seed.
LONG WINAPI on_unhandled(EXCEPTION_POINTERS* info) {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr));
  const auto at = reinterpret_cast<std::uintptr_t>(info->ExceptionRecord->ExceptionAddress);
  std::fprintf(stderr,
               "\nCRASH (exception 0x%lx at %p, image offset 0x%llx) while playing seed %llu\n",
               static_cast<unsigned long>(info->ExceptionRecord->ExceptionCode),
               info->ExceptionRecord->ExceptionAddress,
               static_cast<unsigned long long>(at - base),
               static_cast<unsigned long long>(g_current_seed));
  // A few return addresses up the faulting thread's stack, as image offsets.
  void* frames[16];
  const USHORT n = CaptureStackBackTrace(0, 16, frames, nullptr);
  for (USHORT k = 0; k < n; ++k) {
    std::fprintf(stderr, "  frame %u: image offset 0x%llx\n", k,
                 static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(frames[k]) - base));
  }
  std::fflush(stderr);
  return EXCEPTION_EXECUTE_HANDLER;
}
#endif

struct Args {
  std::uint64_t seed_from = 1;
  std::uint64_t seed_to = 100;
  int jobs = 0;
  std::string out;
  std::string report;
  std::string log_dir = "logs";
  std::string variant = "Throw It in a Hole (5 Suits)";
  int players = 3;
  double endgame_timeout = 6.0;
};

void usage() {
  std::cerr << "usage: self_play [--seeds A..B] [--jobs N] [--out DIR] [--report FILE.md]\n"
               "                 [--log-dir DIR|''] [--variant NAME] [--players N]\n";
}

bool parse(int argc, char** argv, Args& a) {
  for (int i = 1; i < argc; ++i) {
    const std::string k = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) throw std::runtime_error("missing value for " + k);
      return argv[++i];
    };
    if (k == "--seeds") {
      const std::string v = next();
      const auto dots = v.find("..");
      if (dots == std::string::npos) {
        a.seed_from = a.seed_to = std::stoull(v);
      } else {
        a.seed_from = std::stoull(v.substr(0, dots));
        a.seed_to = std::stoull(v.substr(dots + 2));
      }
    } else if (k == "--jobs") {
      a.jobs = std::stoi(next());
    } else if (k == "--out") {
      a.out = next();
    } else if (k == "--report") {
      a.report = next();
    } else if (k == "--log-dir") {
      a.log_dir = next();
    } else if (k == "--variant") {
      a.variant = next();
    } else if (k == "--players") {
      a.players = std::stoi(next());
    } else if (k == "--endgame-timeout") {
      a.endgame_timeout = std::stod(next());
    } else {
      return false;
    }
  }
  return a.seed_from <= a.seed_to;
}

const char* end_name(EndCondition e) {
  switch (e) {
    case EndCondition::NORMAL: return "normal";
    case EndCondition::STRIKEOUT: return "strikeout";
    case EndCondition::TERMINATED: return "terminated";
  }
  return "?";
}

struct GameRecord {
  SimResult result;
  std::vector<Issue> issues;
  double seconds = 0;
};

std::string md_escape(std::string s) {
  std::string out;
  for (char c : s) {
    if (c == '|') out += "\\|";
    else if (c == '\n') out += ' ';
    else out += c;
  }
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  std::signal(SIGSEGV, on_crash);
#ifdef _WIN32
  SetUnhandledExceptionFilter(on_unhandled);
#endif
  std::signal(SIGABRT, on_crash);
  Args args;
  try {
    if (!parse(argc, argv, args)) {
      usage();
      return 2;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    usage();
    return 2;
  }
  if (args.jobs <= 0) {
    args.jobs = std::max(1u, std::thread::hardware_concurrency() / 2);
  }
  if (!args.log_dir.empty()) std::filesystem::create_directories(args.log_dir);

  std::vector<std::uint64_t> seeds;
  for (auto s = args.seed_from; s <= args.seed_to; ++s) seeds.push_back(s);
  std::vector<GameRecord> records(seeds.size());
  std::atomic<std::size_t> next{0};
  std::atomic<int> done{0};
  std::mutex io;
  const auto t0 = std::chrono::steady_clock::now();

  auto worker = [&]() {
    for (;;) {
      const std::size_t i = next.fetch_add(1);
      if (i >= seeds.size()) return;
      const auto g0 = std::chrono::steady_clock::now();
      SimConfig cfg;
      cfg.variant = args.variant;
      cfg.num_players = args.players;
      cfg.seed = seeds[i];
      cfg.game_id = 9000000 + static_cast<int>(seeds[i]);
      cfg.log_dir = args.log_dir;
      cfg.endgame_timeout = args.endgame_timeout;
      GameRecord rec;
      try {
        Sim sim(cfg);
        Diagnostics diag(sim);
        g_current_seed = cfg.seed;
        rec.result = sim.run(diag.hooks());
        diag.finish(sim, rec.result);
        rec.issues = diag.issues();
      } catch (const std::exception& e) {
        rec.result.seed = cfg.seed;
        rec.result.game_id = cfg.game_id;
        rec.result.error = SimError{"crash", 0, -1, e.what()};
        rec.issues.push_back(Issue{"crash", "error", 0, -1, -1, json{{"what", e.what()}}});
      }
      rec.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - g0).count();
      const int n = ++done;
      {
        std::lock_guard<std::mutex> lk(io);
        int crit = 0;
        for (const auto& is : rec.issues) crit += is.critical();
        std::fprintf(stderr, "[%3d/%zu] seed %llu: %2d/%d strikes=%d turns=%d %s critical=%d (%.1fs)\n",
                     n, seeds.size(), static_cast<unsigned long long>(cfg.seed),
                     rec.result.score, rec.result.max_score, rec.result.strikes,
                     rec.result.turns, end_name(rec.result.end), crit, rec.seconds);
      }
      records[i] = std::move(rec);
    }
  };
  std::vector<std::thread> pool;
  for (int j = 0; j < args.jobs; ++j) pool.emplace_back(worker);
  for (auto& th : pool) th.join();
  const double wall =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

  // --- Raw output ------------------------------------------------------------
  if (!args.out.empty()) {
    std::filesystem::create_directories(args.out);
    std::ofstream games(args.out + "/games.jsonl");
    std::ofstream issues(args.out + "/issues.jsonl");
    for (const auto& r : records) {
      json g{{"seed", r.result.seed},
             {"game_id", r.result.game_id},
             {"score", r.result.score},
             {"max_score", r.result.max_score},
             {"reachable", r.result.reachable},
             {"clues", r.result.clues},
             {"plays", r.result.plays},
             {"misses", r.result.misses},
             {"discards", r.result.discards},
             {"strikes", r.result.strikes},
             {"turns", r.result.turns},
             {"end", end_name(r.result.end)},
             {"seconds", r.seconds}};
      if (r.result.error) g["error"] = r.result.error->what;
      games << g.dump() << "\n";
      for (const auto& is : r.issues) {
        issues << json{{"seed", r.result.seed},
                       {"game_id", r.result.game_id},
                       {"class", is.cls},
                       {"kind", is.kind},
                       {"turn", is.turn},
                       {"seat", is.seat},
                       {"order", is.order},
                       {"detail", is.detail}}
                      .dump()
               << "\n";
      }
    }
  }

  // --- Summary -----------------------------------------------------------------
  std::map<int, int> hist;
  int perfect = 0, strikeouts = 0, errors = 0;
  double total = 0;
  for (const auto& r : records) {
    ++hist[r.result.score];
    total += r.result.score;
    if (r.result.score == r.result.max_score) ++perfect;
    if (r.result.end == EndCondition::STRIKEOUT) ++strikeouts;
    if (r.result.error) ++errors;
  }
  // class -> kind -> (occurrence count, games)
  struct Agg {
    int count = 0;
    std::set<std::uint64_t> games;
    std::vector<std::pair<std::uint64_t, const Issue*>> examples;
  };
  std::map<std::string, std::map<std::string, Agg>> agg;
  int critical_total = 0;
  std::set<std::uint64_t> critical_games;
  long div_turns = 0, div_common = 0, div_pair = 0;
  for (const auto& r : records) {
    for (const auto& is : r.issues) {
      if (is.cls != "stat") continue;
      div_turns += is.detail.value("turns", 0);
      div_common += is.detail.value("common_turns", 0);
      div_pair += is.detail.value("pair_turns", 0);
    }
  }
  for (const auto& r : records) {
    for (const auto& is : r.issues) {
      Agg& a = agg[is.cls][is.kind];
      ++a.count;
      a.games.insert(r.result.seed);
      if (a.examples.size() < 6) a.examples.emplace_back(r.result.seed, &is);
      if (is.critical()) {
        ++critical_total;
        critical_games.insert(r.result.seed);
      }
    }
  }

  // Wrong inferences: how many cards ever carry one at some seat, and where each
  // card's first one came from (the `origin` every class-1 issue is stamped with).
  auto origin_key = [](const Issue& is) {
    const json o = is.detail.value("origin", json::object());
    if (o.empty()) return std::string("?");
    std::string k = o.value("action", std::string("?"));
    if (k == "clue") k += o.value("stable", false) ? " (stable)" : " (reactive)";
    k += ", wrong at " + o.value("role", std::string("?"));
    if (o.value("disagree", false)) k += ", seats disagree";
    if (o.contains("same_card")) {
      k += o.value("same_card", false) ? ", the card itself" : ", another card";
    }
    return k;
  };
  std::map<std::pair<std::uint64_t, int>, const Issue*> first_wrong;
  std::map<std::string, int> wrong_events_by_origin;
  int wrong_events = 0;
  for (const auto& r : records) {
    for (const auto& is : r.issues) {
      if (is.cls != "1") continue;
      ++wrong_events;
      ++wrong_events_by_origin[origin_key(is)];
      const auto key = std::make_pair(r.result.seed, is.order);
      auto it = first_wrong.find(key);
      if (it == first_wrong.end() || is.turn < it->second->turn) first_wrong[key] = &is;
    }
  }
  std::map<std::string, int> wrong_cards_by_origin;
  for (const auto& [key, is] : first_wrong) ++wrong_cards_by_origin[origin_key(*is)];

  std::ostringstream md;
  md << "# Self-play results — " << hanabi::kBotVersion << "\n\n";
  md << "- Variant: " << args.variant << ", " << args.players << " players, seeds "
     << args.seed_from << ".." << args.seed_to << " (" << records.size() << " games)\n";
  md << "- **25/25 (max score): " << perfect << " / " << records.size() << "**\n";
  md << "- Mean score: " << (records.empty() ? 0.0 : total / records.size())
     << "; strikeouts: " << strikeouts << "; harness errors/crashes: " << errors << "\n";
  md << "- Critical issues (classes 1-4): " << critical_total << " in "
     << critical_games.size() << " games\n";
  md << "- Wrong inferences (class 1): " << wrong_events << " events; "
     << first_wrong.size() << " cards ever read wrongly ("
     << (records.empty() ? 0.0 : 100.0 * first_wrong.size() / records.size())
     << " per 100 games)\n";
  if (div_turns > 0) {
    md << "- Seats disagree after " << (100.0 * div_common / div_turns)
       << "% of actions on the common view, " << (100.0 * div_pair / div_turns)
       << "% on some pair view\n";
  }
  md << "- Wall time: " << static_cast<int>(wall) << " s with " << args.jobs << " jobs\n\n";
  md << "## Score histogram\n\n| score | games |\n|---|---|\n";
  for (auto it = hist.rbegin(); it != hist.rend(); ++it) {
    md << "| " << it->first << " | " << it->second << " |\n";
  }
  md << "\n## Where wrong inferences come from\n\n"
        "Each card's FIRST wrong inference, and every class-1 event, by the action that "
        "produced it and the part the wrong seat played in it.\n\n"
        "| origin | cards (first) | events |\n|---|---|---|\n";
  {
    std::vector<std::pair<int, std::string>> rows;
    for (const auto& [k, n] : wrong_cards_by_origin) rows.emplace_back(n, k);
    for (const auto& [k, n] : wrong_events_by_origin) {
      if (!wrong_cards_by_origin.count(k)) rows.emplace_back(0, k);
    }
    std::sort(rows.rbegin(), rows.rend());
    for (const auto& [n, k] : rows) {
      md << "| " << k << " | " << n << " | " << wrong_events_by_origin[k] << " |\n";
    }
  }
  md << "\n## Issues by class\n\n| class | kind | occurrences | games |\n|---|---|---|---|\n";
  for (const auto& [cls, kinds] : agg) {
    for (const auto& [kind, a] : kinds) {
      md << "| " << cls << " | " << kind << " | " << a.count << " | " << a.games.size()
         << " |\n";
    }
  }
  md << "\n## Examples\n";
  for (const auto& [cls, kinds] : agg) {
    for (const auto& [kind, a] : kinds) {
      md << "\n### " << cls << " / " << kind << "\n\n";
      for (const auto& [seed, is] : a.examples) {
        json d = is->detail;
        d.erase("replay");
        md << "- seed " << seed << " (game " << 9000000 + seed << ") turn " << is->turn
           << " seat " << is->seat << " order " << is->order << ": `" << md_escape(d.dump())
           << "`\n";
      }
    }
  }
  const std::string text = md.str();
  if (!args.report.empty()) {
    std::filesystem::path rp(args.report);
    if (rp.has_parent_path()) std::filesystem::create_directories(rp.parent_path());
    std::ofstream(args.report) << text;
  }
  std::cout << text;
  return 0;
}
