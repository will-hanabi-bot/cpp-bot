// Port of python-bot/src/hanabi_bot/conventions/reactor/interpret_reaction.py.
// React to plays/discards after a reactive clue.
#pragma once

#include <optional>

#include "hanabi/basics/game.h"

namespace hanabi::reactor {

// Reactor's slot arithmetic: target/react slot mapping. 1-indexed.
int calc_slot(int focus_slot, int slot, int hand_size);

// Compute (react_slot, target_slot) for the reacter's played/discarded
// order. Returns nullopt if the mapping fails (order not in reacter's prev
// hand, target slot out of range, or the target card already left the
// receiver's hand). Anchor-agnostic — wc.focus_slot may hold reactor's
// focus slot or reactor0's clue-value anchor.
std::optional<std::pair<int, int>> calc_target_slot(const Game& prev,
                                                    const Game& game,
                                                    int order,
                                                    const ReactorWC& wc);

// Mark receiver-slot as CalledToDiscard, filtering out criticals from inferred.
// Mutates game.common and game.meta. target_slot is 1-indexed.
void target_i_discard(const Game& prev, Game& game, const ReactorWC& wc,
                       int target_slot);

// Mark receiver-slot as CalledToPlay, intersecting with playable+connectors.
void target_i_play(const Game& prev, Game& game, const ReactorWC& wc,
                    int target_slot);

// elim_* helpers - after a reactive interpretation, eliminate
// (play|trash) ids from earlier slots in the receiver's hand. Mutate game's
// common+meta. target_slot is 1-indexed (use len(receiver_hand)+1 to mean
// "process all slots"). prev_state is the state at the time of the original
// clue (pre-react-action) — the play/discard that triggers react_play has
// already mutated game.state, but the elim_* helpers should reason from the
// pre-action playable/trash sets and reacter's then-hand. (Mirrors Python's
// elim_*(state=prev.state, common=game.common, ...) signature.)
void elim_play_play(const State& prev_state, Game& game,
                     const std::vector<int>& receiver_hand,
                     int reacter, int focus_slot, int target_slot);
void elim_play_dc(const State& prev_state, Game& game,
                   const std::vector<int>& receiver_hand,
                   int reacter, int focus_slot, int target_slot);
void elim_dc_play(const State& prev_state, Game& game,
                   const std::vector<int>& receiver_hand,
                   int reacter, int focus_slot, int target_slot);
void elim_dc_dc(const State& prev_state, Game& game,
                 const std::vector<int>& receiver_hand,
                 int reacter, int focus_slot, int target_slot);

// Top-level: handle a play/discard while a waiting reactive connection is
// active. Mutates game with the resolved interpretation. Returns true if a
// rewind (re-interpret prior clue as reactive) occurred — in that case the
// caller must NOT do any further work on the current action, because the
// rewind's replay already handled it end-to-end (including with_move and
// elim); calling with_move again would double-record into move_history.
bool react_discard(const Game& prev, Game& game, int player_index, int order,
                    const ReactorWC& wc);
bool react_play(const Game& prev, Game& game, int player_index, int order,
                 const ReactorWC& wc);

// Apply the deferred reactive negative inference recorded in
// `Game::pending_dc_elim`, then leave the caller to clear it.
//
// Called from `Game::interpret_discard` once the receiver's called discard has
// actually happened AND the card proved to be trash -- which is the evidence
// that the target walk really did pass over the earlier slots without finding a
// play. Running it at reaction time instead is unsound on an inverted suit,
// where a called discard is a CHUCK that puts the card on its stack.
void apply_pending_dc_elim(Game& game);

// Decide a held receiver-chuck inference: void it if the chuck was a play,
// apply it if the chuck was a discard, or leave it held while that is still
// open. Judged from this seat's own knowledge -- observers resolve at reaction
// time, the receiver when they work the identity out. Cheap and idempotent, so
// it is safe to call after every interpretation. reactor0 only.
void resolve_pending_dc_elim(Game& game);

}  // namespace hanabi::reactor
