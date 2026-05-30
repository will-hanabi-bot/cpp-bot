#include "hanabi/net/notes.h"

#include <algorithm>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"

namespace hanabi::net {

std::string format_play_segment(int turn, IdentitySet ids, const State& state) {
  std::vector<Identity> sorted = ids.to_list();
  std::sort(sorted.begin(), sorted.end(),
             [](Identity a, Identity b) { return a.to_ord() < b.to_ord(); });
  std::string id_str;
  for (size_t i = 0; i < sorted.size(); ++i) {
    if (i) id_str += ",";
    id_str += state.log_id(sorted[i]);
  }
  return "turn " + std::to_string(turn) + ": [f] " + id_str;
}

std::string format_discard_segment(int turn) {
  return "turn " + std::to_string(turn) + ": [kt]";
}

std::string format_reset_segment(int turn) {
  return "turn " + std::to_string(turn) + ": [reset]";
}

std::vector<std::pair<int, std::string>> compute_note_segments(const Game& prev,
                                                                  const Game& cur) {
  const State& state = cur.state;
  int me_idx = state.our_player_index;
  const Player& me_new = cur.players[me_idx];
  const Player& me_prev = prev.players[me_idx];
  std::vector<std::pair<int, std::string>> out;
  int prev_meta_len = static_cast<int>(prev.meta.size());
  int prev_thought_len = static_cast<int>(me_prev.thoughts.size());

  for (int order = 0; order < static_cast<int>(cur.meta.size()); ++order) {
    CardStatus new_status = cur.meta[order].status;
    CardStatus prev_status =
        order < prev_meta_len ? prev.meta[order].status : CardStatus::NONE;

    if (new_status != prev_status) {
      if (new_status == CardStatus::CALLED_TO_PLAY) {
        out.emplace_back(order, format_play_segment(state.turn_count,
                                                       me_new.thoughts[order].inferred, state));
      } else if (new_status == CardStatus::CALLED_TO_DISCARD) {
        out.emplace_back(order, format_discard_segment(state.turn_count));
      } else if (new_status == CardStatus::NONE &&
                  (prev_status == CardStatus::CALLED_TO_PLAY ||
                  prev_status == CardStatus::CALLED_TO_DISCARD)) {
        out.emplace_back(order, format_reset_segment(state.turn_count));
      }
      continue;
    }

    if (new_status != CardStatus::CALLED_TO_PLAY) continue;
    if (order >= prev_thought_len) continue;
    IdentitySet prev_inferred = me_prev.thoughts[order].inferred;
    IdentitySet new_inferred = me_new.thoughts[order].inferred;
    if (new_inferred != prev_inferred && new_inferred.length() < prev_inferred.length()) {
      out.emplace_back(order, format_play_segment(state.turn_count, new_inferred, state));
    }
  }
  return out;
}

}  // namespace hanabi::net
