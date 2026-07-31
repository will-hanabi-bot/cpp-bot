// Shared helpers for the reactor0 test suite.
#pragma once

#include <map>
#include <optional>
#include <vector>

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

// The convention marks on one card, for before/after diffing.
//
// A reacter's hand may legitimately carry SEVERAL CALLED_TO_PLAY stamps at
// once — they accumulate across clues, and the reacter actions them
// most-recently-stamped first, skipping any it knows from empathy must be
// trash. So "how many urgent cards does the reacter hold" is not an
// invariant. What IS one: a single reactive clue names exactly ONE card for
// the reacter to act on, so the set of cards *this clue* stamped must be a
// singleton (or empty, when the clue is rejected). That is a diff, never a
// count.
struct Mark {
  bool urgent = false;
  CardStatus status = CardStatus::NONE;
  std::optional<int> signal_turn;
  bool operator==(const Mark&) const = default;
};

inline std::map<int, Mark> hand_marks(const Game& g, TestPlayer player) {
  std::map<int, Mark> out;
  for (int o : g.state.hands[static_cast<int>(player)]) {
    out[o] = Mark{g.meta[o].urgent, g.meta[o].status, g.meta[o].signal_turn};
  }
  return out;
}

// Orders still in `player`'s hand whose marks changed since `before` — the
// cards the intervening action stamped. Cards drawn since are skipped: they
// cannot have been stamped by a clue that predates them.
inline std::vector<int> newly_marked(const std::map<int, Mark>& before,
                                     const Game& after, TestPlayer player) {
  std::vector<int> out;
  for (int o : after.state.hands[static_cast<int>(player)]) {
    auto it = before.find(o);
    if (it == before.end()) continue;
    Mark now{after.meta[o].urgent, after.meta[o].status,
             after.meta[o].signal_turn};
    if (!(it->second == now)) out.push_back(o);
  }
  return out;
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
