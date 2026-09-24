// Throw It in a Hole — the shared piece both conventions need.
//
// Lives in the variants layer rather than in `tiiah/` because reactor0's call
// invariants read it too, and a convention must not depend on a sibling
// convention. Keyed on `Variant::throw_it_in_a_hole` like every other rule this
// variant brings, which keeps it right at a table the convention itself will
// not act on (4+ seats).
#pragma once

#include <optional>

#include "hanabi/basics/state.h"

namespace hanabi {
class Game;
struct ClueAction;
}

namespace hanabi::reactor::variants {

// The stacks as they will stand once the plays already QUEUED have happened: a
// card stamped CALLED_TO_PLAY, or one whose common empathy is entirely playable
// identities. Walked to a fixpoint, so a chain advances in order.
//
// This is what "under stack simulation" means in tiiah/CONVENTION.md §1c, and
// it is why a call in that convention can be a DELAYED play: the reacter's card
// may only become playable once the receiver has played what they already know.
// A call like that is alive — it is simply not actionable yet.
//
// `only_player` restricts the simulation to one hand, which is what target
// selection wants; omitting it simulates every hand, which is what asking
// "could this call ever play" wants.
//
// `except_order` leaves one card out. Asking whether a CALL is dead means
// asking whether it can play once the OTHER queued plays have happened —
// simulate the call itself and its own identity looks spent, so it reads dead
// exactly when it is most alive.
State stacks_after_queued_plays(const Game& game,
                                std::optional<int> only_player = std::nullopt,
                                std::optional<int> except_order = std::nullopt);

// Does this seat hold a KNOWN PLAY? A card stamped CALLED_TO_PLAY whose
// inference still contains a playable identity, or a card whose global empathy
// is entirely playable identities. Read from `common`, so every seat answers it
// the same way. tiiah/CONVENTION.md §1c.
bool has_known_play(const Game& game, int player);

// TIIAH's dispatch (§1c), asked of the position BEFORE the clue: a clue is
// REACTIVE when it goes to the giver's Bob, that Bob holds a known play and the
// giver's Cathy does not — with Cathy reacting and Bob receiving, the reverse of
// reactor0's.
//
// The seat form takes a hypothetical giver and target, for the decision layer's
// "could my partner handle this with a stable clue?" questions, where no
// `ClueAction` exists yet. Both are named relative to the GIVER, not to us.
//
// Here rather than in `tiiah/` for the same reason as the simulation above:
// reactor0's DECISION layer has to predict what a candidate clue would mean, so
// it needs the dispatch, and a convention must not depend on a sibling. Neither
// form tests the variant flag — the caller has already decided which convention
// it is asking about.
bool reverse_reactive(const Game& prev, int giver, int target);
bool reverse_reactive(const Game& prev, const ClueAction& action);

}  // namespace hanabi::reactor::variants
