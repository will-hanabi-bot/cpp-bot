// Variant-family detection predicates used to route convention
// interpretation. Substring predicates matching the Python's regex
// variants. Each name set here is the regex's alternation expanded as
// individual substrings.
#pragma once

namespace hanabi {
struct State;
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

}  // namespace hanabi::reactor::variants
