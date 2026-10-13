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

bool epoch_shift(const Variant& variant) {
  if (!variant.throw_it_in_a_hole) return false;
  const int n = non_inverted_count(variant);
  return n == 5 || n == 6;
}

int bucket_count(const Variant& variant) { return non_inverted_count(variant); }

std::vector<std::vector<int>> variant_buckets(const Variant& variant) {
  std::vector<std::vector<int>> out;
  for (int s : bucketed_suits(variant)) out.push_back({s});
  return out;
}

std::optional<int> bucket_of(const Variant& variant, int suit_index) {
  if (suit_index < 0 || suit_index >= static_cast<int>(variant.suits.size())) {
    return std::nullopt;
  }
  if (variant.suits[suit_index].suit_type.inverted) return std::nullopt;
  // Its position among the non-inverted suits.
  int position = 0;
  for (int i = 0; i < suit_index; ++i) {
    if (!variant.suits[i].suit_type.inverted) ++position;
  }
  return position;
}

int bucket_shift(const Variant& variant, ClueKind kind, int clue_turn) {
  int k = 1;
  if (epoch_shift(variant)) {
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

bool high_score_phase(const State& state, int clue_turn) {
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
