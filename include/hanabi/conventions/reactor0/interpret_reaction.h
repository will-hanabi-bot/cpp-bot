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

}  // namespace hanabi::reactor0
