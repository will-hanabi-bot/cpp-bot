// Starting required efficiency, used to pick the per-variant default for
// reactor0's allow_reactive_locks flag: reactive locks default ON except in
// hard variants (starting required efficiency > 1.42), where burning a
// whole hand on a lock is too expensive.
#pragma once

#include "hanabi/basics/variant.h"

namespace hanabi::reactor0 {

// max_score / (8 + starting_pace + suit-regain count), with the regain pool
// halved under Clue Starved (a 5 returns half a token there). Matches
// hanab.live's "required efficiency" to within its rounding.
double starting_required_efficiency(const Variant& variant, int num_players);

// True (reactive locks allowed) iff starting_required_efficiency <= 1.42.
bool default_allow_reactive_locks(const Variant& variant, int num_players);

}  // namespace hanabi::reactor0
