// Port of python-bot/src/hanabi_bot/basics/game.py.
// Original Scala: scala-bot/src/scala_bot/basics/Game.scala + basics.scala.
//
// Game is the full game tree at a point in time. Convention-specific hooks
// (interpret_clue, interpret_play, etc.) default to identity at this layer;
// they're filled in by the reactor implementation (Phase 4). Reactor-specific
// fields (`waiting`, `zcs_turn`) live on Game directly — we don't subclass.
//
// API departs from Python: methods that the Python returns-a-new-Game (on_*,
// handle_action, with_*, elim) are *mutating* in C++. Use clone() / copy
// constructor when an explicit copy is needed. simulate_*() return copies.
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"

namespace hanabi {

struct Note {
  int turn = 0;
  std::string last;
  std::string full;

  bool operator==(const Note&) const = default;
};

// Reactor-specific waiting-connection record. Lives on Game to keep the
// data model flat (we have one convention; subclassing isn't needed). Fully
// populated in Phase 4 — for now the struct is declared so Game can hold a
// vector of them.
struct ReactorWC {
  int giver = 0;
  int reacter = 0;
  int receiver = 0;
  std::vector<int> receiver_hand;
  Clue clue{ClueKind::COLOUR, 0, 0};
  int focus_slot = 0;
  bool inverted = false;
  int turn = 0;

  bool operator==(const ReactorWC&) const = default;
};

class Game {
 public:
  // --- Fields ---
  int table_id = 0;
  State state;
  std::vector<Player> players;
  Player common;

  // Base snapshot taken at game start; used for rewinds. Captured as a tuple
  // of (state, meta, players, common). Held by value (deep copy).
  struct Base {
    State state;
    std::vector<ConvData> meta;
    std::vector<Player> players;
    Player common;
  };
  Base base;

  std::vector<ConvData> meta;
  std::vector<std::optional<Identity>> deck_ids;
  std::vector<IdentitySet> future;
  bool catchup = false;
  std::unordered_map<int, Note> notes;
  std::vector<std::optional<Action>> last_actions;
  std::vector<Interp> move_history;
  std::vector<std::pair<std::string, std::string>> queued_cmds;
  std::optional<Interp> next_interp;
  bool no_recurse = false;
  int rewind_depth = 0;
  bool in_progress = true;
  bool good_touch = false;

  // Reactor-specific (used in Phase 4).
  std::vector<ReactorWC> waiting;
  int zcs_turn = -1;

  // --- Factory ---
  static Game create(int table_id, State state);

  // --- Convention hooks (filled in by Phase 4) ---
  // Each hook mutates *this* in place. The Python signatures `(self, prev, action) -> Game`
  // become `(prev, action)` mutating self.
  void interpret_clue(const Game& prev, const ClueAction& action);
  void interpret_discard(const Game& prev, const DiscardAction& action);
  void interpret_play(const Game& prev, const PlayAction& action);
  void update_turn(const TurnAction& action);

  // Default: pass-through. Conventions may rewrite (e.g. ones in order).
  std::vector<int> filter_playables(const Player& player, int player_index,
                                      const std::vector<int>& orders,
                                      bool assume = true) const;

  // Default: true. Good-touch conventions disallow assigning trash to clued cards.
  bool valid_arr(Identity id, int order) const;

  // Hooks called from update_hypo_stacks; default identity.
  void refresh_after_play(const Game& prev, const PlayAction& action);
  void clean_hypo();

  // Score a (recorded) action. Default 0.0; conventions override.
  double eval_action(const Action& action) const;

  // --- Action handlers (mutate) ---
  void on_clue(const ClueAction& action);
  void on_discard(const DiscardAction& action);
  void on_play(const PlayAction& action);
  void on_draw(const DrawAction& action);

  // Top-level dispatcher. Mutates self.
  void handle_action(const Action& action);

  // --- Empathy elimination (real impl in Phase 2b once player_elim lands) ---
  // For now this is a stub that just clears `dirty` so things compile.
  void elim(std::optional<int> except = std::nullopt);

  // --- Simulation: return a new Game ---
  Game simulate_action(const Action& action,
                        std::optional<Identity> draw = std::nullopt) const;
  Game simulate_clue(const ClueAction& action, bool free = false) const;
  Game simulate(const Action& action) const { return simulate_action(action); }

  // --- Generic state updates (mutate self) ---
  void with_state(const std::function<void(State&)>& f);
  void with_meta(int order, const std::function<void(ConvData&)>& f);
  void with_card(int order, const std::function<void(Card&)>& f);
  void with_thought(int order, const std::function<Thought(const Thought&)>& f);
  void with_id(int order, Identity id);

  // --- Status predicates ---
  bool is_touched(int order) const;
  bool is_blind_playing(int order) const;
  bool is_saved(int order) const;
  bool order_matches(int order, Identity id, bool infer = false) const;
  // Variant-suit predicate: true iff every possible id of `order` is a suit
  // whose name contains the substring `needle`, or has rank == special_rank.
  bool known_as(int order, std::string_view needle,
                 std::optional<int> special_rank = std::nullopt) const;

  // --- Accessors ---
  const Player& me() const { return players[state.our_player_index]; }
  Player& me() { return players[state.our_player_index]; }
  std::optional<Interp> last_move() const {
    if (move_history.empty()) return std::nullopt;
    return move_history.back();
  }
  bool in_endgame() const { return state.pace() < state.num_players; }
};

}  // namespace hanabi
