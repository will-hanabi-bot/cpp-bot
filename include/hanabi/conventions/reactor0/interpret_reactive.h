// Reactor0 reactive interpretation. The anchor of the slot arithmetic
// (react_slot + target_slot ≡ anchor, mod hand size) is the CLUE VALUE:
// the rank for rank clues, the fixed colour value (colour_value.h) for
// colour clues. There is no reactive focus.
//
// Rank = an even number of plays (2 or 0):
//   A. double play — target = leftmost playable in the receiver's hand,
//      INCLUDING already-CTP'd cards; the reacter blind-plays the react
//      slot. Targets whose react slot is visibly unworkable are skipped
//      leftmost-first.
//   B. finesse — target = leftmost one-away-from-playable; the reacter
//      must hold the connector at the react slot.
//   C. double discard — the reacter DISCARDS the react slot and the
//      receiver discards the dc-target (leftmost trash / same-hand dupe;
//      none → reactive lock when rlocks is on, else sacrifice).
//
// Colour = one play:
//   1. receiver has a playable → reacter discards react slot, receiver
//      plays the target (leftmost playable; react slots holding a known
//      critical are skipped by advancing the target).
//   2. no playable → reacter BLIND-PLAYS the react slot (the giver only
//      gives the clue when the react-slot card is visibly playable) and
//      the receiver discards the dc-target / locks.
#pragma once

#include <optional>

#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"

namespace hanabi::reactor0 {

std::optional<ClueInterp> interpret_reactive(const Game& prev, Game& game,
                                             const ClueAction& action,
                                             int reacter);

}  // namespace hanabi::reactor0
