#include "hanabi/conventions/reactor0/decision.h"

#include <algorithm>
#include <variant>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/interpret_reaction.h"
#include "hanabi/conventions/variants/inverted.h"

namespace hanabi::reactor0 {

namespace variants = hanabi::reactor::variants;

const char* shape_name(ClueShape s) {
  switch (s) {
    case ClueShape::REACTIVE_PLAY: return "reactive_play";
    case ClueShape::REACTIVE_DISCARD: return "reactive_discard";
    case ClueShape::DOUBLE_DISCARD: return "double_discard";
    case ClueShape::REACTIVE_LOCK: return "reactive_lock";
    case ClueShape::STABLE_PLAY: return "stable_play";
    case ClueShape::STABLE_DISCARD: return "stable_discard";
    case ClueShape::TRASH_REVEAL: return "trash_reveal";
    case ClueShape::STABLE_LOCK: return "stable_lock";
    case ClueShape::OTHER: break;
  }
  return "other";
}

Outcome outcome_of(const State& s, int order, CardStatus button) {
  auto id = s.deck[order].id();
  // Our own card: the giver never designates one, and cannot judge it anyway.
  if (!id) return Outcome::NONE;
  const bool inverted = variants::is_inverted_id(s, *id);
  switch (button) {
    case CardStatus::CALLED_TO_PLAY:
      // Press Play. On an inverted suit that is a PITCH: the card goes to the
      // discard pile and regains a clue. It can never strike, whatever the id.
      if (inverted) return Outcome::DISCARD;
      return s.is_playable(*id) ? Outcome::PLAY : Outcome::STRIKE;
    case CardStatus::CALLED_TO_DISCARD:
      // Press Discard. On an inverted suit that is a CHUCK: a play attempt onto
      // the stack, which advances it or strikes.
      if (inverted) return s.is_playable(*id) ? Outcome::PLAY : Outcome::STRIKE;
      return Outcome::DISCARD;
    default:
      return Outcome::NONE;
  }
}

namespace {

// The receiver's button, given the clue kind and the button the reacter was
// told to press. Fixed by the resolution parity table in react_play /
// react_discard (reactor0/interpret_reaction.cpp:82, :88-91, :131, :136-139).
// The receiver is not stamped at clue time on the phases that matter here, so
// the reading has to come from the parity rather than from the meta.
CardStatus receiver_button(ClueKind kind, CardStatus reacter_button) {
  const bool reacter_plays = reacter_button == CardStatus::CALLED_TO_PLAY;
  if (kind == ClueKind::RANK) {
    return reacter_plays ? CardStatus::CALLED_TO_PLAY
                         : CardStatus::CALLED_TO_DISCARD;
  }
  return reacter_plays ? CardStatus::CALLED_TO_DISCARD
                       : CardStatus::CALLED_TO_PLAY;
}

ClueShape shape_of(Outcome reacter, Outcome receiver) {
  const bool a = reacter == Outcome::PLAY;
  const bool b = receiver == Outcome::PLAY;
  if (a && b) return ClueShape::REACTIVE_PLAY;
  if (a != b) return ClueShape::REACTIVE_DISCARD;
  return ClueShape::DOUBLE_DISCARD;
}

// The stable side: which order this clue designated, and how.
ClueReading read_stable(const Game& game, const Game& hypo,
                        const ClueAction& action, ClueInterp interp) {
  ClueReading r;
  const int target = action.target;

  if (interp == ClueInterp::LOCK) {
    r.shape = ClueShape::STABLE_LOCK;
    return r;
  }

  // A newly stamped CTP/CTD in the target's hand is the designation. Only
  // ADDITIONS count: enforce_call_invariants runs straight after interpretation
  // (decide.cpp:65) and erases older calls, so a raw before/after diff would
  // also report cards this clue un-designated.
  for (int o : hypo.state.hands[target]) {
    const CardStatus before = game.meta[o].status;
    const CardStatus after = hypo.meta[o].status;
    if (after == before) continue;
    if (after != CardStatus::CALLED_TO_PLAY &&
        after != CardStatus::CALLED_TO_DISCARD) {
      continue;
    }
    r.stable_subject = o;
    const Outcome out = outcome_of(game.state, o, after);
    r.shape = out == Outcome::PLAY ? ClueShape::STABLE_PLAY
                                   : ClueShape::STABLE_DISCARD;
    return r;
  }

  // A trash reveal stamps no status at all - it sets meta.trash
  // (reactor0/interpret_clue.cpp:558). That one-field diff isolates it from the
  // other REVEAL branches, none of which flag a newly touched card.
  for (int o : hypo.state.hands[target]) {
    if (!game.meta[o].trash && hypo.meta[o].trash) {
      r.shape = ClueShape::TRASH_REVEAL;
      r.stable_subject = o;
      return r;
    }
  }
  return r;
}

}  // namespace

ClueReading read_clue(const Game& game, const Game& hypo,
                      const ClueAction& action) {
  ClueReading r;
  auto move = hypo.last_move();
  if (!move || !std::holds_alternative<ClueInterp>(*move)) return r;
  const ClueInterp interp = std::get<ClueInterp>(*move);
  // A MISTAKE has no shape. Drop it before anything reads a stamp: the post-elim
  // demotion (decide.cpp:228-230) leaves real-looking CTP stamps on a hypo whose
  // interp is MISTAKE, so classifying stamps first would call it a play clue.
  if (interp == ClueInterp::MISTAKE) return r;

  const State& s = game.state;
  const int alice = s.our_player_index;
  const int bob = s.next_player_index(alice);
  if (action.target == bob) return read_stable(game, hypo, action, interp);

  // --- reactive ---------------------------------------------------------
  if (!wc_is_fresh(game, hypo, alice, action.target, bob)) return r;
  const ReactorWC& wc = hypo.waiting.front();
  const CardStatus reacter_status = hypo.meta[wc.react_order].status;

  if (predicts_reactive_lock(hypo)) {
    r.shape = ClueShape::REACTIVE_LOCK;
    r.reacter_side = {wc.react_order, reacter_status,
                      outcome_of(s, wc.react_order, reacter_status)};
    return r;
  }

  auto receive_order = predicted_receiver_order(hypo);
  if (!receive_order) return r;
  if (reacter_status != CardStatus::CALLED_TO_PLAY &&
      reacter_status != CardStatus::CALLED_TO_DISCARD) {
    return r;
  }
  r.reacter_side = {wc.react_order, reacter_status,
                    outcome_of(s, wc.react_order, reacter_status)};

  // Bob acts FIRST, so the receiver's card is judged against the stacks Bob
  // leaves behind. Without this a finesse reads as a strike - its whole point is
  // that the receiver's card is one away until the blind play lands - and a
  // chained double play (r1 then r2) reads as play-then-strike.
  State after_bob = s;
  if (r.reacter_side.outcome == Outcome::PLAY) {
    if (auto id = s.deck[wc.react_order].id()) after_bob = s.with_play(*id);
  }
  const CardStatus rb = receiver_button(action.clue.kind, reacter_status);
  r.receiver_side = {*receive_order, rb,
                     outcome_of(after_bob, *receive_order, rb)};
  r.shape = shape_of(r.reacter_side.outcome, r.receiver_side.outcome);
  return r;
}

}  // namespace hanabi::reactor0
