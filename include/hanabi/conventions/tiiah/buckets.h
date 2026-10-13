// Throw It in a Hole — the suit buckets.
//
// A card in the hole is invisible, so the reactive clues (CONVENTION.md §1c)
// cannot name an identity the way a stable clue does. Instead the clue KIND
// encodes the relationship between the reacter's suit and the receiver's: under
// a rank clue the receiver's target sits `bucket_shift` buckets higher, under a
// colour clue as many lower, wrapping.
//
// ONE BUCKET PER SUIT: every non-inverted suit is a bucket of its own, numbered
// by its position among the non-inverted suits (an inverted suit is in no
// bucket). Five or six non-inverted suits since v23.0.0, three or four since
// v24.0.0 (the user's rulings); until v24.0.0 three or four kept three buckets,
// 4 suits grouped {[0,1],[2],[3]}.
//
// THE EPOCH SHIFT (v23.0.0) applies at five or six non-inverted suits only: the
// shift is +-1 when the clue's EPOCH, ceil(turn / 3), is odd and +-2 when it is
// even. +1, -1, +2, -2 are distinct and non-zero mod 5 and mod 6, so the clue kind
// is never ambiguous and a target never sits in the reacter's own suit. At three
// or four the shift is +-1 in every epoch: +-2 would collide there (2 == -2 mod 4,
// 2 == -1 mod 3).
#pragma once

#include <optional>
#include <vector>

#include "hanabi/basics/clue.h"

namespace hanabi {
struct Variant;
struct State;
}

namespace hanabi::tiiah {

// Five or six non-inverted suits under Throw It in a Hole: the shift follows the
// epoch (v23.0.0). Three or four: +-1 always (v24.0.0).
bool epoch_shift(const Variant& variant);

// How many buckets the variant has: its non-inverted suit count.
int bucket_count(const Variant& variant);

// The buckets, one per non-inverted suit in suit-index order, each holding that
// suit's ORIGINAL index (`& Orange (6 Suits)`: R, Y, G, B, P, with orange in none).
std::vector<std::vector<int>> variant_buckets(const Variant& variant);

// Which bucket `suit_index` sits in -- its position among the non-inverted suits --
// or nullopt for an inverted suit.
std::optional<int> bucket_of(const Variant& variant, int suit_index);

// How many buckets UP a reactive clue given on `clue_turn` (1-based, the turn
// the clue itself was given) moves from the reacter's card to the target:
// positive for a rank clue, negative for a colour clue. +-1, except under
// `epoch_shift`, where it is +-2 when ceil(clue_turn / 3) is even.
int bucket_shift(const Variant& variant, ClueKind kind, int clue_turn);

// The bucket the receiver's target must sit in, given the reacter's bucket.
int named_bucket(const Variant& variant, ClueKind kind, int clue_turn, int from);

// The inverse: the bucket the reacter's card must sit in, given the target's.
int reacter_bucket_for(const Variant& variant, ClueKind kind, int clue_turn,
                       int target_bucket);

// THE HIGH SCORE PHASE (v23.23.0 as "the late game", named by the user in v23.26.0;
// replay 2026455 T49): the plays made
// before `clue_turn` are at least 2/3 of the maximum score, 5 per suit. Counted off
// the action list, which every seat holds alike -- a miss included, since in TIIAH a
// miss looks like a play -- and never off the score, which is hidden. Late, the giver
// may give a globally known bucket violation, and a reader whose bucket offers nothing
// the card could be reads any playable, not the finesse first (CONVENTION.md §1d);
// and a play clue that reveals a card leading into the other hands is HIGH
// (reactor0 `high_score_lead_in`, tiiah/CONVENTION.md §2s, v23.26.0).
bool high_score_phase(const State& state, int clue_turn);

}  // namespace hanabi::tiiah
