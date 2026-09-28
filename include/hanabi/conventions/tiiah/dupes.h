// Throw It in a Hole — the two playable-dupe conventions (CONVENTION.md §1j, §1k).
//
// A card in the hole can be a copy nobody has named yet, so a seat can hold a card
// that LOOKS playable to the team and is in fact already down -- or two seats can
// both be about to play the same card. Two conventions let a seat that can see the
// duplicate say so without striking:
//
//   * PASSBACK (§1j): a seat that knows its card is X, and sees another seat's
//     called card that really is X while that seat's reading is not exactly X,
//     discards its own copy. The other holder reads the discard as "yours is X".
//   * DISCHARGE (§1k): a reacter whose reaction card is X, already played by the
//     giver without the giver knowing, discards it instead of playing; the
//     receiver still plays, and the giver learns what it threw in the hole.
#pragma once

#include <optional>

#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"

namespace hanabi {
class Game;
struct ReactorWC;
}  // namespace hanabi

namespace hanabi::tiiah {

// PASSBACK, the discarder's side: the card of ours to discard instead of playing,
// or nullopt. Only when a discard is legal (fewer than 8 clue tokens).
std::optional<int> dupe_passback(const Game& game);

// PASSBACK, the other holder's side, read from the RAW action before the dispatch:
// a partner discarded a card the team could name as X while a third seat holds a
// called card whose reading contains X but is not exactly X. That card narrows to
// {X}, for every seat. Returns whether it applied -- rule 7 then stays silent,
// since the discard explained itself.
bool read_passback(Game& game, const Action& raw);

// DISCHARGE, the reading side: the giver's hole card the reacter's discarded card
// `id` must have been -- a superposition held by the giver that still admits it --
// or nullopt, in which case the discard is read as an ordinary reaction.
std::optional<int> discharge_hole_card(const Game& game, const ReactorWC& wc,
                                       Identity id);

// DISCHARGE, found from the RAW action before the dispatch: the reacter of the
// standing reaction discarded a card, and it is a discharge of this giver hole card.
std::optional<int> find_discharge(const Game& game, const Action& raw);

// DISCHARGE, the reacter's side: whether to discard our reaction card `order`
// rather than play it. We can name it as X, X is already down on our own stacks,
// and the giver still holds a hole card that could be X.
bool discharge_instead(const Game& game, int order);

}  // namespace hanabi::tiiah
