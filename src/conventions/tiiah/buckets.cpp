#include "hanabi/conventions/tiiah/buckets.h"

#include "hanabi/basics/variant.h"

namespace hanabi::tiiah {

std::array<std::vector<int>, 3> suit_buckets(const Variant& variant) {
  // Inverted suits are dropped before anything is counted, so what follows is
  // indexed by POSITION AMONG THE REMAINING SUITS, not by suit index.
  std::vector<int> remaining;
  remaining.reserve(variant.suits.size());
  for (size_t i = 0; i < variant.suits.size(); ++i) {
    if (variant.suits[i].suit_type.inverted) continue;
    remaining.push_back(static_cast<int>(i));
  }

  // Each row says which re-indexed positions each bucket covers.
  static const std::array<std::vector<int>, 3> kEmpty{};
  std::array<std::vector<int>, 3> pattern;
  switch (remaining.size()) {
    case 3: pattern = {std::vector<int>{0}, {1}, {2}}; break;
    case 4: pattern = {std::vector<int>{0, 1}, {2}, {3}}; break;
    case 5: pattern = {std::vector<int>{0, 1}, {2, 3}, {4}}; break;
    case 6: pattern = {std::vector<int>{0, 1}, {2, 3}, {4, 5}}; break;
    default: return kEmpty;
  }

  std::array<std::vector<int>, 3> out;
  for (size_t b = 0; b < out.size(); ++b) {
    out[b].reserve(pattern[b].size());
    for (int position : pattern[b]) out[b].push_back(remaining[position]);
  }
  return out;
}

std::optional<int> bucket_of(const Variant& variant, int suit_index) {
  if (suit_index < 0 || suit_index >= static_cast<int>(variant.suits.size())) {
    return std::nullopt;
  }
  if (variant.suits[suit_index].suit_type.inverted) return std::nullopt;
  const auto buckets = suit_buckets(variant);
  for (size_t b = 0; b < buckets.size(); ++b) {
    for (int s : buckets[b]) {
      if (s == suit_index) return static_cast<int>(b);
    }
  }
  return std::nullopt;
}

}  // namespace hanabi::tiiah
