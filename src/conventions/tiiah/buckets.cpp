#include "hanabi/conventions/tiiah/buckets.h"

#include <algorithm>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

namespace hanabi::tiiah {

namespace {

// The non-inverted suits, in suit-index order.
std::vector<int> bucketed_suits(const Variant& variant) {
  std::vector<int> remaining;
  remaining.reserve(variant.suits.size());
  for (size_t i = 0; i < variant.suits.size(); ++i) {
    if (variant.suits[i].suit_type.inverted) continue;
    remaining.push_back(static_cast<int>(i));
  }
  return remaining;
}

int non_inverted_count(const Variant& variant) {
  int n = 0;
  for (const auto& suit : variant.suits) n += suit.suit_type.inverted ? 0 : 1;
  return n;
}

}  // namespace

bool single_suit_buckets(const Variant& variant) {
  if (!variant.throw_it_in_a_hole) return false;
  const int n = non_inverted_count(variant);
  return n == 5 || n == 6;
}

int bucket_count(const Variant& variant) {
  return single_suit_buckets(variant) ? non_inverted_count(variant) : 3;
}

std::array<std::vector<int>, 3> suit_buckets(const Variant& variant) {
  // Inverted suits are dropped before anything is counted, so what follows is
  // indexed by POSITION AMONG THE REMAINING SUITS, not by suit index.
  const std::vector<int> remaining = bucketed_suits(variant);

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

std::vector<std::vector<int>> variant_buckets(const Variant& variant) {
  if (single_suit_buckets(variant)) {
    std::vector<std::vector<int>> out;
    for (int s : bucketed_suits(variant)) out.push_back({s});
    return out;
  }
  const auto three = suit_buckets(variant);
  return {three.begin(), three.end()};
}

std::optional<int> bucket_of(const Variant& variant, int suit_index) {
  if (suit_index < 0 || suit_index >= static_cast<int>(variant.suits.size())) {
    return std::nullopt;
  }
  if (variant.suits[suit_index].suit_type.inverted) return std::nullopt;
  if (single_suit_buckets(variant)) {
    // Its position among the non-inverted suits.
    int position = 0;
    for (int i = 0; i < suit_index; ++i) {
      if (!variant.suits[i].suit_type.inverted) ++position;
    }
    return position;
  }
  const auto buckets = suit_buckets(variant);
  for (size_t b = 0; b < buckets.size(); ++b) {
    for (int s : buckets[b]) {
      if (s == suit_index) return static_cast<int>(b);
    }
  }
  return std::nullopt;
}

int bucket_shift(const Variant& variant, ClueKind kind, int clue_turn) {
  int k = 1;
  if (single_suit_buckets(variant)) {
    // ceil(turn / 3), turn 1-based. A turn before the first (a test position set
    // up without any action) is read as the first.
    const int epoch = (std::max(clue_turn, 1) + 2) / 3;
    k = epoch % 2 == 1 ? 1 : 2;
  }
  return kind == ClueKind::RANK ? k : -k;
}

namespace {
int wrap(int b, int n) { return ((b % n) + n) % n; }
}  // namespace

int named_bucket(const Variant& variant, ClueKind kind, int clue_turn, int from) {
  return wrap(from + bucket_shift(variant, kind, clue_turn), bucket_count(variant));
}

int reacter_bucket_for(const Variant& variant, ClueKind kind, int clue_turn,
                       int target_bucket) {
  return wrap(target_bucket - bucket_shift(variant, kind, clue_turn),
              bucket_count(variant));
}

bool late_game(const State& state, int clue_turn) {
  int plays = 0;
  const int turns = std::min(clue_turn, static_cast<int>(state.action_list.size()));
  for (int t = 0; t < turns; ++t) {
    for (const auto& a : state.action_list[t]) {
      if (std::holds_alternative<PlayAction>(a)) ++plays;
    }
  }
  return 3 * plays >= 2 * 5 * static_cast<int>(state.variant->suits.size());
}

}  // namespace hanabi::tiiah
