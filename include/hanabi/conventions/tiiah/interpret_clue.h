// Throw It in a Hole — clue interpretation.
//
// The ruling reference is `src/conventions/tiiah/CONVENTION.md`.
#pragma once

#include <optional>

#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/interp.h"

namespace hanabi {
class Game;
struct State;
}

namespace hanabi::tiiah {

// Top-level entry, called from `Game::interpret_clue` when
// `game.convention == Convention::TIIAH`. Returns the interpretation, or
// nullopt for MISTAKE (the caller stamps it).
//
// The order it reads in, and why each step is where it is (CONVENTION.md):
//
//   1. the REFUSAL (§1c), ahead of the dispatch because it has to pre-empt it,
//      and an envelope — it falls through to the ladders;
//   2. the DISPATCH (§1c), both arms, the position deciding which;
//   3. the FIX CLUE (§1h), stable-only by construction, and it SUPERSEDES the
//      ladders rather than riding along;
//   4. the STABLE ladders (§1b), reactor0's, read on the pair's stacks (§1.3),
//      with §1f's re-pin on top.
//
// The stable half landed in v16.0.0 and the reactive half over v16.2.0-v16.9.0;
// §0's status table is the current account of what is implemented.
std::optional<ClueInterp> interpret_clue(const Game& prev, Game& game,
                                         const ClueAction& action);

// THE STABLE 1 (v23.18.0, the user's rule; CONVENTION.md §1b). The one identity a stable 1 play
// clue may call, judged on the globally known stacks: the special suit's 1 while
// its stack is at 0 (the special suit is the variant's last suit, when not plain);
// otherwise the 1 of the rightmost suit at 0 -- never a Brown, Dark Brown, Muddy
// Rainbow, Cocoa Rainbow, Null or Dark Null special suit. Nullopt when no 1 is
// left to call, and then no stable 1 play clue may be given.
std::optional<Identity> stable_one_identity(const State& state);

}  // namespace hanabi::tiiah
