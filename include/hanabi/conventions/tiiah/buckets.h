// Throw It in a Hole — the three suit buckets.
//
// A card in the hole is invisible, so the reactive clues (CONVENTION.md §1c)
// cannot name an identity the way a stable clue does. Instead the clue KIND
// encodes the relationship between the reacter's suit and the receiver's: under
// a rank clue the receiver's target sits one bucket higher, under a colour clue
// one bucket lower, wrapping. Three buckets is what makes ±1 unambiguous.
#pragma once

#include <array>
#include <optional>
#include <vector>

namespace hanabi {
struct Variant;
}

namespace hanabi::tiiah {

// The three buckets, each holding the ORIGINAL suit indices it covers, in
// ascending order.
//
// Inverted (Orange / Dark Orange) suits are removed first and belong to no
// bucket: they are never a reactive target in the first place (§1c), and the
// remaining suits are re-indexed from 0 before the table below is applied. The
// table is keyed on how many suits REMAIN, so `& Orange (6 Suits)` uses the
// five-suit row and `& Orange (4 Suits)` — Red, Green, Blue — the three-suit one:
//
//   3 -> {0:[0],   1:[1],   2:[2]}
//   4 -> {0:[0,1], 1:[2],   2:[3]}
//   5 -> {0:[0,1], 1:[2,3], 2:[4]}
//   6 -> {0:[0,1], 1:[2,3], 2:[4,5]}
//
// Any other count (no TIIAH variant has one) returns three empty buckets, which
// reads as "no suit has a bucket" and so proposes nothing.
std::array<std::vector<int>, 3> suit_buckets(const Variant& variant);

// Which bucket `suit_index` sits in, or nullopt for an inverted suit.
std::optional<int> bucket_of(const Variant& variant, int suit_index);

}  // namespace hanabi::tiiah
