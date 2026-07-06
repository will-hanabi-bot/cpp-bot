#include "hanabi/conventions/variants/reactive_table.h"

#include <cassert>
#include <cctype>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "hanabi/basics/variant.h"

namespace hanabi::reactor::variants {
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

// Cache key: (variant.name, hand_size). The previous keying used the raw
// `const Variant*` pointer, which is unsafe when callers stack-allocate
// Variants: a returning test frees its Variant's stack slot, the next
// test's Variant lands at the same address, and the cache hands back the
// previous test's result. Production variants are loaded once with stable
// unique names, and the reactive_value_table tests already use unique
// short names ("t1"..."trp"), so name-keying is safe in both cases.
struct CacheKey {
  std::string name;
  int hand_size;
  bool operator==(const CacheKey& o) const {
    return name == o.name && hand_size == o.hand_size;
  }
};

struct CacheKeyHash {
  size_t operator()(const CacheKey& k) const {
    return std::hash<std::string>{}(k.name) ^ (k.hand_size * 0x9e3779b1);
  }
};

}  // namespace

std::vector<int> reactive_value_table(const Variant& variant, int hand_size) {
  static std::unordered_map<CacheKey, std::vector<int>, CacheKeyHash> cache;
  static std::mutex cache_mu;

  {
    std::lock_guard<std::mutex> lock(cache_mu);
    auto it = cache.find({variant.name, hand_size});
    if (it != cache.end()) return it->second;
  }

  std::vector<Suit> colourable = variant.colourable_suits();
  assert(!colourable.empty() &&
         "reactive_value_table requires colourable suits");

  std::unordered_map<std::string, int> vanilla_value;
  for (size_t i = 0; i < kVanillaOrder.size(); ++i) {
    vanilla_value[kVanillaOrder[i]] = static_cast<int>(i % hand_size) + 1;
  }

  // Key the vanilla lookup by the CLUE COLOUR name, not the suit name.
  // Ambiguous variants collapse several suits onto one colour and name
  // the representative suits Tomato / Berry / ... — but the clue a
  // partner actually gives is "Red" / "Blue", so the reactive value must
  // anchor to the colour (Ambiguous & Rainbow (5 Suits): {Red, Blue} →
  // {1, 4}, not the positional fallback {1, 2}). `clue_colour_names[i]`
  // is index-aligned with `colourable_suit_indices[i]` by construction.
  // Synthetic test Variants populate `colourable_suit_indices` directly
  // and leave `clue_colour_names` empty — fall back to the suit name,
  // which coincides with the colour for real colour names.
  const bool use_colour_names =
      variant.clue_colour_names.size() == colourable.size();

  std::unordered_set<int> taken;
  int prev_value = 0;
  std::vector<int> result;
  for (size_t idx = 0; idx < colourable.size(); ++idx) {
    const std::string& key = use_colour_names ? variant.clue_colour_names[idx]
                                              : colourable[idx].name;
    int value;
    auto it = vanilla_value.find(key);
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
  cache[{variant.name, hand_size}] = result;
  return result;
}

std::string format_reactive_settings(const Variant& variant, int hand_size,
                                        bool all_plays) {
  // "odd" half = the color-clue slot ruleset (rainbow color → slot, else
  // focus slot). "even" half = the rank-clue slot ruleset (pinkish rank →
  // slot, else focus slot).
  std::string colour_half;
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
    colour_half = "{";
    for (int slot = 1; slot <= hand_size; ++slot) {
      if (slot > 1) colour_half += ", ";
      auto it = slot_to_suit.find(slot);
      colour_half += (it != slot_to_suit.end()) ? it->second : "-";
    }
    colour_half += "}";
  } else {
    colour_half = "{slot focus}";
  }

  std::string rank_half;
  if (is_pinkish(variant)) {
    rank_half = "{";
    for (int rank = 1; rank <= hand_size; ++rank) {
      if (rank > 1) rank_half += ", ";
      rank_half += rank_blocked(variant, rank) ? "-" : std::to_string(rank);
    }
    rank_half += "}";
  } else {
    rank_half = "{slot focus}";
  }

  if (all_plays) {
    // Both halves are now under "even plays:". Collapse a redundant
    // "{slot focus} + {slot focus}" to a single "{slot focus}".
    if (rank_half == colour_half) return "even plays: " + rank_half;
    return "even plays: " + rank_half + " + " + colour_half;
  }
  return "odd plays: " + colour_half + ", even plays: " + rank_half;
}

}  // namespace hanabi::reactor::variants
