// Issue detectors for the self-play harness (v17_self_play_diagnostics/README.md).
//
//   1  a card's inference (or possible set, or hole superposition) excludes its
//      true identity;
//   2  a CALLED_TO_PLAY card is never actioned: the call is dropped while the card
//      is still in hand, it stands to the end of the game, the holder discards a
//      playable call, or another seat sees a call the holder does not;
//   3  giver and reacter disagree on a reactive clue: its dispatch, its pairing,
//      or the receiver card the reaction finally calls;
//   4  a stack view sits below the highest rank everyone in the view has seen
//      played (`4-above`, a view above the truth, is informational);
//   5  a strike, with what each seat knew (informational, triaged by hand).
#pragma once

#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <nlohmann/json.hpp>

#include "hanabi/basics/card.h"
#include "hanabi/basics/state.h"
#include "sim.h"

namespace hanabi::selfplay {

struct Issue {
  std::string cls;   // "1".."5", "2w" (warning), "4-above", "crash", "harness_error"
  std::string kind;  // detector-specific sub-kind
  int turn = 0;      // 1-based, the numbering the STATE records and replay_log use
  int seat = -1;     // the seat whose view is wrong (or the actor)
  int order = -1;
  nlohmann::json detail = nlohmann::json::object();

  // Critical classes are what the stop criterion counts.
  bool critical() const { return cls == "1" || cls == "2" || cls == "3" || cls == "4"; }
};

class Diagnostics {
 public:
  explicit Diagnostics(const Sim& sim);

  SimHooks hooks();
  // Once `Sim::run` has returned.
  void finish(const Sim& sim, const SimResult& result);

  const std::vector<Issue>& issues() const { return issues_; }

 private:
  void before_action(const Sim& sim);
  void after_action(const Sim& sim, const Outcome& o);

  void check_inferences(const Sim& sim, int turn);
  void check_calls(const Sim& sim, const Outcome& o, int turn);
  void check_clue_reading(const Sim& sim, const Outcome& o, int turn);
  void check_reaction(const Sim& sim, const Outcome& o, int turn);
  void check_stacks(const Sim& sim, int turn);
  void check_strike(const Sim& sim, const Outcome& o, int turn);

  void add(Issue i);
  std::string id_str(Identity id) const;
  std::string set_str(const IdentitySet& s) const;

  const Variant* variant_ = nullptr;
  int np_ = 0;
  std::vector<std::string> log_paths_;

  // Pre-action snapshot, per seat.
  std::vector<std::vector<CardStatus>> pre_status_;
  std::vector<State> pre_state_;
  TrueState pre_truth_;
  // What the actor could name each card of its hand as, before acting.
  std::map<int, Identity> actor_named_;        // by its own reading
  std::map<int, Identity> actor_named_common_;  // by the common reading

  // Landed plays, for the stack floors.
  struct Landed {
    int order;
    Identity id;
    int player;
    bool named_by_player;   // at play time, by the player's own reading
    bool named_in_common;   // at play time, by the common reading
  };
  std::vector<Landed> landed_;

  // Holder-view call tracking.
  struct CallTrack {
    int stamped_turn = 0;
    int idle_turns = 0;     // holder turns the call stood, truly playable, unplayed
    bool idle_reported = false;
  };
  std::map<int, CallTrack> calls_;  // by order, in the holder's own view

  // A reactive clue awaiting its reaction, keyed by receiver seat.
  struct PendingReaction {
    int giver, reacter, receiver;
    int target;       // giver's receiver_target_order
    int react_order;  // giver's react_order
    int clue_turn;
  };
  std::map<int, PendingReaction> pending_;

  std::map<std::tuple<std::string, std::string, int, int, int>, int> seen_;  // dedup
  std::vector<Issue> issues_;
};

}  // namespace hanabi::selfplay
