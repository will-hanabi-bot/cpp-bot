// Throw It in a Hole — superposition (CONVENTION.md §1e).
//
// A card that goes into the hole takes its identity with it, the player who
// played it included. What that player keeps instead is a SUPERPOSITION: the
// set of identities the card could have been. Everyone tracks everyone's, from
// `common`, so the seats agree about what each of them knows.
//
// These entry points are keyed on `Variant::throw_it_in_a_hole` rather than on
// `Game::convention`, because a 4+ player TIIAH table falls back to reactor and
// needs the same bookkeeping.
#pragma once

#include "hanabi/basics/action.h"

namespace hanabi {
class Game;
}

namespace hanabi::tiiah {

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
