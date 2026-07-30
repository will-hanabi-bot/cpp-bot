#include "hanabi/basics/convention.h"

#include <cctype>
#include <string>

namespace hanabi {

std::string_view convention_name(Convention c) {
  switch (c) {
    case Convention::REACTOR: return "reactor";
    case Convention::REACTOR0: return "reactor0";
  }
  return "reactor";
}

std::optional<Convention> parse_convention(std::string_view s) {
  std::string lower;
  lower.reserve(s.size());
  for (char c : s) {
    lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  if (lower == "reactor" || lower == "reactor1") return Convention::REACTOR;
  if (lower == "reactor0") return Convention::REACTOR0;
  return std::nullopt;
}

}  // namespace hanabi
