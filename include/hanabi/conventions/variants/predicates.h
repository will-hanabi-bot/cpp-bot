// Variant-family detection predicates used to route convention
// interpretation. Substring predicates matching the Python's regex
// variants. Each name set here is the regex's alternation expanded as
// individual substrings.
#pragma once

#include "hanabi/basics/clue.h"

namespace hanabi {
struct State;
struct Variant;
}  // namespace hanabi

namespace hanabi::reactor::variants {

bool includes_rainbowish(const State& state);
bool includes_pinkish(const State& state);
bool includes_brownish(const State& state);

// True when any suit in the variant is inverted (Orange / Dark Orange),
// i.e. the play and discard buttons swap for that suit. Unlike the three
// above this reads the real `SuitType::inverted` flag rather than matching
// suit names, since `SuitType::of_name` already owns that classification.
bool includes_inverted(const State& state);

// True when an inverted suit in this variant is also DARK (Dark Orange).
// Dark suits have a single copy of every rank (src/basics/variant.cpp:183), so
// every Dark Orange card is critical and pitching one is an unrecoverable
// loss — which is why the convention chucks rather than pitches there.
bool includes_dark_inverted(const State& state);

// --- Odds and Evens -------------------------------------------------------
//
// One definition of the swap, so every reactive site reads the same rule.

// Which parity ruleset a clue carries. Normally a RANK clue is the EVEN-parity
// family (zero or two plays: double play, or double discard) and a COLOUR clue
// the ODD one (exactly one play). Odds and Evens swaps the two: there a colour
// clue is the double play / double discard and a rank clue is the one-play
// reactive.
bool uses_even_parity(const Variant& variant, ClueKind kind);

// A rank clue's reactive value -- its anchor. Normally the clue value IS the
// rank, so it doubles as the anchor. Under Odds and Evens the value names a
// parity rather than a rank, so it maps: odd (1) -> 3, even (2) -> 4.
int rank_reactive_value(const Variant& variant, int clue_value);

}  // namespace hanabi::reactor::variants
