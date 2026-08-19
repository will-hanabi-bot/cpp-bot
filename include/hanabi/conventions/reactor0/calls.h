// Reactor0's per-player call tracking, and the two action lists phase 2 walks
// (DECISION_MAKING.md, "Decision phase 2").
//
// The spec describes four structures kept for every player — a reacter-CTP
// card, a reacter-CTD card, a receiver-CTP deque and a receiver-CTD card — with
// push and pop rules as clues arrive and reactions resolve.
//
// They are DERIVED here, not stored. `PLAN.md` §7 assumed phase 2 would need a
// new mutable container plumbed through every stamp and clear site; it does
// not, because the three properties that make the derivation exact are already
// maintained:
//
//   * `ConvData::urgent` is set only on the REACTER's call
//     (`reactor0/interpret_clue.cpp:199` takes it as a parameter, and
//     `stamp_receiver_play` never sets it). So urgent splits reacter from
//     receiver, which is the distinction `PLAN.md` §7 says is missing.
//   * `enforce_call_invariants` rule 1 keeps a hand's CTP calls running newest
//     slot to oldest in play order, erasing any call a later clue pointed past.
//     That IS the deque, and it is exactly the spec's "pop from the front until
//     the front is strictly older" rule, expressed as an erase at stamp time.
//   * rule 2 keeps at most one CTD per hand, which is the receiver-CTD slot's
//     "a new call replaces the standing one".
//
// A reaction pops its own slot for free: the reacted card leaves
// `state.hands`, and everything here iterates the hand, so the orphaned `meta`
// entry `PLAN.md` §7 warns about is simply never seen.
//
// Deriving also means zero serialisation work and no way to desync — a stored
// container would have to be rebuilt correctly by `apply_snapshot`, which
// replays actions and never reads `meta`.
#pragma once

#include <vector>

#include "hanabi/basics/game.h"

namespace hanabi::reactor0 {

// One player's outstanding calls. Orders, or -1 / empty for "none".
struct PlayerCalls {
  int reacter_ctp = -1;
  int reacter_ctd = -1;
  // Newest slot first. Rule 1 guarantees this is also play order.
  std::vector<int> receiver_ctp;
  int receiver_ctd = -1;

  // The pending reaction, if any. The spec keeps at most one card between the
  // two reacter slots, so this is unambiguous.
  int reacter_call() const {
    return reacter_ctp >= 0 ? reacter_ctp : reacter_ctd;
  }
  bool has_reaction() const { return reacter_call() >= 0; }
};

PlayerCalls calls_of(const Game& game, int player);

// Dependence, which the spec defines only over the receiver-CTP deque because
// it is the only structure that can hold more than one card. Card `a` depends
// on card `b` when `b` sits ahead of `a` in the deque AND their inferences —
// judged from the deciding player's own view, not common knowledge — leave open
// that they share a suit.
//
// The point is that actioning `a` first could strand `b`: if they might be the
// same suit, `b` may be the prerequisite.
bool depends_on(const Game& game, int player, int a_order, int b_order);

// The two lists of Actionable Card Priority.
struct ActionLists {
  // Pitchable orders (cards the holder would press PLAY on), partitioned into
  // dependence chains. Each chain runs front-first; a card outside the
  // receiver-CTP deque is always its own singleton, since dependence is only
  // defined inside the deque.
  std::vector<std::vector<int>> pitch_chains;
  // The front of each chain — the spec's **pitch list**.
  std::vector<int> pitch;
  // Cards stamped CTD, plus every chuckable card: trash on a plain suit, or
  // playable on an inverted one. The spec's **chuck list**.
  std::vector<int> chuck;
};

ActionLists action_lists(const Game& game, int player);

}  // namespace hanabi::reactor0
