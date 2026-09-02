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
std::string format_discard_segment(int turn, IdentitySet ids, const State& state);
std::string format_reset_segment(int turn);
// Ladder step (b): no reading explains this card. See ConvData::NoteMark.
std::string format_unknown_segment(int turn);

// An UNSTAMPED card of our own whose candidate set has narrowed to something a
// reader can hold in their head: the ids alone, with no `[f]`/`[d]` tag, because
// no call has been made. "turn 11: r2,r5".
std::string format_empathy_segment(int turn, IdentitySet ids, const State& state);

// The most candidates such a card may have and still be worth noting.
inline constexpr int kEmpathyNoteMax = 6;

// (card_order, segment) per change between prev and new.
std::vector<std::pair<int, std::string>> compute_note_segments(const Game& prev,
                                                                  const Game& cur);

}  // namespace hanabi::net
