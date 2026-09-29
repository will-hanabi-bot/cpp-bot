// Self-play simulator: a stand-in for the hanab.live server, driving one bot
// `Game` per seat through the same action path the live client uses.
//
// See v17_self_play_diagnostics/README.md. The simulator is variant-generic;
// what each seat is shown is keyed on `Variant::throw_it_in_a_hole`:
//
//   * TIIAH: every play reaches every seat as a hidden `play` (suit/rank -1),
//     landed or not. No `strike` record and no failed `discard` is sent, and the
//     `status` score is held constant -- strikes and the score are not public in
//     this variant, so only the simulator (the external observer) knows them and
//     ends the game on the third strike.
//   * Every other variant: plays carry their identity, a miss is a `strike` then
//     a failed `discard`, and `status` carries the true score, as on hanab.live.
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "hanabi/basics/action.h"
#include "hanabi/basics/convention.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/variant.h"
#include "hanabi/logging/game_logger.h"

namespace hanabi::selfplay {

struct SimConfig {
  std::string variant = "Throw It in a Hole (5 Suits)";
  int num_players = 3;
  std::uint64_t seed = 1;
  // Written into every seat's log as the game id. Self-play ids are
  // 9000000 + seed so they never collide with a hanab.live table or replay id.
  int game_id = 9000001;
  // Per-seat logs in the live format, `{log_dir}/{name}-{game_id}.log`. Empty
  // disables logging.
  std::string log_dir = "logs";
  double endgame_timeout = 6.0;
  // The bot's `/setall` mode. A TIIAH variant ignores it (`resolve_table_convention`).
  Convention convention_mode = Convention::REACTOR0;
  // Safety valve: abort a game that runs past this many turns.
  int max_turns = 200;
};

// hanab.live end conditions.
enum class EndCondition : int { NORMAL = 1, STRIKEOUT = 2, TERMINATED = 4 };

// What one physical action did, from the observer's seat.
struct Outcome {
  enum class Kind { CLUE, PLAY_LANDED, PLAY_MISSED, DISCARD };
  Kind kind = Kind::CLUE;
  int actor = 0;
  int turn = 0;  // 0-based index of this action
  // Play / discard.
  int order = -1;
  std::optional<Identity> id;
  bool pressed_play = false;  // the button, before inverted-suit orientation
  // Clue.
  int clue_target = -1;
  ClueKind clue_kind = ClueKind::COLOUR;
  int clue_value = 0;
  std::vector<int> touched;
  // Draw that followed, if any.
  std::optional<int> drawn_order;
};

// The pure wire-building half, kept free of any Game so it can be unit tested:
// the messages each seat receives for one action (landed/missed play, discard,
// clue), the draw that follows, the status and the turn / game-over record.
struct WireEvent {
  Outcome outcome;
  std::optional<Identity> drawn_id;  // identity of the drawn card
  int strikes_after = 0;             // strike count after this action
  int clues_after = 8;
  int score_after = 0;
  int max_score_after = 25;
  int next_turn_num = 1;             // `turn.num` to send
  int next_player = 0;
  std::optional<EndCondition> game_over;
};

// Per seat, the ordered server `action` objects (hanab.live wire shape).
std::vector<std::vector<nlohmann::json>> wire_messages(const Variant& variant,
                                                       int num_players,
                                                       const WireEvent& ev);

// The opening deal: per seat, one `draw` per card, own cards hidden.
std::vector<std::vector<nlohmann::json>> deal_messages(
    const std::vector<Identity>& deck, int num_players, int hand_size);

// The deck for `variant`, from the variant's own card counts, shuffled by `seed`.
std::vector<Identity> build_deck(const Variant& variant, std::uint64_t seed);

// hanab.live's hand size for a seat count (5 at 2-3, 4 at 4-5, 3 at 6).
int hand_size_for(int num_players);

// Truth kept by the observer.
struct TrueState {
  std::vector<Identity> deck;       // by order
  std::vector<std::vector<int>> hands;
  std::vector<int> holder;          // by order; -1 not yet drawn
  std::vector<int> stacks;          // cards played per suit (0..5)
  std::vector<int> landed_turn;     // by order; -1 unless landed
  std::vector<bool> discarded;      // by order (includes misses)
  int next_order = 0;
  int clues = 8;
  int strikes = 0;
  int score = 0;
  int turn = 0;                     // 0-based index of the next action
  int current = 0;
  std::optional<int> final_turn;    // the game ends once `turn` reaches it
  std::optional<EndCondition> ended;

  // The rank that plays next on `suit`, honouring reversed suits; 0 if done.
  int next_rank(const Variant& v, int suit) const;
  bool playable(const Variant& v, Identity id) const;
  int max_score(const Variant& v) const;
};

// Observer hooks, called once every seat has processed an action.
struct SimHooks {
  // Before the actor decides (the pre-action position).
  std::function<void(const class Sim&)> before_action;
  // After every seat has processed the action and its draw.
  std::function<void(const class Sim&, const Outcome&)> after_action;
};

struct SimError {
  std::string kind;  // "crash" | "harness_error"
  int turn = 0;
  int seat = -1;
  std::string what;
};

struct SimResult {
  std::uint64_t seed = 0;
  int game_id = 0;
  int score = 0;
  int max_score = 0;
  int reachable = 0;  // the max score still reachable when the game ended
  int clues = 0, plays = 0, misses = 0, discards = 0;  // actions taken
  int strikes = 0;
  int turns = 0;
  EndCondition end = EndCondition::NORMAL;
  std::optional<SimError> error;
};

class Sim {
 public:
  explicit Sim(SimConfig cfg);
  ~Sim();
  Sim(const Sim&) = delete;
  Sim& operator=(const Sim&) = delete;

  SimResult run(const SimHooks& hooks = {});

  const SimConfig& config() const { return cfg_; }
  const Variant& variant() const { return *variant_; }
  const TrueState& truth() const { return truth_; }
  int num_players() const { return cfg_.num_players; }
  const Game& seat(int p) const { return *games_[p]; }
  // For the detectors' own tests, which inject a fault into one seat's view.
  Game& seat_for_test(int p) { return *games_[p]; }
  const std::vector<std::string>& names() const { return names_; }
  const std::string& log_path(int p) const { return log_paths_[p]; }

 private:
  void feed(int p, const nlohmann::json& wire);
  void feed_all(const std::vector<std::vector<nlohmann::json>>& per_seat);
  // Resolve the actor's chosen action against the truth; nullopt + error on an
  // illegal action.
  std::optional<Outcome> resolve(int actor, const PerformAction& perform);
  void finish(EndCondition end);

  SimConfig cfg_;
  const Variant* variant_ = nullptr;
  std::vector<std::string> names_;
  std::vector<std::unique_ptr<Game>> games_;
  std::vector<std::shared_ptr<logging::GameLogger>> loggers_;
  std::vector<std::string> log_paths_;
  TrueState truth_;
  std::optional<SimError> error_;
  int n_clues_ = 0, n_plays_ = 0, n_misses_ = 0, n_discards_ = 0;
};

}  // namespace hanabi::selfplay
