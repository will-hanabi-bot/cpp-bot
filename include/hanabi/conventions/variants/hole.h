// Throw It in a Hole — the shared piece both conventions need.
//
// Lives in the variants layer rather than in `tiiah/` because reactor0's call
// invariants read it too, and a convention must not depend on a sibling
// convention. Keyed on `Variant::throw_it_in_a_hole` like every other rule this
// variant brings, since a 4+ player TIIAH table runs reactor.
#pragma once

#include <optional>

#include "hanabi/basics/state.h"

namespace hanabi {
class Game;
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

}  // namespace hanabi::reactor::variants
