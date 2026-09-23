#include "hanabi/conventions/tiiah/interpret_clue.h"

#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::tiiah {

namespace {

// Does this seat hold a KNOWN PLAY? CONVENTION.md §1c: a card stamped
// CALLED_TO_PLAY whose inference still contains a playable identity, or a card
// whose empathy is entirely playable identities. Read from `common`, so every
// seat answers it the same way.
//
// This is the predicate the reverse-reactive dispatch keys on, and v16.0.0 uses
// it only to recognise a clue it cannot yet read.
bool has_known_play(const Game& game, int player) {
  const State& s = game.state;
  for (int o : s.hands[player]) {
    const IdentitySet live = game.common.thoughts[o].possibilities();
    if (!live.non_empty()) continue;
    if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) {
      if (live.exists([&s](Identity i) { return s.is_playable(i); })) return true;
      continue;
    }
    if (live.forall([&s](Identity i) { return s.is_playable(i); })) return true;
  }
  return false;
}

}  // namespace

std::optional<ClueInterp> interpret_clue(const Game& prev, Game& game,
                                         const ClueAction& action) {
  hanabi::instr::ScopedTimer st("tiiah.interpret_clue");
  hanabi::logging::LogScope ls(
      "tiiah.interpret_clue",
      {{"giver", action.giver}, {"target", action.target}});
  const State& state = game.state;

  // The `emptyClues` table option lets a clue touching nothing be given, and
  // such a clue says nothing. No TIIAH variant is a BLIND family — every one of
  // the 44 carries `throwItInAHole` and no other behavioural flag — so there is
  // no blind arm to write here, unlike reactor0's dispatcher.
  if (state.options.empty_clues && action.list_.empty()) {
    return ClueInterp::USELESS;
  }

  const int bob = state.next_player_index(action.giver);
  const int cathy = state.next_player_index(bob);

  // REVERSE REACTIVE (CONVENTION.md §1c) — specified, not yet implemented.
  //
  // When Bob holds a known play and Cathy does not, a clue to BOB is reactive
  // with CATHY reacting and Bob receiving, and a clue to Cathy is stable. That
  // is the reverse of reactor0's positional dispatch, and it is the shape the
  // bucket encoding and superposition are built on.
  //
  // Reading it wrongly is worse than not reading it: the clue would install a
  // reactor0 reaction under TIIAH meanings and call a partner onto a card
  // nobody named. So it returns nullopt — the caller stamps MISTAKE, no waiting
  // connection is installed and nothing is stamped.
  //
  // Asked of PREV, the position before the clue — as reactor0 asks
  // `clue_is_reactive`. Asking the post-clue game instead makes every play clue
  // to Bob answer "Bob has a known play", because the clue itself just gave him
  // one, and the dispatch would eat its own tail.
  if (cathy != action.giver && action.target == bob &&
      has_known_play(prev, bob) && !has_known_play(prev, cathy)) {
    hanabi::logging::log_branch("tiiah.reactive_unimplemented",
                                {{"bob", bob}, {"cathy", cathy}});
    return std::nullopt;
  }

  // STABLE (CONVENTION.md §1b) — reactor0's ladders, unchanged. They are
  // exported precisely so a sibling convention can share them rather than fork
  // a copy that drifts.
  const bool stall_ctx = prev.common.obvious_locked(prev, action.giver) ||
                         game.in_endgame() || prev.state.clue_tokens == 8;
  return action.clue.kind == ClueKind::COLOUR
             ? reactor0::stable_colour(prev, game, action, stall_ctx)
             : reactor0::stable_rank(prev, game, action, stall_ctx);
}

}  // namespace hanabi::tiiah
