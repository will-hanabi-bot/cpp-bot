// Throw It in a Hole — the suit buckets.
//
// A card in the hole is invisible, so the reactive clues (CONVENTION.md §1c)
// cannot name an identity the way a stable clue does. Instead the clue KIND
// encodes the relationship between the reacter's suit and the receiver's: under
// a rank clue the receiver's target sits `bucket_shift` buckets higher, under a
// colour clue as many lower, wrapping.
//
// SINGLE-SUIT BUCKETS (v23.0.0, the user's ruling): a TIIAH variant with five or
// six NON-INVERTED suits gives every one of them its own bucket, numbered by its
// position among the non-inverted suits (an inverted suit is in no bucket, as
// before), and the shift depends on the clue's EPOCH, ceil(turn / 3): +-1 on an
// odd epoch, +-2 on an even one. +1, -1, +2, -2 are distinct and non-zero mod 5
// and mod 6, so the clue kind is never ambiguous and a target never sits in the
// reacter's own suit. A TIIAH variant with three or four non-inverted suits keeps
// three buckets and +-1, which is what makes +-1 unambiguous there.
#pragma once

#include <array>
#include <optional>
#include <vector>

#include "hanabi/basics/clue.h"

namespace hanabi {
struct Variant;
}

namespace hanabi::tiiah {

// Five or six non-inverted suits under Throw It in a Hole: one bucket per
// non-inverted suit, and the epoch-dependent shift (v23.0.0).
bool single_suit_buckets(const Variant& variant);

// How many buckets the variant has: its non-inverted suit count under
// `single_suit_buckets`, else 3.
int bucket_count(const Variant& variant);

// The THREE-bucket table, each bucket holding the ORIGINAL suit indices it
// covers, in ascending order. Under `single_suit_buckets` the variant does not use
// it (`variant_buckets` is what it uses).
//
// Inverted (Orange / Dark Orange) suits are removed first and belong to no
// bucket: they are never a reactive target in the first place (§1c), and the
// remaining suits are re-indexed from 0 before the table below is applied. The
// table is keyed on how many suits REMAIN, so `& Orange (4 Suits)` — Red, Green,
// Blue — uses the three-suit one. The five- and six-suit rows are for a variant
// outside Throw It in a Hole only; a TIIAH one uses single-suit buckets:
//
//   3 -> {0:[0],   1:[1],   2:[2]}
//   4 -> {0:[0,1], 1:[2],   2:[3]}
//   5 -> {0:[0,1], 1:[2,3], 2:[4]}
//   6 -> {0:[0,1], 1:[2,3], 2:[4,5]}
//
// Any other count (no TIIAH variant has one) returns three empty buckets, which
// reads as "no suit has a bucket" and so proposes nothing.
std::array<std::vector<int>, 3> suit_buckets(const Variant& variant);

// The buckets the variant actually uses: under `single_suit_buckets`, one per
// non-inverted suit in suit-index order (`& Orange (6 Suits)`: R, Y, G, B, P,
// with orange in none); else `suit_buckets`.
std::vector<std::vector<int>> variant_buckets(const Variant& variant);

// Which bucket `suit_index` sits in, or nullopt for an inverted suit.
std::optional<int> bucket_of(const Variant& variant, int suit_index);

// How many buckets UP a reactive clue given on `clue_turn` (1-based, the turn
// the clue itself was given) moves from the reacter's card to the target:
// positive for a rank clue, negative for a colour clue. Three buckets: +-1.
// Single-suit buckets: +-1 when ceil(clue_turn / 3) is odd, +-2 when it is even.
int bucket_shift(const Variant& variant, ClueKind kind, int clue_turn);

// The bucket the receiver's target must sit in, given the reacter's bucket.
int named_bucket(const Variant& variant, ClueKind kind, int clue_turn, int from);

// The inverse: the bucket the reacter's card must sit in, given the target's.
int reacter_bucket_for(const Variant& variant, ClueKind kind, int clue_turn,
                       int target_bucket);

}  // namespace hanabi::tiiah
