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

// Drop a card's call: clear the status, urgency and signal turn -- and NOTHING
// else. The inference the call installed stays on the card.
//
// Until v8.0.0 this reverted `inferred` to `old_inferred`, the `check_missed`
// idiom. Under the static-inferred rule that is exactly backwards: a call is a
// SIGNAL, which can come and go, while an inference is PERMANENT. Withdrawing
// the signal must not withdraw what the team learned.
//
// Replay 1967558 is the cost of the old behaviour. yagami_black's slot 4 was
// stamped CALLED_TO_PLAY and narrowed to {p1}; will-bot69 then played the other
// p1, so rule 3 erased the dead call -- and took `{p1}` with it, restoring all
// five purples. yagami_black no longer knew the card was trash, so
// `has_no_safe_action` was true, priority 3 fired, and will-bot67 spent a clue
// on Bob's chop instead of playing its own CTP.
//
// `NoteMark::RESET` carries the withdrawal to the notes. The `[reset]` segment
// is driven by the CTP/CTD -> NONE status transition, which still happens here,
// but marking it explicitly keeps the two paths that drop a call (this one and
// the reactive bluff, which never stamped at all) saying the same thing.
void erase_call(Game& game, int order) {
  int turn = game.state.turn_count;
  game.with_meta(order, [turn](ConvData& m) {
    m = m.cleared().reason(turn);
    m.note_mark = NoteMark::RESET;
    m.note_mark_turn = turn;
  });
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

  // Which KIND of call is doing the erasing. `urgent` is the discriminator the
  // rest of the convention already uses -- `calls_of` routes an urgent CTP to
  // `reacter_ctp` and the rest to `receiver_ctp` (calls.cpp) -- so it is read
  // here rather than a second flag being invented.
  const bool newest_is_reacter = game.meta[hand[newest_call]].urgent;

  // Everything in a newer slot was called earlier, and a later clue would
  // not have pointed past a card still playable — so those calls are dead.
  //
  // ASYMMETRIC, and deliberately so.
  //
  //   * A RECEIVER call erases both kinds to its left. Landing to the right of
  //     a standing reacter call means leftmost targeting has left that reacter
  //     card unactionable, so the urgent call must go with it.
  //   * A REACTER call erases only other REACTER calls. It is actioned by the
  //     urgent scan on the holder's very next turn and never joins the receiver
  //     deque `calls_of` builds, so it has no standing to retire a receiver
  //     call -- the two orderings are maintained against different consumers.
  //
  // v10.12.0, replay 1974512 T8. will-bot67 held a receiver-CTP on slot 4
  // (order 11, the p1, stamped when will-bot69 reacted at num 1). At num 7
  // will-bot69's R3 clue made will-bot67 the reacter and stamped an urgent CTP
  // on slot 5 -- an OLDER slot -- which became `newest_call` and erased the
  // receiver call. It was never recovered: order 11 read NONE for the rest of
  // the game, so at T12 the bot discarded its chop with a playable p1 in hand.
  for (int i = 0; i < newest_call; ++i) {
    int o = hand[i];
    if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    if (newest_is_reacter && !game.meta[o].urgent) continue;
    erase_call(game, o);
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
