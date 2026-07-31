// Reactor0's decision-layer seam. See src/conventions/reactor0/CONVENTION.md §2.
//
// Reactor0 reuses reactor's evaluator wholesale — `get_result`, `advance`,
// `eval_state`, `eval_game`, the `take_action` ladder and the play/discard
// branches of `eval_action` are all reactor's. The ONE thing reactor0 owns is
// which clues it is willing to give at a low clue count: the **pace-clue tier
// gate** below replaces reactor's v0.34 low-clue-count gate
// (src/conventions/reactor/state_eval.cpp:474-497), which never runs under
// reactor0.
//
// NOTE ON NAMING: "clue value" already means something else in reactor0 — it
// is the reactive *anchor* (include/hanabi/conventions/reactor0/colour_value.h,
// src/conventions/reactor0/GLOSSARY.md). The worth of giving a clue is called
// its **tier** here, never its value.
#pragma once

#include "hanabi/basics/action.h"

namespace hanabi {
class Game;
struct State;
}  // namespace hanabi

namespace hanabi::reactor0 {

// How worthwhile a candidate clue is. Ordered, so the gate can compare
// against a required minimum.
enum class ClueTier { LOW = 0, MEDIUM = 1, HIGH = 2 };

// Classify a candidate clue. `hypo` must be `game.simulate(action)` — the
// tier depends on what the clue achieves, which is read off the CTP stamps
// the interpretation produced.
ClueTier clue_tier(const Game& game, const Game& hypo, const ClueAction& action);

// True when Alice's own outstanding calls force the HIGH-only tier: she holds
// a card stamped CALLED_TO_PLAY, or — in a variant containing an inverted
// (Orange) suit, where a discard is how an inverted card is played — a card
// stamped CALLED_TO_DISCARD. Reads the stamp literally; an empathy-known
// playable carrying no stamp does not count.
bool requires_high_tier(const Game& game);

// Top-level scorer, the reactor0 counterpart of `reactor::eval_action`.
// Non-clue actions delegate to reactor unchanged.
double eval_action(const Game& game, const Action& action);

}  // namespace hanabi::reactor0
