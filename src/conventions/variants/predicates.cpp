#include "hanabi/conventions/variants/predicates.h"

#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

namespace hanabi::reactor::variants {

bool includes_rainbowish(const State& state) {
  return state.includes_variant("Rainbow") || state.includes_variant("Omni");
}

bool includes_pinkish(const State& state) {
  // Funnels and Chimneys variants share the "rank clue touches multiple
  // ranks" property, so they should go through the same convention path
  // as pinkish variants (focus_slot = clue.value, etc.).
  if (state.variant->funnels || state.variant->chimneys) return true;
  return state.includes_variant("Pink") || state.includes_variant("Omni");
}

bool includes_brownish(const State& state) {
  return state.includes_variant("Brown") || state.includes_variant("Muddy") ||
         state.includes_variant("Cocoa") || state.includes_variant("Null");
}

bool includes_inverted(const State& state) {
  for (const Suit& suit : state.variant->suits) {
    if (suit.suit_type.inverted) return true;
  }
  return false;
}

bool includes_dark_inverted(const State& state) {
  for (const Suit& suit : state.variant->suits) {
    if (suit.suit_type.inverted && suit.suit_type.dark) return true;
  }
  return false;
}

bool uses_even_parity(const Variant& variant, ClueKind kind) {
  return variant.odds_and_evens ? kind == ClueKind::COLOUR
                                : kind == ClueKind::RANK;
}

int rank_reactive_value(const Variant& variant, int clue_value) {
  // odd (1) -> 3, even (2) -> 4. Both are valid slots at every hand size the
  // reactive family runs at (5 cards at 3 players, 4 at 4-5).
  return variant.odds_and_evens ? clue_value + 2 : clue_value;
}

}  // namespace hanabi::reactor::variants
