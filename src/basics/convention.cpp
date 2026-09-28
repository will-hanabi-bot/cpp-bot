#include "hanabi/basics/convention.h"

#include <cctype>
#include <string>

namespace hanabi {

std::string_view convention_name(Convention c) {
  switch (c) {
    case Convention::REACTOR: return "reactor";
    case Convention::REACTOR0: return "reactor0";
    case Convention::TIIAH: return "tiiah";
  }
  return "reactor";
}

bool is_reactor0_family(Convention c) {
  return c == Convention::REACTOR0 || c == Convention::TIIAH;
}

bool uses_reactor0_decisions(Convention c) { return is_reactor0_family(c); }

Convention resolve_table_convention(bool throw_it_in_a_hole, int num_players,
                                    Convention mode) {
  if (throw_it_in_a_hole) return Convention::TIIAH;
  return (mode == Convention::REACTOR0 && num_players == 3) ? Convention::REACTOR0
                                                            : Convention::REACTOR;
}

std::optional<Convention> parse_convention(std::string_view s) {
  std::string lower;
  lower.reserve(s.size());
  for (char c : s) {
    lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  if (lower == "reactor" || lower == "reactor1") return Convention::REACTOR;
  if (lower == "reactor0") return Convention::REACTOR0;
  // Accepted so a TIIAH snapshot round-trips (`state_snapshot.cpp`), NOT so a
  // human can select it: `/setall tiiah` is refused in `chat_setall`, because
  // the convention is variant-derived and a stray chat line must not make the
  // bot stand down at every ordinary table.
  if (lower == "tiiah") return Convention::TIIAH;
  return std::nullopt;
}

}  // namespace hanabi
