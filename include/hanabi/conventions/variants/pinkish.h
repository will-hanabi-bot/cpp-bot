// Convention helpers for pinkish variants (Pink / Omni suits, pink_s
// special rank), where a rank clue can touch ranks other than the one
// spoken and therefore carries a rank "promise".
#pragma once

#include <optional>
#include <vector>

#include "hanabi/basics/game.h"

namespace hanabi::reactor::variants {

// Pink-promise: in a pinkish variant a rank clue that touches the receiver's
// chop (rightmost unclued card) promises that the chop has that rank. Two
// substitutions apply when the natural rank clue for the special rank is
// unavailable (pink_s makes rank-K clue not touch rank-K cards):
//   pink_s + special_rank=5: rank-4 promises rank-5
//   pink_s + special_rank=1: rank-2 promises rank-1
// Returns true when the giver can see that the chop's rank cannot satisfy
// the promise — i.e., the clue is illegal.
//
// NOTE: intentionally uses a flag-based pinkish check (`pink_s` or any
// suit's `suit_type.pinkish`), which is narrower than the name-based
// `includes_pinkish` (no funnels/chimneys) — do not unify the two.
bool violates_pink_promise(const Game& prev, const ClueAction& action);

// Rank-promise on a ref-discard target (lock order or promised order):
// narrow the order's inferred to the clue's rank and mark it focused.
// Returns false when the visible deck identity contradicts the promise
// (callers treat the clue as uninterpretable).
bool apply_rank_promise(Game& game, int order, const BaseClue& clue);

// Pinkish focus selection for the playable-rank stable path: the focus is
// the leftmost (lowest order) newly-touched unclued card, falling back to
// the rightmost newly-touched card.
int playable_rank_focus(const Game& prev, const State& state,
                        const ClueAction& action,
                        const std::vector<int>& newly_touched);

}  // namespace hanabi::reactor::variants
