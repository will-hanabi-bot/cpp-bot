// Hardcoded "forced endgame" rules. These detect specific late-game
// configurations where the bot's correct move is mechanically known and
// doesn't need a search.
//
// `forced_endgame_action` is invoked from `Game::take_action()` inside the
// endgame fork (`src/basics/decide.cpp`), ABOVE both the fork's `pace()` gate
// and the solver. If it returns a `PerformAction` that action is taken
// immediately, so a rule that fires wrongly is expensive.
//
// Rules, in the order they are asked:
//   * 0. Certain play — `cards_left == 0` and a card in hand advances a stack
//     on every reading it still has. A guaranteed point, so nothing can outbid
//     it. The `cards_left == 0` gate is load-bearing: with a card still in the
//     deck a point is sometimes worth less than a stall.
//   * 0b. Required play — `cards_left == 0`, nothing in hand is certain, and
//     laying some identity would raise the best score the rest of the final
//     round can reach. Gambles on the leftmost clued card that could be it,
//     else the leftmost of any. Replay 1970943 T24.
//   * 0c. Required play, one card early — `cards_left == 1`, nothing certain,
//     and the same ceiling test. With a card still in the deck that test is a
//     much weaker signal, so the CANDIDATE has to carry the confidence: it must
//     be clued, and everything it could be that is not already trash must be
//     the one required identity. Replay 1972670 T25.
//   * 4. Two criticals, dead partner — three players, `cards_left == 2`, the
//     NEXT seat's hand entirely trash, and CP holding two cards every reading of
//     which is playable AND critical. CP gets two turns from here if they act
//     now and one if they stall, and the partner cannot play whatever he is
//     told, so no stall buys the skipped turn back. Replay 1974303 T44. Note
//     the test is "every reading", not Rule 2's pinned identity.
//   * 2. Two-critical play — CP knows they hold two critical cards, one
//     playable, at `cards_left == 1` and `clue_tokens < n`.
//   * 3. Sole holder — CP pins a playable identity no other seat holds, whose
//     successor is still obtainable.
//   * 1. 5-lockout — clue to delay deck-empty when the 5-holder would
//     otherwise be locked out of their post-4 final turn.
//
// Rules 0c and 1-3 are `cards_left == 1` only; 0 and 0b are `cards_left == 0`
// only; Rule 4 is `cards_left == 2` only and is therefore the sole rule that
// fires at that deck size. 0c is asked ABOVE 1-3 because it is a play that must
// happen now and those rules can answer with a stall clue.
// See `src/endgame/forced_endgame.cpp` for the predicates.
#pragma once

#include <optional>

#include "hanabi/basics/action.h"

namespace hanabi {
class Game;
}

namespace hanabi::endgame {

std::optional<PerformAction> forced_endgame_action(const Game& game);

}  // namespace hanabi::endgame
