// Port of python-bot/src/hanabi_bot/basics/game.py.
// Original Scala: scala-bot/src/scala_bot/basics/Game.scala + basics.scala.
//
// Game is the full game tree at a point in time. Convention-specific hooks
// (interpret_clue, interpret_play, etc.) are implemented by the reactor
// convention layer (src/conventions/reactor/). Reactor-specific fields
// (`waiting`, `zcs_turn`) live on Game directly — we don't subclass.
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
#include "hanabi/basics/convention.h"
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

// Waiting-connection record for the reactive family. Lives on Game to keep
// the data model flat (conventions share the struct; we don't subclass).
// Under reactor, focus_slot is the reactive focus slot; under reactor0 it
// is the clue-value anchor — both feed the same slot arithmetic.
struct ReactorWC {
  int giver = 0;
  int reacter = 0;
  int receiver = 0;
  std::vector<int> receiver_hand;
  Clue clue{ClueKind::COLOUR, 0, 0};
  int focus_slot = 0;
  bool inverted = false;
  int turn = 0;
  // Snapshot of Game::all_plays at the time the WC was created. When true,
  // the reaction is play+play regardless of clue.kind — so the reacter has
  // no discard available to them, and one is read as a known mistake. See
  // interpret_reaction.cpp. Reactor only: /allplays is never set on a
  // reactor0 game (commands.cpp) and reactor0 always stores false here,
  // because its parity is fixed by clue.kind alone.
  bool all_plays = false;
  // The order the reactive interp called the reacter to act on (the
  // urgent CTP/CTD stamp). -1 when unknown — e.g. the receiver's own POV
  // interprets the clue without computing the reacter's slot. Diagnostic:
  // published in the per-game log's STATE snapshot so reaction bugs can
  // be triaged without re-deriving the reacter's called slot (added while
  // debugging replay 1916791).
  int react_order = -1;
  // Snapshot of the clue's PARITY BUCKET at WC creation (reactor0 only), so a
  // `/set` that lands mid-game cannot change what an already-given clue meant.
  // The same insulation `rlocks` gets below. Nullopt for a WC built without it
  // -- notably a reactor WC resolved under reactor0 -- where resolution falls
  // back to `variants::uses_even_parity`.
  std::optional<bool> even_parity;
  // Snapshot of Game::allow_reactive_locks at WC creation (reactor0 only):
  // resolution happens a turn later and /rlocks can be flipped mid-game,
  // so the reading must bind at clue time. Reactor never reads it. Kept
  // LAST — reactor aggregate-initializes the struct positionally.
  bool rlocks = false;

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

  // The reactive negative inference, held until the RECEIVER acts (reactor0).
  //
  // A reaction's negatives say "the slots the walk passed over were not
  // playable". Which slots, and which set, depend on what the receiver does
  // with their target -- and that is not settled when the reacter acts:
  //
  //   * receiver advanced a stack, and NOT the one the reacter advanced
  //       -> the passed-over slots (left of the target) held no playable;
  //   * receiver advanced the SAME stack the reacter did -- a FINESSE
  //       -> nothing in the hand was directly playable, and the passed-over
  //          slots were not one-away either;
  //   * receiver advanced no stack -- they discarded
  //       -> nothing in the hand was directly playable.
  //
  // Reading the reacter's side alone cannot distinguish these, and on an
  // inverted suit it cannot even tell a play from a discard: a CTD is a CHUCK,
  // which puts the card on its stack. So the whole inference waits.
  //
  // Everything is captured as of the REACTION, because "playable" and
  // "one away" have to be read as of then, not as of whenever the receiver
  // gets round to acting.
  //
  // A pure function of action history, so `apply_snapshot` rebuilds it by
  // replay and it needs no serialisation.
  struct PendingReactionElim {
    bool active = false;
    int receiver = -1;
    int target_order = -1;  // fires when the receiver actions THIS card
    int target_slot = 0;
    std::vector<int> receiver_hand;
    // The suit the reacter advanced, or -1 if their action stacked nothing (a
    // discard, or a pitch, or a strike). The finesse test compares against it.
    int reacter_suit = -1;
    IdentitySet playable;  // playable_set as of the reaction
    IdentitySet one_away;  // exactly one away from playable, as of the reaction
  };
  PendingReactionElim pending_reaction_elim;
  // Decide any held receiver-chuck inference (reactor0 only). Called after every
  // interpretation -- the deciding fact can arrive from any of them.
  void resolve_deferred_elims();

  // Run a held reactive negative inference, if the receiver has just actioned
  // the card it was waiting on. `prev` is the game before that action.
  void fire_reaction_elim(const Game& prev, int player_index, int order);
  // Reactor /allplays toggle. When true, every reactive clue (color or rank)
  // is interpreted as play+play; both the receiver and the reacter end up
  // CALLED_TO_PLAY. When false (default), color clues are play+dc and rank
  // clues are play+play (standard Reactor convention).
  bool all_plays = false;

  // Which convention this game interprets clues under. Defaults to REACTOR
  // so historical snapshots (which predate the field) and existing tests
  // replay under the convention they were played with; new live games get
  // BotClient's convention_mode_ at on_init.
  Convention convention = Convention::REACTOR;
  // reactor0 only: the reactive-lock reading (a reactive dc-target on the
  // receiver's oldest slot reads as a whole-hand lock). Set at game init
  // from the variant's starting required efficiency, or the /rlocks
  // override. Reactor ignores it.
  bool allow_reactive_locks = true;
  // reactor0 only: manual `/set` reassignments of clue -> (bucket, reactive
  // value). Empty means "the variant decides", which is every game that has not
  // used the command -- so an empty list reproduces the built-in table exactly.
  std::vector<ReactiveOverride> reactive_overrides;

  // Reactive-family state (shared by reactor and reactor0).
  std::vector<ReactorWC> waiting;
  int zcs_turn = -1;

  // --- Factory ---
  static Game create(int table_id, State state);

  // --- Convention hooks (implemented by src/conventions/reactor/) ---
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

  // Reset state to `base`, then replay action_list with `new_action`
  // inserted at the start of turn `turn`. Used by the convention layer to
  // re-interpret an earlier clue when a reaction event reveals the prior
  // stable interpretation was wrong (port of Python's Game.rewind).
  // `new_action` is typically an InterpAction(ClueInterp::REACTIVE) which
  // forces the next clue handler to take the reactive path.
  Game rewind(int turn, const Action& new_action) const;

  // --- Empathy elimination (card_elim + good_touch_elim; port of game.py:613-710) ---
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

  // Narrow `order`'s inference to `keep`, running reactor0's escalation ladder
  // if that empties it (CONVENTION.md §1i).
  //
  // Under reactor0 an inferred set may only ever SHRINK: it is never reset by a
  // re-derivation, by a dropped call, or by a strike. The one exception is a
  // genuine contradiction -- a narrowing that leaves nothing -- and it
  // escalates in three steps:
  //
  //   (1) `inferred ∩ keep` is non-empty  -> write it.
  //   (2) empty -> reset this card to global empathy and re-derive against
  //       `keep`. `possible` is itself already clue-narrowed, so this is
  //       exactly "reset to global empathy, then apply what we know".
  //   (3) still empty -> hard reset, drop the call, mark `[?]` and return
  //       FALSE. Nothing explains this card; the caller must refuse to
  //       interpret rather than invent a reading.
  //
  // Under any other convention this is a plain assignment, so shared callers
  // keep their existing behaviour.
  //
  // Returns false only at step (3).
  bool narrow_thought(int order, IdentitySet keep);

  // Widen `order`'s inference deliberately, bypassing the reactor0 clamp in
  // `with_thought`. The clamp exists so a widening cannot happen by accident;
  // this is how the ladder's own hard reset, and any future path that genuinely
  // must re-baseline a card, says so out loud.
  void reset_thought_to_empathy(int order);

  // Set `order`'s inference outright, bypassing the reactor0 no-widening rule.
  // For undoing an intermediate write made inside a single interpretation.
  void reset_thought_to(int order, IdentitySet set);

  // Append (or overwrite the latest) interpretation entry in move_history.
  // Port of game.py:274 with_move.
  void with_move(const Interp& interp, bool overwrite = false);

  // Clear the urgent flag on any card the current player was supposed to act
  // on but didn't (restoring old_inferred). Port of reactor.scala check_missed.
  void check_missed(int player_index, int action_order);

  // Void the call on a card whose `inferred` just emptied — the chain that
  // justified it has been contradicted. Meta half of elim's step-1 reset.
  void clear_contradicted_call(int order);

  // Reset the zcs_turn marker (zero-clue-starved tracking).
  void reset_zcs() { zcs_turn = -1; }

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
  bool in_endgame() const;  // reactor variant overrides via field; see game.cpp

  // --- Reactor convention helpers (live on Game so state_eval can use them) ---

  // The discard candidate for `player_index`: priority is CalledToDiscard, then
  // newest-unclued-NONE filtered by zcs_turn.
  std::optional<int> chop(int player_index) const;

  // Whether Bob (next player after current) has permission to discard.
  bool has_ptd() const;

  // Enumerate clue candidates the giver could give, filtering out mistakes
  // and useless duplicates (mirrors reactor.py find_all_clues). Unranked;
  // callers (take_action, endgame solver) score the candidates themselves.
  std::vector<PerformAction> find_all_clues(int giver) const;

  // Returns one discard candidate: trash head, else chop, else locked_discard.
  std::vector<PerformAction> find_all_discards(int player_index) const;

  // Pick the bot's action. The main decision function - port of reactor.scala
  // take_action. Invokes the endgame solver in the endgame.
  PerformAction take_action() const;
};

}  // namespace hanabi
