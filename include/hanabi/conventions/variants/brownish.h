// Convention helpers for brownish variants (Brown / Muddy / Cocoa / Null
// suits), whose cards are untouched by rank clues.
#pragma once

#include <vector>

#include "hanabi/basics/game.h"

namespace hanabi::reactor::variants {

// Brownish trash reveal: a rank clue whose focus is known trash but which
// leaves the newest card untouched, while a brownish suit still has unplayed
// cards, reads as a REVEAL rather than a referential play.
bool brownish_trash_reveal(const Game& prev, const Game& game,
                           const ClueAction& action,
                           const std::vector<int>& newly_touched);

}  // namespace hanabi::reactor::variants
