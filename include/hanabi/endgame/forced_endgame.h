// Hardcoded "forced endgame" rules. These detect specific late-game
// configurations where the bot's correct move is mechanically known and
// doesn't need a search — e.g., positions where playing the bot's known
// playable would empty the deck and lock the 5-holder out of their final
// turn before the corresponding 4 advances the stack.
//
// `forced_endgame_action` is invoked from `Game::take_action()` inside
// the existing endgame fork (`src/basics/game.cpp`). If it returns a
// `PerformAction`, that action is taken immediately, short-circuiting
// the full `EndgameSolver`.
//
// Currently implemented rules:
//   * 5-lockout — see `src/endgame/forced_endgame.cpp`.
#pragma once

#include <optional>

#include "hanabi/basics/action.h"

namespace hanabi {
class Game;
}

namespace hanabi::endgame {

std::optional<PerformAction> forced_endgame_action(const Game& game);

}  // namespace hanabi::endgame
