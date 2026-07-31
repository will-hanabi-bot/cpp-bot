// Rank-direction helpers for reversed suits (play direction 5 → 1).
// Orange's `inverted` flag (play↔discard action swap) does NOT change the
// stack direction — only `reversed` does.
#pragma once

#include <optional>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

namespace hanabi::reactor::variants {

// The rank that regains a clue token when played: 5 for normal suits,
// 1 for reversed suits.
inline bool is_clue_regain_rank(const State& s, Identity id) {
  bool reversed = s.variant->suits[id.suit_index].suit_type.reversed;
  return reversed ? (id.rank == 1) : (id.rank == 5);
}

// The first or second rank in the suit's play direction: 1/2 for normal
// suits, 5/4 for reversed suits.
inline bool is_first_or_second_rank(const State& s, Identity id) {
  bool reversed = s.variant->suits[id.suit_index].suit_type.reversed;
  if (reversed) return id.rank == 4 || id.rank == 5;
  return id.rank == 1 || id.rank == 2;
}

// The card that must play immediately BEFORE `id` — its connector.
// `State::with_play` (src/basics/state.cpp:102-118) advances a reversed stack
// with `id.prev()`, so the prerequisite runs the other way: `id.next()` on a
// reversed suit, `id.prev()` everywhere else. Returns nullopt when `id` is the
// first card in its suit's play direction (rank 1 normal, rank 5 reversed).
inline std::optional<Identity> connector_of(const State& s, Identity id) {
  bool reversed = s.variant->suits[id.suit_index].suit_type.reversed;
  return reversed ? id.next() : id.prev();
}

}  // namespace hanabi::reactor::variants
