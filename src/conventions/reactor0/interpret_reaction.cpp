#include "hanabi/conventions/reactor0/interpret_reaction.h"

#include <algorithm>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor0 {

using hanabi::reactor::calc_target_slot;
using hanabi::reactor::elim_dc_dc;
using hanabi::reactor::elim_dc_play;
using hanabi::reactor::elim_play_dc;
using hanabi::reactor::elim_play_play;
using hanabi::reactor::target_i_discard;
using hanabi::reactor::target_i_play;

// The reactive-lock reading: a dc-target on the receiver's OLDEST slot,
// with rlocks bound into the WC at clue time, locks the whole hand — even
// when the slot actually holds trash (the receiver cannot tell the two
// apart and must take the conservative reading).
bool is_lock_target(const ReactorWC& wc, int target_slot) {
  return wc.rlocks &&
         target_slot == static_cast<int>(wc.receiver_hand.size());
}

bool predicts_reactive_lock(const Game& hypo) {
  if (hypo.waiting.empty()) return false;
  const ReactorWC& wc = hypo.waiting.front();
  if (wc.react_order < 0) return false;
  const auto& reacter_hand = hypo.state.hands[wc.reacter];
  auto it = std::find(reacter_hand.begin(), reacter_hand.end(), wc.react_order);
  if (it == reacter_hand.end()) return false;
  const int react_slot = static_cast<int>(it - reacter_hand.begin()) + 1;
  // `calc_slot` is its own inverse in the slot argument, so feeding it the
  // reacter's called slot recovers the receiver's target slot — the same
  // number `calc_target_slot` derives at resolution time. Hand size comes
  // from `kHandSize`, matching the reactive selection paths
  // (interpret_reactive.cpp:219, :408).
  const int target_slot = hanabi::reactor::calc_slot(
      wc.focus_slot, react_slot, kHandSize[hypo.state.num_players]);
  return is_lock_target(wc, target_slot);
}

void reactive_lock(Game& game, const ReactorWC& wc) {
  int turn = game.state.turn_count;
  int giver = wc.giver;
  const auto& cur_hand = game.state.hands[wc.receiver];
  for (int o : wc.receiver_hand) {
    if (std::find(cur_hand.begin(), cur_hand.end(), o) == cur_hand.end()) {
      continue;
    }
    game.with_meta(o, [turn, giver](ConvData& m) {
      m.status = CardStatus::CHOP_MOVED;
      m.by = giver;
      m = m.reason(turn);
    });
  }
}

bool react_play(const Game& prev, Game& game, int player_index, int order,
                const ReactorWC& wc) {
  hanabi::instr::ScopedTimer st("reactor0.react_play");
  hanabi::logging::LogScope ls(
      "reactor0.react_play",
      {{"player_index", player_index}, {"order", order}, {"reacter", wc.reacter}});
  if (player_index != wc.reacter) return false;

  auto slots = calc_target_slot(prev, game, order, wc);
  if (!slots) return false;
  auto [react_slot, target_slot] = *slots;
  (void)react_slot;
  // Parity is fixed by clue kind alone — `wc.all_plays` is deliberately not
  // consulted (reactor0 never sets it; see interpret_reactive.cpp). Reading
  // it here is what made resolution contradict clue-time selection.
  if (wc.clue.kind == ClueKind::RANK) {
    // Even parity: reacter played → double play.
    target_i_play(prev, game, wc, target_slot);
    elim_play_play(prev.state, game, wc.receiver_hand, wc.reacter,
                   wc.focus_slot, target_slot);
  } else {
    // Odd parity, reacter played → the receiver's target is a discard (or
    // the lock).
    if (is_lock_target(wc, target_slot)) {
      reactive_lock(game, wc);
    } else {
      target_i_discard(prev, game, wc, target_slot);
      elim_play_dc(prev.state, game, wc.receiver_hand, wc.reacter,
                   wc.focus_slot, target_slot);
    }
  }
  return false;
}

bool react_discard(const Game& prev, Game& game, int player_index, int order,
                   const ReactorWC& wc) {
  hanabi::instr::ScopedTimer st("reactor0.react_discard");
  hanabi::logging::LogScope ls(
      "reactor0.react_discard",
      {{"player_index", player_index}, {"order", order}, {"reacter", wc.reacter}});
  if (player_index != wc.reacter) {
    game.with_move(DiscardInterp::NONE);
    return false;
  }

  // reactor0 never sets `all_plays` (see interpret_reactive.cpp), so this only
  // guards a WC inherited from elsewhere — a replayed snapshot, or a reactor
  // WC resolved under reactor0. Under /allplays the agreement is play+play:
  // the reacter has no discard available to them at all, so a discard is a
  // known mistake rather than the other half of a parity. Read it as such and
  // apply no marks — the receiver learns nothing about their target.
  if (wc.all_plays) {
    game.with_move(DiscardInterp::MISTAKE);
    return false;
  }

  auto slots = calc_target_slot(prev, game, order, wc);
  if (!slots) {
    game.with_move(DiscardInterp::NONE);
    return false;
  }
  auto [react_slot, target_slot] = *slots;
  (void)react_slot;
  // Parity is fixed by clue kind alone; see react_play.
  if (wc.clue.kind == ClueKind::COLOUR) {
    // Odd parity: reacter discarded → the receiver plays the target.
    target_i_play(prev, game, wc, target_slot);
    elim_dc_play(prev.state, game, wc.receiver_hand, wc.reacter,
                 wc.focus_slot, target_slot);
  } else {
    // Even parity, reacter discarded → double discard (or the lock).
    if (is_lock_target(wc, target_slot)) {
      reactive_lock(game, wc);
    } else {
      target_i_discard(prev, game, wc, target_slot);
      elim_dc_dc(prev.state, game, wc.receiver_hand, wc.reacter,
                 wc.focus_slot, target_slot);
    }
  }
  game.with_move(DiscardInterp::NONE);
  return false;
}

}  // namespace hanabi::reactor0
