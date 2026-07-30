// Shared helpers for the reactor0 test suite.
#pragma once

#include <optional>

#include "hanabi/basics/card.h"
#include "hanabi/basics/convention.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"

namespace hanabi::test::reactor0 {

// Opt a SetupOptions into reactor0 (with rlocks as given).
inline void use_reactor0(SetupOptions& opts, bool rlocks = true) {
  opts.init = [rlocks](Game& g) {
    g.convention = Convention::REACTOR0;
    g.allow_reactive_locks = rlocks;
  };
}

inline int order_at(const Game& g, TestPlayer player, int slot) {
  return g.state.hands[static_cast<int>(player)][slot - 1];
}

inline CardStatus status_at(const Game& g, TestPlayer player, int slot) {
  return g.meta[order_at(g, player, slot)].status;
}

inline bool urgent_at(const Game& g, TestPlayer player, int slot) {
  return g.meta[order_at(g, player, slot)].urgent;
}

inline bool any_status(const Game& g, TestPlayer player, CardStatus s) {
  for (int o : g.state.hands[static_cast<int>(player)]) {
    if (g.meta[o].status == s) return true;
  }
  return false;
}

inline std::optional<ClueInterp> last_clue_interp(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  const auto& last = g.move_history.back();
  if (auto* ci = std::get_if<ClueInterp>(&last)) return *ci;
  return std::nullopt;
}

}  // namespace hanabi::test::reactor0
