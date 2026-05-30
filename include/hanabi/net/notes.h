// Port of python-bot/src/hanabi_bot/net/notes.py.
// Diffs prev vs new Game to emit note segments for per-card status changes.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "hanabi/basics/identity_set.h"

namespace hanabi {
class Game;
struct State;
}  // namespace hanabi

namespace hanabi::net {

std::string format_play_segment(int turn, IdentitySet ids, const State& state);
std::string format_discard_segment(int turn);
std::string format_reset_segment(int turn);

// (card_order, segment) per change between prev and new.
std::vector<std::pair<int, std::string>> compute_note_segments(const Game& prev,
                                                                  const Game& cur);

}  // namespace hanabi::net
