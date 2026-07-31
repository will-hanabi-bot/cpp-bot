#include "hanabi/conventions/reactor0/call_invariants.h"

#include <cstddef>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"

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

}  // namespace

void enforce_call_invariants(Game& game) {
  for (const auto& hand : game.state.hands) {
    enforce_play_order(game, hand);
    enforce_single_discard_call(game, hand);
  }
}

}  // namespace hanabi::reactor0
