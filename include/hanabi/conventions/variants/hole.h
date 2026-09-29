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
//
// `base` is the stack vector the walk starts from. Omitting it takes the SHARED
// view, which is right for anything every seat must agree on; a reader deciding
// what a clue between two named seats means passes the view those two share
// instead (`State::stacks_known_to_both`, tiiah/CONVENTION.md §1.3).
State stacks_after_queued_plays(const Game& game,
                                std::optional<int> only_player = std::nullopt,
                                std::optional<int> except_order = std::nullopt,
                                const std::vector<int>* base = nullptr);

// Does this seat hold a KNOWN PLAY? A card whose clue-touch empathy (`possible`
// in `common`) allows only identities playable on the shared view (v18.0.0).
// Read from what every seat computes alike. Part of a STANDING play, below.
// tiiah/CONVENTION.md §1c.
bool has_known_play(const Game& game, int player);

// Does this seat hold a STANDING PLAY (v18.2.0)? A known play, or a CLUED card
// stamped CALLED_TO_PLAY, whatever its inference: a stable play clue stamps the
// call, and every seat stamps it alike (v18.3.0: clued only). It decides the reverse-reactive position
// (v18.3.0), and with it role inversion.
bool has_standing_play(const Game& game, int player);

// ROLE INVERSION (v18.2.0): this clue goes to the giver's Cathy and is STABLE,
// because the table is in the reverse-reactive position. Without it a clue to
// Cathy is reactor0's ordinary reactive. Since v18.3.0 it is exactly the
// clue-to-Cathy half of `reverse_reactive_position`. Asked of the position BEFORE
// the clue. tiiah/CONVENTION.md §1c.
bool inverted_stable(const Game& prev, int giver, int target);

// Is the table in the REVERSE-REACTIVE POSITION (§1c)? The giver's Bob holds a
// standing play and the giver's Cathy does not (v18.3.0). Asked of the position BEFORE the
// clue, and of the seats as the GIVER names them, not as we do.
//
// The position is what decides which seat's clue carries the reaction — TIIAH
// has both dispatches, and this is the switch between them:
//
//   position | clue to Bob                       | clue to Cathy
//   ---------|----------------------------------|---------------------------
//   holds    | REACTIVE, Cathy reacts, Bob gets | stable
//   else     | stable                           | REACTIVE, Bob reacts (reactor0's)
//
bool reverse_reactive_position(const Game& prev, int giver);

// The top-left square above: this clue is a REVERSE reactive. `position` and a
// clue aimed at the giver's Bob.
//
// The seat form takes a hypothetical giver and target, for the decision layer's
// "could my partner handle this with a stable clue?" questions, where no
// `ClueAction` exists yet.
//
// Here rather than in `tiiah/` for the same reason as the simulation above:
// reactor0's DECISION layer has to predict what a candidate clue would mean, so
// it needs the dispatch, and a convention must not depend on a sibling. None of
// the three tests the variant flag — the caller has already decided which
// convention it is asking about.
bool reverse_reactive(const Game& prev, int giver, int target);
bool reverse_reactive(const Game& prev, const ClueAction& action);

}  // namespace hanabi::reactor::variants
