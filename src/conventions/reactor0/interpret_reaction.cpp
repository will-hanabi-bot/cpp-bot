#include "hanabi/conventions/reactor0/interpret_reaction.h"

#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "hanabi/conventions/variants/predicates.h"

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


// Which suit's stack grew between two states, or -1 if none did. An inverted
// CHUCK advances a stack exactly as a plain play does, which is the point: this
// asks what reached the table, not which button produced it.
int advanced_suit(const State& before, const State& after) {
  const int n = static_cast<int>(after.play_stacks.size());
  for (int si = 0; si < n; ++si) {
    if (si >= static_cast<int>(before.play_stacks.size())) continue;
    if (after.play_stacks[si] > before.play_stacks[si]) return si;
  }
  return -1;
}

// Every identity exactly one rank away from playable.
IdentitySet one_away_set(const State& s) {
  const int n = static_cast<int>(s.variant->suits.size()) * 5;
  return IdentitySet::create(
      [&s](Identity i) { return s.playable_away(i) == 1; }, n);
}

// Hold the reaction's negative inference until the receiver actions the target.
//
// Everything is read as of the REACTION: "playable" and "one away" describe the
// position the clue was given into, not whatever the stacks look like by the
// time the receiver gets round to acting.
void arm_reaction_elim(const Game& prev, Game& game, const ReactorWC& wc,
                       int target_slot) {
  if (target_slot - 1 < 0 ||
      target_slot - 1 >= static_cast<int>(wc.receiver_hand.size())) {
    return;
  }
  Game::PendingReactionElim p;
  p.active = true;
  p.receiver = wc.receiver;
  p.target_order = wc.receiver_hand[target_slot - 1];
  p.target_slot = target_slot;
  p.receiver_hand = wc.receiver_hand;
  p.playable = prev.state.playable_set;
  p.one_away = one_away_set(prev.state);
  p.reacter_suit = advanced_suit(prev.state, game.state);
  game.pending_reaction_elim = std::move(p);
}

// --- the receiver's call --------------------------------------------------

// Did the reaction advance a NON-INVERTED stack? The bluff readings key on
// this rather than on the reacter's identity, so they uniformly cover a play
// and an inverted chuck, and correctly exclude a pitch (which stacks nothing)
// and a chuck that advanced an inverted stack.
bool advanced_plain_stack(const State& old_s, const State& new_s) {
  const int n = static_cast<int>(new_s.play_stacks.size());
  for (int si = 0; si < n; ++si) {
    if (si >= static_cast<int>(old_s.play_stacks.size())) continue;
    if (new_s.play_stacks[si] <= old_s.play_stacks[si]) continue;
    if (!variants::is_inverted_id(new_s, Identity(si, 1))) return true;
  }
  return false;
}

// Every identity exactly one away from playable on a non-inverted suit.
IdentitySet one_away_plain(const State& s) {
  const int n = static_cast<int>(s.variant->suits.size()) * 5;
  return IdentitySet::create([&s](Identity i) {
    return !variants::is_inverted_id(s, i) && s.playable_away(i) == 1;
  }, n);
}

// Withdraw the signal but keep the inference. Rule 1: a call is a signal that
// can come and go, an inference is permanent. `NoteMark::RESET` is what makes
// this visible -- notes.cpp's `[reset]` fires off a CTP/CTD -> NONE status
// transition, and since v8.0.0 the receiver was never stamped in the first
// place, so there is no transition for it to see.
void drop_call(Game& game, int order) {
  int turn = game.state.turn_count;
  game.with_meta(order, [turn](ConvData& m) {
    m = m.cleared().reason(turn);
    m.note_mark = NoteMark::RESET;
    m.note_mark_turn = turn;
  });
}

// The receiver's call, made HERE -- when the reacter acts -- and not at clue
// time (CONVENTION.md 1d).
//
// The set is read against TWO states. A play has to land on the stacks the
// reacter LEAVES BEHIND, so the play arm uses `new_s`; what the receiver can
// afford to throw away was settled when the clue was given, so the throw-away
// arm uses `old_s`. reactor0 is three-player, so the reacter moves on the turn
// immediately after the clue and `prev.state` here IS the clue-time state.
//
// Replacing reactor's `target_i_play` / `target_i_discard` rather than wrapping
// them is deliberate. `target_i_discard` narrows to the non-critical ids, and
// the inverted arm of a CTD is frequently critical, so writing over its result
// would EXPAND the set -- illegal under Rule 1. `target_i_play` additionally
// pins `info_lock` to a set built the wrong way, and an `info_lock` survives
// every reset, so a wrapper could not undo it.
void stamp_receiver_call(const Game& prev, Game& game, const ReactorWC& wc,
                         int target_slot, CardStatus button, int react_order) {
  if (target_slot - 1 < 0 ||
      target_slot - 1 >= static_cast<int>(wc.receiver_hand.size())) {
    return;
  }
  const int order = wc.receiver_hand[target_slot - 1];
  const auto& cur = game.state.hands[wc.receiver];
  if (std::find(cur.begin(), cur.end(), order) == cur.end()) return;

  const State& old_s = prev.state;
  const State& new_s = game.state;
  const IdentitySet allowed = button == CardStatus::CALLED_TO_DISCARD
                                  ? receiver_ctd_set(old_s, new_s)
                                  : receiver_ctp_set(old_s, new_s);

  auto stamp = [&](int o) {
    int turn = game.state.turn_count;
    int giver = wc.giver;
    game.with_meta(o, [turn, giver, button](ConvData& m) {
      m.status = button;
      m.by = giver;
      m.focused = true;
      m = m.reason(turn).signal(turn);
    });
  };

  // The ordinary case. `narrow_thought` intersects with any inference already
  // on the card (Rule 1) and runs the escalation ladder if that empties.
  if (game.common.thoughts[order].possible.intersect(allowed).non_empty()) {
    if (game.narrow_thought(order, allowed)) stamp(order);
    return;
  }

  // Nothing the card could be is advanced by the button it was handed. If the
  // reaction stacked a plain card, the reacter did not play what the pairing
  // predicted -- a BLUFF. Plays only: an empty discard set is just a mistake.
  if (button == CardStatus::CALLED_TO_PLAY &&
      advanced_plain_stack(old_s, new_s)) {
    // Unwind to the stacks as they were and look one rank further out. The
    // receiver's card is not playable yet; it becomes playable when the card
    // ahead of it lands, so the call is DROPPED and only the inference kept.
    const IdentitySet oneaway = one_away_plain(old_s);
    if (game.common.thoughts[order].possible.intersect(oneaway).non_empty()) {
      game.narrow_thought(order, oneaway);
      drop_call(game, order);
      return;
    }
    // DUPE BLUFF. Not even a one-away reading survives, so the only account
    // left is that the reacter just played the very card the receiver was
    // holding -- the other copy. It is trash now, and the chuck list collects
    // it by the ordinary rules.
    // `react_order` is the order the reacter ACTED on, passed down from the
    // engine hook -- not `wc.react_order`, which is -1 from the receiver's own
    // seat (§1d: the receiver never runs target selection, so it never recorded
    // one). This is the seat that has to read the dupe bluff.
    if (auto played = new_s.deck[react_order].id()) {
      game.narrow_thought(order, IdentitySet::single(*played));
      drop_call(game, order);
      return;
    }
  }

  // No reading explains this. The ladder resets the card and marks `[?]`.
  game.narrow_thought(order, allowed);
}

// The parity this WC was GIVEN under. Bound at clue time so a `/set` landing
// mid-game cannot change what an already-given clue meant -- the same
// insulation `wc.rlocks` provides. Absent only for a WC built without it (a
// reactor WC resolved under reactor0), where the variant rule still answers.
bool wc_even_parity(const Game& prev, const ReactorWC& wc) {
  if (wc.even_parity) return *wc.even_parity;
  return variants::uses_even_parity(*prev.state.variant, wc.clue.kind);
}

// Resolve a reaction, keyed on the BUTTON the reacter was handed rather than on
// which engine hook fired.
//
// The two are not the same thing on an inverted suit. Pressing Play on an
// orange card is a PITCH, which reaches the engine as a discard; pressing
// Discard on one is a CHUCK, which reaches it as a play. Reading the hook
// therefore picks the wrong row of the parity table for exactly the cards the
// inverted rules exist for -- and the receiver ends up called to the opposite
// button from the one the giver chose.
//
// This was TODO 24, and it was invisible until v8.0.0 because the receiver was
// stamped at CLUE time, when the button was still known. Now that the call is
// made here, the resolution has to carry the same information the prediction
// does -- `read_clue` has always keyed on the button (decision.cpp).
//
// `prev.meta` is the right place to read it: the reacter's card has left their
// hand by now, so `game.meta` may already have been cleared.
// Which BUTTON the reacter pressed, backed out from what landed on the table.
//
// The parity row is the BUTTON, never the outcome: what matters is whether the
// reacter pitched or chucked, independently of whether an inverted card
// happened to reach its stack. A chuck that strikes was still a chuck.
//
// The stamp cannot answer this. From the RECEIVER's own seat
// `interpret_reactive` returns before any phase runs (1d: the receiver never
// runs target selection), so the reacter carries no status and `wc.react_order`
// is -1 -- the receiver learns the pairing only now, from `calc_target_slot`.
// Reading `prev.meta` would give every seat but the receiver one answer and the
// receiver the opposite, which is the POV asymmetry 1g forbids.
//
// The engine already applies the inversion for us (`Game::on_discard`,
// src/basics/game.cpp: a physical discard of an inverted card advances its
// stack). So the HOOK IS THE BUTTON, and no suit test is needed:
//
//              plain suit              inverted suit
//   Play    -> play, or a misplay      PITCH  -> discard pile
//   Discard -> discard                 CHUCK  -> onto the stack, or a strike
//
// The one ambiguity is a misplay, which arrives as a FAILED DISCARD whichever
// button produced it. There the suit does decide: a plain card can only reach a
// strike via the Play button, an inverted card only via Discard.
CardStatus reacter_button_pressed(const Game& prev, const Game& game, int order,
                                  bool hook_was_play) {
  if (hook_was_play) return CardStatus::CALLED_TO_PLAY;
  const bool failed = game.state.strikes > prev.state.strikes;
  if (!failed) return CardStatus::CALLED_TO_DISCARD;
  auto id = game.state.deck[order].id();
  const bool inverted = id && variants::is_inverted_id(game.state, *id);
  return inverted ? CardStatus::CALLED_TO_DISCARD   // a chuck that struck
                  : CardStatus::CALLED_TO_PLAY;     // a blind play that struck
}

void resolve_reaction(const Game& prev, Game& game, const ReactorWC& wc,
                      int target_slot, CardStatus reacter_button,
                      int react_order) {
  const CardStatus button =
      receiver_button(wc_even_parity(prev, wc), reacter_button);

  if (button == CardStatus::CALLED_TO_PLAY) {
    stamp_receiver_call(prev, game, wc, target_slot, button, react_order);
    // The negative inference waits for the receiver -- see `arm_reaction_elim`.
    arm_reaction_elim(prev, game, wc, target_slot);
    return;
  }

  if (is_lock_target(wc, target_slot)) {
    reactive_lock(game, wc);
    return;
  }
  // `stamp_receiver_call` builds the chuck reading itself, so the separate
  // `narrow_to_stamped_button` that used to follow `target_i_discard` is gone --
  // it existed because reactor's shared helper only removes `critical_set`,
  // which left the promise far too wide (replay 1967287: {o1,o2,o3,o4} instead
  // of {o1}, and the chuck struck once o1 went trash).
  stamp_receiver_call(prev, game, wc, target_slot, button, react_order);
  arm_reaction_elim(prev, game, wc, target_slot);
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
  // A play on the table is a CHUCK when the button was Discard, so the button
  // -- not this hook -- decides the parity row. `wc.all_plays` is deliberately
  // not consulted (reactor0 never sets it); reading it here is what made
  // resolution contradict clue-time selection.
  resolve_reaction(prev, game, wc, target_slot,
                   reacter_button_pressed(prev, game, order, /*hook_was_play=*/true),
                   order);
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
  // A discard on the table is a PITCH when the button was Play; see
  // `resolve_reaction`.
  resolve_reaction(prev, game, wc, target_slot,
                   reacter_button_pressed(prev, game, order, /*hook_was_play=*/false),
                   order);
  game.with_move(DiscardInterp::NONE);
  return false;
}

}  // namespace hanabi::reactor0
