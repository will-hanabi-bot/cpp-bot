#include "hanabi/conventions/reactor/reactive_table.h"

#include <cassert>
#include <cctype>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "hanabi/basics/variant.h"

namespace hanabi::reactor {
namespace {

bool name_contains(const std::string& name, std::string_view needle) {
  return name.find(needle) != std::string::npos;
}

bool is_rainbowy(const Variant& v) {
  if (v.rainbow_s) return true;
  for (const auto& s : v.suits) {
    if (name_contains(s.name, "Rainbow") || name_contains(s.name, "Omni")) return true;
  }
  return false;
}

bool is_pinkish(const Variant& v) {
  if (v.pink_s) return true;
  for (const auto& s : v.suits) {
    if (name_contains(s.name, "Pink") || name_contains(s.name, "Omni")) return true;
  }
  return false;
}

bool rank_blocked(const Variant& v, int rank) {
  return v.special_rank && *v.special_rank == rank &&
         (v.pink_s || v.brown_s || v.deceptive_s);
}

struct CacheKey {
  const Variant* variant;
  int hand_size;
  bool operator==(const CacheKey& o) const {
    return variant == o.variant && hand_size == o.hand_size;
  }
};

struct CacheKeyHash {
  size_t operator()(const CacheKey& k) const {
    return std::hash<const Variant*>{}(k.variant) ^ (k.hand_size * 0x9e3779b1);
  }
};

}  // namespace

std::vector<int> reactive_value_table(const Variant& variant, int hand_size) {
  static std::unordered_map<CacheKey, std::vector<int>, CacheKeyHash> cache;
  static std::mutex cache_mu;

  {
    std::lock_guard<std::mutex> lock(cache_mu);
    auto it = cache.find({&variant, hand_size});
    if (it != cache.end()) return it->second;
  }

  std::vector<Suit> colourable = variant.colourable_suits();
  bool has_red = false;
  for (const auto& s : colourable) {
    if (s.name == "Red") {
      has_red = true;
      break;
    }
  }
  assert(has_red && "reactive_value_table requires Red in colourable_suits");
  (void)has_red;

  std::unordered_map<std::string, int> vanilla_value;
  for (size_t i = 0; i < kVanillaOrder.size(); ++i) {
    vanilla_value[kVanillaOrder[i]] = static_cast<int>(i % hand_size) + 1;
  }

  std::unordered_set<int> taken;
  int prev_value = 0;
  std::vector<int> result;
  for (const Suit& suit : colourable) {
    int value;
    auto it = vanilla_value.find(suit.name);
    if (it != vanilla_value.end()) {
      value = it->second;
    } else {
      value = 1;
      for (int offset = 1; offset <= hand_size; ++offset) {
        int candidate = ((prev_value - 1 + offset) % hand_size) + 1;
        if (!taken.count(candidate)) {
          value = candidate;
          break;
        }
      }
    }
    taken.insert(value);
    prev_value = value;
    result.push_back(value);
  }

  std::lock_guard<std::mutex> lock(cache_mu);
  cache[{&variant, hand_size}] = result;
  return result;
}

std::string format_reactive_settings(const Variant& variant, int hand_size) {
  std::string odd;
  if (is_rainbowy(variant)) {
    auto table = reactive_value_table(variant, hand_size);
    std::vector<Suit> colourable = variant.colourable_suits();
    std::unordered_map<int, std::string> slot_to_suit;
    for (size_t i = 0; i < colourable.size(); ++i) {
      std::string abbrev;
      if (colourable[i].abbreviation) {
        abbrev = std::string(1, *colourable[i].abbreviation);
      } else {
        abbrev = std::string(1, static_cast<char>(std::tolower(colourable[i].name[0])));
      }
      slot_to_suit[table[i]] = abbrev;
    }
    odd = "{";
    for (int slot = 1; slot <= hand_size; ++slot) {
      if (slot > 1) odd += ", ";
      auto it = slot_to_suit.find(slot);
      odd += (it != slot_to_suit.end()) ? it->second : "-";
    }
    odd += "}";
  } else {
    odd = "{slot focus}";
  }

  std::string even;
  if (is_pinkish(variant)) {
    even = "{";
    for (int rank = 1; rank <= hand_size; ++rank) {
      if (rank > 1) even += ", ";
      even += rank_blocked(variant, rank) ? "-" : std::to_string(rank);
    }
    even += "}";
  } else {
    even = "{slot focus}";
  }

  return "odd plays: " + odd + ", even plays: " + even;
}

}  // namespace hanabi::reactor
