// Throw It in a Hole — the reverse reactive (CONVENTION.md §1c, §1d).
//
// Reactor0 dispatches positionally: a clue to Cathy is reactive, with Bob
// reacting and Cathy receiving. TIIAH reverses it when Bob holds a known play
// and Cathy does not — a clue to BOB is reactive, with **Cathy** reacting and
// **Bob** receiving. Bob plays what he already knows, Cathy answers, and Bob's
// target is waiting for him by the time he comes round again.
#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/identity.h"

#include "hanabi/basics/action.h"
#include "hanabi/basics/interp.h"

namespace hanabi {
class Game;
struct ReactorWC;
}
namespace hanabi::reactor0 {
struct ClueCandidate;
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
//
// `base` is the stack vector the walk is judged against; omitting it takes the
// SHARED view. A reader deciding what a clue between two named seats means
// passes what those two share instead (§1.3).
std::vector<ReceiverTarget> receiver_targets(const Game& game, int receiver,
                                             bool receiver_acts_first = true,
                                             const std::vector<int>* base = nullptr);

// The receiver's target: the next playable in their hand under STACK
// SIMULATION, where every known play in that hand is assumed already played
// (§1c). Returns the order, or nullopt when the walk finds nothing.
//
// A card already stamped CALLED_TO_PLAY is never a target — reactor's rule,
// which reactor0 deliberately reversed and TIIAH restores — and it falls out of
// the simulation: a called card is one of the plays assumed to have happened,
// so it is no longer waiting to be played.
std::optional<int> receiver_target(const Game& game, int receiver,
                                   bool receiver_acts_first = true,
                                   const std::vector<int>* base = nullptr);

// Read a reverse-reactive clue. Installs the waiting connection, stamps the
// reacter's blind play, and leaves the receiver's own call for reaction time,
// exactly as reactor0 does — the resolution machinery is shared.
//
// Returns nullopt when the clue cannot be read, which the caller stamps as a
// MISTAKE: with no waiting connection installed, nothing downstream fires.
std::optional<ClueInterp> interpret_reactive(const Game& prev, Game& game,
                                             const ClueAction& action,
                                             int reacter, int receiver);

// What the REACTER played, read by the receiver once the reaction has resolved
// (§1d, v16.19.0). The receiver returns before the target walk at clue time, so its
// copy of the reacter's card was never narrowed; this is the first moment it knows
// which slot answered, and the only chance it gets.
//
// Called from the same engine seam as `narrow_receiver_call`, just before it, since
// it can move the shared stacks the receiver's own reading then rests on.
//
// A no-op outside Throw It in a Hole, at the giver's and the reacter's own seats
// (where the clue-time reading already resolved the card), and for a card the
// receiver cannot see.
void narrow_reacter_play(const Game& prev, Game& game, const ReactorWC& wc,
                         int react_order);

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

// Keep the resolved reaction as evidence for world feasibility (§1e, v16.24.0):
// the receiver's hand at clue time, the frame the receiver could walk it in, and
// the card the reacter's answer named by the sum rule. Called from the engine seam
// when the REACTER pressed Play. A no-op outside Throw It in a Hole and for a
// connection that carries no receiver frame (the reverse arm).
void record_reaction(const Game& prev, Game& game, const ReactorWC& wc,
                     int react_order);

// REVERSE-REACTIVE CONFIRMATION (v18.11.0, the reviewer's rule). A reverse
// reactive stands only if its RECEIVER's next non-clue action plays one of the
// receiver's standing called cards (called before this action, in `prev`). A play
// of an uncalled card or a discard means the clue was something else -- a fix, when
// only the giver and the receiver could tell (2013726 T5 with green's o7 a b2) --
// and the reverse reactive is withdrawn: the waiting connection cleared, its
// pending copy retired, and the reacter's call on its reaction card dropped.
// Called from `Game::handle_action` BEFORE anything books the action, since the
// standing test reads the game as it stood. Returns whether it withdrew. A no-op
// outside Throw It in a Hole.
bool confirm_reverse_reactive(Game& game, int actor, int order, bool was_play);

// A reaction a newer reactive to the same receiver displaced (`Game::displaced_
// reactions`, v22.11.0): put back in its receiver's slot when its reacter plays or
// discards, so the deferred path reads it; `restore_displaced_slot` returns the
// newer entry afterwards.
std::optional<std::pair<int, std::optional<ReactorWC>>> promote_displaced_reaction(
    Game& game, int reacter);
void restore_displaced_slot(Game& game,
                            const std::optional<std::pair<int, std::optional<ReactorWC>>>& stash);

// THE TEAM's reading of a reacter's blind play, predicted at the GIVER's and the
// REACTER's seats (v17.2.0): what the receiver will be able to name at reaction
// time -- a proven finesse's connector, else the reacter's bucket on the shared
// frame -- intersected with what the card was read as before the clue. Those two
// seats know the card exactly (they saw the target); the team only knows this.
// Nullopt when `order` is not the card a reaction of `player` answers, at the
// receiver's own seat, or when the prediction does not contain `id`. The second
// member is that reaction's receiver.
std::optional<std::pair<IdentitySet, int>> reaction_team_reading(const Game& game,
                                                                 int player, int order,
                                                                 Identity id);

// The frame a reactive's target is walked in: the MINIMUM across every world the
// reacter can live in, from the giver's perspective (§1e, v16.24.0).
// `except_order` leaves one hole card out of those worlds: the reacter's own card,
// when the frame is asked after it has been played (`narrow_reacter_play`).
std::vector<int> reacter_frame(const Game& game, int giver, int reacter,
                               int except_order = -1);

// The giver's prediction of how many identities the receiver of a REACTIVE_PLAY
// candidate will read its called card as (§2, v16.28.0), written into
// `ClueCandidate::receiver_reading_size`. `hypo` is the game with the clue given.
// Passed to `reactor0::analyse_clues` as its annotator by the engine, since a
// convention may not depend on a sibling (reactor0 cannot call this itself).
void annotate_candidate(const Game& game, const Game& hypo,
                        reactor0::ClueCandidate& c);

}  // namespace hanabi::tiiah
