// Port of python-bot/src/hanabi_bot/conventions/reactor/state_eval.py.
// Game-state evaluator used by reactor's take_action.
#pragma once

#include "hanabi/basics/action.h"

namespace hanabi {
class Game;
struct State;
}

namespace hanabi::reactor {

// Score a clue based on its effect on the hypothetical game.
double get_result(const Game& game, const Game& hypo, const ClueAction& action);

// `eval_action`'s clue-branch arithmetic: `get_result` damped by whether we
// already hold a play, minus the flat tempo tax. Shared with reactor0, whose
// `eval_action` replaces only the gate in front of it. `hypo` must be
// `game.simulate(action)`; `we_hold_a_play` is
// `!me().obvious_playables(game, our_player_index).empty()`, passed in so
// callers that already computed it don't pay for it twice.
double clue_branch_value(const Game& game, const Game& hypo,
                         const ClueAction& action, bool we_hold_a_play);

// Recursive game-tree advance by simulating each player's choice.
double advance(const Game& orig, const Game& game, int offset);

// Top-level scorer for take_action.
double eval_action(const Game& game, const Action& action);

// Pure state-based evaluation.
double eval_state(const State& state, bool in_endgame);

// Full game-tree leaf evaluation.
double eval_game(const Game& orig, const Game& game);

}  // namespace hanabi::reactor
