// Port of python-bot/src/hanabi_bot/conventions/reactor/reactive_table.py.
// Per-variant reactive-value table for color clues in rainbow-ish variants.
#pragma once

#include <array>
#include <string>
#include <vector>

namespace hanabi {
struct Variant;
}  // namespace hanabi

namespace hanabi::reactor {

inline constexpr std::array<const char*, 6> kVanillaOrder = {
    "Red", "Yellow", "Green", "Blue", "Purple", "Teal"};

// For each colourable suit in stack order, the reactive value (1..hand_size).
// Result is cached per (Variant*, hand_size).
std::vector<int> reactive_value_table(const Variant& variant, int hand_size);

// Format the variant's reactive-clue table for the /settings chat command.
std::string format_reactive_settings(const Variant& variant, int hand_size);

}  // namespace hanabi::reactor
