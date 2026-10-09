// THE POSITIONAL DISCARD (reactor0 and Throw It in a Hole, v23.2.0, the user's
// ruling). In the final round -- the deck empty -- with Bob still to act:
//
//   * Alice, holding no CERTAIN play, discards the slot that matches a card in
//     Bob's hand she can see is playable; Bob plays his card in that slot.
//   * Every discard Alice makes in that position is read so, whatever the card
//     in Bob's slot could be.
//   * A play the team can see is certain always comes first (v23.7.0, the user's
//     ruling): a Bob holding one plays it and ignores the discard, which then
//     speaks to Cathy -- if she still acts and holds no certain play herself --
//     and otherwise to nobody (`positional_reader`).
//   * When Bob holds no play, the remaining play is Alice's: she gambles on Rule
//     0b's selection -- the leftmost clued card that could be the card, else the
//     leftmost -- on the Play button (a play is never read as positional).
//   * When there is still no message and nothing to gamble on, Alice does not
//     discard: she stalls with a clue, else throws a slot Bob does not hold.
//
// A certain play (endgame rule 0) is the one thing that outranks it, and the
// test of it is the one every seat can make: Alice's cards as the team reads
// them, on the shared stacks. A discard that answers a reaction, or throws a card
// called to discard, says what that call says and is not positional.
//
// Replay 2024288 T61: the deck was empty, will-bot69 held the u5 in slot 5 and
// would act last, and will-bot67 had nothing certain. It gambled its slot 1 into
// a strike; the positional discard of its slot 5 wins the game.
#pragma once

#include <optional>

#include "hanabi/basics/action.h"

namespace hanabi {
class Game;
struct DiscardAction;
}  // namespace hanabi

namespace hanabi::reactor0 {

// The position, for `alice` about to act in `game`: a reactor0-family game, the
// deck empty, Bob (the next seat) still to act, Alice owing no reaction, and no
// certain play in Alice's hand as the team reads it.
bool positional_position(const Game& game, int alice);

// The seat a positional discard by `alice` speaks to: Bob, or Cathy when Bob holds
// a play the team can see is certain, or nobody (`std::nullopt`) when Cathy has no
// turn left or a certain play of her own.
std::optional<int> positional_reader(const Game& game, int alice);

// Our discard, as Alice: the slot of a playable card in Bob's hand, when the
// position holds, we hold no certain play ourselves, and some playable of Bob's
// sits in a slot we have. `std::nullopt` otherwise.
std::optional<PerformAction> positional_discard_signal(const Game& game);

// Our gamble, as Alice, when Bob holds no play we can see: Rule 0b's required
// play, else its selection over every playable identity, on the Play button only.
std::optional<PerformAction> positional_gamble(const Game& game);

// The guard when there is no message: while somebody reads our discard, one we
// would make that is not a signal becomes a stall clue, else the discard of a slot
// the reader does not hold. Any other action is returned unchanged.
PerformAction positional_guard(const Game& game, const PerformAction& chosen);

// Our play, as Bob: the card a positional discard named, while the deck is empty.
std::optional<PerformAction> positional_play(const Game& game);

// Reading Alice's discard (`prev` is the game before it): marks Bob's card in the
// matching slot `positional_play`, CALLED_TO_PLAY and urgent.
void read_positional_discard(const Game& prev, Game& game, const DiscardAction& action);

}  // namespace hanabi::reactor0
