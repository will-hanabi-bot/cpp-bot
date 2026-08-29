#include "hanabi/conventions/reactor0/reactive_assignment.h"

#include <algorithm>
#include <cctype>

#include "hanabi/conventions/reactor0/colour_value.h"
#include "hanabi/conventions/variants/predicates.h"

namespace hanabi::reactor0 {
namespace {

std::string lower(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

}  // namespace

ReactiveAssignment reactive_assignment(
    const Variant& variant, const std::vector<ReactiveOverride>& overrides,
    ClueKind kind, int clue_value) {
  for (const ReactiveOverride& o : overrides) {
    if (o.kind == kind && o.clue_value == clue_value) {
      return ReactiveAssignment{o.even, o.reactive_value};
    }
  }
  const bool even = hanabi::reactor::variants::uses_even_parity(variant, kind);
  const int value =
      kind == ClueKind::RANK
          ? hanabi::reactor::variants::rank_reactive_value(variant, clue_value)
          : colour_clue_value(variant, clue_value);
  return ReactiveAssignment{even, value};
}

ReactiveAssignment reactive_assignment_for(
    const Variant& variant, const std::vector<ReactiveOverride>& overrides,
    ClueKind kind, int clue_value, bool target_is_bob) {
  ReactiveAssignment a = reactive_assignment(variant, overrides, kind, clue_value);
  if (hanabi::reactor::variants::uses_target_parity(variant)) {
    // The value -- including a `/set` override of it -- stands. Only the bucket
    // moves, because in these variants the giver cannot choose the kind that
    // would otherwise have carried it.
    a.even = !target_is_bob;
  }
  return a;
}

std::string clue_label(const Variant& variant, ClueKind kind, int clue_value) {
  if (kind == ClueKind::COLOUR) {
    if (clue_value >= 0 &&
        clue_value < static_cast<int>(variant.clue_colour_names.size())) {
      return variant.clue_colour_names[clue_value];
    }
    return "?";
  }
  // Under Odds and Evens the clue value names a parity class, not a rank, so
  // "1" and "2" would be actively misleading in the table a partner reads.
  if (variant.odds_and_evens) {
    if (clue_value == 1) return "Odd";
    if (clue_value == 2) return "Even";
  }
  return std::to_string(clue_value);
}

std::optional<std::pair<ClueKind, int>> parse_clue_label(const Variant& variant,
                                                          const std::string& text) {
  const std::string want = lower(text);
  for (int r : variant.clue_ranks) {
    if (lower(clue_label(variant, ClueKind::RANK, r)) == want) {
      return std::make_pair(ClueKind::RANK, r);
    }
  }
  for (int i = 0; i < static_cast<int>(variant.clue_colour_names.size()); ++i) {
    if (lower(clue_label(variant, ClueKind::COLOUR, i)) == want) {
      return std::make_pair(ClueKind::COLOUR, i);
    }
  }
  // Under Odds and Evens the raw rank digits still name the two clues, even
  // though the table shows them as Odd / Even.
  if (variant.odds_and_evens) {
    if (want == "1") return std::make_pair(ClueKind::RANK, 1);
    if (want == "2") return std::make_pair(ClueKind::RANK, 2);
  }
  return std::nullopt;
}

ReactiveTable build_reactive_table(
    const Variant& variant, const std::vector<ReactiveOverride>& overrides) {
  ReactiveTable out;
  auto add = [&](ClueKind kind, int clue_value) {
    const ReactiveAssignment a =
        reactive_assignment(variant, overrides, kind, clue_value);
    ReactiveRow row{clue_label(variant, kind, clue_value), a.value};
    (a.even ? out.even : out.odd).push_back(std::move(row));
  };
  for (int r : variant.clue_ranks) add(ClueKind::RANK, r);
  for (int i = 0; i < static_cast<int>(variant.clue_colour_names.size()); ++i) {
    add(ClueKind::COLOUR, i);
  }
  return out;
}

std::string format_settings(const Variant& variant,
                             const std::vector<ReactiveOverride>& overrides,
                             bool rlocks) {
  const ReactiveTable table = build_reactive_table(variant, overrides);
  auto render = [](const std::vector<ReactiveRow>& rows) {
    std::string out = "{";
    for (size_t i = 0; i < rows.size(); ++i) {
      if (i) out += ", ";
      out += rows[i].label + "=" + std::to_string(rows[i].value);
    }
    return out + "}";
  };
  // A target-parity variant has no per-clue bucket to report: the parity comes
  // from who is clued, so splitting the table into even/odd would describe a
  // rule this game does not use. Print the one thing that still varies per
  // clue -- its value -- and state the rule that replaces the split.
  if (hanabi::reactor::variants::uses_target_parity(variant)) {
    std::vector<ReactiveRow> all = table.even;
    all.insert(all.end(), table.odd.begin(), table.odd.end());
    // The 60% switch belongs here too: a human reading /settings needs to know
    // that a clue to Bob stops being reactive partway through the game, and in
    // Synesthesia what it becomes instead.
    const std::string late =
        variant.synesthesia
            ? ", from 60% of max OR at 8 clues a clue to Bob is STABLE: "
              "Red=pitch1, Yellow=pitch2, Green=chuck3, Blue=chuck2, "
              "Purple=pitch5, Orange=chuck1, other=pitch4"
            : ", from 60% of max OR at 8 clues a clue to Bob is STABLE";
    return "reactor0 — no stable clues below 60%: to Bob = odd, to Cathy = even"
           ", reactive values: " +
           render(all) + late + ", rlocks: " + (rlocks ? "on" : "off");
  }
  return "reactor0 — even reactive values: " + render(table.even) +
         ", odd reactive values: " + render(table.odd) +
         ", rlocks: " + (rlocks ? "on" : "off");
}

}  // namespace hanabi::reactor0
