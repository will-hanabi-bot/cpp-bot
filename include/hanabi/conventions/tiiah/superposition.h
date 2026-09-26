// Throw It in a Hole — superposition (CONVENTION.md §1e).
//
// A card that goes into the hole takes its identity with it, the player who
// played it included. What that player keeps instead is a SUPERPOSITION: the
// set of identities the card could have been. Everyone tracks everyone's, from
// `common`, so the seats agree about what each of them knows.
//
// These entry points are keyed on `Variant::throw_it_in_a_hole` rather than on
// `Game::convention`, because a TIIAH table with 4+ seats is one the convention
// refuses to ACT on while still tracking, and it
// needs the same bookkeeping.
#pragma once

#include <utility>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"

namespace hanabi {
class Game;
}

namespace hanabi::tiiah {

// One of the worlds a seat's outstanding hole plays leave open: an assignment
// of one identity to each of them, and the stacks that assignment produces.
struct OpenWorld {
  std::vector<std::pair<int, Identity>> assignment;
  State state;
};

// The worlds `holder`'s cards still in the hole leave open, starting from
// `base` and applying each assignment in PLAY ORDER so a chain lands
// (CONVENTION.md §1e).
//
// Exactly one world — `base` with an empty assignment — when the holder has
// nothing in the hole, which keeps every reading that calls this unchanged
// until somebody plays a card they cannot name. Also one when the product would
// exceed `cap`: a partial enumeration would read as a conditional set that is
// missing worlds, which is worse than reading it unconditionally.
std::vector<OpenWorld> open_worlds(const Game& game, const State& base,
                                   int holder, int cap = 64);

// The other half of a conditional reading: an antecedent has narrowed to
// `still`, so every world it contradicts is gone, and so is every candidate that
// had no other world left to stand in (`ConvData::ConditionalReading`).
//
// Called whenever a superposition narrows or settles — from
// `collapse_superpositions` and from its `settle`, which has to do it before it
// clears the set. Returns whether anything moved.
bool refute_worlds(Game& game, int antecedent, const IdentitySet& still);

// Called from `Game::handle_action` with the RAW wire action, before
// `resolve_hidden_action` has filled in what we could see. A play that reached
// the hole without a common-knowledge identity stamps
// `ConvData::superposition`; one WITH such an identity advances the shared
// stacks instead, since every seat can follow it.
//
// No-op outside TIIAH, and for every action that is not a hidden play.
void note_hidden_action(Game& game, const Action& raw);

// Called after an action has been interpreted. Applies §1e's collapsing rules
// to every outstanding superposition:
//
//   SHARED, so they also move the shared stacks --
//     1. another player played a card of that identity;
//     2. a clue called a playable card of that identity to play.
//   Both say the identity was still needed, so the superposed card was not it.
//
//   PRIVATE, so it moves our believed stacks alone --
//     3. every copy is accounted for between the discard pile and the hands we
//        can see.
//
// A set that reaches one identity leaves the map, and the stack it belongs to
// advances: our own believed one whenever the card was ours, and the shared one
// only when the collapse was shared.
//
// `prev` is the pre-action game, which is what makes "this clue newly called
// that card to play" answerable.
void collapse_superpositions(Game& game, const Game& prev, const Action& action);

}  // namespace hanabi::tiiah
