// Port of python-bot/src/hanabi_bot/basics/clue.py.
// Original Scala: scala-bot/src/scala_bot/basics/clue.scala.
#pragma once

#include <compare>
#include <cstdint>

namespace hanabi {

enum class ClueKind : std::int8_t {
  COLOUR = 0,
  RANK = 1,
};

struct BaseClue {
  ClueKind kind;
  int value;

  constexpr BaseClue(ClueKind k, int v) : kind(k), value(v) {}

  // Stable small-int identifier. Port of BaseClue.hash.
  constexpr int hash_int() const {
    return (kind == ClueKind::COLOUR ? 0 : 10) + value;
  }

  constexpr auto operator<=>(const BaseClue&) const = default;
};

struct CardClue {
  ClueKind kind;
  int value;
  int giver;
  int turn;

  constexpr CardClue(ClueKind k, int v, int g, int t)
      : kind(k), value(v), giver(g), turn(t) {}

  constexpr BaseClue base() const { return BaseClue{kind, value}; }

  constexpr auto operator<=>(const CardClue&) const = default;
};

struct Clue {
  ClueKind kind;
  int value;
  int target;

  constexpr Clue(ClueKind k, int v, int t) : kind(k), value(v), target(t) {}

  constexpr BaseClue base() const { return BaseClue{kind, value}; }

  constexpr auto operator<=>(const Clue&) const = default;
};

// BaseClue::to_clue lives here to break a circular dependency on declaration order.
constexpr Clue to_clue(BaseClue b, int target) {
  return Clue{b.kind, b.value, target};
}

}  // namespace hanabi
