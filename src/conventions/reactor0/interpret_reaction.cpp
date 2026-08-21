#include "hanabi/conventions/reactor0/interpret_reaction.h"

#include "hanabi/conventions/variants/inverted.h"

#include <algorithm>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor0 {

namespace variants = hanabi::reactor::variants;

using hanabi::reactor::calc_target_slot;
using hanabi::reactor::elim_dc_dc;
using hanabi::reactor::elim_dc_play;
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

bool wc_is_fresh(const Game& game, const Game& hypo, int giver, int receiver,
                 int reacter) {
  if (hypo.waiting.empty()) return false;
  const ReactorWC& wc = hypo.waiting.front();
  return wc.turn >= game.state.turn_count && wc.giver == giver &&
         wc.receiver == receiver && wc.reacter == reacter &&
         wc.react_order >= 0;
}

std::optional<int> predicted_target_slot(const Game& hypo) {
  if (hypo.waiting.empty()) return std::nullopt;
  const ReactorWC& wc = hypo.waiting.front();
  if (wc.react_order < 0) return std::nullopt;
  const auto& reacter_hand = hypo.state.hands[wc.reacter];
  auto it = std::find(reacter_hand.begin(), reacter_hand.end(), wc.react_order);
  if (it == reacter_hand.end()) return std::nullopt;
  const int react_slot = static_cast<int>(it - reacter_hand.begin()) + 1;
  // `calc_slot` is its own inverse in the slot argument, so feeding it the
  // reacter's called slot recovers the receiver's target slot — the same
  // number `calc_target_slot` derives at resolution time. Hand size comes
  // from `kHandSize`, matching the reactive selection paths
  // (interpret_reactive.cpp:219, :408).
  return hanabi::reactor::calc_slot(wc.focus_slot, react_slot,
                                    kHandSize[hypo.state.num_players]);
}

std::optional<int> predicted_receiver_order(const Game& hypo) {
  auto slot = predicted_target_slot(hypo);
  if (!slot) return std::nullopt;
  const ReactorWC& wc = hypo.waiting.front();
  if (*slot < 1 || *slot > static_cast<int>(wc.receiver_hand.size())) {
    return std::nullopt;
  }
  return wc.receiver_hand[*slot - 1];
}

bool predicts_reactive_lock(const Game& hypo) {
  auto target_slot = predicted_target_slot(hypo);
  if (!target_slot) return false;
  return is_lock_target(hypo.waiting.front(), *target_slot);
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

// Capture a receiver-chuck's negative inference instead of applying it.
//
// Both branches that call the receiver to CHUCK -- colour + reacter played
// (`elim_play_dc`) and rank + reacter discarded (`elim_dc_dc`) -- reason "the
// slots the walk passed over are not playable". That holds only if the chuck is
// really a DISCARD. On an inverted suit a chuck puts the card on its stack, so
// it is a PLAY: the walk passed over nothing and the inference is unfounded.
//
// Replay 1966710 is the case in full. The T5 reactive was rank + reacter
// discards, so `elim_dc_dc` ran at once and its nested `elim_play_play` stripped
// the playable b2 from the receiver's slot 3. But the receiver's called card was
// an Orange 1 with the orange stack on 0 -- chucking it at T7 scored -- so the
// reaction was never a double discard and no negative was ever owed. The clue
// that re-targeted slot 3 at T8 then found an inferred set with no playable left
// in it, and the call was dropped as dead.
//
// Everything is snapshotted as of NOW, because "playable" and "trash" have to be
// read as of the reaction. `resolve_pending_dc_elim` decides its fate later.
void defer_receiver_chuck_elim(const Game& prev, Game& game, const ReactorWC& wc,
                               int target_slot, Game::PendingDcElim::Kind kind) {
  Game::PendingDcElim pend;
  pend.kind = kind;
  pend.active = true;
  pend.hand_size = kHandSize[prev.state.num_players];
  pend.focus_slot = wc.focus_slot;
  pend.target_slot = target_slot;
  pend.receiver_hand = wc.receiver_hand;
  pend.reacter_hand = prev.state.hands[wc.reacter];
  pend.playable = prev.state.playable_set;
  pend.trash = prev.state.trash_set;
  pend.critical = prev.state.critical_set;
  for (int o : wc.receiver_hand) {
    pend.receiver_was_clued.push_back(prev.state.deck[o].clued ? 1 : 0);
  }
  if (target_slot - 1 >= 0 &&
      target_slot - 1 < static_cast<int>(wc.receiver_hand.size())) {
    pend.target_order = wc.receiver_hand[target_slot - 1];
    pend.target_was_clued = prev.state.deck[pend.target_order].clued;
  }
  if (pend.target_order >= 0) game.pending_dc_elim = std::move(pend);
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
      defer_receiver_chuck_elim(prev, game, wc, target_slot,
                                Game::PendingDcElim::Kind::PlayDc);
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
      // DEFERRED for the same reason the colour branch above defers
      // `elim_play_dc` -- the receiver is called to CHUCK, and a chuck on an
      // inverted suit is a play. Replay 1966710 T6.
      defer_receiver_chuck_elim(prev, game, wc, target_slot,
                                Game::PendingDcElim::Kind::DcDc);
    }
  }
  game.with_move(DiscardInterp::NONE);
  return false;
}

}  // namespace hanabi::reactor0
