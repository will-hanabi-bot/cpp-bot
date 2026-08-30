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

#include <optional>

#include "hanabi/basics/game.h"

namespace hanabi::reactor0 {

bool react_play(const Game& prev, Game& game, int player_index, int order,
                const ReactorWC& wc);
bool react_discard(const Game& prev, Game& game, int player_index, int order,
                   const ReactorWC& wc);

// Resolve a reaction the reacter DEFERRED (v12.0.0, §1e).
//
// `waiting` is cleared the moment the reacter clues instead of reacting, so a
// lawful deferral -- a VERY HIGH clue out-ranking the reaction, Precedence
// step 1 -- used to destroy the signal for the receiver too. `pending_reactions`
// keeps a durable copy; this consumes it when the reacter finally plays or
// discards.
//
// Call AFTER the ordinary `waiting` path has had its turn, and only when that
// path did not already handle this actor: the two never both apply, because a
// second reactive clue from the same giver replaces its own pending entry.
//
// `was_play` is the HOOK, not the outcome -- the button, as everywhere else in
// this file. Returns true if a pending reaction was consumed (either resolved or
// deliberately dropped), so callers can log it; the action itself is unaffected.
bool resolve_deferred_reaction(const Game& prev, Game& game, int player_index,
                               int order, bool was_play);

// Retire what `player_index` owed without resolving it, because the live
// `waiting` path is about to resolve the very same reaction. Undeferred
// reactions set BOTH records at clue time; this keeps the durable one from
// firing a second time on a later turn.
void retire_pending_reaction(Game& game, int player_index);

// Stamp every still-held order of wc.receiver_hand CHOP_MOVED (the
// reactive lock). Game::chop then returns nullopt for the receiver, which
// is what makes the hand play as locked.
void reactive_lock(Game& game, const ReactorWC& wc);

// The lock reading itself: a dc-target on the receiver's OLDEST slot, with
// rlocks bound into the WC at clue time.
bool is_lock_target(const ReactorWC& wc, int target_slot);

// Is `hypo`'s waiting connection the one THIS candidate clue just installed?
//
// `Game::interpret_clue` clears `waiting` only when the new clue's giver was the
// pending reacter (`decide.cpp:51-53`), so a stale connection from an earlier
// turn otherwise survives into the hypo of an unrelated candidate and every
// clue-time predictor reads it as its own.
//
// The turn comparison is `>=`, NOT `==`. `Game::simulate` routes to
// `simulate_action` (`game.h:180`), which emits a leading `TurnAction` before
// `handle_action` runs the interpretation (`game.cpp:690-696`), and that stamps
// `turn_count = num + 1` (`game.cpp:486`). So the WC is created one turn ahead
// of the caller's `game.state.turn_count`. An exact compare therefore never
// matches and silently disables whatever it guards — which is precisely how VH1
// shipped dead in v7.0.0 step 2. A genuinely stale connection is strictly older,
// so `>=` still rejects it.
//
// `react_order >= 0` additionally excludes the residue of an aborted reactive,
// where the WC is pushed before any phase runs
// (`interpret_reactive.cpp:570-571`) and no phase ever records a slot.
bool wc_is_fresh(const Game& game, const Game& hypo, int giver, int receiver,
                 int reacter);

// --- what a reacter can be asked to press ---------------------------------
//
// One definition each, read by BOTH the clue-time vet (`vet_react_slot`,
// interpret_reactive.cpp) and the deferred negatives that ask whether the
// alternative reading ever existed. `cand` is the reacter's empathy as every
// seat reconstructs it -- `effective_possible_for` -- so the answer is
// POV-invariant.
//
// Both are EXISTENTIAL: a slot is actionable if ANY reading it still admits
// makes that button acceptable. Demanding every reading is a different and much
// stronger claim, and demanding it of a pitch is what made a known orange read
// as unpitchable at replay 1973976 T12.

// Could the reacter press PLAY on this slot -- a real play, or a pitch he can
// spare? On an inverted suit Play DISCARDS the card, so the question there is
// affordability, not playability.
bool slot_is_pitchable(const State& s, const IdentitySet& cand);

// Its inverted half on its own: is there a reading that is inverted and can be
// SPARED? This is the pitch question proper -- asked when the play reading has
// already been ruled out, where the plain half of `slot_is_pitchable` would be
// answering about a play that cannot happen.
bool slot_has_spare_inverted(const State& s, const IdentitySet& cand);

// Could the reacter press DISCARD on this slot -- an ordinary throw he can
// spare, or a chuck that stacks the card?
bool slot_is_chuckable(const State& s, const IdentitySet& cand);

// The receiver's target slot, recovered at CLUE TIME from the reacter's
// called slot. `calc_slot` is its own inverse in the slot argument, so
// feeding it the reacter's slot yields the same number `calc_target_slot`
// derives at resolution time — prediction and resolution therefore cannot
// drift apart.
//
// This is what lets the decision layer see the half of a reactive that is
// not stamped yet: rank Phase B (finesse) and Phase C (double discard) stamp
// only the reacter at clue time, so the receiver's promised card is
// invisible to a walk over `hypo.meta`. Returns nullopt when there is no
// waiting connection, no recorded `react_order`, or the reacter's card has
// already left their hand.
std::optional<int> predicted_target_slot(const Game& hypo);

// The receiver's promised order, i.e. `predicted_target_slot` resolved
// against `wc.receiver_hand`. Nullopt when the slot is out of range.
std::optional<int> predicted_receiver_order(const Game& hypo);

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
