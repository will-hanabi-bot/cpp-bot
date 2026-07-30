#include "hanabi/conventions/reactor0/colour_value.h"

#include <array>
#include <mutex>
#include <string>
#include <unordered_map>

namespace hanabi::reactor0 {

namespace {

// Cache keyed by variant NAME (same rationale as reactor's reactive_table:
// Variant addresses are stable, but keying by name is proof against any
// future reload of the catalog).
std::unordered_map<std::string, std::vector<int>> g_cache;
std::mutex g_cache_mu;

std::vector<int> build_table(const Variant& variant) {
  const auto& names = variant.clue_colour_names;
  std::vector<int> values(names.size(), 0);
  std::array<bool, 6> taken{};  // 1-indexed; taken[v] for v in 1..5.

  auto fixed_value = [](const std::string& name) -> int {
    if (name == "Red") return 1;
    if (name == "Yellow") return 2;
    if (name == "Green") return 3;
    if (name == "Blue") return 4;
    if (name == "Purple") return 5;
    if (name == "Teal") return 1;
    return 0;
  };

  // Rule 1: fixed assignments; every present fixed colour marks its value
  // taken (Teal marks 1 taken just like Red).
  for (size_t i = 0; i < names.size(); ++i) {
    int v = fixed_value(names[i]);
    if (v != 0) {
      values[i] = v;
      taken[v] = true;
    }
  }

  auto assign_from = [&](int idx, const std::array<int, 5>& prefs,
                          int exhausted) {
    for (int v : prefs) {
      if (!taken[v]) {
        values[idx] = v;
        taken[v] = true;
        return;
      }
    }
    values[idx] = exhausted;  // all taken; do NOT mark (collision allowed).
  };

  constexpr std::array<int, 5> kDarkPrefs{4, 3, 5, 2, 1};
  constexpr std::array<int, 5> kOrangePrefs{2, 5, 4, 3, 1};

  // Rule 2: Black, then Pink, then Brown — the assignment PRIORITY is
  // black > pink > brown regardless of their order in clue_colour_names.
  for (const char* dark : {"Black", "Pink", "Brown"}) {
    for (size_t i = 0; i < names.size(); ++i) {
      if (values[i] == 0 && names[i] == dark) assign_from(static_cast<int>(i), kDarkPrefs, 1);
    }
  }
  // Rule 3: Orange.
  for (size_t i = 0; i < names.size(); ++i) {
    if (values[i] == 0 && names[i] == "Orange") {
      assign_from(static_cast<int>(i), kOrangePrefs, 2);
    }
  }
  // Rule 4: anything else, in clue_colour_names order.
  for (size_t i = 0; i < names.size(); ++i) {
    if (values[i] == 0) assign_from(static_cast<int>(i), kDarkPrefs, 1);
  }
  return values;
}

}  // namespace

std::vector<int> colour_value_table(const Variant& variant) {
  std::lock_guard<std::mutex> lk(g_cache_mu);
  auto it = g_cache.find(variant.name);
  if (it != g_cache.end()) return it->second;
  auto table = build_table(variant);
  g_cache.emplace(variant.name, table);
  return table;
}

std::string format_settings(const Variant& variant, bool rlocks) {
  auto table = colour_value_table(variant);
  std::string out = "reactor0 — colour values: {";
  for (size_t i = 0; i < table.size(); ++i) {
    if (i) out += ", ";
    out += variant.clue_colour_names[i] + "=" + std::to_string(table[i]);
  }
  out += "}, rank value: the rank, rlocks: ";
  out += rlocks ? "on" : "off";
  return out;
}

int colour_clue_value(const Variant& variant, int colour_index) {
  auto table = colour_value_table(variant);
  if (colour_index < 0 || colour_index >= static_cast<int>(table.size())) {
    return 1;
  }
  return table[colour_index];
}

}  // namespace hanabi::reactor0
