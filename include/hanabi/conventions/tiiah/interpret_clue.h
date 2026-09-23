// Throw It in a Hole — clue interpretation.
//
// The ruling reference is `src/conventions/tiiah/CONVENTION.md`.
#pragma once

#include <optional>

#include "hanabi/basics/action.h"
#include "hanabi/basics/interp.h"

namespace hanabi {
class Game;
}

namespace hanabi::tiiah {

// Top-level entry, called from `Game::interpret_clue` when
// `game.convention == Convention::TIIAH`. Returns the interpretation, or
// nullopt for MISTAKE (the caller stamps it).
//
// v16.0.0 implements the STABLE half only, and implements it by delegating to
// reactor0's ladders, which TIIAH shares unchanged. The reactive half — reverse
// dispatch, the bucket encoding, superposition — is specified in CONVENTION.md
// §1c and §1d and tracked in TODO.md; until it lands, a clue that would be
// reactive reads as a MISTAKE and stamps nothing, which is the only honest
// answer a convention can give about a meaning it does not yet implement.
std::optional<ClueInterp> interpret_clue(const Game& prev, Game& game,
                                         const ClueAction& action);

}  // namespace hanabi::tiiah
