// Reactor0's fixed colour-value assignment. Reactive clues in reactor0 are
// anchored on the CLUE VALUE (react_slot + target_slot ≡ value, mod hand
// size); a rank clue's value is the rank, and a colour clue's value comes
// from this table, keyed on the clue colour NAME
// (Variant::clue_colour_names) so Ambiguous variants resolve by the colour
// a partner actually says.
//
// Assignment rules (values may collide — Teal duplicates Red by design):
//   1. Fixed for present colours: Red=1, Yellow=2, Green=3, Blue=4,
//      Purple=5, Teal=1.
//   2. Black, then Pink, then Brown: the first value not already taken, in
//      preference order {4, 3, 5, 2, 1}; if all are taken, 1.
//   3. Orange: first untaken from {2, 5, 4, 3, 1}; if all taken, 2.
//   4. Any other colour name: assigned after Brown via rule 2's list, in
//      clue_colour_names order. (Realistic variants only produce the ten
//      names above — whitish / rainbowish / prism colours never enter
//      clue_colour_names.)
// Worked example: Red/Blue/Brown/Orange → Red=1, Blue=4, Brown=3, Orange=2.
#pragma once

#include <vector>

#include "hanabi/basics/variant.h"

namespace hanabi::reactor0 {

// Value (1..5) for the colour clue with this index into clue_colour_names.
int colour_clue_value(const Variant& variant, int colour_index);

// The whole table, aligned with Variant::clue_colour_names.
std::vector<int> colour_value_table(const Variant& variant);

}  // namespace hanabi::reactor0
