// Port of python-bot/src/hanabi_bot/basics/identity.py (Identity portion).
// Original Scala: scala-bot/src/scala_bot/basics/Card.scala lines 33-63.
#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace hanabi {

// Max ordinal: 6 suits × 5 ranks = 30 identities.
inline constexpr int kMaxOrd = 30;

struct Identity {
  std::int8_t suit_index;
  std::int8_t rank;

  constexpr Identity() : suit_index(0), rank(0) {}
  constexpr Identity(int s, int r)
      : suit_index(static_cast<std::int8_t>(s)),
        rank(static_cast<std::int8_t>(r)) {}

  constexpr int to_ord() const { return suit_index * 5 + (rank - 1); }

  static constexpr Identity from_ord(int ord) {
    if (ord < 0 || ord >= kMaxOrd) {
      throw std::invalid_argument("Couldn't convert ordinal to identity");
    }
    return Identity(ord / 5, (ord % 5) + 1);
  }

  constexpr std::optional<Identity> prev() const {
    if (rank > 1) return Identity(suit_index, rank - 1);
    return std::nullopt;
  }

  constexpr std::optional<Identity> next() const {
    if (rank < 5) return Identity(suit_index, rank + 1);
    return std::nullopt;
  }

  constexpr bool played_before(Identity other) const {
    return suit_index == other.suit_index && rank < other.rank;
  }

  constexpr auto operator<=>(const Identity&) const = default;
};

}  // namespace hanabi
