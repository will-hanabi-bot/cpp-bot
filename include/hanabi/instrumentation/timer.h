// Scoped timer + per-game aggregator for the iteration-tooling overhaul.
// `ScopedTimer t("foo.bar");` on entry of a function adds the wall time it
// spent to the thread-local Aggregator. The bot publishes a TIMING record
// per-turn (delta since snapshot-at-turn-start) and per-game (totals + p50/
// p95/max + slowest turn) so we can spot which scopes regress when a fix
// lands.
//
// The aggregator is thread-local-bound by GameLogger::set_current() at the
// start of each compute-thread turn, so ScopedTimer calls in convention
// modules transparently route to the current game's aggregator with no
// per-scope plumbing.
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace hanabi::instr {

struct ScopeStats {
  std::uint64_t calls = 0;
  std::uint64_t total_ns = 0;
  std::uint64_t max_ns = 0;
  // Per-call samples retained for percentile estimation. Capped at a small
  // value so a 5000-call recursion doesn't blow memory; over the cap we just
  // accumulate total/max.
  std::vector<std::uint64_t> samples;
};

class Aggregator {
 public:
  static constexpr std::size_t kMaxSamplesPerScope = 1024;

  void record(std::string_view scope, std::chrono::nanoseconds ns);

  // Returns a {scope -> stats} snapshot. Thread-safe.
  std::unordered_map<std::string, ScopeStats> snapshot() const;

  // Compute delta (current - prior) per scope, useful for per-turn TIMING.
  static std::unordered_map<std::string, ScopeStats> diff(
      const std::unordered_map<std::string, ScopeStats>& current,
      const std::unordered_map<std::string, ScopeStats>& prior);

  // Render snapshot to JSON: { scope: {calls, total_ns, max_ns, p50_ns, p95_ns} }.
  static nlohmann::json to_json(const std::unordered_map<std::string, ScopeStats>& snap);

 private:
  mutable std::mutex mu_;
  std::unordered_map<std::string, ScopeStats> data_;
};

// Thread-local current aggregator. Set by GameLogger::set_current() at turn
// boundaries. nullptr means "don't time anything" (e.g. during catchup or
// tests).
Aggregator* current_aggregator();
void set_current_aggregator(Aggregator* a);

class ScopedTimer {
 public:
  explicit ScopedTimer(std::string_view scope)
      : scope_(scope), start_(std::chrono::steady_clock::now()) {}

  ~ScopedTimer() {
    Aggregator* a = current_aggregator();
    if (!a) return;
    auto ns = std::chrono::steady_clock::now() - start_;
    a->record(scope_, ns);
  }

  ScopedTimer(const ScopedTimer&) = delete;
  ScopedTimer& operator=(const ScopedTimer&) = delete;

 private:
  std::string_view scope_;
  std::chrono::steady_clock::time_point start_;
};

}  // namespace hanabi::instr
