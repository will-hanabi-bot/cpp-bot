// Reactor0 clue interpretation. See src/conventions/reactor0/CONVENTION.md.
//
// Dispatch is purely positional: a clue to Bob (the giver's next player) is
// ALWAYS stable — even if Bob is loaded — and a clue to anyone else is
// ALWAYS reactive with Bob as the reacter, even in stall contexts (locked
// giver, endgame, 8 clue tokens). There is no loadedness heuristic, no
// bad_stable fallback, no re-tasking rule and no response inversion.
#pragma once

#include <optional>
#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"

namespace hanabi::reactor0 {

// The leftmost (slot-ascending) order in `candidates` whose common empathy
// intersects the playable set or a delayed-play successor. POV-safe: only
// common thoughts are consulted. This is the target a stable colour clue
// names (step 2 of `stable_colour` below); it is exported so the decision
// layer can ask what a hypothetical stable colour clue would name without
// paying for a full `simulate()`.
std::optional<int> leftmost_could_be_playable(
    const Game& game, const ClueAction& action,
    const std::vector<int>& candidates);

// Stamp a PITCH on `order`: press Play, which on an inverted (Orange / Dark
// Orange) suit sends the card to the discard pile and regains a clue.
// `inferred` is narrowed to the inverted identities only — a pitched card is
// being thrown away and need not be playable, which is why
// `reactor::target_play` cannot be used for this (it narrows to the playable
// set and bails when that empties). Returns `DISCARD`, the semantic outcome,
// or nullopt when the card cannot be orange at all.
//
// Used by the §1b orange ladder's pitch branch and, with `urgent` set, by the
// §1d rank Phase A reaction that pitches an expendable orange react slot.
std::optional<ClueInterp> stamp_orange_pitch(Game& game, const ClueAction& action,
                                             int order, bool urgent = false);

// Top-level entry: called from Game::interpret_clue when
// game.convention == Convention::REACTOR0. Returns the interpretation, or
// nullopt for MISTAKE (the caller stamps it).
std::optional<ClueInterp> interpret_clue(const Game& prev, Game& game,
                                         const ClueAction& action);

// Stable colour clue — a DIRECT play clue (there is no referential play in
// reactor0):
//   1. play reveal: a previously-clued card filled in as a new obvious
//      playable → REVEAL (fix handling runs first);
//   2. else the LEFTMOST card touched by this clue that could be playable
//      is called to play;
//   3. else STALL.
std::optional<ClueInterp> stable_colour(const Game& prev, Game& game,
                                        const ClueAction& action, bool stall);

// Stable rank clue, in priority order:
//   1. all remaining useful identities of the rank playable → direct play
//      clue (leftmost newly touched; if none newly touched, the leftmost
//      touched card that could be playable);
//   2. all identities of the rank trash → trash reveal (leftmost newly
//      touched marked trash; no newly touched → STALL). Terminal.
//   3. reveals previously-clued cards as trash / dupes → FIX or REVEAL
//      (includes the brownish trash reveal);
//   4./5. touches ≥ 1 new card → reactor's ref_discard (lock slot → LOCK,
//      else referential discard, pink promise included);
//   6. else STALL.
std::optional<ClueInterp> stable_rank(const Game& prev, Game& game,
                                      const ClueAction& action, bool stall);

}  // namespace hanabi::reactor0
