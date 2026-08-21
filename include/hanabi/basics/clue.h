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

// A manual reassignment of one clue's reactive meaning, from `/set`.
//
// Every clue belongs to one of two PARITY buckets -- even (double play, or
// double discard) or odd (exactly one play) -- and carries a reactive value
// within it. Normally rank clues are the even bucket and colour clues the odd;
// Odds and Evens swaps them. `/set` moves a single clue between buckets and
// gives it a value, overriding whatever the variant would have assigned.
//
// Plain data, held here because `ClueKind` is: the rules that read it live in
// reactor0 (`conventions/reactor0/reactive_assignment.h`), which is the only
// convention that honours them.
struct ReactiveOverride {
  ClueKind kind;
  int clue_value;      // rank, or index into Variant::clue_colour_names
  bool even;           // which parity bucket
  int reactive_value;  // the anchor within that bucket

  constexpr auto operator<=>(const ReactiveOverride&) const = default;
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
