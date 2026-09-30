// Port of python-bot/src/hanabi_bot/basics/fix.py.
// Original Scala: scala-bot/src/scala_bot/basics/fix.scala.
//
// Fix-clue detection: was the given clue a "fix" (resetting a previously-clued
// card or revealing a duplicate). Also distribution_clue and rainbow_mismatch.
//
#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"

namespace hanabi {

class Game;
struct Player;
struct ClueAction;

struct FixResultNormal {
  std::vector<int> clued_resets;
  std::vector<int> duplicate_reveals;
  // True when the ONLY thing this clue fixed is "a previously clued card is now
  // known trash" -- no blind-play correction, no duplicate revealed.
  //
  // That is a trash reveal in all but name, and reactor0's stable COLOUR ladder
  // ranks it below every play reading: a colour clue means "action the leftmost
  // card you can", and only says "this one is dead" when nothing can be
  // actioned. The other two arms keep their priority, because both are
  // corrections that prevent a strike or a wasted duplicate rather than
  // information the receiver can afford to act on later.
  bool trash_reveal_only = false;
  bool operator==(const FixResultNormal&) const = default;
};

struct FixResultNoNewInfo {
  bool operator==(const FixResultNoNewInfo&) const = default;
};

struct FixResultNone {
  bool operator==(const FixResultNone&) const = default;
};

using FixResult = std::variant<FixResultNormal, FixResultNoNewInfo, FixResultNone>;

FixResult check_fix(const Game& prev, const Game& game, const ClueAction& action);

// THE FIX CLUE, Throw It in a Hole's form (tiiah/CONVENTION.md §1h, v16.20.0).
//
// A card in `clued`'s hand carries a standing CALLED_TO_PLAY; the holder's inference
// still admits more than one identity; and this clue has just narrowed it to exactly
// one, which the SHARED view says is dead. Returns that card's order.
//
// The clue does not have to TOUCH the card. `Game::on_clue` narrows from the negative
// just as it does from the positive, so a colour clue that misses a card can be what
// names it — which is half the point, since the other cards in the hand are then free
// to be whatever the giver wants touched.
//
// Why the PAIR's view decides the deadness (v17.2.0; the SHARED view until then): a
// clue only has to mean one thing to the two seats it is between (tiiah §1.3), and
// the pair can know a card is down that the third seat cannot -- the third seat's
// own blind play, which both of them watched. That is the duplicate a fix exists
// for: the giver of a call threw the other copy into the hole without knowing. A
// third seat reads the pair's view as the shared one, the floor it can compute.
// The half only the giver has is that THIS card is that identity, which is exactly
// what the clue transfers.
//
// `before` and `after` are the pre- and post-clue games: the reader passes (prev,
// game) and a giver weighing a candidate passes (game, hypo). Lives here rather than
// in either convention because both of them ask it, and neither may reach into the
// other. Nullopt outside Throw It in a Hole.
std::optional<int> dead_call_fix(const Game& before, const Game& after, int giver,
                                 int clued);
// With no giver: the deadness is asked of the SHARED view.
inline std::optional<int> dead_call_fix(const Game& before, const Game& after,
                                        int clued) {
  return dead_call_fix(before, after, /*giver=*/-1, clued);
}

// The same question asked BEFORE the clue, from its touches alone (v18.10.0):
// would this clue narrow one of its target's called cards to a single identity
// that is trash on the stacks the giver and the holder share? The dispatch asks
// it, because a clue to Bob that fixes his dead standing call is a FIX even when
// that call puts the table in the reverse-reactive position (tiiah §1c, §1h):
// replay 2013726 T5, where green's 2 fixes blue's dead `{r1,b2}`. Nullopt outside
// Throw It in a Hole.
std::optional<int> clue_would_fix_dead_call(const Game& before,
                                            const ClueAction& action);

std::optional<IdentitySet> distribution_clue(const Game& prev, const Game& game,
                                                const ClueAction& action, int focus);

bool rainbow_mismatch(const Game& game, const ClueAction& action, Identity id,
                       int prompt);

// If id is given, returns a non-empty list iff it can be made playable by
// `target`'s turn. Otherwise returns the orders that would be playable in
// target's hand by their turn. Port of fix.scala lines 55-78.
std::vector<int> connectable_simple(const Game& game, const Player& player,
                                       int start, int target,
                                       std::optional<Identity> id = std::nullopt);

}  // namespace hanabi
