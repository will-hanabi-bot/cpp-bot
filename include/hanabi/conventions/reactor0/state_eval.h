// Reactor0's clue TIER vocabulary. See src/conventions/reactor0/
// DECISION_MAKING.md "Clue Tier Definitions".
//
// As of v7.0.0 reactor0 no longer scores clues at all: the General Clue
// Evaluation List (reactor0/decision.h) chooses one by walking an ordered
// priority list, and reactor0's fork of `get_result` / `clue_branch_value` /
// `eval_action` is gone. Its non-clue actions still route to
// `reactor::eval_action`, and `advance` / `eval_state` / `eval_game` are still
// reactor's.
//
// What survives here is the tier itself, which is rules rather than tuning:
// `clue_tier` answers "how worthwhile is this clue", and `clue_is_admissible`
// (decision.h) is the only consumer.
//
// NOTE ON NAMING: "clue value" already means something else in reactor0 — it
// is the reactive *anchor* (include/hanabi/conventions/reactor0/colour_value.h,
// src/conventions/reactor0/GLOSSARY.md). The worth of giving a clue is called
// its **tier** here, never its value.
#pragma once

#include <utility>
#include <vector>

#include "hanabi/basics/action.h"

namespace hanabi {
class Game;
struct State;
}  // namespace hanabi

namespace hanabi::reactor0 {

// How worthwhile a candidate clue is. Ordered, so the gate can compare
// against a required minimum.
// Ordered, and compared with `>=` by `clue_is_admissible`, so a new rung slots
// in simply by sitting above the one below it.
//
// VERY_HIGH is the tier that out-ranks a PENDING REACTION (DECISION_MAKING.md
// Precedence step 1). It is deliberately narrow: everything at HIGH is worth a
// clue token, but only these two are worth abandoning a call the receiver is
// already decoding against.
enum class ClueTier { LOW = 0, MEDIUM = 1, HIGH = 2, VERY_HIGH = 3 };

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

}  // namespace hanabi::reactor0
