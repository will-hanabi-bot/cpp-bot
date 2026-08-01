// Reactor0 reaction resolution — what the reacter's actual play/discard
// means for the receiver. Thin wrappers over reactor's exported resolution
// primitives (calc_target_slot, target_i_play/discard, elim_* matrices)
// with the clue-value anchor in wc.focus_slot, plus the one reading
// reactor does not have: a reactive dc-target on the receiver's OLDEST
// slot with wc.rlocks set reads as a whole-hand lock (in both the colour
// play→dc mode and the rank dc→dc mode).
//
// Reactor0 never rewinds (no response inversion), so both entry points
// always return false.
#pragma once

#include "hanabi/basics/game.h"

namespace hanabi::reactor0 {

bool react_play(const Game& prev, Game& game, int player_index, int order,
                const ReactorWC& wc);
bool react_discard(const Game& prev, Game& game, int player_index, int order,
                   const ReactorWC& wc);

// Stamp every still-held order of wc.receiver_hand CHOP_MOVED (the
// reactive lock). Game::chop then returns nullopt for the receiver, which
// is what makes the hand play as locked.
void reactive_lock(Game& game, const ReactorWC& wc);

// The lock reading itself: a dc-target on the receiver's OLDEST slot, with
// rlocks bound into the WC at clue time.
bool is_lock_target(const ReactorWC& wc, int target_slot);

// Clue-time prediction of the above, for the giver's own decision layer
// (CONVENTION.md §2b). The CHOP_MOVED stamps land a turn later in
// `reactive_lock`, so a freshly simulated clue carries none of them and the
// lock cannot be detected by inspecting the hypo's statuses. This recovers
// the receiver's target slot from the reacter's called slot
// (`ReactorWC::react_order`) and feeds it to `is_lock_target`, so the
// prediction and the resolution cannot drift apart.
//
// `hypo` must be a game in which a reactor0 reactive has just been
// interpreted. False when there is no waiting connection, no recorded
// react_order, or rlocks was off when the clue was given.
bool predicts_reactive_lock(const Game& hypo);

}  // namespace hanabi::reactor0
