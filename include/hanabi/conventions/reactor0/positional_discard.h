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

// THE CALLED PLAY IN A THIN ENDGAME (Throw It in a Hole, v23.13.0, the user's ruling;
// TODO: reactor0 and every variant). At pace <= 1 with at least three cards in the
// deck, Alice holding a call to play ALWAYS plays it when Bob holds fewer than two
// critical good cards -- distinct still-needed identities of which every remaining
// copy is in his hand, so both copies of one count once. Human diagnostic 2025488
// T50: green held its called u2 and Bob only the two y4s, and it clued instead.
std::optional<PerformAction> called_play_in_thin_endgame(const Game& game);

// THE POSITIONAL DOUBLE DISCARD (Throw It in a Hole, v23.13.0, the user's ruling;
// TODO: reactor0 and every variant). With 0 or 1 cards in the deck, Bob and Cathy
// both still to act, and each of them holding exactly one card left to play, Alice --
// with no required play of her own -- discards the slot equal to the SUM of their two
// cards' slots, wrapped mod 5 (leftmost copy of each). Each reads its own slot as
// Alice's minus the other's, which it can see. Human diagnostic 2025488 T60: the u4
// and the u5 on slot 5 each, so blue throws its slot 5.
//
// The position as every seat sees it (`alice` about to act): the deck holds 0 or 1
// cards, Bob and Cathy both still act after her, and she owes no reaction.
bool double_position(const Game& game, int alice);

// The position, and both Bob and Cathy hold exactly one card left to play as we see
// them: where they read a discard by `alice` as a double one. As Alice we then give no
// single positional discard, and no discard but the double one.
bool double_reads(const Game& game, int alice);

// Our discard, as Alice, when the double discard applies in full.
std::optional<PerformAction> positional_double_discard_signal(const Game& game);

// Reading Alice's discard as a double one: at Bob's or Cathy's seat, marks our card
// in the decoded slot `positional_play`. Returns whether the discard reads as a double
// one at this seat, in which case the single positional reading does not apply.
bool read_positional_double_discard(const Game& prev, Game& game,
                                    const DiscardAction& action);

}  // namespace hanabi::reactor0
