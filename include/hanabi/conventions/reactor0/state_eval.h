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

// --- §2c Clue scoring ----------------------------------------------------

// Reactor0's own clue scorer, ported from `reactor::get_result` so the two
// conventions can diverge. The intentional difference is that reactor0 does
// not charge a flat penalty for bad touch — see the rationale in
// state_eval.cpp. `hypo` must be `game.simulate(Action{action})`.
double get_result(const Game& game, const Game& hypo, const ClueAction& action);

// `get_result` damped by whether we already hold a play, minus the flat
// tempo tax — reactor0's counterpart of `reactor::clue_branch_value`.
double clue_branch_value(const Game& game, const Game& hypo,
                         const ClueAction& action, bool we_hold_a_play);

// Top-level scorer, the reactor0 counterpart of `reactor::eval_action`.
// Non-clue actions delegate to reactor unchanged; `advance` / `eval_game` /
// `eval_state` are still reactor's.
double eval_action(const Game& game, const Action& action);

// --- §2b The pointless-double-discard filter -----------------------------

// Can `receiver` afford the "discard your dc-target" half of a reactive?
// True when their chop is expendable (not at risk) or they already hold a
// safe action: a known play, a CALLED_TO_PLAY call, a CALLED_TO_DISCARD, or
// known trash.
bool receiver_is_safe(const Game& game, int alice, int receiver);

// True when `action` is a reactive whose entire content is "you two both
// discard" — reactor0's rank Phase C, the only reactive shape that calls no
// play (interpret_reactive.cpp:366-393) — aimed at a receiver who did not
// need telling. Reactive LOCKS are exempt: a lock protects a whole hand, so
// it keeps competing on its own merits. `hypo` must be
// `game.simulate(Action{action})`.
bool is_pointless_double_discard(const Game& game, const Game& hypo,
                                 const ClueAction& action);

// True when `action` is a stable clue to Bob that gets a card played — the
// colour direct play, rank direct play and play-reveal readings, and not a
// stall, referential discard or trash reveal.
bool is_stable_play_clue_for_bob(const Game& game, const Game& hypo,
                                 const ClueAction& action);

// Erase every pointless double discard from `clues`, but only when a
// play-producing stable clue to Bob that is actually worth giving survives
// in the same set — so the double discard stays available when it is the
// best reactor0 has. One `game.simulate` per candidate; called once per turn
// from `Game::take_action` behind a `Convention::REACTOR0` guard, because it
// is a comparison *between* candidates that `eval_action` cannot express.
void drop_pointless_double_discards(
    const Game& game, std::vector<std::pair<PerformAction, Action>>& clues);

}  // namespace hanabi::reactor0
