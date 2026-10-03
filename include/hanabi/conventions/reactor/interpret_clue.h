// Port of python-bot/src/hanabi_bot/conventions/reactor/interpret_clue.py.
// Stable + reactive clue interpretation entry points + ref play / ref discard.
#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/interp.h"

namespace hanabi::reactor {

// Compute the focus slot for a reactive clue.
int reactive_focus(const State& state, int receiver, const ClueAction& action);

// Cards that will be playable by the receiver's turn (via observable plays).
// Returns (order, NEXT identity).
std::vector<std::pair<int, Identity>> delayed_plays(const Game& game, int giver,
                                                      int receiver, bool stable);

// Reference-play: target the card 'referred' (1 slot left of newly touched).
std::optional<ClueInterp> ref_play(const Game& prev, Game& game,
                                      const ClueAction& action);

// Mark `target` as CalledToPlay, narrowing inferred to playable+connector ids.
// Returns the interp (PLAY, STALL, or nullopt). Mutates game even on failure.
std::optional<ClueInterp> target_play(Game& game, const ClueAction& action,
                                         int target, bool urgent = false,
                                         bool stable = true);

// Mark `target` as CalledToDiscard.
std::optional<ClueInterp> target_discard(Game& game, const ClueAction& action,
                                            int target, bool urgent = false);

// Reference-discard: target the slot one right of focus.
std::optional<ClueInterp> ref_discard(const Game& prev, Game& game,
                                         const ClueAction& action, bool stall);

// The referential discard target of a clue, read on the hand BEFORE it: the first
// card right of the leftmost newly touched one that this clue does not touch and
// that was not clued already. Nullopt when there is none (or nothing is new).
std::optional<int> ref_discard_target(const State& prev, const ClueAction& action);

// Rank referential discards on the lock slot (reactor0 CONVENTION.md §1c, v20.0.0):
// in Throw It in a Hole and Clue Starved, under the reactor0 family, a rank clue
// touching the lock slot is a referential discard whenever it has a target, and a
// lock only when it has none.
bool lock_slot_refers(const Game& game);

// Try interpreting as a stable clue (returns null if the stable interp fails).
std::optional<ClueInterp> try_stable(const Game& prev, Game& game,
                                        const ClueAction& action, bool stall);

// Top-level stable interpretation; falls back to reactive if "bad stable".
std::optional<ClueInterp> interpret_stable(const Game& prev, Game& game,
                                              const ClueAction& action, bool stall);

// Top-level reactive interpretation. Dispatches to colour or rank impl.
std::optional<ClueInterp> interpret_reactive(const Game& prev, Game& game,
                                                const ClueAction& action,
                                                int reacter, bool looks_stable);

}  // namespace hanabi::reactor
