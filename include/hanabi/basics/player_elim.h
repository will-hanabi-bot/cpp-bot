// Port of python-bot/src/hanabi_bot/basics/player_elim.py.
// Original Scala: scala-bot/src/scala_bot/basics/playerElim.scala.
//
// Empathy elimination — the hot path that runs after every action to
// propagate the consequences of new information across all perspectives.
// Three layers: basic_elim (singleton-driven), cross_elim (subset locking),
// good_touch_elim (Good Touch Principle).
#pragma once

#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/player.h"

namespace hanabi {

class Game;
struct State;

struct CardElimResult {
  Player player;
  bool changed = false;
  std::unordered_set<int> removals;
  std::unordered_set<int> resets;
  IdentitySet recursive_ids;

  void merge(CardElimResult other);
};

// Returns (orders_reset, new_player). No-op if dirty is empty.
std::pair<std::unordered_set<int>, Player> card_elim(Player p, const State& state);

// Remove trash identities from clued/finessed/gentleman's-discarded cards
// (Good Touch Principle). Skips `except_` (typically the giver of the
// current clue).
std::pair<std::unordered_set<int>, Player> good_touch_elim(
    Player p, const Game& game, std::optional<int> except_ = std::nullopt);

// Resolve a link: focus gets inferred=single(id); siblings lose id from inferred.
Player elim_link(Player p, const Game& game, const std::vector<int>& matches,
                  int focus, Identity id);

// Scan each hand for unlinked cards with matching inferences and create
// Unpromised links.
Player find_links(Player p, const Game& game);

// Walk every Link in p.links, resolve / simplify / drop. Then re-run find_links.
// Returns (orders_resolved_via_sarcastic, new_player).
std::pair<std::vector<int>, Player> refresh_links(Player p, const Game& game);

// For each PlayLink: if all source cards are no longer in any hand, target is
// playable.
Player refresh_play_links(Player p, const Game& game);

}  // namespace hanabi
