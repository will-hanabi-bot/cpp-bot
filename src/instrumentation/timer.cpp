#include "hanabi/instrumentation/timer.h"

#include <algorithm>

namespace hanabi::instr {

namespace {
thread_local Aggregator* g_current = nullptr;
}

Aggregator* current_aggregator() { return g_current; }
void set_current_aggregator(Aggregator* a) { g_current = a; }

void Aggregator::record(std::string_view scope, std::chrono::nanoseconds ns) {
  std::lock_guard<std::mutex> lock(mu_);
  auto& s = data_[std::string(scope)];
  ++s.calls;
  auto v = static_cast<std::uint64_t>(ns.count());
  s.total_ns += v;
  if (v > s.max_ns) s.max_ns = v;
  if (s.samples.size() < kMaxSamplesPerScope) s.samples.push_back(v);
}

std::unordered_map<std::string, ScopeStats> Aggregator::snapshot() const {
  std::lock_guard<std::mutex> lock(mu_);
  return data_;
}

std::unordered_map<std::string, ScopeStats> Aggregator::diff(
    const std::unordered_map<std::string, ScopeStats>& current,
    const std::unordered_map<std::string, ScopeStats>& prior) {
  std::unordered_map<std::string, ScopeStats> out;
  for (const auto& [scope, cur] : current) {
    auto it = prior.find(scope);
    if (it == prior.end()) {
      out[scope] = cur;
      continue;
    }
    const auto& pr = it->second;
    ScopeStats d;
    d.calls = cur.calls - pr.calls;
    d.total_ns = cur.total_ns - pr.total_ns;
    d.max_ns = cur.max_ns;  // can't recover the pre-snapshot max precisely; report current.
    // Samples: we don't track which samples are new vs old; report empty
    // (per-turn p50/p95 isn't critical — per-game aggregate is the goal).
    out[scope] = std::move(d);
  }
  return out;
}

nlohmann::json Aggregator::to_json(
    const std::unordered_map<std::string, ScopeStats>& snap) {
  nlohmann::json out = nlohmann::json::object();
  for (const auto& [scope, stats] : snap) {
    auto& dst = out[scope];
    dst["calls"] = stats.calls;
    dst["total_ns"] = stats.total_ns;
    dst["max_ns"] = stats.max_ns;
    if (!stats.samples.empty()) {
      auto samples = stats.samples;
      std::sort(samples.begin(), samples.end());
      auto pct = [&](double p) {
        auto idx = static_cast<std::size_t>(p * (samples.size() - 1));
        return samples[idx];
      };
      dst["p50_ns"] = pct(0.50);
      dst["p95_ns"] = pct(0.95);
    }
  }
  return out;
}

}  // namespace hanabi::instr
