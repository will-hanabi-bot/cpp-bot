#include "hanabi/conventions/reactor0/call_invariants.h"

#include <cstddef>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"

namespace hanabi::reactor0 {

namespace {

// Stamp recency. A called card with no signal turn was never stamped by a
// clue we track, so it sorts oldest.
int stamped_at(const Game& game, int order) {
  auto st = game.meta[order].signal_turn;
  return st ? *st : -1;
}

// Drop a card's call: revert the narrowing the call installed (the
// `check_missed` idiom, src/basics/game.cpp:105-116) and clear the status,
// urgency and signal turn. Tolerant of a missing `old_inferred` — not every
// path that stamps a call records one.
void erase_call(Game& game, int order) {
  game.with_thought(order, [](const Thought& t) {
    Thought out = t;
    if (t.old_inferred) {
      out.inferred = *t.old_inferred;
      out.old_inferred = std::nullopt;
    }
    out.info_lock = std::nullopt;
    return out;
  });
  int turn = game.state.turn_count;
  game.with_meta(order, [turn](ConvData& m) { m = m.cleared().reason(turn); });
}

// Rule 1: CTP cards run newest slot -> oldest in play order.
void enforce_play_order(Game& game, const std::vector<int>& hand) {
  // The most recently stamped call. On a tie (two stamps in the same turn)
  // keep the NEWEST slot, so simultaneous calls never erase each other.
  int newest_call = -1;
  int newest_call_turn = -1;
  for (size_t i = 0; i < hand.size(); ++i) {
    int o = hand[i];
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    int turn = stamped_at(game, o);
    if (newest_call < 0 || turn > newest_call_turn) {
      newest_call = static_cast<int>(i);
      newest_call_turn = turn;
    }
  }
  if (newest_call <= 0) return;  // no call, or already on the newest slot

  // Everything in a newer slot was called earlier, and a later clue would
  // not have pointed past a card still playable — so those calls are dead.
  for (int i = 0; i < newest_call; ++i) {
    int o = hand[i];
    if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) erase_call(game, o);
  }
}

// Rule 2: at most one CTD per hand; the most recent call wins.
void enforce_single_discard_call(Game& game, const std::vector<int>& hand) {
  int keep = -1;
  int keep_turn = -1;
  int count = 0;
  for (size_t i = 0; i < hand.size(); ++i) {
    int o = hand[i];
    if (game.meta[o].status != CardStatus::CALLED_TO_DISCARD) continue;
    ++count;
    int turn = stamped_at(game, o);
    // Ties keep the newest slot, matching the play-order tiebreak.
    if (keep < 0 || turn > keep_turn) {
      keep = static_cast<int>(i);
      keep_turn = turn;
    }
  }
  if (count < 2) return;

  for (size_t i = 0; i < hand.size(); ++i) {
    int o = hand[i];
    if (static_cast<int>(i) == keep) continue;
    if (game.meta[o].status == CardStatus::CALLED_TO_DISCARD) erase_call(game, o);
  }
}

// Rule 3: a call is only as good as the card. Once COMMON knowledge leaves the
// stamped button with no identity it handles correctly, the call is dead and
// every seat drops it -- which also takes the card out of the reacter-CTP and
// receiver-CTP structures, since those are derived from these stamps.
//
// Replay 1966653: yagami was called to play a Red 2 and never did, while the
// other copy went down; by T17 the red stack was at 3, so the card was globally
// known to be unplayable and the standing CTP would have walked him into a
// second strike.
//
// Tested against `pitch_candidates` rather than against playability, because a
// CTP on an INVERTED card is a pitch -- throwing it away -- and being unplayable
// is exactly what makes that call sensible. What kills the call is having no
// valid pitch left at all.
//
// Common knowledge only. The holder's own view may be narrower, but a call is a
// shared commitment and has to die for every seat at the same moment, or they
// disagree about what is still standing.
void drop_dead_play_calls(Game& game, const std::vector<int>& hand) {
  IdentitySet allowed;
  bool computed = false;
  for (int o : hand) {
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    if (!computed) {
      allowed = pitch_candidates(game.state);
      computed = true;
    }
    const Thought& t = game.common.thoughts[o];
    const IdentitySet& set = t.inferred.non_empty() ? t.inferred : t.possible;
    if (set.is_empty()) continue;
    if (set.intersect(allowed).is_empty()) erase_call(game, o);
  }
}

// Rule 4, the mirror of rule 3 for the other button: erase a CTD whose chuck
// can no longer do anything but strike.
//
// A chuck presses Discard, which on an INVERTED suit is a play attempt. So a
// CTD dies when every reading left is an inverted card that is not the next
// for its stack -- tested against `chuck_candidates`, exactly as rule 3 tests
// a CTP against `pitch_candidates`.
//
// Leaving the dead call in place is not harmless. `Game::chop`'s first pass
// returns a CTD, so a stale one silently becomes the hand's chop; and
// `requires_high_tier` counts it, so the holder stays "occupied" and every
// clue they offer is gated to HIGH for no reason. Replay 1967287: the call was
// stamped when the orange stack was on 0, o1 went trash two turns later, and
// the card was still carrying the chuck.
void drop_dead_chuck_calls(Game& game, const std::vector<int>& hand) {
  IdentitySet allowed;
  bool computed = false;
  for (int o : hand) {
    if (game.meta[o].status != CardStatus::CALLED_TO_DISCARD) continue;
    if (!computed) {
      allowed = chuck_candidates(game.state);
      computed = true;
    }
    const Thought& t = game.common.thoughts[o];
    const IdentitySet& set = t.inferred.non_empty() ? t.inferred : t.possible;
    if (set.is_empty()) continue;
    if (set.intersect(allowed).is_empty()) erase_call(game, o);
  }
}

}  // namespace

void enforce_call_invariants(Game& game) {
  for (const auto& hand : game.state.hands) {
    drop_dead_play_calls(game, hand);
    drop_dead_chuck_calls(game, hand);
    enforce_play_order(game, hand);
    enforce_single_discard_call(game, hand);
  }
}

}  // namespace hanabi::reactor0
