// STATE-channel snapshot serializer + Game reconstructor.
//
// Writer: emit_snapshot(game, logger) writes one JSONL record with two
// halves:
//   (a) "replay": variant/options/players/deck_ground_truth + action history
//       — the canonical reconstruction input. apply_snapshot(json) replays
//       this back into a fresh Game by calling Game::create + handle_action.
//   (b) "debug": play_stacks, discard pile, clue tokens, hands w/ empathy
//       bitmasks, meta (CTP/CTD/focused/urgent), waiting connections, move
//       history. Redundant with (a) — derivable by replaying actions — but
//       supplied directly so show_turn.py can render a turn without invoking
//       the C++ binary.
//
// `apply_snapshot` reconstructs by replay because Game's internal player
// thoughts / certain_map / links / hypo_stacks are derived from elim() runs
// during action replay; bypassing them would require re-implementing elim
// outside Game. Replay is fast in practice (single-digit ms for full games).
#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "hanabi/basics/action.h"  // for the Action variant alias

namespace hanabi {
class Game;
}  // namespace hanabi

namespace hanabi::logging {

class GameLogger;

// Build the STATE JSON record for `game`. Doesn't touch the logger.
nlohmann::json build_state_snapshot(const Game& game, int turn);

// Emit a STATE record via the given logger. Convenience wrapper.
void emit_state_snapshot(GameLogger& logger, const Game& game, int turn);

// Reconstruct a Game from a STATE record (the JSON returned by
// build_state_snapshot). On failure throws std::runtime_error.
Game apply_snapshot(const nlohmann::json& record);

// JSON helpers for the Action variant. The hanab.live wire format isn't
// used here — this is our own internal format that round-trips through
// nlohmann::json. Exposed so replay_log can encode/decode action history.
nlohmann::json action_to_internal_json(const Action& a);
Action action_from_internal_json(const nlohmann::json& j);

}  // namespace hanabi::logging
