// Reactor0's reactive table: which PARITY BUCKET each clue belongs to, and
// what reactive value (anchor) it carries there.
//
// Every clue sits in one of two buckets:
//
//   even -- a double play, or a double discard
//   odd  -- exactly one play
//
// Normally rank clues are the even bucket and colour clues the odd; Odds and
// Evens swaps them (`variants::uses_even_parity`). A clue's value within its
// bucket is the anchor the react/target slot arithmetic keys on: for a rank
// clue the rank itself (odd->3 / even->4 under Odds and Evens,
// `variants::rank_reactive_value`), for a colour clue the fixed assignment in
// `colour_value.h`.
//
// `/set` overrides a single clue's bucket and value. An EMPTY override list
// reproduces the built-in table exactly, which is the state of every game that
// has not used the command.
#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/variant.h"

namespace hanabi::reactor0 {

struct ReactiveAssignment {
  bool even = false;  // which parity bucket
  int value = 1;      // the anchor within it
};

// The assignment for one clue. An override wins outright; otherwise the
// variant decides.
//
// TARGET-BLIND, so it is the wrong function to ask about a clue that has
// actually been given -- see `reactive_assignment_for`. It remains correct for
// the `/settings` table, which describes clues in the abstract.
ReactiveAssignment reactive_assignment(
    const Variant& variant, const std::vector<ReactiveOverride>& overrides,
    ClueKind kind, int clue_value);

// The assignment for a clue GIVEN TO SOMEBODY. Identical to the above except in
// a `variants::uses_target_parity` variant, where the parity comes from who was
// clued rather than from the clue's kind: Bob -> odd, Cathy -> even. The VALUE
// is the same either way, `/set` included.
//
// Every site that asks a real clue or a waiting connection for its parity must
// use this one. `wc.clue.target` is the seat that was clued -- note this is NOT
// the receiver in a target-parity variant, where the receiver is always Cathy.
ReactiveAssignment reactive_assignment_for(
    const Variant& variant, const std::vector<ReactiveOverride>& overrides,
    ClueKind kind, int clue_value, bool target_is_bob);

// One row of the /settings table: the label a partner would say, and the value.
struct ReactiveRow {
  std::string label;
  int value = 1;
};

// The whole table, split into its two buckets.
//
// Rows are enumerated rank-first (in `Variant::clue_ranks` order, so Odds and
// Evens yields two and the Number Mute family none) then colour (in
// `clue_colour_names` order), and each lands in its assigned bucket preserving
// that global order. A clue moved by `/set` therefore appears at the END of its
// new bucket and vanishes from the other.
//
// TARGET-BLIND, like `reactive_assignment` which it calls. In a
// `uses_target_parity` variant the split it reports is not the one the game
// plays by -- `format_settings` concatenates the two buckets there and prints
// the target rule instead of a split.
struct ReactiveTable {
  std::vector<ReactiveRow> even;
  std::vector<ReactiveRow> odd;
};

ReactiveTable build_reactive_table(const Variant& variant,
                                    const std::vector<ReactiveOverride>& overrides);

// The label for a clue: `clue_colour_names[i]` for a colour, the rank digit for
// a rank -- or "Odd" / "Even" under Odds and Evens, where the value names a
// parity class rather than a rank.
std::string clue_label(const Variant& variant, ClueKind kind, int clue_value);

// Parse the `{clue}` argument of `/set` -- a colour name or a rank, matched
// case-insensitively against `clue_label`. Nullopt when it names nothing.
std::optional<std::pair<ClueKind, int>> parse_clue_label(const Variant& variant,
                                                          const std::string& text);

// The `/settings` line for a reactor0 table.
std::string format_settings(const Variant& variant,
                             const std::vector<ReactiveOverride>& overrides,
                             bool rlocks);

}  // namespace hanabi::reactor0
