// Port of python-bot/src/hanabi_bot/conventions/reactor/interpret_reaction.py.
// React to plays/discards after a reactive clue.
#pragma once

#include <optional>

#include "hanabi/basics/game.h"

namespace hanabi::reactor {

// Reactor's slot arithmetic: target/react slot mapping. 1-indexed.
int calc_slot(int focus_slot, int slot, int hand_size);

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

// Top-level: handle a play/discard while a waiting reactive connection is active.
// Mutates game with the resolved interpretation.
void react_discard(const Game& prev, Game& game, int player_index, int order,
                    const ReactorWC& wc);
void react_play(const Game& prev, Game& game, int player_index, int order,
                 const ReactorWC& wc);

}  // namespace hanabi::reactor
