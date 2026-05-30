#include "hanabi/basics/connection.h"

#include <algorithm>

#include "hanabi/basics/state.h"

namespace hanabi {

std::optional<int> WaitingConnection::get_next_index(const State& state) const {
  for (size_t i = 0; i < connections.size(); ++i) {
    int conn_reacting = reacting(connections[i]);
    int conn_order = order(connections[i]);
    const auto& hand = state.hands[conn_reacting];
    if (std::find(hand.begin(), hand.end(), conn_order) != hand.end()) {
      return static_cast<int>(i);
    }
  }
  return std::nullopt;
}

}  // namespace hanabi
