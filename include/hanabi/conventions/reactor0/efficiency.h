// The per-variant default for reactor0's allow_reactive_locks flag. Reactive
// locks default ON, except where burning a whole hand on a lock is too
// expensive: four named variant families, and any variant whose starting
// required efficiency exceeds 1.42.
#pragma once

#include "hanabi/basics/variant.h"

namespace hanabi::reactor0 {

// max_score / (8 + starting_pace + suit-regain count), with the regain pool
// halved under Clue Starved (a 5 returns half a token there). Matches
// hanab.live's "required efficiency" to within its rounding.
double starting_required_efficiency(const Variant& variant, int num_players);

// True (reactive locks allowed) unless any of five tests says otherwise, in
// order: 3 suits, Odds and Evens, Alternating Clues, Synesthesia, and finally
// `starting_required_efficiency > 1.42`. The first four are unconditional
// family vetoes and are NOT derived from the formula -- all four score under the
// threshold and defaulted ON until v13.5.0.
//
// This is the DEFAULT only. `/rlocks on|off` overrides it process-wide and
// retro-applies to running games; a reaction already in flight keeps the value
// bound into `ReactorWC::rlocks` at clue time.
bool default_allow_reactive_locks(const Variant& variant, int num_players);

}  // namespace hanabi::reactor0
