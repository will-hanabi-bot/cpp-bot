// Port of python-bot/src/hanabi_bot/endgame/helper.py.
#pragma once

#include <map>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"
#include "hanabi/endgame/fraction.h"

namespace hanabi {
class Game;
struct State;
}  // namespace hanabi

namespace hanabi::endgame {

// RemainingMap: multiset of unseen identities, keyed by Identity ord. We use
// std::map (ordered) for deterministic iteration order matching Python.
using RemainingMap = std::map<int, int>;

RemainingMap remaining_remove(const RemainingMap& remaining, Identity id);
int remaining_total(const RemainingMap& remaining);

std::vector<Identity> find_must_plays(const State& state, const std::vector<int>& hand);

bool unwinnable_state(const State& state, int player_turn, int depth = 0);

// The most cards `seats` can still stack, playing in the order given, if each
// seat either does nothing or plays a card whose TRUE identity is playable at
// that moment. Counts PLAYS, not score, so reversed suits need no special
// arithmetic at the call site -- every play is worth exactly one point.
//
// OPTIMISTIC by design: a seat is credited with a card it may not be able to
// identify. That is the right model for "is this point still obtainable", which
// is the only question asked of it. It is only sound with the deck EMPTY, since
// it reads `state.deck[o].id()` and a hidden card contributes nothing -- with
// cards still to draw it would silently under-count.
//
// `stacks` is a play_stacks vector, taken by value so a caller can hand it a
// hypothetical. Depth is bounded by `seats.size()` (at most `num_players`) and
// branching by hand size, so this is cheap next to any search.
int best_reachable_plays(const State& state, std::vector<int> stacks,
                         const std::vector<int>& seats);

// One winning line for the trivial case. Empty/"" returned via empty vector +
// false bool.
struct TriviallyResult {
  std::vector<PerformAction> actions;
  Fraction winrate;
  bool found = false;
};
TriviallyResult trivially_winnable(const Game& game, int player_turn);

// Does pressing this action's button on this card advance a stack for EVERY
// reading its holder still has?
//
// Inverted suits make the BUTTON part of the question: Play advances a plain
// card and PITCHES an inverted one, Discard the reverse. A reading set spanning
// both kinds is therefore never certain, even when every identity in it is
// playable, because the two halves need opposite buttons.
//
// Deliberately built on `possibilities()` rather than on `obvious_playables`
// (clue-derived only) or `Thought::id(infer=true)` (needs a pinned singleton).
// A card read as {a5, d5} with both stacks on 4 scores whichever it is, and
// neither of those two notions can see that -- which is exactly how replay
// 1969779 T68 came to gamble a trash card on the final turn while holding one.
bool certainly_advances(const Game& game, int order, const PerformAction& how);

// Every action on our own cards that `certainly_advances`, in hand order.
std::vector<PerformAction> certain_plays(const Game& game);

// Our standing calls whose stamped button COULD advance a stack -- the `exists`
// mirror of `certainly_advances`'s `forall`, in hand order.
//
// A CTP presses Play and a CTD presses Discard, so which readings count depends
// on the suit exactly as it does there. Weaker than a certain play on purpose:
// when the solver has run out of time, actioning a call that might score beats
// taking a truncated search's guess. Skips a CTD at 8 tokens, where discarding
// is illegal.
std::vector<PerformAction> possible_call_actions(const Game& game);

struct GameArr {
  Fraction prob;
  RemainingMap remaining;
  std::optional<Identity> drew;
};

// Returns (undrawn, drawn). Faithful to Python's gen_arrs semantics.
std::pair<std::vector<GameArr>, std::vector<GameArr>> gen_arrs(
    const Game& game, const RemainingMap& remaining, bool clue_only);

}  // namespace hanabi::endgame
