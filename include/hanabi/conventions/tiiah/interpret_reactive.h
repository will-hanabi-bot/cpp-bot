// Throw It in a Hole — the reverse reactive (CONVENTION.md §1c, §1d).
//
// Reactor0 dispatches positionally: a clue to Cathy is reactive, with Bob
// reacting and Cathy receiving. TIIAH reverses it when Bob holds a known play
// and Cathy does not — a clue to BOB is reactive, with **Cathy** reacting and
// **Bob** receiving. Bob plays what he already knows, Cathy answers, and Bob's
// target is waiting for him by the time he comes round again.
#pragma once

#include <optional>
#include <vector>

#include "hanabi/basics/identity.h"

#include "hanabi/basics/action.h"
#include "hanabi/basics/interp.h"

namespace hanabi {
class Game;
struct ReactorWC;
}

namespace hanabi::tiiah {

// A card the clue could be for, and how far off it is: 0 for one that plays
// once the receiver's known plays are done, 1 for one the reacter has to bridge
// to — a finesse, which is how §1c's own worked example reads.
struct ReceiverTarget {
  int order = -1;
  Identity id{0, 1};
  int away = 0;
};

// Every candidate, in the order all three seats walk them: the ones that play
// outright first, then the one-aways, each leftmost-first.
//
// `receiver_acts_first` picks the stacks the walk runs on: the receiver's
// queued plays are simulated only on the REVERSE reactive, where he moves
// before the reacter. It defaults to that, the direction this file was
// written for.
std::vector<ReceiverTarget> receiver_targets(const Game& game, int receiver,
                                             bool receiver_acts_first = true);

// The receiver's target: the next playable in their hand under STACK
// SIMULATION, where every known play in that hand is assumed already played
// (§1c). Returns the order, or nullopt when the walk finds nothing.
//
// A card already stamped CALLED_TO_PLAY is never a target — reactor's rule,
// which reactor0 deliberately reversed and TIIAH restores — and it falls out of
// the simulation: a called card is one of the plays assumed to have happened,
// so it is no longer waiting to be played.
std::optional<int> receiver_target(const Game& game, int receiver,
                                   bool receiver_acts_first = true);

// Read a reverse-reactive clue. Installs the waiting connection, stamps the
// reacter's blind play, and leaves the receiver's own call for reaction time,
// exactly as reactor0 does — the resolution machinery is shared.
//
// Returns nullopt when the clue cannot be read, which the caller stamps as a
// MISTAKE: with no waiting connection installed, nothing downstream fires.
std::optional<ClueInterp> interpret_reactive(const Game& prev, Game& game,
                                             const ClueAction& action,
                                             int reacter, int receiver);

// The receiver's half of 1d's relation, applied once the reaction has been
// resolved and their call stamped: the bucket the reacter's own inference
// names, one step along, together with the continuation of the card the
// reacter played. See the definition for why each is read the way it is.
//
// Called from the engine seam (`Game::interpret_play`) rather than from
// reactor0's `stamp_receiver_call`, which makes the call: that is shared code
// and must not reach into a convention for the buckets.
//
// A no-op outside Throw It in a Hole, and outside a fresh CALLED_TO_PLAY on
// the receiver.
void narrow_receiver_call(const Game& prev, Game& game, const ReactorWC& wc,
                          int react_order);

}  // namespace hanabi::tiiah
