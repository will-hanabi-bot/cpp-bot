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
// The identities for which a stamped button is the RIGHT action.
//
// The stamp is the instruction, so a call's inferred set is built to match it:
// it contains exactly the identities the named button handles correctly, and
// nothing the button would misfire on. That is what makes a call unambiguous
// even when the holder cannot see their own suit -- there is no case where the
// stamp is right but the button is wrong, because the set excludes it.
//
// PITCH (press Play). On a plain suit Play plays the card; on an inverted suit
// it throws the card away. So a pitch call means either
//   * a plain-suit card that is currently PLAYABLE, or
//   * an inverted card that is NOT playable and is not critical -- i.e. one the
//     team can afford to lose.
// The receiver's call sets, read against TWO states: the plain-suit arm asks
// what a play would land on, so it uses the stacks the reacter LEAVES BEHIND
// (`new_s`); the inverted arm asks what we can afford to throw away, which is
// settled when the clue is given (`old_s`). The CTD pair is the mirror.
//
// Passing the same state twice recovers the stable-side readings below, so
// `pitch_candidates` / `chuck_candidates` are these with `old_s == new_s`.
IdentitySet receiver_ctp_set(const State& old_s, const State& new_s);
IdentitySet receiver_ctd_set(const State& old_s, const State& new_s);

IdentitySet pitch_candidates(const State& state);

// CHUCK (press Discard). On a plain suit Discard discards; on an inverted suit
// it puts the card on its stack. So a chuck call means either
//   * a plain-suit card that is NOT playable and not critical, or
//   * an inverted card that IS immediately playable.
IdentitySet chuck_candidates(const State& state);

// Narrow `order`'s inferred set to the candidates its own stamp admits.
//
// Intersects `possible` -- which already carries the clue's positive and
// negative information -- with the set above for whichever of CTP / CTD the
// card carries. A no-op for an unstamped card, and it refuses to empty the set:
// an empty inference erases the reading rather than recording it.
void narrow_to_stamped_button(Game& game, int order);

// Stamp a CHUCK — press Discard, which on an inverted suit puts the card on
// its stack. `reactor::target_discard` is NOT reusable here: it narrows
// `inferred` to the NON-CRITICAL ids, which is the plain-suit reading of "throw
// this away" and empties outright in Dark Orange, where every card is
// one-of-each. This narrows to the identities a chuck actually advances.
//
// Exported for the same reason as the pitch twin below: the REACTIVE side needs
// it. A reacter told to press Discard on a card that could be a playable orange
// is chucking it, and `target_discard` would refuse to stamp at all (replay
// 1967491 T36). `urgent` is what the reactive path adds.
std::optional<ClueInterp> stamp_orange_chuck(Game& game, const ClueAction& action,
                                             int order, bool urgent = false);

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
