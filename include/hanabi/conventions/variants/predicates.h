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

}  // namespace hanabi::reactor::variants
