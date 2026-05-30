// Port of python-bot/src/hanabi_bot/basics/player.py.
// Original Scala: scala-bot/src/scala_bot/basics/Player.scala.
//
// Player is one observer's belief state. player_index == -1 is the "common"
// perspective (what everyone knows everyone knows).
//
// Elimination algorithms (cardElim, goodTouchElim, basicElim, crossElim,
// refreshLinks, findLinks, elimLink) live in player_elim.h — same as the
// Python split.
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"

namespace hanabi {

class Game;
struct State;

struct MatchEntry {
  int order;
  // -1 if everyone knows the identity; otherwise the player index that doesn't know.
  int unknown_to;

  bool operator==(const MatchEntry&) const = default;
};

// --- Link ADT -------------------------------------------------------------

struct PromisedLink {
  std::vector<int> orders;
  Identity id;
  int target;  // order this link enables

  Identity promise() const { return id; }
  bool operator==(const PromisedLink&) const = default;
};

struct SarcasticLink {
  std::vector<int> orders;
  Identity id;

  Identity promise() const { return id; }
  bool operator==(const SarcasticLink&) const = default;
};

struct UnpromisedLink {
  std::vector<int> orders;
  IdentitySet ids;

  // No single promised identity.
  std::optional<Identity> promise() const { return std::nullopt; }
  bool operator==(const UnpromisedLink&) const = default;
};

using Link = std::variant<PromisedLink, SarcasticLink, UnpromisedLink>;

const std::vector<int>& link_orders(const Link& link);
std::optional<Identity> link_promise(const Link& link);

struct PlayLink {
  std::vector<int> orders;
  IdentitySet prereqs;
  int target;

  bool operator==(const PlayLink&) const = default;
};

// --- Player ---------------------------------------------------------------

struct Player {
  int player_index = -1;
  std::string name;
  IdentitySet all_possible;
  std::vector<int> hypo_stacks;

  bool is_common = false;
  std::vector<Thought> thoughts;

  std::vector<Link> links;
  std::vector<PlayLink> play_links;
  std::unordered_set<int> unknown_plays;
  std::unordered_set<int> hypo_plays;
  int linked_plays = 0;

  std::unordered_set<int> dirty;
  std::vector<std::vector<MatchEntry>> certain_map;

  static Player create(int player_index, std::string name, IdentitySet all_possible,
                        std::vector<int> hypo_stacks);

  // Apply f: Thought -> Thought to the thought at `order`, marking it dirty.
  Player with_thought(int order, const std::function<Thought(const Thought&)>& f) const;

  // Display helpers.
  std::string str_infs(const State& state, int order) const;
  std::string str_poss(const State& state, int order) const;

  // Pure helpers (no Game dependency).
  int playable_away(Identity id) const {
    return id.rank - (hypo_stacks[id.suit_index] + 1);
  }
  int unknown_ids(const State& state, Identity id) const;
  std::vector<int> linked_orders(const State& state) const;
  int hypo_score() const;

  // ---- Game-dependent methods (Stage 2b) ----
  int refer(const Game& game, const std::vector<int>& hand, int order,
             bool left = false) const;
  std::optional<int> chop_newest(const Game& game, int player_index) const;
  bool is_duped(const Game& game, Identity id, int exclude_order) const;
  bool is_trash(const Game& game, Identity id, int exclude_order) const;
  bool order_kt(const Game& game, int order) const;
  bool order_trash(const Game& game, int order) const;
  bool order_kp(const Game& game, int order, bool exclude_trash = false) const;
  bool order_playable(const Game& game, int order, bool exclude_trash = false) const;
  std::vector<int> obvious_playables(const Game& game, int player_index) const;
  std::vector<int> thinks_playables(const Game& game, int player_index,
                                      bool exclude_trash = false, bool assume = true) const;
  std::vector<int> thinks_trash(const Game& game, int player_index) const;
  bool thinks_loaded(const Game& game, int player_index) const;
  bool thinks_locked(const Game& game, int player_index) const;
  bool obvious_loaded(const Game& game, int player_index) const;
  bool obvious_locked(const Game& game, int player_index) const;
  bool is_sieved(const Game& game, Identity id, int exclude_order) const;
  std::vector<int> discardable(const Game& game, int player_index,
                                 bool allow_locked_sacrifice = false) const;
  bool valid_prompt(const Game& prev, int order, Identity id,
                     const std::unordered_set<int>& connected = {},
                     bool force_pink = false) const;
  std::optional<int> find_prompt(const Game& prev, int player_index, Identity id,
                                   const std::unordered_set<int>& connected = {},
                                   const std::unordered_set<int>& ignore = {},
                                   bool force_pink = false,
                                   bool rightmost = false) const;
  std::vector<int> find_clued(const Game& prev, int player_index, Identity id,
                                const std::unordered_set<int>& ignore = {}) const;
  int locked_discard(const State& state, int player_index) const;
  std::optional<int> anxiety_play(const State& state, int player_index) const;
  Player update_hypo_stacks(const Game& game) const;
};

// All orders whose card matches `id` from `player`'s perspective AND in the real deck.
std::vector<int> visible_find(const State& state, const Player& player, Identity id,
                                int exclude_order = -1);

// Player indices [start, target) iterated forward mod num_players.
std::vector<int> players_until(int num_players, int start, int target);

// Build the per-player and common-perspective Players for a fresh State.
std::pair<std::vector<Player>, Player> gen_players(const State& state);

}  // namespace hanabi
